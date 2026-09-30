"""Capture ordered PPC checkpoints from a Dolphin GDB session.

This is a validation tool, not a native boot implementation. A synthetic
startup-only disc gives an HLE handoff oracle, never retail IPL parity.
Paths and the expected disc digest are supplied by the caller.
"""

import argparse
import hashlib
import json
import socket
import subprocess
import time
from pathlib import Path


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


class RSP:
    def __init__(self, sock):
        self.sock = sock
        self.sock.settimeout(20)

    def receive_exact(self, size):
        data = bytearray()
        while len(data) < size:
            part = self.sock.recv(size - len(data))
            if not part:
                raise EOFError("RSP disconnected in fixed-length field")
            data += part
        return bytes(data)

    def packet(self):
        while True:
            start = self.sock.recv(1)
            if not start:
                raise EOFError("RSP disconnected before packet")
            if start == b"$":
                break
        data = bytearray()
        while True:
            part = self.sock.recv(1)
            if not part:
                raise EOFError("RSP disconnected")
            if part == b"#":
                break
            data += part
        check = self.receive_exact(2)
        if int(check, 16) != sum(data) & 0xFF:
            self.sock.sendall(b"-")
            raise ValueError("RSP checksum")
        self.sock.sendall(b"+")
        return data.decode("ascii")

    def send(self, payload, reply=True):
        raw = payload.encode("ascii")
        self.sock.sendall(b"$" + raw + b"#" + f"{sum(raw)&255:02x}".encode())
        if self.sock.recv(1) != b"+":
            raise ValueError("RSP not acknowledged")
        return self.packet() if reply else None

    def memory(self, address, size):
        chunks = []
        for offset in range(0, size, 0x100):
            length = min(0x100, size - offset)
            response = self.send(f"m{address+offset:x},{length:x}")
            if len(response) == 3 and response.startswith("E"):
                raise ValueError(f"RSP memory read 0x{address+offset:08x}: {response}")
            data = bytes.fromhex(response)
            if len(data) != length:
                raise ValueError("RSP memory response length mismatch")
            chunks.append(data)
        return b"".join(chunks)

    def snapshot(self):
        names = {64:"pc",65:"msr",66:"cr",67:"lr",68:"ctr",69:"xer",70:"fpscr",
                 119:"hid0",120:"hid1",138:"l2cr"}
        state = {name:self.send(f"p{number:x}") for number,name in names.items()}
        state["gpr"] = self.send("g")
        sp = int(state["gpr"][8:16], 16)
        state["stack_window_address"] = f"{sp-16:08x}"
        state["stack_window"] = self.memory(sp-16, 32).hex()
        state["fpr_source"] = self.memory(0x805F1F30,16).hex()
        state["lowmem_28"] = self.memory(0x80000028,4).hex()
        state["fst_base"] = self.memory(0x80000038,4).hex()
        return state


def main():
    parser = argparse.ArgumentParser()
    for key in ("dolphin", "disc", "user_dir", "out"):
        parser.add_argument(key, type=Path)
    parser.add_argument("--expect-disc-sha256", required=True,
                        help="Full disc SHA-256, to pin the exact reference input")
    parser.add_argument("--port", type=int, default=21619)
    parser.add_argument("--checkpoints", nargs="*", default=[],
                        help="Ordered hexadecimal PCs to stop at after entry")
    args = parser.parse_args()
    disc_sha256 = sha256(args.disc)
    if disc_sha256.lower() != args.expect_disc_sha256.lower():
        raise ValueError("reference disc SHA-256 mismatch")
    dolphin_sha256 = sha256(args.dolphin)
    args.user_dir.mkdir(parents=True, exist_ok=True)
    command = [str(args.dolphin), "-b", "-u", str(args.user_dir),
               "-C", f"Dolphin.General.GDBPort={args.port}",
               "-C", "Dolphin.Core.CPUCore=0",
               "-C", "Dolphin.Interface.DebugModeEnabled=True", "-e", str(args.disc)]
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = subprocess.SW_HIDE
    process = subprocess.Popen(command, creationflags=subprocess.CREATE_NO_WINDOW,
                               startupinfo=startup, cwd=args.dolphin.parent)
    try:
        deadline = time.monotonic() + 35
        sock = None
        while time.monotonic() < deadline:
            if process.poll() is not None:
                raise RuntimeError(f"Dolphin exited early: {process.returncode}")
            try:
                sock = socket.create_connection(("127.0.0.1", args.port), timeout=1)
                break
            except OSError:
                time.sleep(0.1)
        if sock is None:
            raise TimeoutError("GDB port unavailable")
        with sock:
            rsp = RSP(sock)
            state = rsp.snapshot()
            if state["pc"].lower() != "80003154":
                raise RuntimeError(f"unexpected first PC {state['pc']}")
            snapshots = [state]
            for checkpoint in args.checkpoints:
                address = int(checkpoint, 16)
                if rsp.send(f"Z0,{address:x},4") != "OK":
                    raise RuntimeError(f"breakpoint refused at 0x{address:08x}")
                rsp.send("c", reply=False)
                stop = rsp.packet()
                state = rsp.snapshot()
                if state["pc"].lower() != f"{address:08x}":
                    raise RuntimeError(f"first stop at {state['pc']} rather than 0x{address:08x}: {stop}")
                if rsp.send(f"z0,{address:x},4") != "OK":
                    raise RuntimeError(f"breakpoint removal refused at 0x{address:08x}")
                snapshots.append(state)
                print("checkpoint", state["pc"], "msr", state["msr"],
                      "fpscr", state["fpscr"], "FPRsource", state["fpr_source"], flush=True)
            report = {"oracle_kind": "Dolphin HLE startup; retail IPL unproven",
                      "disc_sha256": disc_sha256,
                      "dolphin_sha256": dolphin_sha256,
                      "checkpoints": snapshots}
            args.out.write_text(json.dumps(report,indent=2)+"\n",encoding="utf-8")
            print("entry",snapshots[0]["pc"],"msr",snapshots[0]["msr"],
                  "M",snapshots[0]["lowmem_28"],"FST",snapshots[0]["fst_base"],
                  "FPRsource",snapshots[0]["fpr_source"])
    finally:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()


if __name__ == "__main__":
    main()

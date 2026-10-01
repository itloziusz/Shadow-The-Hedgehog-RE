"""Measure the ICFI/GQR/FPR boundary with a read-only extended HLE oracle.

Optional source-word and initial HID0 experiments are explicitly synthetic.
Pre-entry and older mid-chain source experiments are labelled separately.
No DOL byte or instruction is patched. Original and controlled entry are retained.
All generated files and the private oracle user directory stay in build/.
"""

import argparse
import json
import os
from pathlib import Path
import socket
import subprocess
import time

from capture_dolphin_rsp import RSP, sha256


DISC_SHA = "a8a526ef7006a2d2a8d41f536ef30b208ff197c185c05e9b2276e6e93cb41a8e"
STEPS = [(0x803725FC, False), (0x80372600, True),
         (0x80371730, False), (0x80371734, True),
         (0x80371758, False), (0x80003414, False),
         (0x80370CDC, False), (0x80370CF0, False), (0x80370CFC, False),
         (0x80370D00, True), (0x80370D7C, False),
         (0x80370D80, True), (0x80370D84, True),
         (0x80370DFC, False), (0x80003418, False), (0x80372838, False),
         (0x80372860, False), (0x80372880, False), (0x80372894, False)]


def snapshot(rsp, extended):
    state = rsp.snapshot()
    state["ps0"] = [rsp.send(f"p{i:x}") for i in range(32, 64)]
    state["paired_stack_bytes"] = rsp.memory(0x8060C5E8, 16).hex()
    state["instruction_word"] = rsp.memory(int(state["pc"], 16), 4).hex()
    if extended:
        state["ps1"] = [rsp.send(f"p{i:x}") for i in range(143, 175)]
        state["gqr"] = [rsp.send(f"p{i:x}") for i in range(175, 183)]
        state["hid2"] = rsp.send("pb7")
        state["icache_valid"] = "".join(rsp.send(f"p{i:x}") for i in range(184, 216))
        state["icache_plru"] = "".join(rsp.send(f"p{i:x}") for i in range(216, 248))
        state["icache_disabled"] = rsp.send("pf8")
        state["architectural_xer"] = rsp.send("pf9")
        if any(len(word) != 16 for word in state["ps1"]) or any(
                len(word) != 8 for word in state["gqr"]):
            raise ValueError("extended GDB read map absent or malformed")
    return state


def run(args):
    build_root = Path(__file__).resolve().parents[3] / "build"
    for path in (args.user_dir, args.out):
        if not path.resolve().is_relative_to(build_root.resolve()):
            raise ValueError("capture outputs must stay under repository build/")
    if args.user_dir.exists():
        raise ValueError("use a fresh private oracle user directory")
    if sha256(args.disc) != DISC_SHA:
        raise ValueError("synthetic startup-disc digest mismatch")
    manifest = None
    if not args.stock:
        manifest = json.loads((args.dolphin.parent / "manifest.json").read_text())
        if sha256(args.dolphin) != manifest["instrumented_executable_sha256"]:
            raise ValueError("instrumented oracle digest mismatch")
    args.user_dir.mkdir(parents=True)
    (args.user_dir / "Temp").mkdir()
    args.out.parent.mkdir(parents=True, exist_ok=True)
    command = [str(args.dolphin), "-b", "-u", str(args.user_dir),
               "-C", f"Dolphin.General.GDBPort={args.port}",
               "-C", "Dolphin.Core.CPUCore=0", "-C", "Dolphin.Interface.DebugModeEnabled=True",
               "-e", str(args.disc)]
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = subprocess.SW_HIDE
    process = subprocess.Popen(command, creationflags=subprocess.CREATE_NO_WINDOW,
                               startupinfo=startup, cwd=args.dolphin.parent,
                               env=dict(os.environ, TMP=str(args.user_dir / "Temp"),
                                        TEMP=str(args.user_dir / "Temp")))
    try:
        deadline = time.monotonic() + 35
        sock = None
        while time.monotonic() < deadline:
            if process.poll() is not None:
                raise RuntimeError(f"oracle exited early: {process.returncode}")
            try:
                sock = socket.create_connection(("127.0.0.1", args.port), timeout=1)
                break
            except OSError:
                time.sleep(0.1)
        if sock is None:
            raise TimeoutError("GDB port unavailable")
        with sock:
            rsp = RSP(sock)
            states = [snapshot(rsp, not args.stock)]
            if states[0]["pc"] != "80003154":
                raise ValueError("unexpected entry PC")
            original_entry = states[0]
            if args.initial_hid0 or args.entry_source_bits:
                if args.initial_hid0:
                    if len(args.initial_hid0) != 8:
                        raise ValueError("initial-hid0 must contain exactly four bytes")
                    bytes.fromhex(args.initial_hid0)
                    if rsp.send(f"P77={args.initial_hid0}") != "OK":
                        raise ValueError("controlled pre-entry HID0 experiment refused")
                if args.entry_source_bits:
                    if len(args.entry_source_bits) != 32:
                        raise ValueError("entry-source-bits must contain exactly 16 bytes")
                    bytes.fromhex(args.entry_source_bits)
                    if rsp.send(f"M805f1f30,10:{args.entry_source_bits}") != "OK":
                        raise ValueError("controlled pre-entry BSS experiment refused")
                states[0] = snapshot(rsp, not args.stock)
                if args.initial_hid0 and states[0]["hid0"].lower() != args.initial_hid0.lower():
                    raise ValueError("pre-entry HID0 experiment readback differs")
                if args.entry_source_bits and states[0]["fpr_source"].lower() != args.entry_source_bits.lower():
                    raise ValueError("pre-entry BSS experiment readback differs")
            for address, step in STEPS:
                if step:
                    rsp.send("s", reply=False)
                else:
                    if rsp.send(f"Z0,{address:x},4") != "OK":
                        raise ValueError("breakpoint refused")
                    rsp.send("c", reply=False)
                stop = rsp.packet()
                state = snapshot(rsp, not args.stock)
                if state["pc"].lower() != f"{address:08x}":
                    raise ValueError(f"unexpected stop {state['pc']} expected {address:08x}: {stop}")
                if not step and rsp.send(f"z0,{address:x},4") != "OK":
                    raise ValueError("breakpoint removal refused")
                if address == 0x80370CDC and (args.finite_sources or args.source_bits):
                    data = args.source_bits or "400a0000000000003fc00000c0100000"  # 3.25, 1.5, -2.25
                    if len(data) != 32:
                        raise ValueError("source-bits must contain exactly 16 bytes")
                    bytes.fromhex(data)
                    if rsp.send(f"M805f1f30,10:{data}") != "OK":
                        raise ValueError("controlled live-source experiment refused")
                    state["controlled_bss_write"] = data
                    state["fpr_source_after_controlled_write"] = rsp.memory(0x805F1F30,16).hex()
                states.append(state)
                print("checkpoint", state["pc"], "HID0", state["hid0"],
                      "PS0f0", state["ps0"][0],
                      "PS1f0", state.get("ps1", ["unexposed"])[0], flush=True)
            report = {"oracle_kind": "synthetic startup-only Dolphin HLE interpreter; not retail hardware",
                      "disc_sha256": DISC_SHA, "dolphin_sha256": sha256(args.dolphin),
                      "instrumentation_manifest": manifest,
                      "controlled_live_bss_source_experiment": bool(args.finite_sources or args.source_bits),
                      "original_unmodified_entry": original_entry,
                      "controlled_initial_hid0": args.initial_hid0,
                      "controlled_entry_source_bits": args.entry_source_bits,
                      "checkpoints": states}
            args.out.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    finally:
        process.terminate()
        try:
            process.wait(timeout=5)
        except subprocess.TimeoutExpired:
            process.kill()
            process.wait(timeout=5)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ("dolphin", "disc", "user_dir", "out"):
        parser.add_argument(key, type=lambda p: Path(p).resolve())
    parser.add_argument("--port", type=int, default=21621)
    parser.add_argument("--stock", action="store_true", help="compare original exposed fields")
    parser.add_argument("--finite-sources", action="store_true")
    parser.add_argument("--source-bits", help="controlled 16-byte BSS source, hexadecimal")
    parser.add_argument("--initial-hid0", help="controlled pre-entry HID0 experiment; not retail state")
    parser.add_argument("--entry-source-bits", help="controlled 16-byte BSS input at entry, hexadecimal")
    run(parser.parse_args())

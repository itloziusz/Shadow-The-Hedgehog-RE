"""Controlled L2CR experiments from entry, plus bounded poll falsification.

Uses the existing read-only register export. L2CR input changes occur before
80003154; the optional second-poll perturbation is explicitly NOT chain parity.
No code is patched. Outputs and private process files stay in build/.
"""
import argparse
import json
import os
from pathlib import Path
import socket
import subprocess
import time

from capture_boot_machine_state import DISC_SHA, STEPS, snapshot
from capture_dolphin_rsp import RSP, sha256


def run(args, extension=None):
    root = Path(__file__).resolve().parents[3] / "build"
    for path in (args.user_dir, args.out):
        if not path.resolve().is_relative_to(root.resolve()):
            raise ValueError("outputs must stay in repository build/")
    if args.user_dir.exists():
        raise ValueError("fresh private oracle directory required")
    if sha256(args.disc) != DISC_SHA:
        raise ValueError("synthetic disc digest mismatch")
    manifest = json.loads((args.dolphin.parent / "manifest.json").read_text())
    expected_exe = manifest["base_executable_sha256"] if args.stock else manifest["instrumented_executable_sha256"]
    if sha256(args.dolphin) != expected_exe:
        raise ValueError("oracle digest mismatch")
    if not 0 <= args.l2cr <= 0xFFFFFFFF:
        raise ValueError("L2CR overflow")
    args.user_dir.mkdir(parents=True)
    (args.user_dir / "Temp").mkdir()
    args.out.parent.mkdir(parents=True, exist_ok=True)
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = subprocess.SW_HIDE
    command = [str(args.dolphin), "-b", "-u", str(args.user_dir),
               "-C", f"Dolphin.General.GDBPort={args.port}",
               "-C", "Dolphin.Core.CPUCore=0", "-C", "Dolphin.Interface.DebugModeEnabled=True",
               "-e", str(args.disc)]
    process = subprocess.Popen(command, creationflags=subprocess.CREATE_NO_WINDOW,
                               startupinfo=startup, cwd=args.dolphin.parent,
                               env=dict(os.environ, TMP=str(args.user_dir / "Temp"),
                                        TEMP=str(args.user_dir / "Temp")))
    try:
        deadline = time.monotonic() + 35
        sock = None
        while time.monotonic() < deadline:
            if process.poll() is not None:
                raise RuntimeError(f"oracle exited: {process.returncode}")
            try:
                sock = socket.create_connection(("127.0.0.1", args.port), timeout=1)
                break
            except OSError:
                time.sleep(0.1)
        if sock is None:
            raise TimeoutError("GDB unavailable")
        with sock:
            rsp = RSP(sock)

            def capture():
                state = snapshot(rsp, not args.stock)
                # Fixed window covers both helpers, caller and logger; it is
                # read observation, never a zero-filled native input.
                state["l2_stack_address"] = "8060c570"
                state["l2_stack_bytes"] = rsp.memory(0x8060C570, 0x90).hex()
                if extension is not None:
                    extension.observe(rsp, state)
                return state

            original = capture()
            if original["pc"] != "80003154":
                raise ValueError("unexpected entry")
            for packet in ("P77=0011c064", f"P8a={args.l2cr:08x}"):
                if rsp.send(packet) != "OK":
                    raise ValueError("pre-entry experiment refused")
            if args.source:
                if len(args.source) != 32:
                    raise ValueError("source must be sixteen bytes")
                bytes.fromhex(args.source)
                if rsp.send(f"M805f1f30,10:{args.source}") != "OK":
                    raise ValueError("pre-entry source experiment refused")
            if extension is not None:
                extension.prepare(rsp)
            states = [capture()]
            if int(states[0]["l2cr"], 16) != args.l2cr or int(states[0]["hid0"], 16) != 0x0011C064:
                raise ValueError(f"controlled entry readback mismatch: {states[0]['l2cr']} / {states[0]['hid0']}")

            def reach(address, step=False):
                if step:
                    rsp.send("s", reply=False)
                else:
                    if rsp.send(f"Z0,{address:x},4") != "OK":
                        raise ValueError("breakpoint refused")
                    rsp.send("c", reply=False)
                stop = rsp.packet()
                state = capture()
                if int(state["pc"], 16) != address:
                    raise ValueError(f"unexpected stop {state['pc']} expected {address:08x}: {stop}")
                if not step and rsp.send(f"z0,{address:x},4") != "OK":
                    raise ValueError("breakpoint removal refused")
                states.append(state)
                print(f"checkpoint {state['pc']} L2CR={state['l2cr']} MSR={state['msr']}", flush=True)

            for address, step in STEPS:
                reach(address, step)
            reach(0x80372898)
            reach(0x803728A0)
            if not args.l2cr & 0x80000000:
                for address in (0x80372640, 0x8037266C, 0x80372678, 0x80372684):
                    reach(address)
                if args.l2cr & 1:
                    # Prove the taken edge with one instruction; never wait
                    # unboundedly for an impossible HLE status transition.
                    reach(0x80372678, True)
                else:
                    for address in (0x80372688, 0x80372690, 0x803726B4):
                        reach(address)
                    if args.second_poll_busy:
                        value = int(states[-1]["l2cr"], 16) | 1
                        if rsp.send(f"P8a={value:08x}") != "OK":
                            raise ValueError("second-poll perturbation refused")
                        state = capture()
                        state["controlled_midchain_l2cr_write"] = f"{value:08x}"
                        states[-1] = state
                    reach(0x803726C0)
                    if args.second_poll_busy:
                        reach(0x803726A8, True)
                        for address in (0x80370C8C, 0x80370CB4, 0x80370CD4, 0x803726B4):
                            reach(address)
                        reach(0x803726C0)
                    else:
                        for address in (0x803726C4, 0x803728D4, 0x803728DC,
                                        0x803728EC, 0x80370C8C, 0x80370CB4, 0x80370CD4):
                            reach(address)
            if args.l2cr & 0x80000000 or not (args.l2cr & 1 or args.second_poll_busy):
                reach(0x803728F8)
                reach(0x80372904)
                if extension is not None:
                    extension.advance(rsp, reach, states)
            report = {"oracle_kind": "controlled startup-only HLE interpreter; not physical L2 completion",
                      "disc_sha256": DISC_SHA, "dolphin_sha256": sha256(args.dolphin),
                      "instrumentation_manifest": manifest,
                      "stock_reference": args.stock,
                      "original_unmodified_entry": original,
                      "controlled_initial_hid0": "0011c064",
                      "controlled_initial_l2cr": f"{args.l2cr:08x}",
                      "controlled_entry_source_bits": args.source,
                      "controlled_midchain_l2cr": args.second_poll_busy,
                      "controlled_live_bss_source_experiment": False,
                      "checkpoints": states}
            if extension is not None:
                report["region_extension"] = extension.metadata()
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
    parser.add_argument("--port", type=int, required=True)
    parser.add_argument("--l2cr", type=lambda v: int(v, 16), required=True)
    parser.add_argument("--source", help="controlled pre-entry FPR source bytes")
    parser.add_argument("--stock", action="store_true", help="unchanged oracle, original register exports only")
    parser.add_argument("--second-poll-busy", action="store_true",
                        help="labelled mid-chain falsification, never chain validation")
    run(parser.parse_args())

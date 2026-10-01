"""Compare a connected native run from DOL entry with a controlled HLE run.

No intermediate machine state, branch decision, command acknowledgment or
expected output is supplied to C++. Entry inputs are explicitly serialized.
HLE demonstrates architectural deltas, not physical bus/cache timing.
"""

import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def validate(dol, program, capture, work):
    if hashlib.sha256(dol.read_bytes()).hexdigest() != "fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af":
        raise ValueError("PAL DOL digest mismatch")
    root = Path(__file__).resolve().parents[3] / "build"
    if not work.resolve().is_relative_to(root.resolve()):
        raise ValueError("validation outputs must stay in build/")
    report = json.loads(capture.read_text(encoding="utf-8"))
    if report["dolphin_sha256"] != report["instrumentation_manifest"]["instrumented_executable_sha256"]:
        raise ValueError("instrumentation identity mismatch")
    if report["controlled_live_bss_source_experiment"]:
        raise ValueError("mid-chain source writes cannot validate a run from entry")
    states = {int(s["pc"], 16): s for s in report["checkpoints"]}
    entry = states[0x80003154]
    if any(s["fpr_source"] != entry["fpr_source"] for s in states.values()):
        raise ValueError("live source changed after entry")
    fields = [entry[k] for k in ("msr", "hid0", "hid2", "cr", "architectural_xer", "ctr", "fpscr")]
    fields += entry["ps0"] + entry["ps1"] + entry["gqr"] + ["805f1f30"]
    fields += [entry["fpr_source"][n:n+8] for n in range(0, 32, 8)]
    work.mkdir(parents=True, exist_ok=True)
    fixture = work / (capture.stem + ".entry.txt")
    fixture.write_text("\n".join(fields) + "\n", encoding="ascii")
    run = subprocess.run([str(program.resolve()), str(dol.resolve()), str(fixture)],
                         capture_output=True, text=True, check=True)
    (work / (capture.stem + ".native.txt")).write_text(run.stdout, encoding="ascii")
    compared = memory_bytes = checkpoints = 0
    frame_writes = []
    for line in run.stdout.splitlines():
        if line.startswith("COMMITTED_STACK "):
            frame_writes.append([int(v, 16) for v in line.split()[1:]])
            continue
        if line.startswith("STOP "):
            if line != "STOP pc=0x80372894 reason=LIVE_L2CR_UNRESOLVED":
                raise ValueError("unexpected connected native stop")
            continue
        words = line.split()
        if len(words) != 115:
            raise ValueError(f"native checkpoint field count {len(words)} differs")
        pc = int(words[0], 16)
        if pc not in states:
            continue  # Native earlier checkpoints are separately fixture-gated.
        reference = states[pc]
        expected = [reference[k] for k in ("pc", "msr", "lr", "cr", "architectural_xer", "fpscr")]
        expected += [reference["gpr"][n:n+8] for n in range(0, 256, 8)]
        expected += reference["ps0"] + reference["ps1"]
        expected += [reference[k] for k in ("ctr", "hid0", "hid2")] + reference["gqr"]
        for n, (actual, observed) in enumerate(zip(words[:113], expected)):
            if int(actual, 16) != int(observed, 16):
                raise ValueError(f"first divergence PC {pc:08X} field {n}: native {actual} observed {observed}")
            compared += 1
        actual, observed = bytes.fromhex(words[113]), bytes.fromhex(reference["paired_stack_bytes"])
        for n, known in enumerate(words[114]):
            if known not in "01":
                raise ValueError("invalid native memory validity marker")
            if known == "1":
                if actual[n] != observed[n]:
                    raise ValueError(f"first stack divergence PC {pc:08X} byte {n}")
                memory_bytes += 1
        checkpoints += 1
    if checkpoints != 17:
        raise ValueError(f"reference checkpoint coverage {checkpoints}, expected 17")
    final = states[0x80372894]
    cache_entry = states[0x80372838]
    sp = int(cache_entry["gpr"][8:16], 16)
    # Independently read each destination from the reference memory windows,
    # including the new frame backchain outside the earlier paired window.
    addresses = [sp+4, sp-16, sp-4, sp-8]
    expected_writes = []
    for address in addresses:
        if address == sp+4:
            data = bytes.fromhex(final["paired_stack_bytes"])
            base = 0x8060C5E8
        else:
            data = bytes.fromhex(final["stack_window"])
            base = int(final["stack_window_address"], 16)
        offset = address-base
        if offset < 0 or offset+4 > len(data):
            raise ValueError("cache frame destination not observed")
        expected_writes.append([address, int.from_bytes(data[offset:offset+4], "big")])
    if frame_writes != expected_writes:
        raise ValueError(f"ordered cache-frame memory differs: {frame_writes} vs {expected_writes}")
    summary = {"capture_sha256": hashlib.sha256(capture.read_bytes()).hexdigest(),
               "native_executable_sha256": hashlib.sha256(program.read_bytes()).hexdigest(),
               "entry_fixture_sha256": hashlib.sha256(fixture.read_bytes()).hexdigest(),
               "checkpoints": checkpoints, "state_fields": compared, "memory_bytes": memory_bytes,
               "ordered_cache_frame_words": len(frame_writes),
               "connected_stop": "0x80372894",
               "scope": "controlled HLE deltas + documented bounded native completion; not retail/timing"}
    (work / (capture.stem + ".validation.json")).write_text(json.dumps(summary, indent=2)+"\n", encoding="utf-8")
    print(f"{capture.name}: {checkpoints} connected checkpoints; {compared} fields, {memory_bytes} known stack bytes and four committed frame words agree; stop 80372894")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ("dol", "program", "capture", "work"):
        parser.add_argument(key, type=Path)
    args = parser.parse_args()
    validate(args.dol, args.program, args.capture, args.work)

"""Compare eight native FPR checkpoints against an extended HLE capture.

Only a bounded unconnected projection is validated. Input state is serialized
from observations, not expected checkpoint constants. Work files stay in build/.
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
        raise ValueError("validation work files must stay in build/")
    report = json.loads(capture.read_text(encoding="utf-8"))
    if report["dolphin_sha256"] != report["instrumentation_manifest"]["instrumented_executable_sha256"]:
        raise ValueError("instrumentation identity mismatch")
    states = {int(s["pc"], 16): s for s in report["checkpoints"]}
    entry = states[0x80370CDC]
    source = entry.get("fpr_source_after_controlled_write", entry["fpr_source"])
    fields = [entry[k] for k in ("msr", "pc", "lr", "cr", "architectural_xer", "fpscr", "hid2")]
    fields += [entry["gqr"][0], "805f1f30"]
    fields += [entry["gpr"][i:i+8] for i in range(0, 256, 8)]
    fields += entry["ps0"] + entry["ps1"] + [source[i:i+8] for i in range(0, 32, 8)]
    work.mkdir(parents=True, exist_ok=True)
    fixture = work / (capture.stem + ".input.txt")
    fixture.write_text("\n".join(fields) + "\n", encoding="ascii")
    run = subprocess.run([str(program.resolve()), str(dol.resolve()), str(fixture)],
                         capture_output=True, text=True, check=True)
    lines = [line.split() for line in run.stdout.splitlines()]
    if len(lines) != 8:
        raise ValueError("native checkpoint count mismatch")
    compared = 0
    for words in lines:
        if len(words) != 102:
            raise ValueError("native checkpoint field count mismatch")
        reference = states[int(words[0], 16)]
        expected = [reference[k] for k in ("pc", "msr", "lr", "cr", "architectural_xer", "fpscr")]
        expected += [reference["gpr"][i:i+8] for i in range(0, 256, 8)]
        expected += reference["ps0"] + reference["ps1"]
        for n, (actual, observed) in enumerate(zip(words, expected)):
            if int(actual, 16) != int(observed, 16):
                raise ValueError(f"first divergence PC {words[0]} field {n}: native {actual} observed {observed}")
            compared += 1
    print(f"{capture.name}: 8 checkpoints; {compared} raw state fields agree (bounded HLE projection)")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ("dol", "program", "capture", "work"):
        parser.add_argument(key, type=Path)
    args = parser.parse_args()
    validate(args.dol, args.program, args.capture, args.work)

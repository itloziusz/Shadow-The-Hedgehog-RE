"""Negative coverage regression for the SHA-pinned PAL raw-note gate.

The test mutates an in-memory note only; it never writes the DOL or a fixture.
"""

import argparse
import hashlib
from pathlib import Path

import verify_binary_note as gate


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dol", type=Path)
    parser.add_argument("note", type=Path)
    args = parser.parse_args()
    dol = args.dol.read_bytes()
    if hashlib.sha256(dol).hexdigest() != gate.PAL_DOL_SHA256:
        raise ValueError("PAL DOL SHA-256 mismatch")
    note = args.note.read_text(encoding="utf-8")
    code = set().union(*(gate.address_range(span) for span in (
        "80003154:80003158", "800032B0:8000333C",
        "80003400:80003420", "80370BA8:80370BBC",
        "80371714:80371764", "803725F4:80372600")))
    if gate.verify(dol, note, code, set()) != (78, 8, 17, 0):
        raise AssertionError("original raw-prefix note no longer passes")

    # This once passed the count/byte/branch/SPR gate: remove the required
    # FP-enable OR at 0x80003404 and insert an unrelated valid DOL word.
    replacement_addr = 0x80003424
    replacement_offset = 0x424
    raw = dol[replacement_offset:replacement_offset + 4]
    replacement = (f"| `{replacement_addr:08X}` | `{replacement_offset:06X}` | "
                   f"`{raw.hex(' ').upper()}` / `{int.from_bytes(raw, 'big'):08X}` | "
                   "unrelated DOL word |")
    lines = note.splitlines()
    old = [i for i, line in enumerate(lines) if line.startswith("| `80003404` |")]
    if len(old) != 1:
        raise AssertionError("required OR row missing or duplicated")
    lines[old[0]] = replacement
    mutant = "\n".join(lines)
    if gate.verify(dol, mutant) != (78, 8, 17, 0):
        raise AssertionError("negative mutation no longer preserves old counts")
    try:
        gate.verify(dol, mutant, code, set())
    except ValueError as exc:
        if "code address coverage differs" not in str(exc):
            raise
    else:
        raise AssertionError("valid-word substitution escaped coverage gate")
    print("PASS omitted FP-enable OR rejected despite same raw/count gate totals")


if __name__ == "__main__":
    main()

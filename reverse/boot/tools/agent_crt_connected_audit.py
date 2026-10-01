"""Read-only binary audit of the exact connected CRT candidate.

Prints raw code and descriptor evidence; does not execute/interprete PPC, create
cache defaults, or establish a live native checkpoint. All file input is read
only. This intentionally decodes branch/address fields directly from words
before using the existing mnemonic decoder as a secondary presentation.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from recognizer.machine import DolImage, decode


def s16(n):
    return n - 0x10000 if n & 0x8000 else n


def fields(pc, word):
    op = word >> 26
    d, a, b = (word >> 21) & 31, (word >> 16) & 31, (word >> 11) & 31
    if op == 18:
        disp = word & 0x03fffffc
        if disp & 0x02000000:
            disp -= 0x04000000
        target = (disp if word & 2 else pc + disp) & 0xffffffff
        return f"I op18 disp{disp:+#x} AA{(word >> 1) & 1} LK{word & 1} target{target:08X}"
    if op == 16:
        disp = s16(word & 0xfffc)
        target = (disp if word & 2 else pc + disp) & 0xffffffff
        return f"B op16 BO{d} BI{a} disp{disp:+#x} AA{(word >> 1) & 1} LK{word & 1} target{target:08X}"
    if op == 21:
        return f"M op21 rs{d} ra{a} sh{b} mb{(word >> 6) & 31} me{(word >> 1) & 31} Rc{word & 1}"
    if op == 31:
        return f"X op31 d/s{d} ra{a} rb{b} XO{(word >> 1) & 1023} Rc{word & 1}"
    if op == 19:
        return f"XL op19 BO/BT{d} BI/BA{a} BB{b} XO{(word >> 1) & 1023} LK{word & 1}"
    return f"D op{op} d/s{d} ra{a} imm{word & 0xffff:04X}"


RANGES = ((0x80372908, 0x80372928), (0x8000341c, 0x80003420),
          (0x8000315c, 0x80003264), (0x80003340, 0x800033fc),
          (0x8000540c, 0x800054f0), (0x80370bf0, 0x80370c14))


def tables(image):
    copies = []
    for cursor in range(0x80005544, 0x800055c8, 12):
        src, dst, count = struct.unpack(">III", image.read(cursor, 12))
        copies.append((cursor, src, dst, count))
    zeros = []
    for cursor in range(0x800055c8, 0x800055e8, 8):
        dst, count = struct.unpack(">II", image.read(cursor, 8))
        zeros.append((cursor, dst, count))
    if len(copies) != 11 or copies[-1][1:] != (0, 0, 0):
        raise ValueError("copy terminator differs")
    if any(src != dst or count == 0 for _, src, dst, count in copies[:-1]):
        raise ValueError("identity-copy path is not proved")
    if zeros[-1][1:] != (0, 0):
        raise ValueError("zero terminator differs")
    if any((dst | count) & 3 or count < 32 for _, dst, count in zeros[:-1]):
        raise ValueError("aligned grouped-fill candidate differs")
    return copies, zeros


def check_intervals(image, zeros):
    # Evidence-driven region ownership: only full overwritten intervals can
    # accept unknown initial bytes. No native load is justified by this model.
    intervals = [(dst, dst + count) for _, dst, count in zeros[:-1]]
    assert sum(end - start for start, end in intervals) == 0x77f08
    assert all(a[1] <= b[0] for a, b in zip(intervals, intervals[1:]))
    for start, end in intervals:
        assert all(not (start < s.end and s.address < end) for s in image.sections)
        # Verify bounded, exact writes over hostile old bytes and boundary
        # sentinels. This verifies the interval collapse, not PPC execution.
        memory = bytearray(b"\xA5" * (end - start + 8))
        memory[4:-4] = b"\0" * (end - start)
        assert memory[:4] == memory[-4:] == b"\xA5" * 4
        assert not any(memory[4:-4])
    def covers(address, size):
        return any(lo <= address and address + size <= hi for lo, hi in intervals)
    assert covers(0x80586cb4, 4) and covers(0x805f1f30, 16)
    assert not covers(0x805f277c, 4)
    assert not covers(0x805fc5ec, 4)
    assert not covers(0x8060c5d0, 0x24)
    return intervals


def audit_capture(image, path, intervals):
    """Independent raw/reference checks; no native state is manufactured."""
    report = json.loads(path.read_text(encoding="utf-8"))
    if report["disc_sha256"] != "a8a526ef7006a2d2a8d41f536ef30b208ff197c185c05e9b2276e6e93cb41a8e":
        raise ValueError("reference synthetic disc differs")
    if report["dolphin_sha256"] != report["instrumentation_manifest"]["instrumented_executable_sha256"]:
        raise ValueError("reference oracle manifest differs")
    if report["controlled_midchain_l2cr"] or report["controlled_live_bss_source_experiment"]:
        raise ValueError("midchain source/control experiment is not CRT chain validation")
    extension = report["region_extension"]
    if extension["midchain_writes"] or extension["stop"] != "80003188":
        raise ValueError("unexpected region/control metadata")
    states = report["checkpoints"]
    if int(states[0]["pc"], 16) != 0x80003154 or int(states[-1]["pc"], 16) != 0x80003188:
        raise ValueError("full original-entry/BI2-stop observations required")
    descriptor = image.read(0x80005544, 0xa4)
    fills = extension["filled_ranges"]
    if [(int(f["address"], 16), int(f["size"], 16)) for f in fills] != [(a, b-a) for a, b in intervals]:
        raise ValueError("reference range order/shape differs")
    for fill in fills:
        data = bytes.fromhex(fill["bytes"])
        if len(data) != int(fill["size"], 16) or any(data):
            raise ValueError("full byte-level zero effect differs")
    seeds = extension["preentry_memory_writes"]
    for seed in seeds:
        if int(seed["readback"], 16) != int(seed["value"], 16):
            raise ValueError("claimed preentry seed has no matching readback")
    canaries = states[0]["crt_canaries"]
    if len(canaries) != 6 or any(len(bytes.fromhex(c)) != 4 for c in canaries):
        raise ValueError("six complete outside-canary observations required")
    installed = False
    completed = 0
    crt = False
    crt_reference = None
    seed_checks = 0
    for state in states:
        pc = int(state["pc"], 16)
        for key in ("pc", "msr", "cr", "architectural_xer", "fpscr", "ctr", "l2cr", "instruction_word"):
            if re.fullmatch(r"[0-9a-fA-F]{8}", state[key]) is None:
                raise ValueError(f"incomplete raw state field {key}")
        for key, count, width in (("ps0", 32, 16), ("ps1", 32, 16), ("gqr", 8, 8)):
            values = state[key]
            if len(values) != count or any(re.fullmatch(rf"[0-9a-fA-F]{{{width}}}", value) is None for value in values):
                raise ValueError(f"incomplete raw array {key}")
        if image.word(pc, text=True) != int(state["instruction_word"], 16):
            raise ValueError(f"raw captured code word differs at {pc:08X}")
        if bytes.fromhex(state["crt_descriptors"]) != descriptor:
            raise ValueError("live descriptor no longer immutable image")
        if state["crt_canaries"] != canaries:
            raise ValueError("clear escaped exact boundary")
        if len(bytes.fromhex(state["gpr"])) != 128:
            raise ValueError("truncated GPR observation")
        if pc == 0x803733c4:
            installed = True
        if pc == 0x80005424:
            completed += 1
        if "preentry_seed_words" not in state:
            raise ValueError("live seed timeline absent; recapture with current readback observer")
        words = state["preentry_seed_words"]
        if len(words) != len(seeds):
            raise ValueError("incomplete preentry/clear seed observation")
        for seed, actual in zip(seeds, words, strict=True):
            if len(bytes.fromhex(actual)) != 4:
                raise ValueError("truncated seed word observation")
            address = int(seed["address"], 16)
            expected = int(seed["value"], 16)
            if address == 0x80586cb4 and installed:
                expected = 0x803726d8
            if any(lo <= address and address+4 <= hi for lo, hi in intervals[:completed]):
                expected = 0
            if int(actual, 16) != expected:
                raise ValueError(f"seed write/read producer differs at {pc:08X}/{address:08X}")
            seed_checks += 1
        source = state["fpr_source"]
        expected_source = "00" * 16 if completed >= 2 else states[0]["fpr_source"]
        if re.fullmatch(r"[0-9a-fA-F]{32}", source) is None or source.lower() != expected_source.lower():
            raise ValueError("FPR source clear occurred at the wrong fill boundary")
        if pc == 0x8000315c:
            crt = True
            crt_reference = state
        if crt:
            for key in ("ps0", "ps1", "fpscr", "gqr", "msr", "l2cr", "ctr"):
                if state[key] != crt_reference[key]:
                    raise ValueError(f"integer CRT changed preserved {key}")
    if completed != 3:
        raise ValueError("three distinct fill returns required")
    if int(states[-1]["architectural_xer"], 16) & 0x20000000 == 0:
        raise ValueError("last raw decrement carry missing")
    print(f"PASS reference {path.name}: {len(states)} raw/descriptor/canary observations; {seed_checks} live seed checks; {sum(b-a for a,b in intervals)} full zero bytes")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dol", type=Path)
    parser.add_argument("--raw", action="store_true", help="emit Markdown raw-word rows")
    parser.add_argument("--capture", action="append", type=Path, default=[],
                        help="independently check full raw/reference CRT effect capture")
    args = parser.parse_args()
    image = DolImage(args.dol)
    copies, zeros = tables(image)
    intervals = check_intervals(image, zeros)
    n = 0
    for lo, hi in RANGES:
        for pc in range(lo, hi + 4, 4):
            word = image.word(pc, text=True)
            ins = decode(word, pc)
            if ins.m == ".word":
                raise ValueError(f"unsupported decode at {pc:08X}")
            if word >> 26 in (16, 18):
                # Independent encoded target must match decoder.
                target = int(fields(pc, word).split("target")[1], 16)
                if ins.target != target:
                    raise ValueError(f"branch decoder disagreement at {pc:08X}")
            if args.raw:
                section = image.section(pc, 4)
                offset = section.offset + pc - section.address
                raw = image.read(pc, 4).hex(" ").upper()
                print(f"| `{pc:08X}` | `{offset:06X}` | `{raw}` / `{word:08X}` | {fields(pc, word)} | `{ins.text()}` |")
            n += 1
    print(f"PASS PAL raw words {n}; copy descriptors {len(copies)}; zero descriptors {len(zeros)}")
    print("descriptor SHA256 " + hashlib.sha256(image.read(0x80005544, 0xa4)).hexdigest())
    for cursor, src, dst, count in copies:
        print(f"copy {cursor:08X} source{src:08X} dest{dst:08X} size{count:08X}")
    for cursor, dst, count in zeros:
        print(f"zero {cursor:08X} dest{dst:08X} size{count:08X}")
    for start, end in intervals:
        count = end - start
        print(f"range {start:08X}:{end:08X} groups{count >> 5:X} remain_words{(count >> 2) & 7:X} last_word{end - 4:08X} r6{end - 1:08X}")
    print("post-CRT first unknown read: 80003188 from 800000F4; DOL mapping", image.section(0x800000f4, 4))
    for capture in args.capture:
        audit_capture(image, capture, intervals)


if __name__ == "__main__":
    main()

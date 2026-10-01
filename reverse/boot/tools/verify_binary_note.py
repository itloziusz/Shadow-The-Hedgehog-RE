"""Recheck published PAL boot word tables against the original DOL bytes.

This validates evidence, not native execution or GameCube hardware behavior.
It reads a user-supplied DOL and Markdown note and writes no files.
"""

import argparse
import hashlib
import re
from pathlib import Path


PAL_DOL_SHA256 = "fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af"
ROW = re.compile(
    r"^\| `([0-9A-Fa-f]{8})` \| `([0-9A-Fa-f]{6})` \| "
    r"`([0-9A-Fa-f]{2}(?: [0-9A-Fa-f]{2}){3})` / `([0-9A-Fa-f]{8})` \|"
)
DATA_ROW = re.compile(
    r"^\| `([0-9A-Fa-f]{8})` \| `([0-9A-Fa-f]{6})` \| "
    r"`([0-9A-Fa-f]{2}(?: [0-9A-Fa-f]{2}){3})` \| `([0-9A-Fa-f]{8})` \|"
)
BRANCH = re.compile(r"→ `(bl|b|beq|bne(?: cr1,)?|blt|bgt|bdnz) (?:0x)?([0-9A-Fa-f]{8})`")
SPR = re.compile(r"\bSPR(\d+)\b")
TBR = re.compile(r"\bTBR(\d+)\b")


def be32(data, offset):
    return int.from_bytes(data[offset:offset + 4], "big")


def sections(dol):
    result = []
    for count, off_base, va_base, size_base in (
        (7, 0x00, 0x48, 0x90), (11, 0x1C, 0x64, 0xAC)
    ):
        for i in range(count):
            offset = be32(dol, off_base + i * 4)
            va = be32(dol, va_base + i * 4)
            size = be32(dol, size_base + i * 4)
            if size:
                if offset + size > len(dol):
                    raise ValueError("DOL section exceeds file")
                result.append((va, va + size, offset))
    return result


def signed_branch_target(address, word, mnemonic):
    opcode = word >> 26
    if mnemonic in ("bl", "b"):
        if opcode != 18 or (word & 1) != (mnemonic == "bl"):
            raise ValueError(f"0x{address:08X} is not encoded {mnemonic}")
        delta = word & 0x03FFFFFC
        if delta & 0x02000000:
            delta -= 0x04000000
    else:
        if opcode != 16 or (word & 1) != 0:
            raise ValueError(f"0x{address:08X} is not a conditional branch")
        bo, bi = (word >> 21) & 31, (word >> 16) & 31
        conditions = {"beq": (12, 2), "bne": (4, 2),
                      "bne cr1,": (4, 6), "blt": (12, 0), "bgt": (12, 1), "bdnz": (16, 0)}
        # BO's low bit is the static prediction hint, not a predicate input.
        # The original BI2 beq words use BO13; retain the other four BO bits.
        if (bo & ~1, bi) != conditions[mnemonic]:
            raise ValueError(f"0x{address:08X} condition fields differ")
        delta = word & 0xFFFC
        if delta & 0x8000:
            delta -= 0x10000
    if word & 2:
        raise ValueError(f"0x{address:08X} has absolute-address branch bit")
    return (address + delta) & 0xFFFFFFFF


def address_range(spec):
    match = re.fullmatch(r"([0-9A-Fa-f]{8}):([0-9A-Fa-f]{8})", spec)
    if not match:
        raise ValueError(f"invalid inclusive address range {spec!r}")
    first, last = int(match[1], 16), int(match[2], 16)
    if first > last or (first | last) & 3 or last - first > 0x100000:
        raise ValueError(f"invalid aligned address range {spec!r}")
    return set(range(first, last + 4, 4))


def verify(dol, note, expected_code=None, expected_data=None):
    mapping = sections(dol)
    seen = set()
    code_addresses, data_addresses = set(), set()
    words = branches = sprs = data_words = 0
    for line in note.splitlines():
        match = ROW.match(line)
        is_data = False
        if not match:
            match = DATA_ROW.match(line)
            if not match:
                continue
            is_data = True
        address = int(match[1], 16)
        noted_offset = int(match[2], 16)
        noted_bytes = bytes.fromhex(match[3])
        noted_word = int(match[4], 16)
        if address in seen:
            raise ValueError(f"duplicate instruction row 0x{address:08X}")
        seen.add(address)
        offsets = [offset + address - lo for lo, hi, offset in mapping
                   if lo <= address and address + 4 <= hi]
        if len(offsets) != 1 or offsets[0] != noted_offset:
            raise ValueError(f"DOL mapping differs at 0x{address:08X}")
        actual = dol[noted_offset:noted_offset + 4]
        if actual != noted_bytes or be32(dol, noted_offset) != noted_word:
            raise ValueError(f"raw bytes or BE word differ at 0x{address:08X}")
        if is_data:
            data_words += 1
            data_addresses.add(address)
            continue
        words += 1
        code_addresses.add(address)
        target = BRANCH.search(line)
        if noted_word >> 26 in (16, 18) and target is None:
            raise ValueError(f"direct branch lacks checked target at 0x{address:08X}")
        if target:
            if signed_branch_target(address, noted_word, target[1]) != int(target[2], 16):
                raise ValueError(f"branch target differs at 0x{address:08X}")
            branches += 1
        tbr = TBR.search(line)
        if tbr:
            if noted_word >> 26 != 31 or ((noted_word >> 1) & 1023) != 371 or noted_word & 1:
                raise ValueError(f"TBR opcode/reserved Rc differs at 0x{address:08X}")
            selected = ((noted_word >> 16) & 31) | (((noted_word >> 11) & 31) << 5)
            if selected not in (268,269) or selected != int(tbr[1]):
                raise ValueError(f"TBR selector differs at 0x{address:08X}")
        spr = SPR.search(line)
        if spr:
            if noted_word >> 26 != 31 or ((noted_word >> 1) & 1023) not in (339, 467):
                raise ValueError(f"SPR opcode differs at 0x{address:08X}")
            decoded = ((noted_word >> 16) & 31) | (((noted_word >> 11) & 31) << 5)
            if decoded != int(spr[1]):
                raise ValueError(f"SPR number differs at 0x{address:08X}")
            sprs += 1
    if words == 0:
        raise ValueError("no raw instruction rows found")
    if expected_code is not None and code_addresses != expected_code:
        missing = sorted(expected_code - code_addresses)
        extra = sorted(code_addresses - expected_code)
        raise ValueError(f"code address coverage differs: missing={missing[:4]} extra={extra[:4]}")
    if expected_data is not None and data_addresses != expected_data:
        missing = sorted(expected_data - data_addresses)
        extra = sorted(data_addresses - expected_data)
        raise ValueError(f"data address coverage differs: missing={missing[:4]} extra={extra[:4]}")
    return words, branches, sprs, data_words


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("dol", type=Path)
    parser.add_argument("note", type=Path)
    parser.add_argument("--expected-words", type=int, required=True,
                        help="Expected row count; a missing row must fail")
    parser.add_argument("--expected-branches", type=int)
    parser.add_argument("--expected-sprs", type=int)
    parser.add_argument("--expected-data-words", type=int, default=0)
    parser.add_argument("--required-code-range", action="append", required=True,
                        help="Inclusive aligned START:END VA range; repeat for disjoint regions")
    parser.add_argument("--required-data-range", action="append", default=[],
                        help="Inclusive aligned descriptor VA range; repeat as needed")
    args = parser.parse_args()
    dol = args.dol.read_bytes()
    if hashlib.sha256(dol).hexdigest() != PAL_DOL_SHA256:
        raise ValueError("PAL DOL SHA-256 mismatch")
    code_addresses = set().union(*(address_range(x) for x in args.required_code_range))
    data_addresses = set().union(*(address_range(x) for x in args.required_data_range))
    result = verify(dol, args.note.read_text(encoding="utf-8"),
                    code_addresses, data_addresses)
    if result[0] != args.expected_words:
        raise ValueError(f"instruction row count {result[0]} != {args.expected_words}")
    if args.expected_branches is not None and result[1] != args.expected_branches:
        raise ValueError(f"branch row count {result[1]} != {args.expected_branches}")
    if args.expected_sprs is not None and result[2] != args.expected_sprs:
        raise ValueError(f"SPR row count {result[2]} != {args.expected_sprs}")
    if result[3] != args.expected_data_words:
        raise ValueError(f"data row count {result[3]} != {args.expected_data_words}")
    print(f"PASS raw words={result[0]} branches={result[1]} SPRs={result[2]} data={result[3]}")


if __name__ == "__main__":
    main()

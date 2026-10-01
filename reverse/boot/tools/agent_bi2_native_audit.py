"""Independent read-only BI2/OS-clock binary and portable-bit audit.

Exact DOL bytes are primary. Clock matches are STRUCTURAL_MATCH only; this
tool does not manufacture ticks, execute PPC, or promote the boot frontier.
Portable experiments isolate address safety versus modulo data-word math.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from recognizer.machine import DolImage, decode


RANGES = ((0x80003188, 0x80003264), (0x80003140, 0x80003148),
          (0x80370bf0, 0x80370c14), (0x80370e68, 0x80370ea4),
          (0x80379648, 0x80379668), (0x80379628, 0x8037963c),
          (0x8037611c, 0x8037612c))
ZEROS = ((0x8056fe00, 0x805e4500), (0x805ef020, 0x805f277c),
         (0x805fc540, 0x805fc5ec))


def branch_target(pc, word):
    op = word >> 26
    if op == 18:
        disp = word & 0x03fffffc
        if disp & 0x02000000:
            disp -= 0x04000000
    elif op == 16:
        disp = word & 0xfffc
        if disp & 0x8000:
            disp -= 0x10000
    else:
        raise ValueError("not a direct branch")
    return (disp if word & 2 else pc + disp) & 0xffffffff


def tbr(word):
    if word >> 26 != 31 or (word >> 1) & 1023 != 371 or word & 1:
        return None
    return ((word >> 16) & 31) | ((word >> 6) & 0x3e0)


def stable_sampler(words, pc):
    """Recover complete six-word register/CR/CFG fingerprint, not purpose."""
    if len(words) != 6 or [tbr(w) for w in words[:3]] != [269, 268, 269]:
        return None
    high, low, high2 = [(word >> 21) & 31 for word in words[:3]]
    if len({high, low, high2}) != 3:
        return None  # Overwriting either sample invalidates the dependency.
    compare, branch, ret = words[3:]
    if compare >> 26 != 31 or (compare >> 1) & 1023 != 0 or compare & 1:
        return None
    # cmpw L=0, reserved bit=0. Compare order is immaterial to equality.
    if compare & 0x00600000:
        return None
    field = (compare >> 23) & 7
    if {((compare >> 16) & 31), ((compare >> 11) & 31)} != {high, high2}:
        return None
    if branch >> 26 != 16 or ((branch >> 21) & 31) not in (4, 5):
        return None
    if (branch >> 16) & 31 != 4 * field + 2 or branch & 3:
        return None
    if branch_target(pc+16, branch) != pc or ret != 0x4e800020:
        return None
    return {"status": "STRUCTURAL_MATCH", "high_register": high,
            "low_register": low, "second_high_register": high2,
            "cr_field": field, "retry_target": f"{pc:08X}",
            "producer": "UNKNOWN live time base; no tick model inferred"}


def owned_bi2_extent(image, base, size=0x2000):
    if not 0 <= base <= 0xffffffff or base & 3 or size != 0x2000:
        raise ValueError("unproven BI2 alignment/extent")
    end = base + size  # wider arithmetic; never pointer wrap.
    if base < 0x80003100 or end > 0x81800000:
        raise ValueError("BI2 outside the bounded ordinary-RAM input profile")
    forbidden = [(s.address, s.end) for s in image.sections]
    forbidden += list(ZEROS) + [(0x8060c570, 0x8060c600)]
    if any(base < hi and lo < end for lo, hi in forbidden):
        raise ValueError("BI2 aliases existing immutable/zero/stack owner")
    return end


def load32(blob, offset):
    if offset & 3 or not 0 <= offset <= len(blob)-4:
        raise ValueError("unknown/unaligned word outside owned BI2")
    return int.from_bytes(blob[offset:offset+4], "big")


def relocate(image, base, original):
    """Proven word transform candidate; no full boot/CPU interpretation."""
    owned_bi2_extent(image, base, len(original))
    blob = bytearray(original)
    offset = load32(blob, 8)
    if offset == 0:
        return blob, 0, 0, 0, None
    count = load32(blob, offset)
    if count == 0:
        return blob, 0, 0, base+offset, None
    start = offset+4
    end = start+4*count  # never truncate a count or pointer calculation.
    if end > len(blob):
        raise ValueError("unproven relocation array extent")
    for cursor in range(start, end, 4):
        word = (load32(blob, cursor)+base) & 0xffffffff
        blob[cursor:cursor+4] = word.to_bytes(4, "big")
    return blob, count, base+start, base+end-4, (base+start) & ~31


def experiments(image):
    base = 0x817e54e0
    zeros = bytearray(0x2000)
    assert relocate(image, base, zeros)[1:] == (0, 0, 0, None)
    zeros[8:12] = (0x40).to_bytes(4, "big")
    assert relocate(image, base, zeros)[1:] == (0, 0, base+0x40, None)
    zeros[0x40:0x44] = (3).to_bytes(4, "big")
    values = [0, 0xffffffff, 0x80000000]
    zeros[0x44:0x50] = b"".join(x.to_bytes(4, "big") for x in values)
    result, count, first, last, arena = relocate(image, base, zeros)
    assert [load32(result, n) for n in (0x44, 0x48, 0x4c)] == [base, base-1, 0x017e54e0]
    assert (count, first, last, arena) == (3, base+0x44, base+0x4c, (base+0x44) & ~31)
    assert result[:0x44] == zeros[:0x44] and result[0x50:] == zeros[0x50:]
    # O=8 aliases the offset/count field; its nonzero value is the count.
    alias = bytearray(0x2000)
    alias[8:12] = (8).to_bytes(4, "big")
    alias[12:16] = (4).to_bytes(4, "big")
    result, count, first, last, arena = relocate(image, base, alias)
    assert count == 8 and load32(result, 12) == base+4
    assert result[8:12] == alias[8:12]
    # Debug4 updates one byte in the same CRT-produced owner, preserving all
    # three neighbouring bytes. Metadata/guard reads must use that owner.
    sda = bytearray(0x375c)
    off = 0x805f1ff0-0x805ef020
    sda[off] = 1
    assert sda[off:off+4] == b"\x01\0\0\0"
    for address, word in ((0x805f1f18,0x80000040),(0x805f1f1c,1),(0x805f1f40,1)):
        start = address-0x805ef020
        sda[start:start+4] = word.to_bytes(4,"big")
        assert load32(sda,start) == word
    declines = 0
    for bad_base in (0, 0x800000f4, 0x80003100, 0x8056fe00, 0x805f1ff0,
                     0x8060c570, 0x817ff000, 0xfffff000, 0x817e54e1):
        try:
            owned_bi2_extent(image, bad_base)
        except ValueError:
            declines += 1
        else:
            raise AssertionError(f"bad owner admitted {bad_base:08X}")
    for offset, count in ((1,1),(0x1ffc,1),(0x40,0xffffffff),(0x2000,1)):
        bad = bytearray(0x2000)
        bad[8:12] = offset.to_bytes(4,"big")
        if offset & 3 == 0 and offset+4 <= len(bad):
            bad[offset:offset+4] = count.to_bytes(4,"big")
        try:
            relocate(image, base, bad)
        except ValueError:
            declines += 1
        else:
            raise AssertionError("unsafe array admitted")
    print(f"PASS isolated portable word/extent/SDA probes; {declines} unsafe cases declined")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dol", type=Path)
    args = parser.parse_args()
    image = DolImage(args.dol)
    count = 0
    for lo, hi in RANGES:
        for pc in range(lo, hi+4, 4):
            word = image.word(pc, text=True)
            ins = decode(word, pc)
            if ins.m == ".word":
                raise ValueError(f"undecoded word {pc:08X}")
            if word >> 26 in (16,18) and ins.target != branch_target(pc,word):
                raise ValueError("decoder/direct branch mismatch")
            count += 1
    print(f"PASS original PAL raw words {count}; stop80379628 word{image.word(0x80379628):08X}")
    reads = []
    samplers = []
    for section in image.sections:
        if not section.text:
            continue
        raw = image.read(section.address, section.end-section.address)
        for offset in range(0,len(raw),4):
            word = int.from_bytes(raw[offset:offset+4],"big")
            if tbr(word) not in (268,269):
                continue
            pc = section.address+offset
            reads.append(pc)
            if tbr(word)==269 and offset+24 <= len(raw):
                words = [int.from_bytes(raw[n:n+4],"big") for n in range(offset,offset+24,4)]
                match = stable_sampler(words,pc)
                if match:
                    samplers.append((pc, match))
    print(f"TIMEBASE words{len(reads)} stable-sampler-families{len(samplers)}")
    for pc,match in samplers:
        print(f"{pc:08X} {match}")
    # Falsify dependency erasure: no nonretry/different-CR/aliased-register
    # six-word sample may inherit the validated structural match.
    words = [image.word(pc) for pc in range(0x80379628,0x80379640,4)]
    for index,changed in ((2,words[0]),(3,words[3] ^ 0x00000800),
                          (4,words[4] ^ 0x00010000),(4,words[4] ^ 4)):
        mutation = words.copy();mutation[index]=changed
        assert stable_sampler(mutation,0x80379628) is None
    experiments(image)


if __name__ == "__main__":
    main()

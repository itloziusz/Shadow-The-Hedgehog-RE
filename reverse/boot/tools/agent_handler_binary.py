"""Independent raw PAL handler audit. Read-only DOL; no cache or file output."""
from pathlib import Path
import hashlib
import struct
import sys
import capstone

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "gameplay" / "tools"))
from ppc import decode
cs = capstone.Cs(capstone.CS_ARCH_PPC, capstone.CS_MODE_32 | capstone.CS_MODE_BIG_ENDIAN)

PIN = "fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af"
raw = Path(sys.argv[1]).read_bytes()
assert hashlib.sha256(raw).hexdigest() == PIN
offs = struct.unpack_from(">18I", raw, 0)
addrs = struct.unpack_from(">18I", raw, 0x48)
sizes = struct.unpack_from(">18I", raw, 0x90)
sections = [(a, a + n, o, i < 7) for i, (a, n, o) in enumerate(zip(addrs, sizes, offs)) if n]

def word(a):
    matches = [(lo, off) for lo, hi, off, _ in sections if lo <= a and a + 4 <= hi]
    assert len(matches) == 1
    lo, off = matches[0]
    return int.from_bytes(raw[off + a - lo:off + a - lo + 4], "big"), off + a - lo

def sext(v, width):
    return v - (1 << width) if v & (1 << (width - 1)) else v

def target(a, w):
    op = w >> 26
    if op == 18:
        d = sext(w & 0x3FFFFFC, 26)
    elif op == 16:
        d = sext(w & 0xFFFC, 16)
    else:
        return None
    return ((0 if w & 2 else a) + d) & 0xFFFFFFFF

def fields(w):
    op = w >> 26
    r = f"op{op} D/S{w >> 21 & 31} A{w >> 16 & 31} B{w >> 11 & 31}"
    if op in (20, 21, 23):
        r += f" SH{w >> 11 & 31} MB{w >> 6 & 31} ME{w >> 1 & 31} Rc{w & 1}"
    elif op in (10, 11):
        r = f"op{op} BF{w >> 23 & 7} L{w >> 21 & 1} A{w >> 16 & 31} imm{w & 0xFFFF:04X}"
    elif op == 19:
        r = f"op19 BT/BO{w >> 21 & 31} BA/BI{w >> 16 & 31} BB/BH{w >> 11 & 31} XO{w >> 1 & 1023} LK{w & 1}"
    elif op == 31:
        xo = w >> 1 & 1023
        r += f" XO{xo} Rc{w & 1}"
        if xo in (339, 467):
            r += f" SPR{(w >> 16 & 31) | ((w >> 11 & 31) << 5)}"
    elif op == 16:
        r = f"op16 BO{w >> 21 & 31} BI{w >> 16 & 31} BD{w >> 2 & 0x3FFF:04X} AA{w >> 1 & 1} LK{w & 1}"
    elif op == 18:
        r = f"op18 LI{w >> 2 & 0xFFFFFF:06X} AA{w >> 1 & 1} LK{w & 1}"
    else:
        r += f" imm{w & 0xFFFF:04X}"
    return r

if len(sys.argv) > 2:
    ranges = [(int(x.split(":")[0], 16), int(x.split(":")[1], 16)) for x in sys.argv[2:]]
else:
    ranges = [(0x80372904, 0x8037292C), (0x80373378, 0x803733C8),
              (0x80373564, 0x80373590), (0x8037611C, 0x80376130),
              (0x80376144, 0x80376168), (0x8000341C, 0x80003424),
              (0x8000315C, 0x80003174)]
for lo, hi in ranges:
    print(f"RANGE {lo:08X}:{hi:08X}")
    for a in range(lo, hi, 4):
        w, off = word(a)
        ins = decode(w, a)
        independent = list(cs.disasm(w.to_bytes(4, "big"), a))
        assert len(independent) == 1 and independent[0].size == 4
        t = target(a, w)
        assert t is None or ins.target == t
        print(f"{a:08X} {off:08X} {w:08X} {fields(w)} | {ins.text()}" + (f" ->{t:08X}" if t is not None else "") + f" | capstone={independent[0].mnemonic} {independent[0].op_str}")

print("DIRECT CALLERS")
for lo, hi, off, text in sections:
    if not text:
        continue
    for a in range(lo, hi - 3, 4):
        w = int.from_bytes(raw[off + a - lo:off + a - lo + 4], "big")
        if w >> 26 == 18 and w & 1 and target(a, w) in (0x80373378, 0x8037611C, 0x80376144):
            t = target(a, w)
            if t == 0x80373378:
                print(f"{a:08X} {w:08X} ->{t:08X}")

print("TABLE ADDRESS IMMEDIATE CANDIDATES (NOT PROVEN XREFS)")
for lo, hi, off, text in sections:
    if not text:
        continue
    for a in range(lo, hi - 3, 4):
        w = int.from_bytes(raw[off + a - lo:off + a - lo + 4], "big")
        if w >> 26 in (14, 24, 32, 36) and 0x6CB0 <= w & 0xFFFF <= 0x6CF8:
            print(f"{a:08X} {w:08X} {decode(w, a).text()}")

def bit_mask(mb, me):
    bits = []
    b = mb
    while True:
        bits.append(b)
        if b == me:
            break
        b = (b + 1) % 32
    return sum(1 << (31 - b) for b in bits)

def rotate(x, count):
    return ((x << count) | (x >> (32 - count))) & 0xFFFFFFFF

# Independent bit-selection implementation checked against closed numeric
# formulas on every bit and all selector-low16 values, not reference outputs.
assert bit_mask(17, 15) == 0xFFFF7FFF
assert bit_mask(14, 29) == 0x0003FFFC
assert bit_mask(16, 31) == 0x0000FFFF
for n in range(32):
    m = 1 << n
    assert m & bit_mask(17, 15) == m & ~0x8000
    assert rotate(m, 17) & bit_mask(31, 31) == (m >> 15) & 1
    assert rotate(m, 2) & bit_mask(14, 29) == (m & 0xFFFF) * 4
for selector in range(65536):
    assert rotate(selector, 2) & bit_mask(14, 29) == selector * 4
for m in [0, 0x2032, 0xA032, 0xFFFFFFFF] + [1 << n for n in range(32)]:
    e = rotate(m, 17) & 1
    disabled = m & bit_mask(17, 15)
    restored = disabled | 0x8000 if e else disabled & bit_mask(17, 15)
    assert restored == m
print("BIT_CHECKS: 96 one-bit projections; 65536 selector offsets; 36 MSR disable/restore cases")

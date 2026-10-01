"""Read-only raw BI2 producer audit; optional live blob comparison, no defaults."""
from pathlib import Path
import hashlib
import json
import struct
import sys
import capstone
from boot_state_diff import FIELD_NAMES, compare_fields, hex_field, reference_fields


def gpr_word(state, index):
    """The RSP snapshot is packed hex, never a list of register words."""
    raw = state["gpr"]
    hex_field(raw, 256, "complete reference GPRs")
    if not 0 <= index < 32:
        raise ValueError("GPR index outside architectural bank")
    return hex_field(raw[8 * index:8 * index + 8], 8, f"r{index}")


def audit_connected_reference(capture):
    """Independently check the finite BI2 effect ledger, not the unknown clock.

    The input state is the live checkpoint before 3188; the expected outputs
    follow the raw field/branch proofs in BI2_CONNECTED_AUDIT_40.md. No register
    values are imported from a native implementation or its output.
    """
    entries = [s for s in capture["checkpoints"]
               if hex_field(s["pc"], 8, "PC") == 0x80003188]
    if len(entries) != 1:
        raise ValueError("connected BI2 entry occurrence missing or duplicated")
    entry = entries[0]
    stop = capture["checkpoints"][-1]
    base = hex_field(entry["bi2_pointer"], 8, "BI2 pointer")
    if not base or base & 3:
        raise ValueError("reference profile needs independently known aligned BI2")
    raw = entry["bi2_blob"]
    hex_field(raw, 0x4000, "complete live BI2")
    if hex_field(stop["bi2_pointer"], 8, "stop BI2 pointer") != base:
        raise ValueError("reference BI2 pointer has an unexplained writer")
    blob = bytearray.fromhex(raw)
    word = lambda off: int.from_bytes(blob[off:off + 4], "big")
    debug, offset = word(12), word(8)
    fields = reference_fields(entry)
    expected = dict(zip(FIELD_NAMES, fields))
    so = hex_field(entry["architectural_xer"], 8, "XER") >> 31
    expected["cr"] = f"{(int(entry['cr'], 16) & 0x0FFFFFFF) | 0x20000000 | (so << 28):08x}"
    expected["r7"] = f"{debug:08x}"
    if debug in (2, 3):
        expected.update(pc="800031f4", lr="8039f8e0", r6="8039f8e0", r5=f"{debug - 2:08x}")
        count = 0
        for key in ("l2_stack_bytes", "bi2_globals", "lowmem_34", "lowmem_48"):
            if stop[key] != entry[key]:
                raise ValueError("unexplained memory effects before context-transfer call")
    else:
        count = 0
        arena = None
        expected["r5"] = f"{base:08x}"
        expected["r6"] = "00000000"
        expected["r14"] = expected["r15"] = "00000000"
        if offset:
            if offset & 3 or offset + 4 > len(blob):
                raise ValueError("unproven reference relocation count read")
            count = word(offset)
            if offset + 4 + count * 4 > len(blob):
                raise ValueError("unproven reference relocation extent")
            expected["r6"] = f"{base + offset:08x}"
            if count:
                start = base + offset + 4
                for n in range(count):
                    off = offset + 4 + n * 4
                    blob[off:off + 4] = ((word(off) + base) & 0xFFFFFFFF).to_bytes(4, "big")
                arena = start & 0xFFFFFFE0
                expected.update(r5="80000034", r6=f"{base + offset + count * 4:08x}",
                                r7=f"{arena:08x}", r14=f"{count:08x}", r15=f"{start:08x}", ctr="00000000")
        msr = hex_field(entry["msr"], 8, "live pre-clock MSR")
        old_ee = (msr >> 15) & 1
        expected.update(pc="80379628", lr="8037966c", r0="80370ea8",
                        r1=f"{gpr_word(entry, 1) - 24 - 32:08x}", r3=f"{old_ee:08x}",
                        r4=f"{msr & 0xFFFF7FFF:08x}", r30="805610f8", r31=f"{old_ee:08x}",
                        msr=f"{msr & 0xFFFF7FFF:08x}")
        # OS/clock spills consume prior register values in this exact order.
        parent = gpr_word(entry, 1)
        writes = [(parent + 4, 0x80003268), (parent - 24, parent),
                  (parent - 4, gpr_word(entry, 31)), (parent - 8, gpr_word(entry, 30)),
                  (parent - 12, gpr_word(entry, 29)), (parent - 20, 0x80370EA8),
                  (parent - 56, parent - 24), (parent - 28, 0x80586C40),
                  (parent - 32, 0x805610F8), (parent - 36, gpr_word(entry, 29))]
        stack_base = hex_field(entry["l2_stack_address"], 8, "stack owner")
        before = bytearray.fromhex(entry["l2_stack_bytes"])
        for address, value in writes:
            off = address - stack_base
            if off < 0 or off + 4 > len(before):
                raise ValueError("reference stack write outside known window")
            before[off:off + 4] = value.to_bytes(4, "big")
        if before.hex() != stop["l2_stack_bytes"].lower():
            raise ValueError("reference stack changed outside independent ordered store ledger")
        sda = ["80000040", "00000001", "00000001", "01000000" if debug == 4 else "00000000"]
        if [v.lower() for v in stop["bi2_globals"]] != sda:
            raise ValueError("reference SDA word/byte effects differ")
        if hex_field(stop["lowmem_48"], 8, "metadata word") != 0x00370C60:
            raise ValueError("reference metadata pointer delta differs")
        expected_arena = arena if arena is not None else hex_field(entry["lowmem_34"], 8, "prior arena")
        if hex_field(stop["lowmem_34"], 8, "post-BI2 arena") != expected_arena:
            raise ValueError("reference arena start/alignment or untouched state differs")
    compare_fields(reference_fields(stop), [expected[k] for k in FIELD_NAMES], FIELD_NAMES, "independent BI2 stop ledger")
    if bytes.fromhex(stop["bi2_blob"]) != blob:
        raise ValueError("reference relocated BI2 owner differs from ordered word effects")
    print(f"INDEPENDENT REFERENCE EFFECTS: debug={debug:08X} O={offset:08X} count={count} "
          f"stop={stop['pc']} {len(FIELD_NAMES)} machine fields; complete BI2 owner; stack/SDA/arena checked")


# Distinct byte patterns catch accidentally reading a nibble or wrong word.
probe = {"gpr": "".join(f"{0x12340000 + n:08x}" for n in range(32))}
for n in (0, 6, 7, 14, 31):
    assert gpr_word(probe, n) == 0x12340000 + n
for malformed in (probe["gpr"][:-1], "UNKNOWN", ["00000000"] * 32):
    try:
        gpr_word({"gpr": malformed}, 7)
    except ValueError:
        pass
    else:
        raise AssertionError("incomplete/symbolic GPR bank accepted")

SYS = Path(sys.argv[1])
EXPECTED = {
    "main.dol": "fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af",
    "bi2.bin": "8bbadffc2a7f79eda5982aa2fe30831969c2702aa9206f621b930aae0b70af6b",
    "apploader.img": "8a1fe9487b029b2518a6633b1e0eb9ec0922f5f5eb5afdd7eba1267a85f096fe",
}
data = {}
for name, expected in EXPECTED.items():
    raw = (SYS / name).read_bytes()
    actual = hashlib.sha256(raw).hexdigest()
    if actual != expected:
        raise ValueError(f"original {name} identity differs")
    data[name] = raw
    print(f"INPUT {name} length={len(raw)} sha256={actual}")
bi2 = data["bi2.bin"]
if len(bi2) != 8192:
    raise ValueError("original BI2 length differs")
print("NONZERO WORDS")
for off in range(0, len(bi2), 4):
    value = int.from_bytes(bi2[off:off + 4], "big")
    if value:
        print(f"{off:04X} {value:08X}")
print("REQUIRED FIELDS")
for off in (0, 4, 8, 12, 0x24):
    print(f"{off:04X} {bi2[off:off + 4].hex()}")
cs = capstone.Cs(capstone.CS_ARCH_PPC, capstone.CS_MODE_32 | capstone.CS_MODE_BIG_ENDIAN)
dol = data["main.dol"]
offsets = struct.unpack_from(">18I", dol, 0)
addresses = struct.unpack_from(">18I", dol, 0x48)
sizes = struct.unpack_from(">18I", dol, 0x90)
sections = [(a, a + n, o, i < 7) for i, (a, n, o) in enumerate(zip(addresses, sizes, offsets)) if n]
print("DOL CONTINUATION (END EXCLUSIVE)")
regions = [(0x80003188, 0x80003268), (0x80003140, 0x8000314C),
           (0x80370BF0, 0x80370C18), (0x80370E68, 0x80370EA8),
           (0x80379648, 0x8037966C), (0x80379628, 0x8037962C),
           (0x8037611C, 0x80376130)]
words = 0
branches = 0
for lo, hi in regions:
    print(f"RANGE {lo:08X}:{hi:08X}")
    for a in range(lo, hi, 4):
        matches = [(base, off) for base, end, off, text in sections if text and base <= a and a + 4 <= end]
        if len(matches) != 1:
            raise ValueError("unmapped DOL text word")
        base, off = matches[0]
        off += a - base
        b = dol[off:off + 4]
        instructions = list(cs.disasm(b, a))
        if len(b) != 4 or len(instructions) != 1:
            raise ValueError("unexplained DOL instruction")
        instruction = instructions[0]
        w = int.from_bytes(b, "big")
        op = w >> 26
        extra = ""
        if op in (16, 18):
            displacement = w & (0xFFFC if op == 16 else 0x3FFFFFC)
            width = 16 if op == 16 else 26
            if displacement & (1 << (width - 1)):
                displacement -= 1 << width
            target = ((0 if w & 2 else a) + displacement) & 0xFFFFFFFF
            # Independently computed target must appear in Capstone operands.
            if instruction.op_str.split(",")[-1].strip() != f"0x{target:x}":
                raise ValueError("independent branch displacement disagrees")
            extra = f" target={target:08X} AA={w >> 1 & 1} LK={w & 1}"
            if op == 16:
                extra += f" BO={w >> 21 & 31} BI={w >> 16 & 31}"
            branches += 1
        if op == 31 and w >> 1 & 1023 in (339, 371, 467):
            extra += f" SPR/TBR={(w >> 16 & 31) | ((w >> 11 & 31) << 5)}"
        print(f"{a:08X} {off:08X} {w:08X} op={op} D/S={w >> 21 & 31} A={w >> 16 & 31} B={w >> 11 & 31} {instruction.mnemonic} {instruction.op_str}{extra}")
        words += 1
print(f"RAW CHECKS: {words} words; {branches} independently decoded direct branches")
print("APPLOADER REQUEST PRODUCER")
for a in range(0x81200A98, 0x81200AD0, 4):
    off = 0x20 + a - 0x81200000
    b = data["apploader.img"][off:off + 4]
    ins = list(cs.disasm(b, a))
    if len(b) != 4 or len(ins) != 1:
        raise ValueError("unexplained apploader instruction")
    w = int.from_bytes(b, "big")
    print(f"{a:08X} {off:08X} {w:08X} op={w >> 26} D/S={w >> 21 & 31} A={w >> 16 & 31} imm={w & 0xFFFF:04X} {ins[0].mnemonic} {ins[0].op_str}")

if len(sys.argv) > 2:
    capture = json.loads(Path(sys.argv[2]).read_text(encoding="utf-8"))
    meaningful = {"80003154", "80003188", "80003194", "8000320c", "80003264", "80379628"}
    for state in [capture["original_unmodified_entry"]] + capture["checkpoints"]:
        if state["pc"].lower() not in meaningful:
            continue
        pointer = state.get("bi2_pointer")
        if "gpr" in state:
            values = " ".join(f"r{n}={gpr_word(state, n):08X}" for n in (0, 1, 3, 4, 5, 6, 7, 14, 15, 29, 30, 31))
            print(f"STATE PC={state['pc']} {values}")
        blob = state.get("bi2_blob")
        if blob is None:
            print(f"LIVE PC={state['pc']} pointer={pointer} BLOB=UNKNOWN")
            continue
        loaded = bytes.fromhex(blob)
        if len(loaded) != len(bi2):
            raise ValueError("live BI2 dump incomplete")
        first = next((n for n, (x, y) in enumerate(zip(loaded, bi2)) if x != y), None)
        print(f"LIVE PC={state['pc']} pointer={pointer} len={len(loaded)} sha256={hashlib.sha256(loaded).hexdigest()} first_original_difference={first}")
    if all("bi2_blob" in s for s in capture["checkpoints"]):
        audit_connected_reference(capture)

"""Independent clock-source pins and integer-only falsification checker.

This research helper never executes an oracle, reads guest outputs as clock
inputs, or supplies runtime timing. Samples must record the independent
CoreTiming cycle producer, its epoch, and the clock output being checked.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re

MASK64 = (1 << 64) - 1
SOURCE_PINS = {
    "Source/Core/Core/HW/SystemTimers.cpp": "578abca48d1ab5273d44bdf337431ff66817352d6862e6c28c79890969962bf7",
    "Source/Core/Core/HW/SystemTimers.h": "f81c9aae61e58c7cf069630f162a66d87ea8342111e2c7db34e833c6d206c69d",
    "Source/Core/Core/Boot/Boot_BS2Emu.cpp": "8c204a36bd7fe7171d6c8e03e7f07739c11297e483dd6180c6ff7391973ec06d",
    "Source/Core/Core/PowerPC/Interpreter/Interpreter_SystemRegisters.cpp": "698f153ad3a9e457c7d51f9a84358b86140c29b879f35dc39fe70412f23f8556",
    "Source/Core/Core/PowerPC/Interpreter/Interpreter.cpp": "1993e7a3976be7aa7d276f1e0b25e04bbdcbe1a4555fac837a4794f6bc18128d",
    "Source/Core/Core/CoreTiming.cpp": "742a2eb94f3db96d85f9ba1637ac1eec77e815b9e0bac1a7b0f574e096d6fac1",
    "Source/Core/Core/HW/EXI/EXI_DeviceIPL.cpp": "08bb42b7c1cac9343561053fde838716fbc11621817cdb11cfc5e99085a32aad",
    "Source/Core/Core/HW/HW.cpp": "173015f302d96ba801efe7c47dbf9bcb572955d6ed116ff61d51215325c151a0",
    "Source/Core/Core/HW/CPU.cpp": "d11e6d1a09586559ed871adb6b9a71a2c07fdf2714b7ee7ee1d1293eea59d7dd",
    "Source/Core/Core/PowerPC/PowerPC.cpp": "cd0de27b7b7359674723a18796fa1032ffd7728a1b177a77d6fefd9376463341",
    "Source/Core/Core/PowerPC/MMU.cpp": "ffc04c07b33b2ff77f1d72f9d6e09dca65c042d94a970e0e570755103aef9d78",
    "Source/Core/Common/Timer.cpp": "cc82523205b4dc13fc865e1d2469db67f5a0217c622d3ab2a14343887d7c5c94",
    "Source/Core/Core/OraclePhysicalWrites.cpp": "baee215768579ae4173b7fbb00364d6193e26ee8eb4012fa891640e12eef1a9d",
    "Source/Core/Core/OracleEventAudit.cpp": "bf4665f53049fd4621aea6fa9502cf765902c357bf60062de6ba6ba14662b1c7",
}
LIB_SHA = "eb0e5041b253072d735ddaac5fba9079ed2ca4916f26312845c706bd132b2939"

def digest(raw):
    return hashlib.sha256(raw).hexdigest()

def pin(path, expected):
    raw = path.read_bytes()
    actual = digest(raw)
    if actual != expected:
        raise ValueError(f"source identity declined: {path}: {actual}")
    return {"path": str(path.resolve()), "sha256": actual, "bytes": len(raw)}

def u64_hex(value, label):
    if not isinstance(value, str) or re.fullmatch("[0-9a-fA-F]{16}", value) is None:
        raise ValueError(f"{label}: exactly sixteen hex digits required")
    return int(value, 16)

def clock_value(cycles, origin_cycles, origin_value):
    # These wrap in the linked C++ producer because all operands are u64.
    delta = (cycles - origin_cycles) & MASK64
    return (origin_value + delta // 12) & MASK64

def check_sample(row):
    if not isinstance(row, dict):
        raise ValueError("clock sample is not an object")
    fields = ("cycles", "origin_cycles", "origin_value", "sampled_full_tb")
    values = [u64_hex(row.get(k), k) for k in fields]
    frequency = row.get("cpu_frequency")
    if not isinstance(frequency, str) or frequency.lower() != "1cf7c580":
        raise ValueError("sample does not establish the GC 486MHz source")
    actual = clock_value(*values[:3])
    if actual != values[3]:
        raise ValueError(f"clock equation falsified: {actual:016x} != {values[3]:016x}")
    return {"cycles": row["cycles"], "expected_full_tb": f"{actual:016x}"}

def self_test():
    vectors = [
        (0, 0, 0, 0),
        (11, 0, 0, 0),
        (12, 0, 0, 1),
        (35, 24, 1234, 1234),
        (36, 24, 1234, 1235),
        (12, 0, 0xFFFFFFFF, 0x100000000),
        (12, 0, MASK64, 0),
        (0, 1, 0, MASK64 // 12),
        (1, MASK64, 7, 7),
    ]
    for args in vectors:
        if clock_value(*args[:3]) != args[3]:
            raise ValueError("producer arithmetic self-test failed")
    # Reject signed subtraction, per-read ticking, host-nanosecond units,
    # frequency changes, malformed exports and an altered clock observation.
    base = dict(cycles="000000000000000c", origin_cycles="0000000000000000",
                origin_value="0000000000000000", sampled_full_tb="0000000000000001",
                cpu_frequency="1cf7c580")
    check_sample(base)
    bad = []
    for field, value in [("cycles", 12), ("cycles", "c"),
                         ("origin_cycles", "-000000000000001"),
                         ("origin_value", "00000000000000000"),
                         ("cpu_frequency", "09a7ec80"),
                         ("sampled_full_tb", "000000000000000c")]:
        altered = dict(base)
        altered[field] = value
        bad.append(altered)
    for altered in bad:
        try:
            check_sample(altered)
        except ValueError:
            continue
        raise ValueError("altered producer observation was accepted")
    return {"synthetic_arithmetic_vectors": len(vectors), "declined_samples": len(bad),
            "status": "PASS; mathematical falsification only, no capture parity claim"}


def capture_audit(path):
    payload = json.loads(path.read_text(encoding="utf-8"))
    points = payload.get("checkpoints") if isinstance(payload, dict) else None
    if not isinstance(points, list) or not points:
        raise ValueError("nonempty checkpoint capture required")
    checks = []
    for before, after in zip(points, points[1:]):
        raw = before.get("instruction_word")
        if not isinstance(raw, str) or re.fullmatch("[0-9a-fA-F]{8}", raw) is None:
            raise ValueError("instruction bytes unavailable at checkpoint")
        word = int(raw, 16)
        if word >> 26 != 31 or ((word >> 1) & 1023) not in (339, 371):
            continue
        encoded_spr = (word >> 11) & 1023
        spr = ((encoded_spr & 31) << 5) | (encoded_spr >> 5)
        if spr not in (268, 269):
            continue
        if word & 1:
            raise ValueError("reserved mftb Rc bit set")
        pc = int(before["pc"], 16)
        if int(after["pc"], 16) != ((pc + 4) & 0xffffffff):
            raise ValueError("TB instruction successor absent or exceptional")
        src, dst = before.get("clock_producer"), after.get("clock_producer")
        if not isinstance(src, dict) or not isinstance(dst, dict):
            raise ValueError("independent clock producer exports absent")
        for field in ("epoch_cycles", "epoch_value", "cpu_hz", "rtc_offset"):
            if src.get(field) != dst.get(field):
                raise ValueError("unexplained producer change during TB read")
        sample = dict(cycles=src.get("cycles"), origin_cycles=src.get("epoch_cycles"),
                      origin_value=src.get("epoch_value"),
                      sampled_full_tb=dst.get("cached_tb"), cpu_frequency=src.get("cpu_hz"))
        checked = check_sample(sample)
        if u64_hex(dst.get("cycles"), "after cycles") != ((u64_hex(src.get("cycles"), "before cycles") + 1) & MASK64):
            raise ValueError("capture is not the admitted one-cycle step schedule")
        rd = (word >> 21) & 31
        full = int(checked["expected_full_tb"], 16)
        expected = full >> 32 if spr == 269 else full & 0xffffffff
        for obj in (before, after):
            if not isinstance(obj.get("gpr"), str) or re.fullmatch("[0-9a-fA-F]{256}", obj["gpr"]) is None:
                raise ValueError("incomplete GPR bank")
        bank_before, bank_after = before["gpr"].lower(), after["gpr"].lower()
        if int(bank_after[rd * 8:rd * 8 + 8], 16) != expected:
            raise ValueError("actual TB destination GPR differs")
        for reg in range(32):
            if reg != rd and bank_before[reg * 8:reg * 8 + 8] != bank_after[reg * 8:reg * 8 + 8]:
                raise ValueError("TB read altered another GPR")
        for field in ("msr", "cr", "lr", "ctr", "architectural_xer", "fpscr",
                      "clock_low_words", "clock_globals", "bi2_blob", "l2_stack_bytes"):
            if before.get(field) is None or before[field] != after.get(field):
                raise ValueError("unexplained TB-read effect: " + field)
        checks.append(dict(pc=f"{pc:08x}", rd=rd, tbr=spr, **checked))
    if not checks:
        raise ValueError("capture contains no complete TB instruction transition")
    metadata = payload.get("region_extension", {})
    pause = metadata.get("pause_samples")
    if not isinstance(pause, list) or len(pause) != 2 or pause[0] != pause[1]:
        raise ValueError("paused producer changed or pause proof absent")
    for row in pause:
        for field in ("cycles", "epoch_cycles", "epoch_value", "rtc_offset", "cached_tb"):
            u64_hex(row.get(field), "pause " + field)
    return {"path": str(path.resolve()), "sha256": digest(path.read_bytes()),
            "validated_instruction_reads": checks, "unchanged_pause_samples": len(pause),
            "scope": "one-cycle stepped TB reads; preceding cycle source not reconstructed"}


def capture_falsification(path):
    original = json.loads(path.read_text(encoding="utf-8"))
    capture_audit(path)
    index = next(i for i, row in enumerate(original["checkpoints"])
                 if int(row["pc"], 16) == 0x80379628)
    class MemoryPath:
        def __init__(self, payload):
            self.raw = json.dumps(payload).encode()
        def read_text(self, **kwargs):
            return self.raw.decode()
        def read_bytes(self):
            return self.raw
        def resolve(self):
            return "<controlled in-memory capture mutation>"
    mutations = [
        ("postcycle-used-as-presample", "before", "cycles", "00000000002B30E0"),
        ("producer-epoch-changed", "before", "epoch_value", "0000000000000000"),
        ("cached-result-changed", "after", "cached_tb", "0000000000000000"),
        ("unproved-frequency", "before", "cpu_hz", "09A7EC80"),
        ("malformed-origin", "before", "epoch_cycles", "0"),
    ]
    for label, location, field, value in mutations:
        payload = json.loads(json.dumps(original))
        at = index if location == "before" else index + 1
        # Make the presample phase mutation one cycle later for any profile.
        if label == "postcycle-used-as-presample":
            value = payload["checkpoints"][index + 1]["clock_producer"]["cycles"]
        payload["checkpoints"][at]["clock_producer"][field] = value
        try:
            capture_audit(MemoryPath(payload))
        except ValueError:
            continue
        raise ValueError("capture mutation accepted: " + label)
    for label in ("other-gpr-changed", "pause-changed", "successor-changed"):
        payload = json.loads(json.dumps(original))
        if label == "other-gpr-changed":
            bank = payload["checkpoints"][index + 1]["gpr"]
            payload["checkpoints"][index + 1]["gpr"] = ("1" if bank[0] != "1" else "2") + bank[1:]
        elif label == "pause-changed":
            payload["region_extension"]["pause_samples"][1]["cached_tb"] = "0000000000000001"
        else:
            payload["checkpoints"][index + 1]["pc"] = "80379630"
        try:
            capture_audit(MemoryPath(payload))
        except ValueError:
            continue
        raise ValueError("capture mutation accepted: " + label)
    return {"declined_capture_mutations": len(mutations) + 3, "status": "PASS"}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, required=True,
                        help="matching read-only oracle source directory")
    parser.add_argument("--library", type=Path, required=True,
                        help="matching read-only linked core.lib")
    parser.add_argument("--sample-json", type=Path,
                        help="JSON with nonempty samples list using the exact documented schema")
    parser.add_argument("--capture", type=Path, action="append", default=[], help="existing checkpoint capture; verifies BEFORE producer against AFTER result")
    args = parser.parse_args()
    result = {
        "scope": "read-only source/library pins; independent unsigned producer equation",
        "sources": [pin(args.source_root / rel, expected) for rel, expected in SOURCE_PINS.items()],
        "linked_core_library": pin(args.library, LIB_SHA),
        "falsification": self_test(),
    }
    if args.sample_json:
        payload = json.loads(args.sample_json.read_text(encoding="utf-8"))
        rows = payload.get("samples") if isinstance(payload, dict) else None
        if not isinstance(rows, list) or not rows:
            raise ValueError("nonempty independent clock sample list required")
        result["sample_sha256"] = digest(args.sample_json.read_bytes())
        result["checked_samples"] = [check_sample(row) for row in rows]
    result["capture_audits"] = [capture_audit(path) for path in args.capture]
    result["capture_falsification"] = [capture_falsification(path) for path in args.capture]
    print(json.dumps(result, indent=2))

if __name__ == "__main__":
    main()

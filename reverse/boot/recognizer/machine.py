"""Pinned DOL bytes, normalized PPC instructions, bounded symbolic effects."""

from __future__ import annotations

import hashlib
import re
import struct
import sys
from dataclasses import dataclass
from pathlib import Path

# Reuse the project's Gekko-aware decoder; never substitute a symbol table.
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "gameplay" / "tools"))
from ppc import decode  # noqa: E402

PAL_SHA256 = "fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af"


@dataclass(frozen=True)
class Section:
    address: int
    end: int
    offset: int
    text: bool


class DolImage:
    def __init__(self, path: Path, *, require_pal: bool = True):
        self.path = Path(path)
        self.raw = self.path.read_bytes()
        self.sha256 = hashlib.sha256(self.raw).hexdigest()
        if require_pal and self.sha256 != PAL_SHA256:
            raise ValueError("DOL SHA-256 is not the pinned PAL GUPP8P binary")
        if len(self.raw) < 0xE4:
            raise ValueError("truncated DOL header")
        offsets = struct.unpack_from(">18I", self.raw, 0)
        addresses = struct.unpack_from(">18I", self.raw, 0x48)
        sizes = struct.unpack_from(">18I", self.raw, 0x90)
        self.entry = struct.unpack_from(">I", self.raw, 0xE0)[0]
        self.sections = []
        for i, (offset, address, size) in enumerate(zip(offsets, addresses, sizes)):
            if not size:
                continue
            if (offset < 0x100 or offset + size > len(self.raw) or
                    address + size > 0x100000000 or
                    any(address < s.end and s.address < address + size for s in self.sections)):
                raise ValueError(f"invalid or overlapping DOL section {i}")
            self.sections.append(Section(address, address + size, offset, i < 7))
        if require_pal and self.entry != 0x80003154:
            raise ValueError("PAL DOL entry differs")

    def section(self, address: int, size: int = 1) -> Section | None:
        return next((s for s in self.sections
                     if s.address <= address and address + size <= s.end), None)

    def read(self, address: int, size: int, *, text: bool | None = None) -> bytes:
        s = self.section(address, size)
        if s is None or (text is not None and s.text != text):
            raise ValueError(f"unmapped or wrong-kind DOL address 0x{address:08X}+{size}")
        offset = s.offset + address - s.address
        return self.raw[offset:offset + size]

    def word(self, address: int, *, text: bool | None = None) -> int:
        if address & 3:
            raise ValueError(f"unaligned DOL word 0x{address:08X}")
        return int.from_bytes(self.read(address, 4, text=text), "big")

    def is_text(self, address: int) -> bool:
        s = self.section(address, 4)
        return bool(s and s.text and not address & 3)


def hx(value: int) -> str:
    return f"0x{value & 0xFFFFFFFF:08X}"


def literal(value: str) -> int | None:
    return int(value[2:], 16) if value.startswith("K:") else None


def add(value: str, delta: int) -> str:
    k = literal(value)
    return f"K:{(k + delta) & 0xFFFFFFFF:08X}" if k is not None else (
        value if delta == 0 else f"({value}{delta:+#x})")


def role(reg: int, names: dict[int, str]) -> str:
    fixed = {0: "R0", 1: "SP", 2: "SDA2", 13: "SDA"}
    return fixed[reg] if reg in fixed else names.setdefault(reg, f"t{len(names)}")


def normalize(ins, start: int, end: int, names: dict[int, str]) -> str:
    """Alpha-rename GPR temporaries while retaining important offsets/SPR IDs."""
    args = []
    for op in ins.ops:
        kind = op[0]
        if kind == "r":
            args.append(role(op[1], names))
        elif kind == "f":
            args.append(f"f{op[1]}")  # paired-lane identity matters
        elif kind == "s":
            args.append(f"SPR{op[1]}")
        elif kind == "c":
            args.append(f"CR{op[1]}")
        elif kind == "m":
            disp, base = op[1:]
            offset = str(disp) if base in (1, 2, 13) or abs(disp) < 256 else "DISP16"
            args.append(f"[{role(base, names) if base else 'ZERO'},{offset}]")
        elif kind == "x":
            args.append(f"[{role(op[1], names)}+{role(op[2], names)}]")
        elif kind == "t":
            args.append("BACK" if start <= op[1] < ins.addr else
                        "FWD" if ins.addr <= op[1] < end else "EXTERNAL")
        elif kind == "i":
            args.append(str(op[1]) if abs(op[1]) <= 256 else "CONST16")
        else:
            args.append(str(op))
    return ins.m + (":" + ",".join(args) if args else "")


def coarse_normalize(token: str) -> str:
    """Second matching layer; precise token and raw operands remain retained."""
    token = re.sub(r"f\d+", "F", token)
    token = re.sub(r"\[(SDA2|SDA),-?\d+\]", r"[\1,GLOBAL_OFF]", token)
    token = re.sub(r"\[SP,-?\d+\]", "[SP,STACK_OFF]", token)
    token = re.sub(r"\[(t\d+),-?\d+\]", r"[\1,FIELD_OFF]", token)
    return token


def initial_state() -> dict[str, str]:
    s = {f"r{i}": f"IN:r{i}" for i in range(32)}
    s.update({"r1": "IN:SP", "r2": "IN:SDA2", "r13": "IN:SDA",
              "LR": "IN:LR", "CTR": "IN:CTR", "XER": "IN:XER",
              "MSR": "IN:MSR", "FPSCR": "IN:FPSCR"})
    for cr in range(8):
        s[f"CR{cr}"] = f"IN:CR{cr}"
    for fpr in range(32):
        s[f"f{fpr}.ps0"] = f"IN:f{fpr}.ps0"
        s[f"f{fpr}.ps1"] = f"IN:f{fpr}.ps1"
    return s


def join(old: dict[str, str] | None, new: dict[str, str]) -> tuple[dict[str, str], bool]:
    if old is None:
        return new.copy(), True
    merged = {k: old[k] if old[k] == new[k] else "UNKNOWN" for k in old}
    return merged, merged != old


def transfer(ins, s: dict[str, str], effects: list[dict]) -> None:
    m, rd, ra = ins.m, ins.rd, ins.ra
    src = s[f"r{rd}"]
    if m in ("li", "lis"):
        s[f"r{rd}"] = f"K:{((ins.imm << 16) if m == 'lis' else ins.imm) & 0xFFFFFFFF:08X}"
    elif m in ("addi", "addis"):
        s[f"r{rd}"] = add(s[f"r{ra}"], ins.imm << (16 if m == "addis" else 0))
    elif m in ("ori", "oris", "xori", "xoris", "andi.", "andis."):
        value = literal(src)
        imm = ins.imm << (16 if m in ("oris", "xoris", "andis.") else 0)
        s[f"r{ra}"] = (f"K:{((value | imm) if m.startswith('or') else
                             (value & imm) if m.startswith('and') else (value ^ imm)) & 0xFFFFFFFF:08X}"
                      if value is not None else f"{m}({src},{hx(imm)})")
        if m.startswith("and"):
            s["CR0"] = f"cmp0({s[f'r{ra}']})"
    elif (ins.w >> 26) in (20, 21):
        sh, mb, me = ins.rb, (ins.w >> 6) & 31, (ins.w >> 1) & 31
        mask = 0
        for bit in range(32):
            if (mb <= me and mb <= bit <= me) or (mb > me and (bit >= mb or bit <= me)):
                mask |= 1 << (31 - bit)
        value = literal(src)
        rotated = ((value << sh) | (value >> ((32 - sh) & 31))) & 0xFFFFFFFF if value is not None else None
        old = literal(s[f"r{ra}"])
        result = (((old & ~mask) | (rotated & mask)) if old is not None and rotated is not None
                  else None) if (ins.w >> 26) == 20 else (rotated & mask if rotated is not None else None)
        s[f"r{ra}"] = f"K:{result & 0xFFFFFFFF:08X}" if result is not None else (
            f"rotate_mask({src},sh={sh},mb={mb},me={me})")
        if ins.w & 1:
            s["CR0"] = f"cmp0({s[f'r{ra}']})"
    elif m in ("add", "subf"):
        left, right = literal(s[f"r{ra}"]), literal(s[f"r{ins.rb}"])
        result = (left + right if m == "add" else right - left) if left is not None and right is not None else None
        s[f"r{rd}"] = f"K:{result & 0xFFFFFFFF:08X}" if result is not None else (
            f"{m}({s[f'r{ra}']},{s[f'r{ins.rb}']})")
        if ins.w & 1:
            s["CR0"] = f"cmp0({s[f'r{rd}']})"
    elif m == "mr":
        s[f"r{ra}"] = src
    elif m in ("mflr", "mfctr"):
        s[f"r{rd}"] = s["LR" if m == "mflr" else "CTR"]
    elif m in ("mtlr", "mtctr"):
        s["LR" if m == "mtlr" else "CTR"] = src
    elif m == "mfspr":
        s[f"r{rd}"] = f"SPR:{ins.imm}"
        effects.append({"pc": hx(ins.addr), "kind": "spr_read", "spr": ins.imm})
    elif m == "mtspr":
        effects.append({"pc": hx(ins.addr), "kind": "spr_write", "spr": ins.imm,
                        "value": src})
    elif m in ("stmw", "lmw"):
        disp, base_reg = ins.ops[1][1:]
        base = "K:00000000" if base_reg == 0 else s[f"r{base_reg}"]
        address = add(base, disp)
        if m == "lmw" and base_reg >= rd and base_reg != 0:
            # A destination register used as the address base is not modeled
            # as a sequential source of later effective addresses.
            for key in s:
                s[key] = "UNKNOWN:ambiguous_lmw_base"
            effects.append({"pc": hx(ins.addr), "kind": "unsupported",
                            "mnemonic": m, "reason": "base overlaps loaded register range"})
        else:
            for reg in range(rd, 32):
                ea = add(address, 4 * (reg - rd))
                if m == "stmw":
                    effects.append({"pc": hx(ins.addr), "kind": "store", "address": ea,
                                    "width": 4, "value": s[f"r{reg}"], "register": f"r{reg}",
                                    "fp": False})
                else:
                    effects.append({"pc": hx(ins.addr), "kind": "load", "address": ea,
                                    "width": 4, "value": None, "register": f"r{reg}",
                                    "fp": False})
                    s[f"r{reg}"] = f"MEM32:{ea}"
    elif m in ("lwz", "lbz", "lhz", "lha", "lfs", "lfd", "stw", "stwu", "stb", "sth", "stfs", "stfd",
               "psq_l", "psq_lu", "psq_st", "psq_stu"):
        disp, base_reg = ins.ops[1][1:]
        base = "K:00000000" if base_reg == 0 else s[f"r{base_reg}"]
        address = add(base, disp)
        store = ins.kind == "store"
        value = (s[f"f{rd}.ps0"] if ins.fp else src) if store else None
        effects.append({"pc": hx(ins.addr), "kind": "store" if store else "load",
                        "address": address, "width": ins.size, "value": value,
                        "fp": ins.fp})
        if not store:
            if m.startswith("psq_l"):
                gqr = ins.ops[3][1]
                s[f"f{rd}.ps0"] = f"PSQ0({address},GQR{gqr})"
                s[f"f{rd}.ps1"] = f"PSQ1({address},GQR{gqr},W={ins.ops[2][1]})"
            elif ins.fp:
                s[f"f{rd}.ps0"] = f"MEM{ins.size * 8}:{address}"
                # Scalar load effects on the paired lane are not established
                # by this abstraction. Never retain a previous PS1 value.
                s[f"f{rd}.ps1"] = "UNKNOWN:scalar_fp_lane"
            else:
                s[f"r{rd}"] = f"MEM{ins.size * 8}:{address}"
        if ins.upd:
            s[f"r{base_reg}"] = address
    elif m in ("cmpwi", "cmplwi", "cmpw", "cmplw"):
        cr = ins.ops[0][1]
        s[f"CR{cr}"] = f"{m}({s[f'r{ra}']},{ins.imm if ins.imm is not None else s[f'r{ins.rb}']})"
    elif m == "mfmsr":
        s[f"r{rd}"] = s["MSR"]
    elif m == "mtmsr":
        s["MSR"] = src
        effects.append({"pc": hx(ins.addr), "kind": "msr_write", "value": src,
                        "status": "exception_timing_unresolved"})
    elif m in ("fmr", "ps_mr"):
        source_fpr = ins.rb
        s[f"f{rd}.ps0"] = s[f"f{source_fpr}.ps0"]
        if m == "ps_mr":
            s[f"f{rd}.ps1"] = s[f"f{source_fpr}.ps1"]
        else:
            s[f"f{rd}.ps1"] = "UNKNOWN:scalar_fp_lane"
    elif m == "mtfsf":
        s["FPSCR"] = f"mtfsf({s[f'f{ins.rb}.ps0']},mask={ins.ops[0][1]})"
        effects.append({"pc": hx(ins.addr), "kind": "fpscr_write", "value": s["FPSCR"],
                        "status": "exception_flags_unresolved"})
    elif ins.kind == "call":
        target = hx(ins.target) if ins.target is not None else s["CTR" if "ctr" in m else "LR"]
        effects.append({"pc": hx(ins.addr), "kind": "call", "target": target,
                        "indirect": ins.target is None,
                        "return_address": hx(ins.addr + 4),
                        "callee_effects": "UNTRACED"})
        # This bounded analysis does not descend into callees. Even ABI
        # preservation is a claim about their implementation; retain no
        # post-call constants until the target and effects are checked.
        for key in s:
            s[key] = "UNKNOWN:untraced_call"
    elif m in ("sync", "isync"):
        effects.append({"pc": hx(ins.addr), "kind": "barrier",
                        "status": "unresolved_hardware"})
    elif ins.kind == "ret":
        effects.append({"pc": hx(ins.addr), "kind": "return", "target": s["LR"]})
    elif ins.kind == "branch":
        # The BO field can combine CTR and CR tests. Do not reduce a compound
        # predicate to one of its inputs without decoding the full BO semantics.
        condition = (f"UNKNOWN:BO={ins.bo},BI={ins.bi},CTR={s['CTR']}"
                     if ins.bo is not None and ins.bo not in (4, 5, 12, 13, 20)
                     else s[f"CR{ins.bi >> 2}"] if ins.bi is not None and ins.bo != 20
                     else "ALWAYS" if ins.bo == 20 else "UNKNOWN")
        effects.append({"pc": hx(ins.addr), "kind": "branch",
                        "target": hx(ins.target) if ins.target is not None else
                        s["LR" if "lr" in m else "CTR"],
                        "condition": condition})
    elif m == "nop":
        pass
    else:
        # An unmodeled instruction may write memory, any architectural
        # register, or affect a later predicate. Kill the complete abstract
        # state rather than allowing a stale constant to flow past it.
        for key in s:
            s[key] = "UNKNOWN:raw_opcode" if m == ".word" else "UNKNOWN:unsupported"
        effects.append({"pc": hx(ins.addr), "kind": "unsupported", "mnemonic": m})


def analyze(image: DolImage, start: int, end: int) -> dict:
    """Analyze caller-supplied half-open code range; never assert function extent."""
    if start & 3 or end & 3 or not start < end or end - start > 0x1000:
        raise ValueError("invalid bounded analysis range")
    words = [image.word(pc, text=True) for pc in range(start, end, 4)]
    insns = [decode(word, pc) for pc, word in zip(range(start, end, 4), words)]
    names: dict[int, str] = {}
    rows = []
    for ins, word in zip(insns, words):
        section = image.section(ins.addr)
        rows.append({"pc": hx(ins.addr), "file_offset": section.offset + ins.addr - section.address,
                     "raw": word.to_bytes(4, "big").hex().upper(), "word": hx(word),
                     "asm": ins.text(), "mnemonic": ins.m, "kind": ins.kind,
                     "normalized": normalize(ins, start, end, names)})
        rows[-1]["coarse_normalized"] = coarse_normalize(rows[-1]["normalized"])
    leaders = {start}
    for ins in insns:
        if ins.kind in ("branch", "ret"):
            if ins.target is not None and start <= ins.target < end:
                leaders.add(ins.target)
            if ins.addr + 4 < end:
                leaders.add(ins.addr + 4)
    positions = sorted(leaders)
    blocks = []
    for i, lo in enumerate(positions):
        hi = positions[i + 1] if i + 1 < len(positions) else end
        last = insns[(hi - start) // 4 - 1]
        successors = []
        if last.kind == "branch":
            if last.target is not None and start <= last.target < end:
                successors.append(last.target)
            if last.m not in ("b", "ba") and hi < end:
                successors.append(hi)
        elif last.kind != "ret" and hi < end:
            successors.append(hi)
        unresolved_edge = last.kind == "branch" and last.target is None
        blocks.append({"start": lo, "end": hi, "successors": sorted(set(successors)),
                       "unresolved_edge": unresolved_edge,
                       "external_exit": hx(last.target) if last.kind == "branch" and
                       last.target is not None and not start <= last.target < end else None})
    by_start = {b["start"]: b for b in blocks}
    entries: dict[int, dict[str, str]] = {start: initial_state()}
    pending = [start]
    effects_by_pc: dict[tuple[str, str], dict] = {}
    exit_states: dict[int, dict[str, str]] = {}
    visits = 0
    while pending:
        lo = pending.pop(0)
        visits += 1
        if visits > len(blocks) * 80:
            raise ValueError("data-flow join did not converge")
        state = entries[lo].copy()
        block = by_start[lo]
        for pc in range(lo, block["end"], 4):
            local = []
            transfer(insns[(pc - start) // 4], state, local)
            for slot, effect in enumerate(local):
                effects_by_pc[(effect["pc"], effect["kind"], slot)] = effect
        for successor in block["successors"]:
            merged, changed = join(entries.get(successor), state)
            if changed:
                entries[successor] = merged
                pending.append(successor)
        if not block["successors"]:
            exit_states[block["end"]] = state.copy()
    # Keep within-instruction ordering: lexical register sorting would put
    # r10 before r9 and silently falsify ordered stmw/lmw memory effects.
    effects = [effect for (_, _, _), effect in sorted(
        effects_by_pc.items(), key=lambda item: (int(item[0][0], 16), item[0][2]))]
    return {"start": hx(start), "end": hx(end), "sha256": image.sha256,
            "boundary_status": "caller_supplied_range", "instructions": rows,
            "cfg": [{"start": hx(b["start"]), "end": hx(b["end"]),
                     "successors": [hx(s) for s in b["successors"]],
                     "external_exit": b["external_exit"],
                     "unresolved_edge": b["unresolved_edge"]} for b in blocks],
            "effects": effects, "entry_state": entries[start],
            "exit_states": {hx(pc): state for pc, state in exit_states.items()},
            "unknown_instructions": [r["pc"] for r in rows if r["mnemonic"] == ".word"],
            "unsupported_semantics": [e["pc"] for e in effects if e["kind"] == "unsupported"],
            "external_exits": [b["external_exit"] for b in blocks if b["external_exit"]],
            "unresolved_edges": [hx(b["end"] - 4) for b in blocks if b["unresolved_edge"]]}

#!/usr/bin/env python3
"""Independent static timing audit and falsification probes; never a clock provider.

No guest operation is executed. Trace values are checked as evidence only and
cannot initialize a native clock. No coordinator validator is imported.
"""
from __future__ import annotations

import argparse
import collections
import hashlib
import json
import math
from pathlib import Path
import re
import struct

MASK = (1 << 64) - 1
DOL_SHA = "fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af"
PINS = {
    "CoreTiming.cpp": "742a2eb94f3db96d85f9ba1637ac1eec77e815b9e0bac1a7b0f574e096d6fac1",
    "CoreTiming.h": "fdfab0fbb4bdc953a946f746012636b6005a96d875d82ad3263543cacaa7ea7b",
    "PowerPC/Interpreter/Interpreter.cpp": "1993e7a3976be7aa7d276f1e0b25e04bbdcbe1a4555fac837a4794f6bc18128d",
    "PowerPC/Interpreter/Interpreter_Branch.cpp": "ac92c3fc9f5d05b7bc6fea7a39bdb0682c13d80223fc8ca88131a55a6df29068",
    "PowerPC/Interpreter/Interpreter_SystemRegisters.cpp": "698f153ad3a9e457c7d51f9a84358b86140c29b879f35dc39fe70412f23f8556",
    "PowerPC/PPCTables.cpp": "4814b6c63088c0227eadf2fefa4f32fe2109afde7319b88697b35a7b67dcf771",
    "HLE/HLE.cpp": "81f052fb94a0d3daebcb1d4fa956d71c0f7ba090de61ed4903999036189b45bc",
    "HLE/HLE_OS.cpp": "8da9a89200e5f6c443c32bb7f872e0622d621bb289f9ffd183d5ead4c561e6e3",
    "HW/CPU.cpp": "d11e6d1a09586559ed871adb6b9a71a2c07fdf2714b7ee7ee1d1293eea59d7dd",
    "HW/SystemTimers.cpp": "578abca48d1ab5273d44bdf337431ff66817352d6862e6c28c79890969962bf7",
}


def require(condition: bool, why: str) -> None:
    if not condition:
        raise ValueError(why)


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


class Dol:
    def __init__(self, path: Path):
        self.data = path.read_bytes()
        require(sha(self.data) == DOL_SHA, "original PAL DOL hash mismatch")
        self.sections = []
        for offsets, addresses, sizes, count in ((0, 0x48, 0x90, 7), (0x1C, 0x64, 0xAC, 11)):
            for i in range(count):
                off, address, size = [self.header(base + i * 4) for base in (offsets, addresses, sizes)]
                if size:
                    require(off + size <= len(self.data), "DOL section outside file")
                    self.sections.append((address, off, size))

    def header(self, offset: int) -> int:
        return struct.unpack_from(">I", self.data, offset)[0]

    def word(self, address: int) -> int:
        matches = [(off + address - va) for va, off, size in self.sections if va <= address and address + 4 <= va + size]
        require(len(matches) == 1 and not (address & 3), "unknown or ambiguous DOL word")
        return self.header(matches[0])


def source_audit(root: Path) -> dict:
    base = root / "Source/Core/Core"
    for name, expected in PINS.items():
        require(sha((base / name).read_bytes()) == expected, f"source pin changed: {name}")
    interp = (base / "PowerPC/Interpreter/Interpreter.cpp").read_text(encoding="utf-8")
    require("if (power_pc.CheckAndHandleBreakPoints())\n            return;\n          cycles += SingleStepInner();" in interp,
            "breakpoint early-return ordering changed")
    require("core_timing_globals.slice_length = 1;\n  m_ppc_state.downcount = 0;" in interp,
            "SingleStep unit-cycle reset changed")
    require("return PPCTables::GetOpInfo(m_prev_inst, m_ppc_state.pc)->num_cycles;" in interp,
            "HLE previous-op cost path changed")
    return {"pinned_sources": len(PINS), "scope": "private HLE interpreter scheduling only"}


def source_cost_reader(source_root: Path):
    """Read metadata for a finite static audit, without executing instructions."""
    source = (source_root / "Source/Core/Core/PowerPC/PPCTables.cpp").read_text(encoding="utf-8")
    tables = {}
    for name in ("s_primary_table", "s_table4", "s_table19", "s_table31", "s_table63"):
        match = re.search(r"constexpr std::array<GekkoOPTemplate, \d+> " + name + r"\{\{(.*?)\}\};", source, re.S)
        require(match is not None, f"missing source table {name}")
        entries = re.findall(r'\{(\d+),\s*"([^"]+)",\s*OpType::\w+,\s*(\d+),', match[1])
        tables[name] = {int(op): (opname, int(cost)) for op, opname, cost in entries}
        require(len(tables[name]) == len(entries), f"duplicate table entry {name}")

    def cost(word: int) -> int:
        op = word >> 26
        if op in (4, 19, 31, 63):
            selected, op = tables[f"s_table{op}"], (word >> 1) & 1023
        else:
            selected = tables["s_primary_table"]
        require(op in selected, "unknown static cost opcode")
        require(selected[op][1] > 0, "subtable or unknown static cost")
        return selected[op][1]

    return cost


def static_cost_audit(dol: Dol, source_root: Path) -> dict:
    # Exact whole leaf, including untaken alignment, nonzero and byte paths.
    leaf = [
        0x28050020, 0x5484063E, 0x38C3FFFF, 0x7C872378, 0x41800090,
        0x7CC030F8, 0x540307BF, 0x41820014, 0x7CA32850, 0x3463FFFF,
        0x9CE60001, 0x4082FFF8, 0x28070000, 0x4182001C, 0x54E3C00E,
        0x54E0801E, 0x54E4402E, 0x7C600378, 0x7C800378, 0x7CE70378,
        0x54A3D97F, 0x3886FFFD, 0x4182002C, 0x90E40004, 0x3463FFFF,
        0x90E40008, 0x90E4000C, 0x90E40010, 0x90E40014, 0x90E40018,
        0x90E4001C, 0x94E40020, 0x4082FFDC, 0x54A3F77F, 0x41820010,
        0x3463FFFF, 0x94E40004, 0x4082FFF8, 0x38C40003, 0x54A507BE,
        0x28050000, 0x4D820020, 0x34A5FFFF, 0x9CE60001, 0x4082FFF8, 0x4E800020,
    ]
    wrapper = [0x9421FFF0, 0x7C0802A6, 0x90010014, 0x93E1000C, 0x7C7F1B78, 0x4800001D,
               0x80010014, 0x7FE3FB78, 0x83E1000C, 0x7C0803A6, 0x38210010, 0x4E800020]
    relocation = [0x38C60004, 0x80E60000, 0x7CE72A14, 0x90E60000, 0x4200FFF0]
    sampler = [0x7C6D42E6, 0x7C8C42E6, 0x7CAD42E6, 0x7C032800, 0x4082FFF0, 0x4E800020]
    static_cost = source_cost_reader(source_root)
    for base, words in ((0x8000543C, leaf), (0x8000540C, wrapper), (0x80003230, relocation), (0x80379628, sampler)):
        for i, expected in enumerate(words):
            require(dol.word(base + 4 * i) == expected, f"raw cost gate changed at {base+4*i:08X}")
    require(all(static_cost(word) == 1 for word in leaf+relocation+sampler), "leaf/relocation/sampler table cycle cost changed")
    wrapper_cost = sum(static_cost(word) for word in wrapper)
    require(wrapper_cost == 13, "wrapper cost changed; mtspr/mtlr is two table cycles")
    # Fixed semantic route: aligned address, all-zero fill, size>=32, size%4=0.
    fixed = [*range(0x8000543C, 0x8000545C, 4), 0x8000546C, 0x80005470,
             0x8000548C, 0x80005490, 0x80005494, 0x800054C0, 0x800054C4,
             0x800054D4, 0x800054D8, 0x800054DC, 0x800054E0]
    require(len(fixed) == 19, "fixed zero-fill route cardinality")
    profiles = []
    for i in range(3):
        address, size = dol.word(0x800055C8 + 8*i), dol.word(0x800055CC + 8*i)
        require(address % 4 == 0 and size >= 32 and size % 4 == 0, "admitted CRT fill route changed")
        groups, remaining = size >> 5, (size >> 2) & 7
        profiles.append({"address": f"{address:08X}", "size": size, "groups": groups, "remaining_words": remaining,
                         "leaf_cost": 19 + 10*groups + 3*remaining, "wrapper_cost": wrapper_cost})
    return {"raw_words": len(leaf)+len(wrapper)+len(relocation)+len(sampler),
            "zero_fill_profiles": profiles, "leaf_total": sum(p["leaf_cost"] for p in profiles),
            "leaf_and_wrapper_total": sum(p["leaf_cost"]+p["wrapper_cost"] for p in profiles),
            "bi2_relocation_cost_per_item": 5,
            "native_elapsed_established": False,
            "limits": ["not full-prefix accounting", "active hooks not established", "table cycles are not physical timing",
                       "event interleavings and progressively visible memory not established"]}


def f32(value: float) -> float:
    return struct.unpack("<f", struct.pack("<f", value))[0]


def signed(value: int, bits: int) -> int:
    value &= (1 << bits) - 1
    return value - (1 << bits) if value >> (bits-1) else value


def down_to_cycles(down: int, inverse: float) -> int:
    value = f32(f32(float(down)) * inverse)
    require(math.isfinite(value) and -(1 << 31) <= value < (1 << 31), "unproved float-to-int timing conversion")
    return int(value)


def tb(epoch: int, origin_cycles: int, cycles: int) -> int:
    return (epoch + (((cycles-origin_cycles) & MASK) // 12)) & MASK


def falsifications() -> dict:
    probes = []
    # Equal counter samples hide the residue; no TB->epoch inversion is unique.
    for residue in range(1, 12):
        now = 1000
        left, right = now, now-residue
        require(tb(0, left, now) == tb(0, right, now), "phase counterexample precondition")
        future = now + 12-residue
        require(tb(0, left, future) != tb(0, right, future), "phase counterexample did not diverge")
        probes.append(f"TB-equivalent origins diverge at residue {residue}")
    # Same prefix effects, total cost, and final time do not imply sample schedule.
    block_reads = [tb(0, 90, 100)] * 3
    stepped_reads = [tb(0, 90, c) for c in (100, 101, 102)]
    require(block_reads == [0, 0, 0] and stepped_reads == [0, 0, 1], "block-vs-step counterexample")
    probes.append("same-block sampling differs from read-before-unit-retirement stepping")
    # On debugger return, five executed operations have not retired; continuing
    # forces one unit step and discards those local block cycles.
    continuous_cost, dense_stop_cost = 5+1+1, 1+1
    require(continuous_cost != dense_stop_cost, "breakpoint loss counterexample")
    probes.append("breakpoint discards local block cost; forced step cannot restore it")
    require(3+1 != 1+1, "multi-cycle operation counterexample")
    probes.append("SingleStep ignores a three-cycle mulli table cost")
    # Merely moving an otherwise identical clock read past a retired branch
    # changes its sample without changing the final block-cost sum.
    require(tb(0, 90, 100) != tb(0, 90, 105), "retirement placement counterexample")
    probes.append("equal total cycles with different retirement boundary changes observed TB")
    # The source ForceExceptionCheck equations preserve GetTicks at OC=1.
    global_timer, sl, dc, future = 100, 100, 80, 5
    before = global_timer+sl-dc
    new_sl, new_dc = sl-(dc-future), future
    require(before == global_timer+new_sl-new_dc, "unit-factor force projection")
    # Truncation breaks that inference under a valid nonintegral OC setting.
    oc, inverse = f32(1.5), f32(1.0/f32(1.5))
    sl, dc, future = 100, 100, 1
    before = sl-down_to_cycles(dc, inverse)
    new_sl = sl-(down_to_cycles(dc, inverse)-future)
    new_dc = int(f32(f32(float(future))*oc))
    after = new_sl-down_to_cycles(new_dc, inverse)
    require((before, after) == (34, 35), "nonintegral OC force counterexample")
    probes.append("ForceExceptionCheck is not exact elapsed preservation at OC=1.5")
    # Events with the same due time are ordered by fifo_order, not event name.
    def fifo_result(order):
        owned, read = 0, None
        for name in order:
            if name == "store": owned = 1
            else: read = owned
        return read
    require(fifo_result(("store", "read")) != fifo_result(("read", "store")), "FIFO counterexample")
    probes.append("equal-time callback FIFO ordering changes downstream state")
    # Callbacks may mutate memory without raising a CPU exception.
    pending_before = pending_after = 0
    mem_before, mem_after = 0, 7
    require(pending_before == pending_after and mem_before != mem_after, "pending-flags counterexample")
    probes.append("pending architectural exceptions=0 does not prove absent event effects")
    # A complete block overshoots the current slice; callback cyclesLate differs
    # from a per-instruction retirement substitute.
    require(13-5 == 8 and 5-5 == 0, "slice-lateness counterexample")
    probes.append("block overshoot and per-op ticking produce different callback lateness")
    # Off-thread relative timeout is added to pre-advance committed global time.
    committed, elapsed, relative = 100, 20, 5
    actual_due, guessed_due = committed+relative, committed+elapsed+relative
    require(actual_due == 105 and guessed_due == 125, "thread-ingress counterexample")
    probes.append("MoveEvents timeout uses pre-advance global time, not current GetTicks")
    # An event at the first overdue group must see the first written group only.
    progressive, eager = [0]*8+[1]*8, [0]*16
    require(progressive[8] != eager[8], "eager fill counterexample")
    probes.append("bulk eager zero-fill exposes future bytes to a due event")
    # A replaced operation uses previous-instruction metadata, not current code.
    require(3 != 1, "HLE previous-instruction cost counterexample")
    probes.append("HLE replacement cost depends on previous fetched operation")
    return {"falsified_assumptions": len(probes), "probes": probes,
            "scope": "finite arithmetic counterexamples, not emulation or native production"}


def event_rows(path: Path) -> tuple[list[dict], bool]:
    rows = []
    filtered = False
    last_seq = 0
    for line_no, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        require(line != "LIMIT", f"truncated event log at {line_no}")
        columns = line.split("\t")
        require(len(columns) == 16, f"event schema mismatch at {line_no}")
        seq = int(columns[0])
        if seq == 0 and columns[2] == "event-filter":
            filtered = True
            continue
        require(seq > last_seq, f"non-increasing event ordinal at {line_no}")
        if last_seq and seq != last_seq+1:
            filtered = True
        last_seq = seq
        values = tuple(int(value, 16) for value in columns[4:12])
        require(all(0 <= value <= MASK for value in values), "event value outside uint64")
        rows.append({"seq": seq, "thread": int(columns[1]), "kind": columns[2], "name": columns[3], "v": values})
    require(rows, "empty event trace")
    return rows, filtered


def check_event_row(row: dict) -> int:
    v, kind = row["v"], row["kind"]
    if kind == "clock-read":
        require(v[3] in (0, 1), "unknown timer sane mode")
        inverse = struct.unpack("<f", struct.pack("<I", v[6]))[0]
        expected = v[0]
        if not v[3]:
            expected = (expected + signed(v[1], 32)-down_to_cycles(signed(v[2], 32), inverse)) & MASK
        require(v[7] == expected, f"clock-read arithmetic mismatch at {row['seq']}")
        return 1
    if kind == "retire":
        require(v[3] > 0, "zero retirement block count")
        require(signed(v[0], 32)-signed(v[1], 32) == v[2], f"retirement arithmetic mismatch at {row['seq']}")
        return 1
    if kind == "step-reset":
        require(v[1:3] == (1, 0), f"SingleStep reset mismatch at {row['seq']}")
        return 1
    return 0


def event_audit(path: Path) -> dict:
    rows, filtered = event_rows(path)
    checks = sum(check_event_row(row) for row in rows)
    rejects = 0
    for row in rows:
        index = {"clock-read": 7, "retire": 2, "step-reset": 1}.get(row["kind"])
        if index is None:
            continue
        mutant = dict(row)
        v = list(row["v"])
        v[index] ^= 1
        mutant["v"] = tuple(v)
        try:
            check_event_row(mutant)
        except ValueError:
            rejects += 1
        else:
            raise ValueError("event arithmetic mutation escaped")
    kinds = collections.Counter(row["kind"] for row in rows)
    return {"sha256": sha(path.read_bytes()), "rows": len(rows), "kinds": dict(sorted(kinds.items())),
            "local_arithmetic_checks": checks, "rejected_mutations": rejects, "filtered_or_sequence_gaps": filtered,
            "native_elapsed_established": False,
            "limits": ["local arithmetic is not derivation of executed work", "initial queues and callback effects not proven",
                       "block summaries do not recover each raw operation or active hook", "debugger stops cannot validate uninterrupted timing"]}


def instruction_audit(path: Path, dol: Dol, source_root: Path, first_clock_only: bool = False) -> dict:
    """Evidence-stream checks, not execution of the observed operations."""
    cost_of = source_cost_reader(source_root)
    ordinal = operations = total_cost = blocks = block_cost = 0
    expected_kind = "hook"
    hook = enter = previous_exit = initial = None
    first_clock = None
    seen = set()
    seen_exits = set()
    mutations = 0
    leaf_index = 0
    leaf_route = None
    leaf_cost = 0
    leaf_costs = []

    def end_block(word):
        op, xo = word >> 26, (word >> 1) & 1023
        # Actual interpreter effects for this finite exception-free route.
        # PPCTables FL_ENDBLOCK on mtspr does NOT end an interpreter block.
        return int(op in (16, 18) or op == 19 and xo in (16, 528) or op == 31 and xo == 146)

    def local_checks(row, expected_hook=None, expected_enter=None):
        kind, pc, word, cost, detail, resulting_pc, npc, ticks, global_timer, sl, dc, inverse, msr, exceptions = row
        require(inverse == 0x3F800000 and exceptions == 0, "unsupported timing mode or pending exception")
        require(ticks == ((global_timer + signed(sl, 32)-signed(dc, 32)) & MASK), "instruction timing tuple mismatch")
        if kind == "hook":
            require(word == cost == detail == 0, "active HLE hook is an unresolved effect/cost owner")
            require(resulting_pc == pc, "hook observation occurs at another PC")
        elif kind == "enter":
            require(expected_hook is not None and expected_hook[0] == "hook", "operation lacks hook decision")
            require(pc == expected_hook[1] and resulting_pc == pc and npc == (pc+4) & 0xFFFFFFFF,
                    "enter PC/NPC/control mismatch")
            require(row[7:] == expected_hook[7:], "hook-to-enter timing/state changed")
            require(word == dol.word(pc), "fetched word differs from original DOL")
            require(cost == cost_of(word) and detail == 0, "entered cost or block state mismatch")
        elif kind == "exit":
            require(expected_enter is not None and expected_enter[0] == "enter", "operation exit lacks entry")
            require((pc, word, cost) == expected_enter[1:4], "exit original PC/word/cost mismatch")
            require(resulting_pc == npc, "exit PC/NPC mismatch")
            require(row[7:12] == expected_enter[7:12], "operation changed timing state")
            require(exceptions == expected_enter[-1], "operation changed pending exception state")
            require(detail == end_block(word), "actual interpreter block-end mismatch")
            op, xo = word >> 26, (word >> 1) & 1023
            if not (op == 31 and xo == 146):
                require(msr == expected_enter[-2], "unexpected MSR mutation")
            if op == 18:
                displacement = signed(word & 0x03FFFFFC, 26)
                target = (displacement + (0 if word & 2 else pc)) & 0xFFFFFFFF
                require(resulting_pc == target, "direct branch target mismatch")
            elif op == 16:
                displacement = signed(word & 0xFFFC, 16)
                target = (displacement + (0 if word & 2 else pc)) & 0xFFFFFFFF
                require(resulting_pc in (target, (pc+4) & 0xFFFFFFFF), "conditional branch successor mismatch")
            elif not (op == 19 and xo in (16, 528)):
                require(resulting_pc == (pc+4) & 0xFFFFFFFF, "nonbranch successor mismatch")
        else:
            raise ValueError("unknown instruction observer row or HLE replacement")

    def fill_route(size):
        yield from range(0x8000543C, 0x8000545C, 4)
        yield from (0x8000546C, 0x80005470, 0x8000548C, 0x80005490, 0x80005494)
        for _ in range(size >> 5):
            yield from range(0x80005498, 0x800054C0, 4)
        yield from (0x800054C0, 0x800054C4)
        for _ in range((size >> 2) & 7):
            yield from (0x800054C8, 0x800054CC, 0x800054D0)
        yield from (0x800054D4, 0x800054D8, 0x800054DC, 0x800054E0)

    complete = False
    with path.open(encoding="utf-8") as stream:
        for line in stream:
            parts = line.split()
            require(len(parts) == 15 and line.endswith("\n"), "truncated or malformed instruction trace")
            require(int(parts[0]) == ordinal+1, "instruction ordinal gap/reordering")
            ordinal += 1
            row = (parts[1], *(int(value, 16) for value in parts[2:]))
            require(row[0] == expected_kind, "hook/enter/exit pairing mismatch")
            local_checks(row, hook, enter)
            pc = row[1]
            if row[0] == "hook":
                if initial is None:
                    require(pc == 0x80003154, "wrong passive entry")
                    initial = row
                if previous_exit:
                    require(pc == previous_exit[5], "next operation differs from previous successor")
                    delta = (row[7]-previous_exit[7]) & MASK
                    if previous_exit[4]:
                        require(delta == block_cost, "retired block cost does not produce next observed time")
                        blocks += 1
                        block_cost = 0
                    else:
                        require(delta == 0, "time advanced inside uninterrupted block")
                    require(row[-2:] == previous_exit[-2:], "state changed across operation boundary")
                hook = row
                expected_kind = "enter"
                if pc == 0x80379628 and first_clock is None:
                    require(block_cost == 0, "clock frontier retains unretired caller block")
                    first_clock = {"operations": operations, "table_cost": total_cost, "retired_blocks": blocks,
                                   "entry_ticks": f"{initial[7]:016X}", "frontier_ticks": f"{row[7]:016X}",
                                   "elapsed": (row[7]-initial[7]) & MASK}
                    require(first_clock["elapsed"] == total_cost, "first-clock cost sum differs from elapsed output")
                    if first_clock_only:
                        complete = True
                        break
            elif row[0] == "enter":
                enter = row
                operations += 1
                total_cost += row[3]
                block_cost += row[3]
                expected_kind = "exit"
                if pc == 0x8000543C:
                    require(leaf_route is None and leaf_index < 3, "unexpected CRT fill call")
                    size = dol.word(0x800055CC + 8*leaf_index)
                    leaf_route = iter(fill_route(size))
                    leaf_cost = 0
                if leaf_route is not None:
                    require(pc == next(leaf_route, None), "CRT fill semantic route/count differs from descriptor")
                    leaf_cost += row[3]
                if pc not in seen:
                    seen.add(pc)
                    # Local mutations do not touch an evidence file or CPU state.
                    for field in (2, 3, 4, 10, 11, 12, 13):
                        mutant = list(row)
                        mutant[field] ^= 1
                        try:
                            local_checks(tuple(mutant), hook, None)
                        except ValueError:
                            mutations += 1
                        else:
                            raise ValueError("raw/cost/timing/control mutation escaped")
            elif row[0] == "exit":
                if pc not in seen_exits:
                    seen_exits.add(pc)
                    for field in (3, 4, 5):
                        mutant = list(row)
                        mutant[field] ^= 1
                        try:
                            local_checks(tuple(mutant), hook, enter)
                        except ValueError:
                            mutations += 1
                        else:
                            raise ValueError("exit cost/block/successor mutation escaped")
                if pc == 0x800054E0 and leaf_route is not None:
                    require(next(leaf_route, None) is None, "CRT leaf terminated before expected work")
                    size = dol.word(0x800055CC + 8*leaf_index)
                    expected = 19+10*(size >> 5)+3*((size >> 2) & 7)
                    require(leaf_cost == expected, "CRT finite cost formula disagrees with trace")
                    leaf_costs.append(leaf_cost)
                    leaf_route = None
                    leaf_index += 1
                previous_exit = row
                expected_kind = "hook"
                if pc == 0x80373AC0:
                    require(row[5] == 0x80373AC4 and row[4] == 0, "wrong final successor/block state")
                    require(stream.read() == "", "unexplained rows after passive final boundary")
                    complete = True
                    break
    require(complete and first_clock is not None and leaf_index == 3, "incomplete passive prefix trace")
    require(sum(leaf_costs) == 153597, "independent descriptor fill totals differ")
    result = {"sha256": sha(path.read_bytes()), "scope": "bounded entry-to-first-clock" if first_clock_only else "entry-to-final-exit80373AC0",
              "rows": ordinal, "operations": operations, "distinct_original_pcs": len(seen), "table_cost": total_cost,
              "retired_blocks_observed": blocks, "pending_block_cost_at_stop": block_cost,
              "first_clock": first_clock, "leaf_costs": leaf_costs, "raw_cost_state_control_mutations_rejected": mutations,
              "native_elapsed_established": False,
              "limits": ["trace is validation output only", "all observed HLE decisions are absent in this admitted profile",
                         "event effects need separate queue/device replay", "no physical Gekko timing equivalence"]}
    if not first_clock_only:
        require(((previous_exit[7]-initial[7]) & MASK) == total_cost-block_cost, "final time fails completed-block accounting")
        result["elapsed_to_stop"] = (previous_exit[7]-initial[7]) & MASK
    return result


def capture_audit(path: Path, trace: Path, trace_result: dict) -> dict:
    capture = json.loads(path.read_text(encoding="utf-8"))
    region = capture["region_extension"]
    manifest = capture["instrumentation_manifest"]
    require(manifest["purpose"] == "passive original prefix timing/HLE observer; no native dependency", "wrong passive observer provenance")
    require(capture["dolphin_sha256"] == manifest["instrumented_executable_sha256"], "capture executable identity mismatch")
    require(manifest["passive_interpreter"]["original_sha256"] == PINS["PowerPC/Interpreter/Interpreter.cpp"], "wrong original interpreter source")
    require(capture["instruction_trace"] == trace.name and capture["instruction_trace_sha256"] == trace_result["sha256"], "capture/trace association mismatch")
    require(region["sampler_mode"] == "continuous" and region["stop"].lower() == "80373ac4", "stale or wrong sampler schedule metadata")
    require(capture["sole_internal_stop"].lower() == "80373ac4" and not capture["controlled_midchain_writes"], "unexpected internal stop/control")
    require(region["producer_mode"] == "rtc_initial" and not region["epoch_controls"] and region["requested_epoch"] is None, "unexpected source-epoch forcing")
    for key in ("clock_preentry_writes", "clock_plan", "bi2_preentry_writes"):
        require(not region[key], "unexpected input forcing " + key)
    require(region["midchain_writes"] is False, "midchain forcing present")
    require(len(capture["checkpoints"]) == 2, "unexpected checkpoint schedule")
    initial, final = capture["checkpoints"]
    require(initial["pc"].lower() == "80003154" and final["pc"].lower() == "80373ac4", "wrong captured boundaries")
    require(initial["hid0"].lower() == capture["controlled_initial_hid0"].lower() == "0011c064", "HID0 control not acknowledged at entry")
    require(initial["l2cr"].lower() == capture["controlled_initial_l2cr"].lower(), "L2 control not acknowledged at entry")
    require(int(initial["l2cr"], 16) & ~0xC0480000 == 0, "unproved L2 mode")
    source = initial["clock_producer"]
    require(source == capture["original_unmodified_entry"]["clock_producer"], "pre-entry controls altered source clock")
    require(all(sample == source for sample in capture["pause_source_readbacks"]) and len(capture["pause_source_readbacks"]) >= 2,
            "paused pure source readbacks changed or missing")
    source = {k: int(v, 16) for k, v in source.items()}
    output = {k: int(v, 16) for k, v in final["clock_producer"].items()}
    require(source["cpu_hz"] == 486000000 and source["exceptions"] == output["exceptions"] == 0, "unproved clock units/exceptions")
    require(source["epoch_value"] % 40500000 == 0 and source["epoch_value"] // 40500000 < (1 << 32), "not an admitted RTC initial epoch")
    for key in ("epoch_cycles", "epoch_value", "cpu_hz", "rtc_offset"):
        require(output[key] == source[key], "clock producer rebased or changed at " + key)
    point = trace_result["first_clock"]
    require(source["cycles"] == int(point["entry_ticks"], 16), "entry trace/capture timing disagree")
    require(output["cycles"]-source["cycles"] == trace_result["elapsed_to_stop"], "endpoint trace/capture timing disagree")
    sampled = tb(source["epoch_value"], source["epoch_cycles"], int(point["frontier_ticks"], 16))
    require(output["cached_tb"] == sampled, "actual continuous sampled TB differs from initial source production")
    require(initial["clock_low_words"] == final["clock_low_words"], "low-memory clock inputs changed")
    offset = int(initial["clock_low_words"][0], 16) << 32 | int(initial["clock_low_words"][1], 16)
    timestamp = (sampled + offset) & MASK
    require(final["clock_globals"] == [f"{timestamp >> 32:08x}", f"{timestamp & 0xFFFFFFFF:08x}"], "timestamp globals differ from exact unsigned64 addition")
    return {"sha256": sha(path.read_bytes()), "trace_sha256": trace_result["sha256"],
            "pause_seconds": capture["pause_entry_seconds"], "sampled_tb": f"{sampled:016X}",
            "timestamp": f"{timestamp:016X}", "same_run_initial_and_endpoint_source_parity": True,
            "native_elapsed_established": False,
            "limits": ["does not reconstruct all architectural/guest-memory state", "event/device ownership still separate",
                       "manifest hash association is not a new linker proof", "no physical timing equivalence"]}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dol", type=Path, required=True, help="read-only original PAL DOL")
    parser.add_argument("--source-root", type=Path, required=True, help="matching read-only private source tree")
    parser.add_argument("--event-log", type=Path, action="append", default=[])
    parser.add_argument("--instruction-trace", type=Path, action="append", default=[])
    parser.add_argument("--capture", type=Path, action="append", default=[], help="Passive provenance and source-output parity, paired with instruction traces")
    parser.add_argument("--first-clock-only", action="store_true", help="Audit only the complete bounded interval before the first mftbu; never claim full tail evidence")
    args = parser.parse_args()
    require(not args.capture or len(args.capture) == len(args.instruction_trace) and not args.first_clock_only,
            "captures must pair with complete instruction traces")
    traces = [instruction_audit(path, Dol(args.dol), args.source_root, args.first_clock_only) for path in args.instruction_trace]
    result = {"scope": "independent research42 adversarial audit", "production_frontier": "before80379628",
              "source": source_audit(args.source_root), "static_cost": static_cost_audit(Dol(args.dol), args.source_root),
              "falsifications": falsifications(), "event_logs": [event_audit(path) for path in args.event_log],
              "instruction_traces": traces,
              "captures": [capture_audit(cap, trace, result) for cap, trace, result in zip(args.capture, args.instruction_trace, traces)],
              "connected_clock_admitted": False}
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()

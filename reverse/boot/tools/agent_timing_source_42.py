"""Pinned event/timing observer audit; never a runtime clock provider.

Requires original source/library pins and complete lifecycle rows from init
through the first executed mftbu. It replays clock and queue mutations as an
evidence consistency check. It cannot prove instruction execution from sums.
"""
import argparse
from collections import Counter
import hashlib
import importlib.util
import json
from pathlib import Path
import re

MASK = (1 << 64) - 1
UNIT = 0x3f800000
EXTRA_PINS = {
    "Source/Core/Core/CoreTiming.h": "fdfab0fbb4bdc953a946f746012636b6005a96d875d82ad3263543cacaa7ea7b",
    "Source/Core/Core/OracleEventAudit.h": "781b5f3e344877582d2e7b4506d9fc0f7c12e1ada672fe75fc9c7b9f432a036d",
    "Source/Core/Core/HW/VideoInterface.cpp": "f1dc11d057c8a52e47316580713d27d9e8f6458e84726f45e7c648bdbfd92295",
    "Source/Core/Core/HW/AudioInterface.cpp": "bc698352d4276b63ba99629adf0987b8e8e252b5034ef76e4701717709d76b71",
    "Source/Core/Core/HW/DSPHLE/DSPHLE.cpp": "e1059fccb22935e2e71dd93090836e1358c0fd8dc9e087b3ebf4da7b2122cc8c",
    "Source/Core/Core/HW/DSPLLE/DSPLLE.cpp": "5520fe7ad302db9cba7962ac589601cebff881e4804fc75f672eca32a1402f8e",
    "Source/Core/AudioCommon/Mixer.h": "612025371230d433dcf62e027d1abd2da8be5c3533a8ed2947a0e3b6f9480bd5",
    "Source/Core/Core/PowerPC/PPCTables.cpp": "4814b6c63088c0227eadf2fefa4f32fe2109afde7319b88697b35a7b67dcf771",
    "Source/Core/Core/PowerPC/Interpreter/Interpreter_Branch.cpp": "ac92c3fc9f5d05b7bc6fea7a39bdb0682c13d80223fc8ca88131a55a6df29068",
    "Source/Core/Core/ConfigManager.cpp": "3418e4660cfd4baf1d8d570e6dd7bc18779499c7310ad45684da1914413b1912",
}
EXTRA_PINS.update({"Source/Core/Core/HW/DSP.cpp":"110d30677b3bf63e0bd8043377a566d7c3625f312ed76c09824f08796b2e62dd","Source/Core/Core/HW/DSP.h":"9e98d77bd6af4d462a39d75e0532102b3fc942d95e76b8400977f2ca53b2aecc","Source/Core/Core/HW/DVD/DVDInterface.cpp":"b017c9ce65bd788be55c3310d2a0f4cb8687f2a3a7f0aa674131548e45997da1","Source/Core/Core/HW/ProcessorInterface.cpp":"62310e34368b1d61fff9b9dee7848ab8f1e0125edef4015f8ab3cd3fbb7e6186","Source/Core/Core/HW/ProcessorInterface.h":"3dc6a462d4de00429cffcec4b6c3464ced1cf24a27d02a6c27ffa2ab5fb73763","Source/Core/Core/HW/MMIO.h":"7168d2bd96224d29d85e99e8972aa991ae5608136e82ce3eab25fb730649d94c","Source/Core/Core/PowerPC/GDBStub.cpp":"c2e3ae4869d41eb212bf9fb08940958f17939a4a2f19d5354ec4e386d74ba35e"})
EXTRA_PINS.update({
    "Source/Core/Core/Boot/Boot.cpp": "2245b1ab41fa4ab092d26416016c05d52b5591c9035f2ed4b1e3ffe8d256daaa",
    "Source/Core/Core/HW/DSPHLE/UCodes/ROM.cpp": "16900823dc3d2b70f5413b030869e0507f3f68a3e6a8755101b94b3b00369814",
    "Source/Core/AudioCommon/AudioCommon.cpp": "de119dfd09a5f10309bbdd0b2de83f5666a8c2f413bf87143ab3322d7b527430",
    "Source/Core/VideoCommon/Fifo.cpp": "971d31caf503e18e9435202aa7f576a0ab281630a127e22a42063ab30795a15e",
    "Source/Core/Core/HW/SI/SI.cpp": "4dd9a3b22f3610d7ffe7704b6ee5b4142f29ad6e932c391edc094b22ac6abae7",
    "Source/Core/Core/HW/StreamADPCM.h": "f1159e7c6df1c4058e6854ad2dd32640ab4d970f34875283bf5afe90f7b395b0",
})
REQUIRED = {"init", "init-ready", "config", "register", "advance-enter", "advance-clock",
            "advance-exit", "clock-read", "schedule-clock", "force-enter", "force-exit",
            "enqueue", "dispatch", "callback-return", "ts-publish", "ts-commit", "ts-move",
            "cancel", "cancel-item", "cancel-all", "clear", "adjust", "adjust-item",
            "idle-enter", "idle-exit", "step-enter", "step-reset", "boot-steps", "retire",
            "boundary", "ppc", "unproven-savestate"}
QUEUE_KINDS = {"enqueue", "dispatch", "ts-move", "cancel-item", "adjust-item"}
CLOCK_KINDS = {"init", "init-ready", "clock-read", "schedule-clock", "advance-enter",
               "advance-clock", "advance-exit", "force-enter", "force-exit",
               "idle-enter", "idle-exit"}

def check(value, message):
    if not value:
        raise ValueError(message)

def signed(value):
    return value - (1 << 64) if value >> 63 else value

def hash_file(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def accepts(spec, kind):
    tokens = spec.split(",") if spec else []
    include, exclude = [], []
    for token in tokens:
        negative = token.startswith("-")
        item = token[1:] if negative else token
        (exclude if negative else include).append(item)
    def match(pattern):
        return kind.startswith(pattern[:-1]) if pattern.endswith("*") else pattern == kind
    return (not include or any(match(p) for p in include)) and not any(match(p) for p in exclude)

def read_logs(paths):
    rows, headers, pins = {}, [], []
    for path in paths:
        raw = path.read_bytes()
        pins.append({"name": path.name, "sha256": hashlib.sha256(raw).hexdigest()})
        for line_number, line in enumerate(raw.decode("utf-8").splitlines(), 1):
            check(line != "LIMIT", f"{path.name}:{line_number}: truncated observation")
            parts = line.split("\t")
            check(len(parts) == 16, f"{path.name}:{line_number}: expected sixteen columns")
            check(re.fullmatch(r"[0-9]+", parts[0]) is not None, "invalid sequence")
            check(re.fullmatch(r"[0-9]+", parts[1]) is not None, "invalid thread")
            check(all(re.fullmatch(r"[0-9a-fA-F]{16}", p) for p in parts[4:12]),
                  "invalid u64 observer field")
            row = {"seq": int(parts[0]), "thread": int(parts[1]), "kind": parts[2],
                   "name": parts[3], "v": tuple(int(p, 16) for p in parts[4:12]),
                   "source": parts[12], "line": int(parts[13]), "function": parts[14]}
            if row["kind"] == "event-filter":
                check(row["seq"] == 0, "filter header sequence is not zero")
                headers.append(row["name"])
                continue
            check(row["seq"] > 0, "zero data sequence")
            if row["seq"] in rows:
                check(rows[row["seq"]] == row, "sink overlap contains altered record")
            else:
                rows[row["seq"]] = row
    check(rows, "empty event evidence")
    # A complete startup sink must admit every mutation from initialization;
    # a detailed later sink cannot repair missing startup ancestry.
    if headers:
        complete = False
        for header in headers:
            fields = dict(p.split("=", 1) for p in header.split(";"))
            if fields.get("window") in ("", "init..ppc@80379628", "init..ppc@8037962c", "init..ppc@80379630", "init..ppc@80373ac4", "init..timing-prefix-end@80373ac4"):
                complete |= all(accepts(fields.get("kinds", ""), k) for k in REQUIRED)
        check(complete, "no startup sink admits complete timing/queue lifecycle")
    else:
        numbers = sorted(rows)
        check(numbers == list(range(1, numbers[-1] + 1)), "unfiltered stream has sequence gaps")
    return [rows[k] for k in sorted(rows)], pins, headers

class Audit:
    def __init__(self):
        self.state = None
        self.fifo = 0
        self.queue = {}
        self.registered = set()
        self.configured = False
        self.init_ready = False
        self.advancing = None
        self.forcing = None
        self.schedule = {}
        self.callbacks = []
        self.cancelled = set()
        self.adjustments = {}
        self.ts = []
        self.counts = Counter()
        self.entry = None
        self.endpoint = None
        self.endpoint_pending = None
        self.retired_at_endpoint = None
        self.dispatched_interval = Counter()
        self.retired_cycles = 0
        self.boot_cycles = 0
        self.boot_runs = 0

    def ticks(self):
        g, sl, dc, sane = self.state
        return (g if sane else g + sl - dc) & MASK

    def clock_fields(self, v):
        check(v[6] == UNIT or (not self.init_ready and v[3] == 1 and v[6] == 0),
              "non-unit float conversion lacks reconstructed proof")
        return (signed(v[0]), signed(v[1]), signed(v[2]), v[3])

    def same(self, v):
        check(self.clock_fields(v) == self.state, "unexplained clock-state transition")
        check(v[4] == self.fifo, "unexplained event FIFO transition")

    def event(self, row):
        v = row["v"]
        return (signed(v[0]), v[1], row["name"], v[2])

    def front(self):
        return min(self.queue.values(), key=lambda e: e[:2]) if self.queue else None

    def slice(self, g):
        return min(self.front()[0] - g, 20000) if self.queue else 20000

    def run(self, rows):
        for row in rows:
            kind, v, name, thread = row["kind"], row["v"], row["name"], row["thread"]
            self.counts[kind] += 1
            try:
                self.accept(row)
            except ValueError as error:
                raise ValueError(f"record {row['seq']} {kind}: {error}") from error
        check(self.entry is not None, "missing DOL entry boundary")
        check(self.endpoint is not None, "missing executed first-clock anchor")
        check(not self.advancing and not self.forcing and not self.callbacks,
              "unfinished timing/callback transition")
        check(not self.cancelled and not self.adjustments, "incomplete queue mutation")
        check(not any(not item["committed"] for item in self.ts),
              "incomplete foreign-thread publication")
        return {"scope": "source-conditioned observer consistency; no native timing admission",
                "entry": self.entry, "first_clock": self.endpoint,
                "elapsed_entry_to_clock": self.endpoint["ticks"] - self.entry["ticks"],
                "phase_mod12": self.endpoint["ticks"] % 12,
                "compressed_boot_unit_cycles": self.boot_cycles,
                "compressed_boot_runs": self.boot_runs,
                "retired_block_cycles_before_first_clock": self.retired_at_endpoint,
                "retired_block_cycles_in_complete_stream": self.retired_cycles,
                "interval_dispatch_counts": dict(self.dispatched_interval),
                "pending_at_first_clock": [self.describe(e) for e in self.endpoint_pending],
                "record_counts": dict(self.counts),
                "unresolved": ["independent native ownership of reconstructed boot/work producers",
                               "native instruction/block retirement equivalence",
                               "debugger partial-block cycle loss and forced-step schedule",
                               "callback device/interrupt and foreign-thread input semantics"]}

    @staticmethod
    def describe(event):
        time, fifo, name, userdata = event
        return {"time": time, "fifo": fifo, "name": name, "userdata": f"{userdata:016x}"}

    def accept(self, row):
        kind, v, name, thread = row["kind"], row["v"], row["name"], row["thread"]
        if kind.startswith("unproven-"):
            raise ValueError("unproven state producer")
        if kind == "init":
            check(self.state is None, "second initialization lacks object lifecycle proof")
            check(v[:6] == (0, 20000, 0, 1, 0, 0), "fresh initialization contract differs")
            check(v[6] == 0, "initial inverse factor is not zero")
            self.state = (0, 20000, 0, 1)
            return
        if self.state is None:
            check(kind not in REQUIRED, "timing evidence does not begin with init")
            return
        if self.cancelled and kind != "cancel-item":
            raise ValueError("cancel-item set is incomplete")
        if self.adjustments and kind != "adjust-item":
            raise ValueError("adjust-item set is incomplete")
        if kind == "config":
            check(v[:2] == (UNIT, UNIT), "non-unit clock config unsupported")
            self.configured = True
        elif kind == "init-ready":
            check(self.configured, "missing config producer")
            self.same(v)
            self.init_ready = True
        elif kind == "register":
            check(name not in self.registered, "duplicate event registration")
            self.registered.add(name)
        elif kind == "clock-read":
            self.same(v)
            check(v[7] == self.ticks(), "GetTicks equation differs")
        elif kind == "advance-enter":
            self.same(v)
            check(self.advancing is None, "recursive Advance requires new proof")
            self.advancing = self.state
        elif kind == "advance-clock":
            check(self.advancing is not None, "Advance commit lacks entry")
            old_g, sl, dc, _ = self.advancing
            expected = (old_g + sl - dc, 20000, dc, 1)
            check(self.clock_fields(v) == expected, "Advance elapsed-cycle commit differs")
            check(signed(v[7]) == sl - dc and v[4] == self.fifo, "Advance cycle/FIFO differs")
            self.state = expected
        elif kind == "advance-exit":
            g, _, dc, sane = self.state
            check(self.advancing is not None and sane == 1, "Advance exit lacks commit")
            check(not self.callbacks, "callback did not return")
            sl = self.slice(g)
            check(sl >= 1, "undispatched due event")
            check(self.clock_fields(v) == (g, sl, sl, 0), "next slice producer differs")
            check(v[4] == self.fifo, "next slice FIFO differs")
            self.state, self.advancing = (g, sl, sl, 0), None
        elif kind == "schedule-clock":
            self.same(v)
            check(thread not in self.schedule, "nested schedule producer")
            self.schedule[thread] = (self.ticks(), signed(v[7]))
        elif kind == "force-enter":
            self.same(v)
            check(self.forcing is None, "nested force producer")
            self.forcing = (self.state, max(0, signed(v[7])))
        elif kind == "force-exit":
            check(self.forcing is not None, "force exit without entry")
            (g, sl, dc, sane), future = self.forcing
            expected = (g, sl - dc + future, future, sane) if dc > future else (g, sl, dc, sane)
            check(self.clock_fields(v) == expected and v[4] == self.fifo,
                  "ForceExceptionCheck changed elapsed phase")
            self.state, self.forcing = expected, None
        elif kind in ("enqueue", "ts-move"):
            check(name in self.registered, "unregistered event producer")
            e = self.event(row)
            check(e[1] == self.fifo and e[1] not in self.queue, "FIFO identity differs")
            if kind == "enqueue":
                check(thread in self.schedule, "enqueue lacks CPU schedule-clock")
                ticks, future = self.schedule.pop(thread)
                check(e[0] == signed((ticks + future) & MASK) and signed(v[3]) == future,
                      "enqueue deadline lacks producer")
            else:
                pending = next((p for p in self.ts if not p["moved"]), None)
                check(pending is not None and
                      (name, signed(v[3]), v[2]) == (pending["name"], pending["relative"], pending["userdata"]),
                      "foreign-thread queue order differs")
                check(e[0] == self.state[0] + signed(v[3]), "foreign event uses wrong old global phase")
                pending["moved"] = True
            self.queue[e[1]], self.fifo = e, self.fifo + 1
        elif kind == "dispatch":
            e = self.event(row)
            check(self.state[3] == 1 and self.advancing is not None, "dispatch outside Advance callback phase")
            check(e == self.front(), "dispatch is not earliest deadline/FIFO")
            check(e[0] <= self.state[0] and signed(v[3]) == self.state[0] - e[0], "dispatch lateness differs")
            del self.queue[e[1]]
            self.callbacks.append(e)
            if self.entry is not None and self.endpoint is None:
                self.dispatched_interval[name] += 1
        elif kind == "callback-return":
            e = self.event(row)
            check(self.callbacks and self.callbacks.pop() == e and v[3] == 0,
                  "callback identity/return differs")
        elif kind == "cancel":
            selected = [e for e in self.queue.values() if e[2] == name]
            self.cancelled = set(selected)
            for e in selected:
                del self.queue[e[1]]
        elif kind == "cancel-item":
            e = self.event(row)
            check(e in self.cancelled, "unexplained cancelled queue identity")
            self.cancelled.remove(e)
        elif kind == "cancel-all":
            pass  # Source then moves thread queue and issues normal cancel/item records.
        elif kind == "clear":
            self.queue.clear()
        elif kind == "adjust":
            new, old, ticks = v[:3]
            check(ticks == self.state[0] and old and new, "clock adjustment source invalid")
            for e in self.queue.values():
                delta = (e[0] - signed(ticks)) * new
                scaled = abs(delta) // old * (-1 if delta < 0 else 1)
                changed = (signed(ticks) + scaled, e[1], e[2], e[3])
                self.adjustments[e[1]] = changed
        elif kind == "adjust-item":
            e = self.event(row)
            check(self.adjustments.pop(e[1], None) == e, "queue frequency conversion differs")
            self.queue[e[1]] = e
        elif kind == "ts-publish":
            check(name in self.registered, "unregistered foreign-thread producer")
            self.ts.append({"name": name, "relative": signed(v[0]), "userdata": v[2],
                            "thread": thread, "committed": False, "moved": False})
        elif kind == "ts-commit":
            p = next((p for p in self.ts if p["thread"] == thread and not p["committed"]), None)
            check(p is not None and (name, signed(v[0]), v[2]) ==
                  (p["name"], p["relative"], p["userdata"]), "foreign publication commit differs")
            p["committed"] = True
        elif kind == "retire":
            g, sl, dc, sane = self.state
            before, after = signed(v[0]), signed(v[1])
            check(before == dc and before - after == v[2] and v[3] >= 1,
                  "block retirement sum differs")
            self.retired_cycles += v[2]
            self.state = (g, sl, after, sane)
        elif kind == "step-enter":
            check(self.advancing is None, "step starts inside unfinished Advance")
        elif kind == "step-reset":
            check(v[:3] == (self.state[0] & MASK, 1, 0), "single-step reset differs")
            self.state = (self.state[0], 1, 0, 0)
        elif kind == "boot-steps":
            g, sl, dc, sane = self.state
            start, count, first_sl, last_sl, fifo, first_pc, last_pc, unit = v
            check((g, sl, dc, sane) == (start, 1, 0, 0) and count > 0 and unit == 1,
                  "compressed unit recurrence lacks original state")
            check(fifo == self.fifo and self.slice(g + 1) == first_sl and
                  self.slice(g + count) == last_sl and self.slice(g + count) >= 1,
                  "compressed run crosses unexplained event deadline")
            self.boot_cycles += count
            self.boot_runs += 1
            self.state = (g + count, 1, 0, 0)
        elif kind in ("idle-enter", "idle-exit"):
            if kind == "idle-enter":
                self.same(v)
            else:
                g, sl, dc, sane = self.state
                check(self.clock_fields(v) == (g, sl, 0, sane), "idle recurrence differs")
                self.state = (g, sl, 0, sane)
        elif kind == "boundary":
            check((signed(v[1]), signed(v[2]), signed(v[3])) == self.state[:3] and
                  v[4] == self.ticks(), "boundary cycle projection differs")
            if v[0] == 0x80003154:
                check(self.entry is None, "second entry boundary")
                self.entry = {"ticks": self.ticks(), "phase_mod12": self.ticks() % 12,
                              "pending": [self.describe(e) for e in sorted(self.queue.values())]}
        elif kind == "ppc":
            check(signed(v[4]) == self.state[2] and v[5] == self.ticks(),
                  "PPC boundary clock differs")
            if v[0] == 0x80379628:
                check(self.endpoint is None, "second first-clock endpoint")
                self.endpoint = {"ticks": self.ticks(), "global_timer": self.state[0],
                                 "slice": self.state[1], "downcount": self.state[2]}
                self.endpoint_pending = sorted(self.queue.values())
                self.retired_at_endpoint = self.retired_cycles
        elif kind in REQUIRED:
            raise ValueError("unhandled clock/queue mutation")
        # Other rows are retained in inventory but cannot prove their device semantics.



def opcode_costs(source_root):
    text = (source_root / "Source/Core/Core/PowerPC/PPCTables.cpp").read_text(encoding="utf-8")
    tables = {}
    pattern = r"constexpr std::array<GekkoOPTemplate,\s*[0-9]+>\s+(s_[a-z0-9_]+)\{\{(.*?)\}\};"
    entries = r'\{\s*([0-9]+),\s*"([^"]+)",\s*OpType::([A-Za-z]+),\s*([0-9]+),'
    for name, body in re.findall(pattern, text, re.S):
        tables[name] = {int(code): (opname, int(cost), kind)
                        for code, opname, kind, cost in re.findall(entries, body, re.S)}
    check(set(tables) == {"s_primary_table", "s_table4", "s_table4_2", "s_table4_3",
                          "s_table19", "s_table31", "s_table59", "s_table63", "s_table63_2"},
          "opcode table layout changed")
    for target, extra, count, shift in [("s_table4", "s_table4_2", 32, 5),
                                       ("s_table4", "s_table4_3", 16, 6),
                                       ("s_table63", "s_table63_2", 32, 5)]:
        for code, info in tables[extra].items():
            for i in range(count):
                index = (i << shift) + code
                check(index not in tables[target], "opcode table collision")
                tables[target][index] = info
    def decode(word):
        primary = word >> 26
        info = tables["s_primary_table"].get(primary)
        check(info is not None, "unknown primary opcode")
        if info[2] == "Subtable":
            sub = (word >> 1) & (31 if primary == 59 else 1023)
            info = tables["s_table" + str(primary)].get(sub)
            check(info is not None, "unknown secondary opcode")
        check(info[1] > 0, "unknown/zero-cost opcode")
        return info
    return decode

def instruction_audit(path, source_root, expected_elapsed):
    decode = opcode_costs(source_root)
    stage, pending, previous_exit, index = "hook", None, None, 0
    complete, sum_costs, block, profile = 0, 0, 0, Counter()
    first_tb, first_ticks, stop_ticks = None, None, None
    for line in path.read_text(encoding="utf-8").splitlines():
        fields = line.split()
        check(len(fields) == 15, "instruction trace malformed")
        check(re.fullmatch(r"[0-9]+", fields[0]) is not None, "instruction ordinal malformed")
        widths = [8] * 6 + [16] * 3 + [8] * 4
        check(all(re.fullmatch("[0-9a-fA-F]{" + str(width) + "}", text)
                  for width, text in zip(widths, fields[2:])),
              "instruction trace hex field malformed")
        ordinal, kind = int(fields[0]), fields[1]
        pc, word, cost, detail, result_pc, npc, ticks, g, sl, dc, inverse, msr, exceptions = [
            int(x, 16) for x in fields[2:]]
        check(ordinal == index + 1, "instruction trace gap")
        index = ordinal
        check(inverse == UNIT and ticks == g + sl - dc, "instruction timer projection differs")
        check(kind == stage, "hook/enter/exit order differs")
        if stage == "hook":
            check(detail == 0 and word == 0 and cost == 0,
                  "HLE hook producer is not reconstructed")
            check(pc == result_pc and (previous_exit is None or pc == previous_exit),
                  "unexplained instruction PC transition")
            if previous_exit is None:
                check(pc == 0x80003154, "instruction trace does not begin at DOL entry")
                first_ticks = ticks
            pending, stage = (pc, ticks), "enter"
        elif stage == "enter":
            check((pc, ticks) == pending and result_pc == pc and npc == (pc + 4) & 0xffffffff,
                  "fetched instruction phase differs")
            info = decode(word)
            check(cost == info[1], "fetched opcode cost differs from pinned table")
            if pc == 0x80379628 and first_tb is None:
                # Entry runs continuously: the initial Advance commits the final
                # pending boot cycle; the first DOL opcode retires normally.
                produced = sum_costs - block
                check(produced == expected_elapsed and ticks - first_ticks == produced,
                      "opcode retirement does not produce elapsed interval")
                first_tb = {
                    "completed_instructions_before_first_tb": complete,
                    "source_cost_sum": sum_costs,
                    "nonunit_cost_opcodes": {
                        k: n for k, n in profile.items() if k.split(":")[-1] != "1"},
                    "pending_unretired_block_cost": block,
                    "source_derived_elapsed_for_this_reference_schedule": produced,
                    "first_tb_ordinal": ordinal,
                }
            pending, stage = (pc, word, cost, ticks, info[0]), "exit"
        else:
            check((pc, word, cost, ticks) == pending[:4], "instruction exit word/cost/phase differs")
            check(result_pc == npc and detail in (0, 1), "instruction exit state differs")
            complete += 1
            sum_costs += cost
            profile[pending[4] + ":" + str(cost)] += 1
            block += cost
            if detail == 1:
                block = 0
            previous_exit, stage, stop_ticks = result_pc, "hook", ticks
    check(first_tb is not None, "instruction trace lacks first fetched mftbu")
    check(stage == "hook" and previous_exit == 0x80373ac4, "instruction endpoint is not closed")
    check(sum_costs - block == stop_ticks - first_ticks, "closed prefix retirement differs")
    return dict(first_tb, name=path.name, sha256=hash_file(path),
                complete_instructions_in_closed_prefix=complete,
                full_source_cost_sum=sum_costs,
                stop_pending_unretired_block_cost=block,
                stop_ticks=stop_ticks,
                stop_source_derived_elapsed=sum_costs-block,
                scope="pinned cost/observed continuous instruction schedule; native paths still required")


def mmio_audit(path):
    raw = path.read_bytes()
    lines = raw.decode("utf-8").splitlines()
    check(lines and lines[0].split("\t") ==
          ["seq", "kind", "phase", "level", "pc", "lr", "msr", "ticks",
           "a", "b", "c", "d", "hex", "extra"], "MMIO observer schema differs")
    counts = Counter()
    for ordinal, line in enumerate(lines[1:], 1):
        fields = line.split("\t")
        check(len(fields) == 14 and int(fields[0]) == ordinal, "MMIO observation gap")
        check(fields[1] != "LIMIT", "MMIO observation truncated")
        counts[fields[1]] += 1
    return {
        "name": path.name, "sha256": hashlib.sha256(raw).hexdigest(),
        "records": len(lines)-1, "kind_counts": dict(counts),
        "watched_cpu_mmio_writes": counts["wmw"],
        "watched_cpu_mmio_reads": counts["wmr"],
        "scope": "negative access evidence conditional on captured phase0 watch environment",
        "required_watch_ranges": [
            "0c005000:0c005100", "0c002000:0c002100",
            "0c003000:0c003008", "0c006c00:0c006c20"],
        "required_watch_phase0": "1",
    }


def source_locations(rows, source_root, pins, capture):
    private = capture.get("instrumentation_manifest", {}).get("passive_interpreter", {}) if capture else {}
    seen, report = set(), []
    for row in rows:
        kind = row["kind"]
        if kind not in REQUIRED and not kind.startswith("vt-") and kind != "timing-prefix-end":
            continue
        identity = (row["source"], row["line"], kind)
        if identity in seen:
            continue
        seen.add(identity)
        parts = row["source"].replace("\\", "/").split("/")
        tail = "/".join(parts[parts.index("Source"):]) if "Source" in parts else None
        if tail in pins:
            path, label = source_root / tail, tail
        else:
            check(parts[-1] == "Interpreter.cpp" and private, "unbound copied observer source")
            path, label = Path(row["source"]), "<copied-observer>/Interpreter.cpp"
            check(hash_file(path) == private.get("copied_sha256"), "copied observer pin differs")
            check(private.get("original_sha256") == pins["Source/Core/Core/PowerPC/Interpreter/Interpreter.cpp"],
                  "copied interpreter original pin differs")
        source_lines = path.read_text(encoding="utf-8").splitlines()
        check(1 <= row["line"] <= len(source_lines), "observer source line invalid")
        emitting_kind = "step-enter" if kind == "boot-steps" else kind
        check('"' + emitting_kind + '"' in source_lines[row["line"]-1],
              "emitted observer source location differs")
        report.append({"kind": kind, "source": label, "line": row["line"]})
    return report


def callback_projection(rows):
    entry = next(i for i, r in enumerate(rows)
                 if r["kind"] == "boundary" and r["v"][0] == 0x80003154)
    clock = next(i for i, r in enumerate(rows)
                 if r["kind"] == "ppc" and r["v"][0] == 0x80379628)
    before, interval = rows[:entry], rows[entry:clock+1]
    def select(items, kind):
        return [r for r in items if r["kind"] == kind]
    boot_n = sum(r["v"][1] for r in select(before, "boot-steps")) + len(select(before, "step-reset"))
    check(len(select(before, "step-enter")) == len(select(before, "step-reset")),
          "literal boot step unfinished before entry")
    check(rows[entry]["v"][4] == 20000 + boot_n, "boot count does not produce entry ticks")
    for r in select(rows, "vt-interval"):
        check(r["v"][:6] == (0x476901ad, 429, 0, 486000000, 36, 15444),
              "VI period producer differs")
    presets = select(rows, "vt-preset")
    check(len(presets) == 2 and [r["v"] for r in presets] == [
        (0x476901ad, 6, 0x501f6, 0x401f7, 0, 15, 0, 1),
        (0x476901ad, 6, 0x501f6, 0x401f7, 0, 15, 0, 0)], "VI reset ancestry differs")
    updates, advanced = select(rows, "vt-update"), select(rows, "vt-advanced")
    check(len(updates) == len(advanced), "VI update is unfinished")
    for n, (a, b) in enumerate(zip(updates, advanced)):
        next_poll = 15 if n <= 15 else 999
        check(a["v"][:5] == ((n+1)*15444, n, next_poll, 525, 525),
              "VI update recurrence differs")
        check(b["v"][:5] == ((n+1)*15444, n+1, 999 if n >= 15 else 15, 525, 525),
              "VI post-update recurrence differs")
    polls, rebases = select(rows, "vt-poll-next"), select(rows, "vt-rebase")
    check(len(polls) == 1 and polls[0]["v"][:4] == (16*15444, 15, 999, 492),
          "SI poll source phase differs")
    check(len(rebases) == 1 and rebases[0]["v"][:5] == (15444, 0, 15, 525, 525),
          "SI field rebase differs")
    vi_interval = select(interval, "vt-update")
    check([r["v"][1] for r in vi_interval] == list(range(173,183)),
          "bounded VI half-lines differ")
    check(not select(interval, "vt-poll") and not select(interval, "vt-rebase"),
          "bounded VI entered an input/frame branch")
    pre_counts = Counter(r["name"] for r in select(before, "dispatch"))
    interval_counts = Counter(r["name"] for r in select(interval, "dispatch"))
    check(pre_counts == {"VICallback":173, "AudioDMACallback":22, "GPUSleeper":6,
                         "DSPCallback":6, "FinishExecutingCommand":2},
          "new preentry callback requires source reconstruction")
    check(interval_counts == {"VICallback":10, "GDBStubUpdate":1, "AudioDMACallback":1},
          "new bounded callback requires source reconstruction")
    for r in select(rows, "dispatch"):
        check(r["v"][2] == (0x300000001 if r["name"] == "FinishExecutingCommand" else 0),
              "callback userdata producer differs")
    return {
        "boot_unit_steps": boot_n,
        "boot_literal_steps": len(select(before, "step-reset")),
        "boot_compressed_steps": sum(r["v"][1] for r in select(before, "boot-steps")),
        "entry_schedule": "initial continuous Run Advance commits final pending boot cycle",
        "preentry_dispatch_counts": dict(pre_counts),
        "entry_to_clock_dispatch_counts": dict(interval_counts),
        "bounded_vi_preupdate_half_lines": [173,182],
        "vi_half_line_at_stop": advanced[-1]["v"][1],
        "vi_next_si_poll": advanced[-1]["v"][2],
        "vi_last_line_start_at_stop": advanced[-2]["v"][0],
        "boot_si_poll_count": 1,
        "source_conditional_effects": [
            "VI: no interval field, active-line, SI-poll or interrupt comparator branch",
            "Audio: Enable0 from fresh Reinit and excluded control writes gives only external zero buffer",
            "GDB: no packet available gives only periodic queue rescheduling",
            "boot DSP: unchanged ROM ucode has empty Update",
            "boot GPU: only external AllowSleep",
            "boot DTK: AIplayingfalse gives mixer output plus pending_blocks6 and next event",
        ],
        "retained_guest_state": [
            "VI half-line, last-line-start, next-SI-poll and register/interrupt state",
            "PI interrupt cause/mask and external-exception projection",
            "SI boot poll channel data/status from admitted controller input",
            "DSP ROM mail/control and disabled audio-DMA state",
            "DVD pending blocks/deadline; streamstate remains unknown without writer ancestry",
            "stable main event heap with FIFO tie order",
        ],
        "scope": "bounded source predicates; MMIO environment, debugger command discipline and input ancestry still required",
    }

def self_test():
    # A small independent lifecycle exercises initial20k, a late callback,
    # an event-free unit recurrence, a forced DOL step and block retirement.
    rows = []
    def emit(kind, values=(), name="-"):
        rows.append({"seq": len(rows) + 1, "thread": 1, "kind": kind, "name": name,
                     "v": tuple(values) + (0,) * (8 - len(values)),
                     "source": "synthetic", "line": 1, "function": "synthetic"})
    def clock(kind, g, sl, dc, sane, fifo, extra=0, pc=0, inverse=UNIT):
        emit(kind, (g, sl, dc, sane, fifo, pc, inverse, extra))
    clock("init", 0, 20000, 0, 1, 0, inverse=0)
    emit("register", name="A")
    clock("clock-read", 0, 20000, 0, 1, 0, inverse=0)
    emit("config", (UNIT, UNIT))
    clock("init-ready", 0, 20000, 0, 1, 0)
    clock("schedule-clock", 0, 20000, 0, 1, 0)
    clock("clock-read", 0, 20000, 0, 1, 0)
    emit("enqueue", (0, 0, 0, 0), "A")
    emit("step-enter", (0x81200000,))
    clock("advance-enter", 0, 20000, 0, 1, 1)
    clock("advance-clock", 20000, 20000, 0, 1, 1, 20000)
    emit("dispatch", (0, 0, 0, 20000), "A")
    clock("schedule-clock", 20000, 20000, 0, 1, 1, 20000)
    clock("clock-read", 20000, 20000, 0, 1, 1, 20000)
    emit("enqueue", (40000, 1, 0, 20000), "A")
    emit("callback-return", (0, 0, 0, 0), "A")
    clock("advance-exit", 20000, 20000, 20000, 0, 2)
    emit("step-reset", (20000, 1, 0, 0x81200004))
    emit("boot-steps", (20000, 3, 19999, 19997, 2, 0x81200004, 0x81200010, 1))
    emit("step-enter", (0x80003154,))
    clock("advance-enter", 20003, 1, 0, 0, 2)
    clock("advance-clock", 20004, 20000, 0, 1, 2, 1)
    clock("advance-exit", 20004, 19996, 19996, 0, 2)
    emit("boundary", (0x80003154, 20004, 19996, 19996, 20004, 0, 0, 0))
    emit("step-reset", (20004, 1, 0, 0x80003158))
    clock("advance-enter", 20004, 1, 0, 0, 2)
    clock("advance-clock", 20005, 20000, 0, 1, 2, 1)
    clock("advance-exit", 20005, 19995, 19995, 0, 2)
    emit("retire", (19995, 19975, 20, 2, 0x80379620, 0x80379628))
    clock("clock-read", 20005, 19995, 19975, 0, 2, 20025, 0x80379628)
    emit("ppc", (0x80379628, 0, 0, 0, 19975, 20025, 0, 0))
    report = Audit().run(rows)
    check(report["elapsed_entry_to_clock"] == 21 and report["phase_mod12"] == 9,
          "independent lifecycle result differs")
    mutations = [
        (0, 2, 20000), (2, 7, 1), (10, 7, 19999),
        (11, 3, 19999), (14, 0, 40001), (18, 3, 19998),
        (28, 2, 21), (29, 7, 20026), (30, 5, 20026)]
    for index, field, value in mutations:
        altered = [dict(row) for row in rows]
        values = list(altered[index]["v"])
        values[field] = value
        altered[index]["v"] = tuple(values)
        try:
            Audit().run(altered)
        except ValueError:
            continue
        raise ValueError("altered timing lifecycle accepted")
    return {"synthetic_lifecycle": "PASS", "declined_mutations": len(mutations),
            "scope": "checker falsification only; no reference or native admission"}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, required=True)
    parser.add_argument("--library", type=Path, required=True)
    parser.add_argument("--logs", type=Path, nargs="+")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--instructions", type=Path)
    parser.add_argument("--mmio", type=Path)
    parser.add_argument("--capture", type=Path)
    args = parser.parse_args()
    spec = importlib.util.spec_from_file_location(
        "clock_pins", Path(__file__).with_name("agent_clock_source.py"))
    clock = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(clock)
    pins = dict(clock.SOURCE_PINS, **EXTRA_PINS)
    for relative, expected in pins.items():
        check(hash_file(args.source_root / relative) == expected, "source pin changed: " + relative)
    check(hash_file(args.library) == clock.LIB_SHA, "linked producer library changed")
    report = {"source_pin_count": len(pins), "core_library_sha256": clock.LIB_SHA,
              "scope": "pinned observer and timing source; no native clock admission",
              "checker_falsification": self_test()}
    capture = json.loads(args.capture.read_text(encoding="utf-8")) if args.capture else None
    if args.logs:
        rows, inputs, headers = read_logs(args.logs)
        report.update(Audit().run(rows))
        report["logs"], report["filters"] = inputs, headers
        report["source_locations"] = source_locations(rows, args.source_root, pins, capture)
        report["callback_projection"] = callback_projection(rows)
        if capture:
            check(capture.get("controlled_midchain_writes") == [], "midchain state writer present")
            check(capture.get("sole_internal_stop") == "80373ac4", "capture debugger schedule differs")
            check(len(inputs) == 1 and inputs[0]["name"] == capture["event_log"] and
                  inputs[0]["sha256"] == capture["event_log_sha256"], "capture event binding differs")
            report["capture"] = {"name": args.capture.name, "sha256": hash_file(args.capture),
                                 "dolphin_sha256": capture["dolphin_sha256"],
                                 "controlled_initial_l2cr": capture["controlled_initial_l2cr"],
                                 "pause_entry_seconds": capture["pause_entry_seconds"]}

        closures = []
        for header in headers:
            fields = dict(p.split("=", 1) for p in header.split(";"))
            stop = fields.get("window", "").split("..")[-1]
            closed = True
            if stop:
                kind, value = stop.split("@", 1)
                closed = any(r["kind"] == kind and r["v"][0] == int(value, 16) for r in rows)
            closures.append({"requested_stop": stop, "observed_stop": closed})
        report["window_closure"] = closures
        if not all(c["observed_stop"] for c in closures):
            report["scope"] += "; requested later stop absent, first-clock prefix only"
    if args.instructions:
        check(args.logs is not None, "instruction audit requires independent event replay")
        report["instruction_cost_audit"] = instruction_audit(
            args.instructions, args.source_root, report["elapsed_entry_to_clock"])
    if args.mmio:
        report["mmio_access_audit"] = mmio_audit(args.mmio)
        if capture:
            check(args.mmio.name == capture["mmio_log"] and
                  hash_file(args.mmio) == capture["mmio_log_sha256"], "capture MMIO binding differs")
    if args.mmio and capture:
        environment = capture.get("observer_environment")
        if environment is None:
            report["mmio_access_audit"]["environment_bound"] = False
        else:
            expected = {
                "event_window": "init..timing-prefix-end@80373ac4",
                "event_kinds": "", "event_limit": "3000000",
                "mmio_watch": ";".join(report["mmio_access_audit"]["required_watch_ranges"]),
                "mmio_watch_phase0": True,
            }
            check(environment == expected, "captured observer environment differs")
            check(capture.get("capture_complete") is True, "capture is not complete")
            capture_tool = Path(__file__).with_name("capture_timing_prefix.py")
            check(hash_file(capture_tool) == capture.get("capture_tool_sha256"),
                  "captured tool source pin differs")
            report["mmio_access_audit"]["environment_bound"] = True
            report["mmio_access_audit"]["capture_tool_sha256"] = hash_file(capture_tool)
            check(report["mmio_access_audit"]["watched_cpu_mmio_writes"] == 0 and
                  report["mmio_access_audit"]["watched_cpu_mmio_reads"] == 0,
                  "new watched access requires device reconstruction")
            report["mmio_access_audit"]["excluded_cpu_control_writers"] = [
                "DSP mailbox/control/audio DMA", "VI timing/interrupt registers",
                "PI interrupt cause/mask", "AI control/sample-rate/playback"]
            report["mmio_access_audit"]["scope"] = (
                "bound phase0 exclusion of CPU MMIO accesses in four exact watch ranges; "
                "SI/DVD, non-CPU sources and native ownership remain separate")
    if args.instructions and capture:
        check(args.instructions.name == capture["instruction_trace"] and
              hash_file(args.instructions) == capture["instruction_trace_sha256"],
              "capture instruction binding differs")
    if args.output:
        build_root = Path(__file__).resolve().parents[3] / "build"
        check(args.output.resolve().is_relative_to(build_root.resolve()),
              "audit output must stay under repository build/")
        args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(report, indent=2))

if __name__ == "__main__":
    main()

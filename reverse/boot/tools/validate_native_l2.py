"""Full-entry differential validation for the bounded native L2 owner.

Reference supplies entry state only. No intermediate register, poll result,
completion flag or expected stack bytes are passed to native C++.
"""
import argparse
import json
import re
from pathlib import Path
import subprocess
import sys

from capture_boot_machine_state import DISC_SHA
from capture_dolphin_rsp import sha256
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "recognizer"))
from machine import DolImage


PREFIX_PCS = {0x80371730, 0x80371734, 0x80371758, 0x80003414,
              0x80370CDC, 0x80370CF0, 0x80370CFC, 0x80370D00,
              0x80370D7C, 0x80370D80, 0x80370D84, 0x80370DFC,
              0x80003418, 0x80372838, 0x80372860, 0x80372880, 0x80372894}
L2_COMMON_PCS = {0x80372898, 0x803728A0, 0x803728F8, 0x80372904}
NATIVE_PREFIX_ORDER = [0x80003158, 0x80003400, 0x80371714, 0x80370BA8,
                       0x80371724, 0x8037172C, 0x80371730, 0x80371734,
                       0x80371758, 0x80003414, 0x80370CDC, 0x80370CF0,
                       0x80370CFC, 0x80370D00, 0x80370D7C, 0x80370D80,
                       0x80370D84, 0x80370DFC, 0x80003418, 0x80372838,
                       0x80372860, 0x80372880, 0x80372894]
L2_DISABLED_ORDER = [0x80372640, 0x8037266C, 0x80372678, 0x80372684,
                     0x80372688, 0x80372690, 0x803726B4, 0x803726C0,
                     0x803726C4, 0x803728D4, 0x803728DC, 0x803728EC,
                     0x80370C8C, 0x80370CB4, 0x80370CD4]
L2_DISABLED_PCS = set(L2_DISABLED_ORDER)


def checkpoint_order(disabled):
    return NATIVE_PREFIX_ORDER + [0x80372898, 0x803728A0] + (
        L2_DISABLED_ORDER if disabled else []) + [0x803728F8, 0x80372904]


def checked_validity(pc, paired_mask, l2_mask, disabled):
    """Word-store timeline from raw prologues; unknown bytes cannot hide checks."""
    def mask(base, size, words):
        known = {address+b for address in words for b in range(4)}
        return "".join("1" if base+n in known else "0" for n in range(size))
    paired_words = set()
    if pc not in NATIVE_PREFIX_ORDER or NATIVE_PREFIX_ORDER.index(pc) >= 3:
        paired_words |= {0x8060C5E8, 0x8060C5F4}
    if pc not in NATIVE_PREFIX_ORDER or NATIVE_PREFIX_ORDER.index(pc) >= 20:
        paired_words.add(0x8060C5EC)
    if paired_mask != mask(0x8060C5E8, 16, paired_words):
        raise ValueError(f"paired validity/store timeline differs at {pc:08X}")
    if pc in NATIVE_PREFIX_ORDER:
        if l2_mask != "-":
            raise ValueError("unexpected pre-L2 snapshot window")
        return
    order = checkpoint_order(disabled)
    index = order.index(pc)
    words = {0x8060C5E0, 0x8060C5E8, 0x8060C5EC, 0x8060C5F4}
    if disabled:
        if index >= order.index(0x8037266C): words |= {0x8060C5E4, 0x8060C5D0, 0x8060C5DC}
        if index >= order.index(0x80370CB4): words.add(0x8060C570)
        if index >= order.index(0x80370CD4): words |= {0x8060C578+4*n for n in range(8)}
    if l2_mask != mask(0x8060C570, 0x90, words):
        raise ValueError(f"L2 validity/store timeline differs at {pc:08X}")


def checked_effects(effects, states, disabled):
    """Every live read, write, bounded barrier and completion in original order."""
    entry = int(states[0x80003154]["l2cr"], 16)
    if not disabled:
        if effects != [[0, 0x80372894, entry]]:
            raise ValueError("enabled branch touched L2 state")
        return
    read_states = {0x80372894:0x80372898, 0x803728C0:0x80372640,
                   0x80372654:0x8037266C, 0x80372664:0x8037266C,
                   0x80372678:0x80372684, 0x80372688:0x80372690,
                   0x803726B4:0x803726C0, 0x803728DC:0x803728DC}
    write_states = {0x803728C8:0x80372640, 0x8037265C:0x8037266C,
                    0x8037266C:0x80372678, 0x80372690:0x803726B4,
                    0x803728E8:0x803728EC}
    # Completion kind2 is the private visibility generation (not cycles).
    # Kind3 records actual store completion at each original sync and at the
    # synchronous native invalidate operation; other PCs/kinds are not ignored.
    order = [(0,0x80372894), (3,0x803728AC), (3,0x803728B8), (3,0x803728BC),
             (0,0x803728C0), (1,0x803728C8), (3,0x803728CC), (3,0x80372650),
             (0,0x80372654), (1,0x8037265C), (3,0x80372660), (0,0x80372664),
             (1,0x8037266C), (3,0x8037266C), (2,0x8037266C), (0,0x80372678),
             (0,0x80372688), (1,0x80372690), (0,0x803726B4), (0,0x803728DC), (1,0x803728E8)]
    expected = []
    for kind, pc in order:
        if kind in (0, 1):
            observed_pc = (read_states if kind == 0 else write_states)[pc]
            value = int(states[observed_pc]["l2cr"], 16)
        elif kind == 2: value = 1
        elif pc == 0x8037266C: value = int(states[0x80372678]["l2cr"], 16)
        else: value = entry
        expected.append([kind, pc, value])
    if effects != expected:
        raise ValueError("ordered L2 read/write/completion effects differ")


def checked_states(report, image):
    """Reject stale captures, unlabelled entry changes and mid-chain forcing."""
    if report["disc_sha256"] != DISC_SHA or report["dolphin_sha256"] != report["instrumentation_manifest"]["instrumented_executable_sha256"]:
        raise ValueError("reference identity mismatch")
    if report["controlled_midchain_l2cr"] or report["controlled_live_bss_source_experiment"] or any(
            "controlled_midchain_l2cr_write" in s for s in report["checkpoints"]):
        raise ValueError("mid-chain perturbation is not chain parity")
    states = {int(s["pc"], 16): s for s in report["checkpoints"]}
    if len(states) != len(report["checkpoints"]):
        raise ValueError("repeated checkpoints require a trace-aware comparison")
    for pc, state in states.items():
        for key in ("pc", "msr", "hid0", "hid2", "l2cr", "cr", "architectural_xer", "ctr", "fpscr"):
            if not re.fullmatch(r"[0-9a-fA-F]{8}", state[key]):
                raise ValueError(f"incomplete raw state field {key}")
        for key, count, width in (("ps0",32,16), ("ps1",32,16), ("gqr",8,8)):
            if len(state[key]) != count or any(not re.fullmatch(rf"[0-9a-fA-F]{{{width}}}", v) for v in state[key]):
                raise ValueError(f"incomplete raw state array {key}")
        if not re.fullmatch(r"[0-9a-fA-F]{256}", state["gpr"]) or not re.fullmatch(r"[0-9a-fA-F]{32}", state["fpr_source"]):
            raise ValueError("incomplete raw GPR/source bytes")
        if int(state["instruction_word"], 16) != image.word(pc, text=True):
            raise ValueError(f"captured raw instruction differs at {pc:08X}")
    entry = states[0x80003154]
    original = report["original_unmodified_entry"]
    if int(original["pc"], 16) != 0x80003154:
        raise ValueError("original entry snapshot missing")
    if int(entry["l2cr"], 16) != int(report["controlled_initial_l2cr"], 16) or int(entry["hid0"], 16) != int(report["controlled_initial_hid0"], 16):
        raise ValueError("entry control provenance differs")
    for key in ("msr", "gpr", "hid2", "cr", "architectural_xer", "ctr", "fpscr", "ps0", "ps1", "gqr"):
        if entry[key] != original[key]:
            raise ValueError(f"unlabelled pre-entry {key} change")
    source = report["controlled_entry_source_bits"] or original["fpr_source"]
    if entry["fpr_source"].lower() != source.lower() or any(
            s["fpr_source"] != entry["fpr_source"] for s in states.values()):
        raise ValueError("source provenance changes after entry")
    return states


def validate(dol, program, capture, work):
    if sha256(dol) != "fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af":
        raise ValueError("PAL digest mismatch")
    root = Path(__file__).resolve().parents[3] / "build"
    if not work.resolve().is_relative_to(root.resolve()):
        raise ValueError("outputs must stay in repository build/")
    report = json.loads(capture.read_text(encoding="utf-8"))
    states = checked_states(report, DolImage(dol))
    entry = states[0x80003154]
    disabled = not int(entry["l2cr"], 16) & 0x80000000
    expected_order = checkpoint_order(disabled)
    expected_pcs = PREFIX_PCS | L2_COMMON_PCS | (L2_DISABLED_PCS if disabled else set())
    reference_order = [int(s["pc"], 16) for s in report["checkpoints"] if int(s["pc"], 16) in expected_pcs]
    if reference_order != [pc for pc in expected_order if pc in expected_pcs]:
        raise ValueError("reference checkpoint order/coverage differs from original CFG")
    fields = [entry[k] for k in ("msr", "hid0", "hid2", "cr", "architectural_xer", "ctr", "fpscr")]
    fields += entry["ps0"] + entry["ps1"] + entry["gqr"] + ["805f1f30"]
    fields += [entry["fpr_source"][n:n+8] for n in range(0, 32, 8)] + [entry["l2cr"]]
    work.mkdir(parents=True, exist_ok=True)
    fixture = work / (capture.stem + ".entry.txt")
    fixture.write_text("\n".join(fields)+"\n", encoding="ascii")
    run = subprocess.run([str(program.resolve()), str(dol.resolve()), str(fixture.resolve())],
                         capture_output=True, text=True, check=True)
    (work / (capture.stem + ".native.txt")).write_text(run.stdout, encoding="ascii")
    checkpoints = compared = memory_bytes = 0
    committed = []; prefix_stores = []; effects = []; stop_seen = False
    seen = set()
    native_order = []

    def memory(actual, mask, observed, pc, base):
        nonlocal memory_bytes
        raw = bytes.fromhex(actual); ref = bytes.fromhex(observed)
        if len(raw) != len(mask) or len(ref) != len(raw) or any(c not in "01" for c in mask):
            raise ValueError("memory shape/validity differs")
        for n, known in enumerate(mask):
            if known == "1":
                if raw[n] != ref[n]:
                    raise ValueError(f"first memory divergence PC {pc:08X} address {base+n:08X}")
                memory_bytes += 1

    for line in run.stdout.splitlines():
        if line.startswith("COMMITTED_STACK "):
            prefix_stores.append([int(v, 16) for v in line.split()[1:]]); continue
        if line.startswith("COMMITTED_L2 "):
            committed.append([int(v, 16) for v in line.split()[1:]]); continue
        if line.startswith("L2_EFFECT "):
            effects.append([int(v, 16) for v in line.split()[1:]]); continue
        if line.startswith("STOP "):
            if line != "STOP pc=0x80372904 reason=LIVE_HANDLER_SLOT_UNRESOLVED" or stop_seen:
                raise ValueError("unexpected native stop")
            stop_seen = True; continue
        words = line.split()
        if len(words) != 118:
            raise ValueError("native checkpoint field count differs")
        pc = int(words[0], 16)
        native_order.append(pc)
        if pc not in expected_order:
            raise ValueError("unexpected native checkpoint")
        checked_validity(pc, words[114], words[117], disabled)
        if pc in NATIVE_PREFIX_ORDER and words[116] != "-":
            raise ValueError("unexpected pre-L2 memory window")
        if pc not in states:
            continue  # earlier register-prefix states have separate gates
        if pc in seen:
            raise ValueError("native duplicated checkpoint")
        seen.add(pc)
        ref = states[pc]
        expected = [ref[k] for k in ("pc", "msr", "lr", "cr", "architectural_xer", "fpscr")]
        expected += [ref["gpr"][n:n+8] for n in range(0, 256, 8)]
        expected += ref["ps0"] + ref["ps1"] + [ref[k] for k in ("ctr", "hid0", "hid2")] + ref["gqr"]
        actual = words[:113] + [words[115]]
        expected += [ref["l2cr"]]
        for n, (value, observed) in enumerate(zip(actual, expected)):
            if int(value, 16) != int(observed, 16):
                raise ValueError(f"first divergence PC {pc:08X} field {n}: native {value} ref {observed}")
            compared += 1
        memory(words[113], words[114], ref["paired_stack_bytes"], pc, 0x8060C5E8)
        if words[116] != "-":
            if int(ref["l2_stack_address"], 16) != 0x8060C570:
                raise ValueError("stack observation address differs")
            memory(words[116], words[117], ref["l2_stack_bytes"], pc, 0x8060C570)
        checkpoints += 1
    if not stop_seen or seen != expected_pcs or native_order != expected_order:
        raise ValueError(f"incomplete native chain: missing {sorted(expected_pcs-seen)}; extra {sorted(seen-expected_pcs)}")
    final = states[0x80372904]

    def read_word(address):
        base = int(final["l2_stack_address"], 16)
        raw = bytes.fromhex(final["l2_stack_bytes"])
        offset = address-base
        if offset < 0 or offset+4 > len(raw):
            raise ValueError("write not reference-observed")
        return int.from_bytes(raw[offset:offset+4], "big")

    sp = int(states[0x80372838]["gpr"][8:16], 16)
    expected_prefix = [[a, read_word(a)] for a in (sp+4, sp-16, sp-4, sp-8)]
    if prefix_stores != expected_prefix:
        raise ValueError("prefix committed stores differ")
    if disabled:
        helper_sp = int(states[0x80372640]["gpr"][8:16], 16)
        logger_sp = int(states[0x80370C8C]["gpr"][8:16], 16)-0x70
        addresses = [helper_sp+4, helper_sp-16, helper_sp-4, logger_sp]
        addresses += [logger_sp+8+4*n for n in range(8)]
        expected_stores = [[a, read_word(a)] for a in addresses]
    else:
        expected_stores = []
    checked_effects(effects, states, disabled)
    if committed != expected_stores:
        raise ValueError("ordered committed L2 stack stores differ")
    summary = {"capture_sha256": sha256(capture), "native_executable_sha256": sha256(program),
               "entry_fixture_sha256": sha256(fixture), "checkpoints": checkpoints,
               "state_fields": compared, "known_memory_bytes": memory_bytes,
               "ordered_prefix_frame_words": len(prefix_stores), "ordered_l2_frame_words": len(committed),
               "read_write_effects": sum(e[0] in (0,1) for e in effects),
               "connected_stop": "0x80372904", "scope": "bounded native owner; controlled HLE deltas; no retail/timing claim"}
    (work / (capture.stem+".validation.json")).write_text(json.dumps(summary, indent=2)+"\n", encoding="utf-8")
    print(f"{capture.name}: {checkpoints} checkpoints; {compared} fields; {memory_bytes} known bytes; "
          f"{len(committed)} new committed words; stop 80372904")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ("dol", "program", "capture", "work"):
        parser.add_argument(key, type=Path)
    args = parser.parse_args()
    validate(args.dol, args.program, args.capture, args.work)

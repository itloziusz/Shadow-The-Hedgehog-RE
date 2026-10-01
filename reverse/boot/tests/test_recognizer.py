"""Adversarial gates for the binary-first boot recognizer."""

import json
import sys
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "recognizer"))

from fingerprints import detectors, fingerprint, similarity, stable_timebase_sampler
from cli import family_rankings, frontier, learned_hits, rescan, run_native_probe
from machine import DolImage, analyze, decode, initial_state, transfer
from store import Store


class FakeImage:
    def __init__(self, start, words):
        self.start, self.words = start, list(words)
        self.sha256 = "synthetic-raw-words"

    def section(self, address):
        return SimpleNamespace(address=self.start, offset=0,
                               end=self.start + 4 * len(self.words), text=True)

    def word(self, address, *, text=True):
        if address & 3 or not self.is_text(address):
            raise ValueError("unmapped word")
        return self.words[(address - self.start) // 4]

    def is_text(self, address):
        return self.start <= address < self.start + 4 * len(self.words) and not address & 3


class RecognizerTests(unittest.TestCase):
    def test_validated_region_cannot_inherit_proof_after_binary_or_range_change(self):
        image = FakeImage(0x80001000, [0x38600001, 0x4E800020])
        original = analyze(image, image.start, image.start + 8)
        for change in ("address", "range", "binary", "raw_body"):
            with self.subTest(change=change):
                db = Store(":memory:")
                fp = fingerprint(original)
                db.upsert("reviewed", image.start, image.start + 8, "seed", "old_family",
                          "VALIDATED", ["reviewed original evidence"], original, fp, [])
                altered = dict(original); altered_fp = dict(fp)
                start, end = image.start, image.start + 8
                if change == "address": start, end = start + 16, end + 16
                elif change == "range": end += 4
                elif change == "binary": altered["sha256"] = "another binary"
                else: altered_fp["raw_sha256"] = "another raw body"
                with self.assertRaisesRegex(ValueError, "validated region binary/range changed"):
                    db.upsert("reviewed", start, end, "candidate", "new_family", "UNKNOWN",
                              ["unreviewed"], altered, altered_fp, [])
                self.assertEqual(json.loads(db.get("reviewed")["evidence_json"]),
                                 ["reviewed original evidence"])

    def test_rescan_preserves_reviewed_analysis_and_semantic_name(self):
        image = FakeImage(0x80001000, [0x38600001, 0x4E800020])
        original = analyze(image, image.start, image.start + 8)
        db = Store(":memory:"); fp = fingerprint(original)
        db.upsert("reviewed", image.start, image.start + 8, "seed", "reviewed_family",
                  "VALIDATED", ["reviewed evidence"], original, fp, [])
        proposed = dict(original); proposed["effects"] = [{"kind": "invented"}]
        db.upsert("reviewed", image.start, image.start + 8, "candidate", "guessed_family",
                  "UNKNOWN", ["new hypothesis"], proposed, fp, [])
        row = db.get("reviewed")
        self.assertEqual(row["family"], "reviewed_family")
        self.assertEqual(row["origin"], "seed")
        self.assertEqual(json.loads(row["analysis_json"]), original)
        self.assertEqual(json.loads(row["evidence_json"]), ["reviewed evidence"])

    def test_ctr_update_and_full_bo_predicate_are_preserved(self):
        # Direct conditional branches admit all BO combinations. Test each
        # predicate against independent BO0/BO2 and wrapping arithmetic rules.
        for bo in range(32):
            for ctr in (0, 1, 2, 0xFFFFFFFF):
                with self.subTest(bo=bo, ctr=ctr):
                    word = (16 << 26) | (bo << 21) | (6 << 16) | 8
                    state = initial_state(); state["CTR"] = f"K:{ctr:08X}"
                    effects = []; transfer(decode(word, 0x80001000), state, effects)
                    expected = ctr if bo & 4 else (ctr - 1) & 0xFFFFFFFF
                    self.assertEqual(state["CTR"], f"K:{expected:08X}")
                    condition = effects[0]["condition"]
                    self.assertEqual("CTR" in condition, not bool(bo & 4))
                    self.assertEqual("CRbit6" in condition, not bool(bo & 16))
                    if not bo & 4:
                        self.assertIn("CTR==0" if bo & 2 else "CTR!=0", condition)
                    if not bo & 16:
                        self.assertIn(f"CRbit6=={int(bool(bo & 8))}", condition)

    def test_mfctr_after_count_branch_reads_decremented_value(self):
        image = FakeImage(0x80001000, [0x38600002, 0x7C6903A6,
                                       0x42000004, 0x7C8902A6, 0x4E800020])
        region = analyze(image, image.start, image.start + 20)
        self.assertEqual(region["exit_states"]["0x80001014"]["r4"], "K:00000001")

    def test_conditional_lr_return_keeps_taken_exit_and_fallthrough(self):
        image = FakeImage(0x80001000, [0x4D820020, 0x38600007, 0x4E800020])
        region = analyze(image, image.start, image.start + 12)
        self.assertEqual(region["cfg"][0]["successors"], ["0x80001004"])
        self.assertIn("0x80001004", region["exit_states"])
        self.assertEqual(region["exit_states"]["0x8000100C"]["r3"], "K:00000007")
        first_return = next(e for e in region["effects"] if e["kind"] == "return")
        self.assertIn("CRbit2==1", first_return["condition"])

    def test_unconditional_ctr_branch_has_no_fallthrough(self):
        image = FakeImage(0x80001000, [0x4E800420, 0x906D0000, 0x4E800020])
        region = analyze(image, image.start, image.start + 12)
        self.assertEqual(region["cfg"][0]["successors"], [])
        self.assertFalse(any(e["kind"] == "store" for e in region["effects"]))
        self.assertTrue(region["unresolved_edges"])

    def test_rfi_and_invalid_bcctr_cannot_fabricate_control_state(self):
        for word in (0x4C000064, 0x4E000420):
            with self.subTest(word=f"{word:08X}"):
                state = initial_state(); state["LR"] = "K:80001000"; effects = []
                transfer(decode(word, 0x80001000), state, effects)
                self.assertEqual(effects[0]["kind"], "unsupported")
                self.assertTrue(all(value == "UNKNOWN:unsupported" for value in state.values()))

    def test_symbolic_rlwimi_preserves_prior_destination_dependency(self):
        # The low 24 destination bits survive this high-byte insertion.
        word = (20 << 26) | (3 << 21) | (4 << 16) | (7 << 1)
        results = []
        for previous in ("IN:first_destination", "IN:second_destination"):
            state = initial_state(); state["r4"] = previous; effects = []
            transfer(decode(word, 0x80001000), state, effects)
            self.assertIn(previous, state["r4"])
            self.assertIn("IN:r3", state["r4"])
            results.append(state["r4"])
        self.assertNotEqual(*results)

    def test_compare_and_record_cr_keep_xer_so_provenance(self):
        for word in (0x2C030000, 0x28030000, 0x706300FF, 0x54640001):
            with self.subTest(word=f"{word:08X}"):
                state = initial_state(); state["XER"] = "IN:exception_summary"; effects = []
                transfer(decode(word, 0x80001000), state, effects)
                self.assertIn("SO=XER.SO(IN:exception_summary)", state["CR0"])
                for xer, so in ((0, 0), (0x80000000, 1), (0x40000000, 0), (0xFFFFFFFF, 1)):
                    state = initial_state(); state["XER"] = f"K:{xer:08X}"
                    transfer(decode(word, 0x80001000), state, [])
                    self.assertIn(f"SO={so})", state["CR0"])

    def test_native_probe_keeps_committed_l2_memory_readbacks(self):
        # Diagnostic-parser test, not original machine-state evidence.
        words = ["00000000"] * 118
        for n in range(38,102):words[n]="0000000000000000"
        words[113]="0"*32;words[114]="0"*16;words[116]="0"*288;words[117]="0"*144
        words[0] = "80372904"; words[115] = "80000000"
        output = " ".join(words) + "\nCOMMITTED_STACK 8060c5e0 8060c5f0\n"
        output += "COMMITTED_L2 8060c5e4 803728d4\nSTOP pc=0x80372904 reason=LIVE_HANDLER_SLOT_UNRESOLVED\n"
        done = SimpleNamespace(returncode=0, stdout=output, stderr="")
        with patch("cli.subprocess.run", return_value=done), patch("cli.Path.read_bytes", return_value=b"synthetic entry"):
            result = run_native_probe(Path("native"), Path("dol"),
                                      {"msr":None,"hid2":None,"hid0":None}, Path("entry"))
        self.assertEqual(result["known_fields"]["l2cr"], "80000000")
        self.assertEqual([w["addr"] for w in result["ordered_stack_writes"]],
                         ["0x8060C5E0", "0x8060C5E4"])
        self.assertEqual(result["ordered_stack_writes"][1]["value"], "0x803728D4")
        done.stdout = output.replace("8060c5e4 803728d4", "8060c5e4 UNKNOWN")
        with patch("cli.subprocess.run", return_value=done), self.assertRaises(ValueError):
            run_native_probe(Path("native"), Path("dol"),
                             {"msr":None,"hid2":None,"hid0":None}, Path("entry"))
        for corrupted in (output.replace("00000000", "UNKNOWN", 1),output+"WRITE nonsense\n"):
            done.stdout=corrupted
            with patch("cli.subprocess.run",return_value=done),self.assertRaises(ValueError):
                run_native_probe(Path("native"),Path("dol"),{"msr":None,"hid2":None,"hid0":None},Path("entry"))
        earlier=output.replace('80372904','800031f4').replace('STOP pc=', 'BOOT_BYTE_STORE 80003144 805f1ff0 01 01\nSTOP pc=')
        done.stdout=earlier
        with patch('cli.subprocess.run',return_value=done),patch('cli.Path.read_bytes',return_value=b'synthetic entry'):
            result=run_native_probe(Path('native'),Path('dol'),{'msr':None,'hid2':None,'hid0':None},Path('entry'))
        self.assertEqual(result['stop_pc'],'0x800031F4')
        self.assertEqual(result['ordered_stack_writes'][-1]['width'],1)
        for bad in ('01 UNKNOWN','01 00','0001 01'):
            done.stdout=earlier.replace('01 01',bad)
            with patch('cli.subprocess.run',return_value=done),self.assertRaises(ValueError):
                run_native_probe(Path('native'),Path('dol'),{'msr':None,'hid2':None,'hid0':None},Path('entry'))

    def test_pointer_chain_stays_symbolic_at_indirect_call(self):
        # lis/addi build a DOL address; the live word at that address may
        # have changed, so the indirect target cannot become a DOL constant.
        image = FakeImage(0x80001000, [0x3C608052, 0x3863EB48, 0x80830000,
                                       0x7C8903A6, 0x4E800421, 0x4E800020])
        region = analyze(image, image.start, image.start + 24)
        call = next(e for e in region["effects"] if e["kind"] == "call")
        self.assertTrue(call["indirect"])
        self.assertEqual(call["target"], "ALIGN4(MEM32:K:8051EB48)")
        self.assertEqual(region["instructions"][2]["raw"], "80830000")
        self.assertIn("indirect_dispatch", [d["id"] for d in detectors(region)])

    def test_bclr_uses_aligned_lr_target_but_preserves_lr_word(self):
        # lis/ori materialize an odd target, then mtlr and blr. The Gekko
        # branch PC ignores LR low bits; mflr would still read the odd word.
        image = FakeImage(0x80001000, [0x3C608000, 0x60631003,
                                       0x7C6803A6, 0x4E800020])
        region = analyze(image, image.start, image.start + 16)
        ret = next(e for e in region["effects"] if e["kind"] == "return")
        self.assertEqual(ret["target"], "K:80001000")
        self.assertEqual(region["exit_states"]["0x80001010"]["LR"],
                         "K:80001003")

    def test_cr1_branch_does_not_inherit_cr0(self):
        image = FakeImage(0x80001000, [0x38800000, 0x2C840000,
                                       0x40860008, 0x38600001, 0x4E800020])
        region = analyze(image, image.start, image.start + 20)
        branch = next(e for e in region["effects"] if e["kind"] == "branch")
        self.assertIn("cmpwi", branch["condition"])
        self.assertIn("K:00000000", branch["condition"])
        self.assertEqual(region["instructions"][2]["mnemonic"], "bne")

    def test_gqr_chain_requires_all_eight_ordered_zero_writes(self):
        words = ([0x7C0004AC, 0x38600000] +
                 [0x7C70E3A6 + (i << 16) for i in range(8)] + [0x4E800020])
        image = FakeImage(0x80371730, words)
        region = analyze(image, image.start, image.start + 4 * len(words))
        self.assertIn("sync_gqr_zero_chain", [d["id"] for d in detectors(region)])
        changed = words.copy()
        changed[5] = 0x7C70E3A6  # repeat GQR0 in the fourth write
        bad = analyze(FakeImage(image.start, changed), image.start,
                      image.start + 4 * len(changed))
        self.assertNotIn("sync_gqr_zero_chain", [d["id"] for d in detectors(bad)])
        interrupted = words[:4] + [0x60000000] + words[4:]
        altered = analyze(FakeImage(image.start, interrupted), image.start,
                          image.start + 4 * len(interrupted))
        self.assertNotIn("sync_gqr_zero_chain", [d["id"] for d in detectors(altered)])

    def test_unknown_word_and_external_exit_fail_closed(self):
        unknown = analyze(FakeImage(0x80001000, [0x00000000, 0x4E800020]),
                          0x80001000, 0x80001008)
        self.assertEqual(unknown["unknown_instructions"], ["0x80001000"])
        self.assertTrue(all(value == "UNKNOWN:raw_opcode" for value in
                            unknown["exit_states"]["0x80001008"].values()
                            if value != "IN:LR"))
        # A branch out before the first return forbids a complete initializer claim.
        escaped = analyze(FakeImage(0x80001000, [0x38600000, 0x906D0000,
                                                 0x906D0004, 0x48000020,
                                                 0x906D0008, 0x4E800020]),
                          0x80001000, 0x80001018)
        self.assertTrue(escaped["external_exits"])
        self.assertNotIn("store_initializer", [d["id"] for d in detectors(escaped)])
        # A decoded but unsupported arithmetic operation cannot leave earlier
        # constants or a three-store initializer claim intact.
        opaque = analyze(FakeImage(0x80001000, [0x38600000, 0x906D0000,
                                                0x1C630002, 0x906D0004,
                                                0x906D0008, 0x4E800020]),
                         0x80001000, 0x80001018)
        self.assertTrue(opaque["unsupported_semantics"])
        self.assertNotIn("store_initializer", [d["id"] for d in detectors(opaque)])

    def test_rotate_mask_wraparound_is_exact(self):
        # Independent bit-numbering oracle: PPC mask indices count from MSB.
        source = 0x81234567
        sh, mb, me = 7, 28, 3
        word = (21 << 26) | (3 << 21) | (4 << 16) | (sh << 11) | (mb << 6) | (me << 1)
        image = FakeImage(0x80001000, [0x3C608123, 0x60634567, word, 0x4E800020])
        region = analyze(image, image.start, image.start + 16)
        rotated = ((source << sh) | (source >> (32 - sh))) & 0xFFFFFFFF
        mask = sum(1 << (31 - bit) for bit in list(range(mb, 32)) + list(range(me + 1)))
        self.assertEqual(region["exit_states"]["0x80001010"]["r4"],
                         f"K:{rotated & mask:08X}")

    def test_untraced_call_kills_post_call_state(self):
        # A direct target does not imply a modeled return or ABI behavior.
        region = analyze(FakeImage(0x80001000, [0x38600007, 0x48000009,
                                                 0x906D0000, 0x4E800020]),
                         0x80001000, 0x80001010)
        call = next(e for e in region["effects"] if e["kind"] == "call")
        store = next(e for e in region["effects"] if e["kind"] == "store")
        self.assertEqual(call["callee_effects"], "UNTRACED")
        self.assertEqual(store["value"], "UNKNOWN:untraced_call")
        self.assertNotIn("store_initializer", [d["id"] for d in detectors(region)])

    def test_scalar_float_load_does_not_preserve_unknown_paired_lane(self):
        region = analyze(FakeImage(0x80001000, [0xC0228778, 0x4E800020]),
                         0x80001000, 0x80001008)
        state = region["exit_states"]["0x80001008"]
        self.assertIn("MEM32:", state["f1.ps0"])
        self.assertEqual(state["f1.ps1"], "UNKNOWN:scalar_fp_lane")

    def test_stmw_lmw_expand_all_registers_and_reject_base_overlap(self):
        image = FakeImage(0x80001000, [0x3B600007, 0xBF610008,
                                       0xBB610008, 0x4E800020])
        region = analyze(image, image.start, image.start + 16)
        stores = [e for e in region["effects"] if e["kind"] == "store"]
        loads = [e for e in region["effects"] if e["kind"] == "load"]
        self.assertEqual([e["register"] for e in stores],
                         [f"r{i}" for i in range(27, 32)])
        self.assertEqual([e["register"] for e in loads],
                         [f"r{i}" for i in range(27, 32)])
        self.assertEqual(stores[0]["value"], "K:00000007")
        self.assertEqual(stores[-1]["address"], "((IN:SP+0x8)+0x10)")
        self.assertEqual(region["unsupported_semantics"], [])
        bad = analyze(FakeImage(image.start, [0xBB7B0000, 0x4E800020]),
                      image.start, image.start + 8)
        self.assertTrue(bad["unsupported_semantics"])
        long = analyze(FakeImage(image.start, [0xBD010000, 0x4E800020]),
                       image.start, image.start + 8)
        ordered = [e["register"] for e in long["effects"] if e["kind"] == "store"]
        self.assertEqual(ordered, [f"r{i}" for i in range(8, 32)])

    def test_structural_score_is_not_confidence(self):
        a = fingerprint(analyze(FakeImage(0x80001000, [0x38600000, 0x906D0000,
                                                      0x906D0004, 0x906D0008,
                                                      0x4E800020]),
                                0x80001000, 0x80001014))
        b = fingerprint(analyze(FakeImage(0x80002000, [0x38800000, 0x908D0020,
                                                      0x908D0024, 0x908D0028,
                                                      0x4E800020]),
                                0x80002000, 0x80002014))
        result = similarity(a, b)
        self.assertGreater(result["score"], 0)
        self.assertLessEqual(result["score"], 1)
        self.assertIn("not confidence", result["meaning"])

    def test_promotion_and_frontier_require_evidence(self):
        db = Store(":memory:")
        image = FakeImage(0x80001000, [0x4E800020])
        analysis = analyze(image, image.start, image.start + 4)
        db.upsert("candidate", image.start, image.start + 4, "candidate", "unknown",
                  "UNKNOWN", [], analysis, fingerprint(analysis), [])
        with self.assertRaises(ValueError):
            db.promote("candidate", "foo", {"identification": "looks similar"})
        with self.assertRaises(ValueError):
            db.advance_frontier(0x80371734, {"raw_decode": "sync"})
        self.assertEqual(db.get("candidate")["status"], "UNKNOWN")
        self.assertEqual(db.frontier()["stop_pc"], 0x80371730)

    def test_learning_rebuild_removes_patterns_after_seed_demoted(self):
        db = Store(":memory:")
        image = FakeImage(0x80001000, [0x38600000, 0x906D0000,
                                       0x906D0004, 0x4E800020])
        region = analyze(image, image.start, image.start + 16)
        fp = fingerprint(region)
        for identity in ("a", "b"):
            db.upsert(identity, image.start, image.start + 16, "seed", "same_family",
                      "STRONGLY_SUPPORTED", ["fixture"], region, fp, [])
        self.assertTrue(db.learn())
        db.connection.execute("UPDATE regions SET status='UNKNOWN' WHERE id='b'")
        self.assertEqual(db.learn(), [])
        self.assertEqual(db.connection.execute("SELECT COUNT(*) FROM learned_patterns").fetchone()[0], 0)

    def test_frontier_progress_follows_control_flow_not_address_order(self):
        db = Store(":memory:")
        proof = {k: "synthetic validation-gate fixture" for k in
                 ("raw_decode", "data_flow", "reference_state", "native_replay", "regression")}
        proof.update(previous_stop="0x80371730", unresolved_side_effects=[])
        # The real FPR callee is below sync in address order, but later in
        # execution. Profile-specific advancement must retain the legacy stop.
        db.advance_frontier(0x80370CDC, proof, name="connected_immutable_native_boot")
        self.assertEqual(db.frontier("connected_immutable_native_boot")["stop_pc"], 0x80370CDC)
        self.assertEqual(db.frontier()["stop_pc"], 0x80371730)
        with self.assertRaises(ValueError):
            db.advance_frontier(0x80372894, proof, name="connected_immutable_native_boot")
        proof["previous_stop"] = "0x80370CDC"
        proof["unresolved_side_effects"] = ["missing ordering"]
        with self.assertRaises(ValueError):
            db.advance_frontier(0x80372894, proof, name="connected_immutable_native_boot")


class PalFixtureTests(unittest.TestCase):
    def test_timebase_sampler_register_and_compare_variants_falsified(self):
        if self.dol_path is None:self.skipTest("optional PAL DOL argument")
        image=DolImage(Path(self.dol_path))
        for pc in (0x80379628,0x804035D4):
            words=[image.word(pc+4*n) for n in range(6)]
            self.assertIsNotNone(stable_timebase_sampler(words,pc))
            self.assertIn('stable_timebase_sampler',[h['id'] for h in detectors(analyze(image,pc,pc+24))])
            for index,value in ((2,words[0]),(3,words[3]^0x00000800),(4,words[4]^0x00010000),(4,words[4]^4),(4,words[4]^2),(5,0x4E800021)):
                changed=words.copy();changed[index]=value
                self.assertIsNone(stable_timebase_sampler(changed,pc))

    def test_raw_ee_and_fill_motifs_falsify_register_aliases(self):
        if self.dol_path is None:self.skipTest("optional PAL DOL argument")
        image=DolImage(Path(self.dol_path))
        clear=analyze(image,0x8037611C,0x80376130)
        self.assertIn("interrupt_mask_exchange",[h["id"] for h in detectors(clear)])
        changed=json.loads(json.dumps(clear));changed["instructions"][2]["raw"]="7C600124"
        self.assertNotIn("interrupt_mask_exchange",[h["id"] for h in detectors(changed)])
        changed=json.loads(json.dumps(clear));changed['instructions'][0]['raw']='7C6100A6'
        self.assertNotIn('interrupt_mask_exchange',[h['id'] for h in detectors(changed)])
        fill=analyze(image,0x80005498,0x800054C0)
        self.assertIn("eight_word_fill_group",[h["id"] for h in detectors(fill)])
        changed=json.loads(json.dumps(fill));changed["instructions"][8]["raw"]="94E4001C"
        self.assertNotIn("eight_word_fill_group",[h["id"] for h in detectors(changed)])
    dol_path = None

    def test_actual_fill_tail_keeps_conditional_return_fallthrough(self):
        if self.dol_path is None:
            self.skipTest("optional PAL DOL argument")
        image = DolImage(Path(self.dol_path))
        self.assertEqual(image.word(0x800054E0, text=True), 0x4D820020)
        region = analyze(image, 0x800054DC, 0x800054F4)
        self.assertEqual(region["cfg"][0]["successors"], ["0x800054E4"])
        self.assertIn("0x800054E4", region["exit_states"])
        self.assertIn("0x800054F4", region["exit_states"])
        # Unmodeled carry/update-store semantics stay unsupported rather
        # than disappearing behind the earlier conditional return.
        self.assertIn("0x800054E4", region["unsupported_semantics"])
        self.assertIn("0x800054E8", region["unsupported_semantics"])

    def test_real_frontier_matches_original_bytes(self):
        if self.dol_path is None:
            self.skipTest("optional PAL DOL argument")
        image = DolImage(Path(self.dol_path))
        region = analyze(image, 0x80371730, 0x80371768)
        expected = (["7C0004AC", "38600000"] +
                    [f"{0x7C70E3A6 + (i << 16):08X}" for i in range(8)] +
                    ["8001000C", "38210008", "7C0803A6", "4E800020"])
        self.assertEqual([row["raw"] for row in region["instructions"]], expected)
        self.assertEqual(len([e for e in region["effects"] if e["kind"] == "spr_write"]), 8)
        self.assertIn("sync_gqr_zero_chain", [d["id"] for d in detectors(region)])
        self.assertEqual(region["effects"][0]["status"], "unresolved_hardware")

    def test_native_profile_frontier_retains_unknown_l2_input(self):
        if self.dol_path is None:
            self.skipTest("optional PAL DOL argument")
        image = DolImage(Path(self.dol_path))
        db = Store(":memory:")
        proof = {k: "synthetic test evidence" for k in
                 ("raw_decode", "data_flow", "reference_state", "native_replay", "regression")}
        proof.update(previous_stop="0x80371730", unresolved_side_effects=[])
        db.advance_frontier(0x80372894, proof, name="connected_immutable_native_boot")
        result = frontier(db, image, "connected_immutable_native_boot")
        self.assertEqual(result["connected_stop"], "0x80372894")
        self.assertIsNone(result["next_native_checkpoint"])
        self.assertEqual(result["minimal_experiments"][0]["break_before"], "0x80370AFC")
        self.assertEqual(result["minimal_experiments"][0]["break_after"], "0x80372898")
        row = db.get("frontier_l2cr_call")
        analysis = json.loads(row["analysis_json"])
        self.assertEqual([i["raw"] for i in analysis["instructions"]],
                         ["4BFFE269", "54600000", "28000000", "40820058"])
        self.assertEqual(row["status"], "UNKNOWN")
        self.assertEqual(db.frontier()["stop_pc"], 0x80371730)

    def test_native_l2_frontier_does_not_assume_handler_return(self):
        if self.dol_path is None:
            self.skipTest("optional PAL DOL argument")
        db = Store(":memory:")
        proof = {k: "synthetic test evidence" for k in
                 ("raw_decode", "data_flow", "reference_state", "native_replay", "regression")}
        proof.update(previous_stop="0x80371730", unresolved_side_effects=[])
        db.advance_frontier(0x80372894, proof, name="connected_immutable_native_boot")
        proof["previous_stop"] = "0x80372894"
        db.advance_frontier(0x80372904, proof, name="connected_immutable_native_boot")
        result = frontier(db, DolImage(Path(self.dol_path)), "connected_immutable_native_boot")
        self.assertEqual(result["raw_range"], ["0x80372904", "0x80372908"])
        self.assertIsNone(result["next_native_checkpoint"])
        experiment = result["minimal_experiments"][0]
        self.assertEqual(experiment["break_before"], "0x80373378")
        self.assertEqual(experiment["break_after"], "0x80372908")
        row = db.get("frontier_handler_call")
        analysis = json.loads(row["analysis_json"])
        self.assertEqual(analysis["instructions"][0]["raw"], "48000A75")
        self.assertEqual(analysis["effects"][0]["callee_effects"], "UNTRACED")
        self.assertEqual(row["status"], "UNKNOWN")
        self.assertEqual(db.frontier()["stop_pc"], 0x80371730)

    def test_repeated_constructor_keeps_concrete_write_addresses(self):
        if self.dol_path is None:
            self.skipTest("optional PAL DOL argument")
        image = DolImage(Path(self.dol_path))
        region = analyze(image, 0x8020E1E4, 0x8020E2CC)
        self.assertEqual(region["unknown_instructions"], [])
        self.assertEqual(region["unsupported_semantics"], [])
        stores = {e["address"] for e in region["effects"] if e["kind"] == "store"}
        self.assertIn("K:80545424", stores)
        self.assertIn("K:80545498", stores)
        self.assertIn("bulk_table_propagation", [d["id"] for d in detectors(region)])
        words = [image.word(pc, text=True) for pc in range(0x8020E1E4, 0x8020E2CC, 4)]
        words[3] = 0x1C630002  # replace saved-register sequence with unsupported arithmetic
        altered = analyze(FakeImage(0x8020E1E4, words), 0x8020E1E4, 0x8020E2CC)
        self.assertNotIn("bulk_table_propagation", [d["id"] for d in detectors(altered)])

    def test_eight_distinct_raw_bodies_share_one_normalized_structure(self):
        if self.dol_path is None:
            self.skipTest("optional PAL DOL argument")
        image = DolImage(Path(self.dol_path))
        starts = {135: 0x8020E1E4, 141: 0x802128C8, 143: 0x80213D60,
                  145: 0x80215058, 147: 0x802163F0, 149: 0x80217788,
                  151: 0x80218A80, 155: 0x8021AFD8}
        self.assertEqual(image.read(0x80514CB8, 12, text=False), b"\x00" * 12)
        raw, normalized, destinations = set(), set(), set()
        for index, start in starts.items():
            self.assertEqual(image.word(0x804AAC60 + 4 * index, text=False), start)
            region = analyze(image, start, start + 0xE8)
            fp = fingerprint(region)
            raw.add(fp["raw_sha256"])
            normalized.add(fp["normalized_sha256"])
            self.assertEqual(region["unsupported_semantics"], [])
            self.assertIn("bulk_table_propagation", [d["id"] for d in detectors(region)])
            loads = {e["address"] for e in region["effects"] if e["kind"] == "load"}
            self.assertTrue({"K:80514CB8", "K:80514CBC", "K:80514CC0"} <= loads)
            stores = {e["address"] for e in region["effects"]
                      if e["kind"] == "store" and e["address"].startswith("K:")}
            self.assertEqual(len(stores), 24)
            destinations.add(min(stores))
        self.assertEqual(len(raw), 8)
        self.assertEqual(len(normalized), 1)
        self.assertEqual(len(destinations), 8)

    def test_motion_table_copy_graph_and_rtti_ownership(self):
        if self.dol_path is None:
            self.skipTest("optional PAL DOL argument")
        image = DolImage(Path(self.dol_path))
        rows = [
            (135, 0x8020E1E4, 0x80545400, "Knuckles", 0x8020D7B8),
            (141, 0x802128C8, 0x805459A8, "Maria", 0x80211EC8),
            (143, 0x80213D60, 0x80545BC8, "Tails", 0x80213234),
            (145, 0x80215058, 0x80545DE8, "Omega", 0x802146C4),
            (147, 0x802163F0, 0x80546008, "Espio", 0x802159C0),
            (149, 0x80217788, 0x80546228, "Vector", 0x80216D58),
            (151, 0x80218A80, 0x80546448, "Rouge", 0x802180EC),
            (155, 0x8021AFD8, 0x80546840, "Amy", 0x8021A5A8),
        ]
        copies = {2: 0, 3: 1, 5: 4, 6: None,
                  8: None, 9: 7, 11: None, 12: 10}
        for index, start, base, character, class_start in rows:
            with self.subTest(index=index):
                self.assertEqual(image.word(0x804AAC60 + 4 * index, text=False), start)
                analysis = analyze(image, start, start + 0xE8)
                self.assertEqual(analysis["unsupported_semantics"], [])
                # The adjacent vtable points to an original RTTI class name;
                # five initialized target words point back into its code band.
                ti = image.word(base + 0xE4, text=False)
                name_ptr = image.word(ti, text=False)
                name = image.read(name_ptr, 100, text=False).split(b"\0", 1)[0].decode()
                self.assertEqual(name, f"Player::Npc::{character}::MotionImpl")
                for slot in (0, 1, 4, 7, 10):
                    address = base + 0x0C + slot * 12
                    self.assertEqual(image.word(address, text=False), 0)
                    self.assertEqual(image.word(address + 4, text=False), 0xFFFFFFFF)
                    target = image.word(address + 8, text=False)
                    self.assertTrue(class_start <= target < start)
                global_writes = {}
                seen_writes = set()
                for effect in analysis["effects"]:
                    address = effect.get("address", "")
                    if not address.startswith("K:"):
                        continue
                    if effect["kind"] == "load":
                        self.assertNotIn(address, seen_writes)
                    elif effect["kind"] == "store":
                        self.assertNotIn(address, global_writes)
                        global_writes[address] = effect["value"]
                        seen_writes.add(address)
                expected = {}
                for destination_slot, source_slot in copies.items():
                    for word in range(3):
                        dest = base + 0x0C + destination_slot * 12 + 4 * word
                        src = (0x80514CB8 + 4 * word if source_slot is None else
                               base + 0x0C + source_slot * 12 + 4 * word)
                        expected[f"K:{dest:08X}"] = f"MEM32:K:{src:08X}"
                self.assertEqual(global_writes, expected)

    def test_two_motion_seeds_recognize_holdout_without_self_match(self):
        if self.dol_path is None:
            self.skipTest("optional PAL DOL argument")
        image = DolImage(Path(self.dol_path))
        db = Store(":memory:")
        for identity, start, origin, status in (
            ("motion_seed_135", 0x8020E1E4, "seed", "STRONGLY_SUPPORTED"),
            ("motion_seed_141", 0x802128C8, "seed", "STRONGLY_SUPPORTED"),
            ("ctor_135", 0x8020E1E4, "candidate", "UNKNOWN"),
            ("ctor_143", 0x80213D60, "candidate", "UNKNOWN"),
        ):
            analysis = analyze(image, start, start + 0xE8)
            db.upsert(identity, start, start + 0xE8, origin,
                      "npc_motion_table_copy_graph" if origin == "seed" else "unknown",
                      status, ["raw fixture"], analysis, fingerprint(analysis), [])
        matches = rescan(db)
        self.assertNotIn("motion_seed_135",
                         [m["reference"] for m in matches["ctor_135"]])
        self.assertIn("motion_seed_141",
                      [m["reference"] for m in matches["ctor_135"]])
        self.assertEqual(matches["ctor_143"][0]["score"], 1.0)
        patterns = db.learn()
        self.assertTrue(patterns)
        hits = learned_hits(db, patterns)
        self.assertFalse(any(h["candidate"] == "ctor_135" for h in hits))
        self.assertTrue(any(h["candidate"] == "ctor_143" for h in hits))
        ranked = family_rankings(db, patterns)
        self.assertEqual([hit["candidate"] for hit in ranked], ["ctor_143"])
        self.assertEqual(ranked[0]["shared_ngrams_matched"],
                         ranked[0]["shared_ngrams_total"])
        self.assertTrue(ranked[0]["exact_normalized_sequence"])
        self.assertEqual(ranked[0]["status"], "STRUCTURAL_MATCH")
        self.assertEqual(db.get("ctor_143")["status"], "UNKNOWN")


if __name__ == "__main__":
    fixture = sys.argv[1:]  # unittest would otherwise interpret the path as a test name
    sys.argv = sys.argv[:1]
    if fixture:
        PalFixtureTests.dol_path = fixture[0]
    unittest.main()

"""Adversarial gates for the binary-first boot recognizer."""

import json
import sys
import unittest
from pathlib import Path
from types import SimpleNamespace

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "recognizer"))

from fingerprints import detectors, fingerprint, similarity
from machine import DolImage, analyze, decode
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
    def test_pointer_chain_stays_symbolic_at_indirect_call(self):
        # lis/addi build a DOL address; the live word at that address may
        # have changed, so the indirect target cannot become a DOL constant.
        image = FakeImage(0x80001000, [0x3C608052, 0x3863EB48, 0x80830000,
                                       0x7C8903A6, 0x4E800421, 0x4E800020])
        region = analyze(image, image.start, image.start + 24)
        call = next(e for e in region["effects"] if e["kind"] == "call")
        self.assertTrue(call["indirect"])
        self.assertEqual(call["target"], "MEM32:K:8051EB48")
        self.assertEqual(region["instructions"][2]["raw"], "80830000")
        self.assertIn("indirect_dispatch", [d["id"] for d in detectors(region)])

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
        # A decoded but unsupported store cannot leave earlier constants or
        # a three-store initializer claim intact.
        opaque = analyze(FakeImage(0x80001000, [0x38600000, 0x906D0000,
                                                0xBC030000, 0x906D0004,
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


class PalFixtureTests(unittest.TestCase):
    dol_path = None

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


if __name__ == "__main__":
    fixture = sys.argv[1:]  # unittest would otherwise interpret the path as a test name
    sys.argv = sys.argv[:1]
    if fixture:
        PalFixtureTests.dol_path = fixture[0]
    unittest.main()

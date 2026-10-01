"""Fail-closed L2 reference provenance checks; no emulator or DOL required.

The snapshots below are deliberately synthetic validation inputs, not original
machine-state evidence. They exercise rejection before native execution.
"""

import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import validate_native_l2 as validation


class Image:
    def word(self, pc, text=True):
        if pc not in (0x80003154, 0x80372894, 0x80372898):
            raise ValueError("unmapped instruction address")
        return {0x80003154: 0x4800015D, 0x80372894: 0x4BFFE269,
                0x80372898: 0x54600000}[pc]


def report():
    image = Image()
    entry = {
        "pc": "80003154", "instruction_word": f"{image.word(0x80003154):08x}",
        "msr": "00002032", "gpr": "00000000" * 32,
        "hid0": "0011c064", "hid2": "e0000000", "l2cr": "40480000",
        "cr": "00000000", "architectural_xer": "00000000", "ctr": "00000000",
        "fpscr": "00000000", "ps0": ["0000000000000000"] * 32,
        "ps1": ["0000000000000000"] * 32, "gqr": ["00000000"] * 8,
        "fpr_source": "00000000000000000000000000000000",
    }
    original = copy.deepcopy(entry)
    original["hid0"] = "0011c464"
    original["l2cr"] = "00000000"
    states = [entry]
    for pc in (0x80372894, 0x80372898):
        state = copy.deepcopy(entry)
        state["pc"] = f"{pc:08x}"
        state["instruction_word"] = f"{image.word(pc):08x}"
        states.append(state)
    return {
        "disc_sha256": validation.DISC_SHA,
        "dolphin_sha256": "synthetic-test-oracle",
        "instrumentation_manifest": {"instrumented_executable_sha256": "synthetic-test-oracle"},
        "original_unmodified_entry": original,
        "controlled_initial_hid0": "0011c064", "controlled_initial_l2cr": "40480000",
        "controlled_entry_source_bits": None,
        "controlled_midchain_l2cr": False,
        "controlled_live_bss_source_experiment": False,
        "checkpoints": states,
    }


class ProvenanceTests(unittest.TestCase):
    def test_labelled_entry_inputs_are_accepted(self):
        data = report()
        states = validation.checked_states(data, Image())
        self.assertEqual(set(states), {0x80003154, 0x80372894, 0x80372898})
        self.assertEqual(data["original_unmodified_entry"]["l2cr"], "00000000")
        self.assertEqual(states[0x80003154]["l2cr"], "40480000")

    def test_labelled_source_is_required_to_read_back(self):
        data = report()
        source = "400a0000000000003fc00000c0100000"
        data["controlled_entry_source_bits"] = source
        with self.assertRaisesRegex(ValueError, "source provenance"):
            validation.checked_states(data, Image())
        for state in data["checkpoints"]:
            state["fpr_source"] = source
        validation.checked_states(data, Image())
        self.assertNotEqual(data["original_unmodified_entry"]["fpr_source"], source)

    def test_reference_identity_must_match(self):
        for key in ("disc_sha256", "dolphin_sha256"):
            with self.subTest(key=key):
                data = report()
                data[key] = "different-reference"
                with self.assertRaisesRegex(ValueError, "reference identity"):
                    validation.checked_states(data, Image())

    def test_original_entry_cannot_be_replaced_with_later_snapshot(self):
        data = report()
        data["original_unmodified_entry"]["pc"] = "80372894"
        with self.assertRaisesRegex(ValueError, "original entry"):
            validation.checked_states(data, Image())

    def test_control_label_must_match_entry_readback(self):
        for key in ("hid0", "l2cr"):
            with self.subTest(key=key):
                data = report()
                data["checkpoints"][0][key] = "00000000"
                with self.assertRaisesRegex(ValueError, "control provenance"):
                    validation.checked_states(data, Image())

    def test_unlabelled_pre_entry_scalar_changes_decline(self):
        for key in ("msr", "gpr", "hid2", "cr", "architectural_xer", "ctr", "fpscr"):
            with self.subTest(key=key):
                data = report()
                value = data["checkpoints"][0][key]
                data["checkpoints"][0][key] = value[:-1] + ("1" if value[-1] != "1" else "2")
                with self.assertRaisesRegex(ValueError, "unlabelled pre-entry"):
                    validation.checked_states(data, Image())

    def test_unlabelled_pre_entry_array_changes_decline(self):
        for key in ("ps0", "ps1", "gqr"):
            with self.subTest(key=key):
                data = report()
                data["checkpoints"][0][key][-1] = "1" * len(data["checkpoints"][0][key][-1])
                with self.assertRaisesRegex(ValueError, "unlabelled pre-entry"):
                    validation.checked_states(data, Image())

    def test_unlabelled_source_at_entry_declines(self):
        data = report()
        data["checkpoints"][0]["fpr_source"] = "01" * 16
        with self.assertRaisesRegex(ValueError, "source provenance"):
            validation.checked_states(data, Image())

    def test_live_source_mutation_after_entry_declines(self):
        data = report()
        data["checkpoints"][-1]["fpr_source"] = "01" * 16
        with self.assertRaisesRegex(ValueError, "source provenance"):
            validation.checked_states(data, Image())

    def test_raw_word_mismatch_declines_before_execution(self):
        data = report()
        data["checkpoints"][-1]["instruction_word"] = "54600001"
        with self.assertRaisesRegex(ValueError, "raw instruction differs"):
            validation.checked_states(data, Image())

    def test_repeated_pc_cannot_be_silently_deduplicated(self):
        data = report()
        data["checkpoints"].append(copy.deepcopy(data["checkpoints"][-1]))
        with self.assertRaisesRegex(ValueError, "trace-aware"):
            validation.checked_states(data, Image())

    def test_midchain_flags_are_not_positive_chain_evidence(self):
        for key in ("controlled_midchain_l2cr", "controlled_live_bss_source_experiment"):
            with self.subTest(key=key):
                data = report()
                data[key] = True
                with self.assertRaisesRegex(ValueError, "mid-chain perturbation"):
                    validation.checked_states(data, Image())

    def test_midchain_marker_declines_even_with_false_flags(self):
        data = report()
        data["checkpoints"][-1]["controlled_midchain_l2cr_write"] = "00000001"
        with self.assertRaisesRegex(ValueError, "mid-chain perturbation"):
            validation.checked_states(data, Image())

    def test_short_array_cannot_reduce_zip_comparison_coverage(self):
        for key in ("ps0", "ps1", "gqr"):
            with self.subTest(key=key):
                data = report()
                data["checkpoints"][-1][key].pop()
                with self.assertRaisesRegex(ValueError, "incomplete raw state array"):
                    validation.checked_states(data, Image())

    def test_wrong_width_or_invalid_hex_is_not_a_raw_state(self):
        for key, value in (("gpr", "00000000" * 31), ("fpr_source", "0" * 31),
                           ("l2cr", "0000000"), ("fpscr", "zzzzzzzz")):
            with self.subTest(key=key):
                data = report()
                data["checkpoints"][-1][key] = value
                with self.assertRaisesRegex(ValueError, "incomplete raw"):
                    validation.checked_states(data, Image())


def disabled_effects():
    # Values and order taken from the three consumed bits and original CFG;
    # equal readbacks at separate sites must not make those reads exchangeable.
    return [
        [0, 0x80372894, 0], [3, 0x803728AC, 0], [3, 0x803728B8, 0],
        [3, 0x803728BC, 0], [0, 0x803728C0, 0], [1, 0x803728C8, 0],
        [3, 0x803728CC, 0], [3, 0x80372650, 0], [0, 0x80372654, 0],
        [1, 0x8037265C, 0], [3, 0x80372660, 0], [0, 0x80372664, 0],
        [1, 0x8037266C, 0x200000], [3, 0x8037266C, 0x200000],
        [2, 0x8037266C, 1], [0, 0x80372678, 0x200000],
        [0, 0x80372688, 0x200000], [1, 0x80372690, 0],
        [0, 0x803726B4, 0], [0, 0x803728DC, 0], [1, 0x803728E8, 0x80000000],
    ]


def effect_states():
    values = {0x80003154: 0, 0x80372898: 0, 0x80372640: 0,
              0x8037266C: 0, 0x80372678: 0x200000, 0x80372684: 0x200000,
              0x80372690: 0x200000, 0x803726B4: 0, 0x803726C0: 0,
              0x803728DC: 0, 0x803728EC: 0x80000000}
    return {pc: {"l2cr": f"{value:08x}"} for pc, value in values.items()}


class OrderedEffectTests(unittest.TestCase):
    def test_complete_disabled_effects_are_accepted(self):
        validation.checked_effects(disabled_effects(), effect_states(), True)

    def test_equal_value_reads_at_different_sites_are_not_exchangeable(self):
        effects = disabled_effects()
        self.assertEqual(effects[8][2], effects[11][2])
        effects[8], effects[11] = effects[11], effects[8]
        with self.assertRaisesRegex(ValueError, "ordered L2"):
            validation.checked_effects(effects, effect_states(), True)

    def test_missing_fresh_read_cannot_reuse_previous_poll_value(self):
        effects = disabled_effects()
        del effects[16]  # Fresh read after first poll, before command clear.
        with self.assertRaisesRegex(ValueError, "ordered L2"):
            validation.checked_effects(effects, effect_states(), True)

    def test_completion_cannot_be_reordered_or_claim_another_generation(self):
        for change in ("before_request", "after_poll", "wrong_generation"):
            with self.subTest(change=change):
                effects = disabled_effects()
                if change == "before_request":
                    effects.insert(12, effects.pop(14))
                elif change == "after_poll":
                    effects.insert(15, effects.pop(14))
                else:
                    effects[14][2] = 2
                with self.assertRaisesRegex(ValueError, "ordered L2"):
                    validation.checked_effects(effects, effect_states(), True)

    def test_unknown_effect_is_not_silently_discarded(self):
        effects = disabled_effects()
        effects.insert(10, [4, 0x80372660, 0])
        with self.assertRaisesRegex(ValueError, "ordered L2"):
            validation.checked_effects(effects, effect_states(), True)

    def test_enabled_branch_performs_only_initial_read(self):
        states = {0x80003154: {"l2cr": "c0480000"}}
        validation.checked_effects([[0, 0x80372894, 0xC0480000]], states, False)
        for extra in ([1, 0x803728C8, 0x40480000], [2, 0x8037266C, 1],
                      [3, 0x803728AC, 0xC0480000]):
            with self.subTest(extra=extra):
                with self.assertRaisesRegex(ValueError, "enabled branch"):
                    validation.checked_effects([[0, 0x80372894, 0xC0480000], extra], states, False)


class KnownMemoryTests(unittest.TestCase):
    paired = "1111111100001111"

    @staticmethod
    def stored_mask(addresses):
        raw = ["0"] * 0x90
        for address in addresses:
            for n in range(address - 0x8060C570, address - 0x8060C570 + 4):
                raw[n] = "1"
        return "".join(raw)

    def test_independent_store_timeline_is_required(self):
        addresses = {0x8060C5E0, 0x8060C5E8, 0x8060C5EC, 0x8060C5F4}
        validation.checked_validity(0x80372898, self.paired, self.stored_mask(addresses), True)
        addresses |= {0x8060C5E4, 0x8060C5D0, 0x8060C5DC}
        validation.checked_validity(0x8037266C, self.paired, self.stored_mask(addresses), True)
        addresses.add(0x8060C570)
        validation.checked_validity(0x80370CB4, self.paired, self.stored_mask(addresses), True)
        addresses |= {0x8060C578 + 4*n for n in range(8)}
        validation.checked_validity(0x80370CD4, self.paired, self.stored_mask(addresses), True)
        self.assertEqual(self.stored_mask(addresses).count("1"), 64)

    def test_zero_or_missing_window_cannot_hide_materialized_words(self):
        for mask in ("0" * 0x90, "-"):
            with self.subTest(mask=mask):
                with self.assertRaisesRegex(ValueError, "L2 validity"):
                    validation.checked_validity(0x80370CD4, self.paired, mask, True)

    def test_claiming_unknown_bytes_are_initialized_declines(self):
        with self.assertRaisesRegex(ValueError, "L2 validity"):
            validation.checked_validity(0x80370CD4, self.paired, "1" * 0x90, True)

    def test_paired_mask_cannot_suppress_existing_prefix_stores(self):
        with self.assertRaisesRegex(ValueError, "paired validity"):
            validation.checked_validity(0x80371730, "0" * 16, "-", True)
        validation.checked_validity(0x80371730, "1111000000001111", "-", True)

    def test_enabled_branch_cannot_claim_helper_or_logger_writes(self):
        addresses = {0x8060C5E0, 0x8060C5E8, 0x8060C5EC, 0x8060C5F4}
        validation.checked_validity(0x80372904, self.paired, self.stored_mask(addresses), False)
        addresses.add(0x8060C5E4)
        with self.assertRaisesRegex(ValueError, "L2 validity"):
            validation.checked_validity(0x80372904, self.paired, self.stored_mask(addresses), False)

    def test_checkpoint_order_follows_call_return_edges(self):
        disabled = validation.checkpoint_order(True)
        enabled = validation.checkpoint_order(False)
        self.assertEqual(len(disabled), len(set(disabled)))
        self.assertEqual(disabled[-2:], [0x803728F8, 0x80372904])
        self.assertLess(disabled.index(0x803728A0), disabled.index(0x80372640))
        self.assertLess(disabled.index(0x803726C4), disabled.index(0x803728D4))
        self.assertLess(disabled.index(0x803728EC), disabled.index(0x80370C8C))
        self.assertEqual(enabled, [pc for pc in disabled if pc not in validation.L2_DISABLED_PCS])


if __name__ == "__main__":
    unittest.main()

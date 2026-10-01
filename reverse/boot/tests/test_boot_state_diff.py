"""Adversarial state-schema and opt-in CRT capture input gates.

Pure synthetic inputs test tooling contracts; they are not machine evidence.
"""
from pathlib import Path
import re
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from boot_state_diff import (FIELD_NAMES, OrderedTrace, compare_fields,
                             compare_memory, hex_field, parse_native_state,
                             reference_fields, word_validity)
from capture_crt_state import CrtExperiment, ZERO_RANGES
from validate_native_crt import TAIL_ORDER, checked_report


class ReportImage:
    """Raw-value fixture for report-shape gates, never a game image."""
    def read(self, address, size): return bytes(size)
    def word(self, address, *, text=True): return 0


def crt_report():
    # Minimal prefix plus the full occurrence plan. No assertion is made
    # that these synthetic all-zero register fields can execute this path.
    canaries = [a-4 for a,_ in ZERO_RANGES] + [a+n for a,n in ZERO_RANGES]
    values = [f"{0xD0E0F001+n:08x}" for n in range(6)]
    seed_words = ["81234567"] + values
    seeds = [{"address":f"{address:08x}", "value":value, "readback":value}
             for address,value in zip([0x80586CB4]+canaries,seed_words)]
    states = []
    cut = 2
    first_clear = cut + 20 + 12 + 3
    slot_write = cut + TAIL_ORDER.index(0x803733C4)
    for index,pc in enumerate([0x80003154,0x80372904]+TAIL_ORDER):
        state = reference(); state["pc"] = f"{pc:08x}"
        slot = "81234567" if index < slot_write else "803726d8" if index < first_clear else "00000000"
        state.update(instruction_word="00000000",handler_slot=slot,crt_descriptors="00"*0xA4,
                     fpr_source="00"*16,crt_canaries=values.copy(),bi2_pointer="817e54e0",
                     lowmem_44="00000000",l2_stack_address="8060c570",
                     l2_stack_bytes="00"*0x90,paired_stack_bytes="00"*16,
                     preentry_seed_words=[slot]+values.copy())
        states.append(state)
    return {"checkpoints":states, "region_extension":{
        "name":"handler_crt_39", "midchain_writes":False, "stop":"80003188",
        "controlled_initial_handler":"81234567", "controlled_initial_msr":None,
        "preentry_memory_writes":seeds,
        "canary_ranges":[[f"{address:08x}",4] for address in canaries],
        "filled_ranges":[{"address":f"{address:08x}", "size":f"{size:08x}", "bytes":"00"*size}
                         for address,size in ZERO_RANGES]}}


def native_words(count=120):
    words = ["00000000"] * 113
    words[0] = "80003188"
    for n in range(38, 102): words[n] = "0000000000000000"
    words += ["00" * 16, "0" * 16]
    if count >= 118: words += ["80000000", "00" * 144, "0" * 144]
    if count == 120: words += ["00000000", "00000000"]
    return words


def reference():
    state = {k: "00000000" for k in
             ("pc", "msr", "lr", "cr", "architectural_xer", "fpscr", "ctr", "hid0", "hid2")}
    state["pc"] = "80003188"
    state["gpr"] = "00000000" * 32
    state["ps0"] = ["0000000000000000"] * 32
    state["ps1"] = ["0000000000000000"] * 32
    state["gqr"] = ["00000000"] * 8
    return state


class StateShapeTests(unittest.TestCase):
    def test_each_supported_native_schema_preserves_all_raw_fields(self):
        for count in (115, 118, 120):
            with self.subTest(count=count):
                words = native_words(count)
                self.assertEqual(parse_native_state(" ".join(words)), words)
        self.assertEqual(len(FIELD_NAMES), 113)

    def test_missing_or_extra_native_field_cannot_reduce_coverage(self):
        for count in (0, 114, 116, 117, 119, 121):
            with self.subTest(count=count), self.assertRaises(ValueError):
                parse_native_state(" ".join((native_words() + ["0"] * 2)[:count]))

    def test_unknown_or_short_native_register_lane_global_declines(self):
        for index in (0, 1, 5, 9, 38, 69, 70, 101, 104, 112, 113, 115, 116, 118, 119):
            for replacement in ("UNKNOWN", "0", "-1", "0x00000000"):
                with self.subTest(index=index, replacement=replacement):
                    words = native_words(); words[index] = replacement
                    with self.assertRaises(ValueError): parse_native_state(" ".join(words))

    def test_native_memory_placeholder_requires_both_bytes_and_mask(self):
        words = native_words(118); words[116:118] = ["-", "-"]
        parse_native_state(" ".join(words))
        for pair in (("-", "0" * 144), ("00" * 144, "-")):
            with self.subTest(pair=pair):
                words[116:118] = pair
                with self.assertRaises(ValueError): parse_native_state(" ".join(words))

    def test_memory_mask_shape_and_alphabet_are_strict(self):
        for index in (114, 117):
            for replacement in ("", "UNKNOWN", "2" * (16 if index == 114 else 144), "0"):
                with self.subTest(index=index, replacement=replacement):
                    words = native_words(); words[index] = replacement
                    with self.assertRaises(ValueError): parse_native_state(" ".join(words))

    def test_complete_reference_yields_the_same_113_register_fields(self):
        fields = reference_fields(reference())
        self.assertEqual(fields, native_words()[:113])

    def test_truncated_reference_arrays_cannot_make_zip_pass(self):
        for key in ("ps0", "ps1", "gqr"):
            for alteration in ("short", "long", "unknown", "wrong_width"):
                with self.subTest(key=key, alteration=alteration):
                    state = reference()
                    if alteration == "short": state[key].pop()
                    elif alteration == "long": state[key].append(state[key][0])
                    elif alteration == "unknown": state[key][-1] = "UNKNOWN"
                    else: state[key][-1] = "0"
                    with self.assertRaises(ValueError): reference_fields(state)

    def test_short_reference_gpr_and_symbolic_scalars_decline(self):
        for key, value in (("gpr", "00000000" * 31), ("msr", "UNKNOWN"),
                           ("lr", "0"), ("fpscr", "000000000")):
            with self.subTest(key=key):
                state = reference(); state[key] = value
                with self.assertRaises(ValueError): reference_fields(state)

    def test_generic_hex_fields_reject_noncanonical_numbers(self):
        for value in (None, 0, "-0000000", "000000000", "0x00000000", "0000 0000"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                hex_field(value, 8, "raw")

    def test_field_diff_identifies_first_actual_register(self):
        actual = native_words()[:113]; expected = actual.copy()
        actual[9] = "00000001"; actual[10] = "00000002"
        with self.assertRaisesRegex(ValueError, "checkpoint 4: r3"):
            compare_fields(actual, expected, FIELD_NAMES, "checkpoint 4")

    def test_compare_shapes_decline_instead_of_zipping_short_input(self):
        for actual, expected, names in ((["00000000"], [], ["r3"]),
                                       (["00000000"], ["00000000"], [])):
            with self.subTest(actual=actual, expected=expected):
                with self.assertRaises(ValueError): compare_fields(actual, expected, names, "cp")

    def test_direct_field_comparison_rejects_noncanonical_equal_numbers(self):
        for value in ("000000000", "-0000000", "0x00000000"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                compare_fields([value], ["00000000"], ["r3"], "cp")


class MemoryAndOrderTests(unittest.TestCase):
    def test_byte_mask_requires_independent_store_timeline(self):
        with self.assertRaisesRegex(ValueError, "validity/store timeline"):
            compare_memory("00000000", "0000", "00000000", "1111", "cp", 0x80001000)

    def test_unknown_bytes_are_not_assumed_zero_or_compared_as_known(self):
        self.assertEqual(compare_memory("0000", "10", "00ff", "10", "cp", 0x80001000), 1)
        with self.assertRaisesRegex(ValueError, "address=80001001"):
            compare_memory("0000", "11", "00ff", "11", "cp", 0x80001000)

    def test_memory_length_and_mask_alphabet_are_checked(self):
        for actual, mask, observed in (("0000", "1", "0000"),
                                       ("0000", "11", "00"), ("00", "2", "00")):
            with self.subTest(actual=actual, mask=mask, observed=observed):
                with self.assertRaises(ValueError):
                    compare_memory(actual, mask, observed, mask, "cp", 0x80001000)

    def test_noncanonical_memory_hex_is_rejected(self):
        for actual, observed in (("00 00", "0000"), ("0000", "00 00")):
            with self.subTest(actual=actual, observed=observed), self.assertRaises(ValueError):
                compare_memory(actual, "11", observed, "11", "cp", 0x80001000)

    def test_overlapping_word_writes_form_one_address_mask(self):
        self.assertEqual(word_validity(0x80001000, 8, [0x80001000, 0x80001002]), "11111100")
        for address in (0x80000FFF, 0x80001005):
            with self.subTest(address=address), self.assertRaises(ValueError):
                word_validity(0x80001000, 8, [address])

    def test_repeated_pcs_are_distinct_ordered_occurrences(self):
        states = [{"pc": "8000336c", "descriptor": n} for n in range(3)]
        trace = OrderedTrace(states)
        self.assertEqual([trace.take(0x8000336C)["descriptor"] for _ in states], [0, 1, 2])
        trace.finish()

    def test_order_errors_leftovers_and_premature_end_are_rejected(self):
        trace = OrderedTrace([{"pc": "8000336c"}, {"pc": "800033ac"}])
        with self.assertRaisesRegex(ValueError, "occurrence 0"): trace.take(0x800033AC)
        trace.take(0x8000336C)
        with self.assertRaisesRegex(ValueError, "unconsumed"): trace.finish()
        trace.take(0x800033AC); trace.finish()
        with self.assertRaisesRegex(ValueError, "occurrence 2"): trace.take(0x800033AC)


class CrtReportShapeTests(unittest.TestCase):
    def test_complete_synthetic_report_preserves_all_tail_occurrences(self):
        prefix,tail = checked_report(crt_report(), ReportImage())
        self.assertEqual(len(prefix["checkpoints"]),2)
        self.assertEqual([int(s["pc"],16) for s in tail],TAIL_ORDER)

    def test_missing_or_extra_canary_values_decline_at_any_checkpoint(self):
        for index in (0,2,-1):
            for count in (0,5,7):
                with self.subTest(index=index,count=count):
                    report=crt_report()
                    report["checkpoints"][index]["crt_canaries"] = (["00000000"]*count)
                    with self.assertRaisesRegex(ValueError,"boundary observations incomplete"):
                        checked_report(report,ReportImage())

    def test_canary_range_missing_extra_misaligned_width_or_order_declines(self):
        for change in ("missing","extra","misaligned","wrong_width","wrong_order"):
            with self.subTest(change=change):
                report=crt_report();ranges=report["region_extension"]["canary_ranges"]
                if change=="missing":ranges.pop()
                elif change=="extra":ranges.append(["80000000",4])
                elif change=="misaligned":ranges[0][0]=f"{int(ranges[0][0],16)+1:08x}"
                elif change=="wrong_width":ranges[0][1]=3
                else:ranges[0],ranges[1]=ranges[1],ranges[0]
                with self.assertRaisesRegex(ValueError,"boundary addresses/widths differ"):
                    checked_report(report,ReportImage())

    def test_canary_words_are_complete_raw_words(self):
        for word in ("UNKNOWN","0","000000000","0x00000000"):
            with self.subTest(word=word):
                report=crt_report()
                for state in report["checkpoints"]:state["crt_canaries"][0]=word
                with self.assertRaisesRegex(ValueError,"outside boundary word"):
                    checked_report(report,ReportImage())

    def test_same_address_canary_and_seed_views_cannot_disagree(self):
        report=crt_report()
        for state in report["checkpoints"]:state["crt_canaries"][0]="00000000"
        with self.assertRaisesRegex(ValueError,"boundary/seed observations disagree"):
            checked_report(report,ReportImage())

    def test_wrong_stack_base_or_truncated_window_cannot_relabel_bytes(self):
        for key,value in (("l2_stack_address","8060c574"),("l2_stack_address","0x8060c570"),
                          ("l2_stack_bytes","00"*143),("paired_stack_bytes","00"*15)):
            with self.subTest(key=key):
                report=crt_report();report["checkpoints"][-1][key]=value
                with self.assertRaises(ValueError):checked_report(report,ReportImage())

    def test_fill_labels_require_exact_raw_address_and_size(self):
        for key in ("address","size"):
            for change in ("short","extra","prefixed"):
                with self.subTest(key=key,change=change):
                    report=crt_report();item=report["region_extension"]["filled_ranges"][-1]
                    value=item[key]
                    item[key]=value[1:] if change=="short" else "0"+value if change=="extra" else "0x"+value
                    with self.assertRaises(ValueError):checked_report(report,ReportImage())


class FakeRSP:
    def __init__(self, corrupt_address=None, reject=False, unknown_msr=False, msr_reply=None):
        self.bytes = {}; self.packets = []; self.msr = 0x2032
        self.corrupt_address, self.reject, self.unknown_msr = corrupt_address, reject, unknown_msr
        self.msr_reply = msr_reply

    def send(self, packet):
        self.packets.append(packet)
        if packet.startswith("P41="):
            if self.reject: return "E01"
            self.msr = int(packet[4:], 16); return "OK"
        if packet == "p41":
            if self.msr_reply is not None: return self.msr_reply
            return "UNKNOWN" if self.unknown_msr else f"{self.msr:08x}"
        match = re.fullmatch(r"M([0-9a-f]+),4:([0-9a-f]{8})", packet)
        if not match: raise AssertionError(f"unexpected packet {packet}")
        if self.reject: return "E01"
        address = int(match[1], 16); raw = bytes.fromhex(match[2])
        if address != self.corrupt_address:
            for n, byte in enumerate(raw): self.bytes[address+n] = byte
        return "OK"

    def memory(self, address, size):
        return bytes(self.bytes.get(address+n, 0) for n in range(size))


class CrtCaptureInputTests(unittest.TestCase):
    def test_valid_preentry_seeds_are_labelled_and_read_back(self):
        rsp = FakeRSP(); experiment = CrtExperiment(0x81234567, 0xA032, poison=True)
        experiment.prepare(rsp)
        metadata = experiment.metadata()
        self.assertEqual(metadata["controlled_initial_handler"], "81234567")
        self.assertEqual(metadata["controlled_initial_msr"], "0000a032")
        self.assertFalse(metadata["midchain_writes"])
        self.assertEqual(len(metadata["preentry_memory_writes"]), 16)
        for seed in metadata["preentry_memory_writes"]:
            self.assertEqual(rsp.memory(int(seed["address"], 16), 4).hex(), seed["value"])

    def test_bad_opt_in_word_inputs_decline_before_any_rsp_write(self):
        for handler, msr in ((-1, None), (0x100000000, None), (0, -1), (0, 0x100000000),
                             (None, None), ("00000000", None), (0, "00002032")):
            with self.subTest(handler=handler, msr=msr), self.assertRaises(ValueError):
                CrtExperiment(handler, msr)

    def test_acknowledged_but_missing_handler_or_poison_write_declines(self):
        for address in (0x80586CB4, ZERO_RANGES[0][0], ZERO_RANGES[2][0] + ZERO_RANGES[2][1]):
            with self.subTest(address=address), self.assertRaises(ValueError):
                CrtExperiment(0x81234567, poison=True).prepare(FakeRSP(corrupt_address=address))

    def test_msr_readback_must_be_concrete_and_match_request(self):
        with self.assertRaises(ValueError):
            CrtExperiment(0, 0xA032).prepare(FakeRSP(unknown_msr=True))

    def test_noncanonical_msr_readback_cannot_claim_an_exact_word(self):
        for reply in ("000a032", "00000a032", "0x0000a032", "+0000a032"):
            with self.subTest(reply=reply), self.assertRaises(ValueError):
                CrtExperiment(0, 0xA032).prepare(FakeRSP(msr_reply=reply))

    def test_refused_seed_write_never_becomes_successful_metadata(self):
        experiment = CrtExperiment(0x81234567)
        with self.assertRaises(ValueError): experiment.prepare(FakeRSP(reject=True))
        self.assertEqual(experiment.metadata()["preentry_memory_writes"], [])


if __name__ == "__main__":
    unittest.main()

"""Regressions for occurrence matching and clean early clock declines."""
import argparse
import copy
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from validate_produced_clock import checked_inputs, match_region, ppc_checkpoints, word_effects, pin_original_dol
from agent_si_device_falsifier_42 import select_explicit_port
import agent_initial_events_42 as initial_events

OPTIONS = argparse.Namespace(program=None, dol=None)


def capture():
    entry = dict(pc='80003154', msr='00002032', hid0='0011c064', hid2='e0000000',
                 cr='abcdef12', architectural_xer='c000007f', ctr='89312456', fpscr='00000000',
                 lr='00000000', gpr='0'*256, ps0=['0'*16]*32, ps1=['0'*16]*32,
                 gqr=['0'*8]*8, fpr_source='0'*32, l2cr='00000000', handler_slot='00000000',
                 bi2_pointer='817e54e0', bi2_blob='00'*0x2000, clock_low_words=['0'*8]*5,
                 clock_globals=['0'*8]*2,
                 clock_producer=dict(exceptions='00000000', cycles='0000000000000000',
                                     epoch_cycles='0000000000000000', epoch_value='0000000000000000',
                                     cpu_hz='1cf7c580', rtc_offset='0000000000000000', cached_tb='0000000000000000'))
    end = copy.deepcopy(entry)
    end['pc'] = '80373ac4'
    return dict(capture_complete=True, sole_internal_stop='80373ac4', controlled_midchain_writes=False,
                original_unmodified_entry=copy.deepcopy(entry), checkpoints=[entry, end],
                pause_source_readbacks=[copy.deepcopy(entry['clock_producer']) for _ in range(2)],
                region_extension=dict(sampler_mode='continuous', producer_mode='rtc_initial',
                                      midchain_writes=False, stop='80373ac4', requested_epoch=None,
                                      epoch_controls=[], clock_preentry_writes=[], clock_plan=[], bi2_preentry_writes=[]))


class ProducedClockTools(unittest.TestCase):
    def setUp(self):
        root = Path(__file__).resolve().parents[3] / 'build'
        root.mkdir(exist_ok=True)
        self.temp = tempfile.TemporaryDirectory(prefix='produced-clock-test-', dir=root)
        self.addCleanup(self.temp.cleanup)
        self.work = Path(self.temp.name)

    def port_factory(self, unavailable):
        attempts = []

        class Socket:
            def __enter__(self):
                return self

            def __exit__(self, *_):
                return False

            def bind(self, endpoint):
                attempts.append(endpoint)
                if endpoint[1] in unavailable:
                    raise OSError('excluded or already bound')

        return Socket, attempts

    def test_capture_port_skips_excluded_and_busy_explicit_ports(self):
        factory, attempts = self.port_factory({30000, 30001})
        self.assertEqual(select_explicit_port(socket_factory=factory), 30002)
        self.assertEqual(attempts, [('127.0.0.1', n) for n in (30000, 30001, 30002)])
        factory, attempts = self.port_factory(set())
        self.assertEqual(select_explicit_port(39042, factory), 39042)
        self.assertEqual(attempts, [('127.0.0.1', 39042)])

    def test_invalid_requested_port_declines_before_socket_creation(self):
        for requested in (0, -1, 65536, True, 30000.0, '30000'):
            factory, attempts = self.port_factory(set())
            with self.subTest(requested=requested), self.assertRaises(ValueError):
                select_explicit_port(requested, factory)
            self.assertEqual(attempts, [])

    def test_no_explicit_port_available_declines_without_bind_zero(self):
        factory, attempts = self.port_factory(range(30000, 49152))
        with self.assertRaisesRegex(ValueError, 'no explicitly bindable'):
            select_explicit_port(socket_factory=factory)
        self.assertEqual(attempts, [('127.0.0.1', n) for n in range(30000, 49152)])
        factory, attempts = self.port_factory({39042})
        with self.assertRaisesRegex(ValueError, 'no explicitly bindable'):
            select_explicit_port(39042, factory)
        self.assertEqual(attempts, [('127.0.0.1', 39042)])

    def test_missing_checkpoint_cannot_borrow_later_call(self):
        states = [['8037611c'], ['80376124'], ['8037611c'], ['80376120'], ['80376124']]
        events = [{'pc': int(x, 16)} for x in ('8037611c', '80376120', '80376124')]
        pairs, missing = match_region(states, events, 0, 2)
        self.assertEqual([n for n, _ in pairs], [0, 1])
        self.assertEqual(missing, [0x80376120])
        pairs, missing = match_region(states, events, 2, 5)
        self.assertEqual([n for n, _ in pairs], [2, 3, 4])
        self.assertEqual(missing, [])

    def test_original_input_gate_rejects_unconsumed_binary_changes(self):
        if OPTIONS.dol is None:
            self.skipTest('original fixture argument absent')
        pin_original_dol(OPTIONS.dol)
        raw = bytearray(OPTIONS.dol.read_bytes())
        raw[-1] ^= 1  # outside the current executed prefix; word gates cannot certify it
        path = self.work/'changed-original.dol'; path.write_bytes(raw)
        with self.assertRaisesRegex(ValueError, 'original PAL DOL identity changed'):
            pin_original_dol(path)

    def test_duplicate_pc_preserves_occurrences(self):
        events = [{'pc': 0x80379628}, {'pc': 0x80379628}]
        self.assertEqual([n for n, _ in match_region([['80379628']]*2, events, 0, 2)[0]], [0, 1])
        self.assertEqual(match_region([['80379628']], events, 0, 1)[1], [0x80379628])

    def test_wrong_region_bounds_decline(self):
        for begin, end in ((-1, 1), (1, 0), (0, 2)):
            with self.subTest(begin=begin, end=end), self.assertRaises(ValueError):
                match_region([['80379628']], [], begin, end)

    def test_initial_source_contract_and_unexplained_controls(self):
        checked_inputs(capture())
        for key, value in (('capture_complete', False), ('sole_internal_stop', '80373ac8'),
                           ('controlled_midchain_writes', True)):
            data = capture(); data[key] = value
            with self.subTest(key=key), self.assertRaises(ValueError):
                checked_inputs(data)
        for key, value in (('sampler_mode', 'single_step'), ('producer_mode', 'preentry_rebased'),
                           ('clock_plan', ['80379628']), ('epoch_controls', ['forced']),
                           ('clock_preentry_writes', ['forced']), ('requested_epoch', '0000000000000000')):
            data = capture(); data['region_extension'][key] = value
            with self.subTest(key=key), self.assertRaises(ValueError):
                checked_inputs(data)

    def test_initial_event_parity_requires_independent_capture_identity(self):
        data = capture()
        data.update(capture_tool_sha256=initial_events.BASE_CAPTURE_SHA,
                    dolphin_sha256=initial_events.ORACLE_SHA,
                    disc_sha256=initial_events.DISC_SHA,
                    controlled_initial_hid0='0011c064',
                    controlled_initial_l2cr='00000000',
                    observer_environment=dict(
                        event_window='init..timing-prefix-end@80373ac4',
                        event_kinds='', event_limit='3000000',
                        mmio_watch='0c005000:0c005100;0c002000:0c002100;0c003000:0c003008;0c006c00:0c006c20',
                        mmio_watch_phase0=True),
                    instrumentation_manifest=dict(
                        instrumented_executable_sha256=initial_events.ORACLE_SHA,
                        passive_interpreter=dict(
                            copied_sha256=initial_events.COPIED_INTERPRETER_SHA,
                            original_sha256=initial_events.clock.SOURCE_PINS[
                                'Source/Core/Core/PowerPC/Interpreter/Interpreter.cpp'])))
        initial_events.validate_capture_contract(data)
        self.assertEqual(initial_events.capture_binding_self_test(data), 14)
        # Equality between two supplied hashes does not prove reference identity.
        changed = copy.deepcopy(data)
        changed['dolphin_sha256'] = '0'*64
        changed['instrumentation_manifest']['instrumented_executable_sha256'] = '0'*64
        with self.assertRaisesRegex(ValueError, 'unrelated reference oracle'):
            initial_events.validate_capture_contract(changed)
        for malformed in (None, []):
            with self.subTest(contract=malformed), self.assertRaises(ValueError):
                initial_events.validate_capture_contract(malformed)
            changed = copy.deepcopy(data)
            changed['instrumentation_manifest'] = malformed
            with self.subTest(manifest=malformed), self.assertRaises(ValueError):
                initial_events.validate_capture_contract(changed)
            changed = copy.deepcopy(data)
            changed['instrumentation_manifest']['passive_interpreter'] = malformed
            with self.subTest(private=malformed), self.assertRaises(ValueError):
                initial_events.validate_capture_contract(changed)

    def test_unexplained_entry_source_never_becomes_clock(self):
        for key in ('epoch_cycles', 'epoch_value', 'rtc_offset', 'cached_tb', 'exceptions', 'cpu_hz'):
            data = capture()
            for state in [data['original_unmodified_entry']] + data['checkpoints']:
                state['clock_producer'][key] = '0'*(len(state['clock_producer'][key])-1) + '1'
            data['pause_source_readbacks'] = [copy.deepcopy(data['checkpoints'][0]['clock_producer']) for _ in range(2)]
            with self.subTest(key=key), self.assertRaises(ValueError):
                checked_inputs(data)

    def test_equal_nonzero_initial_and_final_globals_are_not_initialization(self):
        data = capture()
        for state in [data['original_unmodified_entry']] + data['checkpoints']:
            state['clock_globals'] = ['00000001', '00000001']
        with self.assertRaises(ValueError):
            checked_inputs(data)

    def test_truncated_event_snapshot_declines(self):
        row = '\t'.join(['1', '2', 'ppc', '-', '0000000080379628'] + ['0'*16]*7 + ['source', '1', 'function', '-'])
        path = self.work/'events.tsv'; path.write_text(row+'\n', encoding='utf-8')
        with self.assertRaises(ValueError):
            ppc_checkpoints(path)

    def test_word_width_and_unknown_value_decline(self):
        row = ['1', 'w', '1', '0', '80370ea8', '80370ea8', '00002032', '0',
               '005f1f54', '00000004', '805f1f54', '00000001', 'deadbeef', '-']
        path = self.work/'writes.tsv'; path.write_text('\t'.join(row)+'\n', encoding='utf-8')
        self.assertEqual(word_effects(path), [[0x80370EA8, 0x805F1F54, 0xDEADBEEF]])
        for n, value in ((9, '00000008'), (11, '00000000'), (12, 'unknown0')):
            altered = list(row); altered[n] = value
            path.write_text('\t'.join(altered)+'\n', encoding='utf-8')
            with self.subTest(n=n), self.assertRaises(ValueError):
                word_effects(path)

    def test_compiled_earlier_unknown_prefix_declines_without_empty_back(self):
        if OPTIONS.program is None:
            self.skipTest('compiled fixture arguments absent')
        _, _, _, _, fixture = checked_inputs(capture())
        words = fixture.split(); words[-3] = '-'
        path = self.work/'unknown-handler.entry.txt'; path.write_text(' '.join(words)+'\n', encoding='ascii')
        result = subprocess.run([str(OPTIONS.program), str(OPTIONS.dol), str(path), '00000000', '1cf7c580',
                                 '0000000000000000', '0000000000000000', '0000000000000000',
                                 '0000000000000000', '00000000', 'research-continuous'],
                                capture_output=True, text=True, timeout=30)
        self.assertEqual(result.returncode, 1)
        self.assertIn('clock prefix stopped before validated BI2 entry', result.stderr)
        self.assertEqual(result.stdout, '')

    def test_compiled_produced_work_forbids_reference_phase_and_unwitnessed_path(self):
        if OPTIONS.program is None:
            self.skipTest('compiled fixture arguments absent')
        data = capture()
        _, _, _, _, fixture = checked_inputs(data)
        path = self.work/'entry.txt'; path.write_text(fixture, encoding='ascii')
        args = [str(OPTIONS.program), str(OPTIONS.dol), str(path), '00000000', '1cf7c580',
                '0000000000000000', '-', '0000000000000000', '0000000000000000',
                '00000000', 'research-produced-work']
        for n, value in ((6, '0000000000000000'), (5, '0000000000000001')):
            altered = list(args); altered[n] = value
            result = subprocess.run(altered, capture_output=True, text=True, timeout=30)
            self.assertEqual(result.returncode, 1)
            self.assertIn('forbids a supplied frontier or unexplained epoch phase', result.stderr)
        words = fixture.split(); raw = bytearray.fromhex(words[-1]); raw[15] = 2; words[-1] = raw.hex()
        path.write_text(' '.join(words)+'\n', encoding='ascii')
        result = subprocess.run(args, capture_output=True, text=True, timeout=30)
        self.assertEqual(result.returncode, 1)
        self.assertIn('continuous timing witness missing', result.stderr)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--dol', type=Path)
    parser.add_argument('--program', type=Path)
    OPTIONS, rest = parser.parse_known_args()
    if OPTIONS.program:
        OPTIONS.program = OPTIONS.program.resolve(); OPTIONS.dol = OPTIONS.dol.resolve()
    unittest.main(argv=[sys.argv[0]]+rest)

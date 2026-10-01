"""Fail-closed passive observer regressions; not a native clock parity gate."""
import sys
from pathlib import Path
import tempfile
import unittest

TOOLS = Path(__file__).resolve().parents[1] / 'tools'
sys.path.insert(0, str(TOOLS))
from build_timing_trace_oracle import observe_source, OBSERVER
from capture_timing_prefix import validate_instruction_tail, validate_event_tail
from agent_timing_source_42 import self_test as lifecycle_test
from agent_timing_adversarial_42 import falsifications

FIXTURE = '''#include "Core/System.h"
      HLE::TryReplaceFunction(m_ppc_symbol_db, address, PowerPC::CoreMode::Interpreter);
  if (!result)
int Interpreter::SingleStepInner()
{
  OraclePhysicalWrites::Checkpoint(m_system);
    return PPCTables::GetOpInfo(m_prev_inst, m_ppc_state.pc)->num_cycles;
  const GekkoOPInfo* opinfo = PPCTables::GetOpInfo(m_prev_inst, m_ppc_state.pc);
  return opinfo->num_cycles;
}
'''
INSTRUCTION = ('1 exit 80373ac0 3ca08000 00000001 00000000 80373ac4 80373ac4 '
               '00000000002b31bb 00000000002b2015 0000000000003c4b '
               '00002aa5 3f800000 00002032 00000000\n')
EVENT = '\t'.join(['1', '2', 'timing-prefix-end', '-','0000000080373ac4'] +
                  ['0000000000000000'] * 7 + ['source', '1', 'function', '-']) + '\n'

class TimingPrefixTools(unittest.TestCase):
    def setUp(self):
        root = Path(__file__).resolve().parents[3] / 'build'
        root.mkdir(exist_ok=True)
        self.tmp = tempfile.TemporaryDirectory(prefix='timing-prefix-test-', dir=root)
        self.addCleanup(self.tmp.cleanup)
        self.path = Path(self.tmp.name) / 'trace.txt'

    def check_text(self, text, checker):
        self.path.write_text(text, encoding='utf-8')
        return checker(self.path)

    def test_complete_instruction_endpoint(self):
        self.check_text(INSTRUCTION, validate_instruction_tail)

    def test_terminated_process_tail_declines(self):
        for text in ('', INSTRUCTION[:-1], INSTRUCTION[:90]):
            with self.subTest(text=text), self.assertRaises(ValueError):
                self.check_text(text, validate_instruction_tail)

    def test_wrong_state_endpoint_declines(self):
        for text in (INSTRUCTION.replace('80373ac4','80373ac8'),
                     INSTRUCTION.replace('exit','enter'),
                     INSTRUCTION.replace('3f800000','unknown0'),
                     INSTRUCTION.replace('1 exit','0 exit')):
            with self.subTest(text=text), self.assertRaises(ValueError):
                self.check_text(text, validate_instruction_tail)

    def test_complete_event_endpoint(self):
        self.check_text(EVENT, validate_event_tail)

    def test_event_absent_or_unbounded_declines(self):
        for text in ('', EVENT[:-1], EVENT.replace('timing-prefix-end','ppc'),
                     EVENT.replace('80373ac4','80373ac0'), EVENT+'LIMIT\n'):
            with self.subTest(text=text), self.assertRaises(ValueError):
                self.check_text(text, validate_event_tail)

    def test_every_source_anchor_is_required(self):
        for anchor in ('#include "Core/System.h"', 'HLE::TryReplaceFunction',
                       'int Interpreter::SingleStepInner()',
                       'return PPCTables::GetOpInfo', 'const GekkoOPInfo* opinfo',
                       '  return opinfo->num_cycles;'):
            with self.subTest(anchor=anchor), self.assertRaises(ValueError):
                observe_source(FIXTURE.replace(anchor, 'missing', 1))

    def test_observation_does_not_replace_work(self):
        generated = observe_source(FIXTURE)
        self.assertIn('const u32 shadow_pc=m_ppc_state.pc;', generated)
        self.assertEqual(generated.count('HLE::TryReplaceFunction('), 1)
        self.assertEqual(generated.count('PPCTables::GetOpInfo('), 2)
        self.assertIn('return shadow_cost;', generated)
        self.assertIn('return opinfo->num_cycles;', generated)
        # No observer memory/MMIO/opcode access or virtual-time advancement.
        for operation in ('Advance(', 'SingleStep(', 'ReadFullTimeBaseValue(',
                          'Read_Opcode(', 'HostRead<', 'HostWrite<'):
            self.assertNotIn(operation, OBSERVER)
        self.assertIn('std::fflush(observer.file)', OBSERVER)
        self.assertIn('"timing-prefix-end"', OBSERVER)

    def test_lifecycle_mutation_rejections(self):
        self.assertEqual(lifecycle_test()['declined_mutations'], 9)

    def test_shortcut_counterexamples(self):
        self.assertEqual(falsifications()['falsified_assumptions'], 22)

if __name__ == '__main__':
    unittest.main()

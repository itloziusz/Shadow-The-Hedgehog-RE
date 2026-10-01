"""Fail-closed timing and raw-selector regression without private captures."""
import sys
from pathlib import Path
import unittest
from copy import deepcopy
from unittest.mock import patch
from types import SimpleNamespace
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'recognizer'))
from validate_clock_research import counter, ordered_clock_plan, checked_epoch_controls
from agent_clock_adversarial_semantics import compare_high
from cli import run_native_probe
import verify_binary_note as raw_gate

class ClockToolTests(unittest.TestCase):
    def test_before_cycle_phase_and_source_modulo(self):
        self.assertEqual(counter(0,0,11),0)
        self.assertEqual(counter(0,0,12),1)
        self.assertEqual(counter((1<<64)-1,0,12),0)
        self.assertEqual(counter(0,(1<<64)-1,11),1)
    def test_rollover_retries_are_derived_and_ordered(self):
        start=12*((1<<32)-1)+11
        plan=ordered_clock_plan(0,0,start,False)
        self.assertEqual(plan[:5],[0x8037962C,0x80379630,0x80379634,0x80379638,0x80379628])
        self.assertEqual(plan[5:10],[0x8037962C,0x80379630,0x80379634,0x80379638,0x8037963C])
        self.assertEqual(plan[-1],0x80373AC4)
        self.assertLess(plan.index(0x80370EA8),plan.index(0x80370EAC))
    def test_prior_ee_selects_restore_arm_without_assuming_full_msr(self):
        zero=ordered_clock_plan(0,0,0,False);enabled=ordered_clock_plan(0,0,0,True)
        self.assertIn(0x80376158,zero);self.assertNotIn(0x80376150,zero)
        self.assertIn(0x80376150,enabled);self.assertNotIn(0x80376158,enabled)
    def test_rebase_requires_ordered_entry_source_provenance(self):
        original=dict(exceptions='00000000',cycles='0000000000000017',epoch_cycles='0000000000000000',epoch_value='0123456789abcdef',cpu_hz='1cf7c580',rtc_offset='0000000000000000',cached_tb='0000000000000000')
        expected=dict(original);effects=[];request=0xFFFFFFFFFFFFCDD9
        for reg,value in ((250,request&0xffffffff),(251,request>>32),(252,1)):
            expected['epoch_value']=f'{(int(original["epoch_value"],16)&0xffffffff00000000)|value:016x}' if reg==250 else f'{request:016x}'
            if reg==252:expected.update(epoch_cycles=expected['cycles'],cached_tb=expected['epoch_value'])
            effects.append(dict(packet=f'P{reg:x}={value:08x}',pc='80003154',readback=dict(expected)))
        report=dict(original_unmodified_entry=dict(pc='80003154',clock_producer=original),checkpoints=[dict(pc='80003154',clock_producer=expected)],instrumentation_manifest=dict(clock_perturbations=dict(enabled=True,allowed_pc='80003154')),region_extension=dict(producer_mode='preentry_rebased',requested_epoch=f'{request:016x}',epoch_controls=effects))
        self.assertEqual(checked_epoch_controls(report),request)
        bad=[]
        x=deepcopy(report);x['region_extension']['epoch_controls'].reverse();bad.append(x)
        x=deepcopy(report);x['region_extension']['producer_mode']='rtc_initial';bad.append(x)
        x=deepcopy(report);x['region_extension']['epoch_controls'][2]['pc']='80379628';bad.append(x)
        x=deepcopy(report);x['instrumentation_manifest']['clock_perturbations']['enabled']=False;bad.append(x)
        x=deepcopy(report);x['checkpoints'][0]['clock_producer']['cached_tb']='0000000000000000';bad.append(x)
        for field in original:
            x=deepcopy(report);w=x['region_extension']['epoch_controls'][0]['readback'][field];x['region_extension']['epoch_controls'][0]['readback'][field]=f'{int(w,16)^1:0{len(w)}x}';bad.append(x)
        for x in bad:
            with self.assertRaises(ValueError):checked_epoch_controls(x)

    def test_failed_high_samples_compare_signed_and_preserve_so(self):
        self.assertEqual(compare_high(0x7fffffff,0x80000000,0x0abcdef0,0x80000000),0x5abcdef0)
        self.assertEqual(compare_high(0xffffffff,0,0x0abcdef0,0),0x8abcdef0)
    def test_research_stop_cannot_be_parsed_as_native_progress(self):
        completed=SimpleNamespace(returncode=0,stdout='RESEARCH_STOP pc=0x80373ac4 reason=NO_NATIVE_CLOCK_PROVIDER\n',stderr='')
        with patch('cli.subprocess.run',return_value=completed):
            with self.assertRaises(ValueError):run_native_probe(Path('native-research'),Path('dol'),dict(msr=None,hid2=None,hid0=None))
    def test_raw_branch_gate_checks_greater_predicate(self):
        self.assertEqual(raw_gate.signed_branch_target(0x80373ACC,0x41810010,'bgt'),0x80373ADC)
        for word in (0x41800010,0x41810011,0x41810012,0x41810014):
            if word==0x41810014:continue # a different legal target is checked against its note
            with self.assertRaises(ValueError):raw_gate.signed_branch_target(0x80373ACC,word,'bgt')

if __name__=='__main__':unittest.main()

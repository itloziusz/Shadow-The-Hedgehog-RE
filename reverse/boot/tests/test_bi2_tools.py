"""Provenance/shape and packed-register regression; no invented reference proof."""
import copy
from pathlib import Path
import sys
import unittest
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'tools'))
from capture_bi2_state import gpr
from validate_native_bi2 import checked_input,plan,word
from verify_binary_note import signed_branch_target


def fixture():
    blob=bytes(8192)
    def state(pc):
        return dict(pc=f'{pc:08x}',msr='00000000',lr='00000000',cr='00000000',architectural_xer='00000000',fpscr='00000000',
                    gpr='0'*256,ps0=['0'*16]*32,ps1=['0'*16]*32,gqr=['0'*8]*8,ctr='0'*8,hid0='0'*8,hid2='0'*8,
                    bi2_pointer='817e54e0',bi2_blob=blob.hex(),bi2_globals=['0'*8]*4,lowmem_34='0'*8,lowmem_48='0'*8,
                    instruction_word='0'*8,l2_stack_address='8060c570',l2_stack_bytes='0'*288,paired_stack_bytes='0'*32)
    report=dict(original_unmodified_entry=state(0x80003154),checkpoints=[state(0x80003154),state(0x80003188)]+[state(pc) for pc in plan(blob)],
                region_extension=dict(name='bi2_os_40',midchain_writes=False,bi2_pointer='817e54e0',bi2_preentry_writes=[],stop='80379628'))
    class Image:
        def word(self,pc,**kw):return 0
    return report,blob,Image()


class Tools(unittest.TestCase):
    def test_original_bo13_prediction_hint_keeps_eq_predicate(self):
        self.assertEqual(signed_branch_target(0x80003208,0x41A20050,'beq'),0x80003258)
        self.assertEqual(signed_branch_target(0x80003214,0x41A20044,'beq'),0x80003258)
        with self.assertRaises(ValueError):signed_branch_target(0x80003208,0x41E20050,'beq')
    def test_packed_bank_not_nibble_or_list(self):
        values=[0xA1000000+n for n in range(32)]
        s={'gpr':''.join(f'{v:08x}' for v in values)}
        for n in (6,7,14,31):self.assertEqual(gpr(s,n),values[n])
        for raw in ([f'{v:08x}' for v in values],s['gpr'][:-1],'UNKNOWN'+s['gpr'][7:],s['gpr']+' '):
            with self.assertRaises(ValueError):gpr({'gpr':raw},7)
        with self.assertRaises(ValueError):gpr(s,32)

    def test_exact_entry_blob_and_route(self):
        report,blob,image=fixture();self.assertEqual(checked_input(report,blob,image)[4],plan(blob))
        for mutation in ('partial','original','pointer','midchain','duplicate','readback','entry','order','stack','symbolic','lowmem'):
            bad=copy.deepcopy(report)
            if mutation=='partial':bad['checkpoints'][0]['bi2_blob']=blob.hex()[:-2]
            elif mutation=='original':bad['original_unmodified_entry']['bi2_blob']='ff'+blob.hex()[2:]
            elif mutation=='pointer':bad['checkpoints'][-1]['bi2_pointer']='817e54e4'
            elif mutation=='midchain':bad['region_extension']['midchain_writes']=True
            elif mutation in ('duplicate','readback'):
                seed=dict(address='817e54ec',value='00000000',readback='00000000')
                bad['region_extension']['bi2_preentry_writes']=[seed,copy.deepcopy(seed)] if mutation=='duplicate' else [dict(seed,readback='00000001')]
            elif mutation=='entry':bad['checkpoints'][0]['bi2_blob']='01'+blob.hex()[2:]
            elif mutation=='order':bad['checkpoints'][-1]['pc']='80379664'
            elif mutation=='stack':bad['checkpoints'][-1]['l2_stack_address']='8060c574'
            elif mutation=='symbolic':bad['checkpoints'][-1]['bi2_globals'][0]='UNKNOWN!'
            elif mutation=='lowmem':bad['checkpoints'][1]['lowmem_48']='00000001'
            with self.subTest(mutation=mutation),self.assertRaises(ValueError):checked_input(bad,blob,image)

    def test_debug_and_count_routes_derived_from_input(self):
        for debug in (2,3,4,0xFFFFFFFF):
            blob=bytearray(8192);blob[12:16]=debug.to_bytes(4,'big');route=plan(blob)
            self.assertEqual(route[-1],0x800031F4 if debug in (2,3) else 0x80379628)
            self.assertEqual(0x80003140 in route,debug==4)
        blob=bytearray(8192);blob[8:12]=(0x100).to_bytes(4,'big');blob[0x100:0x104]=(3).to_bytes(4,'big')
        self.assertEqual(plan(blob).count(0x80003240),3)
        blob[0x100:0x104]=(0xFFFFFFFF).to_bytes(4,'big')
        with self.assertRaises(ValueError):plan(blob)
        blob[8:12]=(0x101).to_bytes(4,'big')
        with self.assertRaises(ValueError):plan(blob)


if __name__=='__main__':unittest.main()

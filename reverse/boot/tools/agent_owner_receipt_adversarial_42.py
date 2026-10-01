#!/usr/bin/env python3
"""Independent receipt/provenance audit; imports no authoring validator."""
from __future__ import annotations
import argparse
import ast
import copy
import hashlib
import json
from pathlib import Path
import re
import shlex
import subprocess

PINS = {
 'Source/Core/Core/PowerPC/Interpreter/Interpreter.cpp':'1993e7a3976be7aa7d276f1e0b25e04bbdcbe1a4555fac837a4794f6bc18128d',
 'Source/Core/Core/PowerPC/Interpreter/Interpreter_LoadStore.cpp':'dbfc463df0e83aa989e7728bec097d3d95f05ec2e0fa2a409291e3b985234553',
 'Source/Core/Core/PowerPC/Interpreter/Interpreter_SystemRegisters.cpp':'698f153ad3a9e457c7d51f9a84358b86140c29b879f35dc39fe70412f23f8556',
 'Source/Core/Core/PowerPC/MMU.cpp':'ffc04c07b33b2ff77f1d72f9d6e09dca65c042d94a970e0e570755103aef9d78',
 'Source/Core/Core/PowerPC/PowerPC.cpp':'cd0de27b7b7359674723a18796fa1032ffd7728a1b177a77d6fefd9376463341',
 'Source/Core/Core/PowerPC/PowerPC.h':'74beb71dd8f8246ace89a463914eb2290faa6b309deb8f22059b8ed92b5fd675',
 'Source/Core/Core/Boot/Boot_BS2Emu.cpp':'8c204a36bd7fe7171d6c8e03e7f07739c11297e483dd6180c6ff7391973ec06d',
 'Source/Core/Core/Boot/Boot.cpp':'2245b1ab41fa4ab092d26416016c05d52b5591c9035f2ed4b1e3ffe8d256daaa',
 'Source/Core/Core/HW/Memmap.cpp':'c8b5b5ea180c73f54736b6835e20d8027d0203cda7641349254fbce99bb2747f',
 'Source/Core/Core/OraclePhysicalWrites.cpp':'baee215768579ae4173b7fbb00364d6193e26ee8eb4012fa891640e12eef1a9d',
 'Source/Core/Core/HW/DVD/DVDInterface.cpp':'b017c9ce65bd788be55c3310d2a0f4cb8687f2a3a7f0aa674131548e45997da1',
 'Source/Core/AudioCommon/Mixer.cpp':'24f91152ffe824019053f89acc77a974992fa9ed19e68b0c94194521987f90a3',
 'Source/Core/AudioCommon/Mixer.h':'612025371230d433dcf62e027d1abd2da8be5c3533a8ed2947a0e3b6f9480bd5',
 'Source/Core/AudioCommon/AudioCommon.cpp':'de119dfd09a5f10309bbdd0b2de83f5666a8c2f413bf87143ab3322d7b527430',
 'Source/Core/AudioCommon/SoundStream.h':'6d78d46f205f66a6804e59975df1d2344f4474021f59b1a85bf7716af3374a43',
 'Source/Core/AudioCommon/WaveFile.cpp':'bd9020f3769c3a4df30932cae3fa7efeb5dddd6f5251ce59221d59edba9d4961',
 'Source/Core/Common/Config/Layer.cpp':'af5702de9c65d59902989b362da716acbc8bdcfb9a8c9fbf23ab67f389ad2c8f',
 'Source/Core/Common/Config/Layer.h':'66fed0ac7d91e93e1f6168e837d1c8efa837df8d106993060eb6bc8cde9b2470',
 'Source/Core/Common/Config/Config.cpp':'7cc14cab8048767c336c5219234b1527a434576cc9db09db13ccc716bfcacb05',
 'Source/Core/Core/Config/MainSettings.cpp':'31349025a4a3d4ed7a5b32e9aa6b4d6aa52c522b2bc85b0e909876f685c78c71',
 'Source/Core/Core/OracleEventAudit.h':'781b5f3e344877582d2e7b4506d9fc0f7c12e1ada672fe75fc9c7b9f432a036d',
 'Source/Core/Core/OracleEventAudit.cpp':'bf4665f53049fd4621aea6fa9502cf765902c357bf60062de6ba6ba14662b1c7',
 'Source/Core/Core/PowerPC/ConditionRegister.h':'72c2aa97de7a70ee6954dab3a5718602ca0ecfc592c7d0155eaf9f690b8edfb2',
 'Source/Core/Core/PowerPC/ConditionRegister.cpp':'9e2722d92305d4c1cabc1e947d386321453888900acffd9f2025993a41a18c4f',
 'Source/Core/Core/Core.cpp':'8bf9e90fc56b5b4d7a1e42410ea2e2291f44b5d86fb4e982b3aa68912edf5944',
 'Source/Core/Common/BlockingLoop.h':'410f0fc12388580664481862d8cbc975539b7bfb56ecdef935d07c255adb3043',
 'Source/Core/Common/Flag.h':'4f1cb93ddb864189ba5d10e230c1e43986414d7131bd19fb8a789f0072889b00',
 'Source/Core/VideoCommon/Fifo.cpp':'971d31caf503e18e9435202aa7f576a0ab281630a127e22a42063ab30795a15e',
 'Source/Core/VideoCommon/AsyncRequests.cpp':'c564c643eafd485c6bf50f66bbb95a5ba20f7a98a097b5d16a353f132bb19f67',
 'Source/Core/VideoCommon/AsyncRequests.h':'484d89b0605a2242cd76e9bbec8fa2d64646e3eb3cafbe78799cf40a7a8fd7fa',
 'Source/Core/VideoCommon/CommandProcessor.cpp':'c4a3d2fbe508bfe203721917a666c34516e6b72d2c2504096c3e422a4f125623',
 'Source/Core/Core/HW/SystemTimers.cpp':'578abca48d1ab5273d44bdf337431ff66817352d6862e6c28c79890969962bf7',
}
APP_SHA='8a1fe9487b029b2518a6633b1e0eb9ec0922f5f5eb5afdd7eba1267a85f096fe'
ENTRY_EXE_SHA='ef208d1a55fc635aa8ac1d47aff2b44e6b669df6d05c9ddeb67632d0cbdc2933'
NESTED_EXE_SHA='016b4a460f2b5cb7dd92bb4529e805641732af9cb16b7b73709b9a60c500366b'
ENTRY_OBSERVER_SHA='fcd6c7420a09c470c12b6646e537ec64c31e56b99e540c263389284600c2d3e9'
BASE_LINK_SHA='2d7b7158514692413d61caa4440ab1e96c40f26985a8e1b34e2efcfffdf71acd'
ENTRY_FIELDS=('seq stage instruction_pc word pc npc r0 r1 lr msr exceptions cr hid0 hid1 hid2 '
 'dcache ram_real ram_mask dbat0u dbat0l dbat1u dbat1l global_timer slice downcount stack handler report').split()
WATCH='015edc98:32,01200258:12,00000c00:4,01300000:4'
NESTED_FIELDS=ENTRY_FIELDS[:-3]+['r3','r4','r5','r29','r30','r31']+ENTRY_FIELDS[-3:]
NESTED_WATCH='015edc78:64,01200258:16,0120039c:32,00000c00:4,01300000:4'
DTK_EXE_SHA='9da7238efe5ac44ae1f01088c4b95995d0232edb3ceebdd453d6141a9ec3e6b2'
DTK_HELPER_SHA='220c45b5526c03ee27612105e006ba967df867960b3b4494bd8883f92c409c6d'
DTK_ARTIFACTS={
 'Mixer.cpp':'363c36cf09afc75ee411bbb4c6d7dbb556a44282de80b44bb4c3e1b79a792fec',
 'Mixer.obj':'cd4f86c247f772fb71326a54ef82ef6d1b829892a37c2ee07c14ac0bbb1ecfbf',
 'Mixer.cl.rsp':'e267faeff2d35ca6cc41293fa8ab39d641668e89f7091470b8fb25f65f42c985',
 'Mixer.cl.log':'9f07772ffb904ddc79912c5dc8011fb51cb86b52b57eb8dc5bab55e59cc8ee6e',
 'Layer.cpp':'0ad400f952b2eafcf79f8e2b2beec02ec9982576b01cc91d1a7dccd9866c26b2',
 'Layer.obj':'f6789fc690f631f2ed6756da6ba7d5761103e727f28e07540ab4f39298723f36',
 'Layer.cl.rsp':'567a6a39249c4330c2b00bd14774a14eb614c1f3e9cf0e510ee21dc61652a8de',
 'Layer.cl.log':'c7f3dbb006a5e4ba5a954cd833865dc67c913a51d26b50f580afaf7e6e20ffd9',
 'link.rsp':'dc03c80ffd10d2f5db2d1f150cbd22fa9cb3a9168d44ce30d0f8947c71930f5f',
 'link.log':'831cc41e7d5cd788e1f5e8a086f3a033f7726db1646312731d9924dd9c5aaaa5',
 'Dolphin.map':'83e71e5ad0f7fe505015f4b0251a48129ee00615a0e5c7e6fe976fd17e5b913c',
 'observer_helper.py':DTK_HELPER_SHA,
}
DTK_TRANSFORMS={'REPLACEMENTS':'f1ee824e942fd66bf2f1f37de0f598c497c7e33ab29633df630ae8b352f5ebb7',
 'LAYER_REPLACEMENTS':'8adf398ffbb427afb645932c8449eb70f64c0653030695fe3f564ba904d43dfa'}
EVENT_WINDOW='..timing-prefix-end@80373ac4'

def require(ok, message):
 if not ok: raise ValueError(message)

def exact(a,b):
 # Python considers False==0 and True==1. Evidence/schema parity must not.
 if type(a) is not type(b):return False
 if isinstance(a,dict):
  return len(a)==len(b) and all(type(k) is str for k in a) and set(a)==set(b) and all(exact(a[k],b[k]) for k in b)
 if isinstance(a,(list,tuple)):return len(a)==len(b) and all(exact(x,y) for x,y in zip(a,b))
 return a==b

def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as stream:
  for b in iter(lambda:stream.read(4*1024*1024),b''):h.update(b)
 return h.hexdigest()

def literal(path,name):
 tree=ast.parse(path.read_text(encoding='utf-8'))
 items=[n.value for n in tree.body if isinstance(n,ast.Assign) and
        any(isinstance(t,ast.Name) and t.id==name for t in n.targets)]
 require(len(items)==1,'ambiguous builder literal '+name)
 return ast.literal_eval(items[0])

def rsp_args(s):
 # Microsoft response quoting: backslashes are literal except immediately
 # before a quote. This also preserves literal quotes inside /D values.
 args=[];i=0
 while i<len(s):
  while i<len(s) and s[i].isspace():i+=1
  if i==len(s):break
  out=[];quoted=False
  while i<len(s) and (quoted or not s[i].isspace()):
   if s[i]=='\\':
    start=i
    while i<len(s) and s[i]=='\\':i+=1
    n=i-start
    if i<len(s) and s[i]=='"':
     out.extend('\\'*(n//2))
     if n%2:out.append('"')
     else:quoted=not quoted
     i+=1
    else:out.extend('\\'*n)
   elif s[i]=='"':quoted=not quoted;i+=1
   else:out.append(s[i]);i+=1
  require(not quoted,'unbalanced response quote');args.append(''.join(out))
 return args

def undo_entry(text,observer,original,nested=False):
 require(hashlib.sha256(observer.encode()).hexdigest()==ENTRY_OBSERVER_SHA,
         'unreviewed observer body')
 if nested:
  observer=observer.replace('if(original_pc<0x81200258||original_pc>0x81200264)return;','if(original_pc!=0x81200264&&(original_pc<0x8120039c||original_pc>0x812003b8))return;')
  observer=observer.replace('if(ram&&memory.GetRamSizeReal()>=0x015edcb8)','std::fprintf(log.file,"%08x\\t%08x\\t%08x\\t%08x\\t%08x\\t%08x\\t",s.gpr[3],s.gpr[4],s.gpr[5],s.gpr[29],s.gpr[30],s.gpr[31]);\n  if(ram&&memory.GetRamSizeReal()>=0x015edcb8)')
  observer=observer.replace('for(u32 i=0;i<32;++i)std::fprintf(log.file,"%02x",ram[0x015edc98+i]);','for(u32 i=0;i<64;++i)std::fprintf(log.file,"%02x",ram[0x015edc78+i]);')
  observer=observer.replace('if(original_pc==0x81200264)log.finished=true;','if(original_pc==0x812003b8)log.finished=true;')
 additions=[observer,'  const u32 shadow_entry_pc42=m_ppc_state.pc;\n',
  '\n  ShadowEntryObserve42(m_system,"enter",shadow_entry_pc42,m_prev_inst.hex);',
  '  ShadowEntryObserve42(m_system,"exit",shadow_entry_pc42,m_prev_inst.hex);\n',
  '  if(m_ppc_state.pc=='+('0x81200264' if nested else '0x81200258')+')ShadowEntryObserve42(m_system,"before-advance",m_ppc_state.pc,0);\n']
 for addition in additions:
  require(text.count(addition)==1,'missing/duplicate passive hook')
  text=text.replace(addition,'',1)
 require(text==original,'nonobserver Interpreter alteration')

def entry_provenance(j,root,app,nested=False):
 require(j['capture_complete'] and j['no_rsp_writes'] and not j['preentry_breakpoints'],
         'mutated/incomplete entry capture')
 require(j['disc_sha256']=='a8a526ef7006a2d2a8d41f536ef30b208ff197c185c05e9b2276e6e93cb41a8e',
         'original disc differs')
 require(sha(app)==APP_SHA,'apploader source differs')
 m=j['observer_manifest'];oracle=Path(m['oracle_directory'])
 require(m['instrumented_executable_sha256']==(NESTED_EXE_SHA if nested else ENTRY_EXE_SHA),'unreviewed receipt executable')
 require(m['fields']==(NESTED_FIELDS if nested else ENTRY_FIELDS),'unreviewed receipt fields')
 if nested:require(m['scope']=='nested','wrong observer scope')
 bindings={'Interpreter.cpp':'copied_interpreter_sha256','Interpreter.obj':'observer_object_sha256',
  'Dolphin.exe':'instrumented_executable_sha256','builder_snapshot.py':'builder_snapshot_sha256',
  'cl.rsp':'compile_recipe_sha256','link.rsp':'link_recipe_sha256'}
 for file,key in bindings.items():require(sha(oracle/file)==m[key],'artifact binding differs '+file)
 require(m['tool_sha256']==m['builder_snapshot_sha256'],'missing exact build helper')
 require(Path(m['original_source']).resolve()==(root/'Source/Core/Core/PowerPC/Interpreter/Interpreter.cpp').resolve(),
         'Interpreter source identity substituted')
 require(sha(Path(m['original_source']))==m['original_interpreter_sha256']==PINS['Source/Core/Core/PowerPC/Interpreter/Interpreter.cpp'],
         'original Interpreter binding differs')
 undo_entry((oracle/'Interpreter.cpp').read_text(encoding='utf-8'),
            literal(oracle/'builder_snapshot.py','OBSERVER'),Path(m['original_source']).read_text(encoding='utf-8'),nested)
 for key in ['gdb_object','compiler']:require(sha(Path(m[key]))==m[key+'_sha256'],key+' binding differs')
 for file,digest in m['original_source_pins'].items():require(sha(Path(file))==digest,'producer source differs')
 for file,digest in m['inherited_link_input_sha256'].items():require(sha(Path(file))==digest,'link input differs')
 # Independently account actual saved link recipe, including the one TU replacement.
 tokens=shlex.split((oracle/'link.rsp').read_text(encoding='utf-16'),posix=False)
 base=Path(m['gdb_object']).parent
 require(sha(base/'Dolphin.exe')==m['base_oracle_sha256']=='84be4d74d3a566b92e46c416d5e9c4ff87f313612fb2f4e529852cb74b0cfe78','base executable differs')
 require(sha(base/'link.rsp')==BASE_LINK_SHA,'base link recipe differs')
 oldtokens=[t.strip('"') for t in shlex.split((base/'link.rsp').read_text(encoding='utf-16'),posix=False)]
 transformed=[str(oracle/'Interpreter.obj')]+[t for t in oldtokens if not t.lower().startswith(('/out:','/pdb:','/implib:')) and Path(t).name.lower()!='interpreter.obj']+['/OUT:'+str(oracle/'Dolphin.exe'),'/PDB:'+str(oracle/'Dolphin.pdb'),'/IMPLIB:'+str(oracle/'Dolphin.lib')]
 require([t.strip('"') for t in tokens]==transformed,'link recipe is not exact single-TU replacement')
 actual=[Path(t.strip('"')).resolve() for t in tokens if not t.startswith('/') and t.strip('"').lower().endswith(('.obj','.lib','.res')) and Path(t.strip('"')).is_file()]
 replacement=(oracle/'Interpreter.obj').resolve()
 expected={Path(p).resolve() for p in m['inherited_link_input_sha256'] if Path(p).name.lower()!='interpreter.obj'}|{replacement,Path(m['gdb_object']).resolve()}
 require(len(actual)==len(set(actual)) and set(actual)==expected,'actual link inputs/override differ')
 require(m['clock_call_added'] is False and m['opcode_fetch_added'] is False and m['state_writer_added'] is False,
         'observer changed execution inputs')
 return oracle,len(actual)

def expected_entry(app):
 b=app.read_bytes();words=[b[0x20+0x258+4*i:0x20+0x25c+4*i].hex() for i in range(4)]
 require(words==['7c0802a6','9421fff8','9001000c','48000139'],'raw first-entry instructions differ')
 specs=[('before-advance',0,0x81200258,0,0,20000,0),
 ('enter',0,0x81200258,0x8120025c,20000,10888,10888),
 ('exit',0,0x8120025c,0x8120025c,20000,10888,10888),
 ('enter',1,0x8120025c,0x81200260,20001,10887,10887),
 ('exit',1,0x81200260,0x81200260,20001,10887,10887),
 ('enter',2,0x81200260,0x81200264,20002,10886,10886),
 ('exit',2,0x81200264,0x81200264,20002,10886,10886),
 ('enter',3,0x81200264,0x81200268,20003,10885,10885)]
 result=[]
 for i,(stage,n,pc,npc,g,length,down) in enumerate(specs):
  stack=bytearray(32)
  if i>=4:stack[8:12]=(0x815edca8).to_bytes(4,'big')
  vals=[str(i+1),stage,f'{0x81200258+4*n:08x}', '00000000' if i==0 else words[n],
   f'{pc:08x}',f'{npc:08x}','00000000','815edca8' if i<4 else '815edca0',
   '00000000','00002032','00000000','00000000','0011c464','80000000','e0000000',
   '00000000','01800000','01ffffff','80001fff','00000002','c0001fff','0000002a',
   f'{g:016x}',f'{length:016x}',f'{down:08x}',stack.hex(),'4c000064','00000000']
  result.append(dict(zip(ENTRY_FIELDS,vals,strict=True)))
 return result

def compare_entry(rows,app):
 require(rows==expected_entry(app),'entry register/store/phase field differs')

def read_writes(path,watch=WATCH):
 lines=path.read_text().splitlines()
 require(lines[0]=='0\twatch-config\textra='+watch+';only=1','physical watch changed')
 rows=[line.split('\t') for line in lines[1:]]
 require(all(len(r)==18 and int(r[0])==i+1 for i,r in enumerate(rows)),'physical receipt truncated/malformed')
 return rows

def compare_writes(rows):
 early=[r for r in rows if int(r[9])<=20003 and int(r[7])>0]
 projected=[(r[1],r[2],int(r[3]),r[4],r[5],r[6],int(r[7]),r[8],int(r[9]),r[12],r[16]) for r in early]
 expected=[]
 for addr,size in [(0x15edc98,32),(0x1200258,12),(0xc00,4),(0x1300000,4)]:
  expected.append(('memory-clear','00000000',33554432,'00000000','00000000',f'{addr:08x}',size,'00000000',0,'0','00'*size))
 expected += [('cpu-translated-write','00000c00',4,'80000c00','00000c00','00000c00',4,'00000000',0,'0','4c000064'),
  ('pointer-lease','01200000',122424,'01200000','01200000','01200258',12,'00000000',0,'0','-'),
  ('memory-copy-write','01200000',122424,'01200000','01200000','01200258',12,'00000000',0,'0','7c0802a69421fff89001000c'),
  ('pointer-lease','01200240',32,'01200240','01200240','01200258',8,'81200258',20000,'0','-'),
  ('cpu-translated-write','015edca0',4,'815edca0','015edca0','015edca0',4,'8120025c',20001,'0','815edca8'),
  ('pointer-lease','01200260',32,'01200260','01200260','01200260',4,'81200260',20002,'0','-'),
  ('cpu-translated-write','015edcac',4,'815edcac','015edcac','015edcac',4,'81200260',20002,'0','00000000')]
 require(projected==expected,'unexplained early physical write/read/lease')
 for r in early:
  if r[1]=='pointer-lease':
   require('MemoryManager::CopyToEmu(' in r[15] or 'MemoryManager::CopyFromEmu(' in r[15],
           'unknown pointer owner')
 return len(early)

def audit_entry(path,root,app,falsify):
 j=json.loads(path.read_text());oracle,n=entry_provenance(j,root,app)
 paths={k:path.parent/v for k,v in j['logs'].items()}
 for k,p in paths.items():require(sha(p)==j['log_sha256'][k],'receipt digest differs '+k)
 rows=[dict(zip(ENTRY_FIELDS,line.split('\t'),strict=True)) for line in paths['states'].read_text().splitlines()]
 compare_entry(rows,app);writes=read_writes(paths['writes']);early=compare_writes(writes)
 require(j['original_dol_entry']['cycles'].lower()=='000000000028d70d','later DOL cycle parity differs')
 rejects=0
 if falsify:
  for i,row in enumerate(rows):
   for field,value in row.items():
    changed=copy.deepcopy(rows);changed[i][field]=value+'x' if field=='stage' else value[:-1]+('1' if value[-1]!='1' else '0')
    try:compare_entry(changed,app)
    except ValueError:rejects+=1
    else:raise AssertionError('entry mutation admitted')
  for i,r in enumerate(writes):
   if int(r[9])>20003 or int(r[7])==0:continue
   changed=copy.deepcopy(writes);changed[i][16]='1'+r[16][1:]
   try:compare_writes(changed)
   except ValueError:rejects+=1
   else:raise AssertionError('early physical mutation admitted')
  for index in [16,18]:
   changed=writes[:index]+writes[index+1:]
   try:compare_writes(changed)
   except ValueError:rejects+=1
   else:raise AssertionError('zero-value store omission admitted')
 return dict(capture_sha256=sha(path),oracle_sha256=sha(oracle/'Dolphin.exe'),state_rows=len(rows),
  state_fields=len(rows)*len(ENTRY_FIELDS),actual_link_inputs=n,early_physical_records=early,
  mutation_rejections=rejects,observer_inverse_exact=True,
  complete_foreign_writer_exclusion=False,native_production_admission=False)

def expected_nested(app):
 data=app.read_bytes();pcs=[0x81200264]+list(range(0x8120039c,0x812003b8,4));words=[int.from_bytes(data[0x20+pc-0x81200000:0x24+pc-0x81200000],'big') for pc in pcs]
 require(words==[0x48000139,0x7c0802a6,0x90010004,0x9421ffd8,0xbfa1001c,0x90610008,0x9081000c,0x90a10010],'nested raw unit differs')
 boundary=int.from_bytes(data[0x20+0x3b8:0x24+0x3b8],'big');require(boundary==0x3c608120,'next unconsumed instruction differs')
 state=expected_entry(app)[-1].copy();state['npc']='81200264';state['global_timer']=f'{20002:016x}';state['slice']=f'{1:016x}';state['downcount']='00000000'
 state.update(r3='80003100',r4='80003104',r5='80003108',r29='00000000',r30='00000000',r31='00000000')
 stack=bytearray(64);stack[40:44]=(0x815edca8).to_bytes(4,'big');result=[]
 def save(stage,pc,word):
  row=state.copy();row.update(seq=str(len(result)+1),stage=stage,instruction_pc=f'{pc:08x}',word=f'{word:08x}',stack=stack.hex());result.append(row)
 save('before-advance',pcs[0],0)
 for index,(pc,word) in enumerate(zip(pcs,words)):
  state.update(pc=f'{pc:08x}',npc=f'{pc+4:08x}',global_timer=f'{20003+index:016x}',slice=f'{10885-index:016x}',downcount=f'{10885-index:08x}');save('enter',pc,word)
  if index==0:state.update(lr=f'{pc+4:08x}',npc='8120039c')
  elif index==1:state['r0']=state['lr']
  elif index==2:stack[44:48]=int(state['r0'],16).to_bytes(4,'big')
  elif index==3:
   old_sp=int(state['r1'],16);new_sp=(old_sp-40)&0xffffffff;stack[new_sp-0x815edc78:new_sp-0x815edc78+4]=old_sp.to_bytes(4,'big');state['r1']=f'{new_sp:08x}'
  elif index==4:
   for n in range(29,32):stack[28+4*(n-29):32+4*(n-29)]=int(state['r'+str(n)],16).to_bytes(4,'big')
  else:stack[8+4*(index-5):12+4*(index-5)]=int(state['r'+str(index-2)],16).to_bytes(4,'big')
  state['pc']=state['npc'];save('exit',pc,word)
 state.update(pc='812003b8',npc='812003bc',global_timer=f'{20011:016x}',slice=f'{10877:016x}',downcount=f'{10877:08x}');save('enter',0x812003b8,boundary)
 return result

def compare_nested(rows,app):require(rows==expected_nested(app),'nested semantic state/store/phase differs')

def nested_spills(rows):
 selected=[r for r in rows if r[1]=='cpu-translated-write' and int(r[8],16) in range(0x8120039c,0x812003b8,4)]
 actual=[(int(r[8],16),int(r[4],16),int(r[2],16),int(r[3]),int(r[9]),r[12],r[16]) for r in selected]
 expected=[(0x812003a0,0x815edca4,0x15edca4,4,20005,'0','81200268'),(0x812003a4,0x815edc78,0x15edc78,4,20006,'0','815edca0')]
 expected += [(0x812003a8,0x815edc94+4*i,0x15edc94+4*i,4,20007,'0','00000000') for i in range(3)]
 expected += [(0x812003ac+4*i,0x815edc80+4*i,0x15edc80+4*i,4,20008+i,'0',f'{0x80003100+4*i:08x}') for i in range(3)]
 require(actual==expected,'nested spill write receipt differs')
 return len(actual)

def audit_nested(path,root,app,falsify):
 j=json.loads(path.read_text());oracle,inputs=entry_provenance(j,root,app,True);paths={k:path.parent/v for k,v in j['logs'].items()}
 for key,p in paths.items():require(sha(p)==j['log_sha256'][key],'nested stream binding differs')
 rows=[dict(zip(NESTED_FIELDS,line.split('\t'),strict=True)) for line in paths['states'].read_text().splitlines()];compare_nested(rows,app)
 writes=read_writes(paths['writes'],NESTED_WATCH);spills=nested_spills(writes);rejects=0
 if falsify:
  for i,row in enumerate(rows):
   for field,value in row.items():
    changed=copy.deepcopy(rows);changed[i][field]=value+'x' if field=='stage' else value[:-1]+('1' if value[-1]!='1' else '0')
    try:compare_nested(changed,app)
    except ValueError:rejects+=1
    else:raise AssertionError('nested field mutation admitted')
  for i,r in enumerate(writes):
   if not (r[1]=='cpu-translated-write' and int(r[8],16) in range(0x8120039c,0x812003b8,4)):continue
   changed=copy.deepcopy(writes);changed[i][16]='1'+r[16][1:]
   try:nested_spills(changed)
   except ValueError:rejects+=1
   else:raise AssertionError('nested spill mutation admitted')
   try:nested_spills(writes[:i]+writes[i+1:])
   except ValueError:rejects+=1
   else:raise AssertionError('nested zero-store omission admitted')
 return dict(capture_sha256=sha(path),oracle_sha256=sha(oracle/'Dolphin.exe'),state_rows=len(rows),state_fields=len(rows)*len(NESTED_FIELDS),actual_link_inputs=inputs,spill_records=spills,mutation_rejections=rejects,observer_inverse_exact=True,next_pc='812003b8',native_production_admission=False)

def literal_segment(path,name):
 s=path.read_text(encoding='utf-8');tree=ast.parse(s)
 nodes=[n.value for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id==name for t in n.targets)]
 require(len(nodes)==1,'ambiguous transform literal')
 return hashlib.sha256(ast.get_source_segment(s,nodes[0]).encode()).hexdigest()

def inverse_transform(text,original,replacements):
 for before,after in reversed(replacements):
  require(text.count(after)==1,'missing or duplicate passive replacement')
  text=text.replace(after,before,1)
 require(text==original,'nonobserver TU alteration')

def event_rows(path,startup=True):
 previous=0;last=None
 with path.open(encoding='utf-8',newline='') as stream:
  for index,line in enumerate(stream):
   require(line.endswith('\n'),'truncated event line')
   f=line.rstrip('\r\n').split('\t')
   require(len(f)==16 and f[0].isdigit() and f[1].isdigit() and f[13].isdigit(),'malformed event schema')
   require(all(re.fullmatch('[0-9a-fA-F]{16}',x) for x in f[4:12]),'malformed event values')
   row=dict(seq=int(f[0]),thread=int(f[1]),kind=f[2],name=f[3],v=tuple(int(x,16) for x in f[4:12]),source=f[12],line=int(f[13]),function=f[14])
   if index==0:
    window=EVENT_WINDOW if startup else 'init'+EVENT_WINDOW
    require(row['kind']=='event-filter' and row['seq']==0 and row['name']=='kinds=;window='+window+';limit=3000000' and row['v']==(3000000,0,0,int(not startup),1,0,0,0),'event header/window differs')
   else:
    if startup:require(row['seq']==previous+1,'startup event sequence gap')
    else:require(row['seq']>previous,'reference event sequence disorder')
    previous=row['seq'];last=row
    yield row
 require(last is not None and last['kind']=='timing-prefix-end' and last['v'][0]==0x80373ac4,'missing explicit endpoint')

def semantic_projection(path,startup):
 # Queue/device rows have stable semantic boundaries. Passive host records can
 # split lossless step/retire runs, so work is checked separately below.
 kinds={'init','init-ready','config','register','adjust','adjust-item','enqueue','cancel',
  'cancel-item','cancel-all','clear','ts-publish','ts-commit','ts-move',
  'dispatch','callback-return','boundary','timing-prefix-end'}
 started=False
 for row in event_rows(path,startup):
  if row['kind']=='init':started=True
  if started and (row['kind'] in kinds or row['kind'].startswith('vt-')):
   yield row['kind'],row['name'],row['v']

def normalized_work(path,startup):
 started=False;units=retired=cost=0
 for r in event_rows(path,startup):
  if r['kind']=='init':started=True
  if not started:continue
  v=r['v']
  if r['kind']=='boot-steps':
   require(v[1]>0 and 1<=v[2]<=20000 and 1<=v[3]<=20000 and v[7]==1,'invalid compressed SingleStep recurrence')
   units+=v[1]
  elif r['kind']=='step-reset':
   require(v[1]==1 and v[2]==0,'invalid literal SingleStep reset');units+=1
  elif r['kind']=='retire':
   require(v[3]>0 and v[2]==(v[0]-v[1])&((1<<64)-1),'invalid retirement recurrence');cost+=v[2];retired+=v[3]
 return dict(apploader_unit_steps=units,retired_instructions=retired,retired_cost_units=cost)

def replay_dtk(rows,control):
 alive={};seen=set();first_init=None;active=None;consumers=[];writes=[];calls=[];config=[];completed=0
 for r in rows:
  k,v,n=r['kind'],r['v'],r['name']
  if k=='init':require(first_init is None,'duplicate initialization');first_init=r
  if k=='dispatch' and n=='FinishExecutingCommand' and v[2]==0x300000001:
   require(active is None and first_init is not None,'unpaired DTK dispatch')
   active=dict(row=r,consumers=0)
  if k=='callback-return' and n=='FinishExecutingCommand' and v[2]==0x300000001:
   require(active is not None and r['thread']==active['row']['thread'] and v[:3]==active['row']['v'][:3] and active['consumers']==1,'unpaired DTK callback return/consumer')
   require(all(s['push'] is None for s in alive.values()),'unfinished callback sample consumer')
   active=None;completed+=1
  if k=='dtk-config':
   require(n in {'construct','construct-loader','load-save-end','set','delete','delete-all','add-live','resolve-result'},'unknown config ingress or mutable DSP lease')
   require(v[1] in (0,1,2) and not any(v[4:]),'unknown config value/padding')
   if not control:require(v[1]!=2,'true DumpAudio ingress on baseline')
   config.append(r)
  if not k.startswith('dtk-owner-'):continue
  key=v[0];require(key!=0,'null Mixer identity')
  if k=='dtk-owner-ctor':
   require(key not in seen and v[1]==0 and v[2]>0 and not any(v[3:]),'unexplained/reused Mixer constructor');seen.add(key)
   alive[key]=dict(flag=0,ctor=r,pending=None,push=None,entries=0)
   continue
  require(key in alive,'owner record lacks live constructor');s=alive[key]
  if k=='dtk-owner-dtor':
   require(s['pending'] is None and s['push'] is None and not any(v[1:]),'incomplete Mixer destruction')
   del alive[key]
  elif k in {'dtk-owner-start','dtk-owner-stop'}:
   require(s['pending'] is None and s['push'] is None and v[1]==s['flag'] and not any(v[2:]),'unexplained/racing logging entry')
   s['pending']=(k,v[1],False,r['thread']);s['entries']+=1;calls.append(r)
  elif k=='dtk-owner-write':
   expected=('dtk-owner-start',0,1) if n=='start' else ('dtk-owner-stop',1,0)
   require(n in {'start','stop'} and s['pending']==(expected[0],expected[1],False,r['thread']) and v[1]==expected[2] and not any(v[2:]),'unexplained effective flag write')
   s['flag']=v[1];s['pending']=(expected[0],expected[1],True,r['thread']);writes.append(r)
  elif k in {'dtk-owner-start-exit','dtk-owner-stop-exit'}:
   spec={'started':('dtk-owner-start',0,True,1),'failed':('dtk-owner-start',0,False,0),'already-started':('dtk-owner-start',1,False,1),'stopped':('dtk-owner-stop',1,True,0),'already-stopped':('dtk-owner-stop',0,False,0)}.get(n)
   require(spec is not None and k==spec[0]+'-exit' and s['pending']==(*spec[:3],r['thread']) and v[1]==spec[3]==s['flag'] and not any(v[2:]),'unexplained logging exit')
   s['pending']=None
  elif k=='dtk-owner-push':
   require(active is not None and r['thread']==active['row']['thread'] and s['pending'] is None and s['push'] is None and v[2] in (0,1) and not any(v[3:]),'unpaired/racing sample push')
   s['push']=r
  elif k=='dtk-owner-read':
   require(active is not None and s['pending'] is None and s['push'] is not None and r['thread']==s['push']['thread']==active['row']['thread'] and v[1]==s['flag'] and v[2]==s['push']['v'][1] and not any(v[3:]),'unpaired/unexplained actual flag consumer')
   if not consumers:
    require(s['ctor']['seq']<first_init['seq']<active['row']['seq']<r['seq'] and v[2]==0 and s['flag']==int(control),'first zero-sample consumer ancestry differs')
    require(s['entries']==int(control) and len(writes)==int(control),'unexpected prior effective logging ingress')
    if control:require(any(x['name']=='set' and x['v'][1]==2 and x['seq']<r['seq'] for x in config),'missing initial DumpAudio true writer')
   active['consumers']+=1;consumers.append(r);s['push']=None
  else:raise ValueError('unknown owner kind')
 require(active is None and consumers and completed==len(consumers) and all(s['pending'] is None and s['push'] is None for s in alive.values()),'incomplete lifecycle/DTK callback ancestry')
 return dict(consumers=len(consumers),completed_dtk_callbacks=completed,effective_writes=len(writes),logging_calls=len(calls),first_flag=consumers[0]['v'][1],first_samples=consumers[0]['v'][2],sample_counts=[r['v'][2] for r in consumers],config_records=len(config))

def audit_dtk(path,reference,helper,root,falsify):
 j=json.loads(path.read_text());b=json.loads(reference.read_text());o=j['instrumentation_manifest']['dtk_owner'];oracle=Path(o['copied_mixer_path']).parent
 require(j['capture_complete'] and j['controlled_midchain_writes']==[] and j['sole_internal_stop']=='80373ac4','controlled/incomplete DTK receipt')
 require(sha(oracle/'Dolphin.exe')==j['dolphin_sha256']==DTK_EXE_SHA,'unreviewed DTK executable')
 require(sha(helper)==sha(oracle/'observer_helper.py')==o['helper_sha256']==j['capture_tool_sha256']==DTK_HELPER_SHA,'helper identity drift')
 require(o['build_artifact_sha256']==DTK_ARTIFACTS,'unreviewed build artifact map')
 for file,digest in DTK_ARTIFACTS.items():require(sha(oracle/file)==digest,'DTK build artifact drift '+file)
 for tu in ['Mixer','Layer']:
  recipe=rsp_args((oracle/(tu+'.cl.rsp')).read_text(encoding='utf-16'))
  require(recipe==o['compile_recipes'][tu] and recipe[-1]==str(oracle/(tu+'.cpp')) and '/Fo'+str(oracle/(tu+'.obj')) in recipe,'exact copied TU recipe differs')
 for name,expected in DTK_TRANSFORMS.items():require(literal_segment(helper,name)==expected,'unreviewed passive transform literal')
 for name,file,relative in [('REPLACEMENTS','Mixer.cpp','Source/Core/AudioCommon/Mixer.cpp'),('LAYER_REPLACEMENTS','Layer.cpp','Source/Core/Common/Config/Layer.cpp')]:
  inverse_transform((oracle/file).read_text(encoding='utf-8'),(root/relative).read_text(encoding='utf-8'),literal(helper,name))
 for file,key in {'Mixer.cpp':'copied_mixer_sha256','Mixer.obj':'copied_mixer_object_sha256','Layer.cpp':'copied_layer_sha256','Layer.obj':'copied_layer_object_sha256','Dolphin.map':'link_map_sha256'}.items():require(sha(oracle/file)==o[key],'DTK artifact differs '+file)
 toolbin=Path(o['msvc'])/'bin/Hostx64/x64'
 for name in ['compiler','linker']:require(sha(toolbin/('cl.exe' if name=='compiler' else 'link.exe'))==o[name+'_sha256'],'tool binary differs')
 linkmap=(oracle/'Dolphin.map').read_text()
 for symbol,file in [('PushStreamingSamples@Mixer','Mixer.obj'),('StartLogDTKAudio@Mixer','Mixer.obj'),('StopLogDTKAudio@Mixer','Mixer.obj'),('OracleViAudit@Config','Layer.obj'),('GetSection@Layer','Layer.obj'),('??0Layer@Config','Layer.obj'),('Load@Layer','Layer.obj'),('Save@Layer','Layer.obj')]:
  lines=[s for s in linkmap.splitlines() if symbol in s and ' f ' in s]
  require(lines and all(s.rstrip().endswith(file) for s in lines),'linked consumer/writer TU differs '+symbol)
 require('LNK4006' not in (oracle/'link.log').read_text(),'duplicate TU definitions')
 linktokens=[s.strip('"') for s in shlex.split((oracle/'link.rsp').read_text(encoding='utf-16'),posix=False)]
 base=next(Path(t).parent for t in linktokens if Path(t).name=='GDBStub.obj')
 require(sha(base/'link.rsp')==o['base_link_response_sha256']==BASE_LINK_SHA,'base link differs')
 old=[s.strip('"') for s in shlex.split((base/'link.rsp').read_text(encoding='utf-16'),posix=False)]
 expected=[str(oracle/'Mixer.obj'),str(oracle/'Layer.obj')]+[s for s in old if not s.lower().startswith(('/out:','/pdb:','/implib:'))]+['/OUT:'+str(oracle/'Dolphin.exe'),'/PDB:'+str(oracle/'Dolphin.pdb'),'/IMPLIB:'+str(oracle/'Dolphin.lib'),'/MAP:'+str(oracle/'Dolphin.map')]
 require(linktokens==expected,'DTK link recipe changed beyond two copied TUs')
 require(sha(base/'manifest.json')==o['base_manifest_sha256'],'base manifest differs')
 base_m=json.loads((base/'manifest.json').read_text())
 for file,digest in base_m['link_input_sha256'].items():require(sha(Path(file))==digest,'inherited DTK link input differs')
 for file,key in [('GDBStub.obj','gdb_object_sha256'),('Interpreter.obj','interpreter_object_sha256')]:require(sha(base/file)==o[key],'inherited object differs')
 for key,field in [('events','event_log'),('instructions','instruction_trace'),('mmio','mmio_log')]:require(sha(path.with_name(j[field]))==j[field+'_sha256'],'DTK stream binding differs '+key)
 require(j['instruction_trace_sha256']==b['instruction_trace_sha256'],'DTK observer changed exact executed instruction/work stream')
 event=path.with_name(j['event_log']);refevent=reference.with_name(b['event_log'])
 count=0
 sentinel=object()
 import itertools
 for observed,original in itertools.zip_longest(semantic_projection(event,True),semantic_projection(refevent,False),fillvalue=sentinel):
  require(observed==original,'original scheduler/device/CPU projection differs');count+=1
 work=normalized_work(event,True)
 require(work==normalized_work(refevent,False)==dict(apploader_unit_steps=2656493,retired_instructions=15557,retired_cost_units=154286),'normalized source work differs')
 selected=[];rtc=[];total=0
 for row in event_rows(event):
  total+=1
  if row['kind'].startswith('dtk-') or row['kind'] in {'init','dispatch','callback-return'}:selected.append(row)
  if row['kind'].startswith('rtc-'):rtc.append(row)
  if row['kind'].startswith('dtk-owner-') or row['kind']=='dtk-config':
   tu=oracle/('Mixer.cpp' if row['kind'].startswith('dtk-owner-') else 'Layer.cpp');lines=tu.read_text().splitlines()
   require(Path(row['source']).resolve()==tu.resolve() and 1<=row['line']<=len(lines) and '"'+row['kind']+'"' in lines[row['line']-1],'receipt source site differs')
 local=[r for r in rtc if r['kind']=='rtc-local'];result=[r for r in rtc if r['kind']=='rtc-result']
 require(len(local)==len(result)==2 and all(a['v'][:3]==(c['v'][0],0,c['v'][1]) and c['v'][1]==0x386d4380 and c['v'][2]==(c['v'][0]-c['v'][1])&0xffffffff for a,c in zip(local,result)),'live RTC input equation differs')
 control=j['capture_adaptation']['initial_dump_audio_control'];receipt=replay_dtk(selected,control);rejects=0
 if falsify:
  for i,r in enumerate(selected):
   if not (r['kind'].startswith('dtk-owner-') or r['kind']=='dtk-config'):continue
   fields=range(8) if r['kind'].startswith('dtk-owner-') else [1,4,5,6,7]
   for field in fields:
    changed=copy.deepcopy(selected);v=list(r['v'])
    if r['kind']=='dtk-config' and field==1:v[field]=3 if control else 2
    elif r['kind']=='dtk-owner-push' and field==2:v[field]=2 # invalid boolean, not a different legitimate rate input
    elif r['kind']=='dtk-owner-ctor' and field==2:v[field]=0 # missing admitted positive backend rate
    else:v[field]^=1
    changed[i]['v']=tuple(v)
    try:replay_dtk(changed,control)
    except ValueError:rejects+=1
    else:raise AssertionError('DTK field mutation admitted '+r['kind']+'/'+str(field))
   if r['kind'].startswith('dtk-owner-'):
    changed=selected[:i]+selected[i+1:]
    try:replay_dtk(changed,control)
    except ValueError:rejects+=1
    else:raise AssertionError('DTK lifecycle omission admitted')
  for i,r in enumerate(selected):
   if r['kind'] in {'dtk-owner-push','dtk-owner-read','dtk-owner-write','dtk-owner-start-exit'}:
    changed=copy.deepcopy(selected);changed[i]['thread']+=1
    try:replay_dtk(changed,control)
    except ValueError:rejects+=1
    else:raise AssertionError('DTK thread mutation admitted')
 return dict(capture_sha256=sha(path),oracle_sha256=sha(oracle/'Dolphin.exe'),event_records=total,semantic_projection_records=count,normalized_work=work,**receipt,mutation_rejections=rejects,source_inverse_exact=True,
  mixer_compile_recipe_separately_archived=True,historical_original_flag_parity=False,arbitrary_host_race_equivalence=False,true_branch_wavefile_owned=False,native_production_admission=False)

def expected_owned_initial(enabled):
 # Fresh source state and finite callback arithmetic, independently derived;
 # no reference private flag/clock/queue value is supplied to native execution.
 times=[0,0,15444,0,121392,8108100]
 names=['FinishExecutingCommand','GPUSleeper','VICallback','DSPCallback','AudioDMACallback','PatchEngine']
 user=0x300000001
 def event(index):return dict(name=names[index],deadline=times[index],fifo=index,userdata=f'{user if index==0 else 0:016x}')
 events=[event(i) for i in range(6)];events.sort(key=lambda x:(x['deadline'],x['fifo']))
 journal=[]
 def record(kind,name='-',values=()):journal.append(dict(kind=kind,name=name,v=[f'{n:016x}' for n in (*values,*([0]*(8-len(values))))]))
 record('dtk-owned-mixer-construct',values=(0,48000));record('init',values=(0,20000,0,1))
 record('init-ready',values=(0,20000,0,1,0,0,0x3f800000))
 for i in range(6):record('enqueue',names[i],(times[i],i,user if i==0 else 0,times[i]))
 record('dtk-owned-dump-enabled' if enabled else 'dtk-owned-dump-default',values=(int(enabled),))
 if enabled:stop='AudioCommon.cpp:78 unknown-audio-dump-started';active=None
 else:
  record('advance-enter',values=(0,20000,0,1,6,0,0x3f800000))
  record('advance-clock',values=(20000,20000,0,1,6,0,0x3f800000,20000))
  record('dispatch',names[0],(0,0,user,20000));record('dtk-transfer-zero');record('dtk-zero-sample-request');record('dtk-owned-log-read');record('dtk-no-wave-write');record('dtk-pending-blocks',values=(6,))
  period=(486000000*6*28*2248)//108000000
  record('enqueue',names[0],(period,6,user,period-20000));record('callback-return',names[0],(0,0,user))
  record('dispatch',names[1],(0,1,0,20000));record('gpu-allow-sleep-request',values=(1,))
  active=event(1);events=[e for e in events if e['fifo'] not in (0,1)]+[dict(name=names[0],deadline=period,fifo=6,userdata=f'{user:016x}')];events.sort(key=lambda x:(x['deadline'],x['fifo']))
  stop='BlockingLoop.h:233 undelivered-allow-sleep'
 record('stop',stop)
 return dict(schema='initial-boot-events-42-v1',scope='research-only',control='owned-initial-dump-enabled' if enabled else 'owned-fresh-dtk-source',stop=stop,
  scheduler=dict(global_timer=0), # replace Python-reserved JSON key below
  queue=events,active_callback=active,pi=dict(cause=0x10100,mask=0,exceptions=0),
  dvd=dict(dimar=0,dilength=0,stream=0,pending_blocks=0 if enabled else 6,decoded_blocks=0,push_calls=0 if enabled else 1,streaming_frames=0),
  ai=dict(playing=0,ais_divisor=2248,aid_divisor=3372),dsp=dict(hle_rom=1,dma_enabled=0,mail_halted=1,rom_mail=0x8071feed,slice=0,update_calls=0,last_update_cycles=0),
  vi=dict(half_line=0,next_si_poll=15,last_line_start=0,odd_first=520,odd_last=519,even_first=1045,even_last=1044),
  movie={k:None for k in ['frame','lag','polled','total_frames','total_lag']},effects=dict(gpu_sleep_delivered=0,gpu_allow_sleep_calls=0,new_field_calls=0,achievement_return_calls=0,guest_ram_write_bytes=0),
  dtk_logging_owner=dict(constructor_owned=1,configuration_owned=int(not enabled),flag_read=int(not enabled),enabled=0,backend_sample_rate=48000),journal=journal)

def owned_initial_expected(enabled):
 r=expected_owned_initial(enabled);r['scheduler']={'global':0 if enabled else 20000,'slice':20000,'downcount':0,'sane':1,'next_fifo':6 if enabled else 7,'inverse_bits':0x3f800000};return r

def compare_native_initial(actual,enabled):
 require(exact(actual,owned_initial_expected(enabled)),'compiled finite owned DTK typed state/journal differs')

def json_leaves(value,prefix=()):
 if isinstance(value,dict):
  for key,v in value.items():yield from json_leaves(v,prefix+(key,))
 elif isinstance(value,list):
  for i,v in enumerate(value):yield from json_leaves(v,prefix+(i,))
 else:yield prefix,value

def audit_native_initial(program,digest,falsify):
 require(digest is not None and sha(program)==digest,'unbound compiled initial owner executable')
 reports=[]
 for enabled in [False,True]:
  mode='--dump-owned-dtk-enabled' if enabled else '--dump-owned-dtk'
  output=subprocess.run([str(program.resolve()),mode],check=True,capture_output=True,text=True).stdout
  actual=json.loads(output);compare_native_initial(actual,enabled)
  declines=0
  if falsify:
   for path,value in json_leaves(actual):
    for replacement in [str(value)+'x' if type(value) is not int else value+1, bool(value)]:
     changed=copy.deepcopy(actual);parent=changed
     for key in path[:-1]:parent=parent[key]
     parent[path[-1]]=replacement
     try:compare_native_initial(changed,enabled)
     except ValueError:declines+=1
     else:raise AssertionError('native owner value/type mutation admitted')
   # Closed schema also rejects extra/missing keys and array arity changes.
   for altered in [dict(actual,unexplained=0),{k:v for k,v in actual.items() if k!='stop'},dict(actual,journal=actual['journal'][:-1])]:
    try:compare_native_initial(altered,enabled)
    except ValueError:declines+=1
    else:raise AssertionError('native owner structural mutation admitted')
  reports.append(dict(mode=mode,journal_records=len(actual['journal']),state_and_journal_fields=sum(1 for _ in json_leaves(actual)),mutation_rejections=declines,stop=actual['stop'],producer_scope='explicit source capsule with excluded ingress; no live host/UI provider'))
 return dict(executable_sha256=digest,modes=reports,native_production_admission=False)

def expected_native_frames(app):
 entry=expected_entry(app);nested=expected_nested(app)
 def snapshot(row,nested_stack=False):
  fields=('pc npc r0 r1 lr msr exceptions cr hid0 hid1 hid2 dcache ram_real ram_mask dbat0u dbat0l dbat1u dbat1l').split()
  result={key:int(row[key],16) for key in fields}
  result.update(xer=0,ctr=0,fpscr=0)
  gpr=[0]*32
  gpr[2]=0x814b5b20;gpr[13]=0x814b4fc0
  gpr[3:6]=[0x80003100,0x80003104,0x80003108]
  for n in [0,1,3,4,5,29,30,31]:
   if 'r'+str(n) in row:gpr[n]=int(row['r'+str(n)],16)
  result.update({key:gpr[int(key[1:])] for key in ['r3','r4','r5','r29','r30','r31']})
  result.update(gpr=gpr,stack_base=0x815edc78,physical_base=0x015edc78,
   stack=row['stack'] if nested_stack else '00'*32+row['stack'],known='1'*64)
  return result
 states=[]
 for row in entry[1:7]:
  states.append(dict(instruction_pc=int(row['instruction_pc'],16),word=int(row['word'],16),
   stage='inner-exit' if row['stage']=='exit' else row['stage'],state=snapshot(row)))
 for row in nested[1:17]:
  states.append(dict(instruction_pc=int(row['instruction_pc'],16),word=int(row['word'],16),
   stage='inner-exit' if row['stage']=='exit' else row['stage'],state=snapshot(row,True)))
 stores=[]
 for pc,address,value in [(0x8120025c,0x815edca0,0x815edca8),(0x81200260,0x815edcac,0),
  (0x812003a0,0x815edca4,0x81200268),(0x812003a4,0x815edc78,0x815edca0),
  (0x812003a8,0x815edc94,0),(0x812003a8,0x815edc98,0),(0x812003a8,0x815edc9c,0),
  (0x812003ac,0x815edc80,0x80003100),(0x812003b0,0x815edc84,0x80003104),(0x812003b4,0x815edc88,0x80003108)]:
  stores.append(dict(instruction_pc=pc,logical_address=address,physical_address=address&0x01ffffff,value=value))
 before=snapshot(nested[0],True);boundary=snapshot(nested[16],True)
 return dict(schema='apploader-entry-frames-research-42',profile='conditional-source-only',
  native_admitted=False,falsification=False,completed_source_instructions=11,handler=0x4c000064,
  report=0,source_entry=snapshot(entry[0]),before_nested=before,boundary=boundary,states=states,stores=stores)

def compare_native_frames(actual,app):
 require(exact(actual,expected_native_frames(app)),'compiled finite frame typed state/store ledger differs')

def audit_native_frames(program,digest,output,dol,app,falsify):
 require(digest is not None and sha(program)==digest,'unbound compiled frame executable')
 require(dol is not None and output is not None,'frame audit requires explicit DOL and output paths')
 if not output.exists():
  # A new independent artifact is emitted by the pinned C++ executable itself.
  completed=subprocess.run([str(program.resolve()),str(dol.resolve()),'--output',str(output.resolve())],check=True,capture_output=True,text=True)
 else:completed=subprocess.run([str(program.resolve()),str(dol.resolve())],check=True,capture_output=True,text=True)
 require('49 negative gates passed' in completed.stdout and sha(program)==digest,'compiled frame regression/identity differs')
 actual=json.loads(output.read_text());compare_native_frames(actual,app);declines=0
 if falsify:
  for path,value in json_leaves(actual):
   replacements=[not value, int(value)] if type(value) is bool else [value+1,bool(value)] if type(value) is int else [value+'x',False]
   for replacement in replacements:
    changed=copy.deepcopy(actual);parent=changed
    for key in path[:-1]:parent=parent[key]
    parent[path[-1]]=replacement
    try:compare_native_frames(changed,app)
    except ValueError:declines+=1
    else:raise AssertionError('native frame value/type mutation admitted')
  variants=[dict(actual,unknown=0),{key:value for key,value in actual.items() if key!='boundary'}]
  variants.extend(dict(actual,stores=actual['stores'][:i]+actual['stores'][i+1:]) for i in range(len(actual['stores'])))
  variants.extend(dict(actual,states=actual['states'][:i]+actual['states'][i+1:]) for i in range(len(actual['states'])))
  for changed in variants:
   try:compare_native_frames(changed,app)
   except ValueError:declines+=1
   else:raise AssertionError('native frame structural/store omission admitted')
 return dict(executable_sha256=digest,output_sha256=sha(output),instruction_states=len(actual['states']),
  ordered_stores=len(actual['stores']),state_and_store_fields=sum(1 for _ in json_leaves(actual)),
  mutation_rejections=declines,compiled_negative_gates=49,source_only_fields='XER/CTR/FPSCR, complete GPRs, retained SR/GQR/paired/IBAT/reservation; finite receipts cover only their observed subset',
  stop='812003b8 before lis; clock and scheduler unowned',native_production_admission=False)

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--source-root',type=Path,required=True);p.add_argument('--apploader',type=Path,required=True)
 p.add_argument('--entry-capture',type=Path,required=True);p.add_argument('--self-test',action='store_true')
 p.add_argument('--dtk-capture',type=Path,action='append',default=[]);p.add_argument('--baseline-capture',type=Path);p.add_argument('--dtk-helper',type=Path)
 p.add_argument('--nested-capture',type=Path)
 p.add_argument('--initial-program',type=Path);p.add_argument('--initial-program-sha256')
 p.add_argument('--frame-program',type=Path);p.add_argument('--frame-program-sha256')
 p.add_argument('--frame-output',type=Path);p.add_argument('--main-dol',type=Path)
 a=p.parse_args()
 for relative,expected in PINS.items():require(sha(a.source_root/relative)==expected,'unreviewed source '+relative)
 result={'source_pins':len(PINS),'entry':audit_entry(a.entry_capture,a.source_root,a.apploader,a.self_test),
  'scope':'independent passive receipt parity only; no native provider or frontier promotion',
  'remaining':'typed native first-Advance GPU/Movie/NewField/achievement effects; whole apploader stores/devices/event ownership'}
 if a.dtk_capture:
  require(a.baseline_capture is not None and a.dtk_helper is not None,'DTK requires explicit baseline and helper paths')
  result['dtk']=[audit_dtk(path,a.baseline_capture,a.dtk_helper,a.source_root,a.self_test) for path in a.dtk_capture]
 if a.nested_capture:result['nested']=audit_nested(a.nested_capture,a.source_root,a.apploader,a.self_test)
 if a.initial_program:result['native_initial']=audit_native_initial(a.initial_program,a.initial_program_sha256,a.self_test)
 if a.frame_program:result['native_frames']=audit_native_frames(a.frame_program,a.frame_program_sha256,a.frame_output,a.main_dol,a.apploader,a.self_test)
 print(json.dumps(result,indent=2))

if __name__=='__main__':main()

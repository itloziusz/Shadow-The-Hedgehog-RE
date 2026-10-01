"""Separate passive original apploader first-three state/store receipt.

Only a copied Interpreter TU is changed. Original source/build/game and older
oracles are read-only. The observer reads live state and direct RAM bytes,
never a guest MMU read, opcode fetch, clock producer or state writer.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import socket
import subprocess
import time
from build_readonly_boot_oracle import command_args, digest, response_run
from capture_dolphin_rsp import RSP
from capture_boot_machine_state import DISC_SHA
from agent_apploader_entry_42 import SOURCE_SHA

ROOT = Path(__file__).resolve().parents[3]
INTERPRETER_SHA = '1993e7a3976be7aa7d276f1e0b25e04bbdcbe1a4555fac837a4794f6bc18128d'
BASE_EXE_SHA = '84be4d74d3a566b92e46c416d5e9c4ff87f313612fb2f4e529852cb74b0cfe78'
WATCH = '015edc98:32,01200258:12,00000c00:4,01300000:4'
FIELDS = ('seq stage instruction_pc word pc npc r0 r1 lr msr exceptions cr hid0 hid1 hid2 '
          'dcache ram_real ram_mask dbat0u dbat0l dbat1u dbat1l global_timer slice downcount '
          'stack handler report').split()
NESTED_FIELDS = FIELDS[:-3] + ['r3','r4','r5','r29','r30','r31'] + FIELDS[-3:]
NESTED_WATCH = '015edc78:64,01200258:16,0120039c:32,00000c00:4,01300000:4'
OBSERVER = r'''
#include <cstdio>
#include <cstdlib>
#include "Core/HW/Memmap.h"
namespace {
struct ShadowEntryReceipt42 {
  FILE* file=nullptr; u64 sequence=0; bool finished=false;
  ShadowEntryReceipt42() { if(const char* p=std::getenv("SHADOW_ENTRY_RECEIPT_42"))file=std::fopen(p,"wx"); }
  ~ShadowEntryReceipt42(){if(file)std::fclose(file);}
};
void ShadowEntryObserve42(Core::System& system,const char* stage,u32 original_pc,u32 word) {
  if(original_pc<0x81200258||original_pc>0x81200264)return;
  static ShadowEntryReceipt42 log;if(!log.file||log.finished)return;
  const auto& s=system.GetPPCState();const auto& g=system.GetCoreTiming().GetGlobals();
  auto& memory=system.GetMemory();const auto* ram=memory.GetRAM();
  std::fprintf(log.file,"%llu\t%s\t%08x\t%08x\t%08x\t%08x\t%08x\t%08x\t%08x\t%08x\t%08x\t%08x\t%08x\t%08x\t%08x\t%08x\t%08x\t%08x\t%08x\t%08x\t%08x\t%08x\t%016llx\t%016llx\t%08x\t",
    ++log.sequence,stage,original_pc,word,s.pc,s.npc,s.gpr[0],s.gpr[1],s.spr[SPR_LR],
    s.msr.Hex,s.Exceptions,s.cr.Get(),s.spr[SPR_HID0],s.spr[SPR_HID1],s.spr[SPR_HID2],
    u32(s.m_enable_dcache),memory.GetRamSizeReal(),memory.GetRamMask(),
    s.spr[SPR_DBAT0U],s.spr[SPR_DBAT0L],s.spr[SPR_DBAT1U],s.spr[SPR_DBAT1L],
    u64(g.global_timer),u64(g.slice_length),u32(s.downcount));
  if(ram&&memory.GetRamSizeReal()>=0x015edcb8) {
    for(u32 i=0;i<32;++i)std::fprintf(log.file,"%02x",ram[0x015edc98+i]);
    std::fputc('\t',log.file);
    for(u32 i=0;i<4;++i)std::fprintf(log.file,"%02x",ram[0xc00+i]);
    std::fputc('\t',log.file);
    for(u32 i=0;i<4;++i)std::fprintf(log.file,"%02x",ram[0x01300000+i]);
  } else std::fputs("-\t-\t-",log.file);
  std::fputc('\n',log.file);std::fflush(log.file);
  if(original_pc==0x81200264)log.finished=true;
}
}
'''


def require(condition, message):
    if not condition:
        raise ValueError(message)


def owned_output(path):
    path = path.resolve()
    require(path.is_relative_to((ROOT / 'build').resolve()), 'output outside repository build')
    return path


def nested_observer():
    text = OBSERVER.replace('if(original_pc<0x81200258||original_pc>0x81200264)return;',
        'if(original_pc!=0x81200264&&(original_pc<0x8120039c||original_pc>0x812003b8))return;')
    text = text.replace('if(ram&&memory.GetRamSizeReal()>=0x015edcb8)',
        'std::fprintf(log.file,"%08x\\t%08x\\t%08x\\t%08x\\t%08x\\t%08x\\t",s.gpr[3],s.gpr[4],s.gpr[5],s.gpr[29],s.gpr[30],s.gpr[31]);\n  if(ram&&memory.GetRamSizeReal()>=0x015edcb8)')
    text = text.replace('for(u32 i=0;i<32;++i)std::fprintf(log.file,"%02x",ram[0x015edc98+i]);',
        'for(u32 i=0;i<64;++i)std::fprintf(log.file,"%02x",ram[0x015edc78+i]);')
    return text.replace('if(original_pc==0x81200264)log.finished=true;',
        'if(original_pc==0x812003b8)log.finished=true;')


def instrument(original, scope='entry'):
    text = original
    anchor = '#include "Core/System.h"'
    require(text.count(anchor) == 1, 'source include anchor differs')
    text = text.replace(anchor, anchor + (OBSERVER if scope=='entry' else nested_observer()), 1)
    anchor = 'int Interpreter::SingleStepInner()\n{\n'
    require(text.count(anchor) == 1, 'SingleStepInner source anchor differs')
    text = text.replace(anchor, anchor + '  const u32 shadow_entry_pc42=m_ppc_state.pc;\n', 1)
    anchor = '  const GekkoOPInfo* opinfo = PPCTables::GetOpInfo(m_prev_inst, m_ppc_state.pc);'
    require(text.count(anchor) == 1, 'existing fetched opcode anchor differs')
    text = text.replace(anchor, anchor + '\n  ShadowEntryObserve42(m_system,"enter",shadow_entry_pc42,m_prev_inst.hex);', 1)
    anchor = '  return opinfo->num_cycles;'
    require(text.count(anchor) == 1, 'ordinary exit anchor differs')
    text = text.replace(anchor, '  ShadowEntryObserve42(m_system,"exit",shadow_entry_pc42,m_prev_inst.hex);\n' + anchor, 1)
    anchor = '  // Declare start of new slice\n  core_timing.Advance();'
    require(text.count(anchor) == 1, 'first Advance source anchor differs')
    start_pc = '0x81200258' if scope=='entry' else '0x81200264'
    text = text.replace(anchor, f'  if(m_ppc_state.pc=={start_pc})ShadowEntryObserve42(m_system,"before-advance",m_ppc_state.pc,0);\n' + anchor, 1)
    return text


def build(a):
    output = owned_output(a.output)
    require(not output.exists(), 'fresh oracle directory required')
    original = a.source / 'Source/Core/Core/PowerPC/Interpreter/Interpreter.cpp'
    require(digest(original) == INTERPRETER_SHA, 'original Interpreter identity differs')
    old = json.loads((a.base / 'manifest.json').read_text())
    require(digest(a.base / 'Dolphin.exe') == BASE_EXE_SHA, 'base oracle identity differs')
    require(not old.get('clock_perturbations'), 'writer oracle is not admissible base')
    for path, expected in old['link_input_sha256'].items():
        require(digest(Path(path)) == expected, 'old link input identity changed: ' + path)
    output.mkdir(); (output / 'tmp').mkdir()
    shutil.copy2(Path(__file__), output / 'builder_snapshot.py')
    copied = output / 'Interpreter.cpp'
    copied.write_text(instrument(original.read_text(encoding='utf-8'), a.scope), encoding='utf-8')
    env = dict(os.environ, TMP=str(output / 'tmp'), TEMP=str(output / 'tmp'))
    tool_bin = a.msvc / 'bin/Hostx64/x64'
    env['PATH'] = str(tool_bin) + ';' + str(a.sdk_root / 'bin' / a.sdk_version / 'x64') + ';' + env.get('PATH', '')
    env['INCLUDE'] = ';'.join(map(str, [a.msvc / 'include'] + [a.sdk_root / 'Include' / a.sdk_version / p for p in ('ucrt','shared','um','winrt')]))
    env['LIB'] = ';'.join(map(str, [a.msvc / 'lib/x64'] + [a.sdk_root / 'Lib' / a.sdk_version / p / 'x64' for p in ('ucrt','um')]))
    args = command_args((a.base / 'cl.rsp').read_text(encoding='utf-16'))
    args = [p for p in args if not p.lower().startswith(('/fo','/fd')) and not p.upper().endswith('INTERPRETER.CPP')]
    args += ['/Fo' + str(output / 'Interpreter.obj'), '/Fd' + str(output / 'Interpreter.pdb'), str(copied)]
    response_run('cl', args, output, env, tool_bin)
    args = command_args((a.base / 'link.rsp').read_text(encoding='utf-16'))
    args = [p for p in args if not p.lower().startswith(('/out:','/pdb:','/implib:')) and Path(p).name.lower() != 'interpreter.obj']
    args = [str(output / 'Interpreter.obj')] + args + ['/OUT:' + str(output / 'Dolphin.exe'), '/PDB:' + str(output / 'Dolphin.pdb'), '/IMPLIB:' + str(output / 'Dolphin.lib')]
    response_run('link', args, output, env, tool_bin)
    for p in a.base.glob('*.dll'):
        shutil.copy2(p, output / p.name)
    for name in ('qt.conf',):
        if (a.base / name).exists():
            shutil.copy2(a.base / name, output / name)
    for name in ('Sys', 'QtPlugins'):
        shutil.copytree(a.base / name, output / name)
    manifest = dict(purpose='passive original apploader first-three receipt; no native dependency',
        tool_sha256=digest(Path(__file__)), base_oracle_sha256=BASE_EXE_SHA,
        original_interpreter_sha256=digest(original), copied_interpreter_sha256=digest(copied),
        observer_object_sha256=digest(output / 'Interpreter.obj'),
        instrumented_executable_sha256=digest(output / 'Dolphin.exe'),
        inherited_link_input_sha256=old['link_input_sha256'],
        gdb_object_sha256=digest(a.base / 'GDBStub.obj'), fields=FIELDS if a.scope=='entry' else NESTED_FIELDS,
        scope=a.scope,
        original_source=str(original), oracle_directory=str(output),
        builder_snapshot_sha256=digest(output / 'builder_snapshot.py'),
        gdb_object=str(a.base / 'GDBStub.obj'),
        original_source_pins={str(a.source / n):h for n,h in {
            **SOURCE_SHA, 'Source/Core/Core/PowerPC/Interpreter/Interpreter_Branch.cpp':
            'ac92c3fc9f5d05b7bc6fea7a39bdb0682c13d80223fc8ca88131a55a6df29068'}.items()},
        compiler=str(tool_bin / 'cl.exe'), compiler_sha256=digest(tool_bin / 'cl.exe'),
        compile_recipe_sha256=digest(output / 'cl.rsp'), link_recipe_sha256=digest(output / 'link.rsp'),
        transform='copied TU: observe before original firstAdvance, existing fetched enter/exit first3 and enter81200264; direct live state/RAM only',
        clock_call_added=False, state_writer_added=False, opcode_fetch_added=False)
    (output / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print('PASS built copied Entry receipt oracle', manifest['instrumented_executable_sha256'])


def capture(a):
    output, user = owned_output(a.output), owned_output(a.user_dir)
    require(not output.exists() and not user.exists(), 'fresh capture/profile required')
    require(digest(a.disc) == DISC_SHA, 'original startup disc identity differs')
    manifest = json.loads((a.dolphin.parent / 'manifest.json').read_text())
    scope = manifest.get('scope','entry')
    watch = WATCH if scope=='entry' else NESTED_WATCH
    require(digest(a.dolphin) == manifest['instrumented_executable_sha256'], 'receipt oracle identity differs')
    user.mkdir(); (user / 'Temp').mkdir()
    paths = {k: output.with_suffix('.' + k + '.tsv') for k in ('states', 'writes', 'events')}
    env = dict(os.environ, TMP=str(user / 'Temp'), TEMP=str(user / 'Temp'))
    for key in list(env):
        if key.startswith(('DOLPHIN_EVENT_', 'DOLPHIN_SI_', 'DOLPHIN_OSINIT_', 'DOLPHIN_COMPOSITION_', 'SHADOW_TIMING_', 'SHADOW_ENTRY_')):
            env.pop(key)
    env.update(SHADOW_ENTRY_RECEIPT_42=str(paths['states']), DOLPHIN_SI_WRITE_LOG=str(paths['writes']),
        DOLPHIN_SI_WATCH_EXTRA=watch, DOLPHIN_SI_WATCH_ONLY='1', DOLPHIN_SI_EVENT_LOG=str(paths['events']),
        DOLPHIN_EVENT_WINDOW='init..timing-prefix-end@80373ac4', DOLPHIN_EVENT_LIMIT='3000000')
    startup = subprocess.STARTUPINFO(); startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW; startup.wShowWindow = subprocess.SW_HIDE
    command = [str(a.dolphin), '-b', '-u', str(user), '-C', f'Dolphin.General.GDBPort={a.port}',
        '-C', 'Dolphin.Core.CPUCore=0', '-C', 'Dolphin.Interface.DebugModeEnabled=True', '-e', str(a.disc)]
    proc = subprocess.Popen(command, env=env, cwd=a.dolphin.parent, startupinfo=startup, creationflags=subprocess.CREATE_NO_WINDOW)
    result = None
    try:
        deadline = time.monotonic() + 40; sock = None
        while time.monotonic() < deadline:
            require(proc.poll() is None, 'original oracle exited before startup stop')
            try:
                sock = socket.create_connection(('127.0.0.1', a.port), timeout=1); break
            except OSError:
                time.sleep(.1)
        require(sock is not None, 'original DOL startup stop unavailable')
        with sock:
            rsp = RSP(sock)
            pc = rsp.send('p40')
            require(pc.lower() == '80003154', 'unexpected original startup stop')
            result = dict(name='apploader_entry_receipt_42', scope='original GC HLE reference; no native owner',
                capture_complete=False, disc_sha256=DISC_SHA, observer_manifest=manifest,
                no_rsp_writes=True, preentry_breakpoints=[], original_dol_entry=dict(pc=pc,
                r0=rsp.send('p0'), r1=rsp.send('p1'), lr=rsp.send('p43'),
                stack=rsp.memory(0x815edc98,32).hex(), syscall=rsp.memory(0x80000c00,4).hex(),
                report=rsp.memory(0x81300000,4).hex(), cycles=rsp.send('pfa')),
                observer_environment=dict(write_watch=watch, write_only=True, event_window=env['DOLPHIN_EVENT_WINDOW']),
                logs={k:p.name for k,p in paths.items()})
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill(); proc.wait(timeout=5)
    require(result is not None, 'capture did not return an original startup state')
    result['log_sha256'] = {k:digest(p) for k,p in paths.items()}
    result['capture_complete'] = True
    output.write_text(json.dumps(result, indent=2) + '\n')
    verified = check(output)
    verified['verified_tool_sha256'] = digest(Path(__file__))
    output.write_text(json.dumps(verified, indent=2) + '\n')


def check(path):
    j = json.loads(path.read_text())
    require(j['capture_complete'] and j['no_rsp_writes'] and not j['preentry_breakpoints'], 'capture mutation or incomplete')
    manifest = j['observer_manifest']
    scope = manifest.get('scope','entry')
    require(scope in ('entry','nested'), 'unknown finite receipt scope')
    oracle = Path(manifest['oracle_directory'])
    require(oracle.is_relative_to((ROOT / 'build').resolve()), 'receipt oracle outside owned build')
    bindings = {'Interpreter.cpp':'copied_interpreter_sha256', 'Interpreter.obj':'observer_object_sha256',
                'Dolphin.exe':'instrumented_executable_sha256', 'builder_snapshot.py':'builder_snapshot_sha256',
                'cl.rsp':'compile_recipe_sha256', 'link.rsp':'link_recipe_sha256'}
    for name,key in bindings.items():
        require(digest(oracle / name) == manifest[key], 'copied oracle binding differs: ' + name)
    original = Path(manifest['original_source'])
    require(digest(original) == INTERPRETER_SHA == manifest['original_interpreter_sha256'], 'original Interpreter binding differs')
    require((oracle / 'Interpreter.cpp').read_text(encoding='utf-8') == instrument(original.read_text(encoding='utf-8'),scope),
            'copied observer transform differs from exact original source')
    require(manifest['tool_sha256'] == manifest['builder_snapshot_sha256'], 'build-time helper snapshot differs')
    require(digest(Path(manifest['gdb_object'])) == manifest['gdb_object_sha256'], 'read-only GDB object differs')
    require(digest(Path(manifest['compiler'])) == manifest['compiler_sha256'], 'matching compiler binary differs')
    for source,expected in manifest['original_source_pins'].items():
        require(digest(Path(source)) == expected, 'original producer source differs: ' + source)
    for source,expected in manifest['inherited_link_input_sha256'].items():
        require(digest(Path(source)) == expected, 'original link input differs: ' + source)
    p = {k:path.parent / n for k,n in j['logs'].items()}
    for k in p:
        require(digest(p[k]) == j['log_sha256'][k], 'receipt log identity changed')
    lines = p['states'].read_text().splitlines()
    fields = FIELDS if scope=='entry' else NESTED_FIELDS
    rows = [dict(zip(fields, line.split('\t'), strict=True)) for line in lines]
    if scope=='nested':
        return check_nested(j,rows,p)
    expected = [('before-advance','81200258'),('enter','81200258'),('exit','81200258'),
        ('enter','8120025c'),('exit','8120025c'),('enter','81200260'),('exit','81200260'),('enter','81200264')]
    require([(r['stage'], r['instruction_pc']) for r in rows] == expected, 'first-three state receipt incomplete/order differs')
    for i,r in enumerate(rows,1):
        require(int(r['seq']) == i, 'state ordinal differs')
        require(r['r1'] == ('815edca8' if i <= 4 else '815edca0'), 'original SP transition differs')
        require(r['lr'] == '00000000' and r['exceptions'] == '00000000', 'LR/fault state differs')
        require(r['dcache'] == '00000000' and r['ram_real'] == '01800000', 'actual RAM/cache profile differs')
        require((r['dbat0u'],r['dbat0l']) == ('80001fff','00000002'), 'actual DBAT0 mapping differs')
        require((r['dbat1u'],r['dbat1l'],r['ram_mask']) == ('c0001fff','0000002a','01ffffff'), 'actual alternate mapping/mask differs')
        require(r['msr']=='00002032', 'actual DR/IR/no-exception profile differs')
        require((r['cr'],r['hid0'],r['hid1'],r['hid2']) == ('00000000','0011c464','80000000','e0000000'), 'retained Entry register field differs')
        require(r['handler'] == '4c000064' and r['report'] == '00000000', 'handler/report Entry timeline differs')
        if i >= 3:
            require(r['r0'] == '00000000', 'mflr result differs')
        if i >= 5:
            require(r['stack'][16:24] == '815edca8', 'observed backchain missing')
        require(r['stack'][40:48] == '00000000', 'LR spill backing differs')
        expected_stack = bytearray(32)
        if i>=5:
            expected_stack[8:12]=bytes.fromhex('815edca8')
        require(r['stack']==expected_stack.hex(), 'unexplained watched stack-byte effect')
    require([(r['pc'],r['npc']) for r in rows] == [
        ('81200258','00000000'),('81200258','8120025c'),('8120025c','8120025c'),
        ('8120025c','81200260'),('81200260','81200260'),('81200260','81200264'),
        ('81200264','81200264'),('81200264','81200268')], 'actual PC/NPC transition differs')
    require([(int(r['global_timer'],16),int(r['slice'],16),int(r['downcount'],16)) for r in rows] == [
        (0,20000,0),(20000,10888,10888),(20000,10888,10888),
        (20001,10887,10887),(20001,10887,10887),(20002,10886,10886),
        (20002,10886,10886),(20003,10885,10885)], 'before/after Advance phases differ')
    for i,word in ((1,'7c0802a6'),(2,'7c0802a6'),(3,'9421fff8'),(4,'9421fff8'),(5,'9001000c'),(6,'9001000c')):
        require(rows[i]['word'] == word, 'already-fetched original word differs')
    write_lines = p['writes'].read_text().splitlines()
    require(write_lines[0] == '0\twatch-config\textra=' + WATCH + ';only=1', 'writer watch declaration differs')
    writes = [line.split('\t') for line in write_lines[1:]]
    stores = [r for r in writes if len(r)==18 and r[1]=='cpu-translated-write' and r[8] in ('8120025c','81200260')]
    require([(r[8],r[2],r[3],r[4],r[5],r[16]) for r in stores] == [
        ('8120025c','015edca0','4','815edca0','015edca0','815edca8'),
        ('81200260','015edcac','4','815edcac','015edcac','00000000')], 'exact ordered physical stores missing')
    require(all(r[12]=='0' for r in stores), 'write receipts route through unowned cache')
    require(any(r[1]=='memory-clear' and r[6]=='015edc98' and r[7]=='32' and r[16]=='00'*32 for r in writes if len(r)==18), 'fresh stack clear receipt missing')
    require(any(r[1]=='memory-copy-write' and r[2]=='01200000' and r[3]=='122424' and r[16]=='7c0802a69421fff89001000c' for r in writes if len(r)==18), 'original app-byte producer missing')
    require(all(len(r)==18 for r in writes), 'malformed physical-writer receipt')
    first_store=int(stores[0][0])
    for r in writes:
        if int(r[0])>=first_store or int(r[7])==0:
            continue
        if r[1]=='memory-clear':
            require(r[2]=='00000000' and r[3]=='33554432' and r[16]=='00'*int(r[7]), 'fresh clear producer differs')
        elif r[1]=='cpu-translated-write':
            require((r[2],r[3],r[4],r[8],r[16])==('00000c00','4','80000c00','00000000','4c000064'), 'unexplained pre-entry translated writer')
        elif r[1]=='memory-copy-write':
            require((r[2],r[3],r[6],r[16])==('01200000','122424','01200258','7c0802a69421fff89001000c'), 'unexplained pre-entry copied writer')
        elif r[1]=='pointer-lease':
            require('MemoryManager::CopyToEmu(' in r[15] or 'MemoryManager::CopyFromEmu(' in r[15], 'unexplained pre-entry pointer lease')
            require(r[6]=='01200258' and r[8] in ('00000000','81200258'), 'unexplained pre-entry lease address/path')
        else:
            raise ValueError('unexplained overlapping pre-entry record: ' + r[1])
    report_writes = [r for r in writes if r[1]=='cpu-translated-write' and r[2]=='01300000']
    require(len(report_writes)==1 and report_writes[0][8]=='00000000' and report_writes[0][9]=='118726' and report_writes[0][16]=='4e800020', 'report word installation interval differs')
    event_rows = [line.split('\t') for line in p['events'].read_text().splitlines()]
    functions = [(r[2],r[4]) for r in event_rows if len(r)==16 and r[2] in ('boot-function-enter','boot-function-exit')]
    require(functions[:3]==[('boot-function-enter','0000000081200258'),('boot-function-exit','0000000000000000'),('boot-function-enter','0000000081200278')], 'Entry exit to Init entry brackets missing')
    require(j['original_dol_entry']['cycles'].lower()=='000000000028d70d', 'original pending DOL-entry cycle changed')
    j['admission'] = 'PASS original first-three state/store receipt; native producer and complete foreign-write exclusion remain separate'
    j['verified_state_rows'] = len(rows)
    print(j['admission'])
    print('ordered stores:', [(r[8],r[4],r[16]) for r in stores])
    return j


def check_nested(j, rows, paths):
    words = {0x81200264:'48000139',0x8120039c:'7c0802a6',0x812003a0:'90010004',
        0x812003a4:'9421ffd8',0x812003a8:'bfa1001c',0x812003ac:'90610008',
        0x812003b0:'9081000c',0x812003b4:'90a10010',0x812003b8:'3c608120'}
    executed = list(words)[:-1]
    expected = [('before-advance','81200264')] + [
        (stage,f'{pc:08x}') for pc in executed for stage in ('enter','exit')] + [('enter','812003b8')]
    require([(r['stage'],r['instruction_pc']) for r in rows]==expected, 'nested state receipt incomplete/order differs')
    before=rows[0]
    args = {n:before[n] for n in ('r3','r4','r5')}
    saved = {n:before[n] for n in ('r29','r30','r31')}
    require(args==dict(r3='80003100',r4='80003104',r5='80003108'), 'actual source Entry arguments differ')
    for i,r in enumerate(rows,1):
        require(int(r['seq'])==i, 'nested state ordinal differs')
        require(r['exceptions']=='00000000' and r['msr']=='00002032', 'nested fault/translation profile differs')
        require((r['dbat0u'],r['dbat0l'],r['dbat1u'],r['dbat1l'],r['dcache'],r['ram_real'],r['ram_mask'])==
            ('80001fff','00000002','c0001fff','0000002a','00000000','01800000','01ffffff'), 'nested actual mapping/cache profile differs')
        require((r['cr'],r['hid0'],r['hid1'],r['hid2'])==('00000000','0011c464','80000000','e0000000'), 'nested retained fields differ')
        require({n:r[n] for n in args}==args and {n:r[n] for n in saved}==saved, 'argument/saved registers changed before their modeled consumer')
        require(r['r1']==('815edca0' if i<9 else '815edc78'), 'nested actual SP effect differs')
        require(r['r0']==('00000000' if i<5 else '81200268'), 'nested mflr effect differs')
        require(r['lr']==('00000000' if i<3 else '81200268'), 'nested call LR effect differs')
        require(r['handler']=='4c000064' and r['report']=='00000000', 'nested handler/report timeline differs')
        stack=bytearray(64)
        def put(address,value):
            stack[address-0x815edc78:address-0x815edc78+4]=bytes.fromhex(value)
        put(0x815edca0,'815edca8')
        if i>=7: put(0x815edca4,'81200268')
        if i>=9: put(0x815edc78,'815edca0')
        if i>=11:
            for address,n in zip((0x815edc94,0x815edc98,0x815edc9c),saved,strict=True): put(address,saved[n])
        if i>=13: put(0x815edc80,args['r3'])
        if i>=15: put(0x815edc84,args['r4'])
        if i>=17: put(0x815edc88,args['r5'])
        require(r['stack']==stack.hex(), 'unexplained nested stack-byte effect')
        if i==1:
            require((r['pc'],r['npc'],r['global_timer'],r['slice'],r['downcount'])==
                ('81200264','81200264','0000000000004e22','0000000000000001','00000000'), 'nested before-Advance phase differs')
        else:
            instruction_pc=int(r['instruction_pc'],16)
            require(r['word']==words[instruction_pc], 'nested already-fetched raw word differs')
            target=0x8120039c if instruction_pc==0x81200264 else instruction_pc+4
            pc=instruction_pc if r['stage']=='enter' else target
            npc=instruction_pc+4 if r['stage']=='enter' else target
            require((r['pc'],r['npc'])==(f'{pc:08x}',f'{npc:08x}'), 'nested actual PC/NPC effect differs')
            step=executed.index(instruction_pc) if instruction_pc in executed else len(executed)
            expected_timer=20003+step
            require((int(r['global_timer'],16),int(r['slice'],16),int(r['downcount'],16))==
                (expected_timer,30888-expected_timer,30888-expected_timer), 'nested source phase differs')
    lines=paths['writes'].read_text().splitlines()
    require(lines[0]=='0\twatch-config\textra='+NESTED_WATCH+';only=1', 'nested writer watch differs')
    writes=[line.split('\t') for line in lines[1:]]
    require(all(len(r)==18 for r in writes), 'nested physical writer record malformed')
    stores=[r for r in writes if r[1]=='cpu-translated-write' and int(r[8],16) in executed]
    expected_stores=[('812003a0','815edca4','81200268'),('812003a4','815edc78','815edca0'),
        ('812003a8','815edc94',saved['r29']),('812003a8','815edc98',saved['r30']),('812003a8','815edc9c',saved['r31']),
        ('812003ac','815edc80',args['r3']),('812003b0','815edc84',args['r4']),('812003b4','815edc88',args['r5'])]
    require([(r[8],r[4],r[16]) for r in stores]==expected_stores, 'nested ordered physical spills differ')
    for r in stores:
        require(int(r[2],16)==int(r[4],16)&0x3fffffff and r[5]==r[2] and r[3]=='4' and r[7]=='4' and r[12]=='0', 'nested exact physical alias/size/cache differs')
        step=executed.index(int(r[8],16))
        require(int(r[9])==20003+step, 'nested write instruction phase differs')
    clear=[r for r in writes if r[1]=='memory-clear' and r[6]=='015edc78']
    require(len(clear)==1 and clear[0][3]=='33554432' and clear[0][7]=='64' and clear[0][16]=='00'*64, 'nested fresh range clear differs')
    copies=[r for r in writes if r[1]=='memory-copy-write' and r[2]=='01200000']
    require([(r[6],r[7],r[16]) for r in copies]==[
        ('01200258','16','7c0802a69421fff89001000c48000139'),
        ('0120039c','32','7c0802a6900100049421ffd8bfa1001c906100089081000c90a100103c608120')], 'nested raw code producer differs')
    last_store=int(stores[-1][0])
    allowed_stores={*expected_stores,('00000000','80000c00','4c000064'),
        ('8120025c','815edca0','815edca8'),('81200260','815edcac','00000000')}
    code_overlaps={'01200258','01200260','0120039c','012003a0'}
    for r in writes:
        if int(r[0])>last_store or int(r[7])==0:
            continue
        if r[1]=='cpu-translated-write':
            require((r[8],r[4],r[16]) in allowed_stores, 'unexplained nested-window translated writer')
        elif r[1]=='memory-clear':
            require(r[2]=='00000000' and r[3]=='33554432' and r[16]=='00'*int(r[7]), 'unexplained nested-window clear')
        elif r[1]=='memory-copy-write':
            require(r in copies, 'unexplained nested-window copy')
        elif r[1]=='pointer-lease':
            require(r[6] in code_overlaps and ('MemoryManager::CopyToEmu(' in r[15] or
                'MemoryManager::CopyFromEmu(' in r[15]), 'unexplained nested-window pointer lease')
        else:
            raise ValueError('unexplained overlapping nested-window record: '+r[1])
    require(j['original_dol_entry']['cycles'].lower()=='000000000028d70d', 'nested observer changed original pending startup cycle')
    j['verified_state_rows']=len(rows)
    j['directly_observed_saved_registers']=saved
    j['directly_observed_entry_arguments']=args
    j['verified_spills']=[dict(pc=pc,address=address,value=value) for pc,address,value in expected_stores]
    j['admission']='PASS original nested Entry call/prologue receipt; native producer and next effect ownership remain separate'
    print(j['admission']);print('actual saved registers:',saved);print('ordered spills:',expected_stores)
    return j



# Immutable reference receipts and independent audit summary: later helper
# edits cannot silently relabel the accepted original evidence.
ENTRY_CAPTURE_SHA42 = 'a639cd3c33267d629753298fc1745dba03117bcd69ebd26e34ab0b1671a327c3'
NESTED_CAPTURE_SHA42 = '8caee16bc973b29532fe392111b921f89e3ee806389f26aa25ee3eab52d5af0f'
INDEPENDENT_SUMMARY_SHA42 = 'e10970f2427e5275bee87506eb91d8054b57bf23eeb5eb076fdd0b70e1fd6b6a'
# This exact reviewer snapshot is archived separately from its growing tool.
INDEPENDENT_AUDITOR_SHA42 = '9ae357277f05b7f1b5156a79617c0fa9ab6fb497f43f340db5902a4fafead53e'
ENTRY_NATIVE_SOURCE_PINS42 = {
    'reverse/boot/include/shadow/boot/ApploaderEntryFrames.hpp':
        '346f3a7f6b652b3a1c4d0004abe1c37eddc83ae923faf256bfb0a808f18512a1',
    'reverse/boot/src/ApploaderEntryFrames.cpp':
        '851301dbb6a9c263f71410109088f42a537488908fe67f3d949c21575b364f8f',
    'reverse/boot/tests/test_apploader_entry_frames.cpp':
        'f2d53ba5120ed1e1d62126eab029e02a4f6587208018aaf34d018c318a29276f',
}
ENTRY_ORIGINAL_INPUT_PINS42 = {
    'apploader.img':'8a1fe9487b029b2518a6633b1e0eb9ec0922f5f5eb5afdd7eba1267a85f096fe',
    'boot.bin':'7d387ce9162342a93a3becac1d2df6db1cccf6a2439f3fc97b29b2f7047d8e88',
    'bi2.bin':'8bbadffc2a7f79eda5982aa2fe30831969c2702aa9206f621b930aae0b70af6b',
}


def strict_json(path):
    def pairs(items):
        result={}
        for key,value in items:
            require(key not in result, 'duplicate JSON key: '+key)
            result[key]=value
        return result
    return json.loads(Path(path).read_text(),object_pairs_hook=pairs,
                      parse_constant=lambda value: (_ for _ in ()).throw(ValueError('nonfinite JSON '+value)))


def same_json(actual, expected, path='$'):
    # Python's False==0/True==1 and1.0==1 must never satisfy an owner field.
    require(type(actual) is type(expected), 'JSON scalar/container type differs at '+path)
    if type(expected) is dict:
        require(set(actual)==set(expected), 'JSON keys differ at '+path)
        for key in expected:
            same_json(actual[key],expected[key],path+'.'+key)
    elif type(expected) is list:
        require(len(actual)==len(expected), 'JSON list arity differs at '+path)
        for i,value in enumerate(expected):
            same_json(actual[i],value,path+f'[{i}]')
    else:
        require(actual==expected, 'JSON value differs at '+path)


def entry_reference_rows(capture, fields):
    j=strict_json(capture)
    paths={k:Path(capture).parent/n for k,n in j['logs'].items()}
    rows=[dict(zip(fields,line.split('\t'),strict=True))
          for line in paths['states'].read_text().splitlines()]
    writes=[line.split('\t') for line in paths['writes'].read_text().splitlines()[1:]]
    return rows,writes


def expected_entry_native42(entry_path,nested_path):
    e,ew=entry_reference_rows(entry_path,FIELDS)
    n,nw=entry_reference_rows(nested_path,NESTED_FIELDS)
    observed=('pc npc r0 r1 lr msr exceptions cr hid0 hid1 hid2 dcache '
              'ram_real ram_mask dbat0u dbat0l dbat1u dbat1l').split()
    saved=('r3 r4 r5 r29 r30 r31').split()
    # Unreceipted scalar fields are source-Reset facts, separately audited.
    # No such scalar is claimed directly observed by the first-three receipt.
    def snapshot(row,full=False):
        s={key:int(row[key],16) for key in observed}
        s.update(xer=0,ctr=0,fpscr=0)
        s.update({key:int(row[key],16) if key in row else value
                  for key,value in zip(saved,(0x80003100,0x80003104,0x80003108,0,0,0),strict=True)})
        gpr=[0]*32
        gpr[2],gpr[13]=0x814b5b20,0x814b4fc0
        for i in (0,1,3,4,5,29,30,31):gpr[i]=s['r'+str(i)]
        s.update(gpr=gpr,stack_base=0x815edc78,physical_base=0x015edc78,
                 stack=row['stack'] if full else '00'*32+row['stack'],known='1'*64)
        return s
    states=[]
    for row,full in [(r,False) for r in e if r['stage'] in ('enter','exit')
                     and int(r['instruction_pc'],16)<=0x81200260]+[
                     (r,True) for r in n if r['stage'] in ('enter','exit')
                     and r['instruction_pc']!='812003b8']:
        states.append(dict(instruction_pc=int(row['instruction_pc'],16),word=int(row['word'],16),
                           stage='inner-exit' if row['stage']=='exit' else 'enter',state=snapshot(row,full)))
    stores=[]
    for row in [r for r in ew if r[1]=='cpu-translated-write' and r[8] in ('8120025c','81200260')]+[
                r for r in nw if r[1]=='cpu-translated-write' and r[8] in
                ('812003a0','812003a4','812003a8','812003ac','812003b0','812003b4')]:
        stores.append(dict(instruction_pc=int(row[8],16),logical_address=int(row[4],16),
                           physical_address=int(row[2],16),value=int(row[16],16)))
    require(len(states)==22 and len(stores)==10, 'finite original reference topology differs')
    return dict(schema='apploader-entry-frames-research-42',profile='conditional-source-only',
        native_admitted=False,falsification=False,completed_source_instructions=11,
        handler=int(e[0]['handler'],16),report=int(e[0]['report'],16),source_entry=snapshot(e[0]),
        before_nested=snapshot(n[0],True),boundary=states[-1]['state'],states=states,stores=stores)


def validate_entry_native42(actual,expected,raw_inputs):
    import hashlib
    require(type(raw_inputs) is dict and set(raw_inputs)==set(ENTRY_ORIGINAL_INPUT_PINS42),
            'original input topology differs')
    for name,sha in ENTRY_ORIGINAL_INPUT_PINS42.items():
        require(type(raw_inputs[name]) is bytes and hashlib.sha256(raw_inputs[name]).hexdigest()==sha,
                'raw original input differs: '+name)
    same_json(actual,expected)
    return True


def falsify_entry_native42(actual,expected,raw_inputs):
    import copy
    count=0
    def reject(mutant,inputs=raw_inputs):
        nonlocal count
        try:validate_entry_native42(mutant,expected,inputs)
        except (ValueError,TypeError,KeyError):
            count+=1
            return
        raise ValueError('native frame mutation admitted')
    # Every leaf: value/type mutation; every container: key/arity mutation.
    # Mutants are submitted to exactly the same reusable validator as success.
    def walk(node,path):
        yield path,node
        if type(node) is dict:
            for key,value in node.items():yield from walk(value,path+[key])
        elif type(node) is list:
            for key,value in enumerate(node):yield from walk(value,path+[key])
    def change(path,value):
        mutant=copy.deepcopy(actual)
        parent=mutant
        for key in path[:-1]:parent=parent[key]
        parent[path[-1]]=value
        reject(mutant)
    for path,value in list(walk(actual,[])):
        if not path:continue
        if type(value) is int:
            change(path,value^1)
            change(path,bool(value)) # False/True cannot masquerade as integer
            change(path,float(value))
        elif type(value) is bool:
            change(path,not value);change(path,int(value))
        elif type(value) is str:
            change(path,('0' if value[:1]!='0' else '1')+value[1:]);change(path,None)
        elif type(value) is dict:
            mutant=copy.deepcopy(actual);parent=mutant
            for key in path:parent=parent[key]
            parent['unexplained_field']=0;reject(mutant)
            for omitted in value:
                mutant=copy.deepcopy(actual);parent=mutant
                for key in path:parent=parent[key]
                del parent[omitted];reject(mutant)
        elif type(value) is list:
            change(path,value[:-1]);change(path,value+[None])
    reject({**actual,'unexplained_top_level':0})
    for key in actual:
        mutant=copy.deepcopy(actual);del mutant[key];reject(mutant)
    # Omitted/reordered zero-valued spill records are effects, not no-ops.
    for i in range(len(actual['stores'])):
        mutant=copy.deepcopy(actual);del mutant['stores'][i];reject(mutant)
    mutant=copy.deepcopy(actual);mutant['stores'].reverse();reject(mutant)
    raw_pc=(0x81200258,0x8120025c,0x81200260,0x81200264,0x8120039c,0x812003a0,
            0x812003a4,0x812003a8,0x812003ac,0x812003b0,0x812003b4,0x812003b8)
    for pc in raw_pc:
        b=bytearray(raw_inputs['apploader.img']);b[32+pc-0x81200000+3]^=1
        reject(actual,{**raw_inputs,'apploader.img':bytes(b)})
    for name,offset in (('apploader.img',0x13),('boot.bin',3),('bi2.bin',0x1b)):
        b=bytearray(raw_inputs[name]);b[offset]^=1
        reject(actual,{**raw_inputs,name:bytes(b)})
    return count


ENTRY_NATIVE_EXE_SHA42 = '07075d9cbe4440e9d78e5da10c0227bb37a893304489f1905cc0840702c9096b'
INDEPENDENT_REAUDIT_SHA42 = 'fa1facaa4e7ed8ce37572a5559ad44ee6658016e8a7abb3c1e189431293d7175'
ENTRY_EXTRA_SOURCE_PINS42 = {
    'PowerPC.h':'74beb71dd8f8246ace89a463914eb2290faa6b309deb8f22059b8ed92b5fd675',
    'ConditionRegister.h':'72c2aa97de7a70ee6954dab3a5718602ca0ecfc592c7d0155eaf9f690b8edfb2',
    'Gekko.h':'fad9f1cbb274f63955b44680627bfab542e1940b24b6f6625ee1653436a6020c',
}


def compare_native(a):
    require(digest(a.entry)==ENTRY_CAPTURE_SHA42 and digest(a.nested)==NESTED_CAPTURE_SHA42,
            'immutable original capture identity differs')
    require(digest(a.auditor_summary)==INDEPENDENT_SUMMARY_SHA42, 'independent accepted summary differs')
    require(digest(a.archived_auditor)==INDEPENDENT_AUDITOR_SHA42, 'archived independent auditor differs')
    require(digest(a.reaudit_summary)==INDEPENDENT_REAUDIT_SHA42, 'archived independent re-audit differs')
    summary=strict_json(a.auditor_summary)
    reaudit=strict_json(a.reaudit_summary)
    for key,rows,fields,rejects in (('entry',8,224,237),('nested',18,612,628)):
        require(type(reaudit[key]['native_production_admission']) is bool
                and reaudit[key]['native_production_admission'] is False
                and reaudit[key]['state_rows']==rows and reaudit[key]['state_fields']==fields
                and reaudit[key]['mutation_rejections']==rejects, 'archived re-audit finite scope differs')
    require(type(summary['native_production_admission']) is bool
            and not summary['native_production_admission'], 'receipt incorrectly promotes production')
    require(summary['entry']['capture_sha256']==ENTRY_CAPTURE_SHA42
            and summary['nested']['capture_sha256']==NESTED_CAPTURE_SHA42, 'auditor receipt identities differ')
    # Re-read the original TUs/object/library/recipe bindings and every original
    # row; don't accept a frozen JSON manifest as a replacement for its inputs.
    check(a.entry);check(a.nested)
    for name,sha in ENTRY_NATIVE_SOURCE_PINS42.items():
        require(digest(a.candidate_root/name)==sha, 'reviewed C++ source differs: '+name)
    ppc=Path(strict_json(a.entry)['observer_manifest']['original_source']).parents[1]
    for name,sha in ENTRY_EXTRA_SOURCE_PINS42.items():
        require(digest(ppc/name)==sha, 'unreceipted retained-field source producer differs: '+name)
    raw_inputs={name:(a.fixtures/name).read_bytes() for name in ENTRY_ORIGINAL_INPUT_PINS42}
    require(digest(a.executable)==ENTRY_NATIVE_EXE_SHA42, 'reviewed MSVC frame executable differs')
    owned_output(a.output)
    cpp_output=a.output.with_suffix('.cpp-run.json')
    require(not a.output.exists() and not cpp_output.exists(), 'comparison evidence already exists')
    command=[str(a.executable),str(a.fixtures/'main.dol'),'--output',str(cpp_output)]
    run=subprocess.run(command,check=True,capture_output=True,text=True)
    # Re-execute the pinned candidate with original-byte locations only.
    # Neither original receipt nor a observed scalar appears in its arguments.
    require(cpp_output.exists(), 'compiled candidate emitted no output')
    same_json(strict_json(a.native),strict_json(cpp_output))
    expected=expected_entry_native42(a.entry,a.nested)
    actual=strict_json(a.native)
    validate_entry_native42(actual,expected,raw_inputs)
    mutations=falsify_entry_native42(actual,expected,raw_inputs) if a.self_test else 0
    result=dict(scope='conditional research frame snapshots/stores; no native clock/event admission',
        schema='entry-native-comparison-42',snapshot_rows=len(expected['states']),ordered_stores=len(expected['stores']),
        native_json_sha256=digest(a.native),native_executable_sha256=digest(a.executable),
        reviewed_source_sha256=ENTRY_NATIVE_SOURCE_PINS42,entry_receipt_sha256=ENTRY_CAPTURE_SHA42,
        nested_receipt_sha256=NESTED_CAPTURE_SHA42,independent_summary_sha256=INDEPENDENT_SUMMARY_SHA42,
        archived_independent_auditor_sha256=INDEPENDENT_AUDITOR_SHA42,
        archived_reaudit_sha256=INDEPENDENT_REAUDIT_SHA42,extra_original_source_sha256=ENTRY_EXTRA_SOURCE_PINS42,
        actual_native_command=command,rerun_json_sha256=digest(cpp_output),native_test_stdout=run.stdout.strip(),
        original_inputs_sha256=ENTRY_ORIGINAL_INPUT_PINS42,validator_sha256=digest(Path(__file__)),
        scalar_type_key_arity_mutation_rejections=mutations,native_production_admission=False,
        compiler_link_provenance='coordinator/author build receipt is separate; executable hash alone is not a build proof')
    owned_output(a.output)
    require(not a.output.exists(), 'native comparison output already exists; preserve old proof')
    a.output.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(f'PASS finite native research frame parity:22 rows/10 stores; {mutations} reusable-validator mutation rejects')
    return result



def main():
    p = argparse.ArgumentParser(description=__doc__); sub=p.add_subparsers(dest='mode',required=True)
    b=sub.add_parser('build')
    for n in ('source','base','output','msvc','sdk-root'):
        b.add_argument('--'+n,type=lambda x:Path(x).resolve(),required=True)
    b.add_argument('--sdk-version',required=True)
    b.add_argument('--scope',choices=('entry','nested'),default='entry')
    c=sub.add_parser('capture')
    for n in ('dolphin','disc','user-dir','output'):
        c.add_argument('--'+n,type=lambda x:Path(x).resolve(),required=True)
    c.add_argument('--port',type=int,required=True)
    v=sub.add_parser('check'); v.add_argument('path',type=Path)
    n=sub.add_parser('compare-native')
    for name in ('native','entry','nested','auditor-summary','archived-auditor','reaudit-summary','candidate-root','fixtures','executable','output'):
        n.add_argument('--'+name,type=lambda x:Path(x).resolve(),required=True)
    n.add_argument('--self-test',action='store_true')
    a=p.parse_args()
    if a.mode=='build': build(a)
    elif a.mode=='capture': capture(a)
    elif a.mode=='compare-native': compare_native(a)
    else: check(a.path)


if __name__=='__main__': main()

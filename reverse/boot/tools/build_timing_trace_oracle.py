"""Private passive prefix observer; no instruction fetch, mutation or native dependency.

Compile one copied Interpreter TU against the matching read-only oracle build.
The existing pure timing GDB observer is generated first. Original CoreTiming,
clock producers, op tables, source, library and game inputs remain unchanged.
"""
import argparse
import json
import os
from pathlib import Path
from build_readonly_boot_oracle import build as base_build, digest, response_run, tlog_command

OBSERVER = r'''
#include <cstdio>
#include <cstdlib>
#include <bit>
namespace {
struct ShadowTimingObserver {
  FILE* file=nullptr; bool active=false, finished=false; u64 ordinal=0;
  ShadowTimingObserver() { if(const char* path=std::getenv("SHADOW_TIMING_PREFIX_TRACE")) file=std::fopen(path,"wx"); }
  ~ShadowTimingObserver() { if(file)std::fclose(file); }
};
void ShadowTimingObserve(Core::System& system,const char* kind,u32 pc,u32 word,u32 cost,u32 detail) {
  static ShadowTimingObserver observer;
  if(!observer.file||observer.finished)return;
  if(pc==0x80003154)observer.active=true;
  if(!observer.active)return;
  auto& s=system.GetPPCState();auto& timing=system.GetCoreTiming();auto& g=timing.GetGlobals();
  const u64 ticks=timing.GetTicks(); // existing pure getter; never Advance/read opcode/TB
  std::fprintf(observer.file,"%llu %s %08x %08x %08x %08x %08x %08x %016llx %016llx %016llx %08x %08x %08x %08x\n",
      ++observer.ordinal,kind,pc,word,cost,detail,s.pc,s.npc,ticks,u64(g.global_timer),u64(g.slice_length),
      u32(s.downcount),std::bit_cast<u32>(g.last_OC_factor_inverted),s.msr.Hex,s.Exceptions);
  if(pc==0x80373ac0 && std::string_view(kind)=="exit") { OracleEventAudit::Record("timing-prefix-end","-",{s.pc,ticks,u64(g.global_timer),u64(g.slice_length),u64(s.downcount),s.msr.Hex,s.Exceptions}); std::fflush(observer.file); observer.finished=true; }
}
}
'''


def observe_source(text):
    include='#include "Core/System.h"'
    if text.count(include)!=1:raise ValueError('Interpreter include identity differs')
    result=text.replace(include,include+OBSERVER,1)
    hook='      HLE::TryReplaceFunction(m_ppc_symbol_db, address, PowerPC::CoreMode::Interpreter);\n  if (!result)'
    if result.count(hook)!=1:raise ValueError('HLE result anchor differs')
    result=result.replace(hook,hook.replace('  if (!result)','  ShadowTimingObserve(m_system,"hook",address,0,0,(u32(result.type)<<24)|result.hook_index);\n  if (!result)'),1)
    start='int Interpreter::SingleStepInner()\n{\n  OraclePhysicalWrites::Checkpoint'
    if result.count(start)!=1:raise ValueError('instruction entry anchor differs')
    result=result.replace(start,start.replace('  OraclePhysicalWrites::Checkpoint','  const u32 shadow_pc=m_ppc_state.pc;\n  OraclePhysicalWrites::Checkpoint'),1)
    hret='return PPCTables::GetOpInfo(m_prev_inst, m_ppc_state.pc)->num_cycles;'
    if result.count(hret)!=1:raise ValueError('HLE return cost anchor differs')
    result=result.replace(hret,'const auto shadow_cost=PPCTables::GetOpInfo(m_prev_inst,m_ppc_state.pc)->num_cycles;\n    ShadowTimingObserve(m_system,"replace",shadow_pc,m_prev_inst.hex,shadow_cost,m_end_block);\n    return shadow_cost;',1)
    op='const GekkoOPInfo* opinfo = PPCTables::GetOpInfo(m_prev_inst, m_ppc_state.pc);'
    if result.count(op)!=1:raise ValueError('already-fetched opcode anchor differs')
    result=result.replace(op,op+'\n  ShadowTimingObserve(m_system,"enter",shadow_pc,m_prev_inst.hex,opinfo->num_cycles,m_end_block);',1)
    end='  return opinfo->num_cycles;'
    if result.count(end)!=1:raise ValueError('ordinary return anchor differs')
    result=result.replace(end,'  ShadowTimingObserve(m_system,"exit",shadow_pc,m_prev_inst.hex,opinfo->num_cycles,m_end_block);\n'+end,1)
    return result


def build(source,original_build,output,msvc,sdk_root,sdk_version):
    base_build(source,original_build,output,msvc,sdk_root,sdk_version,timing=True)
    original=source/'Source/Core/Core/PowerPC/Interpreter/Interpreter.cpp'
    copied=output/'Interpreter.cpp'
    copied.write_text(observe_source(original.read_text(encoding='utf-8')),encoding='utf-8')
    tools=msvc/'bin/Hostx64/x64'
    env=dict(os.environ,TMP=str(output/'tmp'),TEMP=str(output/'tmp'))
    env['PATH']=str(tools)+';'+str(sdk_root/'bin'/sdk_version/'x64')+';'+env.get('PATH','')
    env['INCLUDE']=';'.join(map(str,[msvc/'include']+[sdk_root/'Include'/sdk_version/n for n in ('ucrt','shared','um','winrt')]))
    env['LIB']=';'.join(map(str,[msvc/'lib/x64']+[sdk_root/'Lib'/sdk_version/n/'x64' for n in ('ucrt','um')]))
    args=tlog_command(original_build/'Source/Core/Core/core.dir/Release/core.tlog/CL.command.1.tlog','INTERPRETER.CPP')
    args=[a for a in args if not a.lower().startswith(('/fo','/fd')) and not a.upper().endswith('INTERPRETER.CPP')]
    obj=output/'Interpreter.obj';args+=['/Fo'+str(obj),'/Fd'+str(output/'Interpreter.pdb'),str(copied)]
    response_run('cl',args,output,env,tools)
    # Prepend the replacement object before the archive; no original object/source changed.
    from build_readonly_boot_oracle import command_args
    link=command_args((output/'link.rsp').read_text(encoding='utf-16'))
    response_run('link',[str(obj)]+link,output,env,tools)
    manifest_path=output/'manifest.json';manifest=json.loads(manifest_path.read_text(encoding='utf-8'))
    manifest.update(purpose='passive original prefix timing/HLE observer; no native dependency',instrumented_executable_sha256=digest(output/'Dolphin.exe'),
        passive_interpreter=dict(original_sha256=digest(original),copied_sha256=digest(copied),object_sha256=digest(obj),
        mutation='observation only; original fetched word/cost/result; no extra opcode fetch or state writer',
        trace_environment='SHADOW_TIMING_PREFIX_TRACE',start='80003154',stop_after='80373ac0',fields='ordinal kind pc word cost detail resulting_pc npc ticks global_timer slice downcount OC_inverse MSR Exceptions'))
    manifest_path.write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    print('passive timing observer',manifest['instrumented_executable_sha256'])

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for n in ('source','original_build','output'):parser.add_argument(n,type=lambda s:Path(s).resolve())
    for n in ('msvc','sdk_root'):parser.add_argument('--'+n.replace('_','-'),type=lambda s:Path(s).resolve(),required=True)
    parser.add_argument('--sdk-version',required=True)
    build(**vars(parser.parse_args()))

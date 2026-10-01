"""Strict clock RESEARCH parity; never advances the connected boot frontier.

The producer epoch is reconstructed from observed RTC seconds and frequency.
The explicit frontier cycle observation and GDB SingleStep schedule remain
research inputs, not a native runtime timing provider. No TB/register output
is used as a projection input. Prior connected prefix validation runs first.
"""
import argparse
import copy
import json
from pathlib import Path
import subprocess
from boot_state_diff import FIELD_NAMES, compare_fields, compare_memory, hex_field, parse_native_state, reference_fields
from capture_bi2_state import gpr, SDA
from capture_clock_state import CLOCK_GLOBALS, LOW_WORDS, GETTERS
from capture_dolphin_rsp import sha256
from validate_native_bi2 import validate as validate_bi2
from agent_clock_adversarial_semantics import REGIONS
from machine import DolImage

U64=(1<<64)-1

def counter(origin,epoch,cycles):return (origin+((cycles-epoch)&U64)//12)&U64

def ordered_clock_plan(origin,epoch,cycles,enabled):
    order=[]
    for _ in range(16):
        first=counter(origin,epoch,cycles)>>32
        second=counter(origin,epoch,cycles+2)>>32
        order += [0x8037962C,0x80379630,0x80379634,0x80379638,
                  0x80379628 if first!=second else 0x8037963C]
        cycles+=5
        if first==second:break
    else:raise ValueError('source-derived retry unbounded')
    order += [0x8037966C]+list(range(0x80379670,0x80379688,4))+[0x80376144,0x80376148,0x8037614C]
    order += [0x80376150,0x80376154,0x8037615C] if enabled else [0x80376158,0x8037615C]
    order += [0x80376160,0x80376164,0x80379688]+list(range(0x8037968C,0x803796AC,4))
    return order+[0x80370EA8,0x80370EAC,0x80370EB0,0x8037611C,0x80376120,0x80376124,
                  0x80376128,0x8037612C,0x80370EB4,0x80370EB8,0x80373AB4,
                  0x80373AB8,0x80373ABC,0x80373AC0,0x80373AC4]

def checked_epoch_controls(report):
    """Only explicit original-entry producer controls can admit an arbitrary epoch."""
    ext=report['region_extension'];mode=ext.get('producer_mode','rtc_initial')
    entry=report['checkpoints'][0]['clock_producer']
    original=report['original_unmodified_entry']['clock_producer']
    if mode=='rtc_initial':
        if ext.get('epoch_controls') or ext.get('requested_epoch') is not None:raise ValueError('unlabelled epoch forcing')
        if any(entry[k].lower()!=original[k].lower() for _,k,_ in GETTERS):raise ValueError('unexplained entry clock change')
        return None
    if mode!='preentry_rebased':raise ValueError('unknown clock producer mode')
    manifest=report['instrumentation_manifest'].get('clock_perturbations',{})
    if manifest.get('enabled') is not True or manifest.get('allowed_pc')!='80003154':raise ValueError('clock control executable provenance missing')
    requested=hex_field(ext['requested_epoch'],16,'requested entry epoch')
    controls=ext['epoch_controls']
    if len(controls)!=3 or report['original_unmodified_entry']['pc'].lower()!='80003154' or report['checkpoints'][0]['pc'].lower()!='80003154':raise ValueError('clock control entry/sequence missing')
    old=hex_field(original['epoch_value'],16,'original epoch');expected=dict(original)
    for effect,reg,value in zip(controls,(250,251,252),(requested&0xffffffff,requested>>32,1)):
        if effect['pc'].lower()!='80003154' or effect['packet'].lower()!=f'P{reg:x}={value:08x}'.lower():raise ValueError('clock controls not original-entry ordered packets')
        expected['epoch_value']=f'{((old&0xffffffff00000000)|value) if reg==250 else requested:016x}'
        if reg==252:
            expected['epoch_cycles']=expected['cycles'];expected['cached_tb']=expected['epoch_value']
        actual=effect['readback']
        if set(actual)!=set(expected):raise ValueError('clock control readback shape differs')
        for _,k,w in GETTERS:
            if hex_field(actual[k],w,'control '+k)!=hex_field(expected[k],w,'expected '+k):raise ValueError('clock control changed unexplained state')
    if any(entry[k].lower()!=expected[k].lower() for _,k,_ in GETTERS):raise ValueError('committed clock entry differs')
    return requested


def checked_report(report,image):
    ext=report['region_extension']
    if ext['name']!='clock_research_41' or ext['midchain_writes']:raise ValueError('wrong/forced clock capture')
    if ext.get('sampler_mode','single_step')!='single_step':raise ValueError('continuous timing is not a single-step producer')
    states=report['checkpoints']
    cut=next(i for i,s in enumerate(states) if s['pc'].lower()=='80379628')+1
    start=states[cut-1];producer=start['clock_producer']
    for reg,key,width in GETTERS:hex_field(producer[key],width,key)
    if hex_field(producer['exceptions'],8,'pending exceptions'):raise ValueError('pending exception unsupported')
    hz=hex_field(producer['cpu_hz'],8,'cpu frequency')
    if hz!=486000000:raise ValueError('clock domain unsupported')
    rebased=checked_epoch_controls(report)
    origin=hex_field(producer['epoch_value'],16,'clock epoch');seconds,phase=divmod(origin,hz//12)
    if rebased is None and (phase or seconds>0xffffffff):raise ValueError('RTC epoch producer not reconstructed')
    if rebased is not None and origin!=rebased:raise ValueError('pre-entry epoch changed')
    epoch=hex_field(producer['epoch_cycles'],16,'epoch cycle');cycles=hex_field(producer['cycles'],16,'frontier cycles')
    if not epoch<=cycles<=U64-128:raise ValueError('cycle interval unexplained')
    original=report['original_unmodified_entry'];entry=states[0]
    words=list(original['clock_low_words'])
    if len(words)!=5:raise ValueError('clock low-memory observations incomplete')
    for w in words:hex_field(w,8,'low-memory clock word')
    seen=set()
    for effect in ext['clock_preentry_writes']:
        a=hex_field(effect['address'],8,'offset address');v=hex_field(effect['value'],8,'offset value')
        if a not in LOW_WORDS[:2] or a in seen or hex_field(effect['readback'],8,'offset readback')!=v:raise ValueError('unproved offset control')
        seen.add(a);words[LOW_WORDS.index(a)]=f'{v:08x}'
    if [w.lower() for w in entry['clock_low_words']]!=[w.lower() for w in words]:raise ValueError('offset entry provenance differs')
    tail=states[cut:];order=ordered_clock_plan(origin,epoch,cycles,gpr(start,31))
    if [int(s['pc'],16) for s in tail]!=order or [int(p,16) for p in ext['clock_plan']]!=order:raise ValueError('clock occurrence CFG differs')
    if ext['stop'].lower()!='80373ac4':raise ValueError('next unexplained pointer not preserved')
    pauses=ext['pause_samples']
    if len(pauses)!=2 or pauses[0]!=pauses[1] or pauses[0]!=producer:raise ValueError('clock pause/source snapshot differs')
    for i,s in enumerate(states):
        reference_fields(s)
        p=s['clock_producer']
        for _,key,width in GETTERS:hex_field(p[key],width,key)
        if any(p[k].lower()!=producer[k].lower() for k in ('epoch_value','epoch_cycles','cpu_hz','rtc_offset')):raise ValueError('unexplained clock rebase/frequency/RTC change')
        if hex_field(p['exceptions'],8,'exceptions'):raise ValueError('unexplained pending exception')
        if [w.lower() for w in s['clock_low_words']]!=[w.lower() for w in words]:raise ValueError('unexplained low-memory clock writer')
        if len(s['clock_globals'])!=2:raise ValueError('clock global shape differs')
        for w in s['clock_globals']:hex_field(w,8,'clock global')
        if i<cut and any(int(w,16) for w in s['clock_globals']):raise ValueError('clock globals not produced by prior CRT')
        if image.word(int(s['pc'],16))!=hex_field(s['instruction_word'],8,'raw instruction'):raise ValueError('clock raw bytes differ')
    for base,values in REGIONS.items():
        if any(image.word(base+4*n)!=w for n,w in enumerate(values)):raise ValueError('clock raw gate differs')
    for i,s in enumerate(tail):
        if int(s['clock_producer']['cycles'],16)!=cycles+i+1:raise ValueError('GDB step producer schedule differs')
    prefix=copy.deepcopy(report);prefix['checkpoints']=states[:cut]
    prefix['region_extension'].update(name='bi2_os_40',stop='80379628')
    return prefix,tail,order,start,seconds,hz,epoch,cycles,(int(words[0],16)<<32)|int(words[1],16)

def validate(dol,bi2,program,bi2_program,crt_program,l2_program,capture,work):
    root=Path(__file__).resolve().parents[3]/'build'
    if not work.resolve().is_relative_to(root.resolve()):raise ValueError('research outputs outside build')
    image=DolImage(dol);report=json.loads(capture.read_text(encoding='utf-8'))
    prefix,tail,order,start,seconds,hz,epoch,cycles,offset=checked_report(report,image)
    work.mkdir(parents=True,exist_ok=True)
    projection=work/(capture.stem+'.bi2-reference.json');projection.write_text(json.dumps(prefix,indent=2)+'\n',encoding='utf-8')
    previous=validate_bi2(dol,bi2,bi2_program,crt_program,l2_program,projection,work)
    fixture=work/(projection.stem+'.entry.txt')
    producer=start['clock_producer']
    rebased=checked_epoch_controls(report)
    epoch_arg=f'{seconds:08x}' if rebased is None else f'{rebased:016x}'
    mode='research-only' if rebased is None else 'research-rebased-epoch'
    args=[str(program.resolve()),str(dol.resolve()),str(fixture.resolve()),epoch_arg,f'{hz:08x}',f'{epoch:016x}',
          f'{cycles:016x}',f'{offset:016x}',producer['cached_tb'],producer['exceptions'],mode]
    output=subprocess.run(args,capture_output=True,text=True,check=True).stdout
    (work/(capture.stem+'.research.txt')).write_text(output,encoding='ascii')
    lines=output.splitlines();native=[s for s in lines if s[:1].isdigit()];clocks=[s.split() for s in lines if s.startswith('CLOCK_RESEARCH ')]
    if len(native)!=len(order) or len(clocks)!=len(order):raise ValueError('clock native shape/occurrence count differs')
    # Independent word effect timeline, derived from entry and producer inputs.
    consumed=counter(int(producer['epoch_value'],16),epoch,cycles+1)
    # If a high rollover retried, determine the successful low sample's cycle.
    sample_cycles=cycles
    while counter(int(producer['epoch_value'],16),epoch,sample_cycles)>>32 != counter(int(producer['epoch_value'],16),epoch,sample_cycles+2)>>32:sample_cycles+=5
    consumed=counter(int(producer['epoch_value'],16),epoch,sample_cycles+1)
    total=(consumed+offset)&U64
    events=[[0x80370EA8,0x805F1F54,total&0xffffffff,total&0xffffffff],
            [0x80370EAC,0x805F1F50,total>>32,total>>32],
            [0x80373AB8,0x8060C5D4,0x80370EBC,0x80370EBC],
            [0x80373ABC,0x8060C5C8,0x8060C5D0,0x8060C5D0]]
    actual_events=[]
    for line in lines:
        if line.startswith('BOOT_STORE '):
            row=line.split()
            if len(row)!=5:raise ValueError('clock word effect shape')
            actual_events.append([hex_field(v,8,'clock effect') for v in row[1:]])
    if actual_events!=events:raise ValueError('clock word effect order/value differs')
    previous_lines=(work/(projection.stem+'.native.txt')).read_text().splitlines()
    prior=parse_native_state([s for s in previous_lines if s[:1].isdigit()][-1])
    known={0x8060C570+n:b for n,b in enumerate(bytes.fromhex(prior[116])) if prior[117][n]=='1'}
    global_pair=[0,0];pending={0x80370EAC:[events[0]],0x80370EB0:[events[1]],0x80373ABC:[events[2]],0x80373AC0:[events[3]]}
    fields=memory=clock_fields=0;cached=int(producer['cached_tb'],16);before_pc=0x80379628;before_cycles=cycles
    for i,(line,s,pc,clock) in enumerate(zip(native,tail,order,clocks)):
        for _,a,v,_ in pending.get(pc,[]):
            if a in CLOCK_GLOBALS:global_pair[CLOCK_GLOBALS.index(a)]=v
            else:
                for n,b in enumerate(v.to_bytes(4,'big')):known[a+n]=b
        actual=parse_native_state(line);expected=reference_fields(s);label=f'clock research {pc:08X} occurrence{i}'
        fields+=compare_fields(actual[:113],expected,FIELD_NAMES,label)
        fields+=compare_fields([actual[115],actual[118],actual[119]],[s['l2cr'],s['handler_slot'],s['lowmem_44']],['L2CR','handler','low44'],label)
        bits=''.join('1' if 0x8060C570+n in known else '0' for n in range(144))
        memory+=compare_memory(actual[116],actual[117],s['l2_stack_bytes'],bits,label,0x8060C570)
        memory+=compare_memory(actual[113],actual[114],s['paired_stack_bytes'],bits[120:136],label,0x8060C5E8)
        if before_pc in (0x80379628,0x8037962C,0x80379630):cached=counter(int(producer['epoch_value'],16),epoch,before_cycles)
        if len(clock)!=6:raise ValueError('clock effect observation shape')
        clock_fields+=compare_fields(clock[1:2],[f'{pc:08x}'],['PC'],label)
        if hex_field(clock[2],16,'source cycles')!=cycles+i+1 or hex_field(clock[3],16,'cached TB')!=cached or int(s['clock_producer']['cached_tb'],16)!=cached:raise ValueError('clock producer/sample phase mismatch')
        clock_fields+=compare_fields(clock[4:],[f'{v:08x}' for v in global_pair],['high global','low global'],label)
        clock_fields+=compare_fields(s['clock_globals'],[f'{v:08x}' for v in global_pair],['high global','low global'],label)
        # No additional BI2/SDA writes occur in the clock tail.
        if s['bi2_blob']!=start['bi2_blob'] or s['bi2_globals']!=start['bi2_globals']:raise ValueError('clock unexpected owned-byte write')
        before_pc=pc;before_cycles+=1
    current=[s.split() for s in lines if s.startswith('RESEARCH_GLOBAL_RANGE ')]
    old=[s.split() for s in previous_lines if s.startswith('CURRENT_GLOBAL_RANGE ')]
    if len(current)!=len(old):raise ValueError('current owners missing')
    for row,previous_row in zip(current,old):
        if len(row)!=4 or row[1:3]!=previous_row[1:3]:raise ValueError('current owner relabelled')
        a=int(row[1],16);expected=bytearray.fromhex(previous_row[3]);n=int(row[2],16)
        for address,value in zip(CLOCK_GLOBALS,global_pair):
            if a<=address<a+n:expected[address-a:address-a+4]=value.to_bytes(4,'big')
        if row[3]!=expected.hex():raise ValueError('current clock owner differs')
    if lines[-1]!='RESEARCH_STOP pc=0x80373ac4 reason=NO_NATIVE_CLOCK_PROVIDER':raise ValueError('research output falsely advances boot')
    summary=dict(scope='research SingleStep projection only; production frontier remains80379628',capture_sha256=sha256(capture),
                 executable_sha256=sha256(program),checkpoint_count=len(order),state_fields=fields,known_memory_bytes=memory,
                 clock_fields=clock_fields,source_samples=3*sum(pc==0x8037962C for pc in order),producer_mode=report['region_extension'].get('producer_mode','rtc_initial'),word_effects=len(events),prefix=previous,stop='80373ac4')
    (work/(capture.stem+'.validation.json')).write_text(json.dumps(summary,indent=2)+'\n',encoding='utf-8')
    print(f'{capture.name}: RESEARCH {len(order)} CP/{fields} fields/{memory} bytes; production remains80379628')
    return summary

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    for n in ('dol','bi2','program','bi2_program','crt_program','l2_program','capture','work'):p.add_argument(n,type=Path)
    validate(**vars(p.parse_args()))

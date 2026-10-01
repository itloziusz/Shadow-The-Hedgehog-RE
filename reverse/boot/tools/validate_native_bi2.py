"""Connected BI2/OS gate: original live blob, pre-entry controls, exact states.

The earlier CRT replay is independently rerun first. No intermediate or
expected clock/guard/relocation result is passed to the new native backend.
"""
import argparse
import copy
import json
from pathlib import Path
import subprocess
from boot_state_diff import FIELD_NAMES, compare_fields, compare_memory, hex_field, parse_native_state, reference_fields
from capture_bi2_state import SDA, gpr
from capture_crt_state import ZERO_RANGES
from capture_dolphin_rsp import sha256
from validate_native_crt import DolImage, validate as validate_crt

BI2_SHA='8bbadffc2a7f79eda5982aa2fe30831969c2702aa9206f621b930aae0b70af6b'


def word(blob, offset):
    if offset&3 or offset<0 or offset+4>len(blob):raise ValueError('BI2 word outside owned blob')
    return int.from_bytes(blob[offset:offset+4],'big')


def plan(blob):
    debug=word(blob,12)
    result=[0x80003194,0x800031BC,0x800031C4]
    if debug in (2,3):
        if debug==3:result += [0x800031D0]
        return result+[0x800031E8,0x800031F4]
    result += [0x800031D0,0x800031D8]
    if debug==4:result += [0x80003140,0x80003148,0x800031E4]
    result += [0x800031F8,0x80003200,0x8000320C,0x80003214]
    offset=word(blob,8)
    if offset:
        count=word(blob,offset)
        if offset+4+4*count>len(blob):raise ValueError('incomplete relocation extent')
        result += [0x8000321C,0x80003224]
        if count:
            for _ in range(count):result += [0x80003230,0x80003234,0x8000323C,0x80003240]
            result += [0x80003250]
        else:result += [0x80003258]
    else:result += [0x80003258]
    return result+[0x80003260,0x80370BF0,0x80370BFC,0x80370C08,0x80370C10,
                   0x80003264,0x80370E68,0x80370E80,0x80370E98,0x80370EA0,
                   0x80379648,0x80379660,0x8037611C,0x80376124,0x80379664,0x80379628]


def checked_input(report, original_bytes, image):
    ext=report['region_extension']
    if ext['name']!='bi2_os_40' or ext['midchain_writes']:raise ValueError('wrong/forced BI2 experiment')
    pointer=hex_field(ext['bi2_pointer'],8,'BI2 mapping')
    if pointer&3 or not 0x80000000<=pointer<=0x817FE000:raise ValueError('BI2 mapping bounds differ')
    original=report['original_unmodified_entry']
    if hex_field(original['bi2_pointer'],8,'original BI2 pointer')!=pointer:raise ValueError('pointer origin differs')
    hex_field(original['bi2_blob'],0x4000,'original loaded BI2')
    if bytes.fromhex(original['bi2_blob'])!=original_bytes:raise ValueError('live original BI2 is not original disc bytes')
    expected=bytearray(original_bytes);addresses=set()
    for effect in ext['bi2_preentry_writes']:
        address=hex_field(effect['address'],8,'BI2 seed address')
        value=hex_field(effect['value'],8,'BI2 seed value')
        if hex_field(effect['readback'],8,'BI2 seed readback')!=value:raise ValueError('BI2 seed readback differs')
        if address in addresses or address&3 or address<pointer or address+4>pointer+len(expected):raise ValueError('BI2 seed ownership/duplicate differs')
        addresses.add(address);expected[address-pointer:address-pointer+4]=value.to_bytes(4,'big')
    entry=report['checkpoints'][0]
    hex_field(entry['bi2_blob'],0x4000,'entry BI2 blob')
    if bytes.fromhex(entry['bi2_blob'])!=expected:raise ValueError('entry bytes lack original/write provenance')
    cut=next(i for i,s in enumerate(report['checkpoints']) if hex_field(s['pc'],8,'PC')==0x80003188)+1
    prefix=copy.deepcopy(report);prefix['checkpoints']=prefix['checkpoints'][:cut]
    prefix['region_extension']['name']='handler_crt_39';prefix['region_extension']['stop']='80003188'
    tail=report['checkpoints'][cut:];order=plan(expected)
    if [hex_field(s['pc'],8,'PC') for s in tail]!=order:raise ValueError('BI2 CFG occurrence order differs')
    if hex_field(ext['stop'],8,'stop')!=order[-1]:raise ValueError('BI2 declared stop differs')
    for index,state in enumerate(report['checkpoints']):
        reference_fields(state)
        if hex_field(state['bi2_pointer'],8,'live BI2 pointer')!=pointer:raise ValueError('unexplained pointer writer')
        hex_field(state['bi2_blob'],0x4000,'live BI2 blob')
        if index<cut and bytes.fromhex(state['bi2_blob'])!=expected:raise ValueError('pre-CRT BI2 mutation unexplained')
        if len(state['bi2_globals'])!=4:raise ValueError('missing/extra SDA observations')
        for value in state['bi2_globals']:hex_field(value,8,'SDA word')
        for key in ('lowmem_34','lowmem_48'):hex_field(state[key],8,key)
        if hex_field(state['instruction_word'],8,'instruction')!=image.word(int(state['pc'],16),text=True):raise ValueError('captured instruction does not match DOL')
        if hex_field(state['l2_stack_address'],8,'stack base')!=0x8060C570:raise ValueError('stack observation relabelled')
        hex_field(state['l2_stack_bytes'],288,'stack bytes');hex_field(state['paired_stack_bytes'],32,'paired bytes')
        if index<cut:
            for key in ('lowmem_34','lowmem_48'):
                if state[key].lower()!=entry[key].lower():raise ValueError('low-memory mutation before any proved writer')
    return pointer,bytes(expected),prefix,tail,order


def ledger(tail, order, pointer, entry_blob):
    """Stores and application moments from exact opcodes + captured call inputs."""
    events=[];batches={};byte_events=[];byte_batches={}
    def add(after,pc,address,value):
        effect=[pc,address,value,value];events.append(effect);batches.setdefault(after,[]).append(effect)
    for i,pc in enumerate(order):
        state=tail[i]
        if pc in (0x80370E68,0x80379648):
            sp=gpr(state,1);size=24 if pc==0x80370E68 else 32
            add(i+1,pc+4,sp+4,hex_field(state['lr'],8,'saved LR'))
            add(i+1,pc+8,sp-size,sp)
            for n,register in enumerate((31,30,29)):add(i+1,pc+12+4*n,sp-4-4*n,gpr(state,register))
        elif pc==0x80003140:
            effect=[0x80003144,0x805F1FF0,1,1];byte_events.append(effect);byte_batches.setdefault(i+1,[]).append(effect)
        elif pc==0x8000323C:
            # Independent input word/add, not the native result checkpoint.
            address=gpr(state,6);index=(address-pointer-word(entry_blob,8)-4)//4
            value=(word(entry_blob,word(entry_blob,8)+4+4*index)+pointer)&0xFFFFFFFF
            add(i+1,pc,address,value)
        elif pc==0x80003250:add(i+1,pc,0x80000034,gpr(state,15)&0xFFFFFFE0)
        elif pc==0x80370BFC:add(i+1,pc,0x805F1F18,0x80000040)
        elif pc==0x80370C08:add(i+1,pc,0x80000048,0x00370C60)
        elif pc==0x80370C10:add(i+1,pc,0x805F1F1C,1)
        elif pc==0x80370EA0:add(i+1,pc,0x805F1F40,1)
    return events,batches,byte_events,byte_batches


def validate(dol, bi2, program, crt_program, l2_program, capture, work):
    root=Path(__file__).resolve().parents[3]/'build'
    if not work.resolve().is_relative_to(root.resolve()):raise ValueError('outputs outside repository build')
    if sha256(bi2)!=BI2_SHA:raise ValueError('original BI2 identity differs')
    report=json.loads(capture.read_text(encoding='utf-8'));image=DolImage(dol)
    pointer,blob,prefix,tail,order=checked_input(report,bi2.read_bytes(),image)
    work.mkdir(parents=True,exist_ok=True);projection=work/(capture.stem+'.crt-reference.json')
    projection.write_text(json.dumps(prefix,indent=2)+'\n',encoding='utf-8')
    previous=validate_crt(dol,crt_program,l2_program,projection,work)
    prior_fixture=work/(projection.stem+'.entry.txt');fixture=work/(capture.stem+'.entry.txt')
    fixture.write_text(prior_fixture.read_text()+f'{pointer:08x}\n'+blob.hex()+'\n',encoding='ascii')
    output=subprocess.run([str(program.resolve()),str(dol.resolve()),str(fixture.resolve())],capture_output=True,text=True,check=True).stdout
    (work/(capture.stem+'.native.txt')).write_text(output,encoding='ascii');lines=output.splitlines()
    previous_output=(work/(projection.stem+'.native.txt')).read_text()
    prior_states=[line for line in previous_output.splitlines() if line[:1].isdigit()]
    all_states=[line for line in lines if line[:1].isdigit()]
    if all_states[:len(prior_states)]!=prior_states:raise ValueError('previous connected CRT states changed')
    states=all_states[len(prior_states):]
    memory_lines=[line.split() for line in lines if line.startswith('BI2_MEMORY ')]
    global_lines=[line.split() for line in lines if line.startswith('BI2_GLOBALS ')]
    if not len(states)==len(tail)==len(memory_lines)==len(global_lines):raise ValueError('missing/extra typed native checkpoints')
    events,batches,byte_events,byte_batches=ledger(tail,order,pointer,blob)
    prior_effects=[line for line in previous_output.splitlines() if line.startswith('BOOT_STORE ')]
    stores=[line for line in lines if line.startswith('BOOT_STORE ')]
    if stores[:len(prior_effects)]!=prior_effects:raise ValueError('previous applied CRT effects changed')
    for line in stores:
        if len(line.split())!=5:raise ValueError('word effect shape differs')
        for value in line.split()[1:]:hex_field(value,8,'word effect')
    if [[int(v,16) for v in line.split()[1:]] for line in stores[len(prior_effects):]]!=events:raise ValueError('ordered BI2/OS stores differ')
    byte_lines=[line.split() for line in lines if line.startswith('BOOT_BYTE_STORE ')]
    for row in byte_lines:
        if len(row)!=5:raise ValueError('byte effect shape differs')
        for value,width in zip(row[1:],(8,8,2,2)):hex_field(value,width,'byte effect')
    if [[int(v,16) for v in row[1:]] for row in byte_lines]!=byte_events:raise ValueError('byte stores differ')
    known={};last=parse_native_state(prior_states[-1]);raw=bytes.fromhex(last[116])
    for n,bit in enumerate(last[117]):
        if bit=='1':known[0x8060C570+n]=raw[n]
    mutable=bytearray(blob);sda=[0,0,0,0];arena=low48=None
    fields=stack_bytes=bi2_bytes=global_fields=0
    for i,(line,pc,state,mem,globs) in enumerate(zip(states,order,tail,memory_lines,global_lines)):
        actual=parse_native_state(line);label=f'PC {pc:08X} occurrence {i}'
        if len(actual)!=120 or int(actual[0],16)!=pc:raise ValueError('native BI2 CFG differs')
        for _,address,value,_ in batches.get(i,[]):
            if 0x8060C570<=address<0x8060C600:
                for n,b in enumerate(value.to_bytes(4,'big')):known[address+n]=b
            elif pointer<=address<pointer+len(blob):mutable[address-pointer:address-pointer+4]=value.to_bytes(4,'big')
            elif address in SDA:sda[SDA.index(address)]=value
            elif address==0x80000034:arena=value
            elif address==0x80000048:low48=value
            else:raise ValueError('word effect owner unexplained')
        for _,address,value,_ in byte_batches.get(i,[]):
            if address!=SDA[3]:raise ValueError('byte owner unexplained')
            sda[3]=(sda[3]&0x00FFFFFF)|(value<<24)
        fields+=compare_fields(actual[:113]+[actual[115],actual[118],actual[119]],reference_fields(state)+[state['l2cr'],state['handler_slot'],state['lowmem_44']],FIELD_NAMES+['L2CR','handler slot','lowmem44'],label)
        mask=''.join('1' if 0x8060C570+n in known else '0' for n in range(144))
        stack_bytes+=compare_memory(actual[116],actual[117],state['l2_stack_bytes'],mask,label,0x8060C570)
        paired=''.join('1' if 0x8060C5E8+n in known else '0' for n in range(16))
        stack_bytes+=compare_memory(actual[113],actual[114],state['paired_stack_bytes'],paired,label,0x8060C5E8)
        if len(mem)!=4 or hex_field(mem[1],8,'memory PC')!=pc or hex_field(mem[2],8,'mapping')!=pointer:raise ValueError('BI2 native memory mapping differs')
        hex_field(mem[3],0x4000,'native BI2 bytes')
        if bytes.fromhex(mem[3])!=mutable or bytes.fromhex(state['bi2_blob'])!=mutable:raise ValueError(f'BI2 ordered byte delta differs {label}')
        bi2_bytes+=len(blob)
        if len(globs)!=8 or hex_field(globs[1],8,'global PC')!=pc:raise ValueError('SDA trace shape differs')
        expected=[f'{v:08x}' for v in sda]
        global_fields+=compare_fields(globs[2:6],expected,['SDA']*4,label)
        global_fields+=compare_fields(state['bi2_globals'],expected,['SDA']*4,label)
        for key,value,observed,written in (('lowmem_34',globs[6],state['lowmem_34'],arena),('lowmem_48',globs[7],state['lowmem_48'],low48)):
            if written is None:
                if value!='-':raise ValueError('unproduced low-memory word claimed known')
                # The owner is unknown to native because it has not been read;
                # reference still must not show an unexplained prior writer.
                baseline=report['checkpoints'][0][key]
                if observed.lower()!=baseline.lower():raise ValueError('unexplained low-memory mutation before writer')
            else:global_fields+=compare_fields([value,observed],[f'{written:08x}']*2,['low memory']*2,label)
    # Verify the final current storage remains the earlier three fills plus
    # exactly the admitted SDA mutations. No disconnected shadow globals.
    current=[line.split() for line in lines if line.startswith('CURRENT_GLOBAL_RANGE ')]
    if len(current)!=3:raise ValueError('current global owners missing/extra')
    for row,(address,size) in zip(current,ZERO_RANGES):
        if len(row)!=4 or hex_field(row[1],8,'owner address')!=address or hex_field(row[2],8,'owner size')!=size:raise ValueError('current owner shape differs')
        expected=bytearray(size)
        for a,value in zip(SDA,sda):
            if address<=a<address+size:expected[a-address:a-address+4]=value.to_bytes(4,'big')
        hex_field(row[3],size*2,'current owner bytes')
        if bytes.fromhex(row[3])!=expected:raise ValueError('current SDA/storage disagreement')
    reason='CONTEXT_TRANSFER_UNVALIDATED' if order[-1]==0x800031F4 else 'LIVE_TIME_BASE_UNRESOLVED'
    if lines[-1]!=f'STOP pc=0x{order[-1]:08x} reason={reason}':raise ValueError('wrong final fail-closed stop')
    summary=dict(capture_sha256=sha256(capture),native_executable_sha256=sha256(program),entry_fixture_sha256=sha256(fixture),
                 new_checkpoints=len(tail),new_state_fields=fields,known_stack_bytes=stack_bytes,bi2_bytes=bi2_bytes,
                 global_comparisons=global_fields,ordered_word_stores=len(events),ordered_byte_stores=len(byte_events),
                 current_owned_bytes=sum(n for _,n in ZERO_RANGES),prefix=previous,stop=f'{order[-1]:08x}')
    (work/(capture.stem+'.validation.json')).write_text(json.dumps(summary,indent=2)+'\n',encoding='utf-8')
    print(f'{capture.name}: {len(tail)} new CP, {fields} fields, {stack_bytes} stack bytes, {bi2_bytes} BI2 bytes, {global_fields} globals; stop {order[-1]:08X}')
    return summary


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('dol','bi2','program','crt_program','l2_program','capture','work'):parser.add_argument(name,type=Path)
    validate(**vars(parser.parse_args()))

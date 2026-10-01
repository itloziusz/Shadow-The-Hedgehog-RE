"""Occurrence-ordered full-entry handler/CRT differential gate.

The unchanged L2 runner is replayed and validated first. Native receives only
pre-entry fields plus an opaque old-slot word, never fill/post-state values.
"""
import argparse
import copy
import json
from pathlib import Path
import subprocess

from boot_state_diff import (FIELD_NAMES,OrderedTrace,compare_fields,compare_memory,
                             hex_field,parse_native_state,reference_fields,word_validity)
from capture_crt_state import HANDLER_PLAN,ZERO_RANGES
from capture_dolphin_rsp import sha256
from validate_native_l2 import DolImage,validate as validate_l2

TAIL_ORDER=HANDLER_PLAN+[0x8000336C]*11+[0x800033AC]
TAIL_ORDER += [pc for _ in ZERO_RANGES for pc in (0x800033C0,0x8000540C,0x8000543C,0x80005424,0x800033DC)]
TAIL_ORDER += [0x800033C0,0x800033E4,0x80003170,0x80003188]


def checked_report(report,image):
    extension=report["region_extension"]
    if extension["name"]!="handler_crt_39" or extension["midchain_writes"] or extension["stop"]!="80003188":
        raise ValueError("wrong or forced region experiment")
    slot=hex_field(extension["controlled_initial_handler"],8,"entry handler")
    cut=next(i for i,s in enumerate(report["checkpoints"]) if int(s["pc"],16)==0x80372904)+1
    prefix=copy.deepcopy(report);prefix["checkpoints"]=prefix["checkpoints"][:cut]
    tail=report["checkpoints"][cut:]
    if [int(s["pc"],16) for s in tail]!=TAIL_ORDER:raise ValueError("CRT reference CFG occurrence order differs")
    rawtable=image.read(0x80005544,0xA4).hex()
    entry=report["checkpoints"][0]
    if hex_field(entry["handler_slot"],8,"entry handler slot")!=slot:raise ValueError("handler entry readback differs")
    seeds=extension["preentry_memory_writes"]
    addresses=[]
    for seed in seeds:
        address=hex_field(seed["address"],8,"seed address");addresses.append(address)
        value=hex_field(seed["value"],8,"seed word")
        if hex_field(seed["readback"],8,"seed readback")!=value:raise ValueError("seed readback differs")
    canaries=[a-4 for a,_ in ZERO_RANGES]+[a+n for a,n in ZERO_RANGES]
    if extension["canary_ranges"]!=[[f"{a:08x}",4] for a in canaries]:
        raise ValueError("outside boundary addresses/widths differ")
    allowed={0x80586CB4} | {a-4 for a,_ in ZERO_RANGES} | {a+n for a,n in ZERO_RANGES}
    allowed |= {a+offset for a,size in ZERO_RANGES for offset in (0,(size//8)*4,size-4)}
    if len(addresses)!=len(set(addresses)) or not set(addresses)<=allowed or not seeds or addresses[0]!=0x80586CB4:
        raise ValueError("unexplained pre-entry memory write")
    if int(seeds[0]["value"],16)!=slot:raise ValueError("entry slot seed mismatch")
    # Derive clear occurrence from the third/fourth point in each fill call.
    first_clear=cut+len(HANDLER_PLAN)+12+3
    second_clear=first_clear+5
    fill_starts=[first_clear,second_clear,second_clear+5]
    slot_write=cut+TAIL_ORDER.index(0x803733C4)
    source=entry["fpr_source"]
    # A byte address has one value even when captured through two different
    # windows. Seed observations and boundary observations may not disagree.
    seeded_canaries={int(seed["address"],16):int(seed["value"],16)
                     for seed in seeds if int(seed["address"],16) in canaries}
    for index,state in enumerate(report["checkpoints"]):
        reference_fields(state)
        if hex_field(state["instruction_word"],8,"instruction word")!=image.word(int(state["pc"],16),text=True):
            raise ValueError("raw instruction capture differs")
        if state["crt_descriptors"].lower()!=rawtable:raise ValueError("live descriptor writer unexplained")
        expected_slot=slot if index<slot_write else 0x803726D8 if index<first_clear else 0
        if hex_field(state["handler_slot"],8,"handler slot")!=expected_slot:raise ValueError("handler write/clear timeline differs")
        expected_source=source if index<second_clear else "0"*32
        if state["fpr_source"].lower()!=expected_source.lower():raise ValueError("FPR source clear timeline differs")
        if len(state["crt_canaries"])!=6:raise ValueError("outside boundary observations incomplete")
        for word in state["crt_canaries"]:hex_field(word,8,"outside boundary word")
        if state["crt_canaries"]!=entry["crt_canaries"]:raise ValueError("CRT wrote excluded boundary bytes")
        for address,word in zip(canaries,state["crt_canaries"]):
            if address in seeded_canaries and int(word,16)!=seeded_canaries[address]:
                raise ValueError("boundary/seed observations disagree at same address")
        if hex_field(state["l2_stack_address"],8,"stack observation address")!=0x8060C570:
            raise ValueError("stack observation address differs")
        hex_field(state["l2_stack_bytes"],0x90*2,"stack observation bytes")
        hex_field(state["paired_stack_bytes"],16*2,"paired stack observation bytes")
        for name in ("bi2_pointer","lowmem_44"):hex_field(state[name],8,name)
        if state["bi2_pointer"]!=entry["bi2_pointer"]:raise ValueError("unexplained BI2 producer")
        if len(state["preentry_seed_words"])!=len(seeds):raise ValueError("seed observations incomplete")
        for seed,word in zip(seeds,state["preentry_seed_words"]):
            address=int(seed["address"],16);expected=int(seed["value"],16)
            if address==0x80586CB4:expected=expected_slot
            else:
                for n,(start,size) in enumerate(ZERO_RANGES):
                    if start<=address<start+size and index>=fill_starts[n]:expected=0
            if hex_field(word,8,"seed observation")!=expected:raise ValueError("seed write/clear provenance differs")
    for item,(address,size) in zip(extension["filled_ranges"],ZERO_RANGES):
        if (hex_field(item["address"],8,"fill address")!=address or
                hex_field(item["size"],8,"fill size")!=size):raise ValueError("fill range differs")
        hex_field(item["bytes"],size*2,"completed fill bytes")
    if len(extension["filled_ranges"])!=3:raise ValueError("missing/extra fill observation")
    return prefix,tail


def store_ledger(tail):
    """Expected stores from raw instructions and independently captured inputs."""
    batches={};events=[]
    def add(before,pc,address,value):
        effect=[pc,address,value,value];events.append(effect)
        batches.setdefault(before,[]).append(effect)
    gpr=lambda s:[int(s["gpr"][i:i+8],16) for i in range(0,256,8)]
    s=tail[0];r=gpr(s);sp=r[1]
    add(1,0x8037337C,sp+4,int(s["lr"],16));add(1,0x80373380,sp-0x30,sp)
    for pc,off,reg in ((0x80373384,-4,31),(0x80373388,-8,30),(0x8037338C,-12,29),(0x80373394,-16,28)):
        add(1,pc,sp+off,r[reg])
    i=TAIL_ORDER.index(0x803733C0);r=gpr(tail[i]);add(i+1,0x803733C0,r[4],r[28])
    i=TAIL_ORDER.index(0x80370C8C);r=gpr(tail[i]);sp=r[1]-0x70
    add(i+1,0x80370C8C,sp,r[1])
    for reg in range(3,11):add(i+2,0x80370CB4+4*(reg-3),sp+8+4*(reg-3),r[reg])
    i=TAIL_ORDER.index(0x8000315C);r=gpr(tail[i]);sp=r[1]-8
    add(i+1,0x80003160,sp,r[1]);add(i+1,0x80003164,sp+4,0xFFFFFFFF);add(i+1,0x80003168,sp,0xFFFFFFFF)
    i=TAIL_ORDER.index(0x80003340);r=gpr(tail[i]);sp=r[1]-24
    add(i+1,0x80003344,r[1]+4,int(tail[i]["lr"],16));add(i+1,0x80003348,sp,r[1])
    for pc,off,reg in ((0x8000334C,20,31),(0x80003350,16,30),(0x80003354,12,29)):add(i+1,pc,sp+off,r[reg])
    for i,pc in enumerate(TAIL_ORDER):
        if pc==0x8000540C:
            r=gpr(tail[i]);sp=r[1]-16
            add(i+1,pc,sp,r[1]);add(i+1,0x80005414,sp+20,int(tail[i]["lr"],16));add(i+1,0x80005418,sp+12,r[31])
    add(len(tail)-1,0x8000317C,0x80000044,0)
    return events,batches


def validate(dol,program,l2_program,capture,work):
    root=Path(__file__).resolve().parents[3]/"build"
    if not work.resolve().is_relative_to(root.resolve()):raise ValueError("outputs outside repository build")
    report=json.loads(capture.read_text(encoding="utf-8"));image=DolImage(dol)
    prefix,tail=checked_report(report,image);work.mkdir(parents=True,exist_ok=True)
    projection=work/(capture.stem+".prefix-reference.json")
    projection.write_text(json.dumps(prefix,indent=2)+"\n",encoding="utf-8")
    validate_l2(dol,l2_program,projection,work,controlled_entry_msr=report["region_extension"]["controlled_initial_msr"])
    oldfixture=work/(projection.stem+".entry.txt")
    fixture=work/(capture.stem+".entry.txt")
    fixture.write_text(oldfixture.read_text()+report["region_extension"]["controlled_initial_handler"]+"\n",encoding="ascii")
    output=subprocess.run([str(program.resolve()),str(dol.resolve()),str(fixture.resolve())],capture_output=True,text=True,check=True).stdout
    (work/(capture.stem+".native.txt")).write_text(output,encoding="ascii")
    prior_output=(work/(projection.stem+".native.txt")).read_text()
    old_lines=[line for line in prior_output.splitlines() if line[:1].isdigit()]
    lines=output.splitlines();native_states=[line for line in lines if line[:1].isdigit()]
    if native_states[:len(old_lines)]!=old_lines:raise ValueError("connected L2 prefix output changed")
    states=native_states[len(old_lines):]
    if len(states)!=len(tail):raise ValueError("incomplete or extra CRT checkpoints")
    expected_stores,batches=store_ledger(tail)
    effects=[[int(x,16) for x in line.split()[1:]] for line in lines if line.startswith("BOOT_STORE ")]
    if effects!=expected_stores:raise ValueError("ordered applied boot stores differ")
    last=parse_native_state(old_lines[-1]);known={}
    for n,bit in enumerate(last[117]):
        if bit=="1":known[0x8060C570+n]=bytes.fromhex(last[116])[n]
    cursor=OrderedTrace(tail);fields=memory=0
    for i,(line,pc) in enumerate(zip(states,TAIL_ORDER)):
        actual=parse_native_state(line)
        if len(actual)!=120 or int(actual[0],16)!=pc:raise ValueError("CRT native occurrence order/shape differs")
        state=cursor.take(pc);label=f"PC {pc:08X} occurrence {i}"
        fields+=compare_fields(actual[:113]+[actual[115],actual[118]],reference_fields(state)+[state["l2cr"],state["handler_slot"]],FIELD_NAMES+["L2CR","handler slot"],label)
        if i==len(tail)-1:
            fields+=compare_fields([actual[119]],[state["lowmem_44"]],["lowmem44"],label)
        elif actual[119]!="-":raise ValueError("unwritten lowmem44 claimed concrete")
        for _,address,value,_ in batches.get(i,[]):
            if 0x8060C570<=address<0x8060C600:
                for b,v in enumerate(value.to_bytes(4,"big")):known[address+b]=v
        mask="".join("1" if 0x8060C570+n in known else "0" for n in range(144))
        memory+=compare_memory(actual[116],actual[117],state["l2_stack_bytes"],mask,label,0x8060C570)
        paired="".join("1" if 0x8060C5E8+n in known else "0" for n in range(16))
        memory+=compare_memory(actual[113],actual[114],state["paired_stack_bytes"],paired,label,0x8060C5E8)
    cursor.finish()
    zero_lines=[line.split() for line in lines if line.startswith("ZERO_RANGE ")]
    if len(zero_lines)!=3:raise ValueError("missing/extra native zero ranges")
    cleared=0
    for native,observed,(address,size) in zip(zero_lines,report["region_extension"]["filled_ranges"],ZERO_RANGES):
        if len(native)!=4 or int(native[1],16)!=address or int(native[2],16)!=size:raise ValueError("native zero range shape differs")
        hex_field(native[3],size*2,"native completed range")
        if native[3].lower()!=observed["bytes"].lower():raise ValueError("completed zero range bytes differ")
        cleared+=size
    if lines[-1]!="STOP pc=0x80003188 reason=LIVE_BI2_POINTER_UNRESOLVED":raise ValueError("wrong final CRT stop")
    summary={"capture_sha256":sha256(capture),"native_executable_sha256":sha256(program),
             "entry_fixture_sha256":sha256(fixture),"tail_checkpoints":len(tail),"tail_state_fields":fields,
             "known_stack_bytes":memory,"cleared_bytes":cleared,"ordered_stores":len(effects),"stop":"80003188"}
    (work/(capture.stem+".validation.json")).write_text(json.dumps(summary,indent=2)+"\n",encoding="utf-8")
    print(f"{capture.name}: tail {len(tail)} checkpoints; {fields} fields; {memory} stack bytes; {cleared} cleared bytes; {len(effects)} stores; stop80003188")
    return summary


if __name__=="__main__":
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ("dol","program","l2_program","capture","work"):parser.add_argument(name,type=Path)
    args=parser.parse_args();validate(args.dol,args.program,args.l2_program,args.capture,args.work)

"""Independent clock arithmetic/raw/ordering audit, never a PPC interpreter.

The input DOL is read only. Synthetic arithmetic probes isolate ISA contracts;
they supply no measured clock result to the connected native runner. The
production formula probes concern the inspected private Dolphin oracle only.
"""
from __future__ import annotations

import argparse
from copy import deepcopy
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

PAL_SHA = "fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af"
U32 = (1 << 32) - 1
U64 = (1 << 64) - 1
CA = 0x20000000
SO = 0x80000000

# Independently reread literals, including all instructions after the current
# clock stop and both arms of the immediately following pointer decision.
REGIONS = {
    0x80379628: [0x7C6D42E6,0x7C8C42E6,0x7CAD42E6,0x7C032800,0x4082FFF0,0x4E800020],
    0x80379648: [0x7C0802A6,0x90010004,0x9421FFE0,0x93E1001C,0x93C10018,
        0x93A10014,0x4BFFCABD,0x7C7F1B78,0x4BFFFFC1,0x3CC08000,0x80A630DC,
        0x800630D8,0x7FA52014,0x7FC01914,0x7FE3FB78,0x4BFFCAC1,0x7FA4EB78,
        0x7FC3F378,0x80010024,0x83E1001C,0x83C10018,0x83A10014,0x38210020,
        0x7C0803A6,0x4E800020],
    0x8037611C: [0x7C6000A6,0x5464045E,0x7C800124,0x54638FFE,0x4E800020],
    0x80376144: [0x2C030000,0x7C8000A6,0x4182000C,0x60858000,0x48000008,
        0x5485045E,0x7CA00124,0x54838FFE,0x4E800020],
    0x80370EA8: [0x908D5A54,0x906D5A50,0x4800526D,0x387F0050,0x48002BFD],
    0x80373AB4: [0x7C0802A6,0x90010004,0x9421FFF8,0x3CA08000,0x808530F0,
        0x7C052040,0x41810010,0x38A0001C,0x4BC91A21,0x4800000C,0x38000000,
        0x90030000,0x8001000C,0x38210008,0x7C0803A6,0x4E800020],
    0x80376EBC: [0x7CAC42E6,0x7CCC42E6,0x7CE53050,0x28071124,0x4180FFF4],
}


def signed32(word):
    return word if word < 0x80000000 else word - (1 << 32)


def branch_target(pc, word):
    op = word >> 26
    if op == 18:
        disp = word & 0x03FFFFFC
        if disp & 0x02000000:
            disp -= 0x04000000
    elif op == 16:
        disp = word & 0xFFFC
        if disp & 0x8000:
            disp -= 0x10000
    else:
        raise ValueError("not a direct branch")
    return ((0 if word & 2 else pc) + disp) & U32


def read_dol(path):
    raw = path.read_bytes()
    if hashlib.sha256(raw).hexdigest() != PAL_SHA:
        raise ValueError("not the pinned PAL input")
    offsets = struct.unpack_from(">18I", raw, 0)
    addresses = struct.unpack_from(">18I", raw, 0x48)
    sizes = struct.unpack_from(">18I", raw, 0x90)
    regions = [(a,a+n,o) for a,n,o in zip(addresses,sizes,offsets) if n]
    def word(pc):
        for lo,hi,offset in regions:
            if lo <= pc and pc + 4 <= hi:
                return struct.unpack_from(">I", raw, offset+pc-lo)[0]
        raise ValueError(f"unmapped DOL address {pc:08X}")
    return word


def raw_audit(path, show_words=False):
    read = read_dol(path)
    sys.path.insert(0,str(Path(__file__).resolve().parents[3]/"gameplay"/"tools"))
    from ppc import decode
    checked = 0
    for base, expected in REGIONS.items():
        for n,w in enumerate(expected):
            pc = base+4*n
            assert read(pc) == w, f"raw mismatch {pc:08X}"
            ins = decode(w,pc)
            assert ins.m not in (".long",".word","unknown"), ins
            if show_words:
                print(f"{pc:08X} {w:08X} {ins}")
            checked += 1
    # Bitfield checks do not obtain branch/TBR meaning from the decoder.
    assert [((w>>16)&31)|((w>>6)&0x3E0) for w in REGIONS[0x80379628][:3]] == [269,268,269]
    assert [((w>>21)&31) for w in REGIONS[0x80379628][:3]] == [3,4,5]
    assert branch_target(0x80379638,read(0x80379638)) == 0x80379628
    assert (read(0x80379638)>>21)&31 == 4 and (read(0x80379638)>>16)&31 == 2
    for pc,xo,d,a,b in ((0x80379678,10,29,5,4),(0x8037967C,138,30,0,3)):
        w=read(pc)
        assert w>>26 == 31 and (w>>1)&511 == xo
        assert (w>>21)&31 == d and (w>>16)&31 == a and (w>>11)&31 == b
        assert w&1 == 0 and (w>>10)&1 == 0  # Rc=0, OE=0.
    assert (read(0x80373AC8)>>1)&1023 == 32  # cmplw, unsigned.
    assert (read(0x80373AC8)>>16)&31 == 5 and (read(0x80373AC8)>>11)&31 == 4
    assert branch_target(0x80373ACC,read(0x80373ACC)) == 0x80373ADC
    assert branch_target(0x80373AD4,read(0x80373AD4)) == 0x800054F4
    assert branch_target(0x80376ECC,read(0x80376ECC)) == 0x80376EC0
    assert read(0x80376EC8)&0xFFFF == 0x1124
    print(f"PASS {checked} independent exact words, arithmetic/TBR/unsigned/CFG fields")


def add_words(th,tl,oh,ol,xer):
    """ISA addc+adde consequence, with explicitly admitted input bit patterns."""
    lower = tl+ol
    upper = th+oh+(lower>>32)
    return upper&U32,lower&U32,(xer&~CA)|((upper>>32)<<29)


def compare_high(first,second,cr,xer):
    a,b=signed32(first),signed32(second)
    field = 8 if a < b else 4 if a > b else 2
    return (cr&0x0FFFFFFF)|((field|bool(xer&SO))<<28)


def oracle_tb(start_value,start_ticks,current_ticks):
    # Unsigned64 subtraction, division, addition are exactly the inspected
    # private SystemTimers expression. No host clock is called here.
    return (start_value+(((current_ticks-start_ticks)&U64)//12))&U64


def arithmetic_probes():
    values = [0,1,2,0x7FFFFFFF,0x80000000,0xFFFFFFFE,0xFFFFFFFF]
    count=0
    for th in values:
        for tl in values:
            for oh in values:
                for ol in values:
                    for oldxer in (0,CA,SO,SO|CA|0x4000007F):
                        hi,lo,xer = add_words(th,tl,oh,ol,oldxer)
                        wide = ((th<<32)|tl)+((oh<<32)|ol)
                        assert ((hi<<32)|lo) == wide&U64
                        assert bool(xer&CA) == bool(wide>>64)
                        assert (xer&~CA) == (oldxer&~CA)
                        count+=1
    # A two's-complement -1 offset and unsignedFFFFFFFFFFFFFFFF are the same
    # architectural input. A signed C++64 addition would invoke UB at extrema.
    assert add_words(0,1,U32,U32,0) == (0,0,CA)
    assert add_words(0,0,0,0,CA) == (0,0,0)
    # Failed compares can cross the signed32 boundary; unsigned compare differs.
    assert compare_high(0x7FFFFFFF,0x80000000,0x1234567,SO) == 0x51234567
    assert compare_high(0xFFFFFFFF,0,0x1234567,0) == 0x81234567
    assert compare_high(0x80000000,0x80000000,0x1234567,SO) == 0x31234567
    print(f"PASS {count} 64-bit modulo/carry/XER cases and signed high-compare probes")


def producer_rollover_probes():
    # Read epochs are production INPUT cycles, never known sample outputs.
    # First attempt crosses lower rollover; second has equal high samples.
    counter = 0x00000000FFFFFFFF
    triples = ((0,12,24),(36,48,60))
    attempts=[]
    for a,b,c in triples:
        h1=oracle_tb(counter,0,a)>>32
        lo=oracle_tb(counter,0,b)&U32
        h2=oracle_tb(counter,0,c)>>32
        attempts.append((h1,lo,h2,h1==h2))
    assert attempts == [(0,0,1,False),(1,3,1,True)]
    assert ((attempts[-1][0]<<32)|attempts[-1][1]) == counter+4
    # Full64 counter wrap also causes mismatch and a retry.
    assert oracle_tb(U64,0,0)>>32 == U32
    assert oracle_tb(U64,0,12) == 0
    assert oracle_tb(U64,0,24)>>32 == 0
    # Division truncates whole production cycles; epoch and counter are distinct.
    assert [oracle_tb(0x1234,100,n) for n in (100,111,112,123,124)] == [0x1234,0x1234,0x1235,0x1235,0x1236]
    # A counter sample loses the remainder of CPUcycles/12. Equal observed
    # samples are consistent with different subsequent values after one cycle.
    assert oracle_tb(0,100,100) == oracle_tb(0,89,100) == 0
    assert oracle_tb(0,100,101) == 0 and oracle_tb(0,89,101) == 1
    # The debugger read samples before its retirement changes the subsequent
    # checkpoint GetTicks. This phase11 case derives rollover purely from
    # RTCorigin0 and sourcecycles, without an injected counter result.
    source_cycles=100
    frontier=source_cycles+12*((1<<32)-1)+11
    before=[oracle_tb(0,source_cycles,frontier+n) for n in range(3)]
    wrong_after=[oracle_tb(0,source_cycles,frontier+n+1) for n in range(3)]
    assert (before[0]>>32,before[1]&U32,before[2]>>32) == (0,0,1)
    assert (wrong_after[0]>>32,wrong_after[1]&U32,wrong_after[2]>>32) == (1,0,1)
    # At phase10, firsthigh remains equal but the TBL destination itself differs.
    phase10=frontier-1
    assert oracle_tb(0,source_cycles,phase10+1)&U32 == U32
    assert oracle_tb(0,source_cycles,phase10+2)&U32 == 0
    # A failed compare must retry all three reads. A finite no-match profile
    # has no return result; it cannot force the last pair into an exit state.
    assert not attempts[0][3]
    print("PASS production-cycle arithmetic, lower/full64 rollover, retry and division probes")


def ordering_and_owner_probes():
    # Native typed stack arithmetic only, not an emulated address space.
    os_sp = 0x8060C5D0
    clock_sp = os_sp-0x20
    offsets = {"clock_lr":clock_sp+0x24,"clock_r31":clock_sp+0x1C,
               "clock_r30":clock_sp+0x18,"clock_r29":clock_sp+0x14}
    assert offsets == {"clock_lr":0x8060C5D4,"clock_r31":0x8060C5CC,
                       "clock_r30":0x8060C5C8,"clock_r29":0x8060C5C4}
    pointer_sp=os_sp-8
    assert os_sp+4 == offsets["clock_lr"]
    assert pointer_sp == offsets["clock_r30"]
    # Restore only EE in currentMSR. Changes in other bits cannot be undone by
    # an original-MSR snapshot. The returned pre-restore EE is overwritten later.
    for prior in (0,1):
        for live in (0,0x2030,0xA030,0x12342000):
            restore = live|0x8000 if prior else live&~0x8000
            assert (restore&~0x8000) == (live&~0x8000)
            assert bool(restore&0x8000) == bool(prior)
    store_order = [(0x80370EA8,0x805F1F54),(0x80370EAC,0x805F1F50)]
    assert store_order[0][1] > store_order[1][1]  # Low word first, not address order.
    for pointer in (0,1,0x7FFFFFFF,0x80000000,0xFFFFFFFF):
        clear = 0x80000000 > pointer
        assert clear == (pointer < 0x80000000)
    print("PASS stack aliases, current-MSR EE restoration, low-first stores and pointer predicate")


def strict_hex(text,width,name):
    if not isinstance(text,str) or len(text)!=width or any(c not in "0123456789abcdefABCDEF" for c in text):
        raise ValueError(f"malformed {name}")
    return int(text,16)


def registers(state):
    packed=state["gpr"]
    if not isinstance(packed,str) or len(packed)!=256:
        raise ValueError("malformed packed GPR bank")
    return [strict_hex(packed[8*n:8*n+8],8,f"r{n}") for n in range(32)]


def producer_provenance(report):
    """Admit RTC initial state or the separately labelled entry-only rebase."""
    extension=report["region_extension"]
    mode=extension.get("producer_mode","rtc_initial")
    if extension.get("sampler_mode","single_step")!="single_step":
        raise ValueError("continuous sampling cannot enter the SingleStep ledger")
    controls=extension.get("epoch_controls",[])
    if mode=="rtc_initial":
        if controls:raise ValueError("RTC profile has undeclared epoch controls")
        return mode
    if mode!="preentry_rebased" or len(controls)!=3:
        raise ValueError("unknown or incomplete clock source perturbation")
    requested_input=strict_hex(extension.get("requested_epoch"),16,"declared pre-entry counter epoch")
    manifest=report["instrumentation_manifest"]
    policy=manifest.get("clock_perturbations",{})
    if (manifest.get("purpose")!="controlled pre-entry clock-source experiment oracle; no native dependency" or
        policy.get("enabled") is not True or policy.get("allowed_pc")!="80003154" or
        policy.get("exact_packet_value_width")!=8 or policy.get("default_timing_export_unchanged") is not True or
        set(policy.get("write_register_map",{}))!={"250","251","252"}):
        raise ValueError("rebase oracle lacks the exact entry-only control contract")
    widths={"exceptions":8,"cycles":16,"epoch_cycles":16,"epoch_value":16,"cpu_hz":8,"rtc_offset":16,"cached_tb":16}
    original=report["original_unmodified_entry"]
    if strict_hex(original["pc"],8,"original entry")!=0x80003154:
        raise ValueError("rebase lacks a complete original entry")
    prior=dict(original["clock_producer"])
    for key,width in widths.items():strict_hex(prior[key],width,"pre-control "+key)
    if strict_hex(prior["exceptions"],8,"entry exceptions"):
        raise ValueError("pending exception in rebase entry")
    requested=[]
    for n,(control,prefix) in enumerate(zip(controls,("Pfa=","Pfb=","Pfc="))):
        if strict_hex(control["pc"],8,"control pc")!=0x80003154:
            raise ValueError("midchain/per-read clock control")
        packet=control["packet"]
        if not isinstance(packet,str) or not packet.startswith(prefix) or len(packet)!=12:
            raise ValueError("reordered/malformed clock control")
        value=strict_hex(packet[4:],8,"source control value");requested.append(value)
        expected=dict(prior)
        old=strict_hex(prior["epoch_value"],16,"old counter epoch")
        if n==0:expected["epoch_value"]=f"{(old&0xFFFFFFFF00000000)|value:016X}"
        elif n==1:expected["epoch_value"]=f"{(value<<32)|(old&U32):016X}"
        else:
            if value!=1:raise ValueError("clock commit must be exactly one")
            expected["epoch_cycles"]=prior["cycles"]
            expected["cached_tb"]=prior["epoch_value"]
        readback=control["readback"]
        if set(readback)!=set(widths):raise ValueError("incomplete source control readback")
        for key,width in widths.items():
            if strict_hex(readback[key],width,"control "+key)!=strict_hex(expected[key],width,"expected "+key):
                raise ValueError("clock source control consequence differs: "+key)
        prior=dict(readback)
    requested_epoch=(requested[1]<<32)|requested[0]
    if requested_input!=requested_epoch or strict_hex(prior["epoch_value"],16,"committed epoch")!=requested_epoch:
        raise ValueError("wrong combined rebase input")
    entry=report["checkpoints"][0]
    if strict_hex(entry["pc"],8,"entry pc")!=0x80003154 or entry["clock_producer"]!=prior:
        raise ValueError("post-control complete entry differs")
    for state in report["checkpoints"]:
        p=state["clock_producer"]
        for key in ("epoch_cycles","epoch_value","cpu_hz","rtc_offset"):
            if strict_hex(p[key],widths[key],"later "+key)!=strict_hex(prior[key],widths[key],"committed "+key):
                raise ValueError("unexplained clock rebase during the instruction chain")
    return mode


def capture_ledger(path,report=None,quiet=False):
    """Finite clock-effect ledger over reference states; no opcode execution.

    Production reads are checked against independent integer arithmetic on
    measured source getter inputs. Every untouched architectural field and
    observed byte view must remain coherent. Earlier chain parity is separate.
    """
    if report is None:report=json.loads(path.read_text(encoding="utf-8"))
    if report["dolphin_sha256"]!=report["instrumentation_manifest"]["instrumented_executable_sha256"]:
        raise ValueError("oracle identity contradiction")
    if report["controlled_midchain_l2cr"] or report["controlled_live_bss_source_experiment"]:
        raise ValueError("midchain controls are inadmissible")
    extension=report["region_extension"]
    if extension["stop"].lower()!="80373ac4" or extension["midchain_writes"]:
        raise ValueError("unknown stop or midchain writes")
    mode=producer_provenance(report)
    states=report["checkpoints"]
    first=next(i for i,s in enumerate(states) if strict_hex(s["pc"],8,"pc")==0x80379628)
    tail=states[first:]
    if [s["pc"].lower() for s in tail[1:]]!=extension["clock_plan"]:
        raise ValueError("clock plan omits/reorders repeated states")
    if extension["pause_samples"][0]!=extension["pause_samples"][1]:
        raise ValueError("paused source getters changed")
    stable_fields=("ctr","xer","fpscr","hid0","hid1","l2cr","hid2","ps0","ps1","gqr",
                   "icache_disabled","fpr_source","paired_stack_bytes","l2_stack_address",
                   "handler_slot","lowmem_44","bi2_blob","bi2_globals","bi2_pointer","lowmem_34",
                   "lowmem_48","crt_canaries","crt_descriptors","clock_low_words")
    scalar_fields=("msr","cr","lr","architectural_xer")
    raw_words={base+4*n:w for base,words in REGIONS.items() for n,w in enumerate(words)}
    comparisons=0
    cache_changes=[]
    for previous,current in zip(tail,tail[1:]):
        pc=strict_hex(previous["pc"],8,"pc")
        if pc not in raw_words or strict_hex(previous["instruction_word"],8,"instruction")!=raw_words[pc]:
            raise ValueError(f"unknown/contradictory clock word {pc:08X}")
        g=registers(previous)
        wanted={key:strict_hex(previous[key],8,key) for key in scalar_fields}
        stack=bytearray.fromhex(previous["l2_stack_bytes"])
        stack_base=strict_hex(previous["l2_stack_address"],8,"stack address")
        def load(a):
            o=a-stack_base
            if o<0 or o+4>len(stack) or o&3:raise ValueError("unexplained ledger stack read")
            return int.from_bytes(stack[o:o+4],"big")
        def store(a,v):
            o=a-stack_base
            if o<0 or o+4>len(stack) or o&3:raise ValueError("unexplained ledger stack write")
            stack[o:o+4]=v.to_bytes(4,"big")
        producer=previous["clock_producer"]
        following=current["clock_producer"]
        if "exceptions" not in producer or "exceptions" not in following:
            raise ValueError("unobserved pending exception state; old capture cannot validate clock")
        if strict_hex(producer["exceptions"],8,"exceptions") or strict_hex(following["exceptions"],8,"exceptions"):
            raise ValueError("pending exception prevents linear clock ledger")
        cycles=strict_hex(producer["cycles"],16,"cycles")
        if strict_hex(following["cycles"],16,"cycles")!=cycles+1:
            raise ValueError("unexplained step timing")
        for field in ("epoch_cycles","epoch_value","cpu_hz","rtc_offset"):
            if producer[field]!=following[field]:raise ValueError("epoch/frequency rebase inside clock")
        if strict_hex(producer["cpu_hz"],8,"cpu hz")!=486000000:raise ValueError("unvalidated CPUclock")
        epoch=strict_hex(producer["epoch_value"],16,"epoch value")
        if mode=="rtc_initial" and epoch%40500000:raise ValueError("initial RTC epoch not produced by integer seconds")
        epoch_cycles=strict_hex(producer["epoch_cycles"],16,"epoch cycles")
        cached=strict_hex(producer["cached_tb"],16,"cached TB")
        globals_=[strict_hex(x,8,"clock global") for x in previous["clock_globals"]]
        next_pc=pc+4
        # Only this pinned finite semantic ledger is admitted. No generic
        # instruction decoder drives transitions or fabricates earlier state.
        if pc in (0x80379628,0x8037962C,0x80379630):
            cached=oracle_tb(epoch,epoch_cycles,cycles)
            d={0x80379628:3,0x8037962C:4,0x80379630:5}[pc]
            g[d]=(cached&U32) if d==4 else cached>>32
        elif pc==0x80379634:wanted["cr"]=compare_high(g[3],g[5],wanted["cr"],wanted["architectural_xer"])
        elif pc==0x80379638:next_pc=0x8037963C if wanted["cr"]&0x20000000 else 0x80379628
        elif pc in (0x8037963C,0x80376164,0x803796A8,0x8037612C):next_pc=wanted["lr"]&~3
        elif pc==0x8037966C:g[6]=0x80000000
        elif pc==0x80379670:g[5]=strict_hex(previous["clock_low_words"][1],8,"offsetlow")
        elif pc==0x80379674:g[0]=strict_hex(previous["clock_low_words"][0],8,"offsethigh")
        elif pc==0x80379678:
            low=g[5]+g[4];g[29]=low&U32
            wanted["architectural_xer"]=(wanted["architectural_xer"]&~CA)|((low>>32)<<29)
        elif pc==0x8037967C:
            high=g[0]+g[3]+bool(wanted["architectural_xer"]&CA);g[30]=high&U32
            wanted["architectural_xer"]=(wanted["architectural_xer"]&~CA)|((high>>32)<<29)
        elif pc==0x80379680:g[3]=g[31]
        elif pc==0x80379684:wanted["lr"]=0x80379688;next_pc=0x80376144
        elif pc==0x80376144:wanted["cr"]=compare_high(g[3],0,wanted["cr"],wanted["architectural_xer"])
        elif pc==0x80376148:g[4]=wanted["msr"]
        elif pc==0x8037614C:next_pc=0x80376158 if wanted["cr"]&0x20000000 else 0x80376150
        elif pc==0x80376150:g[5]=g[4]|0x8000
        elif pc==0x80376154:next_pc=0x8037615C
        elif pc==0x80376158:g[5]=g[4]&~0x8000
        elif pc==0x8037615C:wanted["msr"]=g[5]
        elif pc==0x80376160:g[3]=(g[4]>>15)&1
        elif pc==0x80379688:g[4]=g[29]
        elif pc==0x8037968C:g[3]=g[30]
        elif pc==0x80379690:g[0]=load(g[1]+0x24)
        elif pc==0x80379694:g[31]=load(g[1]+0x1C)
        elif pc==0x80379698:g[30]=load(g[1]+0x18)
        elif pc==0x8037969C:g[29]=load(g[1]+0x14)
        elif pc==0x803796A0:g[1]=(g[1]+32)&U32
        elif pc==0x803796A4:wanted["lr"]=g[0]
        elif pc==0x80370EA8:globals_[1]=g[4]
        elif pc==0x80370EAC:globals_[0]=g[3]
        elif pc==0x80370EB0:wanted["lr"]=0x80370EB4;next_pc=0x8037611C
        elif pc==0x8037611C:g[3]=wanted["msr"]
        elif pc==0x80376120:g[4]=g[3]&~0x8000
        elif pc==0x80376124:wanted["msr"]=g[4]
        elif pc==0x80376128:g[3]=(g[3]>>15)&1
        elif pc==0x80370EB4:g[3]=(g[31]+0x50)&U32
        elif pc==0x80370EB8:wanted["lr"]=0x80370EBC;next_pc=0x80373AB4
        elif pc==0x80373AB4:g[0]=wanted["lr"]
        elif pc==0x80373AB8:store(g[1]+4,g[0])
        elif pc==0x80373ABC:old=g[1];g[1]=(old-8)&U32;store(g[1],old)
        elif pc==0x80373AC0:g[5]=0x80000000
        else:raise ValueError(f"transition outside finite ledger {pc:08X}")
        if registers(current)!=g:raise ValueError(f"GPR effect mismatch after {pc:08X}")
        if strict_hex(current["pc"],8,"pc")!=next_pc:raise ValueError(f"CFG mismatch after {pc:08X}")
        for key,value in wanted.items():
            if strict_hex(current[key],8,key)!=value:raise ValueError(f"{key} effect mismatch after {pc:08X}")
        if bytes.fromhex(current["l2_stack_bytes"])!=stack:raise ValueError(f"stack alias mismatch after {pc:08X}")
        if [strict_hex(x,8,"clockglobal") for x in current["clock_globals"]]!=globals_:raise ValueError(f"global/store order mismatch after {pc:08X}")
        if strict_hex(following["cached_tb"],16,"cached TB")!=cached:raise ValueError(f"cached TB production mismatch after {pc:08X}")
        for key in stable_fields:
            if previous[key]!=current[key]:raise ValueError(f"unexplained {key} mutation after {pc:08X}")
        for key in ("icache_valid","icache_plru"):
            strict_hex(previous[key],256,key);strict_hex(current[key],256,key)
            a,b=bytes.fromhex(previous[key]),bytes.fromhex(current[key])
            changed=sum(x!=y for x,y in zip(a,b))
            if changed:cache_changes.append((f"{pc:08X}",key,changed))
        comparisons+=32+len(wanted)+3+len(stable_fields)+len(stack)
    if strict_hex(tail[-1]["pc"],8,"stop")!=0x80373AC4:raise ValueError("unknown F0 read was consumed")
    if not quiet:
        print(f"PASS {path.name}: {len(tail)-1} finite clock transitions, {comparisons} field/byte comparisons; pendingexceptions=0")
        print(f"UNKNOWN separate oracle fetch-cache effects: {cache_changes}")


def capture_falsifications(path):
    original=json.loads(path.read_text(encoding="utf-8"))
    first=next(i for i,s in enumerate(original["checkpoints"]) if int(s["pc"],16)==0x80379628)
    store_i=next(i for i,s in enumerate(original["checkpoints"]) if int(s["pc"],16)==0x80370EAC)
    alias_i=next(i for i,s in enumerate(original["checkpoints"]) if int(s["pc"],16)==0x80373AC0)
    def xor_hex(text,bit=1):return f"{int(text,16)^bit:0{len(text)}x}"
    mutations=[]
    for key in ("pc","msr","cr","lr","ctr","architectural_xer","fpscr","gpr","ps0","ps1","gqr",
                "paired_stack_bytes","handler_slot","clock_globals","l2_stack_bytes","clock_low_words"):
        def change(report,key=key):
            state=report["checkpoints"][first+1]
            if isinstance(state[key],list):state[key][0]=xor_hex(state[key][0])
            else:state[key]=xor_hex(state[key])
        mutations.append((key,change))
    for key in ("cycles","epoch_cycles","epoch_value","cpu_hz","rtc_offset","cached_tb","exceptions"):
        def change(report,key=key):
            state=report["checkpoints"][first+1]["clock_producer"]
            state[key]=xor_hex(state[key])
        mutations.append(("producer-"+key,change))
    mutations.append(("missing-exceptions",lambda r:r["checkpoints"][first]["clock_producer"].pop("exceptions")))
    mutations.append(("malformed-gpr",lambda r:r["checkpoints"][first+1].update(gpr="0")))
    def premature_high(report):
        state=report["checkpoints"][store_i]
        state["clock_globals"][0]=xor_hex(state["clock_globals"][0])
    mutations.append(("low-first-premature-high",premature_high))
    def bad_alias(report):
        state=report["checkpoints"][alias_i]
        state["l2_stack_bytes"]=xor_hex(state["l2_stack_bytes"],1<<((0x8060C600-0x8060C5C8-1)*8))
    mutations.append(("later-stack-alias",bad_alias))
    if original["region_extension"].get("producer_mode")=="preentry_rebased":
        mutations.append(("rebase-no-label",lambda r:r["region_extension"].pop("producer_mode")))
        mutations.append(("rebase-missing-control",lambda r:r["region_extension"]["epoch_controls"].pop()))
        mutations.append(("rebase-reordered-controls",lambda r:r["region_extension"]["epoch_controls"].reverse()))
        mutations.append(("rebase-midchain-control",lambda r:r["region_extension"]["epoch_controls"][0].update(pc="80379628")))
        mutations.append(("rebase-partial-readback",lambda r:r["region_extension"]["epoch_controls"][0]["readback"].pop("cached_tb")))
        mutations.append(("rebase-wrong-commit",lambda r:r["region_extension"]["epoch_controls"][2].update(packet="Pfc=00000002")))
        mutations.append(("rebase-unguarded-oracle",lambda r:r["instrumentation_manifest"]["clock_perturbations"].update(allowed_pc="80379628")))
        mutations.append(("rebase-new-epoch-midchain",lambda r:r["checkpoints"][first]["clock_producer"].update(epoch_value="1234567900000000")))
        mutations.append(("rebase-missing-request",lambda r:r["region_extension"].pop("requested_epoch")))
        mutations.append(("rebase-wrong-request",lambda r:r["region_extension"].update(requested_epoch="0000000000000000")))
    rejected=0
    for name,change in mutations:
        report=deepcopy(original);change(report)
        try:capture_ledger(path,report,True)
        except (ValueError,KeyError,AssertionError):rejected+=1
        else:raise AssertionError("ledger admitted mutation "+name)
    print(f"PASS {path.name}: {rejected} independent malformed/changed reference effects rejected")


def compiled_ledger(capture_path,native_text,quiet=False):
    report=json.loads(capture_path.read_text(encoding="utf-8"))
    producer_provenance(report)
    states=report["checkpoints"]
    first=next(i for i,s in enumerate(states) if int(s["pc"],16)==0x80379628)
    refs=states[first+1:]
    rows=[];clocks=[];stores=[];ranges=[];stop=[]
    for line in native_text.splitlines():
        parts=line.split()
        if not parts:continue
        if parts[0]=="CLOCK_RESEARCH":clocks.append(parts[1:])
        elif parts[0]=="BOOT_STORE":stores.append(parts[1:])
        elif parts[0]=="RESEARCH_GLOBAL_RANGE":ranges.append(parts[1:])
        elif parts[0]=="RESEARCH_STOP":stop.append(parts[1:])
        elif len(parts[0])==8 and all(c in "0123456789abcdefABCDEF" for c in parts[0]):rows.append(parts)
        else:raise ValueError("unexpected compiled output record")
    if len(rows)!=len(refs) or len(clocks)!=len(refs):raise ValueError("compiled repeated checkpoint count differs")
    # Validity is independently derived from admitted checkpoint40 stores,
    # not copied from native output. Later stores overwrite these same slots.
    known_words={0x8060C570,0x8060C5B0,0x8060C5F4}
    known_words.update(range(0x8060C578,0x8060C598,4))
    known_words.update(range(0x8060C5C0,0x8060C5F0,4))
    known={a+n for a in known_words for n in range(4)}
    stack_base=0x8060C570
    stack_mask="".join("1" if stack_base+n in known else "0" for n in range(144))
    paired_base=0x8060C5E8
    paired_mask="".join("1" if paired_base+n in known else "0" for n in range(16))
    fields=known_bytes=clock_fields=0
    for row,clock,ref in zip(rows,clocks,refs):
        if len(row)!=120 or len(clock)!=5:raise ValueError("malformed compiled state shape")
        wanted=[strict_hex(ref[k],8,k) for k in ("pc","msr","lr","cr","architectural_xer","fpscr")]
        wanted+=registers(ref)
        wanted+=[strict_hex(x,16,"PS0") for x in ref["ps0"]]
        wanted+=[strict_hex(x,16,"PS1") for x in ref["ps1"]]
        wanted+=[strict_hex(ref[k],8,k) for k in ("ctr","hid0","hid2")]
        wanted+=[strict_hex(x,8,"GQR") for x in ref["gqr"]]
        if len(wanted)!=113:raise ValueError("malformed reference register shape")
        widths=[8]*38+[16]*64+[8]*11
        values=[strict_hex(x,w,"compiled register") for x,w in zip(row[:113],widths)]
        if values!=wanted:raise ValueError(f"compiled state mismatch at {ref['pc']}")
        if row[114]!=paired_mask or row[117]!=stack_mask:raise ValueError("compiled stack validity lacks admitted producer")
        paired=bytes.fromhex(row[113]);stack=bytes.fromhex(row[116])
        if len(paired)!=16 or len(stack)!=144:raise ValueError("compiled stack shape differs")
        reference_stack=bytes.fromhex(ref["l2_stack_bytes"])
        if strict_hex(ref["l2_stack_address"],8,"stackbase")!=stack_base:raise ValueError("relabeled reference stack")
        reference_paired=bytes.fromhex(ref["paired_stack_bytes"])
        for n,valid in enumerate(stack_mask):
            if valid=="1":
                if stack[n]!=reference_stack[n]:raise ValueError("compiled known stack byte differs")
                known_bytes+=1
        for n,valid in enumerate(paired_mask):
            if valid=="1":
                if paired[n]!=reference_paired[n]:raise ValueError("compiled paired stack alias differs")
                if paired[n]!=stack[paired_base+n-stack_base]:raise ValueError("compiled stack views disagree")
                known_bytes+=1
        for index,key in ((115,"l2cr"),(118,"handler_slot"),(119,"lowmem_44")):
            if strict_hex(row[index],8,"compiled extra field")!=strict_hex(ref[key],8,key):raise ValueError("compiled extra state differs")
        p=ref["clock_producer"]
        expected_clock=[strict_hex(ref["pc"],8,"clockpc"),strict_hex(p["cycles"],16,"clockcycles"),
                        strict_hex(p["cached_tb"],16,"clockcache")]+[strict_hex(x,8,"clockglobal") for x in ref["clock_globals"]]
        if [strict_hex(x,w,"compiled clock") for x,w in zip(clock,(8,16,16,8,8))]!=expected_clock:
            raise ValueError("compiled clock phase/global differs")
        fields+=116;clock_fields+=5
    expected_stores=[]
    for previous,current in zip(states[first:],states[first+1:]):
        pc=int(previous["pc"],16);g=registers(previous)
        if pc in (0x80370EA8,0x80370EAC):
            low=pc==0x80370EA8;a=g[13]+(0x5A54 if low else 0x5A50);v=g[4 if low else 3]
            expected_stores.append([pc,a,v,strict_hex(current["clock_globals"][1 if low else 0],8,"store readback")])
        elif pc in (0x80373AB8,0x80373ABC):
            a=g[1]+4 if pc==0x80373AB8 else g[1]-8;v=g[0] if pc==0x80373AB8 else g[1]
            mem=bytes.fromhex(current["l2_stack_bytes"]);off=a-stack_base
            expected_stores.append([pc,a,v,int.from_bytes(mem[off:off+4],"big")])
    if any(len(s)!=4 for s in stores):raise ValueError("malformed compiled store ledger")
    if [[strict_hex(x,8,"compiled store") for x in s] for s in stores]!=expected_stores:
        raise ValueError("compiled store ordering/value/readback differs")
    expected_ranges=[]
    for r in report["region_extension"]["filled_ranges"]:
        address=strict_hex(r["address"],8,"rangeaddress")
        size=strict_hex(r["size"],8,"rangesize")
        data=bytearray.fromhex(r["bytes"])
        if len(data)!=size or any(data):raise ValueError("reference fill lacks complete zero producer")
        expected_ranges.append((address,data))
    final=states[-1]
    writes=list(zip((0x805F1F18,0x805F1F1C,0x805F1F40,0x805F1FF0),final["bi2_globals"]))
    writes+=list(zip((0x805F1F50,0x805F1F54),final["clock_globals"]))
    for a,text in writes:
        matches=[(base,data) for base,data in expected_ranges if base<=a and a+4<=base+len(data)]
        if len(matches)!=1:raise ValueError("reference current global lacks a unique producer")
        base,data=matches[0];data[a-base:a-base+4]=strict_hex(text,8,"current global").to_bytes(4,"big")
    if len(ranges)!=len(expected_ranges):raise ValueError("compiled current global owner count differs")
    owner_bytes=0
    for actual,(address,expected) in zip(ranges,expected_ranges):
        if len(actual)!=3 or strict_hex(actual[0],8,"range address")!=address or strict_hex(actual[1],8,"range size")!=len(expected):
            raise ValueError("compiled current global owner identity differs")
        if bytes.fromhex(actual[2])!=expected:raise ValueError("compiled current global byte differs")
        owner_bytes+=len(expected)
    if stop!=[["pc=0x80373ac4","reason=NO_NATIVE_CLOCK_PROVIDER"]]:raise ValueError("compiled output promotes unknown/native frontier")
    if not quiet:print(f"PASS compiled {capture_path.name}: {len(rows)} CP, {fields} state fields, {known_bytes} known stack bytes, {clock_fields} clock fields, {len(stores)} ordered stores, {owner_bytes} current global bytes")


def compiled_falsifications(capture_path,native_text):
    lines=native_text.splitlines()
    row_i=next(i for i,line in enumerate(lines) if line.startswith("8037962c "))
    clock_i=next(i for i,line in enumerate(lines) if line.startswith("CLOCK_RESEARCH "))
    store_i=next(i for i,line in enumerate(lines) if line.startswith("BOOT_STORE "))
    global_i=next(i for i,line in enumerate(lines) if line.startswith("RESEARCH_GLOBAL_RANGE "))
    mutations=[]
    for token in list(range(113))+[115,118,119]:mutations.append((row_i,token,"xor"))
    for token in range(1,6):mutations.append((clock_i,token,"xor"))
    for token in range(1,5):mutations.append((store_i,token,"xor"))
    mutations += [(row_i,114,"mask"),(row_i,117,"mask"),(row_i,113,"xor"),(row_i,116,"first"),(global_i,3,"first")]
    rejected=0
    for line_i,token,kind in mutations:
        parts=lines[line_i].split();old=parts[token]
        if kind=="xor":parts[token]=f"{int(old,16)^1:0{len(old)}x}"
        elif kind=="mask":parts[token]=("0" if old[0]=="1" else "1")+old[1:]
        else:parts[token]=f"{int(old[:2],16)^1:02x}"+old[2:]
        changed=lines.copy();changed[line_i]=" ".join(parts)
        try:compiled_ledger(capture_path,"\n".join(changed),True)
        except (ValueError,KeyError,AssertionError):rejected+=1
        else:raise AssertionError("compiled ledger admitted mutation")
    changed=lines.copy();a=changed[store_i];changed[store_i]=changed[store_i+1];changed[store_i+1]=a
    try:compiled_ledger(capture_path,"\n".join(changed),True)
    except ValueError:rejected+=1
    else:raise AssertionError("compiled ledger admitted reversed low/high stores")
    print(f"PASS compiled {capture_path.name}: {rejected} register/source/owner/validity/order mutations rejected")


def replay_compiled(dol,capture_path,native_path,executable):
    report=json.loads(capture_path.read_text(encoding="utf-8"))
    mode=producer_provenance(report)
    frontier=next(s for s in report["checkpoints"] if int(s["pc"],16)==0x80379628)
    p=frontier["clock_producer"]
    epoch=strict_hex(p["epoch_value"],16,"epochvalue")
    if mode=="rtc_initial":
        if epoch%40500000 or epoch//40500000>U32:raise ValueError("unexpected initialRTC producer")
        origin=f"{epoch//40500000:08x}";ack="research-only"
    else:
        origin=f"{epoch:016x}";ack="research-rebased-epoch"
    offset="".join(frontier["clock_low_words"][:2])
    fixture=native_path.with_name(native_path.name.replace(".research.txt",".bi2-reference.entry.txt"))
    completed=subprocess.run([str(executable),str(dol),str(fixture),origin,p["cpu_hz"],p["epoch_cycles"],p["cycles"],offset,p["cached_tb"],p["exceptions"],ack],
                             capture_output=True,text=True,check=True)
    if completed.stdout!=native_path.read_text(encoding="utf-8"):
        raise ValueError("stored compiled output differs from fresh independent replay")
    compiled_ledger(capture_path,completed.stdout)
    compiled_falsifications(capture_path,completed.stdout)
    print(f"PASS independent compiled replay; executableSHA256={hashlib.sha256(executable.read_bytes()).hexdigest()}")


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dol",type=Path)
    parser.add_argument("--show-words",action="store_true")
    parser.add_argument("--capture",type=Path,action="append",default=[])
    parser.add_argument("--native-output",type=Path,action="append",default=[])
    parser.add_argument("--native-executable",type=Path)
    args=parser.parse_args()
    raw_audit(args.dol,args.show_words)
    arithmetic_probes()
    producer_rollover_probes()
    ordering_and_owner_probes()
    if args.native_output and len(args.native_output)!=len(args.capture):parser.error("one native output per capture required")
    for n,capture in enumerate(args.capture):
        capture_ledger(capture)
        capture_falsifications(capture)
        if args.native_output:
            output=args.native_output[n]
            if args.native_executable:replay_compiled(args.dol,capture,output,args.native_executable)
            else:
                native_text=output.read_text(encoding="utf-8")
                compiled_ledger(capture,native_text)
                compiled_falsifications(capture,native_text)


if __name__ == "__main__":
    main()

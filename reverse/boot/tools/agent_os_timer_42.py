"""Independent raw ownership/DEC inventory for the PAL next OS dependencies.

Reads original DOL only. The report is static evidence, never a native runner,
clock replacement or frontier promotion. No guest memory is synthesized here.
"""
import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'gameplay' / 'tools'))
from dol import Dol
from ppc import decode

SHA256 = 'fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af'
APP_SHA256 = '8a1fe9487b029b2518a6633b1e0eb9ec0922f5f5eb5afdd7eba1267a85f096fe'
APP_SOURCES = {
    'Source/Core/Core/Boot/Boot_BS2Emu.cpp':'8c204a36bd7fe7171d6c8e03e7f07739c11297e483dd6180c6ff7391973ec06d',
    'Source/Core/Core/Core.h':'5bb9d817552da8f7dc04ae29dd9b29149edf6a6470d3db724d2f398b352390eb',
    'Source/Core/Core/HLE/HLE.cpp':'81f052fb94a0d3daebcb1d4fa956d71c0f7ba090de61ed4903999036189b45bc',
    'Source/Core/Core/HLE/HLE_OS.cpp':'8da9a89200e5f6c443c32bb7f872e0622d621bb289f9ffd183d5ead4c561e6e3',
    'Source/Core/Core/PowerPC/Interpreter/Interpreter.cpp':'1993e7a3976be7aa7d276f1e0b25e04bbdcbe1a4555fac837a4794f6bc18128d',
    'Source/Core/Core/PowerPC/Interpreter/Interpreter_Branch.cpp':'ac92c3fc9f5d05b7bc6fea7a39bdb0682c13d80223fc8ca88131a55a6df29068',
    'Source/Core/Core/HW/SI/SI.cpp':'4dd9a3b22f3610d7ffe7704b6ee5b4142f29ad6e932c391edc094b22ac6abae7',
    'Source/Core/Core/HW/SI/SI.h':'00c23198d5541f76e010eb3119442bb4a37cc8c2d8db2ebff143956860783205',
    'Source/Core/Core/HW/Memmap.cpp':'c8b5b5ea180c73f54736b6835e20d8027d0203cda7641349254fbce99bb2747f',
    'Source/Core/Core/HW/HW.cpp':'173015f302d96ba801efe7c47dbf9bcb572955d6ed116ff61d51215325c151a0',
    'Source/Core/Core/Core.cpp':'8bf9e90fc56b5b4d7a1e42410ea2e2291f44b5d86fb4e982b3aa68912edf5944',
}
TARGETS = (0x80373ab4, 0x80373bb8, 0x80371810, 0x80371a60, 0x80371ac8,
           0x80371e14, 0x803717a8, 0x80370b0c, 0x80372234, 0x80378320)
POINTERS = {*TARGETS, 0x800030f0, 0xc00030f0, 0x000030f0,
            0x80586c90, 0xc0586c90, 0x00586c90}
RANGES = ((0x80373ab4, 0x80373af4), (0x800054f4, 0x80005544),
          (0x803717a8, 0x80371810), (0x80370b0c, 0x80370b14),
          (0x80372214, 0x80372260), (0x803771b8, 0x803771f0))
GOLDEN = {
    0x80373ac0:0x3ca08000, 0x80373ac4:0x808530f0,
    0x80373ac8:0x7c052040, 0x80373acc:0x41810010,
    0x80373ad0:0x38a0001c, 0x80373ad4:0x4bc91a21,
    0x80373adc:0x38000000, 0x80373ae0:0x90030000,
    0x800054f4:0x7c041840, 0x800054f8:0x41800028,
    0x8000550c:0x8c040001, 0x80005510:0x9c060001,
    0x80005530:0x8c04ffff, 0x80005534:0x9c06ffff,
    0x80373ea4:0x936430f0, 0x80373f6c:0x936430f0,
    0x80370b0c:0x7c7603a6, 0x8039f71c:0x7ff602a6,
    0x8039f7f8:0x7f5603a6,
}


def branch(pc, word):
    op = word >> 26
    if op == 18:
        disp = word & 0x03fffffc
        if disp & 0x02000000: disp -= 0x04000000
    elif op == 16:
        disp = word & 0xfffc
        if disp & 0x8000: disp -= 0x10000
    else: return None
    return {'target': (disp if word & 2 else pc+disp) & 0xffffffff,
            'link':bool(word&1), 'absolute':bool(word&2)}


def row(dol, pc):
    w = dol.u32(pc)
    s = dol.find(pc)
    return {'pc':f'{pc:08X}', 'file_offset':f'{s.file_off+pc-s.addr:06X}',
            'word':f'{w:08X}', 'display':decode(w,pc).text()}


def scan(dol):
    lexical = []
    dec = []
    direct = defaultdict(list)
    indirect = []
    count = 0
    for s in dol.text:
        for pc in range(s.addr,s.end-3,4):
            w = dol.u32(pc); count += 1
            if (w & 0xffff) in (0x30f0,0xb0f0,0x6c90,0x9370):
                lexical.append(row(dol,pc))
            if w >> 26 == 31 and (w >> 1) & 1023 in (339,467):
                spr = ((w>>16)&31)|(((w>>11)&31)<<5)
                if spr == 22: dec.append(row(dol,pc))
            b = branch(pc,w)
            if b and b['target'] in TARGETS:
                direct[f"{b['target']:08X}"].append({**row(dol,pc),**b})
            # BCLR with link, any BO/BI/BH bits; retains conditional indirects.
            if w >> 26 == 19 and (w >> 1)&1023 in (16,528) and w&1:
                if 0x803717a8 <= pc < 0x80371e64 or 0x80373ab4 <= pc < 0x8037403c or 0x80378324 <= pc < 0x8037847c:
                    indirect.append(row(dol,pc))
    pointers=[]
    for s in dol.sections:
        for pc in range(s.addr,s.end-3,4):
            w=dol.u32(pc)
            if w in POINTERS:
                pointers.append({'address':f'{pc:08X}','word':f'{w:08X}',
                                 'section':s.key,'text':s.is_text})
    return {'sha256':hashlib.sha256(dol.raw).hexdigest(),'text_words':count,
            'lexical_pointer_candidates':lexical,'all_raw_DEC_forms':dec,
            'direct_edges':dict(direct),'aligned_pointer_words':pointers,
            'bounded_indirect_call_sites':indirect,
            'raw_bounded_ledgers':[[row(dol,pc) for pc in range(a,b,4)] for a,b in RANGES],
            'vector_offsets':[f'{dol.u32(p):08X}' for p in range(0x8056121c,0x80561258,4)],
            'callback_default_word':f'{dol.u32(0x805eec10):08X}',
            'scope':'all aligned DOL text words for DEC/direct edges/lexical candidates; scoped indirect sites; pointer words in all DOL sections; no dynamic alias or target coverage claim'}


def check(dol,r):
    if r['sha256'] != SHA256: raise ValueError('PAL authority digest mismatch')
    for pc,w in GOLDEN.items():
        if dol.u32(pc)!=w: raise ValueError(f'raw authority mismatch at {pc:08X}')
    expected_dec={0x80370b0c:0x7c7603a6,0x8039f71c:0x7ff602a6,0x8039f7f8:0x7f5603a6}
    assert {int(x['pc'],16):int(x['word'],16) for x in r['all_raw_DEC_forms']}==expected_dec
    assert len([x for x in r['direct_edges']['80370B0C'] if x['link']])==15
    assert [x['pc'] for x in r['direct_edges']['80373AB4']]==['80370EB8']
    assert [x['pc'] for x in r['direct_edges']['80373BB8']]==['803741B8']
    assert branch(0x80373acc,0x41810010)['target']==0x80373adc
    assert branch(0x800054f8,0x41800028)['target']==0x80005520
    assert r['vector_offsets'][8]=='00000900'
    assert r['callback_default_word']=='80378320'
    print('PASS PAL pointer/copy raw ledger, whole-text DEC inventory, direct counts and dispatch provenance')


def app_audit(dol, image, source_root):
    """Count one explicitly bounded semantic path; never execute/decode PPC.

    There is no instruction state, generic opcode dispatch, guest memory or
    captured count input. Loops are arithmetic counts of audited raw blocks.
    Default-HLE path predicates remain part of the returned admission scope.
    """
    raw = image.read_bytes()
    if hashlib.sha256(dol.raw).hexdigest() != SHA256:
        raise ValueError('DOL authority digest mismatch')
    if hashlib.sha256(raw).hexdigest() != APP_SHA256:
        raise ValueError('apploader authority digest mismatch')
    pins = {}
    for rel, expected in APP_SOURCES.items():
        got = hashlib.sha256((source_root/rel).read_bytes()).hexdigest()
        if got != expected: raise ValueError('source authority mismatch: '+rel)
        pins[rel] = got
    bs2=(source_root/'Source/Core/Core/Boot/Boot_BS2Emu.cpp').read_text(encoding='utf-8')
    gc_begin=bs2.index('void CBoot::SetupGCMemory(')
    gc_end=bs2.index('bool CBoot::EmulatedBS2_GC(',gc_begin)
    wii_begin=bs2.index('bool CBoot::SetupWiiMemory(')
    explicit_zero=bs2.index('memory.Write_U32(0x00000000, 0x000030f0)')
    assert explicit_zero>wii_begin and not(gc_begin<=explicit_zero<gc_end)
    def word(pc): return struct.unpack_from('>I',raw,pc-0x81200000+32)[0]
    def span(a,b):
        if a&3 or b&3 or b<a: raise ValueError('invalid bounded block')
        return (b-a)//4+1
    def cache(addr,n,kind):
        if not n: return 2
        lines = (n+(addr&31)+31)//32
        # Prefix has 7 instructions; terminal effects: blr, sc/rfi/blr,
        # or sync/isync/blr. SC's physically installed RFI is one more step.
        return 3*lines+{'dcbi':8,'dcbf':10,'dcbst':10,'icbi':10}[kind]
    def fill(addr,n,value=0):
        if n<32: return 12+7+(2+3*n if n else 0)
        k=(-addr)&3
        left=n-k
        return (12+20+(2+3*k if k else 0)+(6 if value else 0)
                +10*(left//32)+3*((left//4)&7)
                +(2+3*(left&3) if left&3 else 0))
    def copy(n): return 4*n+9

    assert struct.unpack_from('>III',raw,16)==(0x81200258,0x1a98,0x1c3a0)
    assert word(0x81200210)==0x3ca00010 and word(0x81200230)==0x7c0037ac
    assert word(0x81200238)==0x4200fff8 and word(0x81200250)==0x4e800020
    lexical=[]
    for off in range(32,len(raw)-3,4):
        w=struct.unpack_from('>I',raw,off)[0]
        if w&0xffff in (0x30f0,0xb0f0):
            lexical.append({'pc':f'{0x81200000+off-32:08X}','word':f'{w:08X}',
                            'inside_header_code':off<32+0x1a98})
    assert [x for x in lexical if x['inside_header_code']]==[
        {'pc':'81201234','word':'808530F0','inside_header_code':True}]
    boot=image.with_name('boot.bin').read_bytes()
    bi2=image.with_name('bi2.bin').read_bytes()
    if hashlib.sha256(boot).hexdigest()!='7d387ce9162342a93a3becac1d2df6db1cccf6a2439f3fc97b29b2f7047d8e88':
        raise ValueError('boot.bin authority mismatch')
    if hashlib.sha256(bi2).hexdigest()!='8bbadffc2a7f79eda5982aa2fe30831969c2702aa9206f621b930aae0b70af6b':
        raise ValueError('bi2.bin authority mismatch')
    fst_n = struct.unpack_from('>I',boot,0x428)[0]
    dol_offset,fst_offset = struct.unpack_from('>II',boot,0x420)
    sim_memory=struct.unpack_from('>I',bi2,4)[0]
    assert dol_offset<fst_offset and sim_memory==0x01800000
    assert struct.unpack_from('>I',bi2,0x28)[0]==0
    assert all(word(p)==0 for p in range(0x81201a4c,0x81201a68,4))
    text=list(dol.text)
    data=[s for s in dol.sections if not s.is_text]
    nt,nd=len(text),len(data)
    assert (nt,nd)==(2,8)
    assert all(s.addr<0x81100000 and s.end<=0x80700000 and not(s.addr&31) for s in dol.sections)
    assert dol.bss_addr<0x81100000 and dol.bss_addr+dol.bss_size<=0x80700000
    fst_base=(0x80000000+sim_memory-fst_n)&~31
    fst_read=(fst_n+31)&~31
    assert fst_read<=0x20000 and fst_base>=0x81700000

    # Thunk extent is source end minus source start, excluding end's nop.
    thunk_bytes=0x81200254-0x81200208
    thunk=span(0x81200208,0x8120022c)+3*(0x100000//32)+span(0x8120023c,0x81200250)
    wrapper=span(0x81200258,0x81200274)
    outer=wrapper+thunk
    prologue=span(0x812007c4,0x8120081c)+2 # call8120115C
    yes_exit=span(0x81201120,0x81201134)
    no_exit=span(0x81201124,0x81201134)
    main_base=outer+prologue+yes_exit
    # Raw footprint loop: each nonempty descriptor has 14 instructions;
    # empty descriptor 8. Each loop's pre-test and final failed test included.
    footprint=7+2+nt*14+(7-nt)*8+2+2+nd*14+(11-nd)*8+4
    # Supplied extents are below81100000 and both tested limits; every nonempty
    # descriptor takes its admitted 23-instruction branch, empty takes8.
    bounds=6+2+nt*23+(7-nt)*8+2+2+nd*23+(11-nd)*8+13
    entry_body=(span(0x8120039c,0x81200414)+1
                +span(0x8120044c,0x81200464)+span(0x81200468,0x81200484))
    entry=(outer+entry_body+copy(thunk_bytes)
           +cache(0x812fff80,thunk_bytes,'dcbst')+cache(0x812fff80,thunk_bytes,'icbi'))
    init=outer+44+fill(0x81201920,0x20)+fill(0x81201940,0x100)+13+2
    # The default-HLE LatestDevkit word10000006 takes BOTH range checks.
    state6_fixed=sum((span(0x81200c30,0x81200c48),span(0x81200c9c,0x81200ca4),
                      span(0x81200ca8,0x81200cc0),span(0x81200ccc,0x81200ce4),
                      span(0x81200d2c,0x81200d38),span(0x81200d54,0x81200d64),
                      1,span(0x81200d94,0x81200db8)))
    state6=(state6_fixed+footprint+15+2*bounds+fill(dol.bss_addr,dol.bss_size)
            +cache(dol.bss_addr,dol.bss_size,'dcbf'))
    main=[]
    main.append(main_base+12+cache(0x81201920,0x20,'dcbi'))
    main.append(main_base+19+1+cache(0x81201a80,0x20,'dcbi'))
    main.append(main_base+65+cache(fst_base-0x2000,0x2000,'dcbi'))
    main.append(main_base+20+12+cache(0x81201940,0x100,'dcbi'))
    for i,s in enumerate(text):
        main.append(main_base+(state6 if i==0 else 0)+46+cache(s.addr,s.size,'dcbi'))
    # The index7 sentinel reads the first nonzero data file offset.
    text_exit=12*(7-nt)+14
    for i,s in enumerate(data):
        main.append(main_base+(text_exit if i==0 else 0)+46+cache(s.addr,s.size,'dcbi'))
    # Index11 sentinel reads text0's nonzero RAM address.
    data_exit=12*(11-nd)+14
    main.append(main_base+data_exit+28+cache(fst_base,fst_read,'dcbi')+14)
    main.append(outer+prologue+no_exit+7+8+20+12)
    close=outer+4
    result={'app_sha256':APP_SHA256,'source_pins':pins,
            'premises':['GC HLE, 24MiB physical RAM, no Riivolution patches or foreign mutation',
                        'initial800030F0<80000000, apploader-loaded state words zero',
                        'LatestDevkit word10000006; SI Init poll492<<16 retained with EN3=0',
                        'original BE boot/BI2/DOL header descriptors; no unexpected exception or hook'],
            'callback_wrappers':wrapper,'return_thunk_bytes':thunk_bytes,
            'return_thunk_steps':thunk,'byte_copy_formula':'4*N+9',
            'cache_formulas':{'dcbi':'8+3*ceil((N+(addr&31))/32)',
                              'dcbf_dcbst_with_default_rfi':'10+3*ceil((N+(addr&31))/32)',
                              'icbi':'10+3*ceil((N+(addr&31))/32)'},
            'state6':{'fixed_blocks':state6_fixed,'footprint_loop':footprint,
                      'each_bounds_loop':bounds,'BSS_fill':fill(dol.bss_addr,dol.bss_size),
                      'BSS_flush':cache(dol.bss_addr,dol.bss_size,'dcbf'),
                      'getter':15,'total':state6},
            'f0_ownership':{'fresh_GC_producer':'MemoryManager::Init -> Clear: memset(m_ram,0,GetRamSize())',
                            'explicit_BS2_30F0_zero_function':'SetupWiiMemory, not admitted GC SetupGCMemory',
                            'all_image_lexical_30F0_candidates':lexical,
                            'normal_callback_30F0_effect':'81201234 read only; no admitted trailer call',
                            'zero_survival_conditions':['fresh memory initialization, no savestate restore or foreign RAM mutation',
                                                        'exact admitted original requests/stack/helper writes exclude physical30F0',
                                                        'validated DOL prefix excludes later transfer producers80373EA4/F6C'],
                            'status':'private fresh GC HLE profile only; retail/reboot history unresolved'},
            'entry_steps':entry,'init_steps':init,'main_steps':main,'close_steps':close,
            'total_steps':entry+init+sum(main)+close,
            'status':'conditional analytical reconstruction; captured totals are validation only'}
    if len(main)!=16: raise ValueError('ordinary callback topology changed')
    return result


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('dol',type=Path);p.add_argument('--report',type=Path)
    p.add_argument('--check',action='store_true')
    p.add_argument('--apploader',type=Path)
    p.add_argument('--source-root',type=Path)
    a=p.parse_args();d=Dol(a.dol);r=scan(d)
    if a.check:check(d,r)
    if a.apploader:
        if not a.source_root: raise ValueError('--apploader requires --source-root')
        r['apploader_analytical_steps']=app_audit(d,a.apploader,a.source_root)
        print('ANALYTICAL conditional apploader steps:',r['apploader_analytical_steps']['total_steps'])
    if a.report:
        if not a.report.resolve().is_relative_to(ROOT.resolve()):raise ValueError('report must stay in repository')
        a.report.parent.mkdir(parents=True,exist_ok=True)
        a.report.write_text(json.dumps(r,indent=2)+'\n',encoding='utf-8')
    elif not a.check:
        print(json.dumps({k:v for k,v in r.items() if k!='raw_bounded_ledgers'},indent=2))


if __name__=='__main__':main()

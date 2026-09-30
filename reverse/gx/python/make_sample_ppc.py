#!/usr/bin/env python3
import struct, sys

def D(op,rt,ra,imm): return (op<<26)|(rt<<21)|(ra<<16)|(imm & 0xffff)
# Two exact WGPIPE stores plus one nearby MMIO-looking store that MUST NOT match.
# lis r12,0xCC01 ; li r3,0x61 ; stb r3,-0x8000(r12) ; li r4,0x1234 ;
# sth r4,-0x8000(r12) ; stw r4,-0x7ffc(r12) -> 0xCC008004 (negative test)
words=[D(15,12,0,0xCC01),D(14,3,0,0x61),D(38,3,12,-0x8000),D(14,4,0,0x1234),D(44,4,12,-0x8000),D(36,4,12,-0x7ffc)]
path=sys.argv[1] if len(sys.argv)>1 else 'sample_ppc.bin'
open(path,'wb').write(b''.join(struct.pack('>I',w) for w in words))
print(f'wrote {len(words)} PPC words to {path}')

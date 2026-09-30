"""Who constructs CharCommands with type ids in a range (default 0x201-0x20C, the enemy-AI commands)?
usage: python agent_enemy2_cmdsend.py [LO HI]
1) every call to CharCommand::CharCommand (0x8006E57C) with its r4 (type) value from the dataflow;
2) raw scan of every 'li rX, IMM' / 'addi rX, rY, IMM' / 'cmpwi/cmplwi rX, IMM' with IMM in range.
"""
import sys, pickle, os
import program as P
from dol import DATA_DIR
from symbols import get_symbols
from ppc import decode

CTOR = 0x8006E57C
if __name__ == '__main__':
    lo = int(sys.argv[1], 16) if len(sys.argv) > 1 else 0x201
    hi = int(sys.argv[2], 16) if len(sys.argv) > 2 else 0x20C
    S = get_symbols(); p = P.load()
    with open(os.path.join(DATA_DIR, 'dataflow.pkl'), 'rb') as f:
        df = pickle.load(f)
    print('== calls to CharCommand::CharCommand with non-constant or in-range type ==')
    n = 0
    for fn, recs in df.items():
        for rec in recs.get('call', []):
            pc, tgt, args = rec[0], rec[1], rec[2]
            if tgt != CTOR:
                continue
            n += 1
            r4 = args[1] if len(args) > 1 else None
            if r4 is None or r4[0] != 'k' or lo <= r4[1] <= hi:
                print('  %08X in %-50s r4=%s' % (pc, S.label(fn)[:50], r4))
    print('  (%d ctor calls total)' % n)
    print('== immediates in range ==')
    for (s, e) in p.funcs:
        a = s
        while a < e:
            w = p.word(a)
            if w is None:
                break
            ins = decode(w, a)
            m = ins.m
            imm = getattr(ins, 'imm', None)
            if imm is not None and lo <= (imm & 0xFFFF) <= hi and m in ('li', 'addi', 'cmpwi', 'cmplwi', 'subi', 'ori'):
                if m == 'addi' and ins.ra in (1, 2, 13):
                    pass
                else:
                    print('  %08X %-44s %s' % (a, S.label(s)[:44], ins.text()))
            a += 4

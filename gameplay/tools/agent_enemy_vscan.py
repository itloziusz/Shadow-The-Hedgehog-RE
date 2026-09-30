"""Scan code for virtual-call sites with a given slot byte offset.
usage: python agent_enemy_vscan.py SLOTOFF [LO HI] [--vp VPOFF] [--grep SUBSTR]
Linear per-function register-expression tracking (approximate; resets at bl).
Prints: pc  function  expr-of-object  vptr-disp  slot-off
"""
import sys
import program as P
from ppc import decode
from symbols import get_symbols


def scan(slotoff, lo=0x80000000, hi=0x81000000, vp=None, grep=None):
    p = P.load()
    S = get_symbols()
    out = []
    for (s, e) in p.funcs:
        if s < lo or s >= hi:
            continue
        reg = {i: 'r%d' % i for i in range(3, 11)}
        reg[1] = 'sp'
        a = s
        while a < e:
            w = p.word(a)
            if w is None:
                break
            ins = decode(w, a)
            m = ins.m
            try:
                if m == 'mr':
                    # mr rA, rS encoded as or rA,rS,rS : ra=dest, rd=src
                    reg[ins.ra] = reg.get(ins.rd, '?')
                elif m == 'addi' and ins.ra != 0:
                    reg[ins.rd] = '(%s+%X)' % (reg.get(ins.ra, '?'), ins.imm) if ins.imm >= 0 else '(%s-%X)' % (reg.get(ins.ra, '?'), -ins.imm)
                elif m == 'lwz':
                    b = reg.get(ins.ra, '?')
                    if ins.rd == 12 and ins.ra == 12 and b.startswith('VP:'):
                        base, vpo = b[3:].rsplit('@', 1)
                        if ins.imm == slotoff and (vp is None or int(vpo, 16) == vp):
                            txt = '%08X  %-50s obj=%s  vptr@+%s  slot+%X' % (a, S.label(s)[:50], base, vpo, ins.imm)
                            if grep is None or grep in txt:
                                out.append(txt)
                        reg[12] = '?'
                    elif ins.rd == 12:
                        reg[12] = 'VP:%s@%X' % (b, ins.imm)
                    else:
                        reg[ins.rd] = '[%s+%X]' % (b, ins.imm)
                elif m in ('bl',):
                    for i in range(3, 13):
                        reg[i] = '?'
                    reg[3] = 'ret'
                elif m.startswith('li') or m.startswith('lis') or m.startswith('lbz') or m.startswith('lhz'):
                    reg[ins.rd] = '?'
            except Exception:
                pass
            a += 4
    return out


if __name__ == '__main__':
    args = sys.argv[1:]
    vp = None
    grep = None
    if '--vp' in args:
        i = args.index('--vp'); vp = int(args[i + 1], 16); del args[i:i + 2]
    if '--grep' in args:
        i = args.index('--grep'); grep = args[i + 1]; del args[i:i + 2]
    slot = int(args[0], 16)
    lo = int(args[1], 16) if len(args) > 1 else 0x80000000
    hi = int(args[2], 16) if len(args) > 2 else 0x81000000
    for l in scan(slot, lo, hi, vp, grep):
        print(l)

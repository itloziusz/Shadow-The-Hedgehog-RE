"""Player-investigation helper (agent_player_*). Read-only on shared caches.

  python agent_player_q.py win ADDR [n]      annotated disasm of n instrs starting at ADDR (inside its function)
  python agent_player_q.py f   ADDR [ADDR..] print f32 values at addresses
  python agent_player_q.py vt  ADDR n        dump n u32 words at ADDR with symbol labels
  python agent_player_q.py fnsz ADDR         function start/end
"""
import sys
from collections import defaultdict
import q
from q import prog, df, fend, describe_addr, fmtv
from dol import get_dol
import program as P
from symbols import get_symbols


def win(addr, n=40):
    a0 = int(addr, 16)
    f = P.func_of(prog(), a0)
    S = get_symbols()
    e = fend(f) if f is not None else a0 + 4 * n
    recs = df().get(f, {}) if f is not None else {}
    ann = defaultdict(list)
    for pc, base, disp, size, st, fp, val in recs.get('acc', []):
        if base is None:
            continue
        if base[0] == 'k':
            t = describe_addr((base[1] + disp) & 0xFFFFFFFF)
            if t:
                ann[pc].append(t)
        elif base[0] in ('a', 'l', '+', 'ret'):
            ann[pc].append(fmtv(base) + '+%X' % disp)
    for pc, a in recs.get('cptr', []):
        t = describe_addr(a)
        if t:
            ann[pc].append('=' + t)
    for pc, ctr, args in recs.get('icall', []):
        ann[pc].append('icall ' + fmtv(ctr))
    print('%s  %08X-%08X' % (S.label(f) if f else '?', f or 0, e))
    a = a0
    for _ in range(int(n)):
        x = prog().ins(a)
        txt = x.text()
        if x.target is not None and f is not None and not (f <= x.target < e):
            txt = '%s %s' % (x.m, S.label(x.target))
        extra = ('   ; ' + ' | '.join(ann[a])) if a in ann else ''
        print('  %08X  %-40s%s' % (a, txt, extra))
        a += 4


def fvals(*addrs):
    d = get_dol()
    for s in addrs:
        a = int(s, 16)
        u = d.u32(a)
        print('%08X  %s  f32=%r  u32=%s' % (a, d.region(a), d.f32(a), ('%08X' % u) if u is not None else 'n/a (bss)'))


def vt(addr, n):
    d = get_dol()
    S = get_symbols()
    a = int(addr, 16)
    for i in range(int(n)):
        v = d.u32(a + 4 * i)
        lab = S.label(v) if d.is_code(v) else (describe_addr(v) if v and d.region(v) else '')
        print('  +%03X %08X  %08X  %s' % (4 * i, a + 4 * i, v, lab or ''))


if __name__ == '__main__':
    cmd = sys.argv[1]
    if cmd == 'win':
        win(*sys.argv[2:])
    elif cmd == 'f':
        fvals(*sys.argv[2:])
    elif cmd == 'vt':
        vt(*sys.argv[2:])
    elif cmd == 'fnsz':
        a = int(sys.argv[2], 16)
        f = P.func_of(prog(), a)
        print('%08X-%08X' % (f, fend(f)))

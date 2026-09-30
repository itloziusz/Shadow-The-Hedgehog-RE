"""Weapon/damage investigation helper (agent_weapon_*). Read-only on shared caches.

  python agent_weapon_q.py grep RANGE_LO RANGE_HI REGEX   raw-disasm lines matching REGEX in [lo,hi) (hex)
  python agent_weapon_q.py callargs TARGET                 every direct call to TARGET with symbolic args r3..r8,f1..f2
  python agent_weapon_q.py words ADDR N                    N u32 words (with f32 view + labels)
  python agent_weapon_q.py funcs LO HI                     functions in [lo,hi) with size and symbol
  python agent_weapon_q.py accs FUNC                       all memory accesses of FUNC (pc, base, disp, size, W/R, value)
  python agent_weapon_q.py fk FUNC                         float constants (sdata2/rodata) loaded by FUNC with pcs
"""
import re
import sys
import q
from q import prog, df, fend, describe_addr, fmtv, index
from dol import get_dol
import program as P
from symbols import get_symbols


def h(s):
    return int(s, 16)


def cmd_grep(lo, hi, pat):
    rx = re.compile(pat)
    pr = prog()
    S = get_symbols()
    a = h(lo)
    e = h(hi)
    while a < e:
        x = pr.ins(a)
        t = x.text()
        if rx.search(t):
            f = P.func_of(pr, a)
            print('  %08X  %-36s in %s' % (a, t, S.label(f) if f else '?'))
        a += 4


def cmd_callargs(target):
    S = get_symbols()
    t = S.lookup(target)
    idx = index()
    for c, pc in sorted(idx['callers'].get(t, [])):
        args = None
        for p2, tt, aa in df().get(c, {}).get('call', []):
            if p2 == pc:
                args = aa
        print('  %08X  %-44s (%s)' % (pc, S.label(c), ', '.join(fmtv(v) for v in (args or [])[:8])))


def cmd_words(addr, n):
    d = get_dol()
    S = get_symbols()
    a = h(addr)
    for i in range(int(n)):
        v = d.u32(a + 4 * i)
        if v is None:
            print('  %08X  (bss)' % (a + 4 * i))
            continue
        lab = S.label(v) if d.is_code(v) else (describe_addr(v) if v and d.region(v) else '')
        fv = d.f32(a + 4 * i)
        fs = ('%g' % fv) if fv is not None and fv == fv and (fv == 0 or 1e-4 < abs(fv) < 1e7) else ''
        print('  +%03X %08X  %08X  %-10s %s' % (4 * i, a + 4 * i, v, fs, lab or ''))


def cmd_funcs(lo, hi):
    pr = prog()
    S = get_symbols()
    for f, e in pr.funcs:
        if h(lo) <= f < h(hi):
            print('  %08X-%08X %5X  %s' % (f, e, e - f, S.label(f)))


def cmd_accs(name):
    S = get_symbols()
    f = S.lookup(name)
    f = P.func_of(prog(), f)
    for pc, base, disp, size, st, fp, val in sorted(df().get(f, {}).get('acc', [])):
        print('  %08X  %s %-24s +%-5X %d%s  %s' % (pc, 'W' if st else 'R', fmtv(base), disp, size, 'f' if fp else '',
                                                  fmtv(val) if st else ''))


def cmd_fk(name):
    S = get_symbols()
    d = get_dol()
    f = P.func_of(prog(), S.lookup(name))
    for pc, base, disp, size, st, fp, val in sorted(df().get(f, {}).get('acc', [])):
        if base is not None and base[0] == 'k' and fp:
            a = (base[1] + disp) & 0xFFFFFFFF
            print('  %08X  %s %08X  %r' % (pc, 'W' if st else 'R', a, d.f32(a)))


def cmd_dsearch(*vals):
    """Find aligned u32 words equal to any VALUE (hex) in non-text sections; VALUE may be LO-HI range."""
    d = get_dol()
    rngs = []
    for v in vals:
        if '-' in v:
            lo, hi = v.split('-')
            rngs.append((h(lo), h(hi)))
        else:
            rngs.append((h(v), h(v) + 1))
    import struct
    for s in d.sections:
        if s.is_text:
            continue
        raw = d.raw[s.file_off:s.file_off + s.size]
        for i in range(0, len(raw) - 3, 4):
            w = struct.unpack('>I', raw[i:i + 4])[0]
            for lo, hi in rngs:
                if lo <= w < hi:
                    print('  %08X  %s  = %08X' % (s.addr + i, s.name, w))


if __name__ == '__main__':
    c = sys.argv[1]
    a = sys.argv[2:]
    {'grep': cmd_grep, 'callargs': cmd_callargs, 'words': cmd_words, 'funcs': cmd_funcs,
     'accs': cmd_accs, 'fk': cmd_fk, 'dsearch': cmd_dsearch}[c](*a)

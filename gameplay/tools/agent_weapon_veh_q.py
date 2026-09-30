"""Vehicle-investigation helper (agent_weapon_veh_*). Read-only on shared caches.

  python agent_weapon_veh_q.py funcs START END          functions in [START,END): size, #callers, name
  python agent_weapon_veh_q.py words ADDR n             n words: hex, f32, label
  python agent_weapon_veh_q.py rows ADDR stride n [w]   n rows of `stride` bytes, first w words each (hex/f32)
  python agent_weapon_veh_q.py fconst FUNC..            float constants (r2/r13/abs) loaded by the function(s), with pc
  python agent_weapon_veh_q.py icalls FUNC..            indirect calls with symbolic ctr value
"""
import sys
import struct
import q
from q import prog, df, fend, describe_addr, fmtv, index
from dol import get_dol
import program as P
from symbols import get_symbols


def funcs(a, b):
    a, b = int(a, 16), int(b, 16)
    S = get_symbols()
    cal = index()['callers']
    for f, e in prog().funcs:
        if a <= f < b:
            print('%08X-%08X %5X  callers=%-3d %s' % (f, e, e - f, len(cal.get(f, [])), S.label(f)))


def _f(u):
    return struct.unpack('>f', struct.pack('>I', u))[0]


def words(addr, n):
    d = get_dol()
    S = get_symbols()
    a = int(addr, 16)
    for i in range(int(n)):
        u = d.u32(a + 4 * i)
        if u is None:
            print('  +%03X %08X  (bss)' % (4 * i, a + 4 * i))
            continue
        lab = ''
        if d.is_code(u):
            lab = S.label(u)
        elif u and d.region(u):
            lab = describe_addr(u) or ''
        fv = _f(u)
        fs = ('%g' % fv) if (u & 0x7F800000) not in (0, 0x7F800000) and abs(fv) < 1e7 else ''
        print('  +%03X %08X  %08X  %-12s %s' % (4 * i, a + 4 * i, u, fs, lab))


def rows(addr, stride, n, w=None):
    d = get_dol()
    a, stride, n = int(addr, 16), int(stride, 16), int(n)
    w = int(w) if w else stride // 4
    for r in range(n):
        base = a + r * stride
        out = []
        for i in range(w):
            u = d.u32(base + 4 * i)
            fv = _f(u)
            if (u & 0x7F800000) not in (0, 0x7F800000) and abs(fv) < 1e7 and u > 0x10000000:
                out.append('%g' % fv)
            else:
                out.append('%X' % u if u > 0xFFFF else str(u if u < 0x80000000 else u - (1 << 32)))
        print('%08X [%d]: %s' % (base, r, ' '.join(out)))


def fconst(*fs):
    d = get_dol()
    for s in fs:
        f = q.resolve(s)
        recs = df().get(f, {})
        for pc, base, disp, size, st, fp, val in recs.get('acc', []):
            if base is not None and base[0] == 'k' and fp and not st:
                ad = (base[1] + disp) & 0xFFFFFFFF
                u = d.u32(ad)
                print('%08X  %08X  %s  %s' % (pc, ad, ('%g' % _f(u)) if u is not None else 'bss', d.region(ad)))


def icalls(*fs):
    for s in fs:
        f = q.resolve(s)
        for pc, ctr, args in df().get(f, {}).get('icall', []):
            print('%08X  %s' % (pc, fmtv(ctr)))


if __name__ == '__main__':
    cmd = sys.argv[1]
    if cmd not in ("grep", "tbl"):
        {"funcs": funcs, "words": words, "rows": rows, "fconst": fconst, "icalls": icalls}[cmd](*sys.argv[2:])


def grep(start, end, pattern):
    """grep START END REGEX: instructions matching REGEX in [START,END), grouped by function."""
    import re
    a, b = int(start, 16), int(end, 16)
    rx = re.compile(pattern)
    S = get_symbols()
    for f, e in prog().funcs:
        if not (a <= f < b):
            continue
        hits = []
        pc = f
        while pc < e:
            t = prog().ins(pc).text()
            if rx.search(t):
                hits.append('%08X %s' % (pc, t))
            pc += 4
        if hits:
            print('%s (%08X)' % (S.label(f), f))
            for h in hits:
                print('    ' + h)


if __name__ == '__main__' and sys.argv[1] == 'grep':
    grep(*sys.argv[2:])


def tbl(base, stride, start, end):
    """tbl BASE STRIDE START END: linear register tracking of `BASE + idx*STRIDE + off` accesses per function."""
    TB, ST = int(base, 16), int(stride, 16)
    a0, b0 = int(start, 16), int(end, 16)
    S = get_symbols()
    for f, e in prog().funcs:
        if not (a0 <= f < b0):
            continue
        R = {}
        out = []
        pc = f
        while pc < e:
            x = prog().ins(pc)
            m, ops = x.m, x.ops
            def reg(o):
                return R.get(o[1]) if o and o[0] == 'r' else None
            try:
                if m == 'lis':
                    R[ops[0][1]] = ('k', (ops[1][1] << 16) & 0xFFFFFFFF)
                elif m in ('addi', 'addic') and ops[1][0] == 'r':
                    v = R.get(ops[1][1]); imm = ops[2][1]
                    if ops[1][1] == 0 and m == 'addi':
                        R[ops[0][1]] = ('k', imm & 0xFFFFFFFF)
                    elif v and v[0] == 'k':
                        R[ops[0][1]] = ('k', (v[1] + imm) & 0xFFFFFFFF)
                    elif v and v[0] in ('m', 'row'):
                        R[ops[0][1]] = (v[0], v[1] + imm)
                    else:
                        R.pop(ops[0][1], None)
                elif m == 'li':
                    R[ops[0][1]] = ('k', ops[1][1] & 0xFFFFFFFF)
                elif m == 'mulli' and ops[2][1] == ST:
                    R[ops[0][1]] = ('m', 0)
                elif m == 'mr':
                    v = R.get(ops[1][1])
                    if v: R[ops[0][1]] = v
                    else: R.pop(ops[0][1], None)
                elif m == 'add':
                    va, vb = R.get(ops[1][1]), R.get(ops[2][1])
                    k = [v for v in (va, vb) if v and v[0] == 'k']
                    mm = [v for v in (va, vb) if v and v[0] == 'm']
                    if k and mm:
                        R[ops[0][1]] = ('row', k[0][1] - TB + mm[0][1])
                    else:
                        R.pop(ops[0][1], None)
                elif len(ops) >= 2 and ops[1][0] == 'm':
                    v = R.get(ops[1][2])
                    if v and v[0] == 'row':
                        out.append('%08X %-6s off=+0x%X' % (pc, m, v[1] + ops[1][1]))
                    if ops[0][0] == 'r' and not m.startswith('st'):
                        R.pop(ops[0][1], None)
                elif len(ops) >= 2 and ops[1][0] == 'x':
                    va, vb = R.get(ops[1][1]), R.get(ops[1][2])
                    vs = [v for v in (va, vb) if v]
                    k = [v for v in vs if v[0] == 'k']
                    mm = [v for v in vs if v[0] == 'm']
                    rw = [v for v in vs if v[0] == 'row']
                    if k and mm:
                        out.append('%08X %-6s off=+0x%X' % (pc, m, k[0][1] - TB + mm[0][1]))
                    elif rw and k:
                        out.append('%08X %-6s off=+0x%X' % (pc, m, rw[0][1] + k[0][1]))
                    if ops[0][0] == 'r' and not m.startswith('st'):
                        R.pop(ops[0][1], None)
                elif m in ('bl', 'bctrl'):
                    for r in (0, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12):
                        R.pop(r, None)
                elif ops and ops[0][0] == 'r':
                    R.pop(ops[0][1], None)
            except Exception:
                pass
            pc += 4
        if out:
            print('%s (%08X)' % (S.label(f), f))
            for o in out:
                print('    ' + o)


if __name__ == '__main__' and sys.argv[1] == 'tbl':
    tbl(*sys.argv[2:])

"""Lock-on / targeting investigation helper (agent_weapon_lock_*). Read-only on shared caches.

  python agent_weapon_lock_q.py range START END [OUT]   annotated disasm of every function in [START, END)
  python agent_weapon_lock_q.py words VALUE [VALUE..]   scan all initialised data sections for u32 == VALUE
  python agent_weapon_lock_q.py fl ADDR n               dump n f32 words starting at ADDR
"""
import sys
import io
import contextlib
import bisect
import q
from q import prog, fend
from dol import get_dol


def rng(start, end, out=None):
    a0, a1 = int(start, 16), int(end, 16)
    pr = prog()
    i = bisect.bisect_left(pr._fstarts, a0)
    buf = io.StringIO()
    while i < len(pr._fstarts) and pr._fstarts[i] < a1:
        f = pr._fstarts[i]
        with contextlib.redirect_stdout(buf):
            q.cmd_dis('%08X' % f)
        i += 1
    txt = buf.getvalue()
    if out:
        with open(out, 'w', encoding='utf-8') as fh:
            fh.write(txt)
        print('wrote', out, len(txt.splitlines()), 'lines')
    else:
        sys.stdout.write(txt)


def words(*vals):
    d = get_dol()
    want = {int(v, 16) for v in vals}
    for s in d.sections:
        if s.is_text:
            continue
        for a in range(s.addr, s.end - 3, 4):
            v = d.u32(a)
            if v in want:
                print('%08X %-9s -> %08X' % (a, s.name, v))


def fl(addr, n):
    d = get_dol()
    a = int(addr, 16)
    for i in range(int(n)):
        x = a + 4 * i
        u = d.u32(x)
        print('%08X  %08X  %r' % (x, u if u is not None else 0, d.f32(x)))


if __name__ == '__main__':
    try:
        sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    except Exception:
        pass
    c = sys.argv[1]
    if c == 'range':
        rng(*sys.argv[2:])
    elif c == 'words':
        words(*sys.argv[2:])
    elif c == 'fl':
        fl(*sys.argv[2:])


def callargs(target):
    """every direct call to TARGET with symbolic args r3..r6 / f1 from dataflow"""
    from symbols import get_symbols
    S = get_symbols()
    t = int(target, 16)
    for f, recs in sorted(q.df().items()):
        for pc, tg, args in recs.get('call', []):
            if tg == t:
                a = ['%s' % (q.fmtv(v) if v is not None else '?') for v in args[:4]]
                fa = args[8:9] if len(args) > 8 else []
                try:
                    fs = ('f1=' + q.fmtv(fa[0])) if fa and fa[0] else ''
                except Exception:
                    fs = 'f1=%r' % (fa[0],)
                print('%08X %-50s %s %s' % (pc, S.label(f)[:50], ' | '.join(a), fs))


if __name__ == '__main__' and sys.argv[1] == 'callargs':
    callargs(sys.argv[2])


def storedval(pcs):
    """value stored at given store pcs (dataflow), resolving float consts"""
    import program as P
    d = get_dol()
    for s in pcs:
        pc = int(s, 16)
        f = P.func_of(q.prog(), pc)
        for rpc, base, disp, size, st, fp, val in q.df().get(f, {}).get('acc', []):
            if rpc == pc and st:
                v = val
                if v is not None and v[0] == 'fk':
                    print('%08X stores f32 %r (const %08X)' % (pc, d.f32(v[1]), v[1]))
                else:
                    print('%08X stores %s' % (pc, q.fmtv(v) if v else '?'))


if __name__ == '__main__' and sys.argv[1] == 'storedval':
    storedval(sys.argv[2:])

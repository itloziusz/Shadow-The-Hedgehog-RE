"""Interactive query tool for the gameplay RE database.

  python q.py dis   ADDR|NAME [n]    annotated disassembly of the function
  python q.py xref  ADDR             code/data references to an address
  python q.py callers ADDR|NAME      direct + tail callers
  python q.py callees ADDR|NAME      direct callees
  python q.py class  NAME|regex      RTTI class: bases, vtables, slots
  python q.py fields ADDR|NAME [n]   offsets accessed through arg rN (default r3)
  python q.py str    regex           strings matching regex + code xrefs
  python q.py vcalls ADDR|NAME       virtual calls made by a function
"""
import re
import sys as _sys
try:
    _sys.stdout.reconfigure(encoding="utf-8", errors="replace")
except Exception:
    pass
import sys
import pickle
import bisect
import os
import struct
from collections import defaultdict
from dol import get_dol, DATA_DIR
import program as P
from symbols import get_symbols

_df = None
_prog = None
_idx = None


def prog():
    global _prog
    if _prog is None:
        _prog = P.load()
    return _prog


def df():
    global _df
    if _df is None:
        with open(os.path.join(DATA_DIR, 'dataflow.pkl'), 'rb') as f:
            _df = pickle.load(f)
    return _df


def index():
    """Reverse indices: address -> references."""
    global _idx
    if _idx is not None:
        return _idx
    p = os.path.join(DATA_DIR, 'xref_index.pkl')
    if os.path.exists(p) and os.path.getmtime(p) > os.path.getmtime(os.path.join(DATA_DIR, 'dataflow.pkl')):
        with open(p, 'rb') as f:
            _idx = pickle.load(f)
        return _idx
    refs = defaultdict(list)     # addr -> [(func, pc, kind)]
    callers = defaultdict(list)  # target -> [(func, pc)]
    for f, recs in df().items():
        for pc, a in recs.get('cptr', []):
            refs[a].append((f, pc, 'addr'))
        for pc, base, disp, size, st, fp, val in recs.get('acc', []):
            if base is not None and base[0] == 'k':
                refs[(base[1] + disp) & 0xFFFFFFFF].append((f, pc, ('W' if st else 'R') + str(size) + ('f' if fp else '')))
            if st and val is not None and val[0] == 'k':
                refs[val[1]].append((f, pc, 'stored'))
        for pc, t, args in recs.get('call', []):
            callers[t].append((f, pc))
            for v in args:
                if v is not None and v[0] == 'k' and v[1] >= 0x80000000:
                    refs[v[1]].append((f, pc, 'arg'))
    d = get_dol()
    for a, v in prog().dataptrs.items():
        refs[v].append((None, a, 'dataptr'))
    _idx = {'refs': dict(refs), 'callers': dict(callers)}
    with open(p, 'wb') as fh:
        pickle.dump(_idx, fh, protocol=pickle.HIGHEST_PROTOCOL)
    return _idx


def fend(f):
    pr = prog()
    i = bisect.bisect_left(pr._fstarts, f)
    return pr.funcs[i][1]


def resolve(s):
    S = get_symbols()
    a = S.lookup(s)
    if a is None:
        raise SystemExit('unknown symbol %s' % s)
    f = P.func_of(prog(), a)
    return f if f is not None else a


def describe_addr(a):
    """Short annotation for an absolute address."""
    d = get_dol()
    S = get_symbols()
    n = S.name(a)
    if n:
        return n
    if d.is_code(a):
        f = P.func_of(prog(), a)
        if f == a:
            return 'fn_%08X' % a
        if f is not None:
            return '%s+%X' % (S.label(f), a - f)
    reg = d.region(a)
    if reg is None:
        return None
    s = d.printable_str(a, 2) if reg in ('.rodata', '.data', '.sdata', '.sdata2') else None
    if reg in ('.sdata2', '.rodata') and (a & 7) == 0:
        raw8 = d.read(a, 8)
        if raw8 in (bytes.fromhex('4330000080000000'), bytes.fromhex('4330000000000000')):
            return '%s %08X (int->float magic %s)' % (reg, a, 'signed' if raw8[4] == 0x80 else 'unsigned')
    if s and len(s) < 4 and reg in ('.sdata2', '.rodata'):
        fv = d.f32(a)
        if fv is not None and fv == fv and (fv == 0 or 1e-6 < abs(fv) < 1e9):
            dv = d.f64(a) if (a & 7) == 0 else None
            if dv is not None and dv == dv and 1e-6 < abs(dv) < 1e18 and abs(fv) > 1e6:
                return '%s %08X (%.6g double)' % (reg, a, dv)
            return '%s %08X (%gf)' % (reg, a, fv)
    if s:
        return '"%s"' % s[:60]
    if a in S.vt_class:
        return 'vtbl %s' % S.vt_class[a]
    ti = S.classes.get('%08X' % a)
    if ti:
        return 'typeinfo %s' % ti['name']
    if reg in ('.sdata2', '.rodata', '.sdata', '.data'):
        fv = d.f32(a)
        if fv is not None and fv == fv and 1e-6 < abs(fv) < 1e7:
            return '%s %08X (%gf)' % (reg, a, fv)
    return '%s %08X' % (reg, a)


def cmd_dis(name, n=None):
    n = int(n) if n else None
    f = resolve(name)
    pr = prog()
    S = get_symbols()
    e = fend(f)
    recs = df().get(f, {})
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
    print('%s  %08X-%08X  starts:%s' % (S.label(f), f, e, ','.join(sorted(pr.starts.get(f, [])))))
    a = f
    cnt = 0
    while a < e:
        x = pr.ins(a)
        txt = x.text()
        if x.target is not None:
            t = x.target
            if not (f <= t < e):
                txt = '%s %s' % (x.m, S.label(t))
        extra = ('   ; ' + ' | '.join(ann[a])) if a in ann else ''
        print('  %08X  %-40s%s' % (a, txt, extra))
        a += 4
        cnt += 1
        if n and cnt >= n:
            break


def fmtv(v):
    if v is None:
        return '?'
    t = v[0]
    if t == 'k':
        return '0x%X' % v[1]
    if t == 'a':
        return 'r%d' % v[1]
    if t == 'fa':
        return 'f%d' % v[1]
    if t == 'l':
        return '[%s+%X]' % (fmtv(v[1]), v[2])
    if t == '+':
        return '(%s+%X)' % (fmtv(v[1]), v[2])
    if t == 'ret':
        return 'ret(%s)' % get_symbols().label(v[1])
    if t == 'sp':
        return 'sp%+d' % v[1]
    if t == 'fk':
        d = get_dol()
        return '%gf' % d.f32(v[1])
    if t == 'fl':
        return 'f[%s+%X]' % (fmtv(v[1]), v[2])
    return repr(v)


def cmd_xref(s):
    a = int(s, 16)
    idx = index()
    S = get_symbols()
    for f, pc, kind in sorted(idx['refs'].get(a, []), key=lambda r: (r[1])):
        print('  %-8s %08X  in %s' % (kind, pc, S.label(f) if f else '(data)'))
    for f, pc in idx['callers'].get(a, []):
        print('  call     %08X  in %s' % (pc, S.label(f)))


def cmd_callers(name):
    f = resolve(name)
    S = get_symbols()
    idx = index()
    seen = set()
    for c, pc in sorted(idx['callers'].get(f, [])):
        print('  %08X  %s' % (pc, S.label(c)))
    for c, pc, kind in idx['refs'].get(f, []):
        print('  %-8s %08X  %s' % (kind, pc, S.label(c) if c else '(data)'))


def cmd_callees(name):
    f = resolve(name)
    S = get_symbols()
    for pc, t, args in df().get(f, {}).get('call', []):
        print('  %08X  %-50s (%s)' % (pc, S.label(t), ', '.join(fmtv(v) for v in args[:4])))
    for pc, ctr, args in df().get(f, {}).get('icall', []):
        print('  %08X  icall %-44s (%s)' % (pc, fmtv(ctr), ', '.join(fmtv(v) for v in args[:4])))


def cmd_class(pat):
    S = get_symbols()
    rx = re.compile(pat)
    groups = defaultdict(list)
    for ti, c in S.classes.items():
        groups[c['name']].append((ti, c))
    exact = pat in groups
    for name in sorted(groups):
        if exact and name != pat:
            continue
        if not exact and not rx.search(name):
            continue
        tis = groups[name]
        bases = None
        for ti, c in tis:
            if c['bases']:
                bases = c['bases']
        print('%s  typeinfos %s' % (name, ','.join(t for t, c in tis)))
        for b in bases or []:
            print('   base %-40s off %X' % (b['name'], b['offset']))
        for ti, c in tis:
            for vt in c['vtables']:
                print('   vtable %s adjust %d  (%d slots)' % (vt['addr'], vt['adjust'], len(vt['slots'])))
                for w in vt['vptr_writers']:
                    print('      vptr write in %s  off %X  base %s' % (S.label(int(w['func'], 16)), w['off'], w['base']))
                for i, s in enumerate(vt['slots']):
                    print('      [%02X] %s  %s' % (i, s, S.label(int(s, 16))))


def cmd_fields(name, reg='3'):
    f = resolve(name)
    reg = int(reg)
    root = ('a', reg)
    rows = defaultdict(set)
    for pc, base, disp, size, st, fp, val in df().get(f, {}).get('acc', []):
        if base is None:
            continue
        path = chain(base, root)
        if path is None:
            continue
        rows[(path, disp)].add(('W' if st else 'R') + str(size) + ('f' if fp else ''))
    for (path, disp), kinds in sorted(rows.items()):
        print('  %-20s +%-5X %s' % (path, disp, ','.join(sorted(kinds))))


def chain(v, root):
    """Render a value as access path relative to root, or None."""
    if v == root:
        return 'this'
    if v is None:
        return None
    if v[0] == 'l':
        c = chain(v[1], root)
        return None if c is None else '%s->%X' % (c, v[2])
    if v[0] == '+':
        c = chain(v[1], root)
        return None if c is None else '%s.%X' % (c, v[2])
    return None


def cmd_str(pat):
    d = get_dol()
    rx = re.compile(pat)
    idx = index()
    S = get_symbols()
    with open(os.path.join(DATA_DIR, 'strings_raw.txt')) as f:
        for line in f:
            a, sec, s = line.rstrip('\n').split(' ', 2)
            if rx.search(s):
                ai = int(a, 16)
                users = sorted(set(S.label(fn) for fn, pc, k in idx['refs'].get(ai, []) if fn))
                print('%s %-40s %s' % (a, s[:40], ', '.join(users[:6])))


def cmd_vcalls(name):
    f = resolve(name)
    for pc, ctr, args in df().get(f, {}).get('icall', []):
        print('  %08X  %s  this=%s' % (pc, fmtv(ctr), fmtv(args[0])))


def cmd_off(off, kind=''):
    """All functions accessing displacement `off` (hex) on non-constant bases."""
    o = int(off, 16)
    S = get_symbols()
    hits = defaultdict(set)
    for f, recs in df().items():
        for pc, base, disp, size, st, fp, val in recs.get('acc', []):
            if disp == o and base is not None and base[0] not in ('k', 'sp', 'lsp'):
                k = ('W' if st else 'R') + str(size) + ('f' if fp else '')
                if kind and kind not in k:
                    continue
                hits[f].add(k + ':' + fmtv(base))
    for f in sorted(hits):
        print('  %-50s %s' % (S.label(f), ' '.join(sorted(hits[f]))[:120]))


if __name__ == '__main__':
    cmd = sys.argv[1]
    args = sys.argv[2:]
    {'dis': cmd_dis, 'xref': cmd_xref, 'callers': cmd_callers,
     'callees': cmd_callees, 'class': cmd_class, 'fields': cmd_fields,
     'str': cmd_str, 'vcalls': cmd_vcalls, 'off': cmd_off}[cmd](*args)

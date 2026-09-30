"""Whole-program structure recovery for main.dol.

Builds (and caches in data/program.pkl):
  * function boundaries
  * direct call graph (bl + tail-call b)
  * switch jump tables
  * pointers from data sections into code

Function start evidence (each start remembers *why* it is a start):
  'entry'   DOL entry point
  'bl'      target of a bl instruction
  'extab'   CodeWarrior extabindex entry
  'dataptr' word in a data section pointing at code that follows a
            terminator/padding (vtable slots, callback tables)
  'codeptr' lis/addi|ori pair in code materialising a code address that
            follows a terminator/padding (callbacks registered at runtime)
  'gap'     code following the reachable end of the previous function
The filters fail closed: ambiguous pointers are not promoted to starts.
"""
import os
import pickle
import struct
import bisect
from dol import get_dol, DATA_DIR
from ppc import decode

R2 = 0x805FA780
R13 = 0x805EC500
CACHE = os.path.join(DATA_DIR, 'program.pkl')

BLR = 0x4E800020
BCTR = 0x4E800420
RFI = 0x4C000064


class Program:
    def __init__(self):
        self.dol = get_dol()
        self.text_secs = [s for s in self.dol.sections if s.is_text]
        self.words = {}
        for s in self.text_secs:
            raw = self.dol.read(s.addr, s.size)
            self.words[s.key] = (s.addr, struct.unpack('>%dI' % (s.size // 4), raw))
        self.starts = {}          # addr -> set(reasons)
        self.jumptables = {}      # bctr addr -> (table addr, [targets])
        self.jt_ranges = []       # (table_start, table_end)
        self.funcs = []           # sorted list of (start, end)
        self.calls = []           # (caller_start, site, target, kind)
        self.dataptrs = {}        # data addr -> code addr

    # ------------------------------------------------------------------
    def word(self, a):
        base, ws = self.words['T1']
        i = (a - base) >> 2
        if 0 <= i < len(ws) and a >= base:
            return ws[i]
        for base, ws in self.words.values():
            i = (a - base) >> 2
            if 0 <= i < len(ws):
                return ws[i]
        return None

    def ins(self, a):
        w = self.word(a)
        return None if w is None else decode(w, a)

    def is_text(self, a):
        return (a & 3) == 0 and self.word(a) is not None

    def _iter_text(self):
        for base, ws in self.words.values():
            for i, w in enumerate(ws):
                yield base + 4 * i, w

    def _is_terminator(self, w):
        if w in (BLR, BCTR, RFI, 0):
            return True
        op = w >> 26
        if op == 18 and not (w & 1):          # b / ba
            return True
        return False

    def _follows_terminator(self, a):
        p = self.word(a - 4)
        if p is None:
            return True
        return self._is_terminator(p)

    def add_start(self, a, why):
        if not self.is_text(a):
            return
        self.starts.setdefault(a, set()).add(why)

    # ------------------------------------------------------------------
    def find_jumptables(self):
        """MWCC switch: lis rT,hi; addi rT,rT,lo; slwi; lwzx rX,rT,rI; mtctr rX; bctr."""
        for a, w in self._iter_text():
            if w != BCTR:
                continue
            window = [self.ins(a - 4 * k) for k in range(1, 16)]
            window = [x for x in window if x is not None]
            # mtctr rX immediately (or nearly) before
            mt = next((x for x in window[:3] if x.m == 'mtctr'), None)
            if mt is None:
                continue
            lx = next((x for x in window if x.m == 'lwzx' and x.rd == mt.rd), None)
            if lx is None:
                continue
            # resolve table base: look for addi rT,rT,lo and lis rT,hi above lwzx
            tbl = None
            for base_reg in (lx.ra, lx.rb):
                lo = hi = None
                for x in window:
                    if x.addr >= lx.addr:
                        continue
                    if lo is None and x.m in ('addi',) and x.rd == base_reg and x.ra == base_reg:
                        lo = x.imm
                    elif lo is None and x.m == 'ori' and x.ra == base_reg and x.rd == base_reg:
                        lo = x.imm
                    elif x.m == 'lis' and x.rd == base_reg:
                        hi = x.imm
                        break
                if hi is not None and lo is not None:
                    tbl = ((hi << 16) + lo) & 0xFFFFFFFF
                    break
            if tbl is None or self.dol.find(tbl) is None:
                continue
            # bound from cmplwi rI,K ; bgt default
            bound = None
            for x in window:
                if x.m == 'cmplwi':
                    bound = x.imm + 1
                    break
            targets = []
            k = 0
            while True:
                if bound is not None and k >= bound:
                    break
                t = self.dol.u32(tbl + 4 * k)
                if t is None or not self.is_text(t) or abs(t - a) > 0x10000:
                    break
                targets.append(t)
                k += 1
                if k > 4096:
                    break
            if targets:
                self.jumptables[a] = (tbl, targets)
                self.jt_ranges.append((tbl, tbl + 4 * len(targets)))

    def _in_jt(self, a):
        for s, e in self.jt_ranges:
            if s <= a < e:
                return True
        return False

    # ------------------------------------------------------------------
    def collect_starts(self):
        d = self.dol
        self.add_start(d.entry, 'entry')
        for s in self.text_secs:
            self.add_start(s.addr, 'section')
        # bl targets
        for a, w in self._iter_text():
            if (w >> 26) == 18 and (w & 1):
                li = w & 0x03FFFFFC
                if li & 0x02000000:
                    li -= 0x04000000
                t = (li if (w & 2) else a + li) & 0xFFFFFFFF
                self.add_start(t, 'bl')
        # extabindex
        ei = d.section('extabindex')
        for k in range(ei.size // 12):
            f = d.u32(ei.addr + 12 * k)
            if f:
                self.add_start(f, 'extab')
        # data pointers into code
        self.find_jumptables()
        for s in d.sections:
            if s.is_text:
                continue
            for off in range(0, s.size - 3, 4):
                a = s.addr + off
                v = d.u32(a)
                if v is None or not self.is_text(v):
                    continue
                if self._in_jt(a):
                    continue
                self.dataptrs[a] = v
                if self._follows_terminator(v) or v in self.starts:
                    self.add_start(v, 'dataptr')
        # code pointers materialised with lis/addi
        for a, w in self._iter_text():
            if (w >> 26) != 15 or ((w >> 16) & 31) != 0:
                continue
            rd = (w >> 21) & 31
            hi = w & 0xFFFF
            for k in range(1, 12):
                x = self.ins(a + 4 * k)
                if x is None:
                    break
                if x.m in ('addi', 'ori') and (x.ra if x.m == 'addi' else x.rd) == rd:
                    lo = x.imm
                    t = (((hi << 16) + lo) & 0xFFFFFFFF) if x.m == 'addi' else ((hi << 16) | lo)
                    if self.is_text(t) and not self._in_jt_target(t) and self._follows_terminator(t):
                        self.add_start(t, 'codeptr')
                    break
                if x.kind in ('branch', 'ret'):
                    break
                # rd overwritten?
                if x.ops and x.ops[0] == ('r', rd) and x.kind not in ('store', 'cmp'):
                    break

    def _in_jt_target(self, t):
        if not hasattr(self, '_jt_targets'):
            self._jt_targets = set()
            for tbl, ts in self.jumptables.values():
                self._jt_targets.update(ts)
        return t in self._jt_targets

    # ------------------------------------------------------------------
    def _reach(self, start, limit):
        """Recursive descent within [start, limit). Returns (maxaddr_end, calls)."""
        seen = set()
        work = [start]
        hi = start
        calls = []
        while work:
            a = work.pop()
            while start <= a < limit and a not in seen:
                seen.add(a)
                x = self.ins(a)
                if x is None or x.m == '.word':
                    break
                hi = max(hi, a + 4)
                if x.kind == 'call':
                    if x.target is not None:
                        calls.append((a, x.target, 'call'))
                    if x.m.startswith('b') and x.target is not None and x.m not in ('bl', 'bla'):
                        # conditional bl -> treat as call + fallthrough
                        pass
                    a += 4
                    continue
                if x.kind == 'branch':
                    if x.m in ('bctr',):
                        jt = self.jumptables.get(a)
                        if jt:
                            for t in jt[1]:
                                if start <= t < limit:
                                    work.append(t)
                        break
                    if x.target is None:
                        # conditional bctr (rare)
                        a += 4
                        continue
                    t = x.target
                    uncond = x.m in ('b', 'ba')
                    if start <= t < limit:
                        work.append(t)
                    else:
                        calls.append((a, t, 'tail'))
                    if uncond:
                        break
                    a += 4
                    continue
                if x.kind == 'ret':
                    if x.m in ('blr', 'rfi'):
                        break
                    a += 4      # conditional return
                    continue
                a += 4
        return hi, calls

    def starts_set_excl(self, cur):
        return self._startset_minus.get(cur, self._startset)

    def build_functions(self, max_iter=8):
        for it in range(max_iter):
            starts = sorted(self.starts)
            self._startset = set(starts)
            self._startset_minus = {}
            new = 0
            funcs = []
            calls = []
            sec_end = {s.addr: s.end for s in self.text_secs}
            for i, st in enumerate(starts):
                sec = self.dol.find(st)
                limit = starts[i + 1] if i + 1 < len(starts) else sec.end
                limit = min(limit, sec.end)
                end, cl = self._reach(st, limit)
                funcs.append((st, end))
                for site, t, kind in cl:
                    calls.append((st, site, t, kind))
                # gap after reachable end?
                g = end
                while g < limit and self.word(g) == 0:
                    g += 4
                if g < limit:
                    if g not in self.starts:
                        self.add_start(g, 'gap')
                        new += 1
            self.funcs = funcs
            self.calls = calls
            if new == 0:
                break
        # tail-call targets that are not starts get recorded as calls only
        return self.funcs

    def run(self):
        self.collect_starts()
        self.build_functions()
        return self

    def save(self):
        with open(CACHE, 'wb') as f:
            pickle.dump({'starts': self.starts, 'funcs': self.funcs,
                         'calls': self.calls, 'jumptables': self.jumptables,
                         'dataptrs': self.dataptrs}, f)


def load():
    p = Program()
    with open(CACHE, 'rb') as f:
        dct = pickle.load(f)
    p.starts = dct['starts']
    p.funcs = dct['funcs']
    p.calls = dct['calls']
    p.jumptables = dct['jumptables']
    p.dataptrs = dct['dataptrs']
    p.jt_ranges = [(t, t + 4 * len(ts)) for t, ts in p.jumptables.values()]
    p._fstarts = [f[0] for f in p.funcs]
    return p


def func_of(p, a):
    i = bisect.bisect_right(p._fstarts, a) - 1
    if i < 0:
        return None
    s, e = p.funcs[i]
    return s if a < max(e, s + 4) else None


if __name__ == '__main__':
    import time
    t = time.time()
    p = Program().run()
    p.save()
    from collections import Counter
    c = Counter()
    for a, why in p.starts.items():
        for w in why:
            c[w] += 1
    print('functions', len(p.funcs), 'calls', len(p.calls),
          'jumptables', len(p.jumptables), 'time %.1f' % (time.time() - t))
    print(c)

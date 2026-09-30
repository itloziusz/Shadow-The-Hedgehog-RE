"""Per-function forward value propagation over the CFG.

For every function in program.pkl this computes, at each instruction, the
symbolic value of the GPRs/FPRs and emits evidence records:

  acc    memory accesses      (pc, base, disp, size, store?, fp?, value)
  call   direct calls         (pc, target, args{r3..r10,f1..f4})
  icall  indirect calls       (pc, ctr_value, args)
  cptr   code/data addresses materialised in registers (pc, addr)

Symbolic values (hashable tuples):
  ('k', c)              constant / absolute address
  ('a', n)              value of GPR n at function entry  (r3 = this / arg0)
  ('fa', n)             value of FPR n at entry
  ('l', base, d, size)  value loaded from [base + d]
  ('+', base, off)      base + constant
  ('sp', off)           r1-relative stack address
  ('ret', target)       return value of a direct call
  ('fk', addr)          float loaded from constant address addr
  ('x', base)           value loaded through an indexed access on base
  None                  unknown

Join = keep a register only if all predecessors agree (fails closed).
"""
import os
import pickle
import time
from collections import defaultdict
from ppc import decode
import program as P
from dol import DATA_DIR

OUT = os.path.join(DATA_DIR, 'dataflow.pkl')

MAXDEPTH = 4


def depth(v):
    dd = 0
    while v is not None and v[0] in ('l', '+', 'x'):
        v = v[1]
        dd += 1
    return dd


def mk_load(base, d, size):
    if base is None:
        return None
    if depth(base) >= MAXDEPTH:
        return None
    if base[0] == 'sp':
        return ('lsp', base[1] + d, size)
    return ('l', base, d, size)


def add_off(base, off):
    if base is None:
        return None
    t = base[0]
    if t == 'k':
        return ('k', (base[1] + off) & 0xFFFFFFFF)
    if t == 'sp':
        return ('sp', base[1] + off)
    if t == '+':
        o = base[2] + off
        return base[1] if o == 0 else ('+', base[1], o)
    if off == 0:
        return base
    if depth(base) >= MAXDEPTH:
        return None
    return ('+', base, off)


def effective(base, d):
    """Split base+d into (root_base, total_disp) folding '+' nodes."""
    if base is None:
        return None, d
    if base[0] == '+':
        return base[1], base[2] + d
    if base[0] == 'k':
        return ('k', (base[1] + d) & 0xFFFFFFFF), 0
    if base[0] == 'sp':
        return ('sp', base[1] + d), 0
    return base, d


VOLATILE_G = [0] + list(range(3, 13))
VOLATILE_F = list(range(0, 14))


class FuncAnalysis:
    def __init__(self, prog, start, end):
        self.p = prog
        self.start = start
        self.end = max(end, start + 4)
        self.ins = {}
        a = start
        while a < self.end:
            x = prog.ins(a)
            self.ins[a] = x
            a += 4

    def blocks(self):
        leaders = {self.start}
        succ = {}
        for a, x in self.ins.items():
            if x.kind == 'branch':
                if x.target is not None and self.start <= x.target < self.end:
                    leaders.add(x.target)
                if x.m == 'bctr':
                    jt = self.p.jumptables.get(a)
                    if jt:
                        for t in jt[1]:
                            if self.start <= t < self.end:
                                leaders.add(t)
                leaders.add(a + 4)
            elif x.kind == 'ret':
                leaders.add(a + 4)
        leaders = sorted(l for l in leaders if self.start <= l < self.end)
        blocks = {}
        for i, l in enumerate(leaders):
            e = leaders[i + 1] if i + 1 < len(leaders) else self.end
            blocks[l] = e
            last = self.ins[e - 4]
            s = []
            if last.kind == 'branch':
                if last.m == 'bctr':
                    jt = self.p.jumptables.get(last.addr)
                    if jt:
                        s = [t for t in jt[1] if self.start <= t < self.end]
                elif last.target is not None:
                    if self.start <= last.target < self.end:
                        s.append(last.target)
                    if last.m not in ('b', 'ba'):
                        s.append(e)
                else:
                    s.append(e)
            elif last.kind == 'ret':
                if last.m not in ('blr', 'rfi'):
                    s.append(e)
            else:
                s.append(e)
            succ[l] = [x for x in s if x in blocks or (self.start <= x < self.end)]
        self.blk = blocks
        self.succ = succ
        return blocks

    def entry_state(self):
        g = {n: ('a', n) for n in range(3, 11)}
        g[1] = ('sp', 0)
        g[2] = ('k', P.R2)
        g[13] = ('k', P.R13)
        for n in range(14, 32):
            g[n] = ('a', n)        # callee-saved: caller's value
        f = {n: ('fa', n) for n in range(1, 9)}
        return (g, f, None)        # ctr

    def run(self, emit):
        self.blocks()
        ins_state = {self.start: self.entry_state()}
        work = [self.start]
        visits = defaultdict(int)
        final_in = {}
        while work:
            b = work.pop()
            visits[b] += 1
            if visits[b] > 12:
                continue
            st = ins_state[b]
            final_in[b] = st
            out = self.transfer(b, st, None)
            for s in self.succ.get(b, []):
                if s not in self.blk:
                    continue
                if s not in ins_state:
                    ins_state[s] = out
                    work.append(s)
                else:
                    m = self.join(ins_state[s], out)
                    if m != ins_state[s]:
                        ins_state[s] = m
                        work.append(s)
        # final emission pass using converged states
        for b in sorted(self.blk):
            if b in ins_state:
                self.transfer(b, ins_state[b], emit)

    @staticmethod
    def join(s1, s2):
        g1, f1, c1 = s1
        g2, f2, c2 = s2
        g = {k: v for k, v in g1.items() if g2.get(k) == v}
        f = {k: v for k, v in f1.items() if f2.get(k) == v}
        return (g, f, c1 if c1 == c2 else None)

    def transfer(self, b, st, emit):
        g = dict(st[0])
        f = dict(st[1])
        ctr = st[2]
        a = b
        end = self.blk[b]
        while a < end:
            x = self.ins[a]
            ctr = self.step(x, g, f, ctr, emit)
            a += 4
        return (g, f, ctr)

    def step(self, x, g, f, ctr, emit):
        m = x.m
        k = x.kind
        if k in ('load', 'store'):
            op1 = x.ops[1] if len(x.ops) > 1 else None
            if op1 and op1[0] == 'm':
                disp, ra = op1[1], op1[2]
                base = ('k', 0) if (ra == 0 and not x.upd) else g.get(ra)
                rb_, d = effective(base, disp)
                if m in ('lmw', 'stmw'):
                    if emit and m == 'lmw':
                        pass
                    if m == 'lmw':
                        for r in range(x.rd, 32):
                            g[r] = None
                    return ctr
                if m.startswith('psq'):
                    if emit:
                        emit('acc', (x.addr, rb_, d, x.size, k == 'store', True,
                                     f.get(x.rd) if k == 'store' else None))
                    if k == 'load':
                        f[x.rd] = None
                    if x.upd:
                        g[ra] = add_off(base, disp)
                    return ctr
                if k == 'load':
                    if emit:
                        emit('acc', (x.addr, rb_, d, x.size, False, x.fp, None))
                    if x.fp:
                        if rb_ is not None and rb_[0] == 'k':
                            f[x.rd] = ('fk', rb_[1])
                        elif rb_ is not None and depth(rb_) < MAXDEPTH and rb_[0] != 'sp':
                            f[x.rd] = ('fl', rb_, d, x.size)
                        else:
                            f[x.rd] = None
                    else:
                        if m in ('lwz', 'lwzu'):
                            g[x.rd] = mk_load(rb_, d, 4)
                        elif m in ('lhz', 'lhzu', 'lha', 'lhau', 'lbz', 'lbzu'):
                            g[x.rd] = mk_load(rb_, d, -2 if m.startswith('lha') else x.size)
                        else:
                            g[x.rd] = None
                else:
                    val = f.get(x.rd) if x.fp else g.get(x.rd)
                    if emit:
                        emit('acc', (x.addr, rb_, d, x.size, True, x.fp, val))
                if x.upd:
                    g[ra] = add_off(base, disp)
                    if ra == 1:
                        g[1] = add_off(base, disp)
                return ctr
            if op1 and op1[0] == 'x':
                ra, rb = op1[1], op1[2]
                va = ('k', 0) if ra == 0 else g.get(ra)
                vb = g.get(rb)
                base = None
                if va is not None and va[0] == 'k' and (vb is None or vb[0] != 'k'):
                    base = va
                elif vb is not None and vb[0] == 'k' and (va is None or va[0] != 'k'):
                    base = vb
                elif va is not None and vb is not None and va[0] == 'k' and vb[0] == 'k':
                    base = ('k', (va[1] + vb[1]) & 0xFFFFFFFF)
                elif va is not None:
                    base = va
                if emit:
                    emit('accx', (x.addr, base, x.size, k == 'store', x.fp,
                                  (f.get(x.rd) if x.fp else g.get(x.rd)) if k == 'store' else None))
                if k == 'load':
                    if x.fp:
                        f[x.rd] = None
                    else:
                        g[x.rd] = ('x', base) if (base is not None and depth(base) < MAXDEPTH) else None
                if x.upd:
                    g[ra] = None
                return ctr
            # lswi/stswi etc
            if k == 'load' and x.ops and x.ops[0][0] == 'r':
                g[x.ops[0][1]] = None
            return ctr

        if m == 'li':
            g[x.rd] = ('k', x.imm & 0xFFFFFFFF)
            return ctr
        if m == 'lis':
            g[x.rd] = ('k', (x.imm << 16) & 0xFFFFFFFF)
            return ctr
        if m == 'addi':
            v = add_off(g.get(x.ra), x.imm)
            g[x.rd] = v
            if emit and v is not None and v[0] == 'k':
                emit('cptr', (x.addr, v[1]))
            return ctr
        if m == 'addis':
            base = g.get(x.ra)
            if base is not None and base[0] == 'k':
                g[x.rd] = ('k', (base[1] + (x.imm << 16)) & 0xFFFFFFFF)
            else:
                g[x.rd] = None
            return ctr
        if m in ('ori', 'oris', 'xori', 'xoris', 'andi.', 'andis.'):
            src = g.get(x.rd)
            dst = x.ra
            if src is not None and src[0] == 'k':
                c = src[1]
                imm = x.imm
                if m == 'ori':
                    r = c | imm
                elif m == 'oris':
                    r = c | (imm << 16)
                elif m == 'xori':
                    r = c ^ imm
                elif m == 'xoris':
                    r = c ^ (imm << 16)
                elif m == 'andi.':
                    r = c & imm
                else:
                    r = c & (imm << 16)
                g[dst] = ('k', r & 0xFFFFFFFF)
                if emit and m == 'ori':
                    emit('cptr', (x.addr, r & 0xFFFFFFFF))
            elif m in ('ori', 'xori') and x.imm == 0:
                g[dst] = src
            else:
                g[dst] = None
            return ctr
        if m in ('mr', 'mr.'):
            g[x.ra] = g.get(x.rd)
            return ctr
        if m == 'nop':
            return ctr
        if m == 'mtctr':
            return g.get(x.rd)
        if m == 'mflr' or m == 'mfctr':
            g[x.rd] = None
            return ctr
        if m == 'mtlr':
            return ctr
        if m == 'fmr':
            f[x.rd] = f.get(x.rb)
            return ctr
        if k == 'call':
            args = tuple(g.get(r) for r in range(3, 11)) + tuple(f.get(r) for r in range(1, 5))
            if emit:
                if x.target is not None:
                    emit('call', (x.addr, x.target, args))
                else:
                    emit('icall', (x.addr, ctr if x.m.startswith('bctr') or 'ctr' in x.m else ('lr',), args))
            for r in VOLATILE_G:
                g[r] = None
            for r in VOLATILE_F:
                f[r] = None
            if x.target is not None:
                g[3] = ('ret', x.target)
            else:
                g[3] = ('iret', x.addr)
            f[1] = ('fret', x.addr)
            return None
        if k == 'branch':
            if x.m == 'bctr' and emit and x.addr not in self.p.jumptables:
                args = tuple(g.get(r) for r in range(3, 11)) + tuple(f.get(r) for r in range(1, 5))
                emit('icall', (x.addr, ctr, args))
            elif x.m in ('b', 'ba') and emit and x.target is not None and not (self.start <= x.target < self.end):
                args = tuple(g.get(r) for r in range(3, 11)) + tuple(f.get(r) for r in range(1, 5))
                emit('call', (x.addr, x.target, args))
            return ctr
        if k in ('ret', 'cmp', 'sys'):
            if m in ('mfspr', 'mfmsr', 'mftb', 'mftbu', 'mfsr', 'mfsrin'):
                g[x.rd] = None
            return ctr
        # generic: first operand is destination
        if x.ops:
            o = x.ops[0]
            if o[0] == 'r':
                g[o[1]] = None
            elif o[0] == 'f':
                f[o[1]] = None
        if m in ('mfcr',):
            g[x.rd] = None
        return ctr


def run_all(prog=None, progress=True):
    prog = prog or P.load()
    res = {}
    t = time.time()
    for i, (s, e) in enumerate(prog.funcs):
        recs = defaultdict(list)

        def emit(kind, r, recs=recs):
            recs[kind].append(r)
        try:
            FuncAnalysis(prog, s, e).run(emit)
        except Exception as ex:  # fail closed: record the failure
            recs['error'].append(repr(ex))
        res[s] = dict(recs)
        if progress and i % 5000 == 0:
            print(i, '%.1fs' % (time.time() - t), flush=True)
    return res


if __name__ == '__main__':
    import sys
    prog = P.load()
    if len(sys.argv) > 1:
        a = int(sys.argv[1], 16)
        f = P.func_of(prog, a)
        e = dict(prog.funcs)[f]
        recs = defaultdict(list)
        FuncAnalysis(prog, f, e).run(lambda k, r: recs[k].append(r))
        for k, v in recs.items():
            print(k)
            for r in v:
                print('   ', r)
        sys.exit()
    res = run_all(prog)
    with open(OUT, 'wb') as fh:
        pickle.dump(res, fh, protocol=pickle.HIGHEST_PROTOCOL)
    errs = sum(1 for v in res.values() if 'error' in v)
    print('functions', len(res), 'errors', errs)

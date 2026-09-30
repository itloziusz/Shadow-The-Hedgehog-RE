"""AI state table for an enemy family: vtable slots (Enter/Update/Exit), ctor, getter(s) + singleton object,
and every requester of the getter.
usage: python agent_enemy2_states.py REGEX          e.g. 'BkSoldierAI|SoldierCommonAI'
TEnemyAIState vtable: [0] dtor [1] BindOwner [2] Enter [3] Update [4] Exit [5] Name
"""
import json, re, sys, os
import program as P
from symbols import get_symbols
from dol import get_dol, DATA_DIR
from ppc import decode


def singleton_of(p, getter, ctor):
    """find the r3 value (addi/lis) right before 'bl ctor' inside getter."""
    a, e = None, None
    for (s, en) in p.funcs:
        if s == getter:
            a, e = s, en
            break
    if a is None:
        return None
    hi = {}
    val = {}
    x = a
    while x < e:
        ins = decode(p.word(x), x)
        m = ins.m
        if m == 'lis':
            hi[ins.rd] = (ins.imm & 0xFFFF) << 16
        elif m == 'addi' and ins.ra in hi:
            val[ins.rd] = (hi[ins.ra] + ins.imm) & 0xFFFFFFFF
        elif m == 'addi' and ins.ra == 13:
            val[ins.rd] = (0x805EC500 + ins.imm) & 0xFFFFFFFF
        elif m == 'mr':
            if ins.rd in val:
                val[ins.ra] = val[ins.rd]
        elif m == 'bl' and ins.target == ctor:
            return val.get(3)
        x += 4
    return None


if __name__ == '__main__':
    pat = re.compile(sys.argv[1])
    c = json.load(open(os.path.join(DATA_DIR, 'classes.json')))
    p = P.load(); S = get_symbols()
    callers = {}
    for (caller, site, target, kind) in p.calls:
        callers.setdefault(target, []).append((caller, site))
    for k, v in sorted(c.items(), key=lambda kv: kv[1]['name']):
        n = v['name']
        if not n.startswith('EnemyAIState_') or not pat.search(n):
            continue
        for vt in v['vtables']:
            sl = [int(x, 16) for x in vt['slots']]
            dt = int(vt['vptr_writers'][0]['func'], 16) if vt['vptr_writers'] else 0
            ctors = [int(w['func'], 16) for w in vt['vptr_writers'] if int(w['func'], 16) != sl[0]]
            print('%s  vt %s' % (n, vt['addr']))
            print('   Bind %08X  Enter %08X  Update %08X  Exit %08X  Name %08X' % tuple(sl[1:6]))
            for ct in ctors:
                for (g, site) in callers.get(ct, []):
                    obj = singleton_of(p, g, ct)
                    print('   ctor %08X  getter %08X %-40s obj %s' % (ct, g, S.label(g)[:40], ('%08X' % obj) if obj else '?'))
                    for (r, rs) in sorted(callers.get(g, [])):
                        if r == g:
                            continue
                        print('        req %08X in %s' % (rs, S.label(r)))

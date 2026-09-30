"""Map AI-state singleton getters -> state class, and list who requests each state.
usage: python agent_enemy_states.py [classname-regex]
"""
import re, sys
import program as P
from symbols import get_symbols
from ppc import decode

pat = re.compile(sys.argv[1] if len(sys.argv) > 1 else r'GunSoldierAI|EnemyBaseAI|SoldierCommonAI')
p = P.load(); S = get_symbols()
ctor_of = {}
for (caller, site, target, kind) in p.calls:
    n = S.label(target)
    if '::EnemyAIState_' in n or n.startswith('EnemyAIState_'):
        cls = n.split('::')[-1]
        if n.count('::') >= 1 and n.split('::')[-1] == n.split('::')[-2]:
            if pat.search(n):
                ctor_of.setdefault(caller, set()).add(n.rsplit('::', 1)[0])
callers = {}
for (caller, site, target, kind) in p.calls:
    callers.setdefault(target, []).append((caller, site))
for g in sorted(ctor_of):
    cls = sorted(ctor_of[g])
    gname = S.label(g)
    if '::EnemyAIState_' in gname or gname.startswith('EnemyAIState_'):
        # ctor itself calling base ctor; skip
        continue
    print('%08X %-28s -> %s' % (g, gname[:28], ', '.join(cls)))
    for c, site in sorted(callers.get(g, [])):
        print('      req from %08X in %s' % (site, S.label(c)))

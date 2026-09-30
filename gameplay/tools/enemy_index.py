"""Build the cross-family enemy index from the SET catalog + dataflow + RTTI.

For each SET descriptor in the enemy/boss id ranges:
  create hook (desc+0x0C) -> operator new(size) -> first call taking ret(new) as r3 = ctor
  resource hook (desc+0x04) -> referenced strings (resource files)
  class -> RTTI bases, AI-state classes named EnemyAIState_<X>AI_* / <ns>::EnemyAIState_*
Every value printed is traceable to the listed addresses (fail closed: '?' when not resolved).
Output: data/enemy_index.json (+ printed markdown table rows)
"""
import os
import re
import json
import pickle
from dol import get_dol, DATA_DIR
from symbols import get_symbols

OPNEW = 0x803A1380


def main():
    d = get_dol()
    S = get_symbols()
    df = pickle.load(open(os.path.join(DATA_DIR, 'dataflow.pkl'), 'rb'))
    cat = json.load(open(os.path.join(DATA_DIR, 'setobj_catalog.json'), encoding='utf-8'))
    classes = S.classes
    names = sorted(set(c['name'] for c in classes.values()))
    out = []
    for desc in cat:
        i = int(desc['id'], 16)
        if not (0x64 <= i <= 0xBE or desc['b1A'] == 2):
            continue
        ch = int(desc['hook0C'], 16)
        rh = int(desc['hook04'], 16)
        size = ctor = None
        extra = []
        calls = df.get(ch, {}).get('call', [])
        for k, (pc, t, args) in enumerate(calls):
            if t == OPNEW and args[0] and args[0][0] == 'k':
                size = args[0][1]
                cands = [(pc2, t2, a2) for pc2, t2, a2 in calls[k + 1:] if a2[0] == ('ret', OPNEW)]
                # prefer a callee proven to be a constructor (stores own vtable), else the last one
                pick = [c for c in cands if S.auto.get(c[1], ('', ''))[1] == 'ctor'] or cands[-1:]
                if pick:
                    ctor = pick[0][1]
                    extra = [repr(x) for x in pick[0][2][1:4]]
                break
        cname = S.name(ctor) if ctor else None
        cls = cname.split('::')[:-1] if cname and '::' in cname else None
        cls = '::'.join(cls) if cls else None
        strs = []
        for pc, a in df.get(rh, {}).get('cptr', []):
            s = d.printable_str(a, 4) if d.region(a) in ('.rodata', '.data', '.sdata', '.sdata2') else None
            if s:
                strs.append(s)
        ai = None
        states = []
        if cls:
            bases = [b['name'] for c in classes.values() if c['name'] == cls for b in c['bases']]
            ais = [b for b in bases if b.endswith('AI') and b not in ('EnemyBaseAI',)]
            ai = ais[-1] if ais else None
            if ai:
                short = ai.split('::')[-1]
                states = sorted(set(n.split('_' + short + '_')[-1] for n in names
                                    if ('EnemyAIState_' + short + '_') in n))
        row = {'id': desc['id'], 'set_name': desc['name'], 'desc': desc['desc'],
               'create_hook': desc['hook0C'], 'resource_hook': desc['hook04'],
               'alloc_size': ('0x%X' % size) if size else None,
               'ctor': ('%08X' % ctor) if ctor else None, 'class': cls,
               'ctor_args': extra, 'resources': strs, 'ai_class': ai, 'ai_states': states,
               'param_names': [p['name'] for p in desc['params']]}
        out.append(row)
    with open(os.path.join(DATA_DIR, 'enemy_index.json'), 'w') as fh:
        json.dump(out, fh, indent=1)
    for r in out:
        print('| %s | %s | %s | %s | %s | %s | %s | %s |' % (
            r['id'], r['set_name'], r['create_hook'], r['alloc_size'] or '?', r['class'] or '?',
            r['ai_class'] or '-', ', '.join(r['ai_states']) or '-', '; '.join(r['resources'])[:80]))


if __name__ == '__main__':
    main()

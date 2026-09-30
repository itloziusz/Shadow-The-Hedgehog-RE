"""Dump a behavior/control factory table: table of ptrs -> rodata descriptor whose word0 is a creator fn.
usage: python agent_player_behtab.py TABLE COUNT [words]"""
import sys
from dol import get_dol
from symbols import get_symbols
import q, program as P

d = get_dol(); S = get_symbols()
tab = int(sys.argv[1], 16); n = int(sys.argv[2]); w = int(sys.argv[3]) if len(sys.argv) > 3 else 3
for i in range(n):
    e = d.u32(tab + 4 * i)
    if not e:
        print('%2d (0x%02X)  NULL' % (i, i)); continue
    words = [d.u32(e + 4 * k) for k in range(w)]
    fn = words[0]
    # find what class the creator news: look for vtable stores in creator's callees
    lab = S.label(fn) if d.is_code(fn) else '?'
    cls = set()
    df = q.df()
    todo = [fn]; seen = set()
    depth = 0
    while todo and depth < 3:
        nxt = []
        for f in todo:
            if f in seen: continue
            seen.add(f)
            for pc, a in df.get(f, {}).get('cptr', []):
                if a in S.vt_class: cls.add(S.vt_class[a])
            for pc, t, args in df.get(f, {}).get('call', []):
                nm = S.label(t)
                if '::' in nm and nm.split('::')[-1] == nm.split('::')[-2]:
                    cls.add('ctor:' + nm)
                nxt.append(t)
        todo = nxt; depth += 1
    print('%2d (0x%02X)  desc %08X  words %s  creator %s  -> %s' % (i, i, e, ' '.join('%08X' % x for x in words), lab, ', '.join(sorted(cls))[:200]))

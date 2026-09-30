"""Table: behavior id -> class -> vtable slots (CanEnter, Enter, Leave, OnCommand, Update, vf06) + object size + name string."""
import re
from dol import get_dol
from symbols import get_symbols
import q
S = get_symbols(); d = get_dol()
TAB = 0x80522DE8
rows = []
for i in range(35):
    desc = d.u32(TAB + 4 * i)
    creator = d.u32(desc)
    # find operator_new size and ctor in creator
    size = None; ctor = None
    for pc, t, args in q.df().get(creator, {}).get('call', []):
        n = S.label(t)
        if n == 'operator_new' and args and args[0] and args[0][0] == 'k':
            size = args[0][1]
        elif n.endswith('::' + n.split('::')[-2]) if '::' in n else False:
            ctor = (t, n)
    cls = ctor[1].rsplit('::', 1)[0] if ctor else '?'
    # vtable: find class in S.classes by name
    vt = None
    for ti, c in S.classes.items():
        if c['name'] == cls and c.get('vtables'):
            vt = c['vtables'][0]
            break
    slots = []
    if vt:
        va = int(vt['addr'], 16) if isinstance(vt, dict) and 'addr' in vt else None
    # fallback: read ctor for vtable store
    name = None
    if ctor:
        for pc, a in q.df().get(ctor[0], {}).get('cptr', []):
            if a in S.vt_class and S.vt_class[a] == cls:
                va = a
        # name string from sdata load passed as r5 to Behavior ctor
        for pc, t, args in q.df().get(ctor[0], {}).get('call', []):
            if S.label(t) == 'Player::Behavior::Behavior::Behavior':
                r5 = args[2]; r6 = args[3]
                if r5 and r5[0] == 'l' and r5[1][0] == 'k':
                    p = d.u32((r5[1][1] + r5[2]) & 0xFFFFFFFF)
                    name = d.cstr(p) if p else None
                ctorid = r6[1] if r6 and r6[0] == 'k' else None
    sl = [d.u32(va + 8 + 4 * k) for k in range(7)] if va else []
    print('%2d 0x%02X %-36s size=%-5s name=%-16s ' % (i, i, cls, hex(size) if size else '?', name) +
          ' '.join('%08X' % s for s in sl[1:]))

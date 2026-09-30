"""Weapon class vtable survey (agent_weapon_*). Read-only.

  python agent_weapon_vt.py            every Weapon::* class: slots 0x0A..0x0E with the float constant each
                                        slot function returns in f1 (last `lfs f1` / `fmr f1` before blr) and
                                        all float constants it loads.
  python agent_weapon_vt.py CLASSREGEX restrict to matching classes

Weapon slot roles (from ControlGun::Update 0x8023C814): 0x0A Fire(dir) -> cooldown, 0x0B FireAt(target) ->
cooldown, 0x0C OnTriggerRelease -> cooldown, 0x0D OnFireStart, 0x0E OnFireEnd.
"""
import re
import sys
from q import prog, df, fend
from dol import get_dol
from symbols import get_symbols


def floats(f):
    d = get_dol()
    out = []
    for pc, base, disp, size, st, fp, val in sorted(df().get(f, {}).get('acc', [])):
        if base is not None and base[0] == 'k' and fp and not st:
            a = (base[1] + disp) & 0xFFFFFFFF
            out.append((pc, a, d.f32(a)))
    return out


def ret_float(f):
    """Float constant loaded into f1 last before the function's final blr (None if not a constant)."""
    pr = prog()
    d = get_dol()
    e = fend(f)
    fl = {pc: (a, v) for pc, a, v in floats(f)}
    a = e - 4
    while a >= f:
        x = pr.ins(a)
        if x.m == 'lfs' and x.ops and x.ops[0] == ('f', 1):
            return fl.get(a, (None, None))
        if x.ops and x.ops[0] == ('f', 1) and x.m not in ('stfs', 'stfd', 'psq_st'):
            return (None, 'f1<-' + x.m)
        a -= 4
    return (None, None)


def main(pat='^Weapon::'):
    S = get_symbols()
    rx = re.compile(pat)
    seen = set()
    for ti, c in sorted(S.classes.items(), key=lambda kv: kv[1]['name']):
        if not rx.search(c['name']) or 'boost' in c['name']:
            continue
        for vt in c['vtables']:
            key = (c['name'], vt['addr'])
            if key in seen or len(vt['slots']) <= 0x0A:
                continue
            seen.add(key)
            print('%s  vtbl %s (%d slots)' % (c['name'], vt['addr'], len(vt['slots'])))
            for i in range(0x0A, len(vt['slots'])):
                f = int(vt['slots'][i], 16)
                ra, rv = ret_float(f)
                fk = ', '.join('%g@%08X' % (v, a) for pc, a, v in floats(f) if v is not None)
                print('   [%02X] %08X %-40s ret=%s   consts: %s' % (
                    i, f, S.label(f), ('%g (%08X)' % (rv, ra)) if ra else rv, fk[:150]))


if __name__ == '__main__':
    main(*sys.argv[1:])

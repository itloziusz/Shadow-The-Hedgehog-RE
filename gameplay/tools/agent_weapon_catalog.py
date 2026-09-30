"""Dump the weapon SET catalog (ids 0xC8..0x114) with the weapon-info block and resolved factories.

  python agent_weapon_catalog.py            table to stdout
  python agent_weapon_catalog.py csv        CSV to stdout
  python agent_weapon_catalog.py registry   weapon registry 0x8051F058 (index -> info block) joined with the catalog

Info block = descriptor + 0x24 (registry 0x8051F058 entries point at it). Static C++ initialisers write the
name (+0x00) and two floats (+0x10/+0x14) at startup, so the DOL image alone shows zeros there; this script
overlays every constant store found in the dataflow cache onto the image values (column `si` marks them).
"""
import json
import os
import sys
from q import df
from dol import get_dol, DATA_DIR
from symbols import get_symbols

BULLET_INIT = 0x80051DF0
OTHER_INIT = 0x8005B888
REGISTRY = 0x8051F058


def call_args(f, target=None):
    out = []
    for pc, t, args in df().get(f, {}).get('call', []):
        if target is None or t == target:
            out.append((pc, t, args))
    return out


def const(v):
    return v[1] if v is not None and v[0] == 'k' else None


_stores = None


def static_stores():
    """addr -> (pc, value-tuple) for every store to a constant address (all functions)."""
    global _stores
    if _stores is None:
        _stores = {}
        for f, recs in df().items():
            for pc, base, disp, size, st, fp, val in recs.get('acc', []):
                if st and base is not None and base[0] == 'k':
                    _stores.setdefault((base[1] + disp) & 0xFFFFFFFF, []).append((f, pc, size, fp, val))
    return _stores


def word(a, as_float=False):
    d = get_dol()
    sts = static_stores().get(a)
    if sts:
        f, pc, size, fp, val = sts[0]
        if val is not None and val[0] == 'fk':
            return d.f32(val[1]), 'si'
        if val is not None and val[0] == 'l' and val[1][0] == 'k':      # pointer loaded from sdata
            p = d.u32((val[1][1] + val[2]) & 0xFFFFFFFF)
            return p, 'si'
        if val is not None and val[0] == 'k':
            return val[1], 'si'
        return None, 'si?'
    return (d.f32(a) if as_float else d.u32(a)), ''


def create_info(f):
    S = get_symbols()
    size = ctor = idx = params = vt = None
    for pc, t, args in call_args(f):
        name = S.label(t)
        if name == 'operator_new' and size is None:
            size = const(args[0])
        elif ctor is None and ('::' in name):
            ctor = name
            idx = const(args[2]) if len(args) > 2 else None
            params = const(args[3]) if len(args) > 3 else None
    for pc, a in df().get(f, {}).get('cptr', []):
        if a in S.vt_class:
            vt = S.vt_class[a]
    return size, ctor, idx, params, vt


def rows():
    d = get_dol()
    cat = json.load(open(os.path.join(DATA_DIR, 'setobj_catalog.json')))
    out = []
    for e in cat:
        i = int(e['id'], 16)
        if not (0xC8 <= i <= 0x114):
            continue
        desc = int(e['desc'], 16)
        info = desc + 0x24
        name, _ = word(info)
        nm = d.cstr(name).decode('latin-1') if name else ''
        f10, s10 = word(info + 0x10, True)
        f14, s14 = word(info + 0x14, True)
        create = d.u32(info + 4)
        size, ctor, idx, params, vt = create_info(create) if create else (None,) * 5
        hook = int(e['hook04'], 16)
        binit = [const(a[0]) for pc, t, a in call_args(hook, BULLET_INIT)]
        oinit = [const(a[0]) for pc, t, a in call_args(hook, OTHER_INIT)]
        out.append(dict(id=i, desc=desc, info=info, name=nm, create=create, item=d.u32(info + 8),
                        extra=d.u32(info + 0xC), f10=f10, f14=f14, si=(s10 + s14),
                        ammo=d.u32(info + 0x18), w1c=d.u32(info + 0x1C), cat=d.u32(info + 0x20),
                        f24=d.f32(info + 0x24), f28=d.f32(info + 0x28), f2c=d.f32(info + 0x2C), size=size, ctor=ctor, idx=idx,
                        params=params, vt=vt, hook=hook, binit=binit, oinit=oinit))
    return out


def fmtf(v):
    return '' if v is None else ('%g' % v)


def main(fmt='txt'):
    R = rows()
    if fmt == 'registry':
        d = get_dol()
        by_info = {r['info']: r for r in R}
        for w in range(0, 80):
            e = REGISTRY + 16 * w
            p = d.u32(e)
            if p is None:
                break
            r = by_info.get(p)
            print('%2d  %08X  info=%08X  +8=%-3d  +C=%08X  %s' % (
                w, e, p, (d.u32(e + 8) ^ 0x80000000) - 0x80000000, d.u32(e + 0xC),
                ('SET %04X %s' % (r['id'], r['name'])) if r else ('(not a catalog weapon)' if p else '(none)')))
        return
    if fmt == 'csv':
        print('set_id,name,desc,info,create,item,extra,f10,f14,ammo,info1C,category,f24,f28,new_size,base_ctor,'
              'ctor_index,params,class,hook04,bullet_init_idx,f2C')
        for r in R:
            print('%04X,%s,%08X,%08X,%08X,%08X,%08X,%s,%s,%d,%d,%04X,%s,%s,%s,%s,%s,%s,%s,%08X,%s,%s' % (
                r['id'], r['name'], r['desc'], r['info'], r['create'] or 0, r['item'] or 0, r['extra'] or 0,
                fmtf(r['f10']), fmtf(r['f14']), r['ammo'], r['w1c'], r['cat'], fmtf(r['f24']), fmtf(r['f28']),
                ('%X' % r['size']) if r['size'] else '', r['ctor'] or '',
                r['idx'] if r['idx'] is not None else '', ('%08X' % r['params']) if r['params'] else '',
                r['vt'] or '', r['hook'], ';'.join(str(x) for x in (r['binit'] or r['oinit'])), fmtf(r['f2c'])))
        return
    for r in R:
        print('%04X %-17s info=%08X f10=%-6s f14=%-6s%-4s ammo=%-3d 1C=%d cat=%04X f24=%-6s f28=%-6s f2C=%-5s new(%s) %s ctorIdx=%s params=%s vt=%s idx=%s' % (
            r['id'], r['name'], r['info'], fmtf(r['f10']), fmtf(r['f14']), r['si'], r['ammo'], r['w1c'], r['cat'],
            fmtf(r['f24']), fmtf(r['f28']), fmtf(r['f2c']), ('%X' % r['size']) if r['size'] else '?', r['ctor'], r['idx'],
            ('%08X' % r['params']) if r['params'] else '-', r['vt'], r['binit'] or r['oinit']))


if __name__ == '__main__':
    main(*(sys.argv[1:] or ['txt']))

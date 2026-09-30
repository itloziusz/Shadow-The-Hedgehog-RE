"""Recover MWCC RTTI: class names, base lists and vtables from main.dol.

Layout (verified on Weapon::Bullet::Cannon, see GAMEPLAY_STRUCTURES.md):
  typeinfo  (.sdata / .data)  : { const char *name; Base *bases; }
  Base list (.data)           : { typeinfo *ti; s32 offset; } ... { 0 }
  vtable    (.data)           : { typeinfo *ti; s32 this_adjust; void (*slot[])(); }
  object vptr points at &vtable.ti; slot i is loaded from vptr + 8 + 4*i.

Constructor/destructor evidence: a function storing the vtable address into
this+off (dataflow 'acc' store of ('k', vtable) to base ('a',3)).

Output: data/classes.json
"""
import os
import re
import json
import pickle
from collections import defaultdict
from dol import get_dol, DATA_DIR
import program as P

NAME_RE = re.compile(r'^[A-Za-z_]\w*(::[A-Za-z_]\w*)*(<.*>)?(::\w+)*$')


def find_typeinfos(d):
    tis = {}
    for sec in d.sections:
        if sec.is_text or sec.name in ('extab', 'extabindex'):
            continue
        for off in range(0, sec.size - 7, 4):
            a = sec.addr + off
            np = d.u32(a)
            if np is None or d.region(np) not in ('.rodata', '.sdata2', '.sdata', '.data'):
                continue
            s = d.printable_str(np, 2)
            if not s or not NAME_RE.match(s) or len(s) > 400:
                continue
            bp = d.u32(a + 4)
            if bp != 0 and d.region(bp) not in ('.data', '.sdata'):
                continue
            tis[a] = {'name': s, 'bases_ptr': bp}
    # validate base lists: entries must be typeinfos
    for a, t in list(tis.items()):
        bases = []
        bp = t['bases_ptr']
        ok = True
        if bp:
            for k in range(32):
                ti = d.u32(bp + 8 * k)
                if ti == 0:
                    break
                if ti not in tis:
                    ok = False
                    break
                bases.append((ti, d.s32(bp + 8 * k + 4)))
            else:
                ok = False
        t['bases'] = bases if ok else None
    # drop candidates whose base list does not parse (fail closed)
    return {a: t for a, t in tis.items() if t['bases'] is not None}


def find_vtables(d, prog, tis):
    code_starts = set(prog.starts)
    vts = {}
    for sec in d.sections:
        if sec.name not in ('.data', '.sdata', '.rodata'):
            continue
        for off in range(0, sec.size - 11, 4):
            a = sec.addr + off
            ti = d.u32(a)
            if ti not in tis:
                continue
            adj = d.s32(a + 4)
            if not (-0x10000 < adj <= 0):
                continue
            slots = []
            k = 0
            while True:
                v = d.u32(a + 8 + 4 * k)
                if v is None or v not in code_starts:
                    break
                slots.append(v)
                k += 1
            # a base list entry also looks like (ti, 0); require >=1 slot
            if not slots:
                continue
            vts[a] = {'ti': ti, 'adjust': adj, 'slots': slots}
    return vts


def ctor_dtor_evidence(df, vts):
    """Functions that store a vtable address into (arg-derived) object memory."""
    ev = defaultdict(list)
    for f, recs in df.items():
        for pc, base, disp, size, st, fp, val in recs.get('acc', []):
            if not st or val is None or val[0] != 'k':
                continue
            if val[1] in vts and base is not None and base[0] in ('a', 'l', 'ret'):
                ev[val[1]].append({'func': f, 'pc': pc, 'base': repr(base), 'off': disp})
    return ev


def main():
    d = get_dol()
    prog = P.load()
    df = pickle.load(open(os.path.join(DATA_DIR, 'dataflow.pkl'), 'rb'))
    tis = find_typeinfos(d)
    vts = find_vtables(d, prog, tis)
    ev = ctor_dtor_evidence(df, vts)
    # keep only typeinfos proven by structure: owning a vtable, being a base
    # of a kept typeinfo, or being referenced from code (dynamic_cast etc.)
    code_refs = set()
    for f, recs in df.items():
        for pc, a in recs.get('cptr', []):
            if a in tis:
                code_refs.add(a)
        for pc, base, disp, size, st, fp, val in recs.get('acc', []):
            if base is not None and base[0] == 'k' and base[1] + disp in tis:
                code_refs.add(base[1] + disp)
    data_refs = set()
    for sec in d.sections:
        if sec.is_text:
            continue
        for off in range(0, sec.size - 3, 4):
            v = d.u32(sec.addr + off)
            if v in tis and v != sec.addr + off:
                data_refs.add(v)
    keep = set(v['ti'] for v in vts.values()) | code_refs | data_refs
    changed = True
    while changed:
        changed = False
        for a in list(keep):
            for b, o in tis[a]['bases']:
                if b not in keep:
                    keep.add(b)
                    changed = True
    tis = {a: t for a, t in tis.items() if a in keep}
    for a, t in tis.items():
        t['code_ref'] = a in code_refs
    vts = {a: v for a, v in vts.items() if v['ti'] in tis}
    classes = {}
    for a, t in tis.items():
        classes['%08X' % a] = {
            'name': t['name'],
            'typeinfo': '%08X' % a,
            'bases': [{'typeinfo': '%08X' % b, 'name': tis[b]['name'], 'offset': o}
                      for b, o in t['bases']],
            'vtables': [],
            'code_ref': t['code_ref'],
        }
    for va, v in sorted(vts.items()):
        c = classes['%08X' % v['ti']]
        c['vtables'].append({
            'addr': '%08X' % va,
            'adjust': v['adjust'],
            'slots': ['%08X' % s for s in v['slots']],
            'vptr_writers': [{'func': '%08X' % e['func'], 'pc': '%08X' % e['pc'],
                              'base': e['base'], 'off': e['off']} for e in ev.get(va, [])],
        })
    out = os.path.join(DATA_DIR, 'classes.json')
    with open(out, 'w') as fh:
        json.dump(classes, fh, indent=1)
    nv = sum(len(c['vtables']) for c in classes.values())
    print('typeinfos', len(tis), 'vtables', nv,
          'classes with vtable', sum(1 for c in classes.values() if c['vtables']))


if __name__ == '__main__':
    main()

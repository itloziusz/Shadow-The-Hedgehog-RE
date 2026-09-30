"""Dump the SET object-type catalog embedded in main.dol.

Evidence chain (see STAGE_GAMEPLAY_DATA_FORMATS.md):
  fn_800CBC54 (stage SET load) passes table 0x8052C1A0 to fn_800CA7F0 and
  fn_800CA780.  The table is a NULL-terminated array of pointers to
  SetObjDesc (0x24 bytes).  fn_800CA7F0 matches desc->id (u16 @+0x18)
  against setid.bin entries and sets desc->flags(+0x1B) bit0; fn_800CA780
  then calls desc->+0x04 for each flagged descriptor.

SetObjDesc:
  +00 char*  name
  +04 fn     per-stage resource load hook   (called by fn_800CA780)
  +08 fn     (unknown, often empty stub)
  +0C fn     (unknown; create/instantiate candidate)
  +10 u32    ?
  +14 u32    ?
  +18 u16    object id  (matches SET record type and setid.bin id)
  +1A u8     ?
  +1B u8     flags (bit0 = used in current stage, runtime)
  +1C u8     ?
  +20 SetParamDesc* params (array of 0x28-byte entries, type==0 terminates)

SetParamDesc (0x28): +00 type, +04 name, +08..+1C type-specific,
  +20 accessor fn, +24 ?
Output: data/setobj_catalog.json and a text listing.
"""
import os
import json
import struct
from dol import get_dol, DATA_DIR

TABLE = 0x8052C1A0


def s(d, p):
    if not p:
        return None
    b = d.cstr(p)
    if b is None:
        return None
    try:
        return b.decode('shift_jis')
    except UnicodeDecodeError:
        return b.decode('latin1')


def f32(w):
    return struct.unpack('>f', struct.pack('>I', w))[0]


def main():
    d = get_dol()
    out = []
    k = 0
    while True:
        p = d.u32(TABLE + 4 * k)
        if not p:
            break
        k += 1
        desc = {
            'desc': '%08X' % p,
            'name': s(d, d.u32(p)),
            'id': '%04X' % d.u16(p + 0x18),
            'hook04': '%08X' % d.u32(p + 4),
            'hook08': '%08X' % d.u32(p + 8),
            'hook0C': '%08X' % d.u32(p + 0xC),
            'w10': '%08X' % d.u32(p + 0x10),
            'w14': '%08X' % d.u32(p + 0x14),
            'b1A': d.u8(p + 0x1A),
            'b1C': d.u8(p + 0x1C),
            'params': [],
        }
        pa = d.u32(p + 0x20)
        j = 0
        while pa and j < 64:
            e = pa + 0x28 * j
            ws = [d.u32(e + 4 * i) for i in range(10)]
            if ws[0] == 0 or ws[0] > 16:
                break
            ent = {'type': ws[0], 'name': s(d, ws[1]), 'raw': ['%08X' % w for w in ws[2:]]}
            if ws[0] == 4:
                ent['floats'] = [round(f32(w), 4) for w in ws[2:8]]
            out_acc = ws[8]
            ent['accessor'] = '%08X' % out_acc
            desc['params'].append(ent)
            j += 1
        out.append(desc)
    with open(os.path.join(DATA_DIR, 'setobj_catalog.json'), 'w', encoding='utf-8') as fh:
        json.dump(out, fh, indent=1, ensure_ascii=False)
    with open(os.path.join(DATA_DIR, 'setobj_catalog.txt'), 'w', encoding='utf-8') as fh:
        for desc in sorted(out, key=lambda x: int(x['id'], 16)):
            fh.write('%s %-28s desc=%s hooks=%s/%s/%s w10=%s w14=%s b1A=%d b1C=%d\n' % (
                desc['id'], desc['name'], desc['desc'], desc['hook04'], desc['hook08'],
                desc['hook0C'], desc['w10'], desc['w14'], desc['b1A'], desc['b1C']))
            for e in desc['params']:
                fh.write('      t%d %-32s %s %s\n' % (e['type'], e['name'], ' '.join(e['raw'][:6]),
                                                   e.get('floats', '')))
    print('descriptors', len(out))


if __name__ == '__main__':
    main()

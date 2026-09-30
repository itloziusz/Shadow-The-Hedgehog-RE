"""Parse Shadow the Hedgehog SET layout files (stgXXXX_{cmn,nrm,hrd,ds1}.dat)
using the object schema recovered from main.dol (setobj_catalog.json).

Format (PROVEN by fn_800CB1A4: magic check + byteswap format "ffffffiisccii"):
  header  : char magic[4]="sky2"; u32 count; u32 miscSize          (little-endian)
  record  : 0x2C bytes  f32 pos[3]; f32 rot[3] (degrees, fn_800CA2BC);
            u32 flagsA (runtime copy, overwritten from flagsB at load);
            u32 flagsB; u16 id; u8 link; u8 range(x100 units, fn_800CA970);
            u32 miscLen; u32 miscPtr (runtime)
  misc    : miscSize bytes, records' misc blocks in order, 4 bytes per param

Validation: for each record, miscLen must equal 4 * number of params in the
catalog entry for its id (fails closed: mismatches are reported, not fixed).

  python setparse.py            validate every stage, print summary
  python setparse.py 0100 [nrm] dump one stage file with named params
"""
import os
import sys
import json
import struct
from collections import Counter, defaultdict
from dol import DATA_DIR, ROOT

FILES = os.path.join(ROOT, 'files')
REC = struct.Struct('<6fIIHBBII')


def load_catalog():
    with open(os.path.join(DATA_DIR, 'setobj_catalog.json'), encoding='utf-8') as f:
        cat = json.load(f)
    by_id = {}
    for d in cat:
        by_id.setdefault(int(d['id'], 16), d)
    return by_id


def parse(path):
    b = open(path, 'rb').read()
    if b[:4] != b'sky2':
        raise ValueError('bad magic %r' % b[:4])
    count, misc_size = struct.unpack_from('<II', b, 4)
    recs = []
    off = 12
    for i in range(count):
        v = REC.unpack_from(b, off)
        recs.append({'pos': v[0:3], 'rot': v[3:6], 'flagsA': v[6], 'flagsB': v[7],
                     'id': v[8], 'link': v[9], 'range': v[10], 'miscLen': v[11], 'miscPtr': v[12]})
        off += 0x2C
    misc = b[off:off + misc_size]
    if len(misc) != misc_size:
        raise ValueError('misc truncated')
    mo = 0
    for r in recs:
        r['misc'] = misc[mo:mo + r['miscLen']]
        mo += r['miscLen']
    return recs, misc_size, mo, len(b) - (off + misc_size)


def decode_params(rec, desc):
    out = []
    m = rec['misc']
    for i, p in enumerate(desc['params'] if desc else []):
        if 4 * i + 4 > len(m):
            break
        raw = m[4 * i:4 * i + 4]
        if p['type'] == 4:
            v = round(struct.unpack('<f', raw)[0], 4)
        else:
            v = struct.unpack('<i', raw)[0]
        out.append((p['name'], v))
    return out


def validate():
    cat = load_catalog()
    tot = Counter()
    bad = defaultdict(list)
    ids = Counter()
    for stg in sorted(os.listdir(FILES)):
        if not stg.startswith('stg') or not os.path.isdir(os.path.join(FILES, stg)):
            continue
        for kind in ('cmn', 'nrm', 'hrd', 'ds1'):
            p = os.path.join(FILES, stg, '%s_%s.dat' % (stg, kind))
            if not os.path.exists(p):
                continue
            recs, ms, used, trailing = parse(p)
            tot['files'] += 1
            if used != ms:
                bad['misc_sum'].append((p, ms, used))
            for r in recs:
                tot['records'] += 1
                ids[r['id']] += 1
                d = cat.get(r['id'])
                if d is None:
                    bad['unknown_id'].append((stg, kind, '%04X' % r['id']))
                    continue
                n = len(d['params'])
                if r['miscLen'] != 4 * n:
                    bad['len_mismatch'].append((stg, kind, '%04X' % r['id'], d['name'], r['miscLen'], 4 * n))
                else:
                    tot['len_ok'] += 1
    print('files %(files)d  records %(records)d  miscLen==4*params: %(len_ok)d' % tot)
    for k, v in bad.items():
        c = Counter(x[2:] if k != 'misc_sum' else x for x in v)
        print('  %s: %d' % (k, len(v)), list(c.items())[:15])
    return ids


def dump(stage, kind):
    cat = load_catalog()
    p = os.path.join(FILES, 'stg' + stage, 'stg%s_%s.dat' % (stage, kind))
    recs, ms, used, trailing = parse(p)
    for i, r in enumerate(recs):
        d = cat.get(r['id'])
        name = d['name'] if d else '?'
        print('%4d %04X %-20s pos(%9.1f %9.1f %9.1f) rot(%6.1f %6.1f %6.1f) fl=%08X/%08X link=%d range=%d' % (
            i, r['id'], name, *r['pos'], *r['rot'], r['flagsA'], r['flagsB'], r['link'], r['range']))
        for n, v in decode_params(r, d):
            print('        %-24s %s' % (n, v))


if __name__ == '__main__':
    if len(sys.argv) > 1:
        dump(sys.argv[1], sys.argv[2] if len(sys.argv) > 2 else 'cmn')
    else:
        ids = validate()
        cat = load_catalog()
        print('most common ids:', [('%04X' % i, cat[i]['name'] if i in cat else '?', n) for i, n in ids.most_common(25)])

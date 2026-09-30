"""Parse files/common/ScoreData.bin (loaded by fn_80206868 at stage init).

Container (generic Sega data file, parser fn_8040F66C): header 0x20 bytes
  +0x00 fileSize, +0x04 u32, +0x08 u32, +0x0C u32 count(1), +0x17 'B' = big-endian ('1B' at +0x16)
Data at +0x20: u32 tableOffset (rel. to +0x20), u32 entryCount; entries 0x18 bytes:
  u32 v[5]; u32 nameOffset (rel. to +0x20)
Consumer: fn_802067B0(ctx, id) -> entry id (id 0x27 "Unknown" returns 0) -> fn_80206740 builds
ScoreCommand(v0..v4) and sends it through the target's command interface.
Output: data/score_table.csv
"""
import os
import csv
import struct
from dol import DATA_DIR, ROOT


def parse(path=os.path.join(ROOT, 'files', 'common', 'ScoreData.bin')):
    b = open(path, 'rb').read()
    if b[0x17:0x18] != b'B':
        raise ValueError('expected big-endian marker')
    base = 0x20
    off, n = struct.unpack_from('>II', b, base)
    rows = []
    for k in range(n):
        e = struct.unpack_from('>6I', b, base + off + 0x18 * k)
        s = base + e[5]
        name = b[s:b.index(b'\0', s)].decode('latin1')
        rows.append((k, name) + e[:5])
    return rows


if __name__ == '__main__':
    rows = parse()
    out = os.path.join(DATA_DIR, 'score_table.csv')
    with open(out, 'w', newline='') as fh:
        w = csv.writer(fh)
        w.writerow(['id', 'name', 'v0', 'v1', 'v2', 'v3', 'v4'])
        w.writerows(rows)
    print('wrote', out, len(rows), 'entries')

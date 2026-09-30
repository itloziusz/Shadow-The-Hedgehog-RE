"""Helper (mission trace agent): count per-team enemies in a stage's SET files using the exact rules of
the enemy-manager scan fn_801A2BA0 (EnemyManager at .bss 0x80580BD8):

  id in [0x64,0x96):
    0x64 GUN_SOLDIER : counted as team 0 unless param AppearType (misc word 9, struct +0x24) == 5
    0x91 BK_LARVA    : team 2 += param Num (misc word 7, struct +0x1C)
    else             : team = 0 if id < 0x78, 1 if id < 0x8C, else 2 ; += 1

  python agent_mission_enemycount.py 0100          (cmn + nrm + ds1, i.e. normal mode)
  python agent_mission_enemycount.py 0100 hrd      (cmn + hrd + ds1)
"""
import os
import sys
import struct
from collections import Counter
import setparse


def count(stage, mode='nrm'):
    tot = Counter()
    per_file = {}
    for kind in ('cmn', mode, 'ds1'):
        p = os.path.join(setparse.FILES, 'stg' + stage, 'stg%s_%s.dat' % (stage, kind))
        if not os.path.exists(p):
            continue
        recs = setparse.parse(p)[0]
        c = Counter()
        for r in recs:
            i = r['id']
            if not (0x64 <= i < 0x96):
                continue
            m = r['misc']
            if i == 0x64:
                appear = struct.unpack_from('<i', m, 4 * 9)[0] if len(m) >= 40 else None
                if appear != 5:
                    c[0] += 1
                else:
                    c['gun_appear5_skipped'] += 1
            elif i == 0x91:
                num = struct.unpack_from('<i', m, 4 * 7)[0] if len(m) >= 32 else 0
                c[2] += num
            else:
                c[0 if i < 0x78 else (1 if i < 0x8C else 2)] += 1
        per_file[kind] = dict(c)
        tot.update(c)
    return tot, per_file


if __name__ == '__main__':
    st = sys.argv[1]
    mode = sys.argv[2] if len(sys.argv) > 2 else 'nrm'
    tot, pf = count(st, mode)
    print('stage', st, mode, 'per file', pf)
    print('total GUN(0)=%d Eggman(1)=%d BlackArms(2)=%d  (GUN soldiers skipped AppearType==5: %d)' % (
        tot[0], tot[1], tot[2], tot['gun_appear5_skipped']))

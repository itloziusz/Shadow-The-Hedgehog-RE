"""List every SET_VEHICLE (0x004F) record in the stage SET files with its `Vehicle` param (read-only).

  python agent_weapon_veh_setscan.py          per-file rows + histogram of Vehicle values
Uses setparse.parse(); Vehicle param = first u32 (LE) of the record's misc block.
"""
import os
import struct
from collections import Counter
import setparse

NAMES = {1: 'Normal Car (Jeep)', 2: 'Open Car', 3: 'Armed Car (GUN)', 4: 'Bike', 5: 'Walker', 6: 'WalkerCannon',
         7: 'AirSaucer', 8: 'AirWing', 9: 'AirBolt', 10: 'BatteryArmy', 11: 'BatteryBlackKiller',
         12: 'GunLiftExpress', 13: 'GunLiftLocal'}


def main():
    hist = Counter()
    per_stage = {}
    for root, _, files in os.walk(setparse.FILES):
        for fn in sorted(files):
            if not (fn.startswith('stg') and fn.endswith('.dat')):
                continue
            try:
                recs, _, _, _ = setparse.parse(os.path.join(root, fn))
            except Exception:
                continue
            for r in recs:
                if r['id'] != 0x4F:
                    continue
                v = struct.unpack_from('<i', r['misc'], 0)[0] if len(r['misc']) >= 4 else None
                hist[v] += 1
                per_stage.setdefault(fn, Counter())[v] += 1
    for fn in sorted(per_stage):
        print('%-24s %s' % (fn, ', '.join('%s x%d' % (NAMES.get(k, k), n) for k, n in sorted(per_stage[fn].items(), key=lambda t: (t[0] is None, t[0])))))
    print()
    for k, n in sorted(hist.items(), key=lambda t: (t[0] is None, t[0])):
        print('Vehicle=%s (%s): %d records' % (k, NAMES.get(k, '?'), n))


if __name__ == '__main__':
    main()

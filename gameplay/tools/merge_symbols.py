"""Merge symbol fragments (notes/*_symbols.csv) into gameplay/symbols_curated.csv.

  python merge_symbols.py ../notes/enemy2_symbols.csv [more.csv ...]

Rows use `address,current_name,subsystem,confidence,evidence`. Addresses already present in the
curated file are skipped (the curated file wins; fix conflicts by hand). Afterwards run
`python export_symbols.py`.
"""
import os
import sys
import csv

HERE = os.path.dirname(os.path.abspath(__file__))
CURATED = os.path.normpath(os.path.join(HERE, '..', 'symbols_curated.csv'))


def norm(a):
    return a.strip().upper().replace('0X', '')


def main(paths):
    with open(CURATED, newline='') as f:
        have = {norm(r['address']): r['current_name'] for r in csv.DictReader(f)}
    total = 0
    with open(CURATED, 'a', newline='') as out:
        w = csv.writer(out)
        for p in paths:
            added = skipped = 0
            with open(p, newline='') as f:
                for r in csv.DictReader(f):
                    a = norm(r['address'])
                    if a in have:
                        if have[a] != r['current_name']:
                            print('  conflict %s: curated=%s fragment=%s (kept curated)' % (a, have[a], r['current_name']))
                        skipped += 1
                        continue
                    w.writerow([a, r['current_name'], r['subsystem'], r['confidence'], r['evidence']])
                    have[a] = r['current_name']
                    added += 1
            print('%s: added %d, skipped %d' % (p, added, skipped))
            total += added
    print('total added', total)


if __name__ == '__main__':
    main(sys.argv[1:])

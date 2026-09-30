"""Census of placed SET records for enemy ids across every stage file (read-only on files/).
usage: python agent_enemy2_census.py ID [ID...]      (hex SET ids, e.g. 64 65 79 8D)
Prints per id: record count, SearchWidth==0 count (by AppearType), value histograms of every int param,
link-id usage, and the stage distribution.
"""
import os, sys
from collections import Counter, defaultdict
import setparse as SP

if __name__ == '__main__':
    ids = [int(x, 16) for x in sys.argv[1:]]
    cat = SP.load_catalog()
    for eid in ids:
        d = cat[eid]
        names = [p['name'] or ('param%d' % i) for i, p in enumerate(d['params'])]
        types = [p['type'] for p in d['params']]
        n = 0; zero = Counter(); hist = defaultdict(Counter); stages = Counter(); links = Counter()
        fl = defaultdict(list)
        for stg in sorted(os.listdir(SP.FILES)):
            if not stg.startswith('stg') or not os.path.isdir(os.path.join(SP.FILES, stg)):
                continue
            for kind in ('cmn', 'nrm', 'hrd', 'ds1'):
                p = os.path.join(SP.FILES, stg, '%s_%s.dat' % (stg, kind))
                if not os.path.exists(p):
                    continue
                recs, *_ = SP.parse(p)
                for r in recs:
                    if r['id'] != eid:
                        continue
                    n += 1
                    stages[stg] += 1
                    links[r['link']] += 1
                    vals = dict(zip(names, [v for _, v in SP.decode_params(r, d)]))
                    for nm, t in zip(names, types):
                        if t != 4 and nm in vals:
                            hist[nm][vals[nm]] += 1
                    for nm in ('FloatWidth', 'AttackStart', 'AttackEnd', 'SparkDischarge', 'SparkWait', 'SearchWidth'):
                        if nm in vals:
                            fl[nm].append(vals[nm])
                    if vals.get('SearchWidth', 1) == 0:
                        zero[vals.get('AppearType', '?')] += 1
        print('== %04X %s: %d records; SearchWidth==0: %d by AppearType %s' % (eid, d['name'], n, sum(zero.values()), dict(zero)))
        for nm in names:
            if nm in hist:
                print('   %-20s %s' % (nm, dict(sorted(hist[nm].items()))))
        for nm, v in fl.items():
            c = Counter(v)
            print('   %-20s min %s max %s top %s' % (nm, min(v), max(v), c.most_common(5)))
        print('   link ids: %d distinct, link 0: %d' % (len(links), links.get(0, 0)))
        print('   stages: %s' % dict(stages.most_common(40)))

"""Find indirect calls whose ctr path matches a regex (fmtv form). usage: agent_player_icall.py REGEX [lo hi]"""
import sys, re
from q import df, fmtv
from symbols import get_symbols
S = get_symbols()
rx = re.compile(sys.argv[1])
lo = int(sys.argv[2], 16) if len(sys.argv) > 2 else 0
hi = int(sys.argv[3], 16) if len(sys.argv) > 3 else 0xFFFFFFFF
for f, recs in sorted(df().items()):
    if not (lo <= f < hi):
        continue
    for pc, ctr, args in recs.get('icall', []):
        s = fmtv(ctr)
        if rx.search(s):
            print('%08X %-45s %s' % (pc, S.label(f), s))

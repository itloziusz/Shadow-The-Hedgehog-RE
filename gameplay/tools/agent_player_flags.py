"""Cross-reference player state-flag bits.
+0xA8 multiword bitset: set fn_80076DDC, clear fn_80076DFC, test fn_8006CEA0.
+0xA0 single-word bitset: set fn_80014E1C, clear fn_800613A4, test BitTest 80014E34.
usage: agent_player_flags.py [A8|A0] [bit]"""
import sys
from collections import defaultdict
from q import df, fmtv
from symbols import get_symbols
S = get_symbols()
which = sys.argv[1] if len(sys.argv) > 1 else 'A8'
only = int(sys.argv[2], 0) if len(sys.argv) > 2 else None
if which == 'A8':
    ops = {0x80076DDC: 'set', 0x80076DFC: 'clr', 0x8006CEA0: 'test'}; off = 0xA8
else:
    ops = {0x80014E1C: 'set', 0x800613A4: 'clr', 0x80014E34: 'test'}; off = 0xA0
res = defaultdict(lambda: defaultdict(set))
for f, recs in df().items():
    for pc, t, args in recs.get('call', []):
        if t in ops and args and len(args) > 1:
            a0, a1 = args[0], args[1]
            if a1 is None or a1[0] != 'k':
                continue
            if a0 is None or a0[0] != '+' or a0[2] != off:
                continue
            res[a1[1]][ops[t]].add('%s@%08X' % (S.label(f), pc))
for bit in sorted(res):
    if only is not None and bit != only:
        continue
    d = res[bit]
    print('bit 0x%02X (%d): set %d clr %d test %d' % (bit, bit, len(d['set']), len(d['clr']), len(d['test'])))
    for k in ('set', 'clr', 'test'):
        if d[k] and (only is not None or len(d[k]) <= 8):
            print('   %-4s %s' % (k, ', '.join(sorted(d[k]))))
        elif d[k]:
            print('   %-4s %s ...' % (k, ', '.join(sorted(d[k])[:8])))

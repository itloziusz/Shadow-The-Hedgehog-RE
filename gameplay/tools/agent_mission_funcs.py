"""Helper (mission trace agent): list functions in an address range with names and caller counts.

  python agent_mission_funcs.py START END            functions in [START, END)
  python agent_mission_funcs.py icall DISP [FILTER]  indirect calls whose ctr is [[X]+DISP] (any X), optionally
                                                     restricted to functions whose label matches FILTER regex
"""
import sys
import re
import q
import program as P
from symbols import get_symbols


def list_range(s, e):
    pr = q.prog()
    S = get_symbols()
    idx = q.index()
    for (fs, fe) in pr.funcs:
        if s <= fs < e:
            c = idx['callers'].get(fs, [])
            dp = [a for a, v in pr.dataptrs.items() if v == fs]
            print('%08X-%08X %-60s callers=%d %s dataptr=%s' % (
                fs, fe, S.label(fs), len(c),
                ','.join('%08X' % p for f, p in c[:6]),
                ','.join('%08X' % a for a in dp[:4])))


def icall_disp(disp, filt=None):
    S = get_symbols()
    rx = re.compile(filt) if filt else None
    for f, recs in q.df().items():
        lab = S.label(f)
        if rx and not rx.search(lab):
            continue
        for pc, ctr, args in recs.get('icall', []):
            if ctr is not None and ctr[0] == 'l' and ctr[2] == disp:
                print('%08X in %-50s ctr=%s args=%s' % (pc, lab, q.fmtv(ctr), [q.fmtv(a) for a in args[:4]]))


if __name__ == '__main__':
    if sys.argv[1] == 'icall':
        icall_disp(int(sys.argv[2], 16), sys.argv[3] if len(sys.argv) > 3 else None)
    else:
        list_range(int(sys.argv[1], 16), int(sys.argv[2], 16))

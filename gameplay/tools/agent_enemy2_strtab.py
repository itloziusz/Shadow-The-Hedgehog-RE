"""Dump a table of string pointers (SET-editor enum labels etc.).
usage: python agent_enemy2_strtab.py ADDR [COUNT]      (stops at first non-string pointer if COUNT omitted)
       python agent_enemy2_strtab.py --range LO HI     (all words in [LO,HI) that point at C strings)
"""
import sys
from dol import get_dol


def is_str_ptr(d, p):
    try:
        s = d.cstr(p)
    except Exception:
        return None
    if s is None or len(s) == 0 or len(s) > 80:
        return None
    if not all(32 <= c < 127 for c in s):
        return None
    return s.decode('ascii')


if __name__ == '__main__':
    d = get_dol()
    a = sys.argv[1:]
    if a[0] == '--range':
        lo, hi = int(a[1], 16), int(a[2], 16)
        for x in range(lo, hi, 4):
            p = d.u32(x)
            s = is_str_ptr(d, p) if p else None
            print('%08X  %08X  %s' % (x, p, repr(s) if s is not None else ''))
        sys.exit()
    base = int(a[0], 16)
    n = int(a[1]) if len(a) > 1 else 64
    for i in range(n):
        p = d.u32(base + 4 * i)
        s = is_str_ptr(d, p)
        if s is None and len(a) == 1:
            break
        print('[%2d] %08X -> %08X %s' % (i, base + 4 * i, p, repr(s)))

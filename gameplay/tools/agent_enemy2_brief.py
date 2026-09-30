"""Condensed annotated disassembly for reading state code quickly.
usage: python agent_enemy2_brief.py LO HI [FAMILY]
Drops prologue/epilogue noise; resolves 'lwz r12,OFF(r12)' virtual slots against the component whose vptr was
loaded (heuristic, by the vptr-field displacement 0xC/0x18/0x0 and the family table in agent_enemy2_vt.py).
"""
import re, sys
import q
from symbols import get_symbols

NOISE = re.compile(r'\s(stwu r1|mflr r0|mtlr r0|stmw |lmw |stfd f3[01], 0x|lfd f3[01], 0x|psq_st f3|psq_l f3|addi r1, r1|stw r0, 0x[0-9A-F]+\(r1\)|lwz r0, 0x[0-9A-F]+\(r1\)|stw r3[01], 0x[0-9A-F]+\(r1\)|lwz r3[01], 0x[0-9A-F]+\(r1\)|mtctr r12|crxor)')

if __name__ == '__main__':
    lo, hi = int(sys.argv[1], 16), int(sys.argv[2], 16)
    pr = q.prog()
    import io, contextlib
    for (s, e) in pr.funcs:
        if not (lo <= s < hi):
            continue
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            q.cmd_dis('0x%X' % s)
        for line in buf.getvalue().splitlines():
            if NOISE.search(line):
                continue
            line = re.sub(r'\s+;\s*$', '', line)
            print(line.rstrip())
        print()

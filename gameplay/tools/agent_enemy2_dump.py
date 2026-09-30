"""Annotated disassembly (q.py dis) of every function in [LO, HI).
usage: python agent_enemy2_dump.py LO HI > out.txt
"""
import sys
import q
import program as P

if __name__ == '__main__':
    lo, hi = int(sys.argv[1], 16), int(sys.argv[2], 16)
    pr = q.prog()
    for (s, e) in pr.funcs:
        if lo <= s < hi:
            q.cmd_dis('0x%X' % s)
            print()

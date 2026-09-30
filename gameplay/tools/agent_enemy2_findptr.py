"""Find every aligned word in the DOL equal to one of the given values (data pointer search).
usage: python agent_enemy2_findptr.py VAL [VAL...]    (hex)
"""
import sys, struct
from dol import get_dol

if __name__ == '__main__':
    d = get_dol()
    vals = set(int(v, 16) for v in sys.argv[1:])
    for s in d.sections:
        base = s.addr
        raw = d.raw[s.file_off:s.file_off + s.size]
        for o in range(0, len(raw) - 3, 4):
            w = struct.unpack_from('>I', raw, o)[0]
            if w in vals:
                print('%08X: %08X' % (base + o, w))

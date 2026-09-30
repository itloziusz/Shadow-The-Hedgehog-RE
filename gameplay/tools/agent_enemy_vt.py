"""Resolve GunSoldier virtual calls: python agent_enemy_vt.py COMP OFF [OFF...]
COMP = byte offset of vptr field inside the complete GunSoldier object (hex) or name:
  main(0x18) param(0x104) status(0x110) disp(0x1B4) move(0x1E8) set(0x250) ai(0x378) task(0x3F8)
OFF  = byte offset used in 'lwz r12, OFF(r12)' (hex).  Also: python agent_enemy_vt.py state VTADDR OFF
"""
import sys
from dol import get_dol
from symbols import get_symbols
V = 0x80539E64
VPTR = {0x18: V, 0x104: V + 0x14, 0x110: V + 0x8C, 0x1B4: V + 0x160, 0x1E8: V + 0x1C8,
        0x250: V + 0x200, 0x378: V + 0x2A4, 0x3F8: V + 0x308}
NAMES = {'main': 0x18, 'param': 0x104, 'status': 0x110, 'disp': 0x1B4, 'move': 0x1E8,
         'set': 0x250, 'ai': 0x378, 'task': 0x3F8}


def resolve(comp, off):
    d = get_dol()
    vp = VPTR[comp]
    t = d.u32(vp + off)
    return vp, (off - 8) // 4, t


if __name__ == '__main__':
    S = get_symbols()
    d = get_dol()
    if sys.argv[1] == 'state':
        vt = int(sys.argv[2], 16)
        for o in sys.argv[3:]:
            off = int(o, 16)
            t = d.u32(vt + off)
            print('state vt %08X +%X slot %X -> %08X %s' % (vt, off, (off - 8) // 4, t, S.label(t)))
        sys.exit()
    c = sys.argv[1]
    comp = NAMES[c] if c in NAMES else int(c, 16)
    for o in sys.argv[2:]:
        off = int(o, 16)
        vp, slot, t = resolve(comp, off)
        print('%s(vptr %08X) +%X slot %02X -> %08X %s' % (c, vp, off, slot, t, S.label(t)))

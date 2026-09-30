"""Resolve virtual calls for BkSoldier / EggPawn / GunBeetle (and GunSoldier) objects.
usage: python agent_enemy2_vt.py FAMILY COMP OFF [OFF...]
FAMILY: gun | bk | pawn | beetle.  COMP: main param status disp move set ai task (or hex vptr-field offset).
OFF = byte offset used in 'lwz r12, OFF(r12)'.
"""
import sys
from dol import get_dol
from symbols import get_symbols

FAM = {
    # V, {vptr field offset: V + k}
    'gun':    (0x80539E64, {0x18: 0, 0x104: 0x14, 0x110: 0x8C, 0x1B4: 0x160, 0x1E8: 0x1C8, 0x250: 0x200, 0x378: 0x2A4, 0x3F8: 0x308}),
    'bk':     (0x8053246C, {0x18: 0, 0x104: 0x14, 0x110: 0x8C, 0x1B4: 0x160, 0x1E8: 0x1C8, 0x250: 0x200, 0x394: 0x294, 0x418: 0x2F8}),
    'pawn':   (0x8052DAC0, {0x18: 0, 0x104: 0x14, 0x110: 0x8C, 0x1B4: 0x160, 0x1E8: 0x1C8, 0x250: 0x200, 0x30C: 0x2A4, 0x390: 0x308}),
    'beetle': (0x80537754, {0x18: 0, 0x104: 0x14, 0x110: 0x8C, 0x1B4: 0x160, 0x1E8: 0x1C8, 0x250: 0x200, 0x3BC: 0x28C, 0x450: 0x2F0}),
}
ORDER = ['main', 'param', 'status', 'disp', 'move', 'set', 'ai', 'task']

if __name__ == '__main__':
    d = get_dol(); S = get_symbols()
    fam = sys.argv[1]
    V, m = FAM[fam]
    keys = sorted(m)
    c = sys.argv[2]
    comp = keys[ORDER.index(c)] if c in ORDER else int(c, 16)
    vp = V + m[comp]
    for o in sys.argv[3:]:
        off = int(o, 16)
        t = d.u32(vp + off)
        print('%s %s(vptr %08X) +%X slot %02X -> %08X %s' % (fam, c, vp, off, (off - 8) // 4, t, S.label(t)))

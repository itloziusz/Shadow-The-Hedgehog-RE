"""Extract the inter-object command catalog (CharCommand type ids) from main.dol.

Every *Command constructor calls its base constructor with a constant type id:
  CharCommand::CharCommand(this, type, ...)          0x8006E57C  -> core ids 0x001..0x01D
  Player::PlayerCommand::PlayerCommand(this, type)   -> 0x101..0x137
  GadgetCommand::GadgetCommand::GadgetCommand        -> 0x301..0x304
  Player::Npc::NpcCommand::NpcCommand                -> 0x401..0x403
The id is stored as u16 at command+4 and switched on by receivers
(e.g. Player_HandleCommand 0x800A7CE4). Output: data/commands.csv
"""
import os
import csv
import pickle
from symbols import get_symbols
from dol import DATA_DIR

BASE_CTOR_NAMES = ('CharCommand', 'PlayerCommand', 'NpcCommand', 'GadgetCommand')


def main():
    S = get_symbols()
    df = pickle.load(open(os.path.join(DATA_DIR, 'dataflow.pkl'), 'rb'))
    bases = {a: n for a, (n, k) in S.auto.items()
             if k == 'ctor' and n.split('::')[-1] in BASE_CTOR_NAMES}
    rows = {}
    for f, recs in df.items():
        for pc, t, args in recs.get('call', []):
            if t not in bases or not args[1] or args[1][0] != 'k':
                continue
            name = S.name(f) or 'fn_%08X' % f
            cls = name.rsplit('::', 1)[0] if '::' in name else name
            key = (args[1][1], cls)
            rows.setdefault(key, (args[1][1], cls, bases[t].rsplit('::', 1)[0], '%08X' % f, '%08X' % pc))
    out = os.path.join(DATA_DIR, 'commands.csv')
    with open(out, 'w', newline='') as fh:
        w = csv.writer(fh)
        w.writerow(['type', 'class', 'base', 'ctor', 'base_call_pc'])
        for r in sorted(rows.values()):
            w.writerow(['0x%03X' % r[0]] + list(r[1:]))
    print('wrote', out, len(rows))


if __name__ == '__main__':
    main()

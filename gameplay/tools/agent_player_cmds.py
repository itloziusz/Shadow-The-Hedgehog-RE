"""List command classes with their u16 command id (CharCommand +4), from ctor code.
Scans every ctor symbol of *Command classes for 'li r4,ID' preceding bl to a base-command ctor."""
import csv, re, os
from dol import get_dol
import program as P
from q import prog, fend
from symbols import get_symbols

S = get_symbols(); d = get_dol(); pr = prog()
rows = []
with open(os.path.join(os.path.dirname(__file__), '..', 'GAMEPLAY_SYMBOLS.csv'), newline='', encoding='utf-8') as f:
    for r in csv.reader(f):
        if len(r) > 1 and re.search(r'Command::\w*Command$', r[1]) and '~' not in r[1]:
            rows.append((int(r[0], 16), r[1]))
BASES = {'CharCommand::CharCommand', 'Player::PlayerCommand::PlayerCommand', 'Player::Npc::NpcCommand::NpcCommand'}
out = {}
for a, name in sorted(rows):
    cls = name.rsplit('::', 1)[0]
    e = fend(a)
    r4 = None; found = None
    x = a
    while x < e:
        i = pr.ins(x)
        t = i.text()
        m = re.match(r'li r4, (-?0x[0-9A-Fa-f]+|-?\d+)$', t)
        if m:
            r4 = int(m.group(1), 0)
        if i.target is not None and t.startswith('bl '):
            tn = S.label(i.target)
            if tn in BASES or (tn.endswith('Command') and tn != name and '::' in tn):
                found = (tn, r4)
                break
        x += 4
    out.setdefault(cls, set()).add((found[1] if found else None, found[0] if found else None, a))
for cls in sorted(out):
    for idv, base, a in sorted(out[cls], key=lambda z: z[2]):
        print('%-45s id=%-6s ctor=%08X base=%s' % (cls, ('0x%X' % idv) if idv is not None else '?', a, base))

"""Helper (mission trace agent): rebuild the effective mission parameter table.

Sources (all evidence-backed, see gameplay/notes/mission_trace.md):
  * stage table  .rodata 0x804C5AE8, 0x3B entries x 0x50   (fn_80177020 / fn_80176FC8)
      +0x00 stage id, +0x04 char* section name, +0x08 flags,
      +0x1C + 0xC*slot : next-stage table index for slot (0=Dark,1=Normal,2=Hero)
      +0x20 + 0xC*slot : mission key                    (fn_80176FC8)
      +0x24 + 0xC*slot : u32 (text/message id?)          (unknown consumer)
      +0x40            : mission key for "hard/expert" mode (fn_8016E4EC, fn_801DF470 MISSIONCOUNT_HARD)
  * default descriptors inserted by fn_8016CB48 via fn_8016C8EC(map, key, type, param) -> {type, param, 1, 0}
  * overrides from files/nukkoro2.inf  MISSIONCOUNT_{D,N,H,HARD} : <success count> <fail count>
      parsed by fn_801DF3F8 / fn_801DF420 / fn_801DF448 / fn_801DF3D0 -> fn_801DF470 -> fn_8016C690
  * factory fn_8016B768 : type -> class and which descriptor words feed the ctor

  python agent_mission_table.py            markdown table
"""
import os
import re
from dol import get_dol
import q

STAGE_TAB = 0x804C5AE8
N_STAGES = 0x3B
TYPE_NAMES = {0: 'Mission', 1: 'GoalMission', 2: 'CountMission', 3: 'EnemyMission', 4: 'TimerMission',
              5: 'TimerGoalMission', 6: 'TimerCountMission', 7: 'RingMission'}
ENEMY_TEAM = {0: 'GUN (SET 0x64-0x77)', 1: 'Eggman (SET 0x78-0x8B)', 2: 'Black Arms (SET 0x8C-0x95)'}
SLOT_NAMES = ['Dark', 'Normal', 'Hero']
INF = os.path.join(os.path.dirname(__file__), '..', '..', 'files', 'nukkoro2.inf')


def s32(v):
    return v - (1 << 32) if v & 0x80000000 else v


def defaults():
    """Parse fn_8016CB48: li r4,key / li r5,type / li r6,param ; bl fn_8016C8EC."""
    pr = q.prog()
    out = {}
    regs = {}
    a, e = 0x8016CB48, 0x8016CEB4
    while a < e:
        x = pr.ins(a)
        t = x.text()
        m = re.match(r'li r(\d+), (-?0x[0-9A-F]+|-?\d+)', t)
        if m:
            regs[int(m.group(1))] = int(m.group(2), 0)
        if x.m == 'bl' and x.target == 0x8016C8EC:
            out[regs[4]] = [regs[5], regs[6], 1, 0, 'default@%08X' % a]
        a += 4
    return out


def stages():
    d = get_dol()
    res = []
    for i in range(N_STAGES):
        b = STAGE_TAB + 0x50 * i
        w = [d.u32(b + 4 * k) for k in range(20)]
        res.append({'idx': i, 'id': w[0], 'name': d.cstr(w[1]).decode('ascii', 'replace'), 'flags': w[2],
                    'slots': [(s32(w[7 + 3 * s]), s32(w[8 + 3 * s]), s32(w[9 + 3 * s])) for s in range(3)],
                    'hard': s32(w[16]), 'w44': w[17], 'w48': s32(w[18]), 'w4C': w[19]})
    return res


def parse_inf():
    """Return list of (section, cmd, a, b, lineno) in file order."""
    out = []
    sec = None
    with open(INF, 'rb') as f:
        for n, raw in enumerate(f.read().decode('shift_jis', 'replace').splitlines(), 1):
            line = raw.strip()
            if line.startswith('['):
                sec = line[1:line.index(']')]
                continue
            if line.startswith('#') or ':' not in line:
                continue
            cmd, rest = [p.strip() for p in line.split(':', 1)]
            if cmd.startswith('MISSIONCOUNT_'):
                nums = [int(v, 0) for v in rest.split()]
                if len(nums) >= 2:
                    out.append((sec, cmd, nums[0], nums[1], n))
    return out


def effective():
    desc = defaults()
    st = stages()
    byname = {s['name']: s for s in st}
    applied = []
    for sec, cmd, a, b, ln in parse_inf():
        s = byname.get(sec)
        if s is None:
            continue
        if cmd == 'MISSIONCOUNT_HARD':
            key = s['hard']
        else:
            slot = {'MISSIONCOUNT_D': 0, 'MISSIONCOUNT_N': 1, 'MISSIONCOUNT_H': 2}[cmd]
            key = s['slots'][slot][1]
        if key in desc:
            desc[key][2] = a
            desc[key][3] = b
            desc[key][4] = 'nukkoro2.inf:%d [%s] %s' % (ln, sec, cmd)
            applied.append((sec, cmd, key, a, b, ln))
    return st, desc, applied


def ctor_params(t, p, c1, c2):
    if t == 3:
        return 'team=%s, required=%d (fail count forced 0)' % (ENEMY_TEAM.get(p, p), c1)
    if t == 2:
        return 'required=%d, fail=%d' % (c1, c2)
    if t == 7:
        return 'required rings=%d' % c1
    if t in (4, 5):
        return 'time=%ds' % c1
    if t == 6:
        return 'required=%d, time=%ds' % (c1, c2)
    return '-'


def main():
    st, desc, applied = effective()
    print('## Default descriptor map (fn_8016CB48)\n')
    print('| key | type | param(+4) |')
    print('|---|---|---|')
    for k in sorted(desc):
        print('| 0x%02X | %d %s | %d |' % (k, desc[k][0], TYPE_NAMES[desc[k][0]], desc[k][1]))
    print('\n## Effective per-stage missions\n')
    print('| idx | stage | name | slot | next idx (stage) | key | class | ctor params | count source |')
    print('|---|---|---|---|---|---|---|---|---|')
    for s in st:
        rows = [(SLOT_NAMES[i],) + s['slots'][i] for i in range(3)] + [('Hard/X', None, s['hard'], None)]
        for sname, nxt, key, txt in rows:
            if key is None or key < 0:
                continue
            dsc = desc.get(key)
            nx = ''
            if nxt is not None and nxt >= 0:
                nx = '%d (%d)' % (nxt, st[nxt]['id'])
            elif nxt is not None:
                nx = str(nxt)
            if dsc is None:
                print('| %d | %d | %s | %s | %s | 0x%02X | (no descriptor) | | |' % (s['idx'], s['id'], s['name'], sname, nx, key))
                continue
            t, p, c1, c2, src = dsc
            print('| %d | %d | %s | %s | %s | 0x%02X | %s | %s | %s |' % (
                s['idx'], s['id'], s['name'], sname, nx, key, TYPE_NAMES[t], ctor_params(t, p, c1, c2), src))
    # key sharing check
    use = {}
    for s in st:
        for i in range(3):
            k = s['slots'][i][1]
            if k >= 0:
                use.setdefault(k, []).append('%s/%s' % (s['name'], SLOT_NAMES[i]))
        if s['hard'] >= 0:
            use.setdefault(s['hard'], []).append('%s/Hard' % s['name'])
    print('\n## Keys used by more than one stage/slot\n')
    for k in sorted(use):
        if len(use[k]) > 1:
            print('- 0x%02X (%s): %s' % (k, TYPE_NAMES[desc[k][0]] if k in desc else '?', ', '.join(use[k])))


def param_rows():
    """Rows in the notes format | Stage | Mission | Parameter | Value/source | Consumer address | Confidence |"""
    st, desc, applied = effective()
    print('| Stage | Mission | Parameter | Value/source | Consumer address | Confidence |')
    print('|---|---|---|---|---|---|')
    for s in st:
        rows = [(SLOT_NAMES[i], i) + s['slots'][i] for i in range(3)] + [('Hard', 1, None, s['hard'], None)]
        for sname, slot, nxt, key, txt in rows:
            if key is None or key < 0:
                continue
            if sname == 'Hard' and key == 0:
                continue  # plain goal-ring mission in hard mode; omitted for brevity
            dsc = desc.get(key)
            if dsc is None:
                continue
            t, p, c1, c2, src = dsc
            stg = '%d %s' % (s['id'], s['name'])
            keysrc = ('stage table 0x%08X +0x%X' % (STAGE_TAB + 0x50 * s['idx'], 0x20 + 0xC * slot)) if sname != 'Hard' \
                else ('stage table 0x%08X +0x40' % (STAGE_TAB + 0x50 * s['idx']))
            print('| %s | %s | mission key | 0x%02X (%s) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |' % (
                stg, sname, key, keysrc))
            print('| %s | %s | class (desc.type) | %d = %s (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |' % (
                stg, sname, t, TYPE_NAMES[t]))
            if t == 3:
                print('| %s | %s | enemy team (desc+4) | %d = %s (fn_8016CB48) | EnemyMission+0x58 -> EnemyMission::vf0B 0x801F3CBC | PROVEN |' % (
                    stg, sname, p, ENEMY_TEAM.get(p, p)))
            if t in (2, 3, 7, 6):
                print('| %s | %s | required count (desc+8) | %d (%s) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |' % (
                    stg, sname, c1, src))
            if t == 2 and c2:
                print('| %s | %s | fail count (desc+0xC) | %d (%s) | CountMission+0xC, CountMission::vf05 0x8016B0A4 -> Fail 0x8016B654 | PROVEN |' % (
                    stg, sname, c2, src))
            if t in (4, 5):
                print('| %s | %s | time limit s (desc+8) | %d (%s) | TimerMission+0x8 (float), fn_8016E4EC min-time -> stage timer .bss 0x8057E7E8 | PROVEN |' % (
                    stg, sname, c1, src))
            if t == 6:
                print('| %s | %s | time limit s (desc+0xC) | %d (%s) | TimerCountMission ctor f1 (0x8016B9DC) | PROVEN |' % (
                    stg, sname, c2, src))
            if nxt is not None and nxt >= 0:
                print('| %s | %s | next stage on clear | idx %d = stage %d (table +0x%X) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |' % (
                    stg, sname, nxt, st[nxt]['id'], 0x1C + 0xC * slot))


def stage_dump():
    st = stages()
    print('| idx | addr | id | name | flags | Dark (next,key,+0x24) | Normal (next,key,+0x30) | Hero (next,key,+0x3C) | +0x40 hard key | +0x44 | +0x48 | +0x4C |')
    print('|---|---|---|---|---|---|---|---|---|---|---|---|')
    for s in st:
        def g(t):
            return '%d, %d, 0x%X' % (t[0], t[1], t[2] & 0xFFFFFFFF) if t[2] >= 0 else '%d, %d, %d' % t
        print('| %d | 0x%08X | %d | %s | 0x%X | %s | %s | %s | %d | 0x%X | %s | %d |' % (
            s['idx'], STAGE_TAB + 0x50 * s['idx'], s['id'], s['name'], s['flags'], g(s['slots'][0]), g(s['slots'][1]),
            g(s['slots'][2]), s['hard'], s['w44'], ('0x%X' % s['w48']) if s['w48'] >= 0 else str(s['w48']), s['w4C']))


if __name__ == '__main__':
    import sys
    if len(sys.argv) > 1 and sys.argv[1] == 'rows':
        param_rows()
    elif len(sys.argv) > 1 and sys.argv[1] == 'stages':
        stage_dump()
    else:
        main()

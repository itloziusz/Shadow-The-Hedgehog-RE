"""Coarse subsystem classification of every function in main.dol.

Evidence per function, strongest first:
  1. library region (bounded by SDK/middleware version & diagnostic strings)
  2. RTTI class ownership (vtable slot / ctor / dtor)  -> family by namespace rule
  3. referenced strings (resource paths, debug names)  -> family by keyword rule
  4. neighbour inheritance: an unlabelled run between two functions of the
     same family (same translation unit, MWCC links TUs contiguously) gets
     that family with confidence LIKELY.
Output: data/func_families.json, data/family_runs.txt
"""
import os
import re
import json
from collections import Counter, defaultdict
import program as P
from dol import get_dol, DATA_DIR
from symbols import get_symbols
from q import df

# Library regions: (start, end, family, evidence)
LIB_REGIONS = [
    (0x80003100, 0x80005600, 'runtime', '.init section (startup, memcpy/memset)'),
    (0x8035C000, 0x80371000, 'ui_csd', 'csd*.cpp assert strings (Chao CSD UI library)'),
    (0x80371000, 0x8039B000, 'sdk_dolphin', '"<< Dolphin SDK - OS/VI/DSP/GX" version strings, DVD/OS messages'),
    (0x8039B000, 0x803A1258, 'sdk_metrotrk', '"MetroTRK for GAMECUBE v2.6"'),
    (0x803A1258, 0x803B5000, 'runtime_msl', 'extabindex entries, std::exception/bad_alloc, printf/strtod tables'),
    (0x803B5000, 0x803B8000, 'sdk_dolphin', '"<< Dolphin SDK - SI"'),
    (0x803B8000, 0x8040C000, 'middleware_cri', 'CRI ADX/ADXF/SJ/SFD/MPV/SFH/CFT version + error strings'),
    (0x8043B000, 0x80440000, 'audio_gcax', '"AX SDLIB", gcaxSndMemory.c'),
    (0x80472000, 0x804AAC60, 'renderware', 'rwRASTERFORMAT*/raster messages, RW file system'),
]

# namespace / class-name rules for game code (first match wins)
NAME_RULES = [
    (r'^Mission::|Mission', 'mission'),
    (r'^Player::Npc::BossSonic|^Boss|Diablon|BlackBull|BlackDoom|DevilDoom|HeavyDog|EggMeca|EggLastMech|BossCtrl', 'boss'),
    (r'^Player::Npc|Npc', 'npc'),
    (r'^Player::|PlayerShadow|Shadow(Behavior|Control|Motion|Executer)', 'player'),
    (r'EnemyAIState|Enemy|^Gun(Soldier|Beetle|Bigfoot|Robot)|^Bk[A-Z]|^BK|^Egg(Pawn|Pierrot|ShadowAndroid)|EnemyTemplate', 'enemy'),
    (r'^Weapon|Bullet|Lockon|TargetSearch|WeaponSpecial|Embedded(Gun|Bazooka)', 'weapon'),
    (r'^Vehicle|^dVehicle|AirSaucer|BirdBase|GunLift|Walker', 'vehicle'),
    (r'Camera|PJSCamera', 'camera'),
    (r'^Effect::|::Effect::|Effect|Particle|LensFlare|Shimmer', 'effects'),
    (r'CharaColli|^SonicteamUSA::System::Range|Colli', 'collision'),
    (r'Motion|Anim|Morph', 'animation'),
    (r'^StageAction|^StageSequence|^StageBase|^Stage|RouteDisplay', 'stage_flow'),
    (r'^Set|SetState|Gadget|Container|Ring|Spring|Switch|Door|Balloon|Elec|Fan|Cage|Weight|Stick|Bomb|Footing|Pole|Coaster|Catapult|Elevator|Circus|Candle|Lantern|Rocket|Pickup|Vacuum|Healing|Satellite|Ark|Defense|Escape|Fall|Break|Hint|Goal|Save|Warp|Tornado|Monster|Threat|Cream|Chao|Researcher', 'stage_object'),
    (r'^SaveCycle|SavePoint|Memory', 'save'),
    (r'^AdvReal|Title|Menu|Option|Tgi|^TGi|Csd|TCsd|Gindows|Hud|HUD|Pause', 'ui_hud'),
    (r'Audio|Sound|Se[A-Z]|Voice', 'audio'),
    (r'^Render|Light|Shadow|Texture|TTexture|BackDrop|ExpandBG', 'rendering'),
    (r'Resource|OneFile|^Land|TBGRead|FileControl|TOneFile', 'resource_fs'),
    (r'^Task$|TaskManager|Singleton|Heap|Thread', 'engine_core'),
    (r'^Vibration|Peripheral|Pad|Input', 'input'),
    (r'^boost::|^std::|basic_string|vector|tree', 'runtime_cpp'),
]
STR_RULES = [
    (r'^enemy/|Enemy', 'enemy'),
    (r'Mission', 'mission'),
    (r'\.BIN$', 'stage_object'),
    (r'^csdFiles|^CsdFiles|\.tp$', 'ui_hud'),
    (r'^sound/|\.adx|\.afs', 'audio'),
    (r'\.dff|\.txd|\.one|\.MTP', 'resource_fs'),
    (r'^stg%04d', 'stage_flow'),
]


def family_of_name(n):
    base = n
    if base.startswith('thunk'):
        base = base.split('_', 1)[1] if '_' in base else base
    for rx, fam in NAME_RULES:
        if re.search(rx, base):
            return fam
    return None


def main():
    d = get_dol()
    p = P.load()
    S = get_symbols()
    fam = {}
    ev = {}
    for s, e in p.funcs:
        for lo, hi, f, why in LIB_REGIONS:
            if lo <= s < hi:
                fam[s] = f
                ev[s] = ('PROVEN' if f != 'renderware' else 'STRONG', 'region: ' + why)
                break
    for s, e in p.funcs:
        if s in fam:
            continue
        n = S.name(s)
        if n:
            f = family_of_name(n)
            if f:
                fam[s] = f
                ev[s] = ('STRONG', 'RTTI/name: ' + n)
                continue
        recs = df().get(s, {})
        for pc, a in recs.get('cptr', []):
            if d.region(a) in ('.rodata', '.data', '.sdata', '.sdata2'):
                t = d.printable_str(a, 4)
                if not t:
                    continue
                for rx, f in STR_RULES:
                    if re.search(rx, t):
                        fam[s] = f
                        ev[s] = ('LIKELY', 'string: %r' % t[:40])
                        break
            if s in fam:
                break
    # neighbour fill: runs of unlabelled functions between same-family ends
    starts = [s for s, e in p.funcs]
    i = 0
    while i < len(starts):
        if starts[i] in fam:
            j = i + 1
            while j < len(starts) and starts[j] not in fam:
                j += 1
            if j < len(starts) and j - i - 1 <= 40 and fam[starts[i]] == fam[starts[j]] \
                    and not fam[starts[i]].startswith(('sdk', 'runtime', 'middleware', 'renderware', 'audio_gcax', 'ui_csd')):
                for k in range(i + 1, j):
                    fam[starts[k]] = fam[starts[i]]
                    ev[starts[k]] = ('LIKELY', 'between %08X and %08X (same TU run)' % (starts[i], starts[j]))
            i = j
        else:
            i += 1
    for s in starts:
        if s not in fam:
            fam[s] = 'unknown'
            ev[s] = ('UNKNOWN', '')
    out = {'%08X' % s: {'family': fam[s], 'confidence': ev[s][0], 'evidence': ev[s][1],
                        'name': S.label(s)} for s in starts}
    with open(os.path.join(DATA_DIR, 'func_families.json'), 'w') as fh:
        json.dump(out, fh, indent=0)
    # runs
    runs = []
    cur = None
    for s, e in p.funcs:
        f = fam[s]
        if cur and cur[2] == f:
            cur[1] = e
            cur[3] += 1
        else:
            if cur:
                runs.append(cur)
            cur = [s, e, f, 1]
    runs.append(cur)
    with open(os.path.join(DATA_DIR, 'family_runs.txt'), 'w') as fh:
        for s, e, f, n in runs:
            fh.write('%08X-%08X %-16s %d\n' % (s, e, f, n))
    c = Counter(fam.values())
    for f, n in c.most_common():
        print('%-16s %6d' % (f, n))


if __name__ == '__main__':
    main()

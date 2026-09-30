# COLLECTIBLES_AND_SCORING — score / karma table and how points reach the player

## 1. `common/ScoreData.bin` (PROVEN format, STRONG column meaning)

Loaded once per stage by `fn_80206868` (called from `StageManager_InitStage` 0x801783D4 at 0x80178584):
file → `fn_8040F66C` (generic Sega data container: header 0x20 bytes, byte +0x17 = 'B' → big-endian, else
byteswapped) → object at `sbss 0x805F0CD8`; `*(obj+0x14)` → table pointer cached at `sbss 0x805F0CDC`.

Data (+0x20): `u32 tableOffset, u32 count (115)`; entries 0x18 bytes: `u32 v[5]; u32 nameOffset`.
Parser: `tools/scoredata.py` → `data/score_table.csv` (all 115 rows with names).

Lookup/award: `fn_802067B0(ctx, id)` — id 0x27 ("Unknown") awards nothing; otherwise copies the 5 values and
calls `fn_80206740`, which allocates a command (`fn_801DA024`, size 0x28) and constructs
**`ScoreCommand`** (`CharCommand(type 0x1C)`, v0..v4 stored at +0x14..+0x24; vtable 0x8053E260), then sends
it through the target's command interface (`[[ctx+0]+0x38]` vslot 0x10). ~60 gameplay objects call
`fn_802067B0` (containers, bombs, balloons, enemies…).

Player handling (`fn_800A7CE4`, the player command dispatcher, `case 0x1C` at 0x800A7FEC):
```text
idx   = player->+0x54 (−1 → 0)
stats = fn_80178F10(fn_8007993C(), idx)
fn_8016F99C(stats+0x2C, 0, v1)     // score counter 0
fn_8016F99C(stats+0x2C, 1, v2)     // score counter 1
fn_8016F99C(stats+0x2C, 2, v0)     // score counter 2
g = fn_8007993C()
fn_8016F4B0(g+0x70C, 0, v4)        // gauge 0
fn_8016F4B0(g+0x70C, 1, v3)        // gauge 1
cmd->flags(+7) |= 2                // consumed
```
Column semantics (STRONG — consistent over all 115 names; strings "Normal Score"/"Hero Score"/"Dark
Score" at 0x804EE468..): **v0 dark score, v1 normal score, v2 hero score, v3 dark-gauge points,
v4 hero-gauge points**. Hence counter indices 0/1/2 = normal/hero/dark (STRONG) and gauge indices
**0 = hero, 1 = dark** (PROVEN power linkage: `Player::Behavior::ChaosControl::vf05` drains gauge 0 via
`KarmaGauge_Add(g+0x70C, 0, −n)` @0x8007BD28 and ends when it is empty; `Player::Behavior::ChaosBlast::vf05`
calls `KarmaGauge_Add(g+0x70C, 1, −10000)` @0x8007AF1C; Chaos Control is the hero power, Chaos Blast the dark one).

### Karma gauges (`fn_8007993C()+0x70C`, 2 × {s32 cur, s32 max, s32 lock})
`KarmaGauge_Add(g, idx, v)` 0x8016F4B0 (PROVEN): if v > 0 and **either** gauge is full (cur ≥ max) → ignored
(no gain while a Chaos power is ready); if `g[idx].lock > 0` → ignored; if v ≤ 0 and the global lock byte
0x8057E828 is set → ignored; else `cur += v` clamped (fn_8016F600, lower bound .sdata 0x805E78C8).
`fn_800AF7B8` (from `ShadowExecuter::vf01`) checks gauge 1 full first, then gauge 0 full, and spawns the
"power ready" object (fn_800A51F0 via fn_800A2FE8 / fn_800A2FC0) unless flags 0x10/0x22/0x23 are set.
`ScoreCounter_Add(c, idx, v)` 0x8016F99C: `c[idx] += v; c.stamp[idx] (+0xC+4·idx) = frame counter 0x8057E7F0`.
Other gauge writers: item boxes (`Player::GetItemCommand::vf01`, both gauges), bosses' attack callbacks.

| Entry (sample) | dark | normal | hero | dark gauge | hero gauge |
|---|---:|---:|---:|---:|---:|
| Ring | 0 | 10 | 0 | 0 | 0 |
| ItemBoxRing10 | 0 | 100 | 0 | 0 | 0 |
| GunSoldier | 150 | 0 | 0 | 3000 | 0 |
| GunSoldierShield | 200 | 0 | 0 | 3000 | 0 |
| GunRobotB | 500 | 0 | 0 | 5000 | 0 |
| BkSoldier | 0 | 0 | 150 | 0 | 2000 |
| BkGiantHanmer | 0 | 0 | 1000 | 0 | 5000 |
| EggPawn / EggPawnSpecific | 0 / 150 | 0 | 150 / 0 | 0 / 2000 | 2000 / 0 |
| EnergyCoreDark / Hero | 200 / 0 | 0 | 0 / 200 | 30000 / 0 | 0 / 30000 |
| MissionObjDark / Hero | 100 / 0 | 0 | 0 / 100 | 1000 / 0 | 0 / 1000 |

Destroying GUN units feeds the dark side, Black Arms units the hero side; Eggman units feed the hero side
except their "Specific" variants (stages where Eggman is the hero-side ally) — a data fact from the table.

## 2. Rings

SET 0x0010 RING (params `type`, `num[1..128]`, `param0`, `param1`, `ghost`), 0x0011 HINTRING, 0x0012 ITEM_BOX
(`item[0..10]`), `RegularRing::Ring` / `RegularRing::RingState`, `ScatteringRing` (0x80116E1C region),
`Weapon::Bullet::Ring`. Ring counting/loss on damage: see PLAYER_GAMEPLAY_RECOVERY.md (player trace).

## 3. Open items

- Label the counter/gauge indices from the HUD (`TGiScoreDisplay`, 0x80200960–0x80203750).
- `fn_8016F99C` / `fn_8016F4B0` internals (clamping, gauge max 30000?).
- Ring pickup and item-box handlers.

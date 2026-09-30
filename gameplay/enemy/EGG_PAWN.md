# Egg Pawn (`EggPawn`, SET id 0x0079 `EGG_PAWN`)

Evidence source: `sys/main.dol` (GC, PAL GUPP8P). Class names are original (RTTI); method names are **semantic
guesses** (proposed in `gameplay/notes/enemy2_symbols.csv`). Confidence: PROVEN / STRONG / LIKELY / UNKNOWN.
Framework and reference: `enemy/GUN_SOLDIER.md`; sibling walker with the soldier AI: `enemy/BK_SOLDIER.md`;
alert/activation analysis: `enemy/ENEMY_ALERT_PATHS.md`.

Notation: `E+x` complete object, `S+x` = `EnemyStatusCommon` (E+0x110), `AI+x` = AI subobject (**E+0x2F4**),
`SB+x` = `TEnemySetBase` (E+0x250). Component slot `N` = `u32(vptr + 8 + 4·N)`. "Primary appended slot" = a virtual
that `EggPawnBase` appended after the `TEnemySetBase` table, called as `[[E+0x18]+off]` → SB-table slot (off−0x208)/4.

---------------------------------------------------------------------------------------------------------------------

## 0. End-to-end chain

| # | Step | Address(es) | Conf |
|---|---|---|---|
| 1 | SET scan → `SetSlot_TrySpawn` → descriptor 0x8052D4C4 hook +0x0C | 0x800CA8D8 | PROVEN |
| 2 | `EggPawn_Create(slot)`: `operator new(0x3B0)`, layer 0xE, `EggPawn::EggPawn(this,1,layer,slot)` | 0x8017BD90 | PROVEN |
| 3 | Ctor chain `EggPawn` 0x8017BFAC → `EnemyTemplate<EggPawnBase,EggPawnAI>` 0x8017C070 → `EggPawnBase` 0x8017D9D4 (**`li r5,0xC`** @0x8017DA20 → `EnemyWalker::EnemyWalker` → … → type 12; writes the catalog defaults into the param block E+0x274) → `EggPawnAI` (E+0x2F4) → `TEnemySetTask` (E+0x378, slot E+0x3A4) | as listed | PROVEN |
| 4 | No post-init virtual in the ctor (no record flag 0x8 at spawn) | 0x8017BFAC..0x8017C070 | PROVEN |
| 5 | Lifecycle 1: resource slot 12+7 = **0x13** ("enemy/EggPawnData.one", hook +0x04 0x8017BE10) | 0x8017BE54 | PROVEN |
| 6 | Lifecycle 2: SB slot 6 → `EggPawnBase::InitFromSet` (vf16) 0x8017D7C0; Disp slot 0 → `EggPawnBase::InitDisplay` (vf23) 0x8017D09C; Status slot 0 = `EnemyStatusCommon::InitHP` (HP = MaxHP = 4.0); AI slot 0xC → **`EggPawnAI::EnterInitialState` 0x8017ECD0** | as listed | PROVEN |
| 7 | Lifecycle 3: Status slot 0xC → `EggPawnBase::UpdateActivation` (vf18) 0x8017C5C4: AppearType 0 active at once; OFFSET/WARP/DASH hidden + inactive until the player enters the search area (WARP: +1 s, warp-in effect Disp slot 0x12) | 0x8017C5D8..0x8017C750 | PROVEN |
| 8 | Lifecycle 4: Status slot 2 = `EnemyStatusCommon::Update` → AI `EnemyBaseAI::Update` → Move slot 1 = `EnemyMoveCommon::Update` → Disp slot 2 `EggPawnBase::vf0B` 0x8017CE84 (weapon placement, loop SE 0x601A) | 0x801A74B4.. | PROVEN |
| 9 | Detection: `EnemyWalkerAI_WaitAction_{Standing,Moving}` start every Update with `Enemy_DetectNearestPlayer` 0x8019B2C0 (sensor test on the nearest player; stores the player ref at E+0x40) → AI slot 0xF = `EnemyWalkerAI::OnTargetFound` 0x8019A85C → `EnemyWalkerAI_Caution` → after 1 s AI slot 0x10 = **`EggPawnAI::StartAttack` 0x8017E8D4** | 0x80199EAC, 0x8019A31C | PROVEN |
| 10 | Damage: `EnemyCharaColliModule::OnDamaged` → Status slot 0x1A = `EggPawnBase::TakeHit` (vf19) 0x8017C168 → `EnemyStatusCommon::ApplyHit`; score 0x63/0x64 (or 0x69/0x6A) | 0x801A9750 → 0x8017C168 | PROVEN |
| 11 | HP 0 edge: `ReportLifeEvent(1)` (team 1) → Status slot 0x26 = `EggPawnBase::OnDeath` (vf15) 0x8017D4BC → type 12 → `TEnemySetBase::DetachKilled` → collision off | 0x801A8C40..0x801A8CA0 | PROVEN |
| 12 | AI: `EnemyBaseAI::OnDead` 0x8019D070 → `EnemyAIState_EnemyBaseAI_Dead` → death motion (front 0x13 / back 0x14 via `EggPawnBase::vf21`) → ≤0.5 s → `EnemyDispCommon::PlayDeathEffect` (team 1: SE 0x6007 + effect 0x72) → `RequestDelete` | 0x8019BB58 | PROVEN |
| 13 | Delete on the next `TEnemySetTask::Update` → `~EggPawn` 0x8017BE70 → `~TEnemySetTask` (no detach, already detached) → `~EggPawnAI` → `~EggPawnBase` 0x8017D8E0 (shield, weapon) | 0x801A6F04 | PROVEN |
| 14 | Killed record: flags \|= 0x8, &= ~0x7, unlinked → never re-armed by the SET scanner | 0x800C9EEC, 0x800CAC8C.. | PROVEN |

---------------------------------------------------------------------------------------------------------------------

## 1. Differences vs GunSoldier (summary)

| Topic | GunSoldier | EggPawn | Conf |
|---|---|---|---|
| Base classes | `SoldierCommonBase`/`SoldierCommonAI` | **`EnemyWalker`/`EnemyWalkerAI`** (different wait states, Caution with a 1 s timer, attack selection through AI slot 0x10) | PROVEN |
| HP / team | 2.0 / GUN | **4.0 / 1 Eggman** | PROVEN |
| HP 0 | injured (Hinshi), no SET kill | **real death**: DetachKilled + Dead state + explosion + delete | PROVEN |
| SET params | 23, clamped into E+0x2E8 | **27**, copied **unclamped** into E+0x274 (`SetSlot_GetParams(slot,E+0x274,0x6C)`) | PROVEN |
| AppearType | STAND/LINEAR/TRIANGLE/RANDOM/OFFSETPOS/HINSHI | **WAIT_ACT(0)/OFFSET(1)/WARP(2)/DASH(3)**; patrol layout moved to a separate `WaitActMoveType` (STAND/LINEAR/TRIANGLE/RANDOM) | PROVEN (labels STRONG) |
| Weapons | 0‑6 | **NONE(0), AUTORIFLE(1), BAZOOKA(2), LANCE(3)**; attack chosen per weapon: 0 → DashAttack, 3 → TukiAttack (lance thrust), 1/2 → WeaponAttack | PROVEN |
| Per-point data | WaitType/WaitSec/MoveSpeedRatio | **ActionType (NONE/ATTACK/HIDE) / ActionTime / MoveSpeedRatio** | PROVEN (labels STRONG) |
| Lost target | GoHome state | Caution / attacks → AI slot 0xD (**ReturnToWait**, straight back to the wait state; no GoHome) | PROVEN |
| Shield SE | 0x6074 / 0x6075 | same 0x6074 / 0x6075 | PROVEN |
| Score | 0x4F/0x50 | **0x63/0x64 EggPawn(Shield)**, or **0x69/0x6A EggPawn(Shield)Specific** in stages 300 HorrorCastle / 402 canyon2 / 403 eWorld | PROVEN |
| Knockback table | {50, 1.3, 1.0} | {40, 1.2, 1.0} (0x804CD6B4[12]) | PROVEN |

---------------------------------------------------------------------------------------------------------------------

## 2. Identity

| Item | Value | Evidence | Conf |
|---|---|---|---|
| SET id / descriptor | 0x0079 `EGG_PAWN`, desc 0x8052D4C4, 27 params | catalog | PROVEN |
| Hooks | +0x04 `EggPawn_LoadResources` 0x8017BE10; +0x08 `EggPawn_ReleaseResources` 0x8017BDE8 (slot 0x13); +0x0C `EggPawn_Create` 0x8017BD90 | disassembly | PROVEN |
| sizeof | 0x3B0 | `li r3,0x3B0` 0x8017BDA4 | PROVEN |
| RTTI | `EggPawn` ← `EnemyTemplate<EggPawnBase, EggPawnAI>` ← `EggPawnBase` ← `EnemyWalker` ← `EnemyWalkerCommon` ← `TEnemyCommonBase` ← `EnemyRefBase` ← `Enemy`; AI `EggPawnAI` ← `EnemyWalkerAI` ← `EnemyBaseAI`; move `EnemyMoveWalker` | `q.py class ^EggPawn$` | PROVEN |
| Type / team / MaxHP | 12 / 1 / 4.0 | `li r5,0xC` 0x8017DA20; tables 0x804CD310, 0x804CD678 | PROVEN |
| Vtable group | V = 0x8052DAC0; E+0x18=V, 0x104=V+0x14, 0x110=V+0x8C, 0x1B4=V+0x160, 0x1E8=V+0x1C8, 0x250=V+0x200, **0x30C=V+0x2A4** (AI), **0x390=V+0x308** (Task) | ctor 0x8017C00C..0x8017C048 | PROVEN |
| Editor labels | `EggPawn_GetSetParamLabel` 0x8017BCA8 (idx 8 → table 0x8052D048 "Weapon : NONE/AUTORIFLE/BAZOOKA/LANCE"; idx 9 → 0x8052D038 "AppearType : WAIT_ACT/OFFSET/WARP/DASH" (remembered in .sbss 0x805F04BC); idx 10‑12 OffsetPos shown only when AppearType ≠ 0; other indices → `EggPawn_GetSetParamLabel2` 0x8017DBC4 with 0x8052D058 "ActionType : NONE/ATTACK/HIDE" and 0x8052E3E0 "WaitActMoveType : STAND/LINEAR/TRIANGLE/RANDOM") | string tables | PROVEN (strings) / STRONG (use) |

---------------------------------------------------------------------------------------------------------------------

## 3. Object layout (fields that differ from GunSoldier; E+0x000..0x24F common framework)

| Offset | Field | Evidence | Conf |
|---|---|---|---|
| 0x268 | loop-sound emitter (SE 0x601A while E+0x2F1) | 0x8017CF74 | LIKELY |
| 0x270 | u8 HaveShield (runtime; cleared on shield break) | 0x8017D854, 0x8017C2A8 | PROVEN |
| **0x274..0x2DF** | the 27 SET params (idx *i* at 0x274 + 4*i*): 0x290 HaveShield, **0x294 WeaponType**, **0x298 AppearType**, 0x29C/0x2A0/0x2A4 OffsetPos X/Z/Y, **0x2A8 WaitActMoveType**, 0x2AC..0x2DC points | `InitFromSet` 0x8017D7EC | PROVEN |
| 0x2E0 | `EnemyWeapon*` | 0x8017D170 | PROVEN |
| 0x2E4 | shield handle | TakeHit 0x8017C188 | PROVEN |
| 0x2EC | f32 appear delay (1.0 for WARP) | 0x8017D868, 0x8017C678 | PROVEN |
| 0x2F0 / 0x2F1 | u8 warp-in effect started / u8 loop-sound flag | 0x8017C6A8, 0x8017CF08 | PROVEN (2nd meaning LIKELY) |
| 0x2F4 | AI (0x84): AI+0x74 `EnemyWalker*` (EnemyWalkerAI), AI+0x7C `EggPawnBase*` | 0x8019A6B8, 0x8017E8E8 | PROVEN |
| 0x378 | `TEnemySetTask` (vptr E+0x390, vbptr E+0x3A0, **slot E+0x3A4**) | ctor | PROVEN |
| 0x3A8 | `EnemyReferer` | ctor | PROVEN |

---------------------------------------------------------------------------------------------------------------------

## 4. SET parameters → consumers

| idx | Param | Consumer / effect | Conf |
|---|---|---|---|
| 0‑5 | MoveRange, Search* | raw record → sensor E+0xA8 (shared); MoveRange also bounds `Enemy_PickRandomPointNear` 0x8019AE98 | PROVEN |
| 6 | MoveSpeedRatio | no reader found | UNKNOWN |
| 7 | HaveShield | E+0x270; shield model in `InitDisplay`; score 0x64/0x6A vs 0x63/0x69; motion variants | PROVEN |
| 8 | WeaponType | E+0x294: `EnemyWeapon` kind table 0x804C7374 = {0, 7, 0x11, 0x21}; `StartAttack`: 0 → DashAttack, 3 LANCE → TukiAttack, 1/2 → WeaponAttack; motion getters vf1A/vf1B/vf1E | PROVEN |
| 9 | AppearType | E+0x298: ≠0 → spawn position += OffsetPos (X, Y, Z) and hidden until detected; 1 OFFSET / 2 WARP → initial state `EnemyWalkerAI_ReturnHome` (walk to the SET position once revealed); 3 DASH → `ReturnHomeDash` (dash motion 4, speed ×2); 2 WARP → +1 s delay and warp-in effect | PROVEN |
| 10‑12 | OffsetPos_X/Z/Y | spawn offset (applied only when AppearType ≠ 0) | PROVEN |
| 13 | WaitActMoveType | `EggPawnBase::GetPatrolMode` (vf12) 0x8017C780: 0 → `WaitAction_Standing`, else `WaitAction_Moving` (patrol layout 1 LINEAR / 2 TRIANGLE / 3 RANDOM) | PROVEN / STRONG (layout) |
| 14‑26 | Pos0..2 ActionTime/ActionType/MoveSpeedRatio/X/Z | accessors `GetPointActionTime` (vf0E), `GetPointActionType` (vf0D), `GetPointMoveSpeedRatio` (vf0C), `GetPatrolPoint` (vf0F: 0 = SET pos, 1/2 = SET pos + (X,Z)); per-point wait motion `vf11`: ActionType 1 ATTACK → attack motion, else wait motion. **No caller of `GetPointMoveSpeedRatio` found** (primary slot 0x238 is only called by soldier/flyer code) | PROVEN (accessors) / UNKNOWN (per-point speed) |

Census (`agent_enemy2_census.py 79`): 498 records; WeaponType {1:111, 2:36, 3:351}; AppearType {0:259, 1:20, 2:215,
3:4}; WaitActMoveType {0:409, 1:19, 3:70}; **SearchWidth = 0 on 3** (stg0602 hrd, AppearType 0 → never alert).

---------------------------------------------------------------------------------------------------------------------

## 5. AI state machine (EggPawnAI vtable V+0x2A4 = 0x8052DD64)

AI slots: **0x0C `EggPawnAI::EnterInitialState`** (AppearType 0 → by WaitActMoveType Standing/Moving; 1,2 →
ReturnHome; 3 → ReturnHomeDash; other → Standing); **0x0D `EggPawnAI::ReturnToWait`** 0x8017EB58 (WaitActMoveType 0 →
Standing else Moving); 0x0F `EnemyWalkerAI::OnTargetFound` → Caution; **0x10 `EggPawnAI::StartAttack`**; 0x14 base
Dead; 0x16 base Recover. PROVEN.

| State (RTTI) | Getter / singleton | Enter | Update | Exit | Behaviour / transitions | Conf |
|---|---|---|---|---|---|---|
| `EnemyAIState_EnemyWalkerAI_WaitAction_Standing` | inline / 0x805F049C | 0x8019A60C | 0x8019A300 | 0x8019A2FC | detect → AI 0x0F; otherwise return home / face SET yaw, per-point action & timer | PROVEN / STRONG |
| `…_WaitAction_Moving` | inline / 0x805F04A8 | 0x8019A218 | 0x80199E90 | 0x80199E8C | detect → AI 0x0F; patrol points (ActionTime wait, ActionType motion) | PROVEN / STRONG |
| `…_Caution` | 0x8019A894 / 0x805F04B4 | 0x80199E0C (motion param 0x13, timer 1 s) | 0x80199D28 | nop | face target; **player left area → AI 0x0D**; timer < 0 → **AI 0x10 StartAttack** | PROVEN |
| `…_ReturnHome` | inline / 0x805F0484 | 0x8019A7D8 (walk motion, home := SET pos) | 0x8019A77C | nop | `Enemy::MoveTowardHome` arrived (XZ < 10) → AI 0x0D | PROVEN |
| `…_ReturnHomeDash` | inline / 0x805F0490 | 0x8019A6FC (motion `EggPawnBase::vf13` = 4; `GetMoveSpeed` doubles speed while motion 4) | 0x8019A6A0 | nop | arrived → AI 0x0D | PROVEN |
| `EnemyAIState_EggPawnAI_WeaponAttack` | 0x8017E954 / 0x805F04EC | 0x8017E1F8 (attack motion vf1E, step 0) | 0x8017DF84 | nop | step0 motion end → aim motion, move target := random point 15‑30 u away (in MoveRange), timer 2 s; step1: **player left area → AI 0x0D**; walk; arrived/timer → face target; step2 turned → attack again | PROVEN |
| `…_TukiAttack` (lance) | 0x8017EA00 / 0x805F04E0 | 0x8017E508 (charge motion 0xC/0xD by shield, target point = AI vf05, timer 3 s) | 0x8017E270 | nop | charge; target distance < 15 → thrust motion 0xE; motion end → pause 1 s; not in area → AI 0x0D; else charge again | PROVEN |
| `…_DashAttack` (unarmed) | 0x8017EAAC / 0x805F04D4 | 0x8017E7F8 (as Tuki) | 0x8017E5E8 | nop | charge to the point (≤3 s) → pause 1 s → not in area → AI 0x0D, else charge again | PROVEN |
| `EnemyBaseAI_Dead` / `_Recover` / `_Guard` / `_WaitFinishDamaged` / … | base (see BK_SOLDIER.md §5) | | | | death motion via param slot 0x16 = `EggPawnBase::vf21`: hit from front → 0x13, else 0x14 | PROVEN |

`EnemyBaseAI::vf05` 0x8019DA1C (target point for charges) uses the nearest player and a camera projection (NDC ±1
tests via `fn_8000F57C`); exact meaning LIKELY "attack point on/near the player within view".

---------------------------------------------------------------------------------------------------------------------

## 6. Damage and HP

* `EggPawnBase::TakeHit` 0x8017C168: same shield algorithm as GunSoldier (frontal block with attack flag 0x40 → Guard,
  SE 0x6074; absorb unless flag 0x4, break → SE 0x6075 + flinch + HaveShield=0), then **always** `ApplyHit`. No hit-side
  copy, no immunity. PROVEN.
* HP 4.0, flinch at 2.0, knockback {40, 1.2}. PROVEN.
* Fall rule (M+0x5F while active → HP 0) as for all enemies (BK_SOLDIER.md §6). PROVEN.

## 7. Death (normal path) — PROVEN
1. Kill score: `GetScoreId` (vf14, Status slot 0x2C) 0x8017D880 → `Enemy_IsEggmanSpecificStage` 0x801A23EC (stage id
   [0x8057E808] ∈ {300, 402, 403}) ? 0x6A/0x69 : 0x64/0x63 (shield/no shield). ScoreData: 0x63/0x64 w2 150/200 w4
   2000; 0x69/0x6A w0 150/200 w3 2000 (column meaning LIKELY hero/dark, see GUN_SOLDIER.md §9).
2. Death edge → `ReportLifeEvent(1|4)` (team 1 counter), `EggPawnBase::OnDeath` 0x8017D4BC (collision 0x100000,
   release shield, throw weapon (0,20,10) → "ScatteringWeapon" pickup, E+0x2E0 = 0, param WeaponType E+0x294 = 0; no SE),
   `DetachKilled`, collision off.
3. Dead state → effect 0x72 + SE 0x6007 → `RequestDelete` → deleted next frame; record never respawns (flag 0x8, no
   re-arm bit).

Heal (`RecoverCommand`): living pawn → `EnemyBaseAI_Recover` (60 s passive, status slot 0x1D(0,1) = collision off LIKELY); first heal of the record
awards 0x68 "EggRecovery" (w0 200, w3 3000) or 0x6E "EggRecoverySpecific" (w2 200, w4 3000) in stages 300/402/403.

## 8. Open items
1. `EnemyBaseAI::vf05` exact geometry (target point) — LIKELY only.
2. Per-point MoveSpeedRatio and MoveSpeedRatio (idx 6) consumers — UNKNOWN.
3. `SB slot 5` = `EggPawnBase::ReinitFromSet` 0x8017C9A4 — no caller found (LIKELY editor/debug).
4. EnemyWeapon kind ids 7/0x11/0x21 → weapon classes — UNKNOWN.

## 9. Native decision implementation (2026-09-30)

`src/EggPawnAI.cpp` reconstructs the **PROVEN** branches in `EggPawnAI::EnterInitialState`
0x8017ECD0, `ReturnToWait` 0x8017EB58 and `StartAttack` 0x8017E8D4. AppearType 0 uses
WaitActMoveType (0 standing, nonzero moving); 1/2 enter ReturnHome; 3 enters ReturnHomeDash;
out-of-range values enter Standing. WeaponType 0 selects DashAttack, 3 selects TukiAttack,
all other values WeaponAttack. The inherited `EnemyWalkerAI::OnTargetFound` 0x8019A85C enters
Caution. Its 1 s timer expires only when **strictly below zero**, after the base AI subtracts
the frame dt at 0x8019EB70..0x8019EB7C. Target leaving the search area returns immediately
to the WaitActMoveType state (0x80199DA0..0x80199DF4). ReturnHome and ReturnHomeDash use the
same `Enemy::MoveTowardHome` arrival result at 0x8019A77C/0x8019A6A0. **PROVEN.**

WeaponAttack substeps 0/1/2 (0x8017DF84..0x8017E1C0), TukiAttack substeps 0/1/2
(0x8017E270..0x8017E4FC), and DashAttack substeps 0/1 (0x8017E5E8..0x8017E7F4)
are native. Their timer values 2/3/1 s, strict expiry tests, lance's `<15` target-distance
thrust trigger, motion-end gates and search-area exits are **PROVEN**. The exact EggPawnBase
motion selectors are also native: attack `vf1E` 0x8017CB50 gives 0xA/0xB for a rifle or
bazooka without/with shield, aim `vf1B` 0x8017CBD8 gives 1/3, and pause `vf1A`
0x8017CC20 gives 7/9; other weapons use their directly observed default branches. The
charging motion is 0xC without shield or 0xD with shield; lance thrust is 0xE.

Engine services still behind **explicit `EggPawnBody` hooks**: player acquisition and tracked
target, display motion timing, movement/arrival through `Enemy::MoveTowardHome`
0x8019B0C8 (which turns and moves before returning its arrival result), the attack-point projection of
`EnemyBaseAI::GetAttackPoint` 0x8019DA1C, terrain-aware random-point selection
0x8019AE98, and the remaining shared walker standing/patrol steps 0x8019A300/
0x80199E90. Their exact outcomes are **UNKNOWN** to this module. `src/tests/test_eggpawn_ai.cpp`
checks all AppearType and WeaponType branches, caution and attack timer boundaries,
search-area exits, shield motion variants and each attack substep; clang compilation with
`-Wall -Wextra -Werror` and the test binary passed. The coordinator integrated
SET 0x0079 into `stage_sim` with labelled harness hooks; the full Release gate passed
21/21 CTest suites. This does not establish parity for activation, movement or weapons.

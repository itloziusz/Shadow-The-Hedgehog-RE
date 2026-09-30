# Black Arms Soldier (`BkSoldier`, SET id 0x008D `BK_SOLDIER`)

Evidence source: `sys/main.dol` (GC, PAL GUPP8P). Class names are original (RTTI); method names are **semantic
guesses** (proposed in `gameplay/notes/enemy2_symbols.csv`). Confidence: PROVEN / STRONG / LIKELY / UNKNOWN.
Reference enemy and shared framework: `enemy/GUN_SOLDIER.md` (read that first — this document only restates what is
needed and concentrates on **differences**). Alert/activation paths common to all enemies: `enemy/ENEMY_ALERT_PATHS.md`.
Helper scripts: `tools/agent_enemy2_{states,brief,vt,strtab,census,cmdsend,findptr,dump}.py`.

Notation as in GUN_SOLDIER.md: `E+x` complete object, `S+x` = `EnemyStatusCommon` (E+0x110), `AI+x` = AI subobject
(**E+0x37C** here), `SB+x` = `TEnemySetBase` (E+0x250). Component slot `N` = `u32(vptr + 8 + 4·N)`.

---------------------------------------------------------------------------------------------------------------------

## 0. End-to-end chain

| # | Step | Address(es) | Conf |
|---|---|---|---|
| 1 | SET scan → `SetSlot_TrySpawn` → descriptor 0x80531E70 hook +0x0C | 0x800CA8D8 | PROVEN |
| 2 | `BkSoldier_Create(slot)`: `operator new(0x438)`, layer `TaskManager_GetLayer(mgr,0xE)`, `BkSoldier::BkSoldier(this,1,layer,slot)` | 0x8018CAD0 | PROVEN |
| 3 | Ctor chain `BkSoldier` 0x8018CCEC → `EnemyTemplate<BkSoldierBase,BkSoldierAI>` 0x8018CDB0 → `BkSoldierBase` 0x8018EF5C (**`li r5,5`** @0x8018EFA8 → `SoldierCommonBase` → … → `Enemy::Enemy` stores type 5) → `BkSoldierAI` 0x80180A4C (AI at E+0x37C) → `TEnemySetTask` (E+0x400, slot E+0x42C, `SetSlot_Attach` flag 0x2) | as listed | PROVEN |
| 4 | **No post-init virtual** (GunSoldier calls `[[E+0x18]+0x2A0]` → `SetSlot_SetFlag8`; BkSoldier's ctor has no indirect call) → record flag 0x8 is *not* set at spawn | 0x8018CCEC..0x8018CDAC | PROVEN |
| 5 | Lifecycle 1: resource slot E+0x24 = 5+7 = **0xC** (`enemy/BkSoldierData.one`, loaded by hook +0x04 0x8018CB50) | 0x801A72D4 | PROVEN |
| 6 | Lifecycle 2: SB slot 6 → `BkSoldierBase::InitFromSet` (vf21) 0x8018E618; Disp slot 0 → `BkSoldierBase::InitDisplay` (vf14) 0x8018E0A4; **Status slot 0 = `EnemyStatusCommon::InitHP` 0x801A9560 directly** (no override → HP = MaxHP = 4.0); AI slot 0 `EnemyBaseAI::Init` → AI slot 0xC = `EnemyBaseAI::vf0C` 0x8019D894 → AI slot 0xD = `BkSoldierAI::ReturnToWait` 0x80180770 | as listed | PROVEN |
| 7 | Lifecycle 3: Status slot 0xC → `BkSoldierBase::UpdateActivation` (vf0F) 0x8018E478 (OFFSETPOS/WARP stay hidden until detection; WARP adds a 1 s warp-in); Status slot 1 = `EnemyStatusCommon::SetupCollision` directly | as listed | PROVEN |
| 8 | Lifecycle 4 per frame: Status slot 2 = **`EnemyStatusCommon::Update` 0x801A8A38 directly** (no temp-state write) → AI `EnemyBaseAI::Update` → Disp → **Move slot 1 = `BkSoldierBase::UpdateMove` (vf1F) 0x8018D204** (drives an AirSaucer when riding, else `EnemyMoveCommon::Update`) → Disp slot 2 `BkSoldierBase::vf0B` 0x8018DE04 (weapon placement, voices) → Status slot 5 | 0x801A74B4.. | PROVEN |
| 9 | Detection: shared `SoldierCommonAI_WaitAction_{Standing,Moving}` → `EnemySensor_IsInSearchArea` → AI slot 0xF = `BkSoldierAI::OnTargetFound` 0x80180738 → `BkSoldierAI_Caution` | 0x801895C0, 0x80188BDC | PROVEN |
| 10 | Damage: CharaColli → `EnemyCharaColliModule::OnDamaged` → Status slot 0x1A = `BkSoldierBase::TakeHit` (vf10) 0x8018D81C (shield) → `EnemyStatusCommon::ApplyHit`; kill score 0x58/0x59 | 0x801A9750 → 0x8018D81C → 0x801A83B0 | PROVEN |
| 11 | HP 0 edge (`EnemyStatusCommon::Update`): `ReportLifeEvent(1)` (team 2) → Status slot 0x26 = `BkSoldierBase::OnDeath` (vf11) 0x8018D3A0 (SE 0x6016, drop shield, throw weapon) → **type 5 ≠ 0 → `TEnemySetBase::DetachKilled` (SB slot 3)** @0x801A8C90 → `EnemyStatus_SetCollisionEnabled(S,0,0)` | 0x801A8C40, 0x801A8C54, 0x801A8C64/0x801A8C90, 0x801A8CA0 | PROVEN |
| 12 | Same frame: `EnemyBaseAI::PreUpdate` death edge → AI slot 0x14 = **`EnemyBaseAI::OnDead` 0x8019D070 (not overridden)** → `EnemyAIState_EnemyBaseAI_Dead` | 0x8019EBE0, 0x8019D084 | PROVEN |
| 13 | Dead state: death motion (front/back), after 0.5 s or motion flag A → `EnemyDispCommon::PlayDeathEffect` (team 2: effect 0x78 + SE 0x6009) → SB slot 2 `RequestDelete` | Enter 0x8019BC30, Update 0x8019BB58 (0x8019BBEC, 0x8019BC0C) | PROVEN |
| 14 | Next `TEnemySetTask::Update`: SB+0x14 → Task kill flag → `~BkSoldier` 0x8018CBB0 → `~TEnemySetTask` (skips `SetSlot_Detach`, SB+0x15 already set) → `~BkSoldierAI` → `~BkSoldierBase` 0x8018EA0C (release AirSaucer, shield, weapon) | 0x801A6F04, 0x801A7034 | PROVEN |
| 15 | Record after a kill: flags \|= 0x8, &= ~0x7, unlinked from the link list → the scanner never re-arms it (no 0x40/0x10000/0x800000) → **no respawn** unless something sets 0x40 (e.g. a `SET GENERATOR`) | 0x800C9EEC, 0x800CAC8C..0x800CAD6C | PROVEN (scanner) / LIKELY (generator case) |

---------------------------------------------------------------------------------------------------------------------

## 1. Differences vs GunSoldier (summary)

| Topic | GunSoldier (type 0) | BkSoldier (type 5) | Conf |
|---|---|---|---|
| HP / team | 2.0 / 0 GUN | **4.0 / 2 Black Arms** (tables 0x804CD678[5], 0x804CD310[5]) | PROVEN |
| HP 0 | converted to injured "Hinshi", never deleted by damage, no SET kill | **really dies**: `DetachKilled` (flag 0x8), Dead state, explosion, deleted ≈0.5 s later | PROVEN |
| AI slot 0x0C/0x14/0x16 | overridden (initial/ToHinshi/Thanks) | base: 0x0C → slot 0x0D; 0x14 → `EnemyBaseAI_Dead`; 0x16 → `EnemyBaseAI_Recover` | PROVEN |
| AppearType 5 | HINSHI (spawn injured) | **WARP** (hidden until detected, 1 s delay, warp effect + SE 0x6012) | PROVEN (labels STRONG) |
| WeaponType | 0‑6 (NONE…MISSILE) | **0‑8** (NONE, BLACK_SWORD, LIGHT_SHOT, FLASH_SHOT, BLACK_BARREL, SPLITTER, VACUUM_POD, HEAVY_SHOT, RING_SHOT); 6 → new state `AttackVacuum` | PROVEN (labels STRONG) |
| WaitType labels | 0 RADIO_CONTACT, 1 ATTACK, 2 HIDE, 3 KAMAE | **0 ATTACK, 1 HIDE, 2 KAMAE** (max 2) | STRONG |
| Extra param | — | idx 23 `RideAirSaucer` → spawns and drives a `Vehicle::AirSaucer` | PROVEN |
| STemporaryData / rescue / Thanks | yes | **none** (no temp data, no Hinshi/Thanks states) | PROVEN |
| Record flag 0x8 at construction | set (`GunSoldier_MarkSlotFlag8`) | **not set** | PROVEN |
| Lost target from Chase/NearAttack | `GunSoldierAI_GoHome` (re-detects) | **`EnemyAIState_EnemyBaseAI_GoHome`** (walks home, no re-detection) | PROVEN |
| Chase → melee distance | 15 / back to chase > 20 | **30 / > 30** | PROVEN |
| Shield SE | 0x6074 guard / 0x6075 break | **0x6076 / 0x6077** | PROVEN |
| Injured immunity | AppearType 5 ignores hits | no such check | PROVEN |
| Score | 0x4F/0x50 GunSoldier(Shield) (w0 150/200, w3 3000) | **0x58/0x59 BkSoldier(Shield) (w2 150/200, w4 2000)** | PROVEN |
| Heal (RecoverCommand) | revive from Hinshi → Thanks, 0x57 GunRecovery | on a *living* soldier: `EnemyBaseAI_Recover` (passive 60 s, S bit 0xD, status slot 0x1D(0,1) → `fn_801A9998(S+8,…)` = collision off LIKELY), first heal awards 0x62 BkRecovery | PROVEN |
| Knockback table | {50, 1.3, 1.0} | same {50, 1.3, 1.0} | PROVEN |

---------------------------------------------------------------------------------------------------------------------

## 2. Identity

| Item | Value | Evidence | Conf |
|---|---|---|---|
| SET id / descriptor | 0x008D `BK_SOLDIER`, desc 0x80531E70, 24 params | `data/setobj_catalog.txt` | PROVEN |
| Hooks | +0x04 `BkSoldier_LoadResources` 0x8018CB50 ("enemy/BkSoldierData.one", slot 0xC); +0x08 `BkSoldier_ReleaseResources` 0x8018CB28; +0x0C `BkSoldier_Create` 0x8018CAD0 | disassembly | PROVEN |
| sizeof | 0x438 | `li r3,0x438` 0x8018CAE4 | PROVEN |
| RTTI | `BkSoldier` ← `EnemyTemplate<BkSoldierBase, BkSoldierAI>` ← `BkSoldierBase` ← `SoldierCommonBase` ← `EnemyWalkerCommon` ← `TEnemyCommonBase` ← `EnemyRefBase` ← `Enemy`; AI `BkSoldierAI` ← `SoldierCommonAI` ← `EnemyBaseAI` | `q.py class ^BkSoldier$` | PROVEN |
| Type / team / MaxHP | 5 / 2 / 4.0 | `li r5,5` 0x8018EFA8; tables | PROVEN |
| Vtable group | V = 0x8053246C; E+0x18=V, 0x104=V+0x14, 0x110=V+0x8C, 0x1B4=V+0x160, 0x1E8=V+0x1C8, 0x250=V+0x200, **0x394=V+0x294** (AI), **0x418=V+0x2F8** (Task) | ctor 0x8018CD4C..0x8018CD88 | PROVEN |
| Editor labels | label accessor `BkSoldier_GetSetParamLabel` 0x8018CEA8 (jump table 0x80532778 on param index 8..22): WeaponType table 0x80531A40, AppearType 0x80531A64 (STAND, LINEAR_MOVE, TRIANGLE_MOVE, RANDOM_MOVE, OFFSETPOS, **WARP**), WaitType 0x80531A7C (ATTACK, HIDE, KAMAE), prefixes "Rand_/Pos0_/Stnd_", "RandomMoveRange", "Pos1_Y" | string tables | PROVEN (strings) / STRONG (meaning) |

---------------------------------------------------------------------------------------------------------------------

## 3. Object layout — only fields that differ from GunSoldier (§2 of GUN_SOLDIER.md holds for E+0x000..0x2E7)

| Offset | Field | Evidence | Conf |
|---|---|---|---|
| 0x270 / 0x274 / 0x278 / 0x27C..0x2E4 | HaveShield / AppearType / WeaponType / patrol data — written by the shared `SoldierCommon_ReadSetParams` 0x80198D90 | call 0x8018E8DC | PROVEN |
| 0x2E8 | 0x60-byte copy of the 24 SET params (`SetSlot_GetParams(slot,E+0x2E8,0x60)`), ints clamped | 0x8018E644 | PROVEN |
| **0x344** | RideAirSaucer (param 23, not clamped) | 0x8018D224, 0x8018DC08, 0x8018E1B0 | PROVEN |
| 0x348 | attack animation variant (frame counter & 1) | 0x8017FB3C, 0x8018003C | PROVEN |
| 0x34C | shield handle (GunSoldier: 0x348) | TakeHit/InitDisplay/OnDeath | PROVEN |
| 0x354 | `EnemyWeapon*` (0x68 bytes; GunSoldier 0x350) | 0x8018E178 | PROVEN |
| 0x358 | u8 last hit came from front (GunSoldier 0x357); selects death/flinch motion | 0x8018D9D4, `BkSoldierBase::vf1C/vf1D` | PROVEN (meaning STRONG) |
| 0x359 | u8 attack-active flag (set in Attack/AttackVacuum fire phase) | 0x8017F704, 0x8017FBEC | PROVEN (meaning LIKELY) |
| 0x35C | f32 voice timer, rand(8,12) s; while the player is in the search area plays SE 0x6013/0x6014 alternately | 0x8018E018..0x8018E084 | PROVEN (voice meaning LIKELY) |
| 0x360 | f32 appear delay (1.0 for WARP, else 0) | 0x8018E94C, 0x8018E52C | PROVEN |
| 0x364 / 0x365 | u8 warp-in effect started / u8 motion event 0xD000 seen | 0x8018E55C, 0x8018DDEC | PROVEN |
| 0x368 | weak handle → ridden `Vehicle::AirSaucer` | 0x8018E244 | PROVEN |
| 0x37C | AI subobject (0x84): AI+0x7C u8 "AppearHome done", AI+0x7D u8 NearAttack repeat flag, AI+0x80 `BkSoldierBase*` (dynamic_cast) | 0x80180880, 0x8017FEF8, 0x80180ABC | PROVEN |
| 0x400 | `TEnemySetTask` (vptr E+0x418, vbptr E+0x428, **SET slot E+0x42C**) | ctor | PROVEN |
| 0x430 | `EnemyReferer` | ctor | PROVEN |

---------------------------------------------------------------------------------------------------------------------

## 4. SET parameters → consumers

Sensor params 0‑5 are read from the raw record by `EnemySensor_SetupFromSetParams` exactly as for GunSoldier (PROVEN,
shared `TEnemySetBase::vf06`). The copy at E+0x2E8 is clamped in `BkSoldierBase::InitFromSet` 0x8018E618 and consumed
by the shared `SoldierCommon_ReadSetParams`.

| idx | Param | Clamp | Consumer / effect | Conf |
|---|---|---|---|---|
| 0‑5 | MoveRange, SearchRange/Angle/Width/Height/HeightOffset | — | sensor (E+0xA8) — see GUN_SOLDIER.md §3 | PROVEN |
| 6 | MoveSpeedRatio | — | no BkSoldier reader found | UNKNOWN |
| 7 | HaveShield | 0..1 | E+0x270; shield created in `InitDisplay`; score 0x59 vs 0x58 (`GetScoreId` reads E+0x304) | PROVEN |
| 8 | WeaponType | 0..8 | E+0x278; `EnemyWeapon` kind = table 0x804C9B18[WeaponType] = {–, 0x1F, 0x08, 0x09, 0x0F, 0x1A, 0x18, 0x0B, 0x0A}; 0/1 → melee (Chase/NearAttack), **6 VACUUM_POD → `AttackVacuum`**, else `Attack`; motion selection vf18/vf1A/vf1B | PROVEN (kind semantics UNKNOWN) |
| 9 | AppearType | 0..5 | E+0x274: 0 Standing, 1‑3 Moving (patrol modes 1/2/3), **4 OFFSETPOS / 5 WARP**: spawn at SET pos + (p13, p16, p14), hidden until detected, first AI state `AppearHome`; WARP also E+0x360 = 1 s delay and warp-in effect | PROVEN |
| 10‑22 | Pos0..2 WaitType/WaitSec/MoveSpeedRatio/X/Z | WaitType 0..2, floats ±65536 | patrol table E+0x288 (shared reader); per-point wait motion `BkSoldierBase::vf0C` 0x8018DA98: WaitType 0 ATTACK → attack motion if armed, 1 HIDE → motion 0xF, 2 KAMAE → stance motion | PROVEN (reads) / STRONG (motions) |
| 23 | RideAirSaucer | none | E+0x344: `InitDisplay` creates vehicle kind 7 (`Vehicle_Create` 0x801B4C74 → `Vehicle::AirSaucer::AirSaucer`, VehicleTask on layer 0xC) at the soldier's position; `UpdateMove` then sends `AccelerateInputCommand` (id 1) toward home (E+0x6C) and copies the vehicle pose; on knockback (`BkSoldierBase::OnKnockback` vf12, Status slot 0x2E) the soldier dismounts (`fn_8018D52C`: E+0x344 = 0, pitch/roll 0) | PROVEN (mechanics) / LIKELY (gameplay meaning) |

Census (`tools/agent_enemy2_census.py 8D`): 402 placed records; AppearType {0:105, 1:57, 2:5, 3:66, 4:133, 5:36};
WeaponType {0:51, 1:50, 2:76, 3:81, 4:17, 5:31, 6:14, 7:27, 8:55}; RideAirSaucer=1 on 13; **SearchWidth = 0 on 19**
(AppearType 0:15, 1:4 — stg0404/0501/0503/0600) → those can never alert (see ENEMY_ALERT_PATHS.md).

---------------------------------------------------------------------------------------------------------------------

## 5. AI state machine (BkSoldierAI vtable V+0x294 = 0x80532700)

AI slots: 0x0C `EnemyBaseAI::vf0C` (→ slot 0x0D); **0x0D `BkSoldierAI::ReturnToWait` 0x80180770**: AppearType 0 →
`SoldierCommonAI_WaitAction_Standing`; 1‑3 → `WaitAction_Moving`; 4/5 → first time (AI+0x7C = 0) `AppearHome`, later
Standing; other → Standing. **0x0F `BkSoldierAI::OnTargetFound` 0x80180738** → Caution. 0x12/0x13 base Guard /
WaitFinishDamaged; **0x14 base `OnDead` → Dead**; 0x16 base → Recover. `BkSoldierAI_StartCombat` 0x80180560: WeaponType
0/1 → ChaseTarget, 6 → AttackVacuum, else Attack. All PROVEN.

| State (RTTI) | Getter / singleton | Enter | Update | Exit | Behaviour / transitions | Conf |
|---|---|---|---|---|---|---|
| `EnemyAIState_SoldierCommonAI_WaitAction_Standing/_Moving` | inline in 0x80180770 / 0x805F04FC, 0x805F0508 | shared (GUN_SOLDIER.md §5.4) | shared | shared | detection → AI 0x0F | PROVEN |
| `EnemyAIState_BkSoldierAI_Caution` | 0x8017F460 / 0x805F0514 | 0x801804E0 (motion param 0x13) | 0x8018043C | nop | turn to target; WeaponType 0/1 → ChaseTarget; else on status motion flag B → StartCombat | PROVEN |
| `…_Attack` (ranged) | 0x801805E0 / 0x805F0538 | 0x8017FE34 (idle motion, E+0x359=0, timer 1 s) | 0x8017FA80 | 0x8017FA30 (E+0x359=0) | step0 aim (turn) until aligned or timer<0 → attack motion (param 0x14), hold position (AI vf05 → home), E+0x359=1, timer 3 s; step1: timer<0 & off-angle >30° → aim again, motion end → replay attack; **player leaves area → `BkSoldierAI_GoHome`** | PROVEN |
| `…_AttackVacuum` (VACUUM_POD) | 0x8018068C / 0x805F0544 | 0x8017F980 (idle, timer 1 s) | 0x8017F5EC | 0x8017F590 (weapon stop `fn_8018D700`) | as Attack but motion 0xD and `fn_8018D764`/`fn_8018D700` start/stop the weapon, re-aim when >30° or timer; leaves area → GoHome | PROVEN (weapon helper meaning LIKELY) |
| `…_ChaseTarget` | 0x8018009C / 0x805F0520 | 0x801803C8 (motion param 0x11) | 0x80180234 | nop | not in area → **`EnemyBaseAI_GoHome`**; target distance < **30** → NearAttack | PROVEN |
| `…_NearAttack` (melee) | 0x8018031C / 0x805F052C | 0x801801A4 (variant, attack motion) | 0x8017FF00 | 0x8017FEF0 (AI+0x7D=0) | not in area → `EnemyBaseAI_GoHome`; motion end: distance > 30 → ChaseTarget else re-attack | PROVEN |
| `…_GoHome` | 0x8017F8D4 / 0x805F0550 | 0x8017F50C (home := SET pos) | 0x8017F374 | nop | walk home; in area → Caution; XZ distance < 10 → AI 0x0D | PROVEN |
| `…_AppearHome` | 0x801808E8 / 0x805F055C | 0x8017F2EC | 0x8017F258 | nop | walk to SET pos; XZ < 10 → StartCombat | PROVEN |
| `EnemyAIState_EnemyBaseAI_GoHome` | inline in Chase/Near / 0x805F0448 | 0x8019BF90 (home := SET pos) | 0x8019BF30 | nop | `Enemy::MoveTowardHome` (Enemy::vf01 0x8019B0C8) until XZ < 10 → AI 0x0D; **no sensor test** | PROVEN |
| `EnemyAIState_EnemyBaseAI_Dead` | 0x8019D120 / 0x805F046C | 0x8019BC30: lock, motion = param slot 0x16 (`BkSoldierBase::vf1D`: front 0x10 / back 0x11), timer 0.5 s | 0x8019BB58: timer<0 or status motion flag A → Disp slot 0x11 (death effect) + SB slot 2 (RequestDelete) | 0x8019BB48 unlock | PROVEN |
| `EnemyAIState_EnemyBaseAI_Recover` | 0x8019CFC0 / 0x805F0478 | 0x8019BA2C: S bit 0xD, motion param 0x10, status slot 0x1D(0,1) (collision off, LIKELY), timer 60 s | 0x8019B934: timer<0 or damaged → AI 0x0D | 0x8019B8BC: status slot 0x1D(1,1), clear bit 0xD | PROVEN |
| Guard / WaitFinishDamaged / WaitFinishFalling / Idle / WarpWait / CaptureWait | base, see GUN_SOLDIER.md and ENEMY_ALERT_PATHS.md | | | | | PROVEN |

---------------------------------------------------------------------------------------------------------------------

## 6. Damage and HP

* `BkSoldierBase::TakeHit` 0x8018D81C (Status slot 0x1A): identical structure to GunSoldier's (shield at E+0x34C; attack
  flag 0x40 + frontal (<90°) → blocked, S bit 5 → Guard, SE 0x6076; unless attack flag 0x4 the shield absorbs the hit:
  break → SE 0x6077, S+0x75=1, HaveShield=0, otherwise SE 0x6076 + S bit 5), **damage is then always applied** via
  `EnemyStatusCommon::ApplyHit`; E+0x358 = (hit side == front). **No AppearType-based immunity.** PROVEN.
* HP: `InitHP` → `EnemyParamCommon::vf19` = 0x804CD678[5] = **4.0**; flinch threshold 0.5·MaxHP = 2.0 (shared). PROVEN.
* `Status slot 0x2E` = `BkSoldierBase::OnKnockback` 0x8018D360, called by `EnemyStatusCommon::Update` 0x801A8B08 when
  S+0x75 (flinch) just before `EnemyMoveCommon::Knockback`: if the AirSaucer handle is still valid → dismount. PROVEN.
* Shared framework rule (applies to all enemies): `EnemyStatusCommon::Update` 0x801A8B54..0x801A8BA0 — if the move
  component reports "fell" (M+0x5F, Move slot 2) while the enemy is active (Status slot 0xB) → **HP := 0** → normal
  death edge. PROVEN.

## 7. Death (normal path) — PROVEN unless marked

1. Killing hit (`ApplyHit`): S+0x76 latch, `AwardKillScore` → `Score_Award(attacker, 0x58 "BkSoldier" or 0x59
   "BkSoldierShield")` (ScoreData: w2 150/200, w4 2000), S bit 0xC if `Attacker_IsPlayerAttack`.
2. Next `EnemyStatusCommon::Update` death edge (S+0x90 && !S+0x91): Status slot 0x2F (0x8017BC80 → 0) →
   `ReportLifeEvent(1|4)` → `EnemyManager_OnEnemyLifeEvent` (team **2** counter +0x34[2]++, link group −1,
   `EnemyDeadByPlayerCommand` if killed by the player).
3. Status slot 0x26 `BkSoldierBase::OnDeath` 0x8018D3A0: `EnemyStatusCommon::vf26` (collision attribute 0x100000),
   SE 0x6016 (once per frame), release shield, **throw the weapon** with velocity rotY(±yaw)·(0,20,10)
   (`EnemyWeapon_Drop` → "ScatteringWeapon" pickup), delete it, E+0x354 = 0, **param copy WeaponType (E+0x308) = 0**
   (GunSoldier clears E+0x278 instead).
4. `E+0x20 = 5 ≠ 0` → `TEnemySetBase::DetachKilled` (SB+0x15 = 1; record flags \|= 0x8; `SetSlot_DisableAndDetach`:
   &= ~0x1, &= ~0x6, unlink from link list) → `EnemyStatus_SetCollisionEnabled(S,0,0)` (GunSoldier passes (0,1);
   meaning of the 2nd arg UNKNOWN).
5. AI (same frame): `EnemyBaseAI::OnDead` → Dead state → ≤0.5 s → `EnemyDispCommon::PlayDeathEffect` 0x801A09A8
   (team 2 branch: SE 0x6009 + effect id 0x78 via `fn_801A2294`) → `RequestDelete`.
6. Deletion as §0 row 14; the SET record is not re-armed → no respawn (row 15).
7. Drops: weapon pickup only; no ring/item drop code found (UNKNOWN beyond that).

`VacuumCommand` (id 0x14, `EnemyBaseAI::HandleMessage` 0x8019E658): score + S bit 0xC, `ReportLifeEvent(1)`,
`OnDeath` hook, then (type ≠ 0) `DetachKilled` + `RequestDelete` — instant kill without the Dead state. PROVEN.

## 8. Reporting

| Channel | Payload | Conf |
|---|---|---|
| Mission / EnemyManager | team 2 defeated counter (+0x34[2]); revive not possible (no revive edge after deletion) | PROVEN |
| Score | 0x58/0x59 at the kill; 0x62 "BkRecovery" (w0 200, w3 3000) on the first heal of the record (`AwardRecoveryScore` 0x801A8894) | PROVEN |
| Stage counts | `EnemyManager_ScanSetEnemies` counts every BK_SOLDIER record (no AppearType exception; the AppearType-5 skip is GUN_SOLDIER-only) | PROVEN (test only for id 0x64) |

## 9. Open items
1. `SB slot 5` = `BkSoldierBase::ReinitFromSet` 0x8018E964 (re-runs InitFromSet, Disp slot 0xC, AI slot 6): no caller
   found by vcall scan (LIKELY editor/debug).
2. EnemyWeapon kind ids (table 0x804C9B18) → weapon class mapping UNKNOWN.
3. MoveSpeedRatio (param 6 and per point) consumer UNKNOWN (as for GunSoldier).

## Native C++17 reconstruction checkpoint (2026-09-30)

- **PROVEN** — src/BkSoldierAI.cpp reproduces vf0D's AppearType dispatch and the one-time AI+0x7C flag (0x80180784..0x801808D0), target-found -> Caution (0x80180738), weapon dispatch {0,1}->Chase / 6->AttackVacuum / other->Attack (0x80180574..0x801805C8), the Chase and NearAttack strict 30-unit branches (0x80180234..0x80180308, 0x8017FF00..0x80180088), and the two distinct GoHome paths (0x8019BF30, 0x8017F374). The synthetic branch test is src/tests/test_bksoldier_ai.cpp.
- **PROVEN** — ranged lost-target detection occurs *after* the state-specific motion/weapon operations, at 0x8017FDC0..0x8017FE04 (Attack) and 0x8017F870..0x8017F8B8 (AttackVacuum). The native body hook runs before that test; a regression assertion checks the ordering.
- **UNKNOWN/native hook** — shared patrol movement integration (0x80188B98), animation/root motion, weapon timing and shot effects in Attack (0x8017FA80) / AttackVacuum (0x8017F5EC), and movement result in base GoHome (Enemy::vf01 0x8019B0C8) are delegated to explicit BkSoldierBody hooks. The shared patrol *decision* routine is now reconstructed for GUN Soldier in `src/GunSoldierAI.cpp`; it is not yet reusable by BK Soldier. `stage_sim` wires BK Soldier SET 0x008D to this partial native AI with labelled harness services and proven HP 4.0 (0x804CD678[5]). This module does not claim full native BK Soldier parity, damage/death effects or vehicle riding. EnemyBaseAI::StateDead is still only a base state placeholder.

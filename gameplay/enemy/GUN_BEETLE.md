# GUN Beetle (`GunBeetle`, SET id 0x0065 `GUN_BEETLE`) — flying enemy

Evidence source: `sys/main.dol` (GC, PAL GUPP8P). Class names are original (RTTI); method names are **semantic
guesses** (proposed in `gameplay/notes/enemy2_symbols.csv`). Confidence: PROVEN / STRONG / LIKELY / UNKNOWN.
Framework and reference: `enemy/GUN_SOLDIER.md`; walker siblings `enemy/BK_SOLDIER.md`, `enemy/EGG_PAWN.md`;
alert/activation analysis `enemy/ENEMY_ALERT_PATHS.md`.

Notation: `E+x` complete object, `S+x` = `EnemyStatusCommon` (E+0x110), `AI+x` = AI subobject (**E+0x3A4**),
`SB+x` = `TEnemySetBase` (E+0x250). Component slot `N` = `u32(vptr + 8 + 4·N)`; "primary appended slot off" =
`[[E+0x18]+off]` → SB-table slot (off−0x208)/4 of the `GunBeetleBase`/`FlyerCommonBase` virtuals.

---------------------------------------------------------------------------------------------------------------------

## 0. End-to-end chain

| # | Step | Address(es) | Conf |
|---|---|---|---|
| 1 | SET scan → `SetSlot_TrySpawn` → descriptor 0x80537158 hook +0x0C | 0x800CA8D8 | PROVEN |
| 2 | `GunBeetle_Create(slot)`: `operator new(0x470)`, layer 0xE, `GunBeetle::GunBeetle(this,1,layer,slot)` | 0x80193DD4 | PROVEN |
| 3 | Ctor chain `GunBeetle` 0x80193FF0 → `EnemyTemplate<GunBeetleBase,GunBeetleAI>` 0x801940B4 → `GunBeetleBase` (**`li r5,1`** @0x80194CB0 → `FlyerCommonBase` → `EnemyFlyerCommon` → … → type 1; param defaults `fn_80194D74`) → `GunBeetleAI` 0x801849E8 (E+0x3A4) → `TEnemySetTask` (E+0x438, slot E+0x464) | as listed | PROVEN |
| 4 | Lifecycle 1: resource slot 1+7 = **8** ("enemy/GunBeetleData.one", hook +0x04 0x80193E54); path data `enemy/EnemyPath.one` is a separate resource (slot 6, see §4) | 0x80193E98 | PROVEN |
| 5 | Lifecycle 2: SB slot 6 → `GunBeetleBase::InitFromSet` (vf1D) 0x80194328 (`SetSlot_GetParams(slot,E+0x310,0x48)` + `FlyerCommonBase_ReadSetParams` 0x80193B44: path or float setup); Disp slot 0 → `GunBeetleBase::InitDisplay` (vf1F) 0x80194848 (weapon mount); Status slot 0 = `EnemyStatusCommon::InitHP` → **param slot 0x19 overridden: `GunBeetleBase::GetMaxHP` 0x80194258** (BodyType 0 → 2.0, else 4.0); AI slot 0xC → `EnemyBaseAI::vf0C` → AI slot 0xD `GunBeetleAI::ReturnToWait` 0x80184914 | as listed | PROVEN |
| 6 | Lifecycle 3: Status slot 0xC → `GunBeetleBase::UpdateActivation` (vf0B) 0x801942A4 → `FlyerCommonBase::UpdateActivation` 0x80193CAC: **path beetles (AppearType 1) stay hidden + inactive until the player is in the search area**; floaters (AppearType 0) activate at once; weapon shown/hidden with IsActive | 0x80193CC4..0x80193D90 | PROVEN |
| 7 | Lifecycle 4: Status `EnemyStatusCommon::Update` → AI → Move slot 1 = `EnemyMoveCommon::Update` → Disp slot 2 = `GunBeetleBase::UpdateWeaponAndSpark` (vf20) 0x80194538 (weapon mount firing, hover SE 0x6036, **spark discharge cycle**) | 0x801A74B4.. | PROVEN |
| 8 | AI (float mode): `FlyerCommonAI_WaitFloating` → player in search area **and ActionType == 2** → AI slot 0xF `GunBeetleAI::OnTargetFound` 0x801848DC → `GunBeetleAI_Caution` (1 s) ↔ `GunBeetleAI_Attack` (3 s firing) | 0x80183F7C, 0x80183FD4 | PROVEN |
| 9 | AI (path mode): `FlyerCommonAI_MoveOnPath` follows its path; fires only while inside the [AttackStart, AttackEnd] path fraction (ActionType 2) — **never uses the sensor after activation** | 0x80183CC4 | PROVEN |
| 10 | Damage: `EnemyCharaColliModule::OnDamaged` → Status slot 0x1A = `GunBeetleBase::TakeHit` (vf11) 0x80194A90 (immune to attacks without flag 0x10 while discharging) → `EnemyStatusCommon::ApplyHit`; score 0x51/0x52 | 0x801A9750 → 0x80194A90 | PROVEN |
| 11 | HP 0 edge: `ReportLifeEvent(1)` (**team 0 = GUN counter**) → Status slot 0x26 = `GunBeetleBase::OnDeath` (vf13) 0x80194984 (drop weapon) → type 1 → `TEnemySetBase::DetachKilled` → collision off | 0x801A8C40..0x801A8CA0 | PROVEN |
| 12 | AI same frame: AI slot 0x14 = **`FlyerCommonAI::OnDead` 0x80184318**: no Dead state — immediately `EnemyDispCommon::PlayDeathEffect` (team 0, type 1: SE 0x6007 + effect 0x72) and SB slot 2 `RequestDelete` | 0x8018433C, 0x80184358 | PROVEN |
| 13 | Next `TEnemySetTask::Update` → Task kill → `~GunBeetle` 0x80193EB4 → `~TEnemySetTask` → `~GunBeetleAI` → `~GunBeetleBase` 0x80194B10 (weapon mount, spark effect, SE handles E+0x36C/E+0x388) | 0x801A6F04 | PROVEN |
| 14 | Killed record: flags \|= 0x8, &= ~0x7, unlinked → not re-armed by the scanner | 0x800C9EEC | PROVEN |

---------------------------------------------------------------------------------------------------------------------

## 1. Differences vs GunSoldier (summary)

| Topic | GunSoldier | GunBeetle | Conf |
|---|---|---|---|
| Movement / bases | walker (`SoldierCommon*`) | **flyer** (`FlyerCommonBase`/`FlyerCommonAI`, `EnemyMoveFlyer`; `EnemyMoveFlyer::vf06` = 1 → PreUpdate never enters WaitFinishFalling) | PROVEN |
| MaxHP | table 2.0 | **param override by BodyType: 2.0 / 4.0** (table 0x804CD678[1] = 2.0 is bypassed) | PROVEN |
| Team | 0 GUN | **0 GUN** — beetle kills increment the same EnemyManager team-0 counter as soldiers | PROVEN |
| HP 0 | injured | **explodes the same frame** (`FlyerCommonAI::OnDead`), DetachKilled, deleted next frame | PROVEN |
| Detection use | wait states + GoHome/attack states | float mode only (and only ActionType 2); path mode only uses the sensor for activation | PROVEN |
| Losing the player | GoHome | Attack → **WaitFloating** directly (no GoHome) | PROVEN |
| SET params | 23 clamped | **18, unclamped** (E+0x310..0x357) | PROVEN |
| Shield | HaveShield | none; instead **spark discharge** (ActionType 1) makes it immune to attacks lacking flag 0x10 | PROVEN (flag meaning UNKNOWN) |
| Enum labels | table 0x80539464 | **none found**: no SET-editor label accessor exists for this TU; param 9 is unnamed in the schema | PROVEN (absence by string/xref search) |
| Score | 0x4F/0x50 | **0x51 "GunBeetle"** (w0 100, w3 1000) / **0x52 "GunBeetleG"** (w0 1000, w3 5000) by BodyType | PROVEN |
| Knockback table | {50, 1.3, 1.0} | {2, 2, 1} (0x804CD6B4[1]); misc {50,50,50,50,50,3,0} | PROVEN |

---------------------------------------------------------------------------------------------------------------------

## 2. Identity

| Item | Value | Evidence | Conf |
|---|---|---|---|
| SET id / descriptor | 0x0065 `GUN_BEETLE`, desc 0x80537158, 18 params | catalog | PROVEN |
| Hooks | +0x04 `GunBeetle_LoadResources` 0x80193E54; +0x08 `GunBeetle_ReleaseResources` 0x80193E2C (slot 8); +0x0C `GunBeetle_Create` 0x80193DD4 | disassembly | PROVEN |
| sizeof | 0x470 | `li r3,0x470` 0x80193DE8 | PROVEN |
| RTTI | `GunBeetle` ← `EnemyTemplate<GunBeetleBase, GunBeetleAI>` ← `GunBeetleBase` ← `FlyerCommonBase` ← `EnemyFlyerCommon` ← `TEnemyCommonBase` ← `EnemyRefBase` ← `Enemy`; AI `GunBeetleAI` ← `FlyerCommonAI` ← `EnemyBaseAI`; move `EnemyMoveFlyer` | `q.py class ^GunBeetle$` | PROVEN |
| Type / team | 1 / 0 | `li r5,1` 0x80194CB0; table 0x804CD310[1] | PROVEN |
| Vtable group | V = 0x80537754; E+0x18=V, 0x104=V+0x14, 0x110=V+0x8C, 0x1B4=V+0x160, 0x1E8=V+0x1C8, 0x250=V+0x200, **0x3BC=V+0x28C** (AI), **0x450=V+0x2F0** (Task) | ctor 0x80194050..0x80194078 | PROVEN |

---------------------------------------------------------------------------------------------------------------------

## 3. Object layout (beyond the common E+0x000..0x24F)

| Offset | Field | Evidence | Conf |
|---|---|---|---|
| 0x268 | path follower object (`fn_801A6A44` setup, `fn_801A6944` sample); +0x58 (E+0x2C0) path length | 0x80193B9C, 0x80193C28 | STRONG |
| 0x2D8 / 0x2DC | float pattern (= param 9 in float mode) / FloatWidth | 0x80193C84, 0x80193C8C | PROVEN |
| 0x2E0 / 0x2E4 | path speed (= MoveSpeedRatio × `EnemyParamCommon::vf06`) / distance travelled on the path | 0x80193BCC, 0x8019384C | PROVEN |
| 0x2E8 / 0x2EC / 0x2F0 | attack window start / end distance (= AttackStart/AttackEnd × path length) / window duration in s | 0x80193C34..0x80193C6C | PROVEN |
| 0x2F4 | path state (0 moving, 1 = end reached, LIKELY) | 0x80193860 | PROVEN (meaning LIKELY) |
| 0x300 | vec3 path velocity (clamped to speed·dt) | 0x801938B4.. | PROVEN |
| 0x30C | u8 on-path (MoveOnPath enter 1 / exit 0) | 0x80183F14, 0x80183CBC | PROVEN |
| **0x310..0x357** | 18 SET params (idx *i* at 0x310+4*i*): 0x328 MoveSpeedRatio, **0x32C AppearType, 0x330 ActionType, 0x334 param 9 (path id / float pattern)**, 0x338 FloatWidth, 0x33C AttackStart, 0x340 AttackEnd, 0x344 PathMirror, **0x348 BodyType, 0x34C WeaponType**, 0x350 SparkDischarge, 0x354 SparkWait | `InitFromSet` | PROVEN |
| 0x358 | weapon mount (0x1C, `GunBeetle_CreateWeaponMount` 0x80195038): +4 `EnemyWeapon*`, +8 → per-type triple at 0x804CAD20 + 0xC·WeaponType, +0x10 u8 firing, +0x18 f32 (SparkDischarge for WeaponType 2) | 0x80195060..0x801950D4 | PROVEN (field meaning STRONG) |
| 0x35C | spark effect handle (effect 0x7E) | 0x801946A0 | PROVEN |
| 0x364 / 0x368 | spark state (0 init, 1 waiting, 2 discharging) / spark timer | vf20 | PROVEN |
| 0x36C / 0x388 | loop SE handles: hover 0x6036 / spark 0x603A | vf20 | PROVEN |
| 0x3A4 | AI (0x94): AI+0x74 `FlyerCommonBase*`, AI+0x78 u8 float direction, AI+0x7C f32 float phase, AI+0x8C `GunBeetleBase*` | 0x801843DC, 0x80184A4C, WaitFloating | PROVEN |
| 0x438 | `TEnemySetTask` (vptr E+0x450, **slot E+0x464**) | ctor | PROVEN |
| 0x468 | `EnemyReferer` | ctor | PROVEN |

---------------------------------------------------------------------------------------------------------------------

## 4. SET parameters → consumers (no clamping)

| idx | Param | Consumer / effect | Conf |
|---|---|---|---|
| 0‑5 | MoveRange, Search* | raw record → sensor E+0xA8 (shared). Used by activation (path mode) and WaitFloating/Attack detection | PROVEN |
| 6 | MoveSpeedRatio | **path speed** E+0x2E0 = MoveSpeedRatio × base speed (`FlyerCommonBase_ReadSetParams` 0x80193BBC) | PROVEN |
| 7 | AppearType (0‑1) | `GunBeetleBase::UsesPath` (vf0C) 0x801849D4 = (AppearType == 1): 1 → `MoveOnPath` + hidden until detected; 0 → `WaitFloating` at the SET position | PROVEN |
| 8 | ActionType (0‑2) | `GunBeetleBase::HasAttackWindow` (vf0D) 0x80193DC0 = (ActionType == 2) → float mode may alert/attack, path mode fires inside the window; **ActionType == 1 → spark discharge cycle** (vf20); 0 → passive (no alert, no firing) | PROVEN (no label strings) |
| 9 | (unnamed, 0‑8) | path mode: **path id** → `fn_801A6B4C`: entry index table 0x804CD8D8 = {2,3,…,10} into resource slot 6 = `enemy/EnemyPath.one`; float mode: **float pattern** E+0x2D8 (0/2 horizontal sweeps along facing, 1 up, 3 down, others none) | PROVEN (pattern shapes LIKELY) |
| 10 | FloatWidth | float amplitude E+0x2DC (float disabled if ≤ 0.1); phase advances at 20 u/s (`FlyerCommonBase::vf0E`) | PROVEN |
| 11 / 12 | AttackStart / AttackEnd (0..1) | fractions of the path length → E+0x2E8/E+0x2EC; `FlyerCommon_IsInAttackWindow` 0x80193AD4 | PROVEN |
| 13 | PathMirror | `fn_801A6A44` builds the path with a transform from axis (1,0,0) (`fn_8001C4B8`) instead of identity | PROVEN (mirror meaning LIKELY) |
| 14 | BodyType (0‑1) | MaxHP 2.0/4.0 (`GetMaxHP`), score 0x51/0x52 (`GetScoreId` vf12 0x80194A78), model variant (`vf1E` 0x801948DC, LIKELY) | PROVEN |
| 15 | WeaponType (0‑4) | mount kind table 0x804CAD5C = {3, 0x13, 0, 2}; 4 → **no weapon mount**; 2 → no `EnemyWeapon`, the mount spawns its own projectile (`fn_80209EDC` on layer 0xE + SE 0x6037, LIKELY) | PROVEN (kind semantics UNKNOWN) |
| 16 / 17 | SparkDischarge / SparkWait | spark cycle durations (s) for ActionType 1; SparkDischarge also copied into mount+0x18 for WeaponType 2 | PROVEN |

`enemy/EnemyPath.one` is loaded by the SET marker descriptor 0x258E `EnemyEntryStart` (desc 0x8053BAB0) hook +0x04
`fn_801A238C` into ResourceManager slot 6 (PROVEN).

Census (`agent_enemy2_census.py 65`): 313 records; AppearType {0:263, 1:50}; ActionType {0:135, 1:26, 2:152};
param 9 {0:188, 1:51, 2:27, 3:13, 5:7, 6:8, 7:2, 8:17}; BodyType {0:302, 1:11}; WeaponType {0:149, 1:17, 2:2, 3:66,
4:79}; **SearchWidth = 0 on 13** (12 floaters — never alert; 1 path beetle in stg0504 with param 9 = 8 that can never be
revealed).

---------------------------------------------------------------------------------------------------------------------

## 5. AI state machine (GunBeetleAI vtable V+0x28C = 0x805379E0)

AI slots: 0x0C base → 0x0D; **0x0D `GunBeetleAI::ReturnToWait`** (UsesPath → MoveOnPath else WaitFloating); **0x0F
`GunBeetleAI::OnTargetFound`** → Caution; 0x12/0x13 base Guard / WaitFinishDamaged; **0x14 `FlyerCommonAI::OnDead`**;
0x16 base Recover. PROVEN.

| State (RTTI) | Getter / singleton | Enter | Update | Exit | Behaviour / transitions | Conf |
|---|---|---|---|---|---|---|
| `EnemyAIState_FlyerCommonAI_WaitFloating` | inline (0x80184914, Attack update) / 0x805F056C | 0x80184270 (motion, AI+0x78 = 1, phase 0) | 0x80183F30 | nop | player in area: turn to target, **ActionType 2 → AI 0x0F**; else hover pattern around the SET position (FloatWidth, pattern E+0x2D8) | PROVEN |
| `EnemyAIState_FlyerCommonAI_MoveOnPath` | inline / 0x805F0578 | 0x80183E8C (motion, E+0x2F8 = E+0x2FC = 1, E+0x30C = 1) | 0x80183CC4 | 0x80183CB0 (E+0x30C = 0) | `FlyerCommon_AdvanceOnPath` 0x80193814 every frame; entering the attack window → `StartFiring` (vf0F, mount+0x10 = 1), aim at target (+10 Y); leaving it → `StopFiring` (vf10) | PROVEN |
| `EnemyAIState_GunBeetleAI_Caution` | 0x80184684 / 0x805F0624 | 0x801848C8 (timer 1 s) | 0x80184784 | nop | aim at target + (0,10,0); timer < 0 → Attack | PROVEN |
| `EnemyAIState_GunBeetleAI_Attack` | 0x8018481C / 0x805F0630 | 0x80184730 (timer 3 s, `StartFiring`) | 0x80184574 | 0x80184540 (`StopFiring`) | **player left area → WaitFloating**; aim; timer < 0 → Caution (fire 3 s / pause 1 s cycle) | PROVEN |
| base Guard / WaitFinishDamaged / Recover / WarpWait / CaptureWait | see BK_SOLDIER.md §5 | | | | WaitFinishFalling unreachable (flyer) | PROVEN |

---------------------------------------------------------------------------------------------------------------------

## 6. Damage and HP

* `GunBeetleBase::TakeHit` 0x80194A90: if spark state == 2 (discharging) and the attacker handle is valid and the
  attack flags (+0x40) lack **0x10** → hit ignored; otherwise `EnemyStatusCommon::ApplyHit`. No shield. PROVEN.
* Spark cycle (ActionType 1, in `UpdateWeaponAndSpark`): state 1 waits SparkWait s → state 2 for SparkDischarge s:
  Status slot 0x1F(4) (collision/attack attribute, LIKELY "electrified"), effect 0x7E at node 1000, loop SE 0x603A →
  back to state 1 with Status slot 0x1F(1). PROVEN (mechanics) / LIKELY (gameplay meaning).
* HP by BodyType 2.0/4.0; flinch at 0.5·MaxHP → knockback {2, 2} and WaitFinishDamaged (base). PROVEN.

## 7. Death (normal path) — PROVEN
1. Killing hit: `AwardKillScore` → 0x51/0x52; S bit 0xC if killed by the player.
2. Death edge: `ReportLifeEvent(1|4)` → EnemyManager team **0** (same counter as GUN soldiers; link group −1).
3. `GunBeetleBase::OnDeath` 0x80194984: collision 0x100000; if the mount holds an `EnemyWeapon` (kind ≠ 0) → drop it
   with velocity rotY(yaw)·(0,0,5) → "ScatteringWeapon" pickup; delete the mount.
4. `DetachKilled` (record flag 0x8, disable, detach, unlink), `EnemyStatus_SetCollisionEnabled(S,0,0)`.
5. Same frame `FlyerCommonAI::OnDead`: death effect 0x72 + SE 0x6007, `RequestDelete`; destroyed next frame. No death
   motion, no Dead state.

## 8. Open items
1. Weapon mount kinds (3, 0x13, 2, and the WeaponType 2 projectile object `fn_80209EDC`) → weapon classes UNKNOWN.
2. Exact float-pattern shapes and the PathMirror transform — LIKELY only.
3. Attack flag 0x10 meaning (what can hurt a discharging beetle) — UNKNOWN.
4. `SB slot 5` = `GunBeetleBase::ReinitFromSet` 0x80194378 (rebuilds mount/model when BodyType changes) — no caller
   found (LIKELY editor/debug).

## 9. Partial native decision AI (2026-09-30)

`src/GunBeetleAI.cpp` and `src/include/shadow/gameplay/GunBeetleAI.hpp` reconstruct the **PROVEN**
AI decisions from 0x80184914, 0x80183F30, 0x80183CC4, 0x80184784, and 0x80184574. Float mode
uses the shared sensor and enters Caution only for ActionType 2. Caution's DOL timer is 1 s
(0x801848C8; float at 0x805F619C), Attack's is 3 s (0x80184730; 0x805F6198), and each
expires only when `timer < 0` after `EnemyBaseAI::PreUpdate` subtracts dt. Attack loss of the
target returns directly to WaitFloating. Path mode checks the inclusive attack window before
advancing its path (0x80183CC4..0x80183DF4); its timing derives from the path length, speed, and
SET fractions at 0x80193C24..0x80193C78. Path mode does not use the sensor during AI Update.
The immediate death effect/delete request follows 0x80184318.

The C++ `GunBeetleBody` methods are **explicit engine hooks** for path geometry and traversal,
float motion, display, aiming, weapon firing, and effects. The native AI does not claim those
systems are reconstructed. `ConfigurePath` requires an engine-provided path length and base
speed; without it, firing window evaluation is disabled (`pathConfigured` is a labelled native
guard, not DOL behavior). Activation result is exposed by `ShouldActivate`; the owner must
apply visibility/activity and call AI Update only when active, following 0x80193CAC.
`src/tests/test_gunbeetle_ai.cpp` checks float/path separation, strict timer edges, window
inclusivity, check-before-path-advance ordering, firing transitions, and the immediate death
request. The isolated clang++ regression and the full Release 21/21 CTest gate passed.
`stage_sim` uses the partial native AI for SET 0x0065 with labelled harness hooks; it
does not load EnemyPath.one geometry or enforce the original path activation rule.

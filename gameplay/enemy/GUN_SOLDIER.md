# GUN Soldier — reference enemy (`GunSoldier::GunSoldier`, SET id 0x0064 `GUN_SOLDIER`)

Evidence source: `sys/main.dol` (GameCube). Class names are original (RTTI). Method/function names that are not
RTTI ctor/dtor names are **semantic guesses** (curated names from `gameplay/symbols_curated.csv` or
`gameplay/notes/enemy_symbols.csv`). Confidence: PROVEN (direct code/data), STRONG (several consistent pieces),
LIKELY (single indirect piece), UNKNOWN. Raw investigation log: `gameplay/notes/enemy_gunsoldier_trace.md`.

Notation: `E+x` complete object; `S+x` = `EnemyStatusCommon` subobject (E+0x110); `AI+x` = AI subobject (E+0x360);
`M+x` = move subobject (E+0x1E8); `SB+x` = `TEnemySetBase` (E+0x250). A virtual call `Comp.slot N` is resolved as
`u32(vptr + 8 + 4·N)` with the GunSoldier vptr values below.

---------------------------------------------------------------------------------------------------------------------

## 0. End-to-end chain

| # | Step | Address(es) | Conf |
|---|---|---|---|
| 1 | SET scan finds an enabled, in-range record with id 0x0064 → `SetSlot_TrySpawn` clears flags 0x80041 and calls descriptor 0x80539868 hook +0x0C | 0x800CA8D8 (0x800CA930, 0x800CA94C) | PROVEN |
| 2 | `GunSoldier_Create(slot)`: `operator new(0x418)`, parent = Enemy layer (`TaskManager_GetLayer(mgr,0xE)`), `GunSoldier::GunSoldier(this,1,layer,slot)` | 0x80196E38 | PROVEN |
| 3 | Ctor chain: vbase ptrs → E+0x410; `EnemyTemplate<GunSoldierBase,GunSoldierAI>` → `GunSoldierBase` (enemy type 0 passed as `li r5,0` @0x80198B40 → `Enemy::Enemy` stores E+0x20 @0x8018F880; SET-param defaults fn_8018F0B4) → `GunSoldierAI` (→ `EnemyBaseAI` ctor: state machine init, message handler bound) → `TEnemySetTask` (Task linked under layer; slot stored at E+0x40C; `SetSlot_Attach` sets record flag 0x2) | 0x8019708C, 0x80198AF4, 0x801886E8, 0x8019F1D4, 0x801A70D0 | PROVEN |
| 4 | Ctor post-init virtual `[[E+0x18]+0x2A0]` = `GunSoldier_MarkSlotFlag8` → record flag \|= 0x8 | 0x80197140 → 0x801975B0 → 0x800C9C44 | PROVEN |
| 5 | Per frame the scheduler calls `TEnemySetTask::Update` → `TEnemySetBase::LifecycleUpdate` (state at SB+8) | 0x8004ECAC → 0x801A6EB8 → 0x801A7280 | PROVEN |
| 6 | Lifecycle 1 (resources): wait until resource slot E+0x24 (= type+7) is loaded and fn_801780AC; start model/async handle SB+0xC from SET position | 0x801A72C8..0x801A7334 | STRONG |
| 7 | Lifecycle 2 (init components, once handle ready): `SB slot 6` → `GunSoldierBase::vf24` (SET params, STemporaryData, `TEnemySetBase::vf06` pos/rot/detection sensor); `Disp slot 0` → `GunSoldierBase::vf17` (display + `EnemyWeapon`); `Status slot 0` → `GunSoldierBase::vf0E` (HP); `AI slot 0` → `EnemyBaseAI::vf00` → `GunSoldierAI::vf0C` (initial state); `Move slot 0` | 0x801A7388..0x801A7430; 0x80198530, 0x80197F64, 0x80198340, 0x8019EEC4, 0x801A6078 | PROVEN |
| 8 | Lifecycle 3 (activation): `Status slot 0xC` → `GunSoldierBase::vf11` (AppearType OFFSETPOS stays hidden/inactive until the player is in the detection area); when S+0x6C (active) → E+0xF8 = frame counter, `Status slot 1` → `GunSoldierBase::vf10` → `EnemyStatusCommon::vf01` (body collision + damage callbacks) → state 4 | 0x801A7434..0x801A74AC; 0x80198450; 0x801980D0 | PROVEN |
| 9 | Lifecycle 4 (running), per frame: Status update (death/revive edges) → AI update (`EnemyBaseAI::vf01`, pre-update `vf09`, current state Update) → Disp/Move → weapon/look-at (`GunSoldierBase::vf0B`) → Status end-of-frame reset | 0x801A74B4..0x801A75AC (§4) | PROVEN |
| 10 | Detection: WaitAction states test `EnemySensor_IsInSearchArea(E+0xA8, nearest player)` → Caution → Attack/ChaseTarget/NearAttack; leaves area → GoHome → wait | 0x801A6C7C; §5 | PROVEN |
| 11 | Interaction/damage: CharaColli callback → `EnemyCharaColliModule::OnDamaged` → `GunSoldierBase::TakeHit` (shield) → `EnemyStatusCommon::ApplyHit` → `ApplyDamage` (HP E+0x18C −= dmg); killing hit awards score to attacker | 0x801A9750 → 0x80197700 → 0x801A83B0 → 0x801A8718 | PROVEN |
| 12 | HP 0 edge (next status update): report defeat to EnemyManager (mission counter, EnemyDeadByPlayer), convert to **injured** (`GunSoldierBase::BecomeInjured`: AppearType := 5 HINSHI, weapon dropped as "ScatteringWeapon", shield dropped); **no SET "killed" call for type 0**; AI → ToHinshi → Hinshi (immune, calls for help) | 0x801A8BD8..0x801A8C78; 0x80198184; 0x8019EBE0 → 0x80188520 → 0x80187FC0 | PROVEN |
| 13 | Optional: `RecoverCommand` (Heal Cannon/`HealingUnit::HealingBlast`) → HP up → revive edge → defeat un-counted, healer gets "GunRecovery" score once → `Thanks` | 0x8019E494..0x8019E614; 0x801A8CB8..0x801A8D4C | PROVEN |
| 14 | Despawn: `TEnemySetTask::Update` fades out when out of fade range / despawn requested / SB delete request → Task kill flag → `~GunSoldier` (writes STemporaryData) → `~TEnemySetTask` → `SetSlot_Detach` (record flags &= ~0x6) | 0x801A6EB8; 0x80196F3C; 0x801A7034; 0x800C9FD0 | PROVEN |

---------------------------------------------------------------------------------------------------------------------

## 1. Identity

| Item | Value | Evidence | Conf |
|---|---|---|---|
| SET object id / name | 0x0064 `GUN_SOLDIER` | descriptor 0x80539868 in catalog 0x8052C1A0 (`data/setobj_catalog.txt`) | PROVEN |
| Descriptor hooks | +0x04 `GunSoldier_LoadResources` 0x80196EB8, +0x08 `GunSoldier_ReleaseResources` 0x80196E90, +0x0C `GunSoldier_Create` 0x80196E38 | catalog | PROVEN |
| Resource file | `enemy/GunSoldierData_%s.one`, suffix table 0x80539440 indexed by `Enemy_GetStageDataVariant`: ark00, ark50, citii, densi, iseki, jungl, kichi, nazca (all present in `files/enemy/`) | 0x80196EE0, 0x80196ED4 | PROVEN |
| sizeof | 0x418 | `li r3,0x418` 0x80196E4C | PROVEN |
| RTTI | `GunSoldier::GunSoldier` (typeinfo 0x805E8FE0) ← `EnemyTemplate<GunSoldier::GunSoldierBase, GunSoldier::GunSoldierAI>` ← `GunSoldierBase` ← `SoldierCommonBase` ← `EnemyWalkerCommon` ← `TEnemyCommonBase` ← `EnemyRefBase` ← `Enemy`; AI `GunSoldier::GunSoldierAI` ← `SoldierCommonAI` ← `EnemyBaseAI` | `q.py class` | PROVEN |
| Enemy type index (E+0x20) | 0 (tables indexed by it: MaxHP 0x804CD678, team 0x804CD310, knockback 0x804CD6B4, misc 0x804CD4D4 stride 0x1C, s16 0x804CD34C) | 0x80198B40, 0x8018F880 | PROVEN |
| Team | 0 = GUN (table 0x804CD310[0]; recovery score names map team 0/1/2 → Gun/Egg/Bk) | `EnemyParamCommon::GetTeam` 0x801A68A0; `EnemyStatusCommon::AwardRecoveryScore` 0x801A8894 | PROVEN |
| Vtable group | V = 0x80539E64; vptr E+0x18=V, E+0x104=V+0x14, E+0x110=V+0x8C, E+0x1B4=V+0x160, E+0x1E8=V+0x1C8, E+0x250=V+0x200, E+0x378=V+0x2A4, E+0x3F8=V+0x308 | ctor 0x801970EC..0x80197128 | PROVEN |

Editor/debug enum names (string table 0x80539464.., indexed by value in the SET-editor label accessor fn_8019725C):
WeaponType 0 NONE, 1 KNIFE, 2 GUN, 3 MACHINEGUN, 4 RIFLE, 5 GRENADE, 6 MISSILE; AppearType 0 STAND, 1 LINEAR_MOVE,
2 TRIANGLE_MOVE, 3 RANDOM_MOVE, 4 OFFSETPOS, 5 HINSHI; WaitType 0 RADIO_CONTACT, 1 ATTACK, 2 HIDE, 3 KAMAE. (STRONG)

---------------------------------------------------------------------------------------------------------------------

## 2. Object layout (sizeof 0x418)

| Offset | Size | Field | Evidence | Conf |
|---|---|---|---|---|
| 0x000 | 4 | ptr → param component (`EnemyParamObjBase`) | status vf00 0x801A9584 | PROVEN |
| 0x004 | 4 | ptr → `EnemyStatusCommon` (E+0x110) | 0x801A7434 | PROVEN |
| 0x008 | 4 | ptr → `EnemyDispCommon` (vptr at comp+0xC) | 0x801A73C4 | PROVEN |
| 0x00C | 4 | ptr → `EnemyMoveWalker` | 0x801A7418 | PROVEN |
| 0x010 | 4 | ptr → AI (vptr at comp+0x18) | 0x801A73FC | PROVEN |
| 0x014 | 4 | ptr → `TEnemySetBase` | 0x801A73A8 | PROVEN |
| 0x018 | 4 | primary vptr (V) | ctor | PROVEN |
| 0x01C | 4 | type + 0x99 | `Enemy::Enemy` 0x8018F87C | PROVEN (meaning UNKNOWN) |
| 0x020 | 4 | s32 enemy type (0) | 0x8018F880 | PROVEN |
| 0x024 | 4 | type + 7 = resource-manager slot (7) tested in lifecycle 1 | 0x8018F884, 0x801A72D4 | STRONG |
| 0x028 | 4 | EnemyManager registration id (`fn_801A2D6C`) | 0x8018F8E0 | PROVEN |
| 0x030 | 4 | ptr → transform (pos vec3 @+0 via fn_8017CA44, rot vec3 @+0xC via fn_8017CA38; dt via [+0x60]) | 0x8017CA38..0x8017CA48, 0x8017C764 | PROVEN |
| 0x048 | 12 | SET spawn position (fn_801A7718) | 0x801A7678 | PROVEN |
| 0x06C | 12 | home / move target (`fn_8017E1C4` copies E+0x48 here on Standing/GoHome/AppearHome enter) | 0x8017E1CC | PROVEN |
| 0x0A8 | 0x44 | **detection sensor**: +0x00/+0x08 home XZ, +0x0C home vec3, +0x18 MoveRange, +0x1C MoveRange², +0x20/+0x28 centre XZ, +0x2C vec3 centre (+0x30 = centre Y), +0x38 SearchWidth, +0x3C SearchWidth², +0x40 SearchHeight | fn_801A6DB8 0x801A6DD8..0x801A6EA0 | PROVEN |
| 0x0EC | 4 | f32 display alpha (fade) | 0x801A6F30 | PROVEN |
| 0x0F8 | 4 | frame counter when activated | 0x801A7488 | PROVEN |
| 0x0FC | 4 | frame stamp of last once-per-frame SE (fn_8019AD10) | 0x8019AD4C | PROVEN |
| 0x100 / 0x108 / 0x114 / 0x1B8 / 0x1EC / 0x24C / 0x254 / 0x37C / 0x408 | 4 each | vbase ptrs → `EnemyReferer` E+0x410 | ctor 0x801970B0..0x801970D0 | PROVEN |
| 0x104 | 0xC | `EnemyParamCommon` (vptr V+0x14; +8 name/param ptr ← 0x805F66A8 set in GunSoldierBase ctor via slot 0xE) | 0x80198BC0 | PROVEN |
| 0x110 | 0x98 | `EnemyStatusCommon` (vptr V+0x8C), see §2.1 | RTTI base list | PROVEN |
| 0x1A8 | 0x40 | `EnemyDispCommon` (vptr at 0x1B4 = V+0x160) | ctor | PROVEN |
| 0x1E8 | 0x68 | `EnemyMoveWalker` (vptr V+0x1C8): M+0x38 height (grounded if < 5.0), M+0x50 vec3 velocity (knockback), M+0x5D/+0x5E knock flags, M+0x5F fell/out flag (LIKELY) | 0x801A5B1C, 0x801A59E8, 0x8017B92C | PROVEN (fields) |
| 0x250 | ~0x18 | `TEnemySetBase` (vptr V+0x200): SB+8 lifecycle state, SB+0xC async handle, SB+0x14 u8 delete request, SB+0x15 u8 SET already detached | 0x801A7334, 0x8017B8EC..0x8017B904, 0x801A7250 | PROVEN |
| 0x270 | 1 | u8 HaveShield (runtime) | fn_80198D90 0x80198DC0 | PROVEN |
| 0x274 | 4 | s32 AppearType (runtime mode; 5 = HINSHI/injured) | 0x80198DC4, 0x801982A0 | PROVEN |
| 0x278 | 4 | s32 WeaponType (runtime; 0 after weapon drop) | 0x80198DC8, 0x80198288 | PROVEN |
| 0x27C | 4 | patrol mode from AppearType (0,1,2,3; 4/5 → 0) | GunSoldierBase::vf0D 0x80197938 | PROVEN |
| 0x280 | 4 | current patrol point index | WaitAction states 0x80189614 | STRONG |
| 0x284 | 4 | f32 RANDOM_MOVE radius = min(param16, MoveRange) | 0x80198ED0..0x80198EE0 | PROVEN |
| 0x288 | 3×0x18 | patrol points {vec3 pos (home+X/Z), f32 WaitSec, f32 MoveSpeedRatio, s32 WaitType} | fn_80198D90 | PROVEN |
| 0x2DC..0x2E4 | 0xC | RANDOM_MOVE wait data (Pos0 WaitSec/MoveSpeedRatio/WaitType) | 0x80198EF4..0x80198F00 | PROVEN |
| 0x2E8 | 0x5C | copy of the 23 SET params (`SetSlot_GetParams`), ints clamped | 0x8019855C..0x801987DC | PROVEN |
| 0x344 | 4 | attack animation variant (frame counter & 1) | 0x80186F8C | PROVEN |
| 0x348 | 8 | shield handle (shared-ptr; empty = no shield); shield HP at [obj]+0xC | fn_8017C554, fn_801A78A4 | STRONG |
| 0x350 | 4 | `EnemyWeapon*` (0x68 bytes) | GunSoldierBase::vf17 0x8019802C | PROVEN |
| 0x354 / 0x355 | 1+1 | look-at/aim state bytes | GunSoldierBase::vf0B 0x80197DB0.. | PROVEN (meaning LIKELY) |
| 0x356 | 1 | u8 rescued (healed) | 0x80198154, 0x80188308 | PROVEN |
| 0x357 | 1 | u8 pose variant: after each hit = (hit side == front); for HINSHI spawns = had shield; selects fall/injured/flinch anims | 0x801978B8..0x801978D0, 0x801988C4 | PROVEN (meaning LIKELY) |
| 0x358 | 1 | u8 copied into weapon[0] when the fire motion event occurs | 0x80197D3C | PROVEN |
| 0x35C | ? | look-at controller (fn_802FAFC4 ctor, fn_802FAF38 modes 0‑4) | 0x80198BAC | STRONG |
| 0x360 | 0x80 | AI subobject, see §2.2 | RTTI | PROVEN |
| 0x3E0 | 0x30 | `TEnemySetTask`: Task (flags u16 at E+0x3E4, vptr E+0x3F8), vbptr E+0x408, **SET slot E+0x40C** | 0x801A7128 | PROVEN |
| 0x410 | 8 | `EnemyReferer` (+0 = E) | 0x80197130 | PROVEN |

### 2.1 Status subobject (S = E+0x110)

| S off | E off | Field | Evidence | Conf |
|---|---|---|---|---|
| +0x08 | 0x118 | collision module (LIKELY `EnemyCharaColliModule`); `fn_801A9918(mod, mask)` sets collision attribute (default 805F6778, dead 0x100000, injured 0x200201) | 0x801A90E4, 0x801A8880, 0x80198114 | STRONG |
| +0x20 | 0x130 | `EnemyMotionSequenceModule`; S+0x3A / S+0x3B motion flags (LIKELY "motion end" / loop) | status vf06/vf08/vf09 | STRONG |
| +0x64 | 0x174 | attached object (fn_8031AFB4 / fn_8031B020 / fn_8031B08C) | 0x801A8CA4 | UNKNOWN |
| +0x6C / +0x6D | 0x17C / 0x17D | u8 active / u8 hide-request (fade out) | vf0A/vf0B/vf0D/vf0E, 0x801A6F98 | PROVEN |
| +0x70 | 0x180 | current motion/action id (`EnemyStatusCommon::vf06` SetMotion) | 0x8017B9E4 | PROVEN |
| +0x74 | 0x184 | damaged this frame | 0x801A872C | PROVEN |
| +0x75 | 0x185 | flinch this frame (kill, 50 % threshold, shield break) | 0x801A8490, 0x801A84C4, 0x80197848 | PROVEN |
| +0x76 | 0x186 | killed latch (no further damage until revive) | 0x801A8448, 0x801A8D10 | PROVEN |
| +0x77 | 0x187 | flag cleared on Hinshi enter, set on exit (`vf1E`) | 0x80187E04 | PROVEN (meaning UNKNOWN) |
| **+0x78** | **0x188** | **f32 MaxHP** | vf00 0x801A959C | PROVEN |
| **+0x7C** | **0x18C** | **f32 HP** (init = MaxHP = 2.0; 0 for HINSHI spawns) | vf00 0x801A9598, 0x80198374 | PROVEN |
| +0x80 | 0x190 | u32 bitset: 2 damage reaction, 5 shield-guard request, 0xC killed by player, 0xD Thanks active, 0xE capture (LIKELY) | BitTest/BitSet refs | PROVEN (bits) |
| +0x84 | 0x194 | s32 hit side 0 front / 1 back / −1 none | 0x801A8528, 0x801A8574, 0x801A8700 | PROVEN |
| +0x8A/+0x8B | 0x19A/0x19B | damaged latch cur/prev | 0x801A8AE4 | PROVEN |
| +0x8C/+0x8D | 0x19C/0x19D | flinch cur/prev | 0x801A8B30 | PROVEN |
| +0x8E/+0x8F | 0x19E/0x19F | airborne cur/prev | 0x801A8ACC | PROVEN |
| **+0x90/+0x91** | **0x1A0/0x1A1** | **IsDead (HP ≤ 0) cur/prev** | 0x801A8BA4..0x801A8BD4 | PROVEN |
| +0x94 | 0x1A4 | f32 recent-hit timer (+1 s per hit, −dt) | 0x801A8734, 0x801A8A8C | PROVEN |

### 2.2 AI subobject (AI = E+0x360) — state-machine block

| AI off | E off | Field | Evidence | Conf |
|---|---|---|---|---|
| +0x18 | 0x378 | vptr (V+0x2A4 = 0x8053A108) | ctor | PROVEN |
| +0x1C | 0x37C | vbptr → E+0x410 | ctor | PROVEN |
| +0x20 | 0x380 | f32 dt | 0x8019EAE0 | PROVEN |
| +0x24 | 0x384 | s32 state sub-step | state code | PROVEN |
| **+0x28** | **0x388** | **SM owner (AI\*)** | fn_8019F528 | PROVEN |
| **+0x2C** | **0x38C** | **current state (TEnemyAIState\*)** | 0x8019EAF4 | PROVEN |
| **+0x30** | **0x390** | **previous state** | 0x8019FD64 | PROVEN |
| **+0x34** | **0x394** | **u8 transition lock** | 0x8019FD18 | PROVEN |
| +0x38 | 0x398 | target tracker; target position at AI+0x44 (E+0x3A4) | fn_8019FA18 | STRONG |
| +0x40 | 0x3A0 | u8 cleared every frame | 0x8019EB6C | PROVEN (meaning UNKNOWN) |
| +0x60 | 0x3C0 | turn helper {E, AI, rate} (`EnemyAI_TurnTowards` fn_8019F8F8) | 0x8019F230 | STRONG |
| +0x6C / +0x70 | 0x3CC / 0x3D0 | f32 state timer (−dt per frame) / reset value | 0x8019EB7C | PROVEN |
| +0x78 | 0x3D8 | SoldierCommonAI soldier ptr | 0x80189610 | STRONG |
| +0x7C | 0x3DC | GunSoldierAI: `dynamic_cast<GunSoldierBase*>` | 0x8018874C | PROVEN |

---------------------------------------------------------------------------------------------------------------------

## 3. SET parameters → consumers

Two readers: `TEnemySetBase::vf06` → fn_801A7608 → **`EnemySensor_SetupFromSetParams` 0x801A6DB8** reads the raw record
misc block (`SetSlot_GetParamPtr` 0x800CA118, record+0x28); `GunSoldierBase::vf24` copies all 23 into E+0x2E8 and
`SoldierCommon_ReadSetParams` 0x80198D90 (shared with BkSoldier) consumes indices 7..22.

| idx | Param | Consumer | Effect | Conf |
|---|---|---|---|---|
| 0 | MoveRange | sensor +0x18/+0x1C (0x801A6DFC) | XZ leash around home; `EnemySensor_IsInsideMoveRange` 0x801A6D00 used by `EnemyMoveCommon::vf01` 0x801A5C44 to clamp movement; RANDOM_MOVE radius cap | PROVEN |
| 1 | SearchRange | 0x801A6E2C | forward offset of detection centre | PROVEN |
| 2 | SearchAngle (deg) | 0x801A6DF8 | offset yaw = rot.y + (180 + SearchAngle)·π/180 | PROVEN |
| 3 | SearchWidth | 0x801A6E80 | detection radius (sensor +0x38, ² at +0x3C) | PROVEN |
| 4 | SearchHeight | 0x801A6E8C | detection half-height (+0x40) | PROVEN |
| 5 | SearchHeightOffset | 0x801A6E18 | centre Y offset | PROVEN |
| 6 | MoveSpeedRatio | not read by GunSoldier code | — | UNKNOWN |
| 7 | HaveShield | E+0x270 (0x80198DC0); E+0x304 → score id 0x4F/0x50 | shield, score | PROVEN |
| 8 | WeaponType (0‑6) | E+0x278 | 0 NONE / 1 KNIFE → melee (ChaseTarget/NearAttack); 2‑6 → ranged `Attack`; anim selection 0x80197A50 | PROVEN |
| 9 | AppearType (0‑5) | E+0x274 | initial state (§5.3); OFFSETPOS spawns at +(p13, p16, p14) (0x801988E0..0x80198924) and is hidden/inactive until detection; HINSHI = injured | PROVEN |
| 10‑12 | Pos0 WaitType/WaitSec/MoveSpeedRatio | E+0x29C/0x294/0x298 (point 0 at home) | patrol point 0 | PROVEN |
| 13‑17 | Pos1 X/Z/WaitType/WaitSec/MoveSpeedRatio | point 1 (E+0x2A0..) ; idx 16 also RANDOM_MOVE radius and OFFSETPOS Y offset | patrol | PROVEN |
| 18‑22 | Pos2 X/Z/WaitType/WaitSec/MoveSpeedRatio | point 2 (E+0x2B8..), TRIANGLE_MOVE only | patrol | PROVEN |

**Detection test** `EnemySensor_IsInSearchArea(E+0xA8, pos)` 0x801A6C7C: `|pos.y − centreY| < SearchHeight &&
distXZ²(pos, centre) < SearchWidth²` (PROVEN). Target position = `EnemyBaseAI::vf02` 0x8019DF7C (nearest player
position; own position if none — STRONG).

---------------------------------------------------------------------------------------------------------------------

## 4. Per-frame update (lifecycle state 4, 0x801A74B4..0x801A75AC) — PROVEN
1. `Status slot 2` → `GunSoldierBase::vf0F` 0x801982FC → `EnemyStatusCommon::vf02` 0x801A8A38 (*StatusUpdate*: timers,
   latches, flinch → knockback, IsDead edge → death/revive handling §7) + `GunSoldier::GunSoldier::WriteTempState` 0x801975DC.
2. `Status slot 3` 0x801A87D0.
3. `AI slot 1` = `EnemyBaseAI::vf01` 0x8019EACC (§5).
4. `Status slot 4` 0x801A8768; `Disp slot 1` 0x801A1370; `Move slot 1` = `EnemyMoveCommon::vf01` 0x801A5B34 (movement, leash).
5. `Disp slot 2` → `GunSoldierBase::vf0B` 0x80197D64 (look-at, weapon placement/fire SE 0x600D).
6. `Status slot 5` = `EnemyStatusCommon::vf05` 0x801A86CC: S+0x74 = S+0x75 = 0, S+0x84 = −1.

---------------------------------------------------------------------------------------------------------------------

## 5. AI state machine

### 5.1 Storage and transition API (PROVEN)
State objects are **static singletons** (function-local statics, guard byte + `__register_global_object`), shared by
all soldiers; the owner is re-bound before every call. `TEnemyAIState` slots: 1 *BindOwner* (`state+4 =
dynamic_cast<AI>(owner)`), 2 *Enter*, 3 *Update*, 4 *Exit*, 5 name (`"none"`).

| Addr | Name | Behaviour |
|---|---|---|
| 0x8019FD00 | `EnemyAISM_ChangeState(sm,new)` | if locked: no-op; else old.Bind, old.Exit; prev = cur; cur = new; new.Bind; new.Enter |
| 0x8019FC54 | `EnemyAISM_RevertToPrevious` | ChangeState(prev) |
| 0x8019FDB8 / 0x8019FDAC | `EnemyAISM_Lock` / `Unlock` | sm+0xC = 1 / 0 |
| 0x8019F528 | `EnemyAISM_Init` | owner = AI, state = `EnemyBaseAI_None` |

Message handler: `EnemyBaseAI::EnemyBaseAI` binds PMF `{0, slot 0x28, vptr 0x18}` (data 0x8053B6B8) = `EnemyBaseAI::vf08`
and registers it (`fn_801A0120` @0x8019F33C).

### 5.2 `EnemyBaseAI::vf01` 0x8019EACC → `vf09` 0x8019EB54 order (PROVEN)
```
AI+0x20 = dt; vf09():
   AI+0x40 = 0; AI+0x6C -= dt
   1. S.IsDead (S+0x90)?  if !S.WasDead: unlock; AI slot 0x14 (GunSoldier: → ToHinshi)      0x8019EBE0 ; return
   2. byte 0x8057E828 != 0 → ChangeState(Idle)                                              0x8019EBF0 ; return
   3. S bit 5 → clear; AI slot 0x12 → Guard                                                  0x8019ECE8 ; return
   4. S+0x8C (flinch) → unlock; AI slot 0x13 → WaitFinishDamaged                            0x8019ED2C ; return
   5. !Move slot 6 (walker: 0) && S+0x8E (airborne) → ChangeState(WaitFinishFalling)         0x8019EDD4
cur.Bind; cur.Update (0x8019EB24); AI slot 0xA (nop)
```

### 5.3 AI slots used as transition requests (GunSoldier vtable 0x8053A108)
| Slot | Implementation | Effect |
|---|---|---|
| 0x0C | `GunSoldierAI::vf0C` 0x801885CC | rescued (E+0x356) → Thanks; AppearType STAND → WaitAction_Standing; LINEAR/TRIANGLE/RANDOM_MOVE → WaitAction_Moving; OFFSETPOS → AppearHome; HINSHI → Hinshi |
| 0x0D | `GunSoldierAI::vf0D` 0x801883E0 | same, but OFFSETPOS → Standing ("return to wait") |
| 0x0F | `GunSoldierAI::vf0F` 0x801882B0 | → GunSoldier Caution (target found) |
| 0x12 / 0x13 | EnemyBaseAI 0x8019D328 / 0x8019D1CC | → Guard / WaitFinishDamaged |
| 0x14 | `GunSoldierAI::vf14` 0x801884E8 | → ToHinshi (instead of base `EnemyBaseAI_Dead`) |
| 0x16 | `GunSoldierAI::vf16` 0x801882E8 | E+0x356 = 1, unlock, → Thanks |
Scripted `CharCommand` ids 0x201..0x20C call AI slots 0x0B..0x16 directly (jump table 0x8053B76C).

### 5.4 States (Enter / Update / Exit, transitions) — PROVEN unless noted
| State (RTTI) | Getter / object | Enter | Update | Exit | Transitions |
|---|---|---|---|---|---|
| `EnemyAIState_SoldierCommonAI_WaitAction_Standing` | inline / 0x805F04FC | 0x801895F4 | 0x80189308 | 0x80189304 | detected → AI 0x0F (Caution) 0x801895C0 |
| `EnemyAIState_SoldierCommonAI_WaitAction_Moving` | inline / 0x805F0508 | 0x801891C4 | 0x80188B98 | 0x80188B58 | detected → Caution 0x80188BDC; patrol between points (STRONG) |
| `GunSoldier::EnemyAIState_GunSoldierAI_Caution` | 0x80186D50 / 0x805F06B0 | 0x80187878 | 0x801877D4 | nop | turn to target; WeaponType 0/1 → ChaseTarget; else on motion flag S+0x3B → `GunSoldierAI_StartCombat` 0x801881A0 (→ Attack) |
| `…_Attack` (ranged) | 0x80188204 / 0x805F06D4 | 0x801872B0 | 0x80186ED0 | 0x80186E80 | aim/fire cycle (timer 3 s, re-aim if off by >30°); **not detected → GoHome** 0x801871D4 |
| `…_ChaseTarget` | 0x801874BC / 0x805F06BC | 0x80187760 | 0x801875F8 | nop | not detected → GoHome; distance < 15 → NearAttack |
| `…_NearAttack` (melee) | 0x801876B4 / 0x805F06C8 | 0x80187568 | 0x80187370 | nop | not detected → GoHome; after attack motion, distance > 20 → ChaseTarget |
| `…_GoHome` | 0x80187204 / 0x805F06E0 | 0x80186DFC | 0x80186C64 | nop | walk to E+0x6C; detected → Caution; XZ distance < 10 → AI 0x0D |
| `…_AppearHome` | 0x801880F4 / 0x805F06EC | 0x80186BDC | 0x80186B48 | nop | walk to home; XZ distance < 10 → StartCombat |
| `…_ToHinshi` | 0x80188520 / 0x805F068C | 0x80188078 (lock, fall anim 0xB/0xC) | 0x80187EE4 | 0x80187EBC | M+0x5F (fell) → SB slot 4 + slot 2 (respawnable despawn); motion end → Hinshi |
| `…_Hinshi` | 0x80187FC0 / 0x805F0698 | 0x80187DB8 (lock, collision 0x200201, anim 9/0xA, timer rand 30‑60) | 0x80187D14 (SE 0x600F on timer) | 0x80187C84 | only via AI 0x16 (healing) |
| `…_Thanks` | 0x80188334 / 0x805F06A4 | 0x80187BCC (S bit 0xD, lock, anim 0xD) | 0x801879A8 | 0x801878E8 | thanks loop while player in area; **if damaged → ChaseTarget** 0x80187B9C |
| `EnemyAIState_EnemyBaseAI_Guard` | 0x8019D3D8 | 0x8019C474 | 0x8019C3D0 | — | → AI 0x0D |
| `EnemyAIState_EnemyBaseAI_WaitFinishDamaged` | 0x8019D27C | 0x8019BE88 (lock, S bit 2) | 0x8019BE18 | 0x8019BDBC | motion end → unlock, AI 0x0D |
| `EnemyAIState_EnemyBaseAI_WaitFinishFalling` / `_Idle` | 0x8019EE18 / 0x8019D970 | 0x8019BD40 / 0x8019CE34 | 0x8019BCD8 / 0x8019CD6C | — | → RevertToPrevious |

Weapon fire (STRONG): the Attack state only plays the fire motion; the motion-event callback `GunSoldierBase::vf18`
0x80197CF4 (Disp slot 0xE) fires on event 0x5000/data 2: `weapon[0] = E+0x358`, `fn_801AB790(weapon)`.

### 5.5 Moving patrol recovered from the original routine

**PROVEN** (`WaitAction_Moving::Enter` 0x801891C4..0x80189300, `Update` 0x80188B98..0x801891A4,
`Exit` 0x80188B58..0x80188B94): the update checks the detection sensor first; a hit requests AI slot 0x0F
and skips all patrol work. On entry it selects point index 1 for LINEAR/TRIANGLE, 3 for RANDOM, or 0 otherwise,
starts motion 0x21, writes 0 to the display movement flag +0x21, and sets E+0x6C from the selected table point.
The table is E+0x288 + index·0x18, `{vec3 position, float WaitSec, float MoveSpeedRatio, int WaitType}`
(`SoldierCommon_ReadSetParams` 0x80198D90..0x80198F04). RANDOM point 3 copies point 0's wait fields and
uses `fn_80198F18` to draw an X displacement in `[-radius,+radius]` and an angle in `[0,pi]`, rotate by Y,
and add E+0x48. Radius is `min(Pos1_WaitSec, MoveRange)` at 0x80198EC8..0x80198EE0.

| AI+0x24 | PROVEN update rule | Addresses |
|---|---|---|
| 0, face | Flatten `(E+0x6C − position)` to XZ. At squared distance <100 clear Disp+0x22; otherwise put the forward-vector dot product in the display rate (negative dot becomes 0.2). Turn toward the point. On completion set substep 1, enable root motion, and set both timer fields to `3 + distance/baseSpeed`. GUN soldier type 0's baseSpeed is 10.0 at 0x804CD4D4. | 0x80188C00..0x80188DB4; speed getter 0x801A6720 |
| 1, walk | Keep turning. If timer <0 or distance <10, enter substep 2 and set display rate 0.2. The subsequent `distance < baseSpeed` rate adjustment at 0x80188E34..0x80188EFC is unreachable for type 0 because baseSpeed=10 and the earlier strict `<10` branch has already returned. | 0x80188DBC..0x80188EFC |
| 2, wait entry | Set rate 0.2; turn toward the **stored spawn yaw** E+0x64 via `fn_80189694` (distinct from the live transform yaw read by `Enemy_GetRotation` 0x8017CA38). Only after the turn completes enter substep 3. Get the current point's WaitType motion, set that motion, copy its WaitSec into both timer fields, and enable root motion. WaitType 0→motion 6, 1→0/1/2 depending on weapon and shield, 2→0x22, 3→weapon-specific attack motion. | 0x80188F00..0x80188FE0; 0x80189694; 0x80197988..0x80197A20; 0x80197B40; 0x80197A50 |
| 3, wait | If timer <0, mode 1 toggles point 0/1; mode 2 cycles 1→2→0→1; mode 3 keeps index 3 and regenerates its random point. Start motion 0x21, set E+0x6C to the next point, and return to substep 0. Otherwise, Status motion flag A repeats the selected wait motion. | 0x80188FE4..0x80189188 |

**PROVEN:** `MoveSpeedRatio` is stored for each point at E+0x298/+0x2B0/+0x2C8/+0x2E0
(0x80198E00..0x80198F00) but is not read by `WaitAction_Moving::Update` 0x80188B98..0x801891A4.
The native `GunSoldierAI::ConfigurePatrol` reads the host-endian 92-byte SET misc block, and `UpdatePatrol`
implements the point, timer, substep and motion-selection rules. The display effects and attack-motion variant
are explicit `GunSoldierBody` engine hooks. **STRONG:** Disp+0x21 controls animation-derived movement because
`EnemyDispCommon::vf08` 0x801A0F78 reads it before extracting an animation displacement. **UNKNOWN:** the wider meaning of Disp+0x22
(`EnemyDispCommon::vf14` 0x8017B9C8) and how display root motion drives terrain-adjusted locomotion
(`EnemyDispCommon::vf0D` 0x801A0EB8). WaitType 1 reads live E+0x270 (shield) at 0x80197B40;
the native `patrolHasShield` mirror must be updated when the engine's shield breaks. A missing SET block leaves
patrol unconfigured; no fabricated points are used.

---------------------------------------------------------------------------------------------------------------------

## 6. Interaction / damage

1. **Callback registration** (STRONG): `EnemyStatusCommon::vf01` → `EnemyCharaColliModule::Setup` 0x801AA010(S+8) builds
   body collision and binds `{0,-1,0x801A978C}` (reaction slot +0x10, contact) and `{0,-1,0x801A9750}` (slot +0x1C,
   damage); argument is `SonicteamUSA::System::CharaColli::AttackCallbackParam&` (bind_t typeinfo 0x805E9318).
2. **`EnemyCharaColliModule::OnDamaged` 0x801A9750**: hit+0x54 = 5 → Status slot 0x1A → `GunSoldierBase::TakeHit` 0x80197700.
3. **`GunSoldierBase::TakeHit`** (PROVEN): AppearType 5 → ignore. Shield present: frontal (<90°) attack with flag 0x40
   → blocked (no damage, S bit 5 → Guard, SE 0x6074); otherwise unless attack flag 0x4 the shield takes the hit
   (break → SE 0x6075, flinch, HaveShield = 0; else SE 0x6074 + guard) **and damage is still applied**; then
   `EnemyStatusCommon::ApplyHit`; E+0x357 = hit from front.
4. **`EnemyStatusCommon::ApplyHit` 0x801A83B0** (PROVEN): unless S+0x76, `EnemyStatusCommon::ApplyDamage` 0x801A8718
   (dmg > 0: S+0x74 = 1, S+0x94 += 1, HP −= dmg, HP ≤ 0 → HP = 0, return 1). Killing hit: S+0x76 = 1, attacker ref
   (hit+0x18) gets `Score_Award(attacker, GunSoldierBase::vf13 = 0x4F or 0x50 if HaveShield)`; if attacker passes
   fn_8021CCDC (LIKELY "is player") → S bit 0xC; S+0x75 = 1. HP crossing 0.5·MaxHP → S+0x75 = 1. Hit effect via
   Disp slot 0x10; S+0x84 = hit side; face attacker when flinching.
5. **Reactions**: S+0x75 → knockback `EnemyMoveCommon::Knockback` 0x801A59C8 (horizontal 50 along ±facing, vertical
   1.3·120) and S+0x8C → AI WaitFinishDamaged; S bit 5 → Guard. (PROVEN mechanism; constants from per-type tables.)
6. **Invulnerability**: injured (AppearType 5), killed latch S+0x76, frontal shield block. No i-frame timer found.
7. **HP**: E+0x18C, max 2.0 (`EnemyParamCommon::vf19` table 0x804CD678[0]); init in `GunSoldierBase::vf0E`
   (HINSHI spawns: HP 0, IsDead/WasDead preset so no death edge fires). Healing via `EnemyStatusCommon::AddHP`
   0x8017BC08 (clamped to MaxHP).
8. `DamageCommand` (CharCommand id 7) is **not** handled by the enemy message handler; damage only arrives via
   CharaColli (PROVEN for `EnemyBaseAI::vf08`).

`EnemyBaseAI::vf08` 0x8019E058 handled CharCommand ids: 0x10 RecoverCommand (heal, §8), 0x14 VacuumCommand (counts
as defeat, injures, SB slot 4 + delete), 0x16 AnnounceAttack (ack), 0x17 SetTransparent, 0x19 Generate → WarpWait,
0x1A Bind → CaptureWait, 0x1B Release → unlock state machine (AI+0x34=0) + AI slot 0x0D ReturnToWait (0x8019E468..0x8019E480; corrected in integration pass), 0x201..0x20C → AI slots 0x0B..0x16.

---------------------------------------------------------------------------------------------------------------------

## 7. HP 0 → injured ("Hinshi") conversion (the GunSoldier "death")

Death edge in `EnemyStatusCommon::vf02` (S+0x90 && !S+0x91, 0x801A8BD8) — PROVEN:
1. `EnemyStatusCommon::ReportLifeEvent(1)` 0x801A8998 (flags |= 4 if S bit 0xC) → `EnemyManager_OnEnemyLifeEvent`
   0x801A250C (see §9).
2. Status slot 0x26 → `GunSoldierBase::BecomeInjured` 0x80198184: collision mask 0x100000; drop shield; throw weapon
   (`fn_801AAC40` → `fn_80221F2C` → 0x80-byte task "ScatteringWeapon" on layer 0xB, the pickup; STRONG), E+0x350 = 0,
   WeaponType = 0; AppearType := 5 and SE 0x600E (once); Disp slot 0x14(0).
3. **Enemy type 0 skips `TEnemySetBase::vf03`** (the SET "killed" detach used by all other enemies) @0x801A8C64/0x801A8C68;
   `fn_801A8D6C(S,0,1)` (collision module off, LIKELY).
4. Same frame AI vf09 → `GunSoldierAI::vf14` → ToHinshi → Hinshi (locked, immune, SE 0x600F every 30‑60 s).
5. `GunSoldier::GunSoldier::WriteTempState` stores STemporaryData state 1 on the SET slot, so a respawn comes back injured.

Contrast: base `EnemyAIState_EnemyBaseAI_Dead` (0x8019D120) requests deletion (SB slot 2) after its motion; GunSoldier
never uses it.

## 8. Healing / Thanks (PROVEN)
`RecoverCommand` (id 0x10, amount +0x1C; constructed only in fn_80108294 of the `HealingUnit::HealingBlast` code):
`AddHP` → effect (LIKELY) + SE 0xE02F → if S bit 0xD clear and first heal of this SET record (record flag
0x01000000 test/set 0x8019E590/0x8019E5A4): `EnemyStatusCommon::AwardRecoveryScore` 0x801A8894 → `Score_Award(healer,
0x57 "GunRecovery")` → AI slot 0x16 → Thanks. Next StatusUpdate sees the revive edge: `ReportLifeEvent(2)` (defeat
count −1, link group +1), S+0x76 = 0, clear S bit 0xC, `GunSoldierBase::OnRevived` 0x8019813C (rescued = 1, AppearType
0), collision on. A rescued soldier that is damaged goes hostile again (Thanks → ChaseTarget).

## 9. Reporting
| Channel | Path | Payload | Conf |
|---|---|---|---|
| Mission counter | `EnemyManager_OnEnemyLifeEvent` 0x801A250C → `EnemyManager_CountDefeat` 0x801A2930 | EnemyManager+0x34[team 0]++ (read by `Mission::EnemyMission::vf0B` 0x801F3CA4 through `EnemyManager_GetDefeatedCount`); revive decrements | PROVEN |
| Link group | same | map EnemyManager+0x10[link id]: alive −1; at 0 → `fn_80169460(fn_800CBA84(), link, 1)` (group cleared event). If the link id is not registered nothing is counted. | PROVEN |
| Player kill | `EnemyManager_CountDefeat` when flags & 4 | +0x40[team]++ and `Player::Npc::EnemyDeadByPlayerCommand` (id 0x403, +0x14 group, +0x18 count; debug text `" Group %d Count %d"`) sent to the player; executed by vf01 0x8007300C → `fn_802F2E20` (partner line every 3rd kill, LIKELY) | PROVEN / LIKELY |
| Score | `Score_Award` 0x802067B0 → `ScoreCommand` (id 0x1C) to attacker/healer; consumed by `Player_HandleCommand` 0x800A7FEC | ScoreData.bin 0x4F "GunSoldier" (150,0,0,3000,0), 0x50 "GunSoldierShield" (200,0,0,3000,0), 0x57 "GunRecovery" (0,0,200,0,3000). w0/w1/w2 → per-player counters 2/0/1, w3/w4 → global gauge 1/0 (Dark/Hero meaning LIKELY) | PROVEN plumbing |
| Stage counts | `EnemyManager_ScanSetEnemies` 0x801A2BA0 @0x801A2C14 skips AppearType 5; `EnemyManager_RecountGroups` 0x801A2A30 skips STemporaryData state 1 | injured soldiers never count as enemies to defeat | PROVEN |
| Drops | weapon → "ScatteringWeapon" pickup; no ring/item drop found | — | STRONG / UNKNOWN (rings) |

`GunSoldier::STemporaryData` (RTTI, base `SetAdapterState`) at SET slot+0x14, created in `GunSoldierBase::vf24`
0x8019886C; state (+4): 0 normal, 1 injured, 2 rescued; read back on respawn (0x801987F0..0x80198864). PROVEN.

---------------------------------------------------------------------------------------------------------------------

## 10. Despawn / destructor (PROVEN)
`TEnemySetTask::Update` 0x801A6EB8: SB+0x14 → kill; else with a slot: `SetSlot_ShouldDespawn` → kill;
`SetSlot_IsOutOfFadeRange` → alpha −= 2·dt, kill at < 0; S+0x6D (hidden) → fade to 0; else fade in; then lifecycle.
Kill = Task flag bit0 → `Task_UpdateChildren` deletes via Task slot 0 (thunk 0x80198D88) → `~GunSoldier` 0x80196F3C:
`GunSoldier::GunSoldier::WriteTempState` → `~EnemyTemplate` 0x80196FE0 → `~TEnemySetTask` 0x801A7034 (`SetSlot_Detach` 0x800C9FD0
unless SB+0x15: record flags &= ~0x6, temp data freed only if flag 0x40000000) → `~GunSoldierAI` →
`~GunSoldierBase` (shield release, weapon delete) → `operator delete`.

`TEnemySetBase` removal slots: 2 `RequestDelete` (SB+0x14 = 1); 3 `TEnemySetBase::DetachKilled` (flag 0x8 + detach +
unlink) — not used by GunSoldier death; 4 `TEnemySetBase::DetachRespawnable` (flag 0x10000 + detach + unlink) — used by
ToHinshi fall-out and VacuumCommand.

Record flags touched by a GunSoldier: TrySpawn &= ~0x80041; Attach \|= 0x2; ctor post-init \|= 0x8; first heal \|=
0x01000000; SB slot 4 \|= 0x10000 & ~0x7; plain removal &= ~0x6. LIKELY: after a plain fade-out removal no re-arm bit
(0x40/0x10000/0x800000) is set, so `SetData_ScanSlots1P` does not respawn that record.

---------------------------------------------------------------------------------------------------------------------

## 11. Open items
1. **Record flag 0x8**: set on every GunSoldier construction (`GunSoldier_MarkSlotFlag8`) and by `TEnemySetBase` slot 3
   ("killed"); checkpoint save converts it to 0x200. It is not a direct spawn veto: the scanner preserves it on explicit
   0x40 re-arm (mask 0x208, 0x800CACAC..0x800CACF4). Full gameplay intent beyond checkpoint persistence is UNKNOWN.
2. **Global byte 0x8057E828** (`.bss 0x8057E760+0xC8`): forces every enemy AI into Idle; also read by player input and
   boss controllers. Writer not found by constant xref. UNKNOWN (LIKELY event/control lock).
3. **Damage values**: the `AttackCallbackParam+0x50` source and most player/weapon values are recovered in
   `WEAPONS_AND_TARGETING.md` (homing attack 2.0 against this soldier's 2.0 MaxHP). SatelliteLaser and a few
   intervals/index paths remain open as listed in `OPEN_QUESTIONS.md` #2.
4. MoveSpeedRatio (param 6 and per-point) consumer outside Moving.Update (that routine does not read the
   per-point values), S+0x64 attachment, S+0x77, second CharaColli callback
   (0x801A978C, hit+0x30), meaning of ScoreCommand kinds (Hero/Dark) — UNKNOWN/LIKELY as noted.

### 11.x Zero-width sensors (added by integration pass)

Census over every placed GUN_SOLDIER record in all stages (`tools/setparse.py`): **54 of 187 have SearchWidth = 0**
(22 STAND, 24 LINEAR_MOVE, 6 TRIANGLE_MOVE, 2 RANDOM_MOVE). With the sensor test `fn_801A6C7C`
(`distXZ² < width²`) such a soldier can never detect the player through the WaitAction states, and the param
init `fn_80198D90` does not write defaults back into the raw record (checked). So these soldiers must be woken by
another path — candidates: AI message 0x205 (`OnTargetFound`) sent by another object/group, damage
(WaitFinishDamaged → ReturnToWait does not engage), or they are intentionally passive. No constant-argument
sender of message types 0x201–0x20C was found (UNKNOWN). Native reconstruction: `src/EnemyAI.cpp`
(`EnemySensor`), `src/GunSoldierAI.cpp`.

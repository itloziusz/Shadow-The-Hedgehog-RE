# ENEMY_BEHAVIOR_INDEX — all enemy & boss families in main.dol

Generated with `tools/enemy_index.py` (→ `data/enemy_index.json`) + RTTI (`data/classes.json`).
Per-family deep dives: `enemy/GUN_SOLDIER.md` (reference trace). Class/state names are ORIGINAL (RTTI);
function roles are recovered.

## 1. Common framework (PROVEN unless marked)

- **Spawn**: SET descriptor create hook (desc+0x0C) → `operator new(size)` → `<Enemy>` ctor(this, 1,
  `TaskManager_GetLayer(14 "Enemy")`, SetSlot*) — see STAGE_GAMEPLAY_DATA_FORMATS.md §5.
- **Resources**: descriptor load hook (desc+0x04) opens `enemy/<Name>Data*.one` as `ResourceOneFile`
  and registers it in the ResourceManager (GUN soldier: slot 7); release hook frees it.
- **Object model** (RTTI base lists): `EnemyTemplate<XBase, XAI>` combining components through
  multiple/virtual inheritance: `EnemyParamCommon`, `EnemyStatusCommon`, `EnemyDispCommon`,
  `EnemyMoveCommon` + `EnemyMoveWalker` | `EnemyMoveFlyer`, `TEnemySetBase`, `EnemyBaseAI` + family AI,
  `TEnemySetTask` (Task), virtual base `EnemyReferer`. Layout example: GAMEPLAY_STRUCTURES.md §4.
- **Per frame**: `TEnemySetTask::Update` 0x801A6EB8 → `TEnemySetBase::LifecycleUpdate` 0x801A7280
  (state 4: Status → AI → Status → Disp → Move → Disp → Status). GAMEPLAY_UPDATE_PIPELINE.md §6.
- **AI**: states are objects of classes derived from `TEnemyAIState` (one class per state).
  Shared states from `EnemyBaseAI`: `None, Idle, Wait, Caution, Guard, GoHome, Dead, Recover,
  WaitFinishDamaged, WaitFinishFalling, WarpWait, CaptureWait` (`EnemyAIState_EnemyBaseAI_*`).
  Intermediate AI layers: `SoldierCommonAI` (`WaitAction_Moving`, `WaitAction_Standing`),
  `EnemyWalkerAI` (`Caution`, `ReturnHome`, `ReturnHomeDash`, `WaitAction_Moving/Standing`),
  `FlyerCommonAI` (`MoveOnPath`, `WaitFloating`). State dispatch mechanics: enemy/GUN_SOLDIER.md.
- **Despawn**: `TEnemySetBase::vf00` → Task kill flag; SET slot despawn/fade tests (0x800C9E4C,
  0x800C9D54); GunSoldier sets SET flag 0x8 at spawn (`SetSlot_SetFlag8`; checkpoint persistence marker, full intent UNKNOWN); its destructor detaches the slot (clears 0x2/0x4). The scanner can explicitly re-arm a record while preserving 0x8 (0x800CACA4..0x800CACF4).

## 2. Standard enemies (SET ids 0x0064–0x0093)

| Id | SET name | Create hook | sizeof | Concrete class | Movement | AI chain → family states | Resource |
|---|---|---|---:|---|---|---|---|
| 0064 | GUN_SOLDIER | 80196E38 | 0x418 | `GunSoldier::GunSoldier` | Walker | EnemyBaseAI→SoldierCommonAI→`GunSoldier::GunSoldierAI`: AppearHome, Attack, Caution, ChaseTarget, GoHome, Hinshi, NearAttack, Thanks, ToHinshi | enemy/GunSoldierData_%s.one (stage-themed) |
| 0065 | GUN_BEETLE | 80193DD4 | 0x470 | `GunBeetle` | Flyer | →FlyerCommonAI→`GunBeetleAI`: Attack, Caution | enemy/GunBeetleData.one |
| 0066 | GUN_BIGFOOT | 80287328 | 0x390 | `GunBigfoot::GunBigfootTask` | Walker | →`GunBigfoot::GunBigfootAI`: GunAttack, HoverAttack, WaitActAttack, WaitCrouching, WaitHovering | enemy/GunBigfootData.one |
| 0068 | GUN_ROBOT | 801952F8 | 0x3B0 | `GunRobotTask` | Walker | →EnemyWalkerAI→`GunRobotAI`: WeaponAttack | enemy/GunRobotData.one |
| 0078 | EGG_PIERROT | 80289B84 | 0x390 | `EggPierrot::EggPierrotTask` | Walker | →`EggPierrot::EggPierrotAI`: BakudanAttack, DashAttack, GoHome, Ottoto, Wait | enemy/EggPierrotData_{horror,busou,pierro}.one |
| 0079 | EGG_PAWN | 8017BD90 | 0x3B0 | `EggPawn` | Walker | →EnemyWalkerAI→`EggPawnAI`: DashAttack, TukiAttack, WeaponAttack | enemy/EggPawnData.one |
| 007A | EGG_SHADOWANDROID | 8029C600 | 0x3E8 | `EggShadowAndroid::EggShadowAndroidTask` | Walker | →`EggShadowAndroid::EggShadowAndroidAI`: Falling, HomingJump, Jump, MissileAttack, RejectHoming | enemy/EggShadowAndroidData.one |
| 008C | BK_GIANT | 8028892C | 0x360 | `BkGiant::BkGiantTask` | Walker | →`BkGiant::BkGiantAI`: ChaseTarget, GunAttack, RejectAttack, StickAttack | enemy/BkGiantData.one |
| 008D | BK_SOLDIER | 8018CAD0 | 0x438 | `BkSoldier` | Walker | →SoldierCommonAI→`BkSoldierAI`: AppearHome, Attack, AttackVacuum, Caution, ChaseTarget, GoHome, NearAttack | enemy/BkSoldierData.one |
| 008E | BK_WINGLARGE | 8018FFF0 | 0x430 | `BkWingLarge` | Flyer | →FlyerCommonAI→`BkWingLargeAI`: Attack, Caution, DeadToAirWing, ResistVacuum | enemy/BkWingLargeData.one |
| 008F | BK_WINGSMALL | 80191640 | 0x420 | `BkWingSmall` | Flyer | →FlyerCommonAI→`BkWingSmallAI`: Attack, Caution, MoveOnPath | enemy/BkWingSmallData.one |
| 0090 | BK_WORM | 801920AC | 0x3A0 | `BkWorm` | Walker | →`BkWormAI`: AfterAttack, Appear, Attack, Move, WaitAppear, WaitStanding | enemy/BkWormData.one |
| 0091 | BK_LARVA | 80189864 | 0x150 | `BkLarvaGenerator` (spawner; larvae = `BkLarva`, `SBkLarvaStateData`) | — | not an EnemyTemplate; native `src/BkLarva.cpp`, evidence in `enemy/BK_LARVA.md` | enemy/BkLarvaData.one |
| 0092 | BK_CHAOS | 802AF70C | 0x3F8 | `BkChaos::BkChaosTask` (+ `BkChibiChaos`) | Flyer | →`BkChaos::BkChaosAI`: AttackRolling, AttackStrech, Caution, Wait, WaitChibi | enemy/BkChaosData.one |
| 0093 | BK_NINJA | 802AC298 | 0x3D8 | `BkNinja::BkNinjaTask` | Walker | →`BkNinja::BkNinjaAI`: AppearHome, GunAttack, MoveDisappear, WaitWarp | enemy/BkNinjaData.one |

Allocation sizes: PROVEN (`li r3,size; bl operator_new` in each create hook). Class: PROVEN (callee is
an RTTI-proven constructor receiving the allocation). Movement/AI chains: PROVEN (RTTI base lists).
Faction (GUN vs Black Arms vs Eggman robots) follows the class/file names; how the game encodes
allegiance at runtime is UNKNOWN (see MISSION_SYSTEM.md for the enemy-count missions).

Shared SET parameters: walkers/soldiers `MoveRange, SearchRange, SearchAngle, SearchWidth, SearchHeight,
SearchHeightOffset, MoveSpeedRatio` + family-specific ones (e.g. GUN_BEETLE `FloatWidth, AttackStart,
AttackEnd, PathMirror, BodyType, WeaponType, SparkDischarge, SparkWait`); full lists in
`data/setobj_catalog.txt`.

### 2.1 Enemy type id (E+0x20), max HP and team (per-type tables)

`Enemy::Enemy` (0x8018F80C) stores the type id passed down each family's base-ctor chain (`li r5,N`); resolved by
constant propagation over the call graph (the chain always ends EnemyRefBase ← TEnemyCommonBase ← Enemy{Walker,Flyer}Common).
Tables indexed by type: max HP `0x804CD678` (f32), team `0x804CD310` (u32: 0 GUN, 1 Eggman, 2 Black Arms),
knockback `0x804CD6B4`, misc `0x804CD4D4` (stride 0x1C) — see enemy/GUN_SOLDIER.md.

| Type | Family | Max HP | Team | Type-id evidence |
|---:|---|---:|---|---|
| 0 | GunSoldier | 2 | GUN | `li r5,0` @0x80198B40 (PROVEN) |
| 1 | GunBeetle | 2 | GUN | GunBeetleBase → FlyerCommonBase → … → Enemy::Enemy (PROVEN) |
| 2 | GunBigfoot | 18 | GUN | ctor → Enemy::Enemy (PROVEN) |
| 3 | GunRobot | 8 | GUN | GunRobot → EnemyWalker → … (PROVEN) |
| 4 | BkGiant | 24 | Black Arms | PROVEN |
| 5 | BkSoldier | 4 | Black Arms | enemy trace (PROVEN) |
| 6 | BkWingLarge | 6 | Black Arms | BkWingLargeBase → FlyerCommonBase → … (PROVEN) |
| 7 | BkWingSmall | 2 | Black Arms | BkWingSmallBase → FlyerCommonBase → … (PROVEN) |
| 8 | BkWorm | 12 | Black Arms | PROVEN |
| 9 | (BkLarva — LIKELY; not an EnemyTemplate) | 1 | Black Arms | no constant-arg ctor path found |
| 10 | BkChaos | 1 | Black Arms | PROVEN |
| 11 | BkNinja | 16 | Black Arms | PROVEN |
| 12 | EggPawn | 4 | Eggman | EggPawnBase → EnemyWalker → … (PROVEN) |
| 13 | EggPierrot | 10 | Eggman | PROVEN |
| 14 | EggShadowAndroid | 8 | Eggman | PROVEN |

HP units: GunSoldier reaches its injured Hinshi state after 2.0 damage. Attack damage values recovered from
`CharaColliAttack+0x34` are in `WEAPONS_AND_TARGETING.md`; the remaining exceptions are listed in `OPEN_QUESTIONS.md`.

## 3. Bosses (SET ids 0x00B4–0x00BE)

Different architecture: `BossRootTask`-derived objects driven by `*CtrlUnit` components and
`*ModeCtrlUnit_<MODE>` state classes (e.g. `BlackBull0210ModeCtrlUnit_{START,WAIT,CHASE,ATK_BREATH,
ATK_CLOSERANGE,ATK_WAVERING,GET_ATK,GET_COMBO,DIZZY,BELOW_HALF,ESCAPE,DEATH,EDIT,MOVETEST}`).

| Id | SET name | Create hook | sizeof | Class | Resource |
|---|---|---|---:|---|---|
| 00B4 | BLACK_BULL_0210 | 80233030 | 0x1C8 | `BlackBull0210` | enemy/boss/BlackBull0210One.one |
| 00B5 | BLACK_BULL_0412 | 80243E64 | 0x1F0 | `BlackBull0412` | enemy/boss/BlackBull0412One.one |
| 00B6 | HEAVY_DOG_0410 | 8024F058 | 0x238 | `HeavyDog0410` | enemy/boss/HeavyDog0410One.one |
| 00B7 | HEAVY_DOG_0510 | 8025447C | 0x240 | `HeavyDog0510` | enemy/boss/HeavyDog0510One.one |
| 00B8 | BLACK_DOOM | 8028390C | 0x2C0 | `BlackDoom` | enemy/boss/BlackDoomOne.one |
| 00B9 | EGG_MECA_0310 | 802EB3A8 | 0x288 | `EggMeca310` | enemy/boss/EggMecaOne.one |
| 00BA | EGG_MECA_0411 | 802EAD84 | 0x288 | `EggMeca411` | enemy/boss/EggMecaOne.one |
| 00BB | EGG_MECA_0511 | 802EA2DC | 0x288 | `EggMeca511` | enemy/boss/EggMecaOne.one |
| 00BC | (unnamed; Diablon) | 802D9108 | 0x40 | (`Boss::Diablon::*`, ctor not resolved) | enemy/boss/diablon.one |
| 00BD | EGG_LASTMECH | 802FE428 | 0x38 | `EggLastMech::EggLastMechTask` (`EggLastMech::AIState::*`) | enemy/boss/EggLastMechData.one |
| 00BE | DEVIL_DOOM | 8030DBBC | 0x670 | `DevilDoom` | enemy/boss/DevilDoomOne.one |

## 4. Status of behaviour recovery

| Family | Spawn/init | AI states | Damage | Death/despawn | Doc |
|---|---|---|---|---|---|
| GUN Soldier | PROVEN | PROVEN (9 states + native moving patrol in `src/GunSoldierAI.cpp`; engine motion hooks remain) | PROVEN (HP E+0x18C, shield) | PROVEN (HP 0 → injured "Hinshi", never deleted by damage; fade/range despawn) | enemy/GUN_SOLDIER.md |
| BK Soldier | PROVEN | PROVEN (AppearType 5 = WARP; weapon 6 VACUUM_POD → AttackVacuum; partial native decision states in `src/BkSoldierAI.cpp`) | PROVEN (HP 4) | PROVEN (EnemyBaseAI_Dead → explode ~0.5 s → delete; record flag 0x8, never re-armed); native death effect remains hook | enemy/BK_SOLDIER.md |
| Egg Pawn | PROVEN | PROVEN (partial native `src/EggPawnAI.cpp`: walker and Dash/Tuki/WeaponAttack decisions; movement/motion hooks remain) | PROVEN | PROVEN (as BK Soldier); native death effect remains hook | enemy/EGG_PAWN.md |
| GUN Beetle | PROVEN | PROVEN (partial native `src/GunBeetleAI.cpp`: float/path firing decisions; EnemyPath.one geometry remains hook) | PROVEN (HP 2/4 by BodyType; spark discharge) | PROVEN (flyers explode + delete same frame); native effect remains hook | enemy/GUN_BEETLE.md |
| GUN Robot | PROVEN | PROVEN (partial native `src/GunRobotAI.cpp`: walker dispatch and weapon-attack steps; shared base Caution, motion and movement remain hooks) | PROVEN (HP 8) | PROVEN (death callback effects without a state change in this function); native effect remains hook | enemy/GUN_ROBOT.md |
| BK WingSmall | PROVEN | PROVEN (partial native `src/BkWingSmallAI.cpp`: path/floating selection, attack window and Caution; attack movement remains hook) | PROVEN (HP 2) | PROVEN (flyer death requests deletion same frame); native effect remains hook | enemy/BK_WINGSMALL.md |
| BK Worm | PROVEN | PROVEN (partial native `src/BkWormAI.cpp`: six-state buried/attack/move loop; terrain, motion and RNG remain hooks) | PROVEN (HP 12) | shared framework; family-specific effects still UNKNOWN | enemy/BK_WORM.md |
| all others | PROVEN (create hook, size, class) | names only (RTTI) | — | shared framework | — |

Alert paths (all families): the search-area sensor is the only alert source found — enemies with SearchWidth 0 are
passive (GUN Soldier 54, BK Soldier 19, Egg Pawn 3, GUN Beetle 13 placements). See enemy/ENEMY_ALERT_PATHS.md (STRONG).

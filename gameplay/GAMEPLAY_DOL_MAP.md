# GAMEPLAY_DOL_MAP — structural map of `sys/main.dol`

Generated from the toolkit in `tools/` (`program.py`, `dataflow.py`, `rtti.py`, `classify.py`).
Machine-readable companions: `data/func_families.json` (every function → family, confidence, evidence),
`data/family_runs.txt` (contiguous runs), `data/region_classes.txt` (RTTI classes per run),
`data/classes.json` (all RTTI classes/vtables).

Confidence tags: **PROVEN** direct code/data evidence · **STRONG** several consistent indicators ·
**LIKELY** single indirect indicator · **UNKNOWN**.

---

## 1. Executable layout (PROVEN — DOL header, `tools/dol.py`)

| Section | Address range | Size | Role / evidence |
|---|---|---:|---|
| T0 `.init` | 80003100–80005600 | 0x2500 | entry `__start` 0x80003154; memset 0x8000540C, memmove 0x800054F4 |
| T1 `.text` | 80008D40–804AAC60 | 0x4A1F20 | all game + library code (≈1.21 M instructions, 34 558 functions) |
| D0 `extab` | 80005600–80007520 | 0x1F20 | C++ EH tables |
| D1 `extabindex` | 80007520–80008D40 | 0x1820 | 512 × {func, size, extab} — covers only MSL/runtime (0x803A…) |
| D2 `.ctors` | 804AAC60–804AB0E0 | 0x480 | static-init functions (per translation unit) |
| D3 `.dtors` | 804AB0E0–804AB100 | 0x20 | |
| D4 `.rodata` | 804AB100–8051D520 | 0x72420 | strings incl. RTTI class names, float tables |
| D5 `.data` | 8051D520–8056FE00 | 0x528E0 | vtables, base-class lists, SET object catalog, param schemas |
| `.bss` | 8056FE00–805E4500 | | global game state (e.g. **0x8057E760** stage/game block) |
| D6 `.sdata` | 805E4500–805EF020 | 0xAB20 | RTTI typeinfo objects; r13 = **0x805EC500** |
| `.sbss` | 805EF020–805F2780 | | singletons' storage (e.g. SetManager 0x805EF77C) |
| D7 `.sdata2` | 805F2780–805FC540 | 0x9DC0 | float constants; r2 = **0x805FA780** |

r1/r2/r13 established by `__start` (0x80003324–0x80003338: `lis/ori r1,0x8060C5F0`, `r2,0x805FA780`, `r13,0x805EC500`).

## 2. How functions were found (PROVEN — `program.py`)

Start evidence counts: `bl` targets 23 757 · data→code pointers following a terminator 9 806 (vtable slots,
callback tables) · lis/addi code pointers 1 895 · extabindex 511 · gap-after-reachable-end 55.
Function extents by recursive descent within [start, next start); 176 MWCC switch jump tables resolved.
Direct call edges: 122 448; indirect call sites: 6 732 (most are vtable calls, see §5).

## 3. Original-name evidence available in the binary (PROVEN)

1. **MWCC RTTI** — 2 170 unique class names (≈4 900 typeinfo objects; header-defined classes are duplicated per TU).
   Layout: typeinfo `{char* name; Base* bases}`; bases `{typeinfo*, s32 offset}…{0}`; vtable
   `{typeinfo*, s32 this_adjust, slots…}`. Verified on `Weapon::Bullet::Cannon` (typeinfo 0x805E4ABC,
   vtable 0x8051EAD0, dtor 0x80053BD8 writes 0x8051EAD0 to `this+0`).
2. **Anonymous-namespace TU names** — e.g. `@unnamed@PlayerShadowWeaponGun_cpp@`, `@unnamed@EnemyVacuumObject_cpp@`,
   `@unnamed@SetCharacter_cpp@` (38 source files, list in `data/strings_raw.txt`).
3. **Task/layer names** — `Task+0` holds a name pointer; layer names at 0x8053ED20 ("System" … "Camera").
4. **SET object catalog** — 286 object type names with parameter names/ranges (0x8052C1A0; see
   `STAGE_GAMEPLAY_DATA_FORMATS.md`).
5. **Library version strings** — Dolphin SDK (OS/VI/DSP/GX/SI, 2004), MetroTRK 2.6, CRI ADX/SFD (Feb 2005).

No original *function* names exist; every method name in this project is a recovered semantic name.

## 4. Library / runtime regions (excluded from gameplay recovery)

| Range | Family | Confidence | Evidence |
|---|---|---|---|
| 80003100–80005600 | runtime startup | PROVEN | `.init` section |
| 8035C0B4–80371348 | Chao CSD UI library (`csd*.cpp`) | PROVEN | `csdLoader.cpp`, `csdmScene.cpp`, `libcsd_rw ASSERT` strings |
| 80371348–8039B0B4 | Dolphin SDK (OS, DVD, VI, DSP, GX, …) | PROVEN | `<< Dolphin SDK - OS/VI/DSP/GX` version strings, `DVDConvertEntrynumToPath`, `OSResetSystem` |
| 8039B0B4–803A1258 | MetroTRK debugger stub | PROVEN | "MetroTRK for GAMECUBE v2.6" |
| 803A1258–803B50A0 | MSL C / C++ runtime | PROVEN | extabindex entries; `std::exception`, strtod tables; operator new 0x803A1380 / delete 0x803A1334; strcmp 0x803AD96C; sprintf family 0x803AA248/0x803AA4AC |
| 803B50A0–803B80CC | Dolphin SDK (SI, …) | PROVEN | `<< Dolphin SDK - SI` |
| 803B80CC–8040C040 | CRI middleware (ADX, ADXF, SJ, SFD/MPV Sofdec movie, CFT) | PROVEN | CRI version + `E0xxxx` error strings |
| 8040C040–804226BC | unclassified library code (bone/anim helpers: "Cannot find bone data.") | UNKNOWN | |
| 804226BC–8043AB68 | Sega effect library `Effect::System::*` | STRONG | RTTI (`Effect::System::EffectArranger`, `EffCmd*Exec`) |
| 8043B618–80440040 | Sega GCAX audio library | PROVEN | "AX SDLIB", `gcaxSndMemory.c` |
| 80440040–80472350 | unclassified (RenderWare toolkits / plugins likely) | UNKNOWN | |
| 80472350–804AAC48 | RenderWare Graphics core + GC driver | STRONG | `rwRASTERFORMAT*` raster messages, RW async file layer |

## 5. Game code: gameplay regions (0x80008D40–0x8035C0B4)

MWCC links translation units contiguously; the runs below are RTTI-anchored (STRONG) with gaps filled from
same-family neighbours (LIKELY). Counts are functions.

| Address range | Family | Key classes (original RTTI names) |
|---|---|---|
| 80008D40–8000DF5C | camera core | `SonicteamUSA::System::Camera::CameraMode{MtnViewer,Stop,User}`, `CameraManagerUnit`, `CameraShake` |
| 800122A0–800172F0 | resource / file | `ResourceManagerTask` (singleton getter 0x800122D4), `Resource`, `ResourceOneFile` |
| 800273E4–80028C94 | effects glue | `SonicteamUSA::System::Effect::*USA` |
| 80028C94–80037948 | collision primitives | `SonicteamUSA::System::Range::Range{Sphere,Box,Capsule,Cylinder,Plane,Triangle}`, `CharaColliBody` |
| 8003AAE8–80043724 | block manager, CSD wrappers | `TBlockManage`, `TCsdSystem`, `TPjsCsd` |
| 80044344–800477A8 | streaming/loading | `FadeTask`, `TBGReadManager`, `TFileControlAsycManager`, `LandDispManager`, `LandALoadMananger` |
| 8004AC78–8004D2C8 | ONE archive async loaders | `TOneFileAsync<RpClump/RpWorld/RtDict/RwTexDictionary/RpDMorphAnimation/void>` |
| **8004EA3C–80050FF4** | **task system** | `Task` (ctor 0x8004F014, dtor 0x8004EF18, scheduler 0x8004ECAC) |
| 80052368–80069BA0 | **weapons & bullets** | `Weapon::Bullet::{BulletBase,Cannon,CannonBomb,BulletGun,Homing,Ring,BulletManagerTask}`, `Weapon::{WeaponBase,WeaponGunBase,WeaponCannonBase,WeaponLockonBase,WeaponStickBase}` + 20 concrete weapons, `Weapon::LockonMark`, `TargetSearchList` |
| 8006C1B8–80076274 | NPC framework | `Player::Npc::{Npc,NpcHand,NpcCommand,EnemyDeadByPlayerCommand,ShowCommand,HideCommand}`, `SonicTypeExecuter` |
| 8006D274–80070588 | player control & commands | `Player::Control::{ControlBase,Approach}`, `CharCommand`, `Player::{PlayerCommand,StartAttackCommand,HomingAttackCommand,EndJumpCommand}` |
| **8007A6E8–800932B8** | **player behaviors (748 fns)** | `Player::Behavior::{AirAttack,ChaosBlast,ChaosControl,Damage,DarkSpin,Dead,DriveVehicle,EdgeHang,Fall,Fly,Grind,Ground,GroundAttack,HomingAttack,HomingJump,Idle,Jump,JumpDash,Landing,Launch,LightDash,LineHang,Ottotto,PickupObject,PickupWeapon,Sliding,SpinDash,Stream,Super,Throw,TurnOver,WallHang,WallJump,…}` |
| 80099A24–800A290C | player commands/controller | `Player::{ResetCommand,ToSuperCommand,…}`, `Player::Controller`, `DeadCommand`, `ClrVelocityCommand` |
| 800AD7E0–800B85E0 | Shadow-specific player | `Player::PlayerShadow`, `Player::Shadow::{ShadowBehavior,ShadowExecuter,ShadowMotion,PlayerTarget}` |
| 800BD4B8–800C05D0 | HUD gauges | `TGiHdGage`, `TGiHissatu`, `TGiHPGaugeBoss`, `GiHPGaugeEnemy` |
| 800C0BB0–800C9560 | character collision | `SonicteamUSA::System::CharaColli::{CharaColliBody,CharaColliGeneral,CharaColliGeneralBone,*CharInfo}` |
| **800C9A04–800CC3B0** | **SET system (stage object layout, spawning)** | `SetManagerTask`, `SetAdapterState`; loader 0x800CBC54, spawner 0x800CB97C |
| 800CC3B0–8015F9BC | **stage objects / gadgets** (≈3 000 fns) | `Container::*`, `GoalRing`, `RegularRing::Ring`, `Spring`, `Switch`, `SavePoint`, `Rocket`, `Door::*`, `Elevator`, `Fan`, `Weight`, `WarpHole`, `Balloon`, `FallBuilding`, `StonePole*`, `Tornado`, `Elec*`, `Monster`, `ThreatObj`, `Catapult`, `BreakWall`, … |
| **8016AE74–8016F2A4** | **mission core** | `Mission::{Mission,CountMission,RingMission,EnemyMission,…}` shared_ptr impls, `Mission::MissionManagerTask` |
| 80170A50–8017B180 | stage manager, boss base | `StageManager` (stage init 0x801783D4), `BossCtrl`, `BossMotion`, `BossTarget` |
| **8017B8C0–801A9750** | **enemy framework + standard enemies** | `EnemyBaseAI`, `TEnemyAIState`, `EnemyAIState_EnemyBaseAI_*`, `TEnemySetBase`, `TEnemySetTask`, `EnemyStatusCommon`, `EnemyDispCommon`, `EnemyMove{Common,Walker,Flyer}`, `EnemyParamCommon`, `GunSoldier::*`, `GunBeetle`, `GunRobot`, `BkSoldier`, `BkWing{Large,Small}`, `BkWorm`, `BkLarva`, `EggPawn` |
| 801AAA94–801B9958 | vehicles | `EnemyWalker`, `EnemyWeapon`, `Vehicle::{Walker,AirSaucer,TVehicleBase,TVehicleCar,VehicleTask,BatteryArmy}`, `BirdBase::AirWing` |
| 801C0E80–801D0250 | stage camera modes | `PJSCamera::CameraMode*`, `ExternalCameraRequest` |
| 801D4EDC–801D6CB4 | audio manager | `AudioManagerTask`, `IAudio` |
| 801DABA4–801DC1A4 | char-command objects | `SpringCommand`, `ScoreCommand`, `BindCommand`, `GenerateCommand`, `AnnounceAttackCommand` |
| 801E2210–801EB790 | frame loop, task manager | frame step 0x801E2210, task-manager ctor 0x801EB734, layer getter 0x801EB614 |
| 801F3C00–801F3F10 | goal/enemy missions | `Mission::GoalMission`, `Mission::EnemyMission` |
| 801FE24C–80203750 | HUD | `TGiCommandWindow`, `TGiScoreDisplay` |
| 802046A8–80206680 | stage flow actions | `StageAction::{Init,Play,Pause,Dead,Continue,Goal,End,Event,Restart,Save,Show,Timeup,TryAgain}Action` |
| 8020F780–8021AFD8 | partner NPCs | `Player::Npc::Knuckles::NpcKnuckles` … |
| 8023A584–8023E028 | Shadow weapon control | `Player::Shadow::{ControlWeaponBase,ShadowWeapon}` |
| 80231E8C–80313D04 (interleaved) | bosses | `BlackBull*`, `HeavyDog*`, `BlackDoom*`, `EggMeca*`, `DevilDoom*`, `Boss::Diablon::*`, `EggLastMech::*` |
| 80287320–802B43F4 (interleaved) | advanced enemies | `GunBigfoot`, `EggPierrot`, `BkGiant`, `EggShadowAndroid`, `BkNinja`, `BkChaos` |
| 802B4868–802B54F4 | mission HUD target | `GiMissionTarget` |
| 802BA34C–802C7F64, 80341934–803432B8 | menus / story | `AdvReal::*::Phase`, `AdvReal::StaffRoll::*` |
| 802C8B14–802C946C | timer missions | `Mission::TimerMission`, `Mission::TimerGoalMission`, `DebugMissionClearCollision` |
| 802D0554–802D3454, 803516F4–80355708 | stage sequencing | `StageSequence::*::Phase`, `StageBase::Base`, `StageRouteDisplay`, `StageAction::{Result2P,Timeup,Flashback}Action` |
| 802E4204–802E61D8, 8035A440–8035BE5C | save/progression | `SaveCycle::{CycleTask,Check,CheckModule,Load,Save}::Phase` |
| 802EBB60–802EFCCC | mission messages | `Mission::{BasicMessage(Manage),CountMessage(Manage),TimerMessage(Manage)}`, `Mission::TimerCountMission` |
| 8034EDB4–8034FFD8 | rumble | `Vibration::{Request,RequestWithPos,Server,ServerGC}` |

Totals from `classify.py` (all 34 558 functions): stage_object 5 437, enemy 2 670, boss ≈2 000, player ≈1 400,
npc ≈650, weapon ≈500, collision ≈470, mission ≈320, camera ≈370, vehicle ≈320, libraries ≈4 200,
unknown ≈12 700 (mostly small unnamed helpers/template instances inside the runs above).

## 6. Gameplay anchors (entry points for further work)

| Address | Current name | Subsystem | Confidence | Evidence |
|---|---|---|---|---|
| 80049078 | MainLoop | engine | STRONG | only caller of frame step |
| 801E2210 | Game_FrameStep | engine | PROVEN | pad read, PAL 50/NTSC 60 select, calls root task update |
| 8001C4F8 | TaskManager_Get | task | PROVEN | lazy static at bss 0x80571C6C, ctor 0x801EB734 |
| 801EB614 | TaskManager_GetLayer(mgr, i) | task | PROVEN | returns `*(mgr+4+4i)` |
| 8004ECAC | Task_UpdateChildren(task, dt) | task | PROVEN | calls vtable slot 1 with f1=dt; deletes killed tasks |
| 8004F014 / 8004EF18 | Task::Task / Task::~Task | task | PROVEN | vptr store 0x8051E768 |
| 801783D4 | StageManager_InitStage | stage | STRONG | creates Set/Command/CharaColli/Effect tasks per layer |
| 800CBC54 | SetData_LoadStage | SET | PROVEN | "setid.bin", "stg%04d/stg%04d_{cmn,nrm,hrd,ds1}.dat" |
| 800CB1A4 | SetData_LoadFile | SET | PROVEN | "sky2" check, byteswap "ffffffiisccii" |
| 800CB97C | SetManager_Update | SET | PROVEN | per-frame spawn scan |
| 800CA8D8 | SetSlot_TrySpawn | SET | PROVEN | calls desc+0x0C create hook |
| 800CA970 | SetSlot_InRange | SET | PROVEN | distance² (to camera unit position) ≤ (u8 range·100)² |
| 80196E38 | GunSoldier_Create | enemy | PROVEN | new(0x418) + GunSoldier ctor |
| 801A6EB8 | TEnemySetTask::Update | enemy | PROVEN | Task slot 1 of every enemy |
| 801A7280 | TEnemySetBase::LifecycleUpdate | enemy | PROVEN | state machine 1→4 |

Further detail: `GAMEPLAY_UPDATE_PIPELINE.md`, `GAMEPLAY_STRUCTURES.md`, `STAGE_GAMEPLAY_DATA_FORMATS.md`,
`enemy/GUN_SOLDIER.md`, `MISSION_SYSTEM.md`, `GAMEPLAY_SYMBOLS.csv`.

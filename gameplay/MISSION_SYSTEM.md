# MISSION_SYSTEM — Shadow the Hedgehog (GC) mission framework

Scope: how a stage's Dark / Normal / Hero missions are configured, created, updated, completed or failed, and how the
result selects the next stage. Everything is derived from `sys/main.dol` with the Python toolkit in `gameplay/tools/`
(`q.py dis/callers/class/xref`), plus `files/nukkoro.inf`, `files/nukkoro2.inf` and the SET files.

Conventions
- **Confidence:** PROVEN (direct code/data evidence), STRONG (several consistent pieces), LIKELY (one indirect piece), UNKNOWN.
- **Names:** class names (`Mission::EnemyMission`, `StageAction::GoalAction`, `EnemyStatusCommon`, `TGameDefaultParam`, …) are
  ORIGINAL (MWCC RTTI or boost template strings). Every method/function name below that is not `fn_XXXXXXXX` or `Class::vfNN`
  is a **semantic guess** made for this document; the positional RTTI name is given next to it. The proposed names are
  collected in `notes/mission_symbols.csv`.
- **Virtual-call notation:** `[[obj]+off]` = slot `(off-8)/4` (the MWCC vptr points at `&vtable.typeinfo`; see `tools/README.md`).
- Parameter tables for every stage: `MISSION_PARAMETER_MAP.md`. Working notes: `notes/mission_trace.md`.
- Helper scripts: `tools/agent_mission_table.py` (parameter table), `tools/agent_mission_enemycount.py` (per-team SET enemy
  census), `tools/agent_mission_funcs.py` (function listing / icall scan).

---------------------------------------------------------------------------------------------------------------------

## 1. Overview

```
 BOOT        fn_80048EFC (RW init cb) -> fn_801E258C -> TGameDefaultParam loads nukkoro.inf, nukkoro2.inf
             MISSIONCOUNT_{D,N,H,HARD} lines -> MissionManager descriptor map {type,param,count,failCount}
 STAGE INIT  fn_801783D4 -> fn_8016E4EC  MissionManager::InitStage: 3 missions via factory fn_8016B768
                          -> fn_801A2CFC  EnemyManager::InitStage: SET enemy census
 FRAME       Task tree -> MissionManagerTask::vf01 -> fn_8016DD58 -> per mission vslot 0xF (Update)
 EVENTS      enemy death -> EnemyManager defeated[team]++ ; objects -> AddCount ; goal ring / clear collision -> Clear
 RESULT      Mission::Clear -> StageState request 6 (GoalAction) -> cleared slot -> next stage = table+0x1C+0xC*slot
             Mission::Fail  -> "MissionFailed" CSD ; all failed -> request 15 (TimeupAction)
```

---------------------------------------------------------------------------------------------------------------------

## 2. Classes (RTTI, ORIGINAL names)

| class | bases (offset) | vtable(s) | ctor | size (factory `new`) | GetType |
|---|---|---|---|---|---|
| `Mission::Mission` | – | 0x8052C8E8 (16) | 0x8016B70C | 0x40 | 0 |
| `Mission::GoalMission` | Mission @0 | 0x8053F54C | 0x801F3C60 | 0x40 | 1 |
| `Mission::CountMission` | Mission (virtual) @0x58 | 0x8052C86C (11), 0x8052C8A0 (16, Mission view) | 0x8016B180 | 0x9C | 2 |
| `Mission::EnemyMission` | CountMission @0, Mission (virtual) @0x5C | 0x8053F5B8 (12), 0x8053F5F0 (16) | 0x801F3D7C | 0xA0 | 3 |
| `Mission::TimerMission` | Mission (virtual) @0x3C | 0x80558EA4 (8), 0x80558ECC (16) | 0x802C93A8 | 0x80 | 4 |
| `Mission::TimerGoalMission` | TimerMission @0, Mission @0x3C | 0x80558E28, 0x80558E50 | 0x802C90EC | 0x80 | 5 |
| `Mission::TimerCountMission` | CountMission @0, TimerMission @0x58, Mission @0x94 | 0x8055AC3C, 0x8055AC70, 0x8055AC9C | 0x802EBD68 | 0xD8 | 6 |
| `Mission::RingMission` | CountMission @0, Mission @0x58 | 0x8052CAB0 (12), 0x8052CAE8 (16) | 0x8016F1C4 | 0x9C | 7 |
| `Mission::MissionManagerTask` | Task @0 | 0x8052CA7C (2) | inline in fn_8016E4EC | 0x28 | – |
| `Mission::Message`, `BasicMessage`, `CountMessage`, `TimerMessage` | Message @0 | 0x8055ADC0, 0x8055AD4C, 0x8055ADB0, 0x8055AE20 | – | – | – |
| `Mission::MessageManage`, `BasicMessageManage`, `CountMessageManage`, `TimerMessageManage` | MessageManage @0 | 0x8055ACF4, 0x8055AD6C, 0x8055ADDC | – | – | – |

GetType values (Mission-view slot 1) are PROVEN (`li r3,N` in 0x8016B31C, 0x801F3C00, 0x8016AE80, 0x801F3C9C, 0x802C9228,
0x802C905C, 0x802EBC24, 0x8016F0D4) and equal the descriptor `type` switched on by the factory (§4.2).
Object sizes: a virtual `Mission` base occupies 0x44 bytes (0x40 + the 4-byte vbase displacement word at Mission+0x40 that the
ctors write, e.g. 0x8016B1FC, and the thunks read, e.g. 0x801F3E28 `lwz r11,0x40(r3)`), so 0x58+0x44 = 0x9C, 0x5C+0x44 = 0xA0,
0x3C+0x44 = 0x80, 0x94+0x44 = 0xD8 — matching the `new` sizes in fn_8016B768 (PROVEN).

### 2.1 Mission-view virtual interface (what the manager and gameplay objects call; `m` = `Mission*`)

| slot | off | Mission default | CountMission / EnemyMission | TimerMission | semantic name (guess) | conf |
|---|---|---|---|---|---|---|
| 0 | +0x08 | dtor 0x8016B6B0 | thunk → dtor | thunk → dtor | destructor | PROVEN (RTTI) |
| 1 | +0x0C | 0x8016B31C → 0 | 2 / 3 | 4 | GetType | PROVEN |
| 2 | +0x10 | 0x8016AE74 → 0 | 0 | 0x802C9054 → 1 | HasTimeLimit | STRONG |
| 3 | +0x14 | 0x8016B60C float const | – | 0x802C904C `lfs f1,8(r3)` | GetTimeLimit | STRONG |
| 4 | +0x18 | 0x8016B624 (idx0: state==1, idx1: state==2) | CountMission::vf02 0x8016B0DC (+0x18/+0x1C) | – | GetCount(idx) | PROVEN |
| 5 | +0x1C | 0x8016B61C → 1 | CountMission::vf03 0x8016B0D4 (+0x8) | – | GetRequired | PROVEN |
| 6 | +0x20 | 0x8016B614 → 1 | CountMission::vf04 0x8016B0CC (+0xC) | – | GetFailCount | PROVEN |
| 7 | +0x24 | nop 0x8016B608 | CountMission::vf05 0x8016B00C | – | AddCount(delta, idx) | PROVEN |
| 8 | +0x28 | nop 0x8016B604 | CountMission::vf06 0x8016AF74 | – | SetCount(value, idx) | PROVEN |
| 9 | +0x2C | nop 0x8016B600 | CountMission::vf07 0x8016AF5C | – | SetRequired(value, idx) | PROVEN |
| 0xA | +0x30 | 0x8016B5F8 → 0 | same | – | unknown query | UNKNOWN |
| 0xB | +0x34 | nop 0x8016B5F4 | same | – | unknown | UNKNOWN |
| 0xC | +0x38 | nop 0x8016B324 | CountMission::vf08 0x8016AF18 | TimerMission (via thunk) | SaveCheckpoint | PROVEN |
| 0xD | +0x3C | nop 0x8016B328 | CountMission::vf09 0x8016AED4 | " | RestoreCheckpoint | PROVEN |
| 0xE | +0x40 | nop 0x8016B32C | CountMission::vf0A 0x8016AE88 | " | Reset | PROVEN |
| 0xF | +0x44 | nop 0x8016AE7C | EnemyMission::vf0B 0x801F3CA4 (thunk 0x801F3E28); RingMission::vf0B 0x8016F0DC | TimerMission::vf07 0x802C92B4 (thunk 0x802C9188) | Update (per frame) | PROVEN |

Data check (PROVEN): `u32(0x8053F5F0+0x44)=0x801F3E28`, `u32(0x8053F5F0+0x24)=0x801F3EA0` (thunk → CountMission::vf05),
`u32(0x8053F5B8+0x20)=0x8016AF74` (EnemyMission primary slot 6 = CountMission::vf06).

---------------------------------------------------------------------------------------------------------------------

## 3. Layouts

### 3.1 `Mission::Mission` (0x40 + 4 when used as virtual base)

| offset | size | field | evidence | confidence |
|---|---|---|---|---|
| 0x00 | 4 | vptr (0x8052C8E8 or derived Mission-view table) | ctor 0x8016B72C | PROVEN |
| 0x04 | 4 | mission key (descriptor map key) | ctor 0x8016B734 (`stw r4`), factory passes key; fn_8016E454 compares 0x1C | PROVEN |
| 0x08 | 4 | slot (0 Dark, 1 Normal, 2 Hero) | ctor 0x8016B738; fn_8016B330 slot→"Dark"/"Hero" | PROVEN |
| 0x0C | 4 | state: 0 running, 1 cleared, 2 failed | fn_8016B688 (=1), fn_8016B654 (=2 unless 1), fn_8016B5B4 | PROVEN |
| 0x10 | 4 | checkpoint copy of state | save fn_8016B554 @0x8016B570, restore fn_8016B4F4 @0x8016B508 | PROVEN |
| 0x14 | 1 | activated flag (mission accepted/active) | set fn_80073BF8 @0x80073F0C, fn_8016E454; read fn_8016E1A8 | PROVEN (flag), meaning STRONG |
| 0x15 | 1 | checkpoint copy of +0x14 | fn_8016B554 @0x8016B578 | PROVEN |
| 0x18 | 0x28 | `Mission::BasicMessageManage` | ctor 0x8016B74C; post fn_802EC3E0(this+0x18, 1/2) | PROVEN |
| 0x40 | 4 | vbase displacement word (only when a virtual base) | CountMission ctor 0x8016B1FC; thunks 0x801F3E28 | PROVEN |

### 3.2 `Mission::CountMission` (Mission is a virtual base at +0x58)

| offset | size | field | evidence | confidence |
|---|---|---|---|---|
| 0x00 | 4 | pointer to virtual base `Mission` (this+0x58) | ctor 0x8016B1BC | PROVEN |
| 0x04 | 4 | vptr (0x8052C86C) | ctor 0x8016B1D8 | PROVEN |
| 0x08 | 4 | required count (success threshold) | ctor 0x8016B200; compared in vf05 0x8016B070, vf06 0x8016AFB8 | PROVEN |
| 0x0C | 4 | fail threshold (0 = never fails) | ctor 0x8016B204; vf05 0x8016B088..0x8016B0B4 | PROVEN |
| 0x10 | 4 | initial count (idx 0) | ctor 0x8016B208; vf0A 0x8016AE98 | PROVEN |
| 0x14 | 4 | initial count (idx 1) | ctor 0x8016B20C; vf0A 0x8016AEA8 | PROVEN |
| 0x18 | 4 | current count (idx 0) | vf05/vf06; vf02 | PROVEN |
| 0x1C | 4 | current fail counter (idx 1) | vf05/vf06; vf02 | PROVEN |
| 0x20 | 4 | checkpoint copy of +0x18 | vf08 0x8016AF30, vf09 0x8016AEE4 | PROVEN |
| 0x24 | 4 | checkpoint copy of +0x1C | vf08/vf09 | PROVEN |
| 0x28 | 0x30 | `Mission::CountMessageManage` | ctor 0x8016B210; vf06 → fn_802EDA58(this+0x28) | PROVEN |
| 0x58 | 0x44 | `Mission::Mission` virtual base | RTTI base offset 0x58 | PROVEN |

### 3.3 `Mission::EnemyMission` (0xA0)

| offset | size | field | evidence | confidence |
|---|---|---|---|---|
| 0x00 | 0x58 | `CountMission` part (vbase ptr → this+0x5C, vptr 0x8053F5B8) | ctor 0x801F3DB4, 0x801F3DF4 | PROVEN |
| 0x58 | 4 | enemy team counted: 0 GUN, 1 Eggman, 2 Black Arms | ctor 0x801F3E10; vf0B 0x801F3CBC | PROVEN |
| 0x5C | 0x44 | `Mission::Mission` virtual base (vptr 0x8053F5F0) | ctor 0x801F3E00 | PROVEN |

Ctor semantics (PROVEN): `EnemyMission(this, constructVbase, key, slot, team, required, initial)` →
`Mission(this+0x5C, key, slot)` (if constructVbase) → `CountMission(this, 0, key, slot, required, 0 /*fail*/, initial, 0)`
(0x801F3DC0..0x801F3DE0: `r7=required, r8=0, r9=initial, r10=0`). **An EnemyMission can never fail by count.**

### 3.4 Other mission classes (fields used by the flow)

| class | offset | size | field | evidence | confidence |
|---|---|---|---|---|---|
| TimerMission | 0x08 | 4 | float time limit (s) | ctor 0x802C9420; GetTimeLimit 0x802C904C | PROVEN |
| TimerMission | 0x0C | – | `Mission::TimerMessageManage` | ctor 0x802C9424 | PROVEN |
| RingMission | (CountMission) | – | required = ring target; count = player rings | ctor call 0x8016BA5C; vf0B 0x8016F0DC → fn_8016F664(player+0x28) | STRONG |
| TimerCountMission | 0x58 | – | TimerMission part (time = desc.failCount as float) | factory 0x8016B9DC..0x8016BA0C | PROVEN |

### 3.5 MissionManager singleton — `.bss 0x80576FBC`
Getter fn_8006D7D0 (lazy; constructs with fn_8016E7B8, registers dtor fn_8016E750). The class has no RTTI name.

| offset | size | field | evidence | confidence |
|---|---|---|---|---|
| 0x00 | 4 | `Mission::MissionManagerTask*` | fn_8016E4EC 0x8016E738; cleared fn_8016DC80 | PROVEN |
| 0x04 | 4 | current stage-table index (−1 when idle) | fn_8016E4EC 0x8016E50C; ctor 0x8016E7EC | PROVEN |
| 0x08 | 8×3 | `boost::shared_ptr<Mission::Mission>` for slot 0/1/2 | fn_8016E4EC 0x8016E5A4; GetMission fn_8007420C (`this+8+8*slot`) | PROVEN |
| 0x20 | 4 | current (displayed/selected) mission slot, default 1 | fn_8016E220 0x8016E238 | PROVEN |
| 0x24 | 4 | checkpoint copy of +0x20 | fn_8016E3DC 0x8016E43C; fn_8016E360 0x8016E3BC | PROVEN |
| 0x28 | 1 | some mission has a time limit | fn_8016E4EC 0x8016E670 | PROVEN |
| 0x29 | 1 | every mission has a time limit | fn_8016E4EC 0x8016E678; suppresses fail CSD in fn_8016B330 | PROVEN |
| 0x2C | 4 | float min time limit; start value of stage timer | fn_8016E4EC 0x8016E64C, 0x8016E6A8 | PROVEN |
| 0x30 | ? | `std::map<int key, MissionDesc>` | fn_8016CB48 fill, fn_8016C7BC find, fn_8016C690 assign | PROVEN |

`MissionDesc` (map node value at node+0x10): `+0 type, +4 param, +8 count, +0xC failCount` — built by fn_8016C8EC
(0x8016C91C..0x8016C934 `{type, param, 1, 0}`) (PROVEN). Stage timer object: `.bss 0x8057E7E8` (game-state block +0x88);
fn_80337D9C writes the limit to +0x14/+0x18/+0x1C, so `.bss 0x8057E800` (+0x18) is the remaining time read by
TimerMission::vf07 (PROVEN writes/reads; "remaining time" meaning STRONG).

### 3.6 EnemyManager singleton — `.bss 0x80580BD8`
Getter fn_80119560 (lazy; ctor fn_801A2EB0). No RTTI name.

| offset | size | field | evidence | confidence |
|---|---|---|---|---|
| 0x10 | ? | map<u16 linkId, {+2: s16 alive count}> (enemy link groups) | fn_801A2BA0 0x801A2CC8; fn_801A250C 0x801A2560; fn_801A3098 | PROVEN |
| 0x20 | 4 | cleared at stage init | fn_801A2CFC 0x801A2D0C | PROVEN |
| 0x24 | 4 | total SET enemies | fn_801A2BA0 0x801A2CD8 | PROVEN |
| 0x28 | 4×3 | enemies placed per team | fn_801A2BA0 | PROVEN |
| 0x34 | 4×3 | **enemies defeated per team** (EnemyMission source) | ++ fn_801A2930 0x801A2958; −− fn_801A250C 0x801A25A8; read fn_801A24FC | PROVEN |
| 0x40 | 4×3 | enemies defeated by the player per team | fn_801A2930 0x801A2968 | PROVEN (count), "by player" STRONG |
| 0x4C | 4×3 | checkpoint copy of +0x34 | fn_801A27B0 0x801A27D8; fn_801A26B8 0x801A26E0 | PROVEN |
| 0x58 | 4×3 | checkpoint copy of +0x40 | same | PROVEN |

Team of an enemy: `EnemyParamCommon::vf00` 0x801A68A0 returns `u32 table 0x804CD310[kind]` with kind = `[[this+4]+0]+0x20`;
table = kinds 0–3 → 0, 4–11 → 2, 12–14 → 1 (PROVEN). Kind ↔ class: LIKELY GUN 0x64/0x65/0x66/0x68 (4), Black Arms
0x8C–0x93 (8), Eggman 0x78–0x7A (3) — the counts match the SET catalog, and the census in §5.1 uses the same team split by SET id.

---------------------------------------------------------------------------------------------------------------------

## 4. Configuration and creation

### 4.1 Parameter sources (details and full tables in `MISSION_PARAMETER_MAP.md`)
- **Stage table** `.rodata 0x804C5AE8` (0x3B × 0x50): per slot `+0x1C+0xC·s` next-stage index, `+0x20+0xC·s` mission key,
  `+0x24+0xC·s` unknown id; `+0x40` hard-mode key (PROVEN).
- **Descriptor defaults** fn_8016CB48 (41 calls to fn_8016C8EC: key 0x00–0x28 → type, param) (PROVEN).
- **Counts** `files/nukkoro2.inf` `MISSIONCOUNT_{D,N,H,HARD} : <success> <fail>` per `[stage name]` section, parsed by
  `TGameDefaultParam` (class name from the boost bind string at 0x804D1EE8) (PROVEN).

### 4.2 Creation — fn_8016E4EC (*MissionManager::InitStage*) (PROVEN)
Called by fn_801783D4 (`StageManager_InitStage`) @0x80178604 with `(MissionManager, task layer 0xF, .bss 0x8057E80C)`.
1. +0x04 = stage index, +0x20 = +0x24 = 1, fn_8016E220(1).
2. If `.bss 0x8057E885` ("hard" flag, also selects `_hrd.dat`): single mission from key `table+0x40`, slot 1, stored at +0x10.
   Else for slot 0..2: `key = fn_80176FC8(index, slot)`; `fn_8016B768(&sp, this+0x30, key, slot)`; store at +8+8·slot.
3. Time aggregation over Mission slots 2/3 → +0x28/+0x29/+0x2C; stage timer start fn_80337D9C(.bss 0x8057E7E8, min−0.001).
   `.bss 0x8057E8AA` = all-timed flag.
4. fn_8016E454: a mission whose key is 0x1C is auto-activated and made current.
5. `new(0x28)` Task (parent = task layer), vptr+0x18 = 0x8052CA7C, +0 = ptr to "MissionManagerTask" (0x804C4660).

Factory fn_8016B768 (*Mission_Create*): `desc = fn_8016C7BC(map, key)` (std::map find; returns node+0x10 or 0); if
`desc.type ≤ 7` jump table 0x8052C930:

| type | target | class | ctor call | descriptor → ctor |
|---|---|---|---|---|
| 0 | 0x8016B7C8 | Mission | 0x8016B7E0 | – |
| 1 | 0x8016B80C | GoalMission | 0x8016B824 | – |
| 2 | 0x8016B850 | CountMission | 0x8016B87C | required = +8, fail = +0xC |
| 3 | 0x8016B8A8 | EnemyMission | 0x8016B8D0 | team = +4, required = +8 |
| 4 | 0x8016B8FC | TimerMission | 0x8016B938 | time = (float)+8 |
| 5 | 0x8016B964 | TimerGoalMission | 0x8016B9A0 | time = (float)+8 |
| 6 | 0x8016B9CC | TimerCountMission | 0x8016BA0C | required = +8, time = (float)+0xC |
| 7 | 0x8016BA38 | RingMission | 0x8016BA5C | required = +8 |

Each object is wrapped in a `boost::shared_ptr` (RTTI `sp_counted_base_impl<Mission::X*, checked_deleter<…>>`).

---------------------------------------------------------------------------------------------------------------------

## 5. Progress

### 5.1 Enemy census — fn_801A2CFC → fn_801A2BA0 (*EnemyManager::ScanSetEnemies*) (PROVEN)
Called at stage init from fn_801783D4 @0x80178724 (and on continue via fn_801A28A8 → fn_801A2A30 recount). For every link id
0..255 (fn_800CB948 list), every SET object with id in [0x64, 0x96):
- 0x64 GUN_SOLDIER → team 0 unless param AppearType (param struct +0x24) == 5;
- 0x91 BK_LARVA → team 2 += param Num (struct +0x1C);
- otherwise team = id < 0x78 ? 0 : id < 0x8C ? 1 : 2.
Updates +0x28[team], +0x24 and the link-group alive count; +0x34/+0x40 are zeroed by fn_801A2E24.

### 5.2 Enemy defeat → EnemyManager (PROVEN)
1. `EnemyStatusCommon::vf02` 0x801A8A38 (per-frame status update): on the frame the enemy becomes dead (`[[this]+0x58]` true,
   `[[this]+0x6C]` false, `[[this]+0xC4]` false) calls `[[this]+0x94]` = `EnemyStatusCommon::vf23(this, 1)` @0x801A8C40;
   on revive `vf23(this, 2)` @0x801A8D04.
2. `EnemyStatusCommon::vf23` 0x801A8998 (*ReportLifeEvent*): if flags==1 and status bit 12 (this+0x80) → flags |= 4
   (STRONG: "defeated by player"); setobj slot = `[[X+0x14]]` slot 9; team = `EnemyParamCommon::vf00`;
   `fn_801A250C(EnemyManager, setobj, team, flags)` @0x801A8A20.
3. fn_801A250C (*EnemyManager::OnEnemyLifeEvent*): group = link id (SET record +0x22 via fn_800CA248); only if the group exists:
   - flags & 2 (revive): defeated[team]−− , group alive++;
   - flags & 1 (death): fn_801A2930, group alive−−; when 0 → `fn_80169460(fn_800CBA84(), linkId, 1)` (link event).
4. fn_801A2930 (*EnemyManager::CountDefeat*): **defeated[team] (+0x34) += 1** unconditionally; if flags & 4: byPlayer[team]
   (+0x40) += 1 and a `Player::Npc::EnemyDeadByPlayerCommand(team, byPlayer[team])` is sent (`[[target+0x38]+0x10]`).
   ⇒ defeats by any cause count toward an EnemyMission.

### 5.3 Per-frame mission update (PROVEN)
`Task_UpdateChildren` 0x8004ECAC → Task slot 1 `Mission::MissionManagerTask::vf01` 0x8016E824 (*Update*) → fn_8016DD58
(*MissionManager::Update*) → per slot fn_8016B5B4 (*Mission::Update*): if state ∉ {1,2} → `[[m]+0x44]`:
- EnemyMission::vf0B 0x801F3CA4 (*SyncFromEnemyManager*): `SetCount(fn_801A24FC(EnemyManager, team), 0)` via `[[this+4]+0x20]`.
- RingMission::vf0B 0x8016F0DC: `SetCount(player ring count, 0)`.
- TimerMission::vf07 0x802C92B4: TimerMessageManage tick fn_802EF79C; if |remaining time `.bss 0x8057E800`| < 0.0001
  (fn_8000A950 = fabs compare) → Fail @0x802C92FC.
The activation flag +0x14 is not tested on this path (PROVEN for fn_8016B5B4).

`CountMission::vf06` 0x8016AF74 (*SetCount*) and `vf05` 0x8016B00C (*AddCount*):
- idx 0: count (+0x18) = value / += delta (clamped ≥ 0); `fn_802EDA58(this+0x28, count)`; **count ≥ required (+0x8) →
  fn_8016B688(vbase Mission)** (Clear).
- idx 1: only if fail threshold (+0xC) ≠ 0: +0x1C updated; ≥ +0xC → fn_8016B654 (Fail).

### 5.4 Other progress / clear sources (PROVEN call sites)
- **AddCount(1, 0)** `[[m]+0x24]` on `fn_8007420C(MissionManager, slot)` (*GetMission*): PowerDeviceCage::vf02 0x800D2AFC,
  PowerDeviceNaked::vf02 0x800D492C, Cream::vf02 0x80134A1C, DefenseSystem::vf02 0x80224570,
  MagmaGuard::SetMagmaGuard::vf02 0x8022B65C, SuperComputer::SetSuperComputer::vf02 0x8022E60C, GiantLantern::vf02 0x80236FFC
  (slot 0), ElecAccessPanel::vf02 0x8024911C / 0x80249194, ElecBomb::vf02 0x802646F4, CityBombBig::vf02 0x802F3C10,
  SecretDisk::SecretDisk::vf02 0x802FA674, EscapePlaneController::Controller::vf01 0x8025F364,
  EggBalloonController::Controller::vf01 0x802745E4, BKTankController::Controller::vf01 0x80323038, fn_800CCBFC 0x800CCD2C,
  fn_8012EB54 0x8012EC78, fn_802F51E4 0x802F52CC, fn_80302E28 (0x80302EE0 AddCount(−1), 0x80302F8C), fn_80319E04 0x80319EB8.
- **Goal ring:** GoalRing contact handler fn_80106824 (dataptr 0x80527278, next to the GOALRING SET descriptor 0x805272CC —
  STRONG) → fn_80107330 → fn_8016DE18 (*MissionManager::OnGoalRing*): clears the first non-failed TimerGoalMission (type 5),
  else the first GoalMission (type 1) (0x8016DEF4 / 0x8016DF50).
- **MissionClearCollision** (SET 0x2595, param "0:DK 1:NM 2:HR" = slot, radius param) `::vf02` 0x80231AEC: player within
  radius → GetMission(slot) → Clear @0x80231BF8, object kills itself. DebugMissionClearCollision::vf02 @0x802C8C9C likewise.
- **Direct Clear/Fail:** ElecCoreProgram::vf02 @0x801F56C0 (Clear, also sets +0x14); Fail from
  EscapePlaneController @0x8025F3F0, EggBalloonController @0x80274670, BKTankController @0x80322F74.

### 5.5 Activation / selection (STRONG)
fn_80073BF8 (*NpcPartner_ActivateMission*), called from `Player::Npc::SonicType::SonicTypeExecuter::vf01`,
`Player::Npc::FlyType::FlyTypeExecuter::vf01`, `Player::Npc::BossSonic::BossSonicExecuter::vf01`: fn_8016E220(slot) and
`GetMission(slot)+0x14 = 1` (@0x80073F0C). `Player::Control::UserInput::vf03` uses fn_8016E1A8 (*IsMissionActivated*: slot 1
always true) and fn_8016E220 to switch the displayed mission.

---------------------------------------------------------------------------------------------------------------------

## 6. Completion, failure, next stage

### 6.1 Clear — fn_8016B688 (*Mission::Clear*) → fn_8016B43C (*Mission::OnClear*) (PROVEN)
state = 1; unless `StageState+8 == 9` (mode 9 meaning UNKNOWN): `fn_802EC3E0(this+0x18, 1)` (BasicMessageManage post
"success", meaning LIKELY) and `fn_801762D0(StageState, 6, 0)`.

### 6.2 StageAction dispatch (PROVEN)
StageState singleton `.bss 0x80577038` (getter fn_80074598). fn_801762D0 (*StageState::RequestAction*): ignored if disabled
(`+0x17+req`) or locked (`+0x14` unless force); else `+0xC = req`. fn_80174974 (*StageState::DispatchAction*) switches on
+0xC via jump table 0x8052CC68:

| req | StageAction class (ORIGINAL) | req | StageAction class |
|---|---|---|---|
| 1 | (no action object) | 9 | DeadAction |
| 2 | InitAction | 10 | SaveAction |
| 3 | PlayAction | 11 | TryAgainAction |
| 4 | EventAction | 12 | ShowAction |
| 5 | PauseAction | 13 | FlashbackAction |
| **6** | **GoalAction** (mission clear) | 14 | Result2PAction |
| 7 | RestartAction | **15** | **TimeupAction** (all missions failed / time up) |
| 8 | ContinueAction | 16 | EndAction |

### 6.3 Next-stage selection (PROVEN unless noted)
- `StageAction::GoalAction::GoalAction` 0x80205ED8 → fn_802053B0 (*GoalAction::DetermineClearedSlot*): slot = 1, then the last
  slot whose mission state == 1 (@0x8020546C); hard/expert path maps save-object (+0x98) values 3/4/5 → 0/1/2; result stored in
  GoalAction+0x10 and `.bss 0x8057E824`.
- `StageAction::GoalAction::vf00` 0x8020554C: "sng_jin_roundclear.adx" jingle; @0x80205D50 fn_80205010
  (*GoalAction::RecordRoute*): appends to the route history in the save object (fn_800BBB34, +0x30 count, +0x34[] indices)
  `next = u32(StageTable[cur] + 0x1C + 0xC*slot)` (@0x802050C4..0x802050D8).
- fn_802D1EC8 (*Story::ResolveNextStage*, from fn_802D2060): result code 3 → +0x1C (Dark), 4 → +0x28 (Normal), 5 → +0x34 (Hero),
  6 → from save +0x98; boss stages (flags bit0) force 6; stores next stage **id** (−2 → −1 = end). Code→slot mapping STRONG.
- Route HUD fn_800BB37C also walks `+0x1C+0xC·i` (skipping boss entries via flags bit0) and prints "%04d".

### 6.4 Fail — fn_8016B654 (*Mission::Fail*) → fn_8016B330 (*Mission::OnFail*) (PROVEN)
Only if state ≠ 1: state = 2; if the failed slot is current → fn_8016E220(1); `fn_802EC3E0(this+0x18, 2)`; unless
MissionManager+0x29: `sprintf("CsdFiles/MissionFailed%s", {_JP,_EN,_FR,_SP}[.bss 0x8057E7B8])` and
fn_80354484(path, "mf", slot 2 ? "Hero" : slot 0 ? "Dark") → `CsdOneShot::CsdOneShot`; fn_801D59AC(fn_8005328C(), 0x1028, 0, 0)
(sound/voice 0x1028, LIKELY). Then fn_8016DDBC: every slot empty or failed → `fn_801762D0(StageState, 15, 0)`.

---------------------------------------------------------------------------------------------------------------------

## 7. HUD and messages
- Counter text: fn_800BC74C → `sprintf("%d/%d", [[m]+0x18](0), [[m]+0x1C]())` (GetCount / GetRequired) (PROVEN strings/calls;
  HUD role STRONG).
- Message managers (vtables 0x8055ACF4 / 0x8055AD6C / 0x8055ADDC; slots 0/1/2 = save/restore/reset, called from the mission
  save/restore/reset paths at `[[+0x18]+8/+0xC/+0x10]`, PROVEN): `BasicMessageManage` post(1|2) fn_802EC3E0 on clear/fail;
  `CountMessageManage` fn_802EDA58 dispatches queued `CountMessage`s whose threshold (+0x14) ≤ count and whose slot == current
  slot; `TimerMessageManage` fn_802EF79C the same for time. Message contents (radio/voice/HUD) — LIKELY, not traced.

## 8. Checkpoint / restart (PROVEN)
| event | mission side | enemy side |
|---|---|---|
| checkpoint touched (fn_80119350) | fn_8016E3DC → fn_8016B554 (state/flag copies, slot 0xC SaveCheckpoint); +0x24 = +0x20 | fn_801A27B0: +0x34/+0x40 → +0x4C/+0x58 |
| ContinueAction::vf00 | fn_8016E360 → fn_8016B4F4 (restore, slot 0xD); +0x20 = +0x24 | fn_801A28A8: restore + regroup fn_801A2A30 |
| RestartAction::vf00 | fn_8016E2D8 → fn_8016B490 (state 0, flags 0, slot 0xE Reset); current = 1; timer restart | (stage re-init) |

---------------------------------------------------------------------------------------------------------------------

## 9. Worked trace — Westopolis (stage 100, "City1") Dark: "defeat 35 GUN soldiers"

| # | step | address(es) | confidence |
|---|---|---|---|
| 1 | Boot: RenderWare init callback (dataptr 0x8051E614) calls the game init | fn_80048EFC @0x80048F4C → fn_801E258C | PROVEN |
| 2 | Config files loaded | fn_801E04EC("nukkoro.inf") @0x801E2680, fn_801E04EC("nukkoro2.inf") @0x801E2690 | PROVEN |
| 3 | Section `[City1]` matched against stage names → TGameDefaultParam+0x2C50 = 5 | fn_801E030C @0x801E03E4..0x801E0414 (name ptr = stage table +4) | PROVEN |
| 4 | Line `MISSIONCOUNT_D : 35 0` dispatched (command table 0x8053E700, entry 21) | fn_801DF3F8 → fn_801DF470(this, slot 0, 0x15) | PROVEN |
| 5 | Two ints parsed (35, 0) | fn_801E0144 / fn_801E01D0 @0x801DF4C0 | PROVEN |
| 6 | Mission key for (stage 5, slot 0) = u32(0x804C5C98) = 1 | fn_80176FC8 @0x801DF4E0 | PROVEN |
| 7 | MissionManager obtained; first use constructs it and inserts defaults (key 1 → {type 3, param 0, 1, 0}) | fn_8006D7D0 → fn_8016E7B8 → fn_8016CB48 @0x8016CB8C | PROVEN |
| 8 | Descriptor for key 1 overwritten with {3, 0, 35, 0} | fn_8016C7BC @0x801DF4F4, fn_8016C690 @0x801DF570 | PROVEN |
| 9 | Stage 100 loads; StageManager init calls mission init with index 5 | StageManager::vf01 → fn_801783D4 @0x80178604 → fn_8016E4EC | PROVEN |
| 10 | Slot 0: key = 1 → factory → type 3 → `new(0xA0)` | fn_80176FC8 @0x8016E58C, fn_8016B768 @0x8016E5A0, jt 0x8052C930[3] | PROVEN |
| 11 | `EnemyMission(key 1, slot 0, team 0 = GUN, required 35)`; Mission +4 = 1, +8 = 0; CountMission +8 = 35, +0xC = 0; Reset → count 0 | 0x8016B8D0 → 0x801F3D7C → 0x8016B70C, 0x8016B180, vf0A 0x8016AE88 | PROVEN |
| 12 | Slots 1/2 built the same way: key 0 → GoalMission, key 2 → EnemyMission(team 2 = Black Arms, required 45) | fn_8016B768 | PROVEN |
| 13 | MissionManagerTask created under task layer 0xF | fn_8016E4EC @0x8016E70C..0x8016E738 | PROVEN |
| 14 | Enemy census: stg0100 cmn+nrm+ds1 → 36 GUN, 45 Black Arms placed | fn_801783D4 @0x80178724 → fn_801A2CFC → fn_801A2BA0; `tools/agent_mission_enemycount.py 0100` | PROVEN |
| 15 | Dark mission activated / selected by the partner NPC (sets +0x14, current slot) | fn_80073BF8 @0x80073E68, @0x80073F0C | STRONG |
| 16 | A GUN soldier is defeated: status update detects death → vf23(1 [|4]) | EnemyStatusCommon::vf02 @0x801A8C40 → vf23 0x801A8998 | PROVEN (path); enemy-side death detection per enemy trace |
| 17 | Team lookup: kind → 0x804CD310 → 0 (GUN) | EnemyParamCommon::vf00 0x801A68A0 (call @0x801A8A08) | PROVEN (table); kind↔class LIKELY |
| 18 | EnemyManager defeated[0] (+0x34) += 1; group alive −1 (link event at 0) | fn_801A250C @0x801A8A20 → fn_801A2930 @0x801A2954 | PROVEN |
| 19 | Next frame: MissionManagerTask update → Mission::Update → vslot 0xF | 0x8016E824 → fn_8016DD58 → fn_8016B5B4 @0x8016B5E0 → thunk 0x801F3E28 | PROVEN |
| 20 | EnemyMission syncs: SetCount(defeated[0], 0); CountMessageManage notified; HUD shows "n/35" | EnemyMission::vf0B 0x801F3CA4 → CountMission::vf06 0x8016AF74 → fn_802EDA58; fn_800BC74C | PROVEN (HUD role STRONG) |
| 21 | 35th defeat: count ≥ required → Clear | vf06 @0x8016AFBC..0x8016AFC8 → fn_8016B688 | PROVEN |
| 22 | OnClear: success message post, request StageAction 6 | fn_8016B43C → fn_802EC3E0(…,1), fn_801762D0(…,6,0) @0x8016B478 | PROVEN |
| 23 | Dispatcher builds `StageAction::GoalAction` | fn_80174974, jt 0x8052CC68[6] = 0x80174AB0 → ctor 0x80205ED8 | PROVEN |
| 24 | Cleared slot = 0 (Dark) → `.bss 0x8057E824` | fn_802053B0 @0x8020546C, @0x802054F0 | PROVEN |
| 25 | Route: next = u32(0x804C5C78+0x1C) = index 6 = stage 200 "Circuit" (Digital Circuit) | GoalAction::vf00 @0x80205D50 → fn_80205010 @0x802050D4; fn_802D1EC8 @0x802D1F58 | PROVEN |
| 26 | Failure path for this mission: none by count (fail threshold forced 0); stage can still end via other slots / TimeupAction | EnemyMission ctor @0x801F3DD8; fn_8016DDBC | PROVEN |

---------------------------------------------------------------------------------------------------------------------

## 10. Open items
- Stage table `+0x24+0xC·slot` (0x227..0x2C5; lead: event/movie id) and `+0x44..+0x4C`: no DOL consumer found — UNKNOWN.
- Stage-table hook pointers `+0x0C..+0x18` (e.g. 0x8016AE14…, only +0xC call site found at 0x80178780) — meaning UNKNOWN.
- Mission slots 0xA/0xB (`+0x30` → 0, `+0x34` nop in all classes seen) — UNKNOWN.
- Where each enemy class sets its *kind* (EnemyReferer target +0x20) — not located; kind↔class mapping LIKELY.
- `EnemyStatusCommon` status bit 12 = "damage from player" — STRONG only by the NPC command it feeds.
- StageState mode value 9 that suppresses OnClear/OnFail — UNKNOWN (possibly 2P / demo).
- Contents of Basic/Count/Timer messages (voice lines, HUD popups) — not traced.
- `fn_802D1EC8` result-code source (vcall `[[x+4]+0xC]` in fn_802D2060) — STRONG mapping only.

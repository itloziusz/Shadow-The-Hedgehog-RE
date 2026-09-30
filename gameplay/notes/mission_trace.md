# Mission trace — Westopolis (stage 100, "City1") Dark mission: EnemyMission "defeat 35 GUN soldiers"

Source: `sys/main.dol` (toolkit `gameplay/tools`), `files/nukkoro.inf`, `files/nukkoro2.inf`, `files/stg0100/*.dat`.
Confidence tags: **PROVEN** (direct code/data evidence), **STRONG** (several consistent pieces of evidence), **LIKELY** (one indirect piece), **UNKNOWN**.
Class names (`Mission::*`, `StageAction::*`, `EnemyStatusCommon`, …) come from RTTI and are ORIGINAL. Function names in
*italics* are semantic guesses made for this note (the DOL has no symbols).

Helper scripts written for this note (read-only, no shared caches touched):
`tools/agent_mission_funcs.py` (range listing, icall scan), `tools/agent_mission_table.py` (parameter table; `rows` mode prints
the table in §9), `tools/agent_mission_enemycount.py` (per-team SET enemy count using the fn_801A2BA0 rules).

---------------------------------------------------------------------------------------------------------------------

## 0. One-screen summary

```
BOOT  RW-init callback fn_80048EFC -> fn_801E258C -> fn_801E04EC("nukkoro.inf") , fn_801E04EC("nukkoro2.inf")
        line parser fn_801E030C: "[City1]" -> TGameDefault+0x2C50 = stage-table index 5
        "MISSIONCOUNT_D : 35 0" -> fn_801DF3F8 -> fn_801DF470(this, slot 0, cmd 0x15)
            key = StageTable[5].slot[0].key (=1)          fn_80176FC8
            desc = MissionManager.map.find(1) = {3,0,1,0} fn_8016C7BC  (defaults from fn_8016CB48)
            map[1] = {3,0,35,0}                           fn_8016C690
STAGE INIT  StageManager::vf01 -> fn_801783D4
        fn_8016E4EC(MissionManager, task, stageIdx=5)
            for slot 0..2: fn_8016B768 factory: desc.type 3 -> new(0xA0) EnemyMission(key 1, slot 0, team 0, required 35)
            new MissionManagerTask (Task, vtbl 0x8052CA7C)
        fn_801A2CFC -> fn_801A2BA0  EnemyManager (.bss 0x80580BD8) scans SET: team totals + link-group counts
GAMEPLAY  GUN soldier dies: EnemyStatusCommon::vf02 -> vf23(this,1|4) -> fn_801A250C -> fn_801A2930
            EnemyManager+0x34[team 0] += 1        (team = table 0x804CD310[enemy kind] via EnemyParamCommon::vf00)
PROGRESS  every frame: MissionManagerTask::vf01 -> fn_8016DD58 -> fn_8016B5B4 -> Mission vslot 0xF
            -> EnemyMission::vf0B: CountMission::vf06 SetCount(EnemyManager+0x34[0], 0)
            count >= +0x8 (35)  -> fn_8016B688 (*Mission::Clear*: state=1)
COMPLETE  fn_8016B43C: BasicMessageManage post(1); StageState request 6 (fn_801762D0)
            -> fn_80174974 jump table 0x8052CC68[6] = StageAction::GoalAction
            GoalAction ctor fn_802053B0: cleared slot = 0 (Dark) -> .bss 0x8057E824
            GoalAction::vf00 -> fn_80205010: next stage = StageTable[5].+0x1C + 0xC*0 = idx 6 = stage 200 (Digital Circuit)
```

---------------------------------------------------------------------------------------------------------------------

## 1. Data structures

### 1.1 Stage table — `.rodata 0x804C5AE8`, 0x3B entries × 0x50 (PROVEN)
Lookup by id: fn_80177020 (loops 0x3B, stride 0x50, compares +0; `-1` → current id from `.bss 0x8057E808`).
Index of current stage: `.bss 0x8057E80C` (= game-state block 0x8057E760 +0xAC; used as `idx*0x50` in fn_8016E4EC,
fn_80176FC8, fn_802D1EC8). Current stage id: `.bss 0x8057E808` (+0xA8, printed with "stg%04d" in fn_801783D4 @0x8017867C).

| off | meaning | evidence | conf |
|---|---|---|---|
| +0x00 | stage id (100, 200, …) | fn_80177020 compare; fn_802D2010 copies it as "next stage id" | PROVEN |
| +0x04 | char* section name ("City1", "Circuit", …) | fn_801E030C compares `[name]` of nukkoro*.inf to it | PROVEN |
| +0x08 | flags (bit0 boss stage, bit2, bit3, bit4 …) | bit0: fn_802D1EC8 @0x802D1F24, fn_800BB4F4; bit3: fn_8016F6F8; bit4: fn_80205058 | PROVEN (bits read), meaning LIKELY |
| +0x0C..+0x18 | 4 code pointers (per-stage hooks) | +0xC called at 0x80178780 in fn_801783D4 | PROVEN (call), meaning UNKNOWN |
| +0x1C+0xC·s | next stage **index** after clearing mission slot s | fn_802D1EC8 (@0x802D1F58/+0x28/+0x34), fn_80205010 @0x802050CC, fn_800BB3C4 | PROVEN |
| +0x20+0xC·s | mission **key** for slot s (−1 = no mission) | fn_80176FC8 `lwz r3,0x20(idx*0x50+slot*0xC)` | PROVEN |
| +0x24+0xC·s | u32 (0x227..0x2C5; probably text/message id) | no consumer found | UNKNOWN |
| +0x40 | mission key used in "hard" mode (single mission) | fn_8016E4EC @0x8016E550; fn_801DF470 @0x801DF514 (MISSIONCOUNT_HARD) | PROVEN |
| +0x44..+0x4C | u32 ×3 (0x7F9…, 0x226…, small int) | not traced | UNKNOWN |

Reconciliation with the coordinator's GekkoForge `kStageTable` lead (w0..w19 = +0x00..+0x4C): verified from the DOL.
The "small sequential id" in each route group (w8/w11/w14 = +0x20/+0x2C/+0x38) **is the mission key**: it is not an index into
a flat parameter array but the key of the MissionManager `std::map` (+0x30) filled by fn_8016CB48 (type, param) and patched by
nukkoro2.inf (counts); fn_8016E4EC (called at 0x80178604 with `.bss 0x8057E80C`) consumes it through fn_80176FC8 and the factory
fn_8016B768 (PROVEN). w16 (+0x40: 0, 0x25, 0x26, 0x27 …) is the hard-mode mission key (keys 0x25–0x27 are CountMission defaults;
City2/ARKPast1/eWorld set them with MISSIONCOUNT_HARD) (PROVEN). w9/w12/w15 (+0x24/+0x30/+0x3C, "event/movie id" per the lead)
has no DOL consumer found yet — UNKNOWN (lead unverified).

Slot semantics **0 = Dark, 1 = Normal, 2 = Hero** (PROVEN): fn_8016B330 appends "Dark" for slot 0 and "Hero" for slot 2
(@0x8016B3D4..0x8016B408); nukkoro command handlers MISSIONCOUNT_D/N/H pass slot 0/1/2 (fn_801DF3F8/fn_801DF420/fn_801DF448);
Westopolis next-stage indices 6/7/8 = stages 200/201/202 match the game's Dark/Neutral/Hero exits.

### 1.2 MissionManager singleton — `.bss 0x80576FBC` (getter fn_8006D7D0, ctor fn_8016E7B8, dtor fn_8016E750) (PROVEN)

| off | meaning | evidence |
|---|---|---|
| +0x00 | `Mission::MissionManagerTask*` | fn_8016E4EC @0x8016E738 |
| +0x04 | current stage index | fn_8016E4EC @0x8016E50C |
| +0x08/+0x10/+0x18 | `boost::shared_ptr<Mission::Mission>` for slot 0/1/2 (8 bytes each) | fn_8016E4EC @0x8016E5A4 (`this+8+8*i`), fn_8007420C *GetMission(slot)* |
| +0x20 | current (displayed/active) mission slot | fn_8016E220 *SetCurrentMission*; default 1 |
| +0x24 | checkpoint copy of +0x20 | fn_8016E3DC @0x8016E43C, fn_8016E360 @0x8016E3BC |
| +0x28 / +0x29 | any mission timed / all missions timed | fn_8016E4EC @0x8016E5D8..0x8016E678 |
| +0x2C | float min time limit over timed missions | fn_8016E4EC @0x8016E64C; fed to stage timer .bss 0x8057E7E8 (fn_80337D9C) |
| +0x30 | `std::map<int key, MissionDesc>` | fn_8016CB48 fill, fn_8016C7BC find, fn_8016C690 assign |

`MissionDesc` = `{ int type; int param; int count; int failCount; }` (node value at node+0x10; fn_8016C8EC builds
`{type, param, 1, 0}`, @0x8016C91C..0x8016C934) (PROVEN).

### 1.3 Mission objects (PROVEN unless noted)
`Mission::Mission` (0x40 bytes, ctor 0x8016B70C): +0x0 vptr, +0x4 key, +0x8 slot, +0xC state (0 running, 1 cleared, 2 failed —
fn_8016B688 / fn_8016B654), +0x10, +0x14 u8 *activated* (fn_8016E1A8 returns it; set by fn_80073BF8, fn_8016E454,
ElecCoreProgram::vf02), +0x15 u8, +0x18 `Mission::BasicMessageManage`.

`Mission::CountMission` (ctor 0x8016B180; Mission is a virtual base at +0x58): +0x0 vbase ptr, +0x4 vptr, **+0x8 required
count**, **+0xC fail count (0 = never fails)**, +0x10/+0x14 initial counts, **+0x18 current count**, +0x1C current fail counter,
+0x20/+0x24 checkpoint copies, +0x28 `Mission::CountMessageManage`.

`Mission::EnemyMission` (0xA0 bytes, ctor 0x801F3D7C): CountMission at 0, Mission vbase at +0x5C, **+0x58 enemy team**.

Mission-level virtual interface (vtable seen through the `Mission*` held by the manager; offsets are `[[m]+off]`):

| slot (off) | Mission default | CountMission / EnemyMission | *semantic guess* |
|---|---|---|---|
| 1 (+0xC) | 0x8016B31C → 0 | CountMission 2, EnemyMission 3 (0x801F3C9C) | *GetType* (Goal 1, Timer 4, TimerGoal 5, TimerCount 6, Ring 7 — equals desc.type, PROVEN) |
| 2 (+0x10) | 0 | 0 (Timer* return 1) | *HasTimeLimit* |
| 3 (+0x14) | float const | – (TimerMission: `lfs f1,8(r3)`) | *GetTimeLimit* |
| 4 (+0x18) | (state==1) | CountMission::vf02 0x8016B0DC | *GetCount(idx)* |
| 5 (+0x1C) | 1 | CountMission::vf03 0x8016B0D4 (+0x8) | *GetRequired* |
| 6 (+0x20) | 1 | CountMission::vf04 0x8016B0CC (+0xC) | *GetFailCount* |
| 7 (+0x24) | nop | CountMission::vf05 0x8016B00C | *AddCount(delta, idx)* |
| 8 (+0x28) | nop | CountMission::vf06 0x8016AF74 | *SetCount(value, idx)* |
| 9 (+0x2C) | nop | CountMission::vf07 0x8016AF5C | *SetRequired(value, idx)* |
| 0xC/0xD/0xE (+0x38/+0x3C/+0x40) | nop | CountMission::vf08/vf09/vf0A | *SaveCheckpoint / RestoreCheckpoint / Reset* |
| 0xF (+0x44) | nop 0x8016AE7C | EnemyMission::vf0B 0x801F3CA4 (thunk 0x801F3E28); RingMission::vf0B 0x8016F0DC; TimerMission::vf07 | *Update (per frame)* |

Verified in data: `u32(0x8053F5F0+0x44)=0x801F3E28` (thunk → EnemyMission::vf0B), `u32(0x8053F5B8+0x20)=0x8016AF74`
(CountMission::vf06), `u32(0x8053F5F0+0x24)=0x801F3EA0` (thunk → CountMission::vf05).

### 1.4 EnemyManager singleton — `.bss 0x80580BD8` (getter fn_80119560, ctor fn_801A2EB0, reset fn_801A2E24) (PROVEN)
| off | meaning | writer/reader |
|---|---|---|
| +0x10 | map<u16 linkID, {…, s16 +2 alive count}> | fn_801A2BA0 @0x801A2CC8, fn_801A250C |
| +0x20 | cleared at init | fn_801A2CFC |
| +0x24 | total enemies (all groups) | fn_801A2BA0 @0x801A2CD8 |
| +0x28[3] | enemies placed per team (0 GUN, 1 Eggman, 2 Black Arms) | fn_801A2BA0 |
| **+0x34[3]** | **enemies defeated per team** (read by EnemyMission) | ++ fn_801A2930 @0x801A2954; −− fn_801A250C @0x801A25A8; read fn_801A24FC |
| +0x40[3] | enemies defeated **by the player** per team | fn_801A2930 @0x801A2968 |
| +0x4C[3]/+0x58[3] | checkpoint copies of +0x34/+0x40 | fn_801A27B0 (save), fn_801A26B8 (restore) |

---------------------------------------------------------------------------------------------------------------------

## 2. Where mission parameters come from (answer: DOL tables + `nukkoro2.inf`, not SET data)

| parameter | source | entry point into logic | conf |
|---|---|---|---|
| which missions a stage has, per slot (key) | DOL stage table 0x804C5AE8 (+0x20+0xC·slot, +0x40 hard) | fn_80176FC8 → fn_8016E4EC @0x8016E58C / @0x8016E550 | PROVEN |
| mission class, target team | DOL: hard-coded `fn_8016C8EC(map,key,type,param)` calls in fn_8016CB48 (0x8016CB68..0x8016CE98) | fn_8016B768 `lwz r0,0(desc)` @0x8016B7A4 (type), EnemyMission ctor r7=desc+4 @0x8016B8B8 | PROVEN |
| required count / time limit / fail count | **`files/nukkoro2.inf`** `MISSIONCOUNT_{D,N,H,HARD} : <success> <fail>` inside `[<stage name>]` sections; default 1/0 from fn_8016C8EC | parser fn_801E030C → handlers fn_801DF3F8/fn_801DF420/fn_801DF448/fn_801DF3D0 → fn_801DF470 → fn_8016C690; consumed by factory fn_8016B768 (@0x8016B8C0 `lwz r8,8(desc)` for EnemyMission) | PROVEN |
| next stage per slot | DOL stage table +0x1C+0xC·slot | fn_80205010 @0x802050CC; fn_802D1EC8 | PROVEN |
| enemy team of each enemy | DOL table 0x804CD310[kind] (kinds 0–3 → 0, 4–11 → 2, 12–14 → 1) | EnemyParamCommon::vf00 0x801A68A0 → EnemyStatusCommon::vf23 @0x801A8A08 | PROVEN (table/read); kind↔class mapping LIKELY (counts 4/8/3 match SET ids 0x64-0x68, 0x8C-0x93, 0x78-0x7A) |
| enemy placement (what can be killed) | SET files stgXXXX_{cmn,nrm|hrd,ds1}.dat | fn_801A2BA0 scan (ids 0x64–0x95) | PROVEN |

`nukkoro2.inf` header comment (Japanese, in `nukkoro.inf`) documents the two numbers as *mission success counter* and
*mission failure counter*: count of targets for destroy/activate missions, number of enemies for defeat missions, seconds for
time-limit missions; `0` fail value disables failure. The code agrees (fn_8016B768 feeds desc+8/desc+C exactly that way).
Boot path is the normal one: RenderWare init callback fn_80048EFC (dataptr 0x8051E614) → fn_801E258C @0x80048F4C → loads
"nukkoro.inf" (@0x801E2680) and "nukkoro2.inf" (@0x801E2690). The number parser fn_801E01D0 accepts decimal, `-`, and `0x`
(hex loop only accepts 0–9 digits). (PROVEN)

Ordering (PROVEN): fn_801DF470 obtains the manager via fn_8006D7D0, whose first call constructs it (fn_8016E7B8 → fn_8016CB48
inserts all defaults), so `.inf` overrides always land on top of defaults. fn_8016CB48 is the only caller of the map clear
fn_8016D798, and fn_8016C690 (assign) has the single caller fn_801DF470, so nothing else rewrites the descriptors.
Consequence (PROVEN): keys are shared — e.g. key 1/2 are also used by stage 0 "Practice" (slots swapped), so City1's
MISSIONCOUNT lines configure Practice too; key 0x28 is shared by the three GUN2P stages and the last section ([GUN2P_0802]) wins.

---------------------------------------------------------------------------------------------------------------------

## 3. Stage initialization → mission object creation

1. `StageManager::vf01` (0x80178A88 calls fn_801783D4, curated name `StageManager_InitStage`) (PROVEN)
   - @0x801785F8 `fn_8006D7D0()` (MissionManager), @0x801785FC `r5 = .bss 0x8057E80C` (stage index), @0x80178604 `bl fn_8016E4EC`.
   - @0x80178720/0x80178724 `fn_80119560()` → `fn_801A2CFC` (EnemyManager scan, §4.1).
2. fn_8016E4EC *MissionManager::InitStage(mgr, parentTask, stageIdx)* (PROVEN)
   - +0x4 = stageIdx; +0x20 = +0x24 = 1; fn_8016E220(mgr, 1) (*SetCurrentMission*).
   - If `.bss 0x8057E885` (+0x125, "hard" flag; also selects `_hrd.dat`) ≠ 0: one mission, key = table+0x40, created with
     slot 1 and stored at +0x10 (@0x8016E534..0x8016E564).
   - Else loop slot 0..2: key = fn_80176FC8(stageIdx, slot); `fn_8016B768(&sp, mgr+0x30, key, slot)`; store at mgr+8+8·slot.
   - Aggregates time limits (+0x28/+0x29/+0x2C) via Mission slots 2/3; starts stage timer object `.bss 0x8057E7E8`
     (fn_80337D9C stores the limit to +0x14/+0x18/+0x1C, i.e. `.bss 0x8057E800` = remaining time).
   - fn_8016E454: any mission with key 0x1C is auto-activated (+0x14=1, +0x15=1) and made current.
   - `new(0x28)` Task → vptr +0x18 = 0x8052CA7C (`Mission::MissionManagerTask`), +0 = ptr to "MissionManagerTask".
3. fn_8016B768 *MissionFactory(ret, map, key, slot)* (PROVEN): `desc = fn_8016C7BC(map,key)` (std::map find, returns node+0x10);
   `switch(desc.type)` via jump table 0x8052C930:

| type | class (size) | ctor call | ctor args from desc |
|---|---|---|---|
| 0 | Mission (0x40) | 0x8016B7E0 | – |
| 1 | GoalMission (0x40) | 0x8016B824 | – |
| 2 | CountMission (0x9C) | 0x8016B87C | required = +8, fail = +0xC |
| 3 | **EnemyMission (0xA0)** | **0x8016B8D0** | **team = +4, required = +8** (fail forced 0 inside ctor @0x801F3DD8) |
| 4 | TimerMission (0x80) | 0x8016B938 | time = (float)+8 |
| 5 | TimerGoalMission (0x80) | 0x8016B9A0 | time = (float)+8 |
| 6 | TimerCountMission (0xD8) | 0x8016BA0C | required = +8, time = (float)+0xC |
| 7 | RingMission (0x9C) | 0x8016BA5C | required rings = +8 |

   Each result is wrapped in `boost::shared_ptr` (sp_counted_base_impl<Mission::X*> ctors 0x8016BAB0…0x8016C528).
4. Westopolis Dark concrete call (PROVEN by data + code): key `u32(0x804C5C78+0x20)=1`; desc = {3, 0, 35, 0} →
   `EnemyMission::EnemyMission(obj, 1, key=1, slot=0, team=0, required=35, 0)` → Mission ctor (+4=1, +8=0, state 0) →
   `CountMission::CountMission(obj, 0, 1, 0, 35, 0, 0, 0)` (+8=35, +0xC=0) → vcall `[[obj+4]+0x30]` = CountMission::vf0A
   (*Reset*: +0x18 = +0x10 = 0) → EnemyMission+0x58 = 0 (GUN).

---------------------------------------------------------------------------------------------------------------------

## 4. Gameplay events → progress

### 4.1 Enemy census at stage start — fn_801A2BA0 (*EnemyManager::ScanSet*) (PROVEN)
For each link id 0..255 (fn_800CB948 group list, next via +8), each SET object with id in [0x64,0x96):
- 0x64 GUN_SOLDIER: team 0 unless param AppearType (struct +0x24) == 5;
- 0x91 BK_LARVA: team 2 += param Num (struct +0x1C);
- else team = id<0x78 ? 0 : id<0x8C ? 1 : 2.
Writes +0x28[team], link-group alive count (map +0x10, s16 at +2) and +0x24. (+0x34 is reset to 0 by fn_801A2E24.)
Check with `agent_mission_enemycount.py`: stg0100 (cmn+nrm+ds1) = **36 GUN**, 45 Black Arms (hrd: 34/47). Every EnemyMission
requirement is ≤ the placed count of its team (100 D 35≤36, H 45≤45; 201 H 60≤80; 301 D 40≤41; 302 D 20≤21; 401 D 60≤61;
404 H 50≤50; 501 H 35≤35; 502 D 28≤29; 504 H 35≤35; 601 D 50≤53) — STRONG corroboration of the team mapping.

### 4.2 Enemy death → EnemyManager (PROVEN code path; enemy-side state details belong to the enemy trace)
- `EnemyStatusCommon::vf02` (0x801A8A38, per-frame status update): when death is detected (`[[this]+0x58]` true, not
  `[[this]+0x6C]`, not `[[this]+0xC4]`) → @0x801A8C40 `this->vf23(1)`; on revive → @0x801A8D04 `vf23(2)`.
- `EnemyStatusCommon::vf23` (0x801A8998): if flags==1 and status bit 0xC (this+0x80, BitTest 0x80014E34) → flags |= 4
  (*killed by player*, LIKELY meaning); setobj = `[[X+0x14]]->vslot(+0x2C)`; team = `[[X+0]]->vslot0` = EnemyParamCommon::vf00
  (`0x804CD310[kind]`); `fn_801A250C(EnemyManager, setobj, team, flags)`.
- fn_801A250C (*EnemyManager::OnEnemyEvent*): key = SET record link id (fn_800CA248: record+0x22); if the link group exists:
  flags&2 → +0x34[team]−−, group alive++; flags&1 → fn_801A2930, group alive−−, and when it reaches 0 →
  `fn_80169460(fn_800CBA84(), link, 1)` (link-group "all defeated" event).
- fn_801A2930 (*EnemyManager::CountKill*): **+0x34[team]++** (@0x801A2954, unconditional); if flags&4: +0x40[team]++ and a
  `Player::Npc::EnemyDeadByPlayerCommand(team, +0x40[team])` (0x1C bytes) is sent through vcall `[[cmd_target+0x38]+0x10]`.
  ⇒ kills by *anyone* count for EnemyMission; the by-player count only feeds the NPC command (PROVEN from code).

### 4.3 Mission update loop (PROVEN)
`Task_UpdateChildren` 0x8004ECAC calls Task slot 1 → `Mission::MissionManagerTask::vf01` 0x8016E824 → fn_8016DD58(mgr, dt)
→ for each slot: fn_8016B5B4(mission): if state ∉ {1,2} → `[[m]+0x44]` (slot 0xF).
For EnemyMission → thunk 0x801F3E28 → `EnemyMission::vf0B` 0x801F3CA4:
```
count = fn_801A24FC(fn_80119560(), this->team /*+0x58*/)   // EnemyManager+0x34[team]
this->vf06 /*[[this+4]+0x20] = CountMission::vf06*/(count, 0)
```
`CountMission::vf06` 0x8016AF74 (*SetCount*): idx 0 → +0x18 = count; `fn_802EDA58(this+0x28, count)` (CountMessageManage:
dispatch queued messages whose threshold ≤ count and whose slot == current slot, LIKELY radio/HUD messages);
**if +0x18 ≥ +0x8 → fn_8016B688(vbase Mission)** (@0x8016AFBC..0x8016AFC8). idx 1 path → fail counter vs +0xC → fn_8016B654.
Note: the poll does not test the activation flag +0x14 (only state) — PROVEN for fn_8016B5B4.

Other progress sources (PROVEN call sites, all `AddCount(1,0)` = `[[m]+0x24]` on `GetMission(slot)` fn_8007420C):
PowerDeviceCage::vf02 0x800D2AFC, PowerDeviceNaked::vf02 0x800D492C, Cream::vf02 0x80134A1C, DefenseSystem::vf02 0x80224570,
MagmaGuard::SetMagmaGuard::vf02 0x8022B65C, SuperComputer::SetSuperComputer::vf02 0x8022E60C, GiantLantern::vf02 0x80236FFC
(slot 0 = Dark), ElecAccessPanel::vf02 0x8024911C/0x80249194, ElecBomb::vf02 0x802646F4, CityBombBig::vf02 0x802F3C10,
SecretDisk::SecretDisk::vf02 0x802FA674, fn_800CCBFC, fn_8012EB54, fn_802F51E4, fn_80302E28 (also AddCount(−1)), fn_80319E04,
EscapePlane/EggBalloon/BKTank controllers. RingMission polls the player's ring count (RingMission::vf0B 0x8016F0DC → SetCount).
Direct clears: GoalRing contact handler fn_80106824 (dataptr 0x80527278 next to GOALRING SET desc 0x805272CC; STRONG) registers
fn_80107330 → fn_8016DE18 (*OnGoalRing*: clears the first non-failed TimerGoalMission, else the first GoalMission);
MissionClearCollision::vf02 0x80231AEC (SET 0x2595, param "0:DK 1:NM 2:HR" = slot) → fn_8016B688 @0x80231BF8 when the player is
inside the radius; ElecCoreProgram::vf02 @0x801F56C0; DebugMissionClearCollision::vf02 @0x802C8C9C.

HUD counter: fn_800BC74C → `sprintf("%d/%d", GetCount(0) [[m]+0x18], GetRequired [[m]+0x1C])` (PROVEN).

### 4.4 Mission activation / selection (STRONG)
fn_80073BF8 (called from Player::Npc::SonicType::SonicTypeExecuter::vf01, FlyType::FlyTypeExecuter::vf01,
BossSonic::BossSonicExecuter::vf01) → fn_8016E220(slot) and `GetMission(slot)+0x14 = 1` (@0x80073F0C): the partner NPC
activates/selects a mission. Player::Control::UserInput::vf03 queries fn_8016E1A8 (*IsMissionActivated*) and calls
fn_8016E220 (switch displayed mission).

---------------------------------------------------------------------------------------------------------------------

## 5. Completion / failure consequences

### 5.1 Success — fn_8016B688 (*Mission::Clear*) → fn_8016B43C (PROVEN)
- state = 1; unless `fn_80074598()+8 == 9` (game-mode 9, LIKELY 2P/special): `fn_802EC3E0(this+0x18, 1)` (BasicMessageManage
  post "success") and `fn_801762D0(StageState, 6, 0)`.
- fn_801762D0 stores a pending request in the StageState singleton (.bss 0x80577038, getter fn_80074598) unless disabled
  (+0x17+req) or locked (+0x14). fn_80174974 dispatches via jump table 0x8052CC68:
  1 –, 2 InitAction, 3 PlayAction, 4 EventAction, 5 PauseAction, **6 GoalAction**, 7 RestartAction, 8 ContinueAction,
  9 DeadAction, 10 SaveAction, 11 TryAgainAction, 12 ShowAction, 13 FlashbackAction, 14 Result2PAction, **15 TimeupAction**,
  16 EndAction (PROVEN).
- `StageAction::GoalAction::GoalAction` 0x80205ED8 → fn_802053B0: cleared slot (+0x10) = 1 by default, then for slot 0..2 the
  last mission with state == 1 (@0x8020546C) — for our trace 0 (Dark); written to `.bss 0x8057E824` (+0xC4). (Hard/expert path:
  from save object fn_800BBB34 +0x98: 3→0, 4→1, 5→2.)
- `StageAction::GoalAction::vf00` 0x8020554C: plays "sng_jin_roundclear.adx", and @0x80205D50 fn_80205010 appends the route
  history in the save object (+0x30 count, +0x34[] indices) with next index = `StageTable[cur].+0x1C + 0xC*slot`
  (@0x802050C4..0x802050D8) → Westopolis Dark → idx 6 = stage 200.
- Story next-stage resolver fn_802D1EC8 (called from fn_802D2060): result code 3 → +0x1C (Dark), 4 → +0x28 (Normal),
  5 → +0x34 (Hero), 6 → from save +0x98; boss stages (flags bit0) force 6; stores next stage id (or −1 for −2 "end").
  (codes→slot mapping STRONG: same 3/4/5 encoding as fn_802053B0.)

### 5.2 Failure — fn_8016B654 (*Mission::Fail*) → fn_8016B330 (PROVEN)
- Only if not already cleared: state = 2. If the failed slot is current → fn_8016E220(1). `fn_802EC3E0(this+0x18, 2)`.
- Unless mgr+0x29 (all missions timed): builds `"CsdFiles/MissionFailed%s"` + language suffix (table 0x8051E648: _JP/_EN/_FR/_SP,
  index .bss 0x8057E7B8) and plays CSD one-shot fn_80354484(path, "mf", "Dark"|"Hero") (CsdOneShot::CsdOneShot), then
  fn_801D59AC(fn_8005328C(), 0x1028, 0, 0) (LIKELY sound/voice 0x1028).
- fn_8016DDBC: if every slot is empty or failed → `fn_801762D0(StageState, 15, 0)` = StageAction::TimeupAction.
- Fail sources: CountMission fail counter (vf05/vf06 idx 1 ≥ +0xC, only if +0xC ≠ 0 — EnemyMission always has 0, so a Westopolis
  EnemyMission cannot fail by count), TimerMission::vf07 0x802C92B4 (remaining time `.bss 0x8057E800` ≈ 0 via fn_8000A950 →
  fn_8016B654 @0x802C92FC; STRONG), EscapePlaneController @0x8025F3F0, EggBalloonController @0x80274670,
  BKTankController @0x80322F74.

### 5.3 Checkpoint / restart (PROVEN)
Checkpoint touch fn_80119350 → fn_8016E3DC (mission *SaveCheckpoint* slot 0xC for all, +0x24 = +0x20) and fn_801A27B0 (EnemyManager
+0x34/+0x40 → +0x4C/+0x58). ContinueAction::vf00 → fn_8016E360 (restore slot 0xD, current = +0x24) and fn_801A28A8 (restore
+0x34/+0x40, rescan groups fn_801A2A30). RestartAction::vf00 → fn_8016E2D8 (reset slot 0xE, state 0, current 1).

---------------------------------------------------------------------------------------------------------------------

## 6. Open items
- Meaning of stage table +0x24+0xC·slot, +0x44..+0x4C and the four hook pointers (+0xC..+0x18): UNKNOWN.
- Where each enemy class writes its *kind* (index into 0x804CD310) — not located (no direct constant store found); LIKELY set
  from the SET id in a shared base ctor.
- CountMessageManage / BasicMessageManage message payloads (what text/voice they show): LIKELY partner radio lines, not traced.
- Game mode value 9 that suppresses the success path (fn_8016B43C): UNKNOWN.

---------------------------------------------------------------------------------------------------------------------

## 7. Westopolis (100 "City1") — per-parameter table

| Stage | Mission | Parameter | Value/source | Consumer address | Confidence |
|---|---|---|---|---|---|
| 100 City1 | Dark | stage-table entry | idx 5 @0x804C5C78 (id 100, name "City1") | fn_80177020 / .bss 0x8057E80C | PROVEN |
| 100 City1 | Dark | mission key | 1 = u32(0x804C5C98) | fn_80176FC8 → fn_8016E4EC @0x8016E58C | PROVEN |
| 100 City1 | Dark | class | desc.type 3 = EnemyMission (fn_8016CB48 @0x8016CB8C: key 1, type 3, param 0) | fn_8016B768 @0x8016B7A4 + jt 0x8052C930[3] | PROVEN |
| 100 City1 | Dark | target team | desc+4 = 0 = GUN (SET ids 0x64–0x77) | EnemyMission ctor +0x58 @0x801F3E10; EnemyMission::vf0B @0x801F3CBC | PROVEN |
| 100 City1 | Dark | required count | 35 — nukkoro2.inf `[City1] MISSIONCOUNT_D : 35 0` (default 1) | fn_801DF470 @0x801DF558 → fn_8016C690; factory @0x8016B8C0; CountMission+0x8 @0x8016B200; compare @0x8016AFBC | PROVEN |
| 100 City1 | Dark | fail count | 0 (file value 0; EnemyMission ctor passes 0 anyway @0x801F3DD8) | CountMission+0xC | PROVEN |
| 100 City1 | Dark | timer | none (EnemyMission slot 2 → Mission::vf02 = 0) | fn_8016E4EC @0x8016E608 | PROVEN |
| 100 City1 | Dark | progress source | EnemyManager+0x34[0] (+1 per GUN death, fn_801A2930 @0x801A2954) | EnemyMission::vf0B → CountMission::vf06 | PROVEN |
| 100 City1 | Dark | placed GUN enemies | 36 (stg0100 cmn 33 + nrm 3) | fn_801A2BA0 | PROVEN (data) |
| 100 City1 | Dark | next stage | idx 6 = stage 200 "Circuit" (u32(0x804C5C94)) | fn_80205010 @0x802050D4; fn_802D1EC8 @0x802D1F58 | PROVEN |
| 100 City1 | Normal | mission key / class | 0 → GoalMission (type 1) | fn_8016B768 jt[1]; cleared by fn_8016DE18 (goal ring) | PROVEN |
| 100 City1 | Normal | next stage | idx 7 = stage 201 "Canyon1" | fn_80205010 | PROVEN |
| 100 City1 | Hero | mission key | 2 = u32(0x804C5CB0) | fn_80176FC8 | PROVEN |
| 100 City1 | Hero | class / team | EnemyMission, desc+4 = 2 = Black Arms (SET 0x8C–0x95) (fn_8016CB48 @0x8016CBA0) | EnemyMission+0x58 | PROVEN |
| 100 City1 | Hero | required count | 45 — nukkoro2.inf `[City1] MISSIONCOUNT_H : 45 0` | fn_801DF448 → fn_801DF470 | PROVEN |
| 100 City1 | Hero | placed Black Arms | 45 (stg0100 cmn) | fn_801A2BA0 | PROVEN (data) |
| 100 City1 | Hero | next stage | idx 8 = stage 202 "Highway" | fn_80205010 | PROVEN |
| 100 City1 | Hard | mission key | u32(0x804C5CB8) = 0 → GoalMission (only one mission, slot 1) | fn_8016E4EC @0x8016E550 | PROVEN |

---------------------------------------------------------------------------------------------------------------------

## 8. All stages — effective mission configuration (generated by `python agent_mission_table.py`)

Missing slots have key −1 in the stage table (e.g. Digital Circuit has no Normal mission). Boss/2P/event stages have no
Dark/Hero keys. "Hard/X" rows use stage table +0x40.

### 8.1 Default descriptor map (fn_8016CB48)

| key | type | param(+4) |
|---|---|---|
| 0x00 | 1 GoalMission | 0 |
| 0x01 | 3 EnemyMission | 0 |
| 0x02 | 3 EnemyMission | 2 |
| 0x03 | 2 CountMission | 0 |
| 0x04 | 2 CountMission | 0 |
| 0x05 | 3 EnemyMission | 2 |
| 0x06 | 2 CountMission | 0 |
| 0x07 | 2 CountMission | 0 |
| 0x08 | 2 CountMission | 0 |
| 0x09 | 3 EnemyMission | 0 |
| 0x0A | 2 CountMission | 0 |
| 0x0B | 3 EnemyMission | 0 |
| 0x0C | 7 RingMission | 0 |
| 0x0D | 6 TimerCountMission | 0 |
| 0x0E | 6 TimerCountMission | 0 |
| 0x0F | 3 EnemyMission | 0 |
| 0x10 | 2 CountMission | 0 |
| 0x11 | 2 CountMission | 0 |
| 0x12 | 2 CountMission | 0 |
| 0x13 | 2 CountMission | 0 |
| 0x14 | 2 CountMission | 0 |
| 0x15 | 3 EnemyMission | 2 |
| 0x16 | 2 CountMission | 0 |
| 0x17 | 2 CountMission | 0 |
| 0x18 | 3 EnemyMission | 2 |
| 0x19 | 3 EnemyMission | 0 |
| 0x1A | 2 CountMission | 0 |
| 0x1B | 2 CountMission | 0 |
| 0x1C | 5 TimerGoalMission | 0 |
| 0x1D | 3 EnemyMission | 2 |
| 0x1E | 2 CountMission | 0 |
| 0x1F | 3 EnemyMission | 0 |
| 0x20 | 2 CountMission | 0 |
| 0x21 | 5 TimerGoalMission | 0 |
| 0x22 | 4 TimerMission | 0 |
| 0x23 | 2 CountMission | 0 |
| 0x24 | 5 TimerGoalMission | 0 |
| 0x25 | 2 CountMission | 0 |
| 0x26 | 2 CountMission | 0 |
| 0x27 | 2 CountMission | 0 |
| 0x28 | 4 TimerMission | 0 |

### 8.2 Effective per-stage missions

| idx | stage | name | slot | next idx (stage) | key | class | ctor params | count source |
|---|---|---|---|---|---|---|---|---|
| 0 | 0 | Practice | Dark | -2 | 0x02 | EnemyMission | team=Black Arms (SET 0x8C-0x95), required=45 (fail count forced 0) | nukkoro2.inf:22 [City1] MISSIONCOUNT_H |
| 0 | 0 | Practice | Normal | -2 | 0x00 | GoalMission | - | default@8016CB78 |
| 0 | 0 | Practice | Hero | -2 | 0x01 | EnemyMission | team=GUN (SET 0x64-0x77), required=35 (fail count forced 0) | nukkoro2.inf:21 [City1] MISSIONCOUNT_D |
| 0 | 0 | Practice | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 5 | 100 | City1 | Dark | 6 (200) | 0x01 | EnemyMission | team=GUN (SET 0x64-0x77), required=35 (fail count forced 0) | nukkoro2.inf:21 [City1] MISSIONCOUNT_D |
| 5 | 100 | City1 | Normal | 7 (201) | 0x00 | GoalMission | - | default@8016CB78 |
| 5 | 100 | City1 | Hero | 8 (202) | 0x02 | EnemyMission | team=Black Arms (SET 0x8C-0x95), required=45 (fail count forced 0) | nukkoro2.inf:22 [City1] MISSIONCOUNT_H |
| 5 | 100 | City1 | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 6 | 200 | Circuit | Dark | 9 (300) | 0x03 | CountMission | required=1, fail=0 | default@8016CBB4 |
| 6 | 200 | Circuit | Hero | 10 (301) | 0x00 | GoalMission | - | default@8016CB78 |
| 6 | 200 | Circuit | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 7 | 201 | Canyon1 | Dark | 9 (300) | 0x04 | CountMission | required=5, fail=0 | nukkoro2.inf:42 [Canyon1] MISSIONCOUNT_D |
| 7 | 201 | Canyon1 | Normal | 10 (301) | 0x00 | GoalMission | - | default@8016CB78 |
| 7 | 201 | Canyon1 | Hero | 11 (302) | 0x05 | EnemyMission | team=Black Arms (SET 0x8C-0x95), required=60 (fail count forced 0) | nukkoro2.inf:43 [Canyon1] MISSIONCOUNT_H |
| 7 | 201 | Canyon1 | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 8 | 202 | Highway | Dark | 28 (210) | 0x00 | GoalMission | - | default@8016CB78 |
| 8 | 202 | Highway | Hero | 28 (210) | 0x06 | CountMission | required=1, fail=0 | default@8016CBF0 |
| 8 | 202 | Highway | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 9 | 300 | HorrorCastle | Dark | 29 (310) | 0x07 | CountMission | required=5, fail=0 | nukkoro2.inf:64 [HorrorCastle] MISSIONCOUNT_D |
| 9 | 300 | HorrorCastle | Normal | 29 (310) | 0x00 | GoalMission | - | default@8016CB78 |
| 9 | 300 | HorrorCastle | Hero | 29 (310) | 0x08 | CountMission | required=2, fail=0 | nukkoro2.inf:65 [HorrorCastle] MISSIONCOUNT_H |
| 9 | 300 | HorrorCastle | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 10 | 301 | PrisonIsland | Dark | 13 (401) | 0x09 | EnemyMission | team=GUN (SET 0x64-0x77), required=40 (fail count forced 0) | nukkoro2.inf:76 [PrisonIsland] MISSIONCOUNT_D |
| 10 | 301 | PrisonIsland | Normal | 14 (402) | 0x00 | GoalMission | - | default@8016CB78 |
| 10 | 301 | PrisonIsland | Hero | 15 (403) | 0x0A | CountMission | required=5, fail=0 | nukkoro2.inf:77 [PrisonIsland] MISSIONCOUNT_H |
| 10 | 301 | PrisonIsland | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 11 | 302 | Circus | Dark | 14 (402) | 0x0B | EnemyMission | team=GUN (SET 0x64-0x77), required=20 (fail count forced 0) | nukkoro2.inf:88 [Circus] MISSIONCOUNT_D |
| 11 | 302 | Circus | Normal | 15 (403) | 0x00 | GoalMission | - | default@8016CB78 |
| 11 | 302 | Circus | Hero | 16 (404) | 0x0C | RingMission | required rings=400 | nukkoro2.inf:89 [Circus] MISSIONCOUNT_H |
| 11 | 302 | Circus | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 12 | 400 | City2 | Dark | 17 (500) | 0x0D | TimerCountMission | required=5, time=480s | nukkoro2.inf:101 [City2] MISSIONCOUNT_D |
| 12 | 400 | City2 | Hero | 18 (501) | 0x0E | TimerCountMission | required=20, time=480s | nukkoro2.inf:102 [City2] MISSIONCOUNT_H |
| 12 | 400 | City2 | Hard/X |  | 0x25 | CountMission | required=5, fail=0 | nukkoro2.inf:103 [City2] MISSIONCOUNT_HARD |
| 13 | 401 | ARKPast1 | Dark | 30 (410) | 0x0F | EnemyMission | team=GUN (SET 0x64-0x77), required=60 (fail count forced 0) | nukkoro2.inf:109 [ARKPast1] MISSIONCOUNT_D |
| 13 | 401 | ARKPast1 | Normal | 30 (410) | 0x00 | GoalMission | - | default@8016CB78 |
| 13 | 401 | ARKPast1 | Hero | 30 (410) | 0x10 | CountMission | required=10, fail=0 | nukkoro2.inf:108 [ARKPast1] MISSIONCOUNT_H |
| 13 | 401 | ARKPast1 | Hard/X |  | 0x26 | CountMission | required=10, fail=0 | nukkoro2.inf:117 [ARKPast1] MISSIONCOUNT_HARD |
| 14 | 402 | canyon2 | Dark | 18 (501) | 0x11 | CountMission | required=5, fail=0 | nukkoro2.inf:130 [canyon2] MISSIONCOUNT_D |
| 14 | 402 | canyon2 | Normal | 19 (502) | 0x00 | GoalMission | - | default@8016CB78 |
| 14 | 402 | canyon2 | Hero | 20 (503) | 0x12 | CountMission | required=5, fail=0 | nukkoro2.inf:131 [canyon2] MISSIONCOUNT_H |
| 14 | 402 | canyon2 | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 15 | 403 | eWorld | Dark | 31 (411) | 0x13 | CountMission | required=30, fail=0 | nukkoro2.inf:142 [eWorld] MISSIONCOUNT_D |
| 15 | 403 | eWorld | Normal | 31 (411) | 0x00 | GoalMission | - | default@8016CB78 |
| 15 | 403 | eWorld | Hero | 31 (411) | 0x14 | CountMission | required=4, fail=0 | nukkoro2.inf:143 [eWorld] MISSIONCOUNT_H |
| 15 | 403 | eWorld | Hard/X |  | 0x27 | CountMission | required=4, fail=0 | nukkoro2.inf:144 [eWorld] MISSIONCOUNT_HARD |
| 16 | 404 | Ruins | Dark | 32 (412) | 0x00 | GoalMission | - | default@8016CB78 |
| 16 | 404 | Ruins | Hero | 32 (412) | 0x15 | EnemyMission | team=Black Arms (SET 0x8C-0x95), required=50 (fail count forced 0) | nukkoro2.inf:154 [Ruins] MISSIONCOUNT_H |
| 16 | 404 | Ruins | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 17 | 500 | ARKRuins1 | Dark | 33 (510) | 0x16 | CountMission | required=4, fail=0 | nukkoro2.inf:164 [ARKRuins1] MISSIONCOUNT_D |
| 17 | 500 | ARKRuins1 | Normal | 33 (510) | 0x00 | GoalMission | - | default@8016CB78 |
| 17 | 500 | ARKRuins1 | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 18 | 501 | Sky | Dark | 22 (600) | 0x17 | CountMission | required=1, fail=1 | nukkoro2.inf:176 [Sky] MISSIONCOUNT_D |
| 18 | 501 | Sky | Normal | 23 (601) | 0x00 | GoalMission | - | default@8016CB78 |
| 18 | 501 | Sky | Hero | 24 (602) | 0x18 | EnemyMission | team=Black Arms (SET 0x8C-0x95), required=35 (fail count forced 0) | nukkoro2.inf:177 [Sky] MISSIONCOUNT_H |
| 18 | 501 | Sky | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 19 | 502 | Jungle | Dark | 34 (511) | 0x19 | EnemyMission | team=GUN (SET 0x64-0x77), required=28 (fail count forced 0) | nukkoro2.inf:188 [Jungle] MISSIONCOUNT_D |
| 19 | 502 | Jungle | Normal | 34 (511) | 0x00 | GoalMission | - | default@8016CB78 |
| 19 | 502 | Jungle | Hero | 34 (511) | 0x1A | CountMission | required=1, fail=1 | nukkoro2.inf:189 [Jungle] MISSIONCOUNT_H |
| 19 | 502 | Jungle | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 20 | 503 | Space | Dark | 24 (602) | 0x1B | CountMission | required=6, fail=0 | nukkoro2.inf:201 [Space] MISSIONCOUNT_D |
| 20 | 503 | Space | Normal | 25 (603) | 0x00 | GoalMission | - | default@8016CB78 |
| 20 | 503 | Space | Hero | 26 (604) | 0x1C | TimerGoalMission | time=300s | nukkoro2.inf:202 [Space] MISSIONCOUNT_H |
| 20 | 503 | Space | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 21 | 504 | ARKPast2 | Normal | 25 (603) | 0x00 | GoalMission | - | default@8016CB78 |
| 21 | 504 | ARKPast2 | Hero | 26 (604) | 0x1D | EnemyMission | team=Black Arms (SET 0x8C-0x95), required=35 (fail count forced 0) | nukkoro2.inf:212 [ARKPast2] MISSIONCOUNT_H |
| 21 | 504 | ARKPast2 | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 22 | 600 | GunsBase | Dark | 36 (611) | 0x1E | CountMission | required=3, fail=0 | nukkoro2.inf:222 [GunsBase] MISSIONCOUNT_D |
| 22 | 600 | GunsBase | Hero | 35 (610) | 0x00 | GoalMission | - | default@8016CB78 |
| 22 | 600 | GunsBase | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 23 | 601 | DoomsBase1 | Dark | 38 (613) | 0x1F | EnemyMission | team=GUN (SET 0x64-0x77), required=50 (fail count forced 0) | nukkoro2.inf:232 [DoomsBase1] MISSIONCOUNT_D |
| 23 | 601 | DoomsBase1 | Hero | 37 (612) | 0x00 | GoalMission | - | default@8016CB78 |
| 23 | 601 | DoomsBase1 | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 24 | 602 | EggmansBase | Dark | 39 (614) | 0x20 | CountMission | required=5, fail=0 | nukkoro2.inf:241 [EggmansBase] MISSIONCOUNT_D |
| 24 | 602 | EggmansBase | Hero | 39 (614) | 0x00 | GoalMission | - | default@8016CB78 |
| 24 | 602 | EggmansBase | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 25 | 603 | ARKRuins2 | Dark | 40 (615) | 0x21 | TimerGoalMission | time=900s | nukkoro2.inf:252 [ARKRuins2] MISSIONCOUNT_D |
| 25 | 603 | ARKRuins2 | Hero | 41 (616) | 0x22 | TimerMission | time=900s | nukkoro2.inf:253 [ARKRuins2] MISSIONCOUNT_H |
| 25 | 603 | ARKRuins2 | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 26 | 604 | DoomsBase2 | Dark | 43 (618) | 0x23 | CountMission | required=4, fail=0 | nukkoro2.inf:263 [DoomsBase2] MISSIONCOUNT_D |
| 26 | 604 | DoomsBase2 | Hero | 42 (617) | 0x00 | GoalMission | - | default@8016CB78 |
| 26 | 604 | DoomsBase2 | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 27 | 700 | DoomsCore | Hero | 44 (710) | 0x24 | TimerGoalMission | time=600s | nukkoro2.inf:271 [DoomsCore] MISSIONCOUNT_H |
| 27 | 700 | DoomsCore | Hard/X |  | 0x00 | GoalMission | - | default@8016CB78 |
| 45 | 800 | GUN2P_0800 | Normal | -2 | 0x28 | TimerMission | time=600s | nukkoro2.inf:404 [GUN2P_0802] MISSIONCOUNT_N |
| 46 | 801 | GUN2P_0801 | Normal | -2 | 0x28 | TimerMission | time=600s | nukkoro2.inf:404 [GUN2P_0802] MISSIONCOUNT_N |
| 47 | 802 | GUN2P_0802 | Normal | -2 | 0x28 | TimerMission | time=600s | nukkoro2.inf:404 [GUN2P_0802] MISSIONCOUNT_N |

### 8.3 Keys used by more than one stage/slot

- 0x00 (GoalMission): Practice/Normal, Practice/Hard, City1/Normal, City1/Hard, Circuit/Hero, Circuit/Hard, Canyon1/Normal, Canyon1/Hard, Highway/Dark, Highway/Hard, HorrorCastle/Normal, HorrorCastle/Hard, PrisonIsland/Normal, PrisonIsland/Hard, Circus/Normal, Circus/Hard, ARKPast1/Normal, canyon2/Normal, canyon2/Hard, eWorld/Normal, Ruins/Dark, Ruins/Hard, ARKRuins1/Normal, ARKRuins1/Hard, Sky/Normal, Sky/Hard, Jungle/Normal, Jungle/Hard, Space/Normal, Space/Hard, ARKPast2/Normal, ARKPast2/Hard, GunsBase/Hero, GunsBase/Hard, DoomsBase1/Hero, DoomsBase1/Hard, EggmansBase/Hero, EggmansBase/Hard, ARKRuins2/Hard, DoomsBase2/Hero, DoomsBase2/Hard, DoomsCore/Hard
- 0x01 (EnemyMission): Practice/Hero, City1/Dark
- 0x02 (EnemyMission): Practice/Dark, City1/Hero
- 0x28 (TimerMission): GUN2P_0800/Normal, GUN2P_0801/Normal, GUN2P_0802/Normal

---------------------------------------------------------------------------------------------------------------------

## 9. All stages — parameter table (generated by `python agent_mission_table.py rows`)

Hard-mode rows whose key is 0 (plain GoalMission) are omitted. Value sources: `default@ADDR` = fn_8016C8EC call site in fn_8016CB48;
`nukkoro2.inf:LINE [section] CMD` = override applied by fn_801DF470.

| Stage | Mission | Parameter | Value/source | Consumer address | Confidence |
|---|---|---|---|---|---|
| 0 Practice | Dark | mission key | 0x02 (stage table 0x804C5AE8 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 0 Practice | Dark | class (desc.type) | 3 = EnemyMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 0 Practice | Dark | enemy team (desc+4) | 2 = Black Arms (SET 0x8C-0x95) (fn_8016CB48) | EnemyMission+0x58 -> EnemyMission::vf0B 0x801F3CBC | PROVEN |
| 0 Practice | Dark | required count (desc+8) | 45 (nukkoro2.inf:22 [City1] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 0 Practice | Normal | mission key | 0x00 (stage table 0x804C5AE8 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 0 Practice | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 0 Practice | Hero | mission key | 0x01 (stage table 0x804C5AE8 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 0 Practice | Hero | class (desc.type) | 3 = EnemyMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 0 Practice | Hero | enemy team (desc+4) | 0 = GUN (SET 0x64-0x77) (fn_8016CB48) | EnemyMission+0x58 -> EnemyMission::vf0B 0x801F3CBC | PROVEN |
| 0 Practice | Hero | required count (desc+8) | 35 (nukkoro2.inf:21 [City1] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 100 City1 | Dark | mission key | 0x01 (stage table 0x804C5C78 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 100 City1 | Dark | class (desc.type) | 3 = EnemyMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 100 City1 | Dark | enemy team (desc+4) | 0 = GUN (SET 0x64-0x77) (fn_8016CB48) | EnemyMission+0x58 -> EnemyMission::vf0B 0x801F3CBC | PROVEN |
| 100 City1 | Dark | required count (desc+8) | 35 (nukkoro2.inf:21 [City1] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 100 City1 | Dark | next stage on clear | idx 6 = stage 200 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 100 City1 | Normal | mission key | 0x00 (stage table 0x804C5C78 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 100 City1 | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 100 City1 | Normal | next stage on clear | idx 7 = stage 201 (table +0x28) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 100 City1 | Hero | mission key | 0x02 (stage table 0x804C5C78 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 100 City1 | Hero | class (desc.type) | 3 = EnemyMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 100 City1 | Hero | enemy team (desc+4) | 2 = Black Arms (SET 0x8C-0x95) (fn_8016CB48) | EnemyMission+0x58 -> EnemyMission::vf0B 0x801F3CBC | PROVEN |
| 100 City1 | Hero | required count (desc+8) | 45 (nukkoro2.inf:22 [City1] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 100 City1 | Hero | next stage on clear | idx 8 = stage 202 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 200 Circuit | Dark | mission key | 0x03 (stage table 0x804C5CC8 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 200 Circuit | Dark | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 200 Circuit | Dark | required count (desc+8) | 1 (default@8016CBB4) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 200 Circuit | Dark | next stage on clear | idx 9 = stage 300 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 200 Circuit | Hero | mission key | 0x00 (stage table 0x804C5CC8 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 200 Circuit | Hero | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 200 Circuit | Hero | next stage on clear | idx 10 = stage 301 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 201 Canyon1 | Dark | mission key | 0x04 (stage table 0x804C5D18 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 201 Canyon1 | Dark | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 201 Canyon1 | Dark | required count (desc+8) | 5 (nukkoro2.inf:42 [Canyon1] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 201 Canyon1 | Dark | next stage on clear | idx 9 = stage 300 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 201 Canyon1 | Normal | mission key | 0x00 (stage table 0x804C5D18 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 201 Canyon1 | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 201 Canyon1 | Normal | next stage on clear | idx 10 = stage 301 (table +0x28) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 201 Canyon1 | Hero | mission key | 0x05 (stage table 0x804C5D18 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 201 Canyon1 | Hero | class (desc.type) | 3 = EnemyMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 201 Canyon1 | Hero | enemy team (desc+4) | 2 = Black Arms (SET 0x8C-0x95) (fn_8016CB48) | EnemyMission+0x58 -> EnemyMission::vf0B 0x801F3CBC | PROVEN |
| 201 Canyon1 | Hero | required count (desc+8) | 60 (nukkoro2.inf:43 [Canyon1] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 201 Canyon1 | Hero | next stage on clear | idx 11 = stage 302 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 202 Highway | Dark | mission key | 0x00 (stage table 0x804C5D68 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 202 Highway | Dark | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 202 Highway | Dark | next stage on clear | idx 28 = stage 210 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 202 Highway | Hero | mission key | 0x06 (stage table 0x804C5D68 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 202 Highway | Hero | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 202 Highway | Hero | required count (desc+8) | 1 (default@8016CBF0) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 202 Highway | Hero | next stage on clear | idx 28 = stage 210 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 300 HorrorCastle | Dark | mission key | 0x07 (stage table 0x804C5DB8 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 300 HorrorCastle | Dark | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 300 HorrorCastle | Dark | required count (desc+8) | 5 (nukkoro2.inf:64 [HorrorCastle] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 300 HorrorCastle | Dark | next stage on clear | idx 29 = stage 310 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 300 HorrorCastle | Normal | mission key | 0x00 (stage table 0x804C5DB8 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 300 HorrorCastle | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 300 HorrorCastle | Normal | next stage on clear | idx 29 = stage 310 (table +0x28) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 300 HorrorCastle | Hero | mission key | 0x08 (stage table 0x804C5DB8 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 300 HorrorCastle | Hero | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 300 HorrorCastle | Hero | required count (desc+8) | 2 (nukkoro2.inf:65 [HorrorCastle] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 300 HorrorCastle | Hero | next stage on clear | idx 29 = stage 310 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 301 PrisonIsland | Dark | mission key | 0x09 (stage table 0x804C5E08 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 301 PrisonIsland | Dark | class (desc.type) | 3 = EnemyMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 301 PrisonIsland | Dark | enemy team (desc+4) | 0 = GUN (SET 0x64-0x77) (fn_8016CB48) | EnemyMission+0x58 -> EnemyMission::vf0B 0x801F3CBC | PROVEN |
| 301 PrisonIsland | Dark | required count (desc+8) | 40 (nukkoro2.inf:76 [PrisonIsland] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 301 PrisonIsland | Dark | next stage on clear | idx 13 = stage 401 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 301 PrisonIsland | Normal | mission key | 0x00 (stage table 0x804C5E08 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 301 PrisonIsland | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 301 PrisonIsland | Normal | next stage on clear | idx 14 = stage 402 (table +0x28) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 301 PrisonIsland | Hero | mission key | 0x0A (stage table 0x804C5E08 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 301 PrisonIsland | Hero | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 301 PrisonIsland | Hero | required count (desc+8) | 5 (nukkoro2.inf:77 [PrisonIsland] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 301 PrisonIsland | Hero | next stage on clear | idx 15 = stage 403 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 302 Circus | Dark | mission key | 0x0B (stage table 0x804C5E58 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 302 Circus | Dark | class (desc.type) | 3 = EnemyMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 302 Circus | Dark | enemy team (desc+4) | 0 = GUN (SET 0x64-0x77) (fn_8016CB48) | EnemyMission+0x58 -> EnemyMission::vf0B 0x801F3CBC | PROVEN |
| 302 Circus | Dark | required count (desc+8) | 20 (nukkoro2.inf:88 [Circus] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 302 Circus | Dark | next stage on clear | idx 14 = stage 402 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 302 Circus | Normal | mission key | 0x00 (stage table 0x804C5E58 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 302 Circus | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 302 Circus | Normal | next stage on clear | idx 15 = stage 403 (table +0x28) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 302 Circus | Hero | mission key | 0x0C (stage table 0x804C5E58 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 302 Circus | Hero | class (desc.type) | 7 = RingMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 302 Circus | Hero | required count (desc+8) | 400 (nukkoro2.inf:89 [Circus] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 302 Circus | Hero | next stage on clear | idx 16 = stage 404 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 400 City2 | Dark | mission key | 0x0D (stage table 0x804C5EA8 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 400 City2 | Dark | class (desc.type) | 6 = TimerCountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 400 City2 | Dark | required count (desc+8) | 5 (nukkoro2.inf:101 [City2] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 400 City2 | Dark | time limit s (desc+0xC) | 480 (nukkoro2.inf:101 [City2] MISSIONCOUNT_D) | TimerCountMission ctor f1 (0x8016B9DC) | PROVEN |
| 400 City2 | Dark | next stage on clear | idx 17 = stage 500 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 400 City2 | Hero | mission key | 0x0E (stage table 0x804C5EA8 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 400 City2 | Hero | class (desc.type) | 6 = TimerCountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 400 City2 | Hero | required count (desc+8) | 20 (nukkoro2.inf:102 [City2] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 400 City2 | Hero | time limit s (desc+0xC) | 480 (nukkoro2.inf:102 [City2] MISSIONCOUNT_H) | TimerCountMission ctor f1 (0x8016B9DC) | PROVEN |
| 400 City2 | Hero | next stage on clear | idx 18 = stage 501 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 400 City2 | Hard | mission key | 0x25 (stage table 0x804C5EA8 +0x40) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 400 City2 | Hard | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 400 City2 | Hard | required count (desc+8) | 5 (nukkoro2.inf:103 [City2] MISSIONCOUNT_HARD) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 401 ARKPast1 | Dark | mission key | 0x0F (stage table 0x804C5EF8 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 401 ARKPast1 | Dark | class (desc.type) | 3 = EnemyMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 401 ARKPast1 | Dark | enemy team (desc+4) | 0 = GUN (SET 0x64-0x77) (fn_8016CB48) | EnemyMission+0x58 -> EnemyMission::vf0B 0x801F3CBC | PROVEN |
| 401 ARKPast1 | Dark | required count (desc+8) | 60 (nukkoro2.inf:109 [ARKPast1] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 401 ARKPast1 | Dark | next stage on clear | idx 30 = stage 410 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 401 ARKPast1 | Normal | mission key | 0x00 (stage table 0x804C5EF8 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 401 ARKPast1 | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 401 ARKPast1 | Normal | next stage on clear | idx 30 = stage 410 (table +0x28) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 401 ARKPast1 | Hero | mission key | 0x10 (stage table 0x804C5EF8 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 401 ARKPast1 | Hero | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 401 ARKPast1 | Hero | required count (desc+8) | 10 (nukkoro2.inf:108 [ARKPast1] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 401 ARKPast1 | Hero | next stage on clear | idx 30 = stage 410 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 401 ARKPast1 | Hard | mission key | 0x26 (stage table 0x804C5EF8 +0x40) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 401 ARKPast1 | Hard | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 401 ARKPast1 | Hard | required count (desc+8) | 10 (nukkoro2.inf:117 [ARKPast1] MISSIONCOUNT_HARD) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 402 canyon2 | Dark | mission key | 0x11 (stage table 0x804C5F48 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 402 canyon2 | Dark | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 402 canyon2 | Dark | required count (desc+8) | 5 (nukkoro2.inf:130 [canyon2] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 402 canyon2 | Dark | next stage on clear | idx 18 = stage 501 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 402 canyon2 | Normal | mission key | 0x00 (stage table 0x804C5F48 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 402 canyon2 | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 402 canyon2 | Normal | next stage on clear | idx 19 = stage 502 (table +0x28) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 402 canyon2 | Hero | mission key | 0x12 (stage table 0x804C5F48 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 402 canyon2 | Hero | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 402 canyon2 | Hero | required count (desc+8) | 5 (nukkoro2.inf:131 [canyon2] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 402 canyon2 | Hero | next stage on clear | idx 20 = stage 503 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 403 eWorld | Dark | mission key | 0x13 (stage table 0x804C5F98 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 403 eWorld | Dark | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 403 eWorld | Dark | required count (desc+8) | 30 (nukkoro2.inf:142 [eWorld] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 403 eWorld | Dark | next stage on clear | idx 31 = stage 411 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 403 eWorld | Normal | mission key | 0x00 (stage table 0x804C5F98 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 403 eWorld | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 403 eWorld | Normal | next stage on clear | idx 31 = stage 411 (table +0x28) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 403 eWorld | Hero | mission key | 0x14 (stage table 0x804C5F98 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 403 eWorld | Hero | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 403 eWorld | Hero | required count (desc+8) | 4 (nukkoro2.inf:143 [eWorld] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 403 eWorld | Hero | next stage on clear | idx 31 = stage 411 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 403 eWorld | Hard | mission key | 0x27 (stage table 0x804C5F98 +0x40) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 403 eWorld | Hard | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 403 eWorld | Hard | required count (desc+8) | 4 (nukkoro2.inf:144 [eWorld] MISSIONCOUNT_HARD) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 404 Ruins | Dark | mission key | 0x00 (stage table 0x804C5FE8 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 404 Ruins | Dark | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 404 Ruins | Dark | next stage on clear | idx 32 = stage 412 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 404 Ruins | Hero | mission key | 0x15 (stage table 0x804C5FE8 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 404 Ruins | Hero | class (desc.type) | 3 = EnemyMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 404 Ruins | Hero | enemy team (desc+4) | 2 = Black Arms (SET 0x8C-0x95) (fn_8016CB48) | EnemyMission+0x58 -> EnemyMission::vf0B 0x801F3CBC | PROVEN |
| 404 Ruins | Hero | required count (desc+8) | 50 (nukkoro2.inf:154 [Ruins] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 404 Ruins | Hero | next stage on clear | idx 32 = stage 412 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 500 ARKRuins1 | Dark | mission key | 0x16 (stage table 0x804C6038 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 500 ARKRuins1 | Dark | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 500 ARKRuins1 | Dark | required count (desc+8) | 4 (nukkoro2.inf:164 [ARKRuins1] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 500 ARKRuins1 | Dark | next stage on clear | idx 33 = stage 510 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 500 ARKRuins1 | Normal | mission key | 0x00 (stage table 0x804C6038 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 500 ARKRuins1 | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 500 ARKRuins1 | Normal | next stage on clear | idx 33 = stage 510 (table +0x28) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 501 Sky | Dark | mission key | 0x17 (stage table 0x804C6088 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 501 Sky | Dark | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 501 Sky | Dark | required count (desc+8) | 1 (nukkoro2.inf:176 [Sky] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 501 Sky | Dark | fail count (desc+0xC) | 1 (nukkoro2.inf:176 [Sky] MISSIONCOUNT_D) | CountMission+0xC, CountMission::vf05 0x8016B0A4 -> Fail 0x8016B654 | PROVEN |
| 501 Sky | Dark | next stage on clear | idx 22 = stage 600 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 501 Sky | Normal | mission key | 0x00 (stage table 0x804C6088 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 501 Sky | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 501 Sky | Normal | next stage on clear | idx 23 = stage 601 (table +0x28) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 501 Sky | Hero | mission key | 0x18 (stage table 0x804C6088 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 501 Sky | Hero | class (desc.type) | 3 = EnemyMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 501 Sky | Hero | enemy team (desc+4) | 2 = Black Arms (SET 0x8C-0x95) (fn_8016CB48) | EnemyMission+0x58 -> EnemyMission::vf0B 0x801F3CBC | PROVEN |
| 501 Sky | Hero | required count (desc+8) | 35 (nukkoro2.inf:177 [Sky] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 501 Sky | Hero | next stage on clear | idx 24 = stage 602 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 502 Jungle | Dark | mission key | 0x19 (stage table 0x804C60D8 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 502 Jungle | Dark | class (desc.type) | 3 = EnemyMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 502 Jungle | Dark | enemy team (desc+4) | 0 = GUN (SET 0x64-0x77) (fn_8016CB48) | EnemyMission+0x58 -> EnemyMission::vf0B 0x801F3CBC | PROVEN |
| 502 Jungle | Dark | required count (desc+8) | 28 (nukkoro2.inf:188 [Jungle] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 502 Jungle | Dark | next stage on clear | idx 34 = stage 511 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 502 Jungle | Normal | mission key | 0x00 (stage table 0x804C60D8 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 502 Jungle | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 502 Jungle | Normal | next stage on clear | idx 34 = stage 511 (table +0x28) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 502 Jungle | Hero | mission key | 0x1A (stage table 0x804C60D8 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 502 Jungle | Hero | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 502 Jungle | Hero | required count (desc+8) | 1 (nukkoro2.inf:189 [Jungle] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 502 Jungle | Hero | fail count (desc+0xC) | 1 (nukkoro2.inf:189 [Jungle] MISSIONCOUNT_H) | CountMission+0xC, CountMission::vf05 0x8016B0A4 -> Fail 0x8016B654 | PROVEN |
| 502 Jungle | Hero | next stage on clear | idx 34 = stage 511 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 503 Space | Dark | mission key | 0x1B (stage table 0x804C6128 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 503 Space | Dark | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 503 Space | Dark | required count (desc+8) | 6 (nukkoro2.inf:201 [Space] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 503 Space | Dark | next stage on clear | idx 24 = stage 602 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 503 Space | Normal | mission key | 0x00 (stage table 0x804C6128 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 503 Space | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 503 Space | Normal | next stage on clear | idx 25 = stage 603 (table +0x28) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 503 Space | Hero | mission key | 0x1C (stage table 0x804C6128 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 503 Space | Hero | class (desc.type) | 5 = TimerGoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 503 Space | Hero | time limit s (desc+8) | 300 (nukkoro2.inf:202 [Space] MISSIONCOUNT_H) | TimerMission+0x8 (float), fn_8016E4EC min-time -> stage timer .bss 0x8057E7E8 | PROVEN |
| 503 Space | Hero | next stage on clear | idx 26 = stage 604 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 504 ARKPast2 | Normal | mission key | 0x00 (stage table 0x804C6178 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 504 ARKPast2 | Normal | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 504 ARKPast2 | Normal | next stage on clear | idx 25 = stage 603 (table +0x28) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 504 ARKPast2 | Hero | mission key | 0x1D (stage table 0x804C6178 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 504 ARKPast2 | Hero | class (desc.type) | 3 = EnemyMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 504 ARKPast2 | Hero | enemy team (desc+4) | 2 = Black Arms (SET 0x8C-0x95) (fn_8016CB48) | EnemyMission+0x58 -> EnemyMission::vf0B 0x801F3CBC | PROVEN |
| 504 ARKPast2 | Hero | required count (desc+8) | 35 (nukkoro2.inf:212 [ARKPast2] MISSIONCOUNT_H) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 504 ARKPast2 | Hero | next stage on clear | idx 26 = stage 604 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 600 GunsBase | Dark | mission key | 0x1E (stage table 0x804C61C8 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 600 GunsBase | Dark | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 600 GunsBase | Dark | required count (desc+8) | 3 (nukkoro2.inf:222 [GunsBase] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 600 GunsBase | Dark | next stage on clear | idx 36 = stage 611 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 600 GunsBase | Hero | mission key | 0x00 (stage table 0x804C61C8 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 600 GunsBase | Hero | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 600 GunsBase | Hero | next stage on clear | idx 35 = stage 610 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 601 DoomsBase1 | Dark | mission key | 0x1F (stage table 0x804C6218 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 601 DoomsBase1 | Dark | class (desc.type) | 3 = EnemyMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 601 DoomsBase1 | Dark | enemy team (desc+4) | 0 = GUN (SET 0x64-0x77) (fn_8016CB48) | EnemyMission+0x58 -> EnemyMission::vf0B 0x801F3CBC | PROVEN |
| 601 DoomsBase1 | Dark | required count (desc+8) | 50 (nukkoro2.inf:232 [DoomsBase1] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 601 DoomsBase1 | Dark | next stage on clear | idx 38 = stage 613 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 601 DoomsBase1 | Hero | mission key | 0x00 (stage table 0x804C6218 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 601 DoomsBase1 | Hero | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 601 DoomsBase1 | Hero | next stage on clear | idx 37 = stage 612 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 602 EggmansBase | Dark | mission key | 0x20 (stage table 0x804C6268 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 602 EggmansBase | Dark | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 602 EggmansBase | Dark | required count (desc+8) | 5 (nukkoro2.inf:241 [EggmansBase] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 602 EggmansBase | Dark | next stage on clear | idx 39 = stage 614 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 602 EggmansBase | Hero | mission key | 0x00 (stage table 0x804C6268 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 602 EggmansBase | Hero | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 602 EggmansBase | Hero | next stage on clear | idx 39 = stage 614 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 603 ARKRuins2 | Dark | mission key | 0x21 (stage table 0x804C62B8 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 603 ARKRuins2 | Dark | class (desc.type) | 5 = TimerGoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 603 ARKRuins2 | Dark | time limit s (desc+8) | 900 (nukkoro2.inf:252 [ARKRuins2] MISSIONCOUNT_D) | TimerMission+0x8 (float), fn_8016E4EC min-time -> stage timer .bss 0x8057E7E8 | PROVEN |
| 603 ARKRuins2 | Dark | next stage on clear | idx 40 = stage 615 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 603 ARKRuins2 | Hero | mission key | 0x22 (stage table 0x804C62B8 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 603 ARKRuins2 | Hero | class (desc.type) | 4 = TimerMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 603 ARKRuins2 | Hero | time limit s (desc+8) | 900 (nukkoro2.inf:253 [ARKRuins2] MISSIONCOUNT_H) | TimerMission+0x8 (float), fn_8016E4EC min-time -> stage timer .bss 0x8057E7E8 | PROVEN |
| 603 ARKRuins2 | Hero | next stage on clear | idx 41 = stage 616 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 604 DoomsBase2 | Dark | mission key | 0x23 (stage table 0x804C6308 +0x20) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 604 DoomsBase2 | Dark | class (desc.type) | 2 = CountMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 604 DoomsBase2 | Dark | required count (desc+8) | 4 (nukkoro2.inf:263 [DoomsBase2] MISSIONCOUNT_D) | CountMission+0x8, compare in CountMission::vf05/vf06 0x8016B070/0x8016AFB8 | PROVEN |
| 604 DoomsBase2 | Dark | next stage on clear | idx 43 = stage 618 (table +0x1C) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 604 DoomsBase2 | Hero | mission key | 0x00 (stage table 0x804C6308 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 604 DoomsBase2 | Hero | class (desc.type) | 1 = GoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 604 DoomsBase2 | Hero | next stage on clear | idx 42 = stage 617 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 700 DoomsCore | Hero | mission key | 0x24 (stage table 0x804C6358 +0x38) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 700 DoomsCore | Hero | class (desc.type) | 5 = TimerGoalMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 700 DoomsCore | Hero | time limit s (desc+8) | 600 (nukkoro2.inf:271 [DoomsCore] MISSIONCOUNT_H) | TimerMission+0x8 (float), fn_8016E4EC min-time -> stage timer .bss 0x8057E7E8 | PROVEN |
| 700 DoomsCore | Hero | next stage on clear | idx 44 = stage 710 (table +0x34) | fn_80205010 0x802050CC / fn_802D1EC8 | PROVEN |
| 800 GUN2P_0800 | Normal | mission key | 0x28 (stage table 0x804C68F8 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 800 GUN2P_0800 | Normal | class (desc.type) | 4 = TimerMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 800 GUN2P_0800 | Normal | time limit s (desc+8) | 600 (nukkoro2.inf:404 [GUN2P_0802] MISSIONCOUNT_N) | TimerMission+0x8 (float), fn_8016E4EC min-time -> stage timer .bss 0x8057E7E8 | PROVEN |
| 801 GUN2P_0801 | Normal | mission key | 0x28 (stage table 0x804C6948 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 801 GUN2P_0801 | Normal | class (desc.type) | 4 = TimerMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 801 GUN2P_0801 | Normal | time limit s (desc+8) | 600 (nukkoro2.inf:404 [GUN2P_0802] MISSIONCOUNT_N) | TimerMission+0x8 (float), fn_8016E4EC min-time -> stage timer .bss 0x8057E7E8 | PROVEN |
| 802 GUN2P_0802 | Normal | mission key | 0x28 (stage table 0x804C6998 +0x2C) | fn_80176FC8 / fn_8016E4EC -> fn_8016B768 | PROVEN |
| 802 GUN2P_0802 | Normal | class (desc.type) | 4 = TimerMission (default@fn_8016CB48) | fn_8016B768 jump table 0x8052C930 | PROVEN |
| 802 GUN2P_0802 | Normal | time limit s (desc+8) | 600 (nukkoro2.inf:404 [GUN2P_0802] MISSIONCOUNT_N) | TimerMission+0x8 (float), fn_8016E4EC min-time -> stage timer .bss 0x8057E7E8 | PROVEN |

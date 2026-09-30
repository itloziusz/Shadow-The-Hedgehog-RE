# GAMEPLAY_STRUCTURES — recovered gameplay data structures

Offsets are hex. "Evidence" cites function addresses in `sys/main.dol`. Class names in `Code` that also
appear in RTTI are **original**; field names are recovered semantic names.
SET structures (SetRecord, SetSlot, SetData, SetObjDesc, SetParamDesc) are in
`STAGE_GAMEPLAY_DATA_FORMATS.md`; mission structures (Mission, CountMission, EnemyMission, MissionManager
0x80576FBC, EnemyManager 0x80580BD8) in `MISSION_SYSTEM.md` §3; the player object (`Player::PlayerShadow`,
0x29C) in `PLAYER_GAMEPLAY_RECOVERY.md` §3; the full GunSoldier layout in `enemy/GUN_SOLDIER.md`; karma gauges
in `COLLECTIBLES_AND_SCORING.md`.

Inheritance statements below come from RTTI base lists (PROVEN) — no hierarchy is assumed beyond them.

---

## 1. `Task` (RTTI `Task`, vtable 0x8051E768) — PROVEN

Base of every updatable object (layers, managers, enemies' `TEnemySetTask`, gadgets…).
Ctor `Task::Task(Task* parent)` 0x8004F014; dtor 0x8004EF18; scheduler 0x8004ECAC.

| Offset | Size | Field | Evidence | Confidence |
|---|---:|---|---|---|
| +0x00 | 4 | `const char* name` (default `*(r13-0x7AC8)`; layers get names from 0x8053ED20) | ctor 0x8004F07C, fn_801EB53C | PROVEN |
| +0x04 | 2 | `u16 flags`: 0x1 kill request, 0x2–0x8 other pending bits (tested as mask 0xF), 0x20 destroying, 0x100 update while paused | 0x8004ECDC–0x8004ED70, dtor 0x8004EF48 | PROVEN |
| +0x08 | 4 | `Task* prev` (first child's prev = last child) | ctor, fn_8004EBC4 | PROVEN |
| +0x0C | 4 | `Task* next` | ctor, scheduler | PROVEN |
| +0x10 | 4 | `Task* parent` | ctor, fn_8004EB04 | PROVEN |
| +0x14 | 4 | `Task* firstChild` | ctor, scheduler | PROVEN |
| +0x18 | 4 | vptr → slot0 `~Task(int del)`, slot1 `Update(float dt)` | 0x8004ED4C, 0x8004ED98 | PROVEN |
| +0x20 | 8 | `u64 lastUpdateTicks` (profiling, OS timebase delta) | fn_8004EDF0 | PROVEN |
| (sizeof ≥ 0x28) | | layer tasks are allocated with 0x28 | fn_801EB53C `new(0x28)` | PROVEN |

Globals: `sbss 0x805EF270` traversal depth, `0x805EF274` profiling enabled, `0x805EF278` live task
count, `0x805EF27C` (u8) global pause.

`fn_8004EB04(Task*)` returns 0 if the task or any ancestor has kill-bit set (IsAlive) — PROVEN.
`fn_8004EECC(Task*)` sets kill-bit on all descendants — PROVEN.
`fn_8004EBC4(Task*, Task* newParent)` re-parents a task — PROVEN.

## 2. `TaskManager` (no RTTI; storage bss 0x80571C6C) — PROVEN

| Offset | Field | Evidence |
|---|---|---|
| +0x00 | `Task* root` | ctor 0x801EB734 |
| +0x04 + 4·i | `Task* layer[19]` (i per GAMEPLAY_UPDATE_PIPELINE.md §3) | fn_801EB614, fn_801EB53C, fn_801EB4A4 |

## 3. Global game/stage state block — bss 0x8057E760 (no RTTI)

Field usage mined with `q.py xref` (count = number of functions touching it).

| Offset | Abs. | Type | Candidate field | Evidence | Confidence |
|---|---|---|---|---|---|
| +0x0C | 8057E76C | s32 | frame budget / skip count (clamped against +0x34) | fn_801E2210 | LIKELY |
| +0x18 | 8057E778 | f32 | **player dt** (PlayerShadow ctor points player+0x60 here @0x800AE3D0) | PLAYER_GAMEPLAY_RECOVERY.md §2 | STRONG |
| +0x20 | 8057E780 | f32 | default object dt (CharInfo ctor points +0x60 here) | same | STRONG |
| +0x24 | 8057E784 | f32 | written by the Chaos Control "Slow" task; read by 49 fns (bosses, attack callbacks) — slowed-world dt | same | LIKELY |
| +0x44..+0x57 | 8057E7A4.. | | pad/video status copied each frame | fn_801E2210 | STRONG |
| +0x50 | 8057E7B0 | s32 | frames per second: 50 (PAL, fn_803071F4) or 60 | fn_801E2210 0x801E22E0 | PROVEN |
| +0x82 | 8057E7E2 | u8 | "frame step entered" flag | fn_801E2210 | PROVEN (code) |
| +0x90 | 8057E7F0 | s32 | frame counter/timestamp (copied into enemy+0xF8 when an enemy goes live) | TEnemySetBase::vf07 0x801A7480 | LIKELY |
| +0xA8 | 8057E808 | s32 | **current stage number** (formatted "stg%04d" in stage init) | fn_801783D4 0x80178684; 55 readers | STRONG |
| +0xAC | 8057E80C | s32 | **stage index** (bit index into setid.bin masks; also passed to fn_8016E4EC) | `SetData_LoadStage(mgr, [+0xA8], [+0xAC])` at 0x8017871C; index table in STAGE_GAMEPLAY_DATA_FORMATS.md §3.1 | PROVEN (role) |
| +0xB2 | 8057E812 | u8 | two-player mode (SET scan uses 2 players) | fn_800CB97C | STRONG |
| +0x125 | 8057E885 | u8 | **hard mode** (load `_hrd.dat` instead of `_nrm.dat`) | fn_800CBC54 | PROVEN |
| +0x126 | 8057E886 | u8 | set in StageManager ctor / InitAction | xref | UNKNOWN |

## 4. Enemy object model (RTTI) — GUN Soldier as reference instance

The standard enemies use a **component architecture built from multiple + virtual inheritance**.
Every concrete enemy is `EnemyTemplate<XBase, XAI>`-derived. `GunSoldier::GunSoldier` (sizeof 0x418 —
`operator new(0x418)` in create hook 0x80196E38) has these subobjects (offsets from RTTI base list
of typeinfo 0x805E8FE0, confirmed by vptr stores in ctor 0x8019708C):

| Offset | Subobject (original class) | vptr at | Role (from class name + calls) | Confidence |
|---|---|---|---|---|
| +0x000 | `EnemyParamObjBase` → ptr to param component | — | interface pointer | STRONG |
| +0x004 | `EnemyStatusObjBase` → `EnemyStatusCommon*` | — | status/health component ptr | PROVEN (used as `[[e+4]]` vcalls in 0x801A7280) |
| +0x008 | `EnemyDispObjBase` → `EnemyDispCommon*` | — | display component ptr (vptr at comp+0xC) | PROVEN |
| +0x00C | `EnemyMoveObjBase` → `EnemyMoveWalker*` | — | movement component ptr | PROVEN |
| +0x010 | `EnemyCtrlObjBase` → `EnemyBaseAI*` | — | AI component ptr (vptr at comp+0x18) | PROVEN |
| +0x014 | `EnemySetObjBase` → `TEnemySetBase*` | — | SET/lifecycle component ptr | PROVEN |
| +0x000 | `Enemy`, `EnemyRefBase`, `TEnemyCommonBase`, `EnemyWalkerCommon`, `SoldierCommonBase`, `GunSoldier::GunSoldierBase`, `EnemyTemplate<…>` | +0x018 | primary chain | PROVEN |
| +0x0EC | f32 | | display alpha (fade in/out, clamp 0..1) | TEnemySetTask::Update 0x801A6F30/fn_8019AE90 | STRONG |
| +0x0F8 | s32 | | frame stamp when enemy became live | 0x801A7488 | STRONG |
| +0x104 | `EnemyParaFunc` / `EnemyParamCommon` | +0x104 | parameter component (28 slots) | PROVEN |
| +0x110 | `EnemyStatFunc` / `EnemyStatusCommon` | +0x110 | status component (51 slots) | PROVEN |
| +0x1A8 | `EnemyDispFunc` / `EnemyDispCommon` | +0x1B4 | display component (24 slots) | PROVEN |
| +0x1E8 | `EnemyMoveFunc` / `EnemyMoveCommon` / `EnemyMoveWalker` | +0x1E8 | movement component (12 slots) | PROVEN |
| +0x250 | `EnemySetFunc` / `TEnemySetBase` | +0x250 | SET binding + lifecycle state machine (39 slots) | PROVEN |
| +0x360 | `EnemyCtrlFunc` / `EnemyBaseAI` / `SoldierCommonAI` / `GunSoldier::GunSoldierAI` | +0x378 | AI (23 slots) | PROVEN |
| +0x3E0 | `Task` / `TEnemySetTask` | +0x3F8 | scheduler node in layer "Enemy" | PROVEN |
| +0x40C | `SetSlot*` = TEnemySetTask+0x2C (ctor arg r6) | | SET slot of this instance | PROVEN |
| +0x410 | `EnemyReferer` (virtual base) | — | `+0 = Enemy*` back-pointer | PROVEN (ctor 0x80197130) |

Virtual-base pointers to `EnemyReferer` (+0x410) are stored at +0x100, +0x108, +0x114, +0x1B8, +0x1EC,
+0x24C, +0x254, +0x37C, +0x408 (ctor 0x801970B0–0x801970D0) — i.e. each component reaches the owning
enemy via `component->vbptr->enemy`.

GunSoldier vtable group base V = 0x80539E64; vptr values: +0x18 V, +0x104 V+0x14, +0x110 V+0x8C,
+0x1B4 V+0x160, +0x1E8 V+0x1C8, +0x250 V+0x200, +0x378 V+0x2A4, +0x3F8 V+0x308.

### 4.1 `TEnemySetTask` (RTTI; bases `Task`@0, `EnemyReferer`@0x30 in standalone layout)

| Offset (in subobject) | Field | Evidence | Confidence |
|---|---|---|---|
| +0x00..+0x27 | `Task` | RTTI | PROVEN |
| +0x28 | vbptr → `EnemyReferer` (→ enemy) | 0x801A6EDC | PROVEN |
| +0x2C | `SetSlot*` (NULL if not spawned from SET) | 0x801A6F24 | STRONG |

### 4.2 `TEnemySetBase` (component @0x250 in GunSoldier)

| Offset | Field | Evidence | Confidence |
|---|---|---|---|
| +0x00 | vptr | | PROVEN |
| +0x04 | vbptr → `EnemyReferer` | 0x801A72CC | PROVEN |
| +0x08 | `s32 lifecycleState` (1 wait-resources, 2 init, 3 wait-ready, 4 running; ≥5 idle) | 0x801A729C | PROVEN |
| +0x0C | async resource/model handle (fn_80081050 / fn_801A75C8 / fn_80080D58) | 0x801A7318 | LIKELY |

## 5. `SetManager` singleton (sbss 0x805EF77C, getter 0x800C9F6C) — PROVEN

| Offset | Field | Evidence |
|---|---|---|
| +0x00 | `SetData*` | fn_800CBC54 (`lwz r3,0(r31)`) |
| +0x04 | `u8 loaded` | fn_800CBC54 0x800CBD70, fn_800CB97C |

## 6. Runtime helpers used everywhere (PROVEN)

| Address | Semantic | Evidence |
|---|---|---|
| 803A1380 | `operator new(size)` (size 0 → 4, new_handler loop) | code |
| 803A1334 | `operator delete(p)` | code |
| 8000540C | `memset` | .init, fn_800CAF38 usage |
| 800054F4 | `memmove` (handles overlap) | code |
| 803AD96C | `strcmp` | code |
| 803AD92C | `memcmp`/`strncmp` (used for "sky2") | fn_800CB1E4 |
| 803A1210 | register global destructor (atexit-like) | singletons |
| 803A1AFC | `__dynamic_cast` (typeinfo args) | fn_801D3374 |
| 800091D0 / 800091EC | Vec3 copy | code |
| 8040DDC4 | Vec3 sub (dst = a − b) | code |
| 8040DE54 / 8040DF7C | Vec3 sub-in-place / length² | fn_800CA970 |
| 80014E34 | bit test `(word[0] >> n) & 1` | code |

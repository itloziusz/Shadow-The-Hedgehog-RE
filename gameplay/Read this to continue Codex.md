# Read this to continue — handoff for Codex

> Historical gameplay working-copy handoff. The local-only path restriction
> below describes that investigation session; use the root README for this
> public repository's layout and build instructions. Game inputs stay read-only
> and are never committed.

**Project:** recover the gameplay systems of *Shadow the Hedgehog* (GameCube, PAL **GUPP8P**) from
`sys/main.dol`, as evidence-backed docs + native C++ (not a better disassembly,
not an emulator). Written 2026-09-30 by the previous agent (Claude). Everything below is in this folder.

---

## Current continuation (checkpoint 22, 2026-09-30)

The canonical public repository builds the C++17 boot-entry foundation under
`reverse/boot/`. Its exact PAL fixture gate stops at `0x80003158` after the
full pinned register helper at `0x800032B0..0x8000333C`. A separate bounded
native slice reproduces application-loop event/exit order at
`0x800511E0..0x80051218`; it is not connected across unresolved hardware,
CRT, constructor and initialization state. Independent `reverse/boot/research/`
notes map the preentry, hardware and CRT frontiers, including unknown pre-CRT
FPR bytes at `0x805F1F30..3F` and IPL-dependent FST placement. The public
Release gate is **32/32 CTest** with read-only content. See `HANDOFF.md`
checkpoint 22 and `reverse/boot/OPEN_QUESTIONS.md`. Gameplay state below
remains unchanged.

### Gameplay continuation from checkpoint 20

Latest `build_and_run.ps1`: **27/27 CTest suites pass**, and stg0100 Dark 35/35 clears →
stage index 6 (stg0200). Direct stg0201 Hero 60/60 clears → index 11 (stg0302), and
stg0404 Hero 50/50 clears → index 32 (stg0412); all three CTest cases assert the
exact destination route, not only process success. All ten documented enemy-count
mission simulator routes now have exact-route CTest cases and pass.
The native scope now includes GunSoldier moving patrol (`0x80188B98`), the SET 0x0091
`BkLarvaGenerator`/child life events, and partial BK Soldier, Egg Pawn, GUN Beetle,
GUN Robot, BK WingSmall and BK Worm decision states. Every
unrecovered animation, world trigger, weapon and movement service is labelled as a hook.
`src/PlayerPhysics.*` includes the PROVEN partial Jump hold step at 0x80087384..0x800873F0;
`PlayerBehavior.*` adds bounded Ground/Fall/Jump/HomingAttack branch slices.
`notes/open_questions_terrain_contact.md` maps terrain contact bits and proves the `Awake`
setter for Chaos-ready flags 0x22/0x23; geometry is still UNKNOWN. Symbols: 1,344 curated
data rows and 8,594 exported data rows. The larva test uses checks that remain active
in Release. BK WingSmall, GUN Robot and BK Worm now have labelled simulator adapters;
stg0100, stg0301, stg0401 and stg0404 report zero stub enemies. Their world
services remain hooks. Normal enemy death now marks its SET slot killed/detached before task deletion;
stg0100 asserts zero duplicate Beetle spawns per SET slot. The enemy teardown fix
detaches a slot only when the old enemy still owns it. SET flag 0x8 does not
itself block re-spawn: the scanner preserves it if external 0x40 explicitly
re-arms the slot. See `HANDOFF.md` checkpoint 20 before changing gameplay code.
The old `gameplay/`-only rule below applied to the original working-copy
investigation; public repository boot/runtime work now lives in its named
root modules, with all build products under the repository's ignored `build/`.

---

## 0. Rules the user set (must follow)

1. **Do nothing outside `gameplay/`** (user's words: "Don't do anything outside this
   folder"): no temp/scratch files, no builds or caches elsewhere, no writes to other folders.
   The toolkit only *reads* the game data at `..\sys\main.dol` and `..\files\`. Every other folder is read-only reference,
   including `GekkoForge_RE_Toolchain\` (a static-recompiler project for the same DOL) and the older asset-loader files
   in the repo root. Don't run GekkoForge's prebuilt `.exe`s either: they write artifacts and `save-data` next to
   themselves.
2. **Build and run the reconstructed code from time to time** (user request): run
   `powershell -ExecutionPolicy Bypass -File gameplay\build_and_run.ps1` (CMake build into `gameplay/build`, all
   CTest suites, then `stage_sim` on stg0100). Do it after every C++ change and before each handoff checkpoint.
3. **Write handoffs regularly** (`HANDOFF.md`, newest checkpoint on top), in case context runs out.
4. **Evidence first.** Tag every claim PROVEN / STRONG / LIKELY / UNKNOWN. Never promote speculation. Class names
   from RTTI are **original**. Function and method names are **recovered semantics**; the original function names
   are not in the binary. Mark placeholders explicitly; never fake unknown behaviour.

## 1. Start here

| File | Why |
|---|---|
| `README.md` | index of all deliverables + build/test commands |
| `HANDOFF.md` | checkpoints (newest first) with decisions, verification and corrections |
| `OPEN_QUESTIONS.md` | prioritized unresolved facts: the best place to pick the next task |
| `tools/README.md` | toolkit usage (query CLI, pipeline, generators) |

Deliverables (all written): `GAMEPLAY_DOL_MAP.md`, `GAMEPLAY_STRUCTURES.md`, `GAMEPLAY_UPDATE_PIPELINE.md`,
`PLAYER_GAMEPLAY_RECOVERY.md`, `ENEMY_BEHAVIOR_INDEX.md`, `enemy/GUN_SOLDIER.md`, `MISSION_SYSTEM.md`,
`MISSION_PARAMETER_MAP.md`, `STAGE_GAMEPLAY_DATA_FORMATS.md`, `CHECKPOINTS_AND_RESTART.md`, `GAMEPLAY_COMMANDS.md`,
`WEAPONS_AND_TARGETING.md`, `VEHICLES.md`, `COLLECTIBLES_AND_SCORING.md`, `enemy/BK_SOLDIER.md`,
`enemy/EGG_PAWN.md`, `enemy/GUN_BEETLE.md`, `enemy/ENEMY_ALERT_PATHS.md`, `GAMEPLAY_SYMBOLS.csv`.
Raw evidence logs: `notes/mission_trace.md`, `notes/enemy_gunsoldier_trace.md`, `notes/player_trace.md`.

**First milestone: done.** Enemy trace (GUN Soldier spawn → init → AI → damage → "death"/injured → despawn) is in
`enemy/GUN_SOLDIER.md` §0. Mission trace (Westopolis Dark "defeat 35 GUN soldiers": stage init → parameters →
events → progress → completion → next stage) is in `MISSION_SYSTEM.md` §9. It is reproduced natively by
`src/tests/test_mission.cpp` on real game data.

## 2. Historical state at the original handoff (superseded by checkpoint 12)

Every investigation that was running has finished; its results are merged:
- Enemy families: `enemy/BK_SOLDIER.md`, `enemy/EGG_PAWN.md`, `enemy/GUN_BEETLE.md`. The normal death path is
  PROVEN: kill reported → weapon dropped → `TEnemySetBase::DetachKilled` sets SET flag 0x8 and disables the record → no automatic re-arm →
  explode/delete. `enemy/ENEMY_ALERT_PATHS.md` concludes (STRONG) that the search sensor is the ONLY alert path, so
  enemies with SearchWidth 0 are passive.
- `WEAPONS_AND_TARGETING.md` (rewritten): hit rule `fn_800C6A4C` (attack level > defense; damage =
  `CharaColliAttack+0x34`), damage values for player moves and every weapon, weapon registry 0x8051F058 (72 rows =
  SET `weapon` index), ammo/fire-rate/lock-on. `VEHICLES.md`: registry 0x8053C890 (SET_VEHICLE 1..13), entry/exit,
  physics, HP.
- Symbols: all investigation fragments merged → **1 307 curated names** (`symbols_curated.csv`), exported to
  `GAMEPLAY_SYMBOLS.csv` (8 586 rows).
- Correction made during integration: in `enemy/GUN_SOLDIER.md`, ReleaseCommand → unlock + AI slot 0x0D
  ReturnToWait (not Caution), verified at 0x8019E468..0x8019E480.
- At the original handoff, `build_and_run.ps1` passed **7/7 test suites**, and `stage_sim` stg0100 cleared Dark
  (35/35) in 9.4 s → stg0200.

GunSoldier patrol has since been recovered; see `enemy/GUN_SOLDIER.md` §5.5 and `src/GunSoldierAI.cpp`.

## 3. Toolkit (Python 3.14 on this machine; no Ghidra needed)

```bash
cd gameplay/tools
python q.py dis 0x80196E38          # annotated disassembly (names, strings, floats, vtables, arg-relative fields)
python q.py callers GunSoldier_Create
python q.py callees 0x801A7280
python q.py class "^Mission::"      # RTTI class: bases (offsets), vtables, slots, vptr writers
python q.py xref 0x8057E808         # code/data references to an address
python q.py off 41C W               # every function writing displacement 0x41C
python q.py fields 0x8019708C       # offsets accessed through r3 (this)
python q.py str "GunSoldierData"    # strings + referencing functions
python setparse.py 0100 cmn         # dump a stage SET file with named params (schema from the DOL)
```
Caches in `data/` are generated and **fine to reuse**: `program.pkl`, `dataflow.pkl`, `classes.json`,
`xref_index.pkl`, `setobj_catalog.*`, `func_families.json`. Rebuild order, only if you change a core tool:
`program.py → dataflow.py → rtti.py → setobj_catalog.py → classify.py`. Curated names go in `symbols_curated.csv`,
which is the source of truth and is read by the tools. Re-export with `python export_symbols.py`.

Key technical facts for reading code (all PROVEN):
- r2 = 0x805FA780, r13 = 0x805EC500. Our own Gekko decoder is `ppc.py`, because Capstone's paired-single mode
  mis-decodes `fcmpo`.
- MWCC RTTI: typeinfo `{name*, bases*}`; vtable `{typeinfo*, this_adjust, slots…}`; the vptr points at
  `&vtable.typeinfo`, so slot *i* is at `vptr + 8 + 4i`. Multiply-inherited objects have one vptr per subobject
  (GunSoldier example in `GAMEPLAY_STRUCTURES.md` §4). Resolve a virtual call `[[obj+X]+off]` as
  `u32(vptr_value_at_X + off)`. Thunks are named `thunk-N_target`.
- Runtime helpers: operator new 0x803A1380, delete 0x803A1334, memset 0x8000540C, memmove 0x800054F4,
  `__dynamic_cast` 0x803A1AFC.

## 4. Architecture in one screen (details in the docs)

- **Frame:** `fn_801E2210` → task tree root → layers, in the order System, Audio, Debug, **Scene**[Manager,
  Controller, Command, CharaColli, Landscape, Gadget, Vehicle, **Enemy**, **Player**, PostManager, Particle, Editor,
  Camera], PostSystem, Render. `Task`: vptr +0x18, Update = slot 1, kill bit = flags bit 0, scheduler 0x8004ECAC.
- **Stage objects:** SET files `files/stgNNNN/stgNNNN_{cmn,nrm|hrd,ds1}.dat` (little-endian `sky2`, 0x2C records).
  The DOL contains the full object catalog with named/typed params (0x8052C1A0, 286 types) and `setid.bin` stage
  masks. Objects spawn when a **camera** unit comes within range·100; the object is created via the descriptor
  hook +0x0C.
- **Enemies:** `EnemyTemplate<XBase,XAI>` component objects; lifecycle `TEnemySetBase::vf07` (states 1–4); AI
  state machine at AI+0x28 with singleton `TEnemyAIState`s; HP / team / type tables at 0x804CD678 / 0x804CD310.
- **Missions:** stage table 0x804C5AE8 (row = stage index) → mission keys per slot (Dark / Normal / Hero) → type
  map (defaults in code) + counts from `files/nukkoro2.inf` → `MissionManager` 0x80576FBC. Kills are counted by
  `EnemyManager` 0x80580BD8. Clear → `StageState` request 6 (`GoalAction`) → next stage from the table.
- **Player:** `Player::PlayerShadow` (0x29C); behaviors via `fn_800A82FC` + factory 0x80522DE8 (35 ids); physics
  integrator `fn_80093BBC` (exact linear-drag solution); command protocol in `GAMEPLAY_COMMANDS.md`.
- **Scoring/karma:** `files/common/ScoreData.bin` (115 entries) → `ScoreCommand` 0x1C → score counters and karma
  gauges (gauge 0 = hero/Chaos Control, 1 = dark/Chaos Blast).

## 5. Native C++ (`src/`, C++17) and tests

Modules: `Task`, `SetSystem` (+ generated `SetObjCatalog`), `EnemyFramework`, `EnemyAI` (state machine and exact
sensor math), `GunSoldierAI`, `GunBeetleAI`, `EggPawnAI`, `BkSoldierAI`, `BkLarva`, `Mission` (+ generated `StageTable`),
`StageFlow`, `PlayerPhysics`, `PlayerBehavior`.
Every function cites the PPC address it was recovered from. Engine services that are not reconstructed are
explicit interfaces (e.g. `GunSoldierBody`), never faked.

```powershell
cmake -S gameplay/src -B gameplay/build
cmake --build gameplay/build --config Release
ctest --test-dir gameplay/build -C Release   # 20 suites at checkpoint 15
```
Prefer `build_and_run.ps1` for the current complete source list and simulator gate.
Tests that read game data take the `files` root as argv[1] (CMake passes `gameplay/../../files`).
Regenerate the generated sources with `tools/gen_catalog_cpp.py` and `tools/gen_stage_table_cpp.py`.

### 5.1 Integration simulator `stage_sim` (`src/sim/stage_sim.cpp`)
Runs reconstructed modules together on real stage data (SET scan/spawn, task layers, native GunSoldier,
GUN Beetle, Egg Pawn and BK Soldier decisions, larva generator, EnemyManager kill counts, missions,
StageState). Harness services
are labelled in the source: a scripted attacker delivering PROVEN 2.0 homing-attack damage every
scripted 0.25 s, instantaneous motions, and stub bodies for enemy types without native code.
Checkpoint 15 results (`build/Release/stage_sim.exe ../files <stage>`):

| Stage | Mission | Result |
|---|---|---|
| 100 | Dark: 35 GUN | cleared → stg0200 (9.4 s simulated) |
| 301, 302, 401, 502, 601 | Dark enemy counts | cleared |
| 501, 504 | Hero enemy counts | cleared |
| 201 | Hero: 60 Black Arms | cleared → stg0302 (21.72 s simulated) |
| 404 | Hero: 50 Black Arms | cleared → stg0412 (21.10 s simulated) |

## 6. Corrections already applied (don't reintroduce)

- SET spawn and despawn ranges are measured from **camera** units, not the player.
- SET flag 0x8 is set at GunSoldier spawn (`SetSlot_SetFlag8`) and on normal enemy death
  by `TEnemySetBase::DetachKilled`. It is **not** itself a no-respawn veto: scanner
  0x800CACA4..0x800CACF4 preserves 0x8 when an external 0x40 re-arm is supplied.
  Normal killed records stay down because enabled 0x1 is cleared with no re-arm trigger.
  Checkpoint save converts 0x8 to 0x200; broader gameplay intent remains UNKNOWN.
- GUN soldiers never truly die from damage. At HP 0 they convert to the injured "Hinshi" state
  (AppearType 5 = HINSHI, from the game's own enum strings at 0x80539464).
- Some vtable-slot names carry the `@vtXXXXXXXX` suffix, because MWCC appends derived virtuals after the secondary
  tables. Real thunks are recognised by `addi r3,r3,-N; b target`.

## 7. Suggested next steps (priority)

1. Continue player behaviors (Jump / Fall / Ground / HomingAttack): bounded branches are native,
   but full update paths and world services are open. Constants and consumers are in
   `PLAYER_GAMEPLAY_RECOVERY.md` §5; framework in `src/PlayerBehavior.*` and `src/PlayerPhysics.*`.
2. Recover remaining enemy families beyond the four partial native AIs and larva generator,
   with labelled engine hooks and focused parity tests.
3. Remaining open items in `OPEN_QUESTIONS.md`: terrain geometry, 0x8057E828, stage-table
   unknown words, and world engine adapters. Chaos flags 0x22/0x23 setter is answered.
4. After each meaningful step: run `build_and_run.ps1`, then add a checkpoint to `HANDOFF.md`.

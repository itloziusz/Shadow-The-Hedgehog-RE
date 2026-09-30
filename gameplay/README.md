# Shadow the Hedgehog (GC, PAL GUPP8P) — gameplay recovery from `sys/main.dol`

> This module preserves the original local gameplay handoff and its relative
> `gameplay/` workflow. For the public repository's multi-module layout and
> build instructions, start at the root README.

Goal: recover the game's gameplay systems as named, typed, evidence-backed documentation and native C++,
not as a better disassembly. Start with **`HANDOFF.md`** (newest checkpoint first).

Rules: evidence first (PROVEN / STRONG / LIKELY / UNKNOWN); RTTI class names are original, method names are
recovered semantics; write only inside this folder (other folders in the repo are read-only references).

**Handing over to another agent (e.g. Codex)?** Read `Read this to continue Codex.md` first. A paste-ready prompt with agent workstreams is in `CODEX_NAVIGATION_PROMPT.md`.

## Documents

| File | Contents |
|---|---|
| `GAMEPLAY_DOL_MAP.md` | DOL layout, library bands, gameplay regions, anchors (Phase 1) |
| `GAMEPLAY_STRUCTURES.md` | Task, TaskManager, global state block, enemy component layout, SetManager, runtime helpers (Phase 2) |
| `GAMEPLAY_UPDATE_PIPELINE.md` | frame step → task tree → 19 named layers; stage bring-up; SET spawn; enemy per-frame path (Phase 3) |
| `PLAYER_GAMEPLAY_RECOVERY.md` | player creation/update, behavior state machine, physics constants + consumers, damage, input (Phase 4) |
| `ENEMY_BEHAVIOR_INDEX.md`, `enemy/GUN_SOLDIER.md` | all 27 enemy/boss types; full GUN Soldier trace spawn → AI → damage → injured → despawn (Phase 5) |
| `MISSION_SYSTEM.md`, `MISSION_PARAMETER_MAP.md` | mission framework; Westopolis "defeat 35 GUN soldiers" end to end; all-stage parameter table (Phase 6) |
| `STAGE_GAMEPLAY_DATA_FORMATS.md` | SET layout files, setid.bin, the in-DOL object catalog/param schema, spawn rules (Phase 7) |
| `CHECKPOINTS_AND_RESTART.md` | SavePoint checkpoint save and ContinueAction restore |
| `GAMEPLAY_COMMANDS.md` | inter-object command protocol (89 command types: damage, dead, score, spring, player actions…) |
| `WEAPONS_AND_TARGETING.md` | hit rule, damage values, weapon registry (SET `weapon` index), ammo, fire rate, lock-on |
| `VEHICLES.md` | vehicle registry (SET_VEHICLE 1..13), entry/exit, control/physics, HP |
| `enemy/BK_SOLDIER.md`, `enemy/EGG_PAWN.md`, `enemy/GUN_BEETLE.md`, `enemy/GUN_ROBOT.md`, `enemy/BK_WINGSMALL.md`, `enemy/BK_WORM.md`, `enemy/ENEMY_ALERT_PATHS.md` | bounded enemy decision slices, normal death path and alert-path analysis |
| `enemy/BK_LARVA.md` | SET 0x0091 generator, child life events, native scope and engine hooks |
| `COLLECTIBLES_AND_SCORING.md` | ScoreData.bin (115 entries), ScoreCommand → score counters / karma gauges |
| `GAMEPLAY_SYMBOLS.csv` | exported symbol DB (curated `symbols_curated.csv` + RTTI-derived) (Phase 8) |
| `OPEN_QUESTIONS.md` | prioritized list of unresolved facts (start here for new work) |
| `notes/open_questions_terrain_contact.md` | DOL-backed terrain contact bit/command chain and recovered Chaos-ready flag setter; unresolved geometry remains marked UNKNOWN |
| `notes/` | raw investigation logs (mission, enemy, player) with full evidence |

## Code

- `tools/` — Python RE toolkit (own Gekko decoder, dataflow, RTTI, SET parser, catalogs, query CLI). See `tools/README.md`.
- `src/` — native C++17 reconstruction (Phase 9): `Task`, `SetSystem`, `EnemyFramework`, `EnemyAI`, `GunSoldierAI`,
  `GunBeetleAI`, `GunRobotAI`, `EggPawnAI`, `BkSoldierAI`, `BkWingSmallAI`, `BkWormAI`,
  `BkLarva`, `PlayerPhysics`, `PlayerBehavior`, `StageFlow`,
  `Mission` + generated `SetObjCatalog`/`StageTable` from the DOL. Tests run on the real `files/` data:
  ```
  cmake -S gameplay/src -B gameplay/build
  cmake --build gameplay/build --config Release
  ctest --test-dir gameplay/build -C Release
  ```
- `build_and_run.ps1` — one command: build into `gameplay/build`, run all tests, run `stage_sim` (do this regularly).
- Player helpers recover bounded Ground/Fall/Jump/HomingAttack decisions, including
  `PlayerPhysics::ApplyJumpHold` at 0x80087384..0x800873F0. Full world and behavior updates remain open.
- `stage_sim` — all ten enemy-count mission stages clear through documented routes;
  route indices are asserted by CTest. GUN Beetle, Egg Pawn, BK Soldier and BK WingSmall use partial native
  decision AI; motion, weapon, path and locomotion remain labelled engine hooks.
  Latest full gate: **25/25 CTest suites**, plus stg0100 Dark 35/35 → stage index 6.
  stg0100 now has zero enemy stubs; BK WingSmall has 18 native SET spawns. GUN Robot and BK Worm
  have native focused tests but remain simulator stubs
  until their world-service adapters are connected.
- `data/` — generated caches and tables (regenerate with the tool pipeline in `tools/README.md`).

## Milestone status

- Enemy trace (GUN Soldier: spawn → init → AI/state → damage → "death" → despawn): **done** (`enemy/GUN_SOLDIER.md`).
- Mission trace (Westopolis Dark: stage init → parameters → events → progress → completion): **done**
  (`MISSION_SYSTEM.md` §9), and reproduced natively by `src/tests/test_mission.cpp` on real data.

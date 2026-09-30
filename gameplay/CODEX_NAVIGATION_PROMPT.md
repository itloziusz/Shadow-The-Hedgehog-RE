# Codex navigation prompt (paste everything below the line into Codex)

> Historical local-workspace prompt. The public repository's root README
> supersedes its paths and old test baseline; retain this for investigation
> provenance, not as current repository instructions.

---

You are continuing a reverse-engineering project on **Shadow the Hedgehog (GameCube, PAL GUPP8P)**. The goal is
to recover the game's gameplay systems from `sys/main.dol` as evidence-backed
documentation and native C++17, not as a better disassembly and not as an emulator.

## Hard rules
1. **Do nothing outside `gameplay/`.** No temp or scratch files elsewhere, no
   builds elsewhere (the build dir is `gameplay\build`), no writes to other folders. Game inputs are **read-only**:
   `..\sys\main.dol` and `..\files\`. Don't touch or run anything in `GekkoForge_RE_Toolchain\`.
2. **Evidence first.** Tag every claim PROVEN / STRONG / LIKELY / UNKNOWN with addresses. RTTI class names are
   original; function and method names are recovered semantics. Never fake unknown behaviour in C++: use explicit,
   labelled engine hooks or stubs.
3. **Build and run regularly:**
   `powershell -ExecutionPolicy Bypass -File gameplay/build_and_run.ps1`
   Expected baseline: 7/7 test suites pass, and `stage_sim` stg0100 prints
   `RESULT: Dark mission cleared -> next stage index 6 (stg0200)`.
4. **After every meaningful step:** add a checkpoint at the top of `gameplay\HANDOFF.md`, and update
   `gameplay\OPEN_QUESTIONS.md` and `gameplay\README.md` if they changed.

## Read first (in this order)
1. `gameplay\Read this to continue Codex.md`: full handoff (state, architecture, corrections, next steps).
2. `gameplay\README.md`: index of every document and code module.
3. `gameplay\OPEN_QUESTIONS.md`: prioritized unresolved facts.
4. `gameplay\tools\README.md`: toolkit (Python 3.14; run from `gameplay\tools`).
   Main query CLI: `python q.py dis|callers|callees|class|xref|fields|off|str <addr|name|regex>`.
   Reuse the caches in `gameplay\data\`. Don't rebuild them unless you change a core tool.

## Where things are
| Need | Go to |
|---|---|
| DOL layout, library bands, gameplay regions | `gameplay\GAMEPLAY_DOL_MAP.md` |
| Task scheduler, 19 update layers, per-frame order | `gameplay\GAMEPLAY_UPDATE_PIPELINE.md` |
| Structures (Task, global block 0x8057E760, enemy components) | `gameplay\GAMEPLAY_STRUCTURES.md` |
| SET stage data, object catalog, spawn rules | `gameplay\STAGE_GAMEPLAY_DATA_FORMATS.md`, `gameplay\data\setobj_catalog.txt` |
| Player (behaviors, physics constants, input, damage) | `gameplay\PLAYER_GAMEPLAY_RECOVERY.md` (+ `notes\player_trace.md`) |
| Enemies | `gameplay\ENEMY_BEHAVIOR_INDEX.md`, `gameplay\enemy\*.md` (GUN_SOLDIER is the reference) |
| Missions and parameters | `gameplay\MISSION_SYSTEM.md`, `gameplay\MISSION_PARAMETER_MAP.md` |
| Weapons, damage, lock-on / vehicles | `gameplay\WEAPONS_AND_TARGETING.md` / `gameplay\VEHICLES.md` |
| Commands (damage, score, player actions) | `gameplay\GAMEPLAY_COMMANDS.md` |
| Scoring and karma gauges / checkpoints | `gameplay\COLLECTIBLES_AND_SCORING.md` / `gameplay\CHECKPOINTS_AND_RESTART.md` |
| Symbol names | `gameplay\symbols_curated.csv` (source of truth) → `tools\export_symbols.py` → `GAMEPLAY_SYMBOLS.csv` |
| Native C++ + tests + simulator | `gameplay\src\` (`CMakeLists.txt`, `include\shadow\gameplay\*.hpp`, `tests\`, `sim\stage_sim.cpp`) |

## How to split work across agents
Give each agent one workstream, the files to read, and the files it may write. Agents must only write inside
`gameplay\`, name helper scripts `gameplay\tools\agent_<stream>_*.py`, and put new function names in
`gameplay\notes\<stream>_symbols.csv` (`address,current_name,subsystem,confidence,evidence`; commas inside
evidence written as `;`). Afterwards, the coordinator merges with
`python tools\merge_symbols.py notes\<stream>_symbols.csv`, then runs `python tools\export_symbols.py`.
Only the coordinator edits `HANDOFF.md`, `README.md`, `OPEN_QUESTIONS.md` and `symbols_curated.csv`.

| Stream | Task | Read | Write | Done when |
|---|---|---|---|---|
| patrol | GunSoldier patrol: `WaitAction_Moving::Update` 0x80188B98 over the patrol table E+0x288 (sub-state AI+0x24, point E+0x6C, timer AI+0x6C/+0x70) | `enemy\GUN_SOLDIER.md`, `notes\enemy_gunsoldier_trace.md` §2.6, `src\GunSoldierAI.*` | `src\GunSoldierAI.*`, a test in `src\tests\`, a section in `enemy\GUN_SOLDIER.md` | `PatrolStep()` hook replaced by the real logic; test passes |
| larva | `BkLarvaGenerator` (SET 0x0091, create hook 0x80189864; the census counts param `Num`) | `ENEMY_BEHAVIOR_INDEX.md`, `MISSION_SYSTEM.md` §5.1, `src\sim\stage_sim.cpp` | `enemy\BK_LARVA.md`, native code + sim factory | `stage_sim` stg0201 and stg0404 clear |
| enemies-native | Native BK Soldier / Egg Pawn / GUN Beetle from their docs; replace the sim stubs for those ids | `enemy\BK_SOLDIER.md`, `EGG_PAWN.md`, `GUN_BEETLE.md`, `src\EnemyAI.*`, `src\GunSoldierAI.*` | new `src\*AI.*`, tests, sim factories | the sim reports fewer stubs; all tests pass |
| player-native | Player behaviors Jump / Fall / Ground / HomingAttack on top of `PlayerBehavior` + `PlayerPhysics` | `PLAYER_GAMEPLAY_RECOVERY.md` §4–§5 | `src\Player*.*`, tests | behavior tests reproduce the documented constants and transitions |
| open-questions | P1/P2 items of `OPEN_QUESTIONS.md` (terrain collision, byte 0x8057E828, Chaos flags 0x22/0x23, unknown stage-table words) | `OPEN_QUESTIONS.md` + the linked docs | `notes\<topic>.md` | each item answered with evidence or explicitly left UNKNOWN |

## Finish protocol
Run `build_and_run.ps1` (all green), merge the symbol fragments, update `HANDOFF.md` with a new checkpoint (what
changed, verification result, next steps), and keep `Read this to continue Codex.md` current for the next agent.

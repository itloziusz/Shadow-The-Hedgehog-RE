# BK_LARVA (SET 0x0091) — PAL GUPP8P

Evidence source: `../sys/main.dol`, queried through `gameplay/tools/q.py` and `ppc.py`; SET values through the existing `setparse.py` cache. The RTTI names `BkLarvaGenerator`, `BkLarva`, and `SBkLarvaStateData` are original. Descriptive method names below are recovered semantics.

## Identity and parameters

- **PROVEN** — SET catalog descriptor `0x80531924` has create hook `0x80189864`. It allocates `0x150` bytes at `0x80189878` and constructs `BkLarvaGenerator` at `0x801898A0` under task layer `0xE` (Enemy). RTTI at `0x805E876C` gives bases `Task` and `SetEnemyBase`; it is not an `EnemyTemplate`.
- **PROVEN** — The generator loads `0x2C` parameter bytes from its SET slot at `0x80189A10..0x80189A1C`. `Num` is word seven, generator offset `+0x74` (SET parameter offset `+0x1C`); the in-DOL editor schema at `0x80531924` names it `Num`, default 10, minimum 1, maximum 100. Census routine `0x801A2BA0` adds that value to the Black Arms placed count and link group alive count (see `MISSION_SYSTEM.md` §5.1).
- **PROVEN** — Constructor `0x8018A3A8..0x8018A3AC` sets SET runtime flag `0x8`. Destructor `0x80189F84..0x80189F98` calls `SetSlot_Detach` after its state-adapter/checkpoint path. The checkpoint branch at `0x80189EF0..0x80189F14` is only entered when generator `+0x148 <= 0`; its full native reconstruction remains open.

## Generator lifecycle

| Original state at `+0x84` | Evidence | Recovered behavior |
|---|---|---|
| 0 | `0x80189BC0..0x80189C10` | **PROVEN:** waits for stage/world predicates (`fn_80012408` equals 3 and `fn_801780AC` true), builds a spatial trigger through `fn_8003A3A8`, then sets state 1. Exact stage-service and spatial-trigger semantics are **UNKNOWN**. |
| 1 | `0x80189C14..0x80189C84` | **PROVEN:** polls the trigger handles using `fn_8003A524`; on a positive result frees the handle array and enters state 2. |
| 2 | `0x80189C88..0x80189CC8` | **PROVEN:** postincrements spawned count `+0x140`, allocates `0xD0` bytes, constructs one `BkLarva` with generator and ordinal, then compares spawned count to `Num` at `+0x74`. If the count reaches `Num`, enters state 3. This happens at most once per generator update. **PROVEN:** malformed `Num <= 0` still requests one larva because the increment/allocation precede the comparison; the SET schema normally forbids this. |
| 3 | `0x80189DB8..0x80189DE0` | **PROVEN:** counts descendants and marks the generator task for deferred death when none remain. |

The generator's helper at `0x801899E0..0x80189B1C` reads position, rotation, and parameters from the SET slot; reads `SBkLarvaStateData` if present at slot `+0x14`; initializes the search sensor. **PROVEN:** generator reset `0x80189B28..0x80189B5C` kills children and resets state and spawned count. **UNKNOWN:** complete save/restore behavior, random trigger placement, and the engine's stage-gate rules.

## Larva and defeat

- **PROVEN** — `BkLarva::BkLarva` at `0x8018BF1C` is a child task under the generator (parent is `r4`). It stores its ordinal at `+0x28`; `ordinal % 5` selects one of five shared effects at `0x8018BF84..0x8018BFA0` and `0x8018C108..0x8018C130`.
- **PROVEN** — Its task update at `0x8018AF18..0x8018B4C8` has six numbered states (`+0xCC`, 0–5), including position/motion and delayed death. State 4 at `0x8018B2C8..0x8018B36C` sets a 0.5 s timer, decrements generator `+0x148`, and enters state 5. State 5 requests task death after its timer becomes negative (`0x8018B374..0x8018B3E4`); the timer is decremented at `0x8018B498..0x8018B4A0`. Full motion, collision detection, and effects are **UNKNOWN** in the native implementation and go through an explicit `BkLarvaEngine` hook.
- **PROVEN** — Collision callback `0x8018A7D0..0x8018A9F8`, installed through function pointer data at `0x80531728`, calls `EnemyManager_OnEnemyLifeEvent` at `0x8018A9CC` with the generator's SET slot, team 2, death flag 1, and flag 4 when `Attacker_IsPlayerAttack` is true (`0x8018A99C..0x8018A9A8`). It enters larva state 4. Command callback `0x8018AA84..0x8018ACA0` handles command `0x14` and makes the same life-event call at `0x8018ABE0`; it decrements generator `+0x148` at `0x8018ABE4..0x8018ABF8`. The difference between the two paths matters for checkpoint state.
- **PROVEN** — `EnemyManager::OnEnemyLifeEvent` at `0x801A250C` advances Black Arms defeated count for every death flag, and by-player count only for flag 4; the group alive count decreases by one. Thus each killed child, not its SET generator record, contributes one mission defeat.

## Native scope and simulation

`src/BkLarva.cpp` reconstructs the parameter load, task-parent relation, one-child-per-update loop, life-event call, delayed state-4/5 cleanup, and stop-after-last-child condition. `BkLarvaEngine` explicitly supplies stage gating, spatial trigger, larva motion, and state-adapter cleanup because those systems have not been recovered. Collision delivery calls `BkLarva::ReportDefeat` once per larva; the engine remains responsible for deciding whether a collision occurred. Command `0x14` has a separate `ReportCommand14Defeat` entry because the DOL requests immediate task death on that path. The simulator's `SimLarvaEngine` is a **HARNESS STUB**: its scripted attacker opens the selected generator trigger and reports a hit every 0.25 s. This timing and world interaction are not game behavior.

The generator destructor performs only the native `SetSlot_Detach` field subset established at `0x800C9FE4..0x800CA09C`: clearing alive/despawn bits and the attached pointer. Owned adapter-state deletion and link-list maintenance have no recovered native service yet. The hook `OnGeneratorDestroyed` marks the checkpoint/state-adapter boundary explicitly.

The simulator counts a generator by its actual `Num`, spawns that many child tasks, and reports one defeat per scripted child hit. `src/tests/test_bklarva.cpp` uses Release-active checks for closed gates, trigger transition, one-per-frame spawn, `Num=3`, no fourth child, `Num=0` postincrement behavior, player attribution, duplicate-event suppression, and malformed parameter refusal. stg0201 Hero 60/60 and stg0404 Hero 50/50 clear in the simulator with exact routes asserted by CTest. Their actual SET records include `Num` values 3, 4, 5, and 10 (PROVEN from stage SET parsing).

**Still UNKNOWN:** exact stage gate/trigger meaning, spatial motion and attack, player collision condition, command `0x14` effects beyond the death report, state-adapter checkpoint transitions, and resource/effect behavior. The native implementation exposes those seams instead of substituting inferred behavior.

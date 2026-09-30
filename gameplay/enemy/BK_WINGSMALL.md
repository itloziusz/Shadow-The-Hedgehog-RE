# BK WingSmall — PAL GUPP8P bounded recovery

The original RTTI classes are `BkWingSmall`, `BkWingSmallBase`,
`BkWingSmallAI`, and `EnemyAIState_BkWingSmallAI_{MoveOnPath,Caution,Attack}`.
Names such as `ReturnToWait` and `OnTargetFound` below describe recovered
semantics, not original symbols. This document covers the native decision slice
in `src/BkWingSmallAI.cpp`; engine movement, path, display and attack services
remain hooks.

## Object and parameters

| Claim | Confidence | Evidence |
|---|---|---|
| SET `BK_WINGSMALL` id 0x008F creates a 0x420-byte `BkWingSmall` task in Enemy layer 14 | PROVEN | descriptor 0x80534BA0 create hook 0x80191640..0x80191684; ctor 0x8019185C |
| `BkWingSmallBase` derives from `FlyerCommonBase`, `EnemyFlyerCommon`; `BkWingSmallAI` derives from `FlyerCommonAI` | PROVEN | RTTI typeinfos 0x805E7FF8/0x805E8AD4 and 0x805E7F40/0x805E8A1C; vtable 0x8052F794 |
| Type id 7 has base HP 2 and team 2 (Black Arms) | PROVEN | type constructor chain summarized in `ENEMY_BEHAVIOR_INDEX.md` §2.1; tables 0x804CD678 and 0x804CD310 |
| SET block has `AppearType`, `ActionType`, `FloatWidth`, `AttackStart`, `AttackEnd`, `PathMirror` following shared flyer parameters | PROVEN | descriptor 0x80534BA0 schema in `data/setobj_catalog.txt`; 0x80191B18..0x80191B50 copies 0x38 bytes into E+0x310 and calls 0x80193B44 |
| `AppearType == 1` selects the path; `ActionType == 1` enables its attack window | PROVEN | E+0x32C test 0x801822E4; E+0x330 test 0x8019162C; primary vtable +0x238/+0x23C from 0x8053579C |

## Decision path

1. **PROVEN** — `BkWingSmallAI::ReturnToWait` at 0x801825D0
   changes to family `MoveOnPath` when the E+0x32C predicate succeeds,
   otherwise the inherited `FlyerCommonAI_WaitFloating` state. `EnemyBaseAI`
   init delegates through this vtable method at 0x8019D894/0x8019EEC4.
2. **PROVEN** — `MoveOnPath::Enter` 0x80181E68 sets AI+0x24 to zero,
   requests an engine motion, and writes 1.0 to E+0x2F8 and E+0x2FC.
   `MoveOnPath::Update` 0x80181D3C checks the inclusive path window only
   while substep 0. When true it increments the substep and changes to
   `Caution`. It still calls `FlyerCommon_AdvanceOnPath` that same update.
   The window helper at 0x80193AD4 requires `ActionType == 1` and
   `start <= E+0x2E4 <= end`. `FlyerCommonBase_ReadSetParams`
   0x80193BBC..0x80193C78 derives start/end by multiplying path length
   and uses `(end-start)*length/speed` for duration when speed > 0.1,
   otherwise 1.0. Native `ConfigurePath` requires engine supplied path
   length and speed; it never invents path geometry.
3. **PROVEN** — inherited `WaitFloating::Update` 0x80183F30 tests the
   search-area sensor, turns toward the target when detected, and invokes
   `OnTargetFound` only when the `ActionType == 1` predicate succeeds.
   With no detection it turns to the spawn heading. Float motion only runs
   when E+0x2DC `FloatWidth > 0.1` (0x80184028..0x8018403C); its world
   transform update remains a body hook. `OnTargetFound` at 0x80182598
   selects `Caution`.
4. **PROVEN** — `Caution::Enter` 0x80182518 requests a motion and SE
   0x603F. `Caution::Update` 0x801823EC turns toward target, then reads
   status motion flag A (S+0x3A via vtable +0x2C, implementation
   0x8017BAB4). A true flag changes to `Attack` immediately.
5. **PROVEN** — `Attack::Enter` 0x801822F8 requests engine motion,
   transforms/sets a home position, plays SE 0x603D, and writes 3.0 to
   AI+0x6C/+0x70 and zero to AI+0x24. Its update 0x80181EFC..0x801822B8
   contains four substeps and movement-vector work. **UNKNOWN in native
   scope:** exact adapter behavior for that transform, model motion, and
   world displacement. `UpdateAttackMotion` is explicitly an engine hook,
   so the native module does not manufacture an attack trajectory.
6. **PROVEN** — `FlyerCommonAI::OnDead` 0x80184318 plays the enemy death
   effect and requests deletion in the same frame. `BkWingSmallAI` inherits
   that vtable entry at 0x8052F794 slot 0x14; the native body exposes both
   operations as engine hooks.

`FlyerCommonBase::UpdateActivation` 0x80193CAC gates path-mode visibility
on the search-area sensor; float mode is active without that gate (PROVEN).
The native `ShouldActivate()` exposes this decision; the engine applies
visibility and activity.

## Validation and limits

`src/tests/test_bkwingsmall_ai.cpp` checks the path and float dispatch,
ActionType 1 gate, both inclusive window boundaries, same-frame path advance,
motion flag A transition, attack entry timer, and same-frame flyer deletion.
Its checks remain active in Release. `Attack::Update` delegates to an
explicit hook, and `EnemyPath.one` geometry, motion flags, search delivery,
world transforms and effects are not claimed as native parity.

## Simulator integration

`src/sim/stage_sim.cpp` reads SET params 7/8/10 for AppearType, ActionType and
FloatWidth, creates the native decision AI, and supplies explicitly labelled
no-op/instant engine services where the reconstructed module requires world
geometry, flight or motion. This is a harness integration, not activation or
attack parity. The `stage_sim_stg0100_wing_native` gate asserts 18 native
BK WingSmall spawns; stg0100 has zero enemy stubs and still clears Dark 35/35.
All 25 Release CTest suites passed at checkpoint 19.

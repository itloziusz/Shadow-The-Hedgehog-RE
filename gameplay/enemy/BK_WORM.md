# BK_WORM — bounded native decision loop (PAL GUPP8P)

`BkWorm`, `BkWormAI` and the six `EnemyAIState_BkWormAI_*` names below are **original RTTI** names.
Method names in `src/BkWormAI.cpp` are **recovered semantics**. The module reconstructs the
AI state decisions; it does not claim the worm's terrain displacement, animation or effect
services. Those are explicit `BkWormBody` hooks.

## Identity and parameters

- **PROVEN** — SET 0x0090 `BK_WORM`, descriptor 0x80535BEC, create hook 0x801920AC,
  allocation 0x3A0 and `BkWorm` constructor 0x801923C8. RTTI `BkWorm` derives from
  `EnemyTemplate<BkWormBase,BkWormAI>` and `EnemyMoveWalker`; the `BkWormAI` vtable
  starts at 0x8052FB74. See `data/enemy_index.json` and `q.py class '^BkWorm'`.
- **PROVEN** — `BkWorm::vf04` 0x8019218C copies 0x28 bytes of SET parameters to
  E+0x270. Thus E+0x290 is `AttackTimes` (SET param 8, default 1) and E+0x294 is
  `IntervalSec` (param 9, default 3.0); names/defaults are in the DOL SET catalog
  0x8052C1A0, extracted in `data/setobj_catalog.txt`. Type id 8 indexes max HP 12
  and Black Arms team in 0x804CD678 / 0x804CD310.
- **PROVEN** — AI ctor 0x80183940 writes AI+0x7C = 1. Initial entry 0x801838E4
  selects `WaitAppear` after a display call. Return-to-wait 0x8018388C selects
  `WaitAppear` while AI+0x7C is nonzero and `WaitStanding` otherwise.

## Six-state decision loop

`EnemyBaseAI::PreUpdate` 0x8019EB70..0x8019EB7C subtracts frame `dt` from AI+0x6C
**before** the current state's update. Every timer transition below uses strict
`timer < 0`, so equality does not transition. AI+0x70 mirrors each timer reset.

| RTTI state | Entry and decision evidence | Native scope |
|---|---|---|
| `WaitAppear` | **PROVEN** 0x801835EC sets AI+0x7C=1, timer from E+0x294, substep 0. Update 0x8018342C: substep 0 checks `EnemySensor_IsInSearchArea` then timer<0; triggers effect at current position, resets timer to 1.0, advances to substep 1. That substep waits for timer<0 then enters `Appear`. | `EnemySensor::Detect`, timer and transition native; display/effect hooks. |
| `Appear` | **PROVEN** 0x801832FC clears AI+0x7C. Update 0x801831D0 turns toward cached target direction; display motion-ended vslot +0x2C sends it to `WaitStanding`. | Decision native; rotation and display hooks. |
| `WaitStanding` | **PROVEN** 0x80183144 draws timer in [1,3] via `fn_8040FCD0`; 0x801830BC turns, waits for timer<0, copies E+0x290 to AI+0x78, enters `Attack`. | Timer, attack budget and transition native; RNG/animation hooks. |
| `Attack` | **PROVEN** 0x80183074 begins motion id 3; 0x80182F48 turns and sends motion-ended to `AfterAttack`. | Decision native; motion and turn hooks. |
| `AfterAttack` | **PROVEN** 0x80182EBC draws timer in [0.8,1.2]; 0x80182CBC turns, then on timer<0 predecrements AI+0x78. Count <1 enters `Move`, otherwise another `Attack`. | Counter/timer branches native; RNG/animation hooks. |
| `Move` | **PROVEN** 0x80182BF8 locks the state machine, draws [0.8,1.2], chooses/sets a new home and resets substep. 0x80182964 step 0 turns toward E+0x6C minus current position; only a true return starts travel, sets AI+0x7C=1 and advances. Step 1 waits for display motion-ended, completes world/home operations, unlocks, enters `WaitAppear`. Exit 0x8018293C also unlocks. | Lock, two substeps and transition native; home choice, travel and effect hooks. |

**PROVEN** — `EnemyAITarget_GetPosition` 0x8019FA18 returns AI-target+0x0C,
which `fn_8019FB08` fills with target-world-position minus worm-world-position
(0x8019FB24..0x8019FB5C). `EnemyAI_TurnTowards` 0x8019F8F8 accepts that direction,
not an absolute world point. `Move` explicitly computes the direction from E+0x6C
and current position at 0x80182984..0x801829A0.

**PROVEN** — `WaitAppear` is the only state in this six-state loop that calls the
search sensor (0x80183470..0x80183484); the visible `Appear`, standing and attack
updates do not recheck detection. This is only a statement about these six update
functions, not every command or status path.

## Limits and verification

- **UNKNOWN** — the exact geometry and terrain conditions in `fn_801836B4`, which
  chooses a Move destination, and the final displacement in `fn_8019B3BC` / display
  operations. `BkWormBody::BeginMove` and `CompleteMoveTravel` are engine hooks; the
  native AI does not invent a route or teleport.
- **UNKNOWN** — display motion selection (`GetTeam`/display vslots), burrow effect
  resources and sound delivery. Hooks keep those calls visible. `BkWormAI::vf14`
  0x80183828 has a family-specific death effect, outside this bounded decision loop.
- `src/tests/test_bkworm_ai.cpp` uses checks that remain active in Release. It covers
  equality timer boundaries, two attacks, the strict final predecrement, a Move turn
  that initially fails, the state lock/unlock, and return-to-wait selection. A passing
  harness test proves this bounded native logic, not full enemy or pixel parity.

# GUN Robot (`GunRobotTask`, SET `GUN_ROBOT` 0x0068)

Source: PAL GUPP8P `sys/main.dol`, read only. RTTI class names below are original;
function names describe recovered semantics. Confidence terms: PROVEN = direct PPC/data,
STRONG = multiple consistent observations, LIKELY = indirect, UNKNOWN = unresolved.
This document covers a bounded decision AI, not the world engine.

## Identity and layout

| Fact | Evidence | Confidence |
|---|---|---|
| SET 0x0068 create hook allocates 0x3B0 and constructs `GunRobotTask` on Enemy layer 14 | 0x801952F8..0x80195338; descriptor 0x805384AC | PROVEN |
| `GunRobotTask` contains `GunRobot` E+0 and `GunRobotAI` E+0x2F8, derived from `EnemyWalkerAI` | ctor 0x801955D8..0x801956BC; RTTI typeinfos 0x805E8EBC / 0x805E8428 | PROVEN |
| Enemy type 3 has max HP 8 and team 0 (GUN) | ctor chain and tables 0x804CD678 / 0x804CD310; `ENEMY_BEHAVIOR_INDEX.md` §2.1 | PROVEN |
| Param copy takes 27 words from the SET record to E+0x270 | 0x80196454..0x80196460 | PROVEN |
| AppearType E+0x28C, WeaponType E+0x290, BodyType E+0x294, WaitActMoveType E+0x2A4 | catalog indices 7/8/9/13; 0x80195890 and 0x8019575C | PROVEN |
| Nonzero AppearType adds OffsetPos X/Y/Z E+0x298..0x2A0 to spawn position; type 2 gets a 1.0 timer | 0x80196464..0x801964C8 | PROVEN |
| `WeaponType` enum labels beyond catalog integer range 0..5 | no matching original enum strings established | UNKNOWN |

The common walker sensor is at E+0xA8; the native body keeps target binding and
search tests as explicit services. SET fields and defaults are in
`data/setobj_catalog.txt` under `0068 GUN_ROBOT`.

## AI dispatch

`GunRobotAI` vtable 0x80530D2C (RTTI) overrides slots 0x0C, 0x0D, 0x0F,
0x10 and 0x14. The AI state machine is AI+0x28; per-state substep is AI+0x24.

| Branch | Result | PPC | Confidence |
|---|---|---|---|
| Initial AppearType 0 | dispatch by WaitActMoveType | 0x80186868..0x801868DC | PROVEN |
| Initial AppearType 1..3 | shared `EnemyWalkerAI_ReturnHome` | 0x8018691C..0x80186954 | PROVEN |
| Other AppearType, including negative | shared standing wait | 0x80186870..0x80186884; 0x80186958..0x8018698C | PROVEN |
| Return to wait: WaitActMoveType 0 / nonzero | shared standing / moving wait | 0x801867A0..0x80186834; accessor 0x8019575C | PROVEN |
| Wait detects player | `GunRobotAI::OnTargetFound` enters **base** `EnemyBaseAI_Caution`, distinct from sibling Egg Pawn's walker caution | 0x801866CC..0x80186718; shared wait detection 0x8019A31C / 0x80199EAC | PROVEN |
| AI slot 0x10 | enters `GunRobotAI_WeaponAttack` singleton | 0x801865E8..0x80186620 | PROVEN |
| AI slot 0x14 at death | invokes display and status services without a state change in this function | 0x80186588..0x801865D0 | PROVEN |

The shared standing/moving waits and return-home movement remain engine adapters in
`GunRobotBody`. The original shared base Caution has its own multi-step update at
0x8019C544..0x8019C780; the native module delegates that entire state explicitly to
`BeginBaseCaution` / `UpdateBaseCaution`. Its effects are **UNKNOWN** in this native
slice. This prevents an invented 1-second walker-Caution transition from being applied
to this different state.

## Family state `EnemyAIState_GunRobotAI_WeaponAttack`

RTTI at 0x805E84F0; state vtable 0x80530D9C. The update dispatches only substeps
0/1/2; other values leave the state unchanged. **PROVEN** at 0x8018631C..0x80186348.

| Step | Native decision and required engine service | PPC | Confidence |
|---|---|---|---|
| Enter | start weapon-dependent attack motion with restart=true; set AI substep 0 | 0x80186528..0x80186570 | PROVEN |
| 0 | wait for display motion end; then play motion 0, pick a point with radius 30 around the current enemy position, set it as movement target, advance to 1 | 0x80186348..0x801863F4; `GunRobot::vf1E` 0x80195AD0 returns 0 | PROVEN |
| 1 | get tracked target position, test search area; if lost, ReturnToWait immediately; otherwise advance movement. On arrival face the tracked target and advance to 2 | 0x801863FC..0x80186490 | PROVEN |
| 2 | call `Enemy::TurnToTargetYaw`; only when it returns true restart the attack motion and reset substep 0 | 0x80186498..0x801864FC; turn helper 0x8019AFB0 | PROVEN |

`GunRobot::vf21` 0x80195A50 selects attack motion by WeaponType: 0,1,5 → 4;
2,3,4 → 5; all other integers → 2. **PROVEN.** Weapon behavior, shot timing,
projectiles, random-point terrain constraints, animation completion, locomotion and
target acquisition remain **UNKNOWN** to the native decision module and are supplied
through labelled `GunRobotBody` hooks. The native code does not claim weapon parity.

## Native verification and next work

`src/GunRobotAI.cpp` implements the bounded branches above and exposes shared walker,
base caution, display and movement services as hooks. `src/tests/test_gunrobot_ai.cpp`
uses persistent `CHECK` failures under `NDEBUG`; it checks initial and wait dispatch,
player detection, attack motion mapping, substep gates and order, lost-target exit,
and the death callback's lack of an AI state transition. The isolated Release
`clang++ -O2 -DNDEBUG -Wall -Wextra -Werror` test passed. The coordinator still needs
to add the module/test to CMake, run the full `build_and_run.ps1` gate, and consider a
labelled simulator factory. Full shared base Caution, activation, weapons and world
services remain open.

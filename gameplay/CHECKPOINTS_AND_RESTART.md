# CHECKPOINTS_AND_RESTART — SavePoint checkpoints and "continue" restore

## 1. Reaching a checkpoint (STRONG)

SET object 0x0005 `SAVEPOINT` (param `priority` 1..31): create hook 0x801177D8 → `new(0x58)`
`SavePoint::SavePoint::SavePoint` in layer 11 "Gadget" (vtable 0x80528004; update = vf02 0x80118154).
The checkpoint callback **`fn_80119350` (Checkpoint_Save)** is referenced from `fn_80117B70` (address
taken at 0x80117BB4 and passed as an argument → registered callback, STRONG). It does, in order:

| Step | Call | Effect | Confidence |
|---|---|---|---|
| 1 | fn_800F35BC/fn_800F3658/fn_8040C* | build respawn position/orientation from the save point | LIKELY |
| 2 | `fn_8016F7B8(game, pos, …)` | store respawn point | LIKELY |
| 3 | `fn_801D59AC(sound, 0xE013, 0, 0)` | checkpoint sound | LIKELY |
| 4 | fn_80074C14 | player-side checkpoint save | UNKNOWN detail |
| 5 | `MissionManager_SaveCheckpoint` 0x8016E3DC | mission states/counters → checkpoint copies | PROVEN (see MISSION_SYSTEM.md §8) |
| 6 | `SetManager_Get` → fn_800CB8F0 → **fn_800CA480** | every SET record with flag 0x8 gets **0x200** | PROVEN |
| 7 | `EnemyManager_SaveCheckpoint` 0x801A27B0 | defeated counters → +0x4C/+0x58 | PROVEN |
| 8 | fn_800CBA84 → fn_801694DC | link-event state save | LIKELY |
| 9 | fn_80178F10(…,0) → fn_8016F664(stats+0x28) | ring count snapshot | LIKELY |

## 2. Continue from checkpoint — `StageAction::ContinueAction::vf00` 0x802046F8 (PROVEN call order)

1. `SetManager_Get` → fn_800CBB40 → **fn_800CA57C**: every SET record gets **0x4** (despawn request);
   then `fn_801EB5D4(TaskManager)` marks-kill the whole **Gadget (11), Vehicle (12) and Enemy (14)** layers
   (manager +0x30/+0x34/+0x3C); `SetManager.loaded = 0`.
2. fn_80074D44, fn_8012D448, fn_80166130 (player / object subsystems restore — UNKNOWN detail).
3. `SetManager_Get` → fn_800CBA3C → **fn_800CA4B4 (SetData_RearmAfterContinue)**, per record:
   - authored flags (+0x1C) has 0x200, or runtime has 0x200 → record left untouched;
   - authored 0x400 → `rt = authored & ~1` (comes back disabled);
   - runtime 0x2000000 → `rt = authored | 0x2000000` (bit preserved across continue);
   - otherwise `rt = authored` and the slot is reset (fn_800C9C7C);
   then link lists rebuilt (fn_800CAEAC); `SetManager.loaded = 1`; fn_800CBA84 → fn_80169558 (link events restore).
4. `StageState_RequestAction(11 = TryAgainAction, force 1)`, then player/stage re-init (fn_80177268(1)…).

Consequence (PROVEN from 1+3): records carrying flag 0x8 at checkpoint time keep 0x200 and are not reset by the
continue; all others return to their authored flags; all live gadgets, vehicles and enemies are torn down and
re-spawned by the normal SET range scan. Because GunSoldier sets 0x8 at spawn (see enemy/GUN_SOLDIER.md), the
exact player-visible meaning of 0x8/0x200 is still UNKNOWN (candidate: "already encountered").

New SET runtime flag meanings from this trace (add to STAGE_GAMEPLAY_DATA_FORMATS.md table):
`0x200` flag-0x8-at-checkpoint (persistent), `0x400` (authored) disabled after continue,
`0x2000000` preserved across continue (meaning UNKNOWN).

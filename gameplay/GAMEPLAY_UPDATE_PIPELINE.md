# GAMEPLAY_UPDATE_PIPELINE — how gameplay runs each frame

All addresses are in `sys/main.dol`. Confidence: PROVEN / STRONG / LIKELY / UNKNOWN.
Names in `code` without a class prefix are recovered semantic names (not original).

## 1. Summary diagram (only evidence-backed edges)

```text
MainLoop                                   fn_80049078                      STRONG
 └─ Game_FrameStep()                       fn_801E2210                      PROVEN
     ├─ pad/video: fn_80486EC0/fn_80486E34 → bss 8057E7A4..; fps = PAL?50:60 (fn_803071F4) → bss 8057E7B0
     ├─ fn_801E24D0, fn_8035A2DC, fn_801E1F2C (per-frame system services)            UNKNOWN detail
     ├─ dt = (float) → sbss 805F0A58  (also copied to 805EF248)
     ├─ fn_80365840 / fn_80416838 (audio/movie time update)                            LIKELY
     ├─ TaskManager_Run(TaskManager_Get(), dt)   fn_801EB680 → fn_8004ECAC(root, dt)    PROVEN
     │    Root task (Task, name via 0x8053ED20)
     │     ├─ [0] System
     │     ├─ [1] Audio
     │     ├─ [2] Debug
     │     ├─ [3] Scene
     │     │    ├─ [6]  Manager      SetManagerTask (spawn/despawn), TStageInitalizeTask, mission manager(?)
     │     │    ├─ [7]  Controller
     │     │    ├─ [8]  Command      CharCommandManagerTask (inter-object command queue)
     │     │    ├─ [9]  CharaColli   CharaColliManageTask (character collision / attack callbacks)
     │     │    ├─ [10] Landscape
     │     │    ├─ [11] Gadget       stage objects (SET gadgets)
     │     │    ├─ [12] Vehicle
     │     │    ├─ [14] Enemy        TEnemySetTask of every enemy (e.g. GunSoldier)
     │     │    ├─ [13] Player
     │     │    ├─ [15] PostManager
     │     │    ├─ [16] Particle     Effect::PJS::EffectTask
     │     │    ├─ [17] Editor
     │     │    └─ [18] Camera
     │     ├─ [4] PostSystem
     │     └─ [5] Render
     ├─ fn_801E41D4(bss 805F0A9C): TaskManager fn_801EB674 / fn_801EB668                UNKNOWN
     ├─ GameSystem mode object (sbss 805F0A50): vslot@+0x10 → if done: vslot@+0x14, delete (vslot0),
     │   then 3 extra TaskManager_Run passes (flush tasks killed by the mode change)    PROVEN (control flow)
     └─ fn_80043724, fn_801E2480, fn_801E5B70, fn_800BB1D0/fn_801E3238/fn_801E31EC (render/present)  UNKNOWN
```

## 2. The Task scheduler (PROVEN)

`Task` is the universal update node (RTTI class `Task`, vtable 0x8051E768: slot0 dtor 0x8004EF18,
slot1 `Update(float dt)` default = `blr` at 0x8004EA80).

`Task::Task(Task* this, Task* parent)` — 0x8004F014:
- `+0x00 = *(r13-0x7AC8)` default name; `+0x04 flags = 0`; `+0x10 parent`; `+0x14 firstChild = 0`;
  `+0x18 vptr`; `+0x20/+0x24 = 0` (u64 profiling ticks)
- links at the **tail** of `parent->firstChild` list: `+0x08 prev` (head's prev = tail), `+0x0C next`
- global live-task counter `sbss 805EF278++`, guarded by fn_80050C40 / fn_80050BF8 (lock/unlock).

`Task_UpdateChildren(Task* parent, float dt)` — 0x8004ECAC (recursive):
```cpp
// reconstructed from 0x8004ECAC; flags: bit0 kill-request, 0x20 destroying, 0x100 run-while-paused
void Task_UpdateChildren(Task* parent, float dt) {
    ++g_taskDepth;                                   // sbss 805EF270
    Task* t = parent->firstChild;
    while (parent->firstChild && t) {
        u16 f = t->flags;
        if (f & 0xF) {                               // pending state bits
            if ((f & 1) && !g_taskPauseAll) {        // sbss 805EF27C (u8)
                t->flags = f & ~1;
                if (!(t->flags & 0x20)) {            // not already destroying
                    Task* victim = t;  t = t->next;
                    while (victim->firstChild) {     // kill subtree first
                        Task_MarkSubtreeKill(victim);     // 0x8004EECC: sets bit0 on all descendants
                        Task_UpdateChildren(victim, dt);  // destroys them
                    }
                    victim->vtbl->deletingDtor(victim, 1);   // slot 0
                    continue;
                }
            }
        } else {
            if (!g_taskPauseAll || (f & 0x100)) {
                if (g_taskProfile)  Task_UpdateProfiled(t, dt);  // 0x8004EDF0 (OS timebase → +0x20)
                else                t->vtbl->Update(t, dt);      // slot 1
            }
            Task_UpdateChildren(t, dt);
        }
        t = t->next;
    }
    --g_taskDepth;
}
```
Consequences (PROVEN): update order is depth-first, parent before children, siblings in creation order;
killing is deferred (set flag bit0, destroyed on the next traversal).

## 3. Task layers (PROVEN)

`TaskManager` singleton (`fn_8001C4F8`, storage bss 0x80571C6C, ctor 0x801EB734):
`+0x00 root`, `+0x04 + 4*i` layer i. The ctor creates the root, then 6 layers under the root from index
table 0x8053ED6C = {0,1,2,3,4,5} (fn_801EB53C) and 13 layers under layer 3 from table 0x8053ED84 =
{6,7,8,9,10,11,12,14,13,15,16,17,18} (fn_801EB4A4). Each layer gets flag 0x100 and its name from
0x8053ED20. Names: System, Audio, Debug, Scene, PostSystem, Render, Manager, Controller, Command,
CharaColli, Landscape, Gadget, Vehicle, Player, Enemy, PostManager, Particle, Editor, Camera.

**Enemy (14) is created before Player (13)**, so enemies update before the player each frame.

## 4. Stage bring-up (STRONG)

`StageManager::vf01` (0x80178A88 site) → `fn_801783D4` builds the per-stage tasks:

| Layer | Object created | Size | Evidence |
|---|---|---:|---|
| 6 Manager | `SetManagerTask` (0x800CB820) | 0x28 | call 0x80178424 |
| 6 Manager | `TStageInitalizeTask` | 0x48 | call 0x80178440 |
| 8 Command | `CharCommandManagerTask` | 0x28 | call 0x801784EC |
| 9 CharaColli | `CharaColliManageTask` | 0x28 | call 0x80178518 |
| 16 Particle | `Effect::PJS::EffectTask` | 0x30 | call 0x80178548 |
| — | `TStageKeyNoInput` | 0x38 | call 0x80178580 |
| 10 Landscape | fn_800460C0 (landscape display) | | call 0x80178594 |
| 15 PostManager | fn_801E79AC, fn_80058B2C (bullet manager), fn_8006D7D0 | | calls 0x801785EC.. |
| (15) | `fn_8016E4EC(…, layer, [0x8057E80C])` — mission region | | call 0x80178604 (see MISSION_SYSTEM.md) |
| 3 Scene | fn_80297B30 | | call 0x8017860C |

Global stage/game block: bss **0x8057E760** (+0xA8 = 0x8057E808 stage number, +0xAC = 0x8057E80C,
+0x125 = hard-mode flag used by the SET loader; see GAMEPLAY_STRUCTURES.md).

## 5. SET (stage object) spawn / despawn loop — layer 6 (PROVEN)

`SetManagerTask::vf01` (0x800CB794, Update):
1. `SetManager_Get()` (0x800C9F6C, lazy static in sbss 0x805EF77C)
2. `SetManager_Update(mgr, dt)` (0x800CB97C): if a SET is loaded and the stage is running
   (`fn_801780AC`) and not blocked: 1P → `fn_800CAC08(setData, cam0)`, 2P (bss 8057E812) →
   `fn_800CAA2C(setData, cam0, cam1)` where camN = `fn_80010244(fn_80009548(), N)` = **camera unit N position**
   (fn_80009548 is the camera manager singleton, bss 0x8056FED4 — PROVEN; objects spawn by distance to the camera).
3. For each of 0x800 runtime slots (0x20 bytes, slot+0 → record): record `+0x18` runtime flags drive:
   - enabled & not alive & in range (`fn_800CA970`: |recPos − cameraPos|² ≤ (rec.u8[0x23]·100)², or
     flag 0x80800 = always) → `fn_800CA8D8` clears 0x80040 and calls **descriptor createHook(slot)**
     (`desc = setData->descTable[slot->descIndex]`, hook at desc+0x0C).
   - flag 0x40 / 0x800000 re-arm logic, link-group lists rebuilt by `fn_800CAE24(setData, linkId)`.
4. Created objects parent themselves to their own layer (GunSoldier → layer 14 "Enemy").

## 6. Enemy per-frame path — layer 14 (PROVEN)

```text
Task_UpdateChildren(layer Enemy)
 └─ TEnemySetTask::Update(dt)                  0x801A6EB8   (Task slot 1; subobject @enemy+0x3E0)
     ├─ dt' = fn_8017C764(enemy, dt)           (time-scale, e.g. Chaos Control)          LIKELY
     ├─ if SetBase.vf00() → flags|=1 (kill)    TEnemySetBase::vf00 0x8017B8EC
     ├─ SET slot (+0x2C): fn_800C9E4C → kill; fn_800C9D54 → fade out alpha(-2/s) → kill at 0
     ├─ else Status.vf0E() ? fade out : fade in (alpha @enemy+0xEC, clamp 0..1) → fn_8019AE90
     └─ SetBase.vf07(dt')                      TEnemySetBase::LifecycleUpdate 0x801A7280
          state 1: resources loaded (ResourceManager fn_80012408 == 3) && stage running → state 2
          state 2: async model ready → Set.init(slot), Disp.init, Status.init, AI.init, Move.init → 3
          state 3: Status ready → enemy+0xF8 = frame counter; Status.start → 4
          state 4 (every frame): Status.vf02 → Status.vf03(dt) → AI.vf01(dt) → Status.vf04(dt)
                                 → Disp.vf01(dt) → Move.vf01(dt) → Disp.vf02(dt) → Status.vf05(dt)
```
Component resolution for GunSoldier is in `enemy/GUN_SOLDIER.md`.


## 6b. Player per-frame path — layers 7 → 8 → 13 (PROVEN; details in PLAYER_GAMEPLAY_RECOVERY.md §2)

```text
Layer 7  Controller: Player::Controller::vf01 0x8009EFF8 → fn_8009F9A4: sample pad (fn_800A8998) → terrain-contact
         checks (DamageCommand/DeadCommand/SetLoopPath/Grind) → Player::Control::UserInput::vf03 0x800A1D70
         → PlayerCommands queued (Player::PlayerBase::vf02 0x80079B04)
Layer 8  Command: CharCommandManagerTask::vf01 0x801DA0A0 → fn_801DA5A4 (7 priority buckets) → receivers
         → ShadowExecuter OnCommand 0x800AF924 (behavior transitions)
Layer 13 Player ("Shadow" root, children in creation order):
         CharaColliMove → ShadowExecuter::vf01 0x800AFD28 → Executer_Update fn_800A8160:
           idle-talk timer → invincibility timer (+0x27C) → gravity fn_800A7AE8 → current Behavior Update
           (accumulate accel → integrate fn_80093BBC → terrain collision fn_80097658) → rotation smoothing →
           history → target/lock-on → clear per-frame accel/input
         → ShadowMotion → Attack → ShadowWeapon → Effect → SoundBase
```
Time bases (game block 0x8057E760): player dt = +0x18 (0x8057E778, set by the PlayerShadow ctor @0x800AE3D0);
objects default to +0x20 (0x8057E780, CharInfo ctor); the Chaos Control "Slow" task writes +0x24 (0x8057E784).
Because layer 14 (Enemy) runs before layer 13 (Player), enemies act on the player's previous-frame state.

## 7. Open items

- Exact semantics of the GameSystem mode slots (+0x0C/+0x10/+0x14) — UNKNOWN (pure virtual in base).
- Collision resolution order inside `CharaColliManageTask` (layer 9 runs *before* Gadget/Enemy/Player,
  so hits are processed from the previous frame's positions) — LIKELY, needs CharaColli trace.
- Which layer the mission manager lives in — see MISSION_SYSTEM.md.

# PLAYER_GAMEPLAY_RECOVERY — Shadow the Hedgehog (GC) player mechanics

All addresses are in `sys/main.dol`. Raw investigation log: `notes/player_trace.md`; symbol rows: `notes/player_symbols.csv`.

**Naming discipline.** Class names that come from RTTI (`Player::Behavior::Jump`, `CharInfo`, `Player::Control::UserInput`,
`StageAction::DeadAction`, …) are ORIGINAL. Every method/field/function name written without an RTTI class (e.g.
`Executer_ChangeBehavior`, "velocity", "ChangeBehavior", "Update") is a recovered **semantic** name. `ClassName::vfNN` = RTTI
vtable slot NN. Confidence: **PROVEN** (direct code/data), **STRONG** (several consistent pieces), **LIKELY** (single indirect
piece), **UNKNOWN**.

Helper scripts (read-only on the shared caches): `tools/agent_player_q.py` (windowed disasm, float/table dumps),
`agent_player_behtab.py` / `agent_player_behvt.py` (behavior factory / vtables), `agent_player_cmds.py` (command ids),
`agent_player_flags.py` (state-flag cross reference), `agent_player_icall.py` (indirect-call search).

Vector helpers referenced below (all PROVEN by disassembly): `8040E048` a+=b · `8040DE20` out=a+b · `8040DDF8` out=v·s ·
`8040DE88` v·=s · `8040DFE0` dot · `8040DFA0` length · `8040DEC4` normalize · `8040DF38` normalized copy · `8040DEB0` zero ·
`804109E0(par,perp,v,axis)` split v along/perpendicular to axis · `800091A4` out=v·s.

---------------------------------------------------------------------------------------------------------------------

## 1. Creation chain (PROVEN)

```text
TStageInitalizeTask::vf01 0x80177510 (layer 6 "Manager"; task+0x28 bit0 = spawn request)
 └─ fn_801773CC(task, 1) @0x801775CC                               StageInit_SpawnPlayers (semantic)
     ├─ 1P stage (row fn_80177020(stage)+8, bit 2 clear): fn_800AD680(0xE /*Shadow*/, 0, 0)          @0x80177450
     └─ 2P stage: char ids from bss 0x8057E89C / 0x8057E8A0 clamped to 0xE..0x13:
                  fn_800AD680(c0, 0, 0) @0x80177498 ; fn_800AD680(c1, 1, 1) @0x801774A8
fn_800AD680(charId, playerIndex)                                   Player_CreatePlayerShadow (semantic)
 ├─ fn_800AE3F0: operator_new(0x29C) → Player::PlayerShadow::PlayerShadow 0x800AE378(this, charId)
 │     Player::Player::Player 0x80077438 → Player::PlayerBase::PlayerBase 0x8007A038(this, 1, charId, name)
 │       → CharInfo::CharInfo 0x801DCDA8(this, 1)            (vptr lives at +0x38; final vtbl Player::PlayerShadow 0x80522D70)
 │     then player vslot 0xA = Player::PlayerShadow::vf0A 0x800AD7E0 (component setup, §1.1)       @0x800AE464
 ├─ player vslot 1 = Player::PlayerBase::vf01 0x80079A1C(index): +0x54 = playerIndex               @0x800AD6CC
 ├─ fn_800A8B04(input(+0x218), index): pad index = [fn_8005185C()+0x11C+4·index]; record = fn_8040EBBC(padMgr, idx)
 └─ index 0 → sbss 0x805EF630 = player+0x298 ; index 1 → sdata 0x805E5A28 = player+0x298        @0x800AD70C/@0x800AD71C
```
Character-id → name table 0x80547870 (reader fn_80228958): 0 "---", 1 Sonic, 2 Tails, 3 Knuckles, 4 Amy, 5 Rouge, 6 Omega,
7 Vector, 8 Espio, 9 Maria, 10 Charmy, 11 EggMonitor, 12 DoomsEye, 13 BossSonic, **14 (0xE) Shadow**, 15 2PYellow,
16 2PGunPink, 17 2PGunBlue, 18 2PCannonOrange, 19 2PCannonGreen.

### 1.1 Components built by `Player::PlayerShadow::vf0A` 0x800AD7E0 (PROVEN; back-pointer stores are in each ctor)
| pc | object (RTTI class where available) | parent | player+ | Update (Task slot 1) |
|---|---|---|---|---|
| 800AD84C | "Player::Watch" task (fn_800B89AC, vtbl 0x805234E0) | layer **7 Controller** | 0x268 | no-op |
| 800AD878 | `Task` renamed "Shadow" (root of per-frame player tasks) | layer **13 Player** | 0x26C | no-op |
| 800AD898 | "Wait" task (fn_800B88C4, vtbl 0x805234C4) — kills itself if player gone or flags 0x46&0x47 | Watch | – | 0x800B8798 |
| 800AD8A8 | `Player::Shadow::PlayerTarget` (fn_800B6A88) | – | 0x24C | vslot 1 called by Executer |
| 800AD8C8 | `Player::Controller` (impl 0x20, fn_8009FD14) | Watch | 0x224 | 0x8009EFF8 |
| 800AD8F4 | `Player::CharaColliMove` (impl 0xA4) | "Shadow" | 0x228 | 0x80094CFC |
| 800AD900 | `Player::Shadow::ShadowExecuter` (0x58) | "Shadow" | 0x22C | 0x800AFD28 |
| 800ADEDC | `Player::Shadow::ShadowMotion` (0x868) | "Shadow" | 0x230 | 0x800B5708 |
| 800ADEF8 | `Player::Attack` (0x30) | "Shadow" | 0x234 | 0x80077B44 |
| 800ADF04 | `Player::Shadow::ShadowWeapon` (0x60) | "Shadow" | 0x238 | 0x8023B6D0 |
| 800ADF20 | `Player::Effect` (0x30) | "Shadow" | 0x23C | 0x800A3194 |
| 800ADF2C | `Player::SoundBase` (0x40, fn_802DBD6C) | "Shadow" | 0x240 | – |
| 800ADF44 | fn_80095EC4 object (4 B) | – | 0x248 | helpers fn_80095D78/DC0/DE4 per frame |
| 800ADF58 | `Player::Shadow::ShadowControl` (control factory) → controller via fn_8009EF94 | | | |
| 800ADF7C | `Player::Shadow::ShadowBehavior` (behavior factory) → executer+0x38 via fn_800A8460 | | | |
| 800ADFB4 | initial control: `Control::InitializeArgs` id 0 (UserInput) via fn_8009EF70 | | | |
| 800AE000.. | char ids 0x10..0x13 (2P gun/cannon): weapon with 999 (0x3E7) ammo + state flag 0x33 | | | |
`Player::PlayerBase::vf0A` 0x80079BE4 (called @0x800AD8A0) also creates +0x250 (fn_800A87B8) and +0x254 (fn_800AB34C).

## 2. Per-frame order

Scene-layer order (from `GAMEPLAY_UPDATE_PIPELINE.md` §3, PROVEN): 6 Manager → **7 Controller** → **8 Command** →
9 CharaColli → 10 Landscape → 11 Gadget → 12 Vehicle → 14 Enemy → **13 Player** → 15 … 18 Camera. Children update in creation
order, parents before children.

```text
Layer 7  Player::Watch
           ├ Wait 0x800B8798                                   lifetime watchdog
           └ Player::Controller::vf01 0x8009EFF8 → fn_8009F9A4(impl, dt)                    PlayerController_Update
                fn_800A8998(input, dt)                          sample pad → logical buttons (§7.2)          PROVEN
                fn_8009F330 / fn_8009F29C / fn_8009F17C / fn_8009F668                         PROVEN (calls)
                   +0xA4 contact bit1 → DamageCommand(power 10); bits 2/3 → DeadCommand; loop-path/grind
                   detection → SetLoopPathCommand / GrindCommand
                control(impl+8) vslot 3 (vptr+0x14)(dt) = Player::Control::UserInput::vf03 0x800A1D70 (§7.3)
                   → CharCommands sent via player vslot 2 = Player::PlayerBase::vf02 0x80079B04 (enqueue)
Layer 8  CharCommandManagerTask::vf01 0x801DA0A0 → fn_801DA5A4                           dispatch, 7 priority buckets
           → receiver list (fn_801A02B4) → ShadowExecuter OnCommand 0x800AF924 (PMF 0x80522F68 bound in ctor 0x800B00B0)
Layer 13 "Shadow" root (no-op) → children in creation order:
           CharaColliMove::vf01 0x80094CFC → fn_80094E88   displacement = pos − prevPos; velocity estimate = disp/dt
           Player::Shadow::ShadowExecuter::vf01 0x800AFD28:
             fn_800AF868  holding-object checks (flag 0x1E → player vslots 0x11 / 0x0F)
             fn_800AF7B8  karma-gauge-full effects
             fn_800AF6A4  Super ring drain (§6.4)
             fn_800AF60C  time-slow (flag 0x15) end check
             fn_800A8160  Executer_Update (base code; Player::Executer's own slot 1 is Task::vf01 blr)       PROVEN order:
               1  fn_800A76CC(dt)   idle-talk timer executer+0x48 → WaitTalkingCommand (0x132)
               2  fn_80079098(p,dt) invincibility timer +0x27C −= dt (flag 0x18 while > 0)
               3  fn_800A7AE8       up-vector frame + gravity → acceleration (+0x24) (§5.2)
               4  clamp |move input (+0xB8)| ≤ 1
               5  current Behavior (executer+0x30) vslot 5 (vptr+0x1C)(dt)  = Behavior Update
                    (each Update: accumulate accel → fn_80079854 integrate → fn_80097658 terrain collision)
               6  fn_800A78F4(dt)   rotation (+0xC) smoothing, factor min(30·dt, 1) (skipped when flag 0x38)
               7  fn_80078E68       history ring (+0x250) advance
               8  flag 0x43 ? fn_80095DC0 : fn_80095DE4 on +0x248; fn_80095D78
               9  PlayerTarget(+0x24C) vslot 1; fn_800AB1FC(+0x254)
              10  zero accel (+0x24) and move input (+0xB8); clear flag 0x1B; prevPos (+0xD0) = pos; fn_80078FB8
           ShadowMotion 0x800B5708 → Attack 0x80077B44 → ShadowWeapon 0x8023B6D0 → Effect 0x800A3194 → SoundBase
```
Net chain (STRONG, every edge PROVEN): **input → command queue/dispatch → behavior OnCommand/Execute → gravity →
behavior Update (acceleration, integration, terrain collision) → rotation → animation → attack/weapon/effects/sound.**

Time base: `dt = *(player+0x60)`. `CharInfo::CharInfo` points +0x60 at bss 0x8057E780 (game block +0x20); the
`PlayerShadow` ctor overrides it with **0x8057E778** (+0x18) @0x800AE3D0. The "Slow" task (§6.5) writes 0x8057E784 (+0x24)
— so the player and world objects use different time bases (STRONG).

## 3. Player object layout (`Player::PlayerShadow`, sizeof 0x29C — PROVEN by `operator_new(0x29C)` @0x800AE408)

Bases from RTTI (PROVEN): `CharInfo` ⊂ `Player::PlayerObject` ⊂ `Player::PlayerBase` ⊂ `Player::Player` ⊂ `Player::PlayerShadow`,
all at offset 0; single vptr at +0x38.

| Offset | Size | Field (semantic) | Evidence | Confidence |
|---|---:|---|---|---|
| +0x000 | 12 | position (Vec3) | integrator writes it (fn_800939E4 passes player as pos); SetPosition handler 0x800A7DF4 | PROVEN |
| +0x00C | 12 | rotation, Euler (+0x10 = heading) | fn_800A78F4 writes +0xC/+0x10/+0x14 after angle normalize fn_8040F9EC; fn_8007975C uses +0x10 | STRONG |
| +0x018 | 12 | **velocity** | integrator fn_80093A18 r4; SetVelocity handler 0x800A7E28; Jump/SpinDash writes | PROVEN |
| +0x024 | 12 | **acceleration accumulator** | gravity add 0x800A7CC4; behaviors add; integrator r5; zeroed 0x800A8284 | PROVEN |
| +0x030 | 4 | int (CharInfo 0, PlayerBase 2) | ctors 0x801DCDE8 / 0x8007A11C | UNKNOWN |
| +0x038 | 4 | vptr | ctors | PROVEN |
| +0x050 | 4 | CharInfo ctor arg (=1 for players) | 0x801DCE00 | UNKNOWN |
| +0x054 | 4 | player index (−1 = none) | CharInfo ctor; PlayerBase::vf01 0x80079A28 | PROVEN |
| +0x05C | 4 | controller kind (1 = Player::Player; UserInput special-cases 2) | PlayerBase ctor 0x8007A0FC (arg 1 from Player ctor); 0x800A1D8C | LIKELY |
| +0x060 | 4 | float* dt source | CharInfo ctor 0x801DCE44; PlayerShadow ctor 0x800AE3D0 | PROVEN |
| +0x064 | 4 | character id (Shadow 0xE) | PlayerBase ctor 0x8007A10C; table indexers | PROVEN |
| +0x0A0 | 4 | **action-enable bits** (§4.4) | BitTest users; set in Enter / cleared in Leave | PROVEN |
| +0x0A4 | 4 | terrain-contact bits (1 damage floor, 2/3 death) | fn_8009F29C / fn_8009F17C; cleared fn_80097658 | PROVEN (code) / LIKELY (meaning) |
| +0x0A8 | 12 | **state flags**, 72 bits (word n>>5, bit n&31) | set fn_80076DDC / clear fn_80076DFC / test fn_8006CEA0 | PROVEN |
| +0x0B8 | 12 | move input (world space, ≤1), cleared per frame | AccelerateInput handler 0x800A7DC8 | PROVEN |
| +0x0D0 | 12 | previous position | 0x800A82AC; CharaColliMove | PROVEN |
| +0x0DC | 12 | position copy written by SetPosition (reset/respawn pos) | 0x800A7E00 | LIKELY |
| +0x0E8 | 12 | default up (0,1,0) | PlayerBase ctor 0x8007A178 | PROVEN |
| +0x0F4 | 12 | current up (gravity reference) | fn_800A7AE8 0x800A7B4C | PROVEN |
| +0x100 | 12 | gravity vector this frame | fn_800A7AE8 0x800A7C7C/0x800A7CA4 | PROVEN |
| +0x10C / +0x11C | 16 / 48 | up-frame quaternion / matrix | fn_800A7AE8 0x800A7B68.. | STRONG |
| +0x1C8 | 12 | ground normal | fn_80097658; gravity/slope code | STRONG |
| +0x1D4 / +0x1E4 | 16 / 48 | ground-frame quaternion / matrix | PlayerBase ctor; fn_8007975C / fn_800A1790 | STRONG |
| +0x214 | 4 | bitset; bit1 = no ring loss / no Super drain | fn_800B8130 0x800B819C; fn_800AF6A4 0x800AF728 | PROVEN (code) / UNKNOWN (meaning) |
| +0x218 | 4 | **input object** (0x88, §7.2) | PlayerBase ctor 0x8007A0D8; fn_8006F9F8 | PROVEN |
| +0x21C | 8 | CharCommandUnit (command receiver) | PlayerBase ctor 0x8007A1E8; vf02 | PROVEN |
| +0x224..+0x240 | 4 each | Controller, CharaColliMove, **Executer**, Motion, Attack, Weapon, Effect, Sound | component ctors (`q.py off 22C W` …) | PROVEN |
| +0x248 | 4 | fn_80095EC4 object | vf0A 0x800ADF4C | UNKNOWN |
| +0x24C | 4 | `Player::Shadow::PlayerTarget` | vf0A 0x800AD8AC | PROVEN |
| +0x250 | 4 | 120-entry × 0x70 history ring | fn_800A86D4 (wraps at 0x78) | LIKELY |
| +0x254 | 4 | 2 CharInfo proxies copied each frame | fn_800AB1FC / fn_800AB12C | UNKNOWN (purpose) |
| +0x260 | 8 | self reference handle | fn_8006EEA8; many `addi r4,rX,0x260` | LIKELY |
| +0x268 / +0x26C | 4 / 4 | Watch task (layer 7) / "Shadow" root task (layer 13) | vf0A 0x800AD850 / 0x800AD880 | PROVEN |
| +0x274 | 4 | float from sbss 0x805EF660 | PlayerShadow ctor 0x800AE3C8 | UNKNOWN |
| +0x278 | 4 | ground sensor (0x28, fn_8009397C) | Fall::Update 0x80080B8C | LIKELY |
| +0x27C | 4 | **invincibility time (s)** | fn_8007915C, fn_80079098 | PROVEN |
| +0x280 / +0x288 | 8 / 8 | int[2] / float[2] (init 1.0) copied to motion | fn_80078FB8 | LIKELY (motion weights) |
| +0x290 | 8 | held-object reference | Player ctor fn_8007374C; Player::vf10 0x80076ED0 | STRONG |
| +0x298 | 4 | player handle (−1 until set) → sbss 0x805EF630 / sdata 0x805E5A28 | Player ctor 0x8007747C; 0x800AD70C | PROVEN |

Related objects:
| Object | Offset | Field | Evidence | Confidence |
|---|---|---|---|---|
| `Player::Executer` (ctor 0x800A85C0) | +0x28 | player reference | ctor; every `lwz r3,0x28(r3)` | PROVEN |
| | **+0x30** | **current Behavior** | fn_800A82FC 0x800A83C0; fn_800A8160 0x800A81C8 | PROVEN |
| | +0x38 | behavior Factory | fn_800A82FC 0x800A832C | PROVEN |
| | +0x48 / +0x4C | idle-talk timer (10.0 / 20.0) / talk counter | ctor 0x800A8618; fn_800A76CC | STRONG |
| `ShadowExecuter` | +0x50 | Super ring-drain accumulator | fn_800AF6A4 0x800AF700 | PROVEN |
| `Player::Behavior::Behavior` (ctor 0x80076D70) | +0x4 / +0xC / **+0x10** | player ref / name string / **behavior id** | ctor | PROVEN |
| `Player::Behavior::InitializeArgs` (vtbl 0x80520F18) | **+0x4** | behavior id; subclass payload from +0x8 | InitializeArgsDamage ctor 0x800724F4 | PROVEN |
| `CharCommand` (ctor 0x8006E57C) | +0x4 u16 / +0x7 u8 / +0x8 | command id / state (bit1 handled) / priority bucket 0..6 | ctor; fn_801DA2DC; handlers | PROVEN |
| Per-player record = `fn_8007993C()` (bss 0x8057707C) + 0x728 + 0x4C·index (fn_80178F10) | **+0x28** | **ring count** | death check 0x80079910; ring loss fn_800B8130; Super drain | STRONG |
| | +0x20 | receives overflow when rings pass 1000 (fn_8016F66C wraps mod 1000) | fn_80178ED4 | PROVEN (code) |

## 4. Behavior state machine

### 4.1 Slots and transitions (PROVEN)
`Player::Behavior::Behavior` vtable (7 slots in every subclass): 0 dtor · **1 CanEnter(prevId, args) → bool** ·
**2 Enter(prevId, args)** · **3 Leave(nextId)** · **4 OnCommand(cmd)** · **5 Update(dt)** · 6 UNKNOWN (default `blr`
0x80076DD8; no caller found).

`Executer_ChangeBehavior` = fn_800A82FC(executer, InitializeArgs*):
```cpp
int prev = exec->cur ? exec->cur->id : 0x25;
Behavior* nb = exec->factory->Create(args->id);          // factory vslot 1 @0x800A8344
exec->talkTimer = 10.0f;                                 // 0x800A8364
if (nb && nb->CanEnter(prev, args)) {                    // vslot 1 @0x800A8388
    if (exec->cur) exec->cur->Leave(nb->id);             // vslot 3 @0x800A83B4
    exec->cur = nb;                                      // fn_800A8420 @0x800A83C0
    nb->Enter(prev, args);                               // vslot 2 @0x800A83E4
    return 1;
}
return 0;
```
Behaviors call it as `fn_800A82FC(player+0x22C, &args)`. Factory `Player::Shadow::ShadowBehavior::vf01` 0x800AE8D0:
table 0x80522DE8[id] → .rodata descriptor, word 0 = creator; with state flag 0x25 (Super) ids 0/2/3 → 0x22.

### 4.2 Commands and who changes behavior
Command ids (u16 at +4; all from ctors, `agent_player_cmds.py`, PROVEN): 1 AccelerateInput, 2 SetPosition, 4 ClrVelocity,
5 SetVelocity, 7 Damage (+0x14 f32 power, +0x18 Vec3 source), 8 Dead, 0xD Use, 0x15 ControlVehicle, 0x102 StartAttack,
0x103 EndAttack, 0x104 StartJump (+0x14 Vec3 dir), 0x105 EndJump, 0x106 StartAction, 0x107 EndAction, 0x108 JumpDash,
0x109 Sliding, 0x10A HomingAttack, 0x10B HomingJump, 0x10C SetLoopPath, 0x10D Grind, 0x10E SpinDash, 0x10F DarkSpin,
0x110 LightDash, 0x113 ChaosBlast, 0x114 ChaosControl, 0x115 StartUseWeapon, 0x116 CancelUseWeapon, 0x117 EndUseWeapon,
0x118 PickupWeapon, 0x119 CastWeapon, 0x11E Throw, 0x121 Launch, 0x122 Stream, 0x123 GetOnVehicle, 0x124/0x125
Start/EndParallelMove, 0x126 Invincible, 0x127 IgnoreInput, 0x12D SetGravity, 0x12F ToSuper, 0x132 WaitTalking.
Command vtable (CharCommand 0x8053E1E8): 0 dtor, **1 Execute(receiver)**, 2 debug text, 3 UNKNOWN.

Handling order in `ShadowExecuter` OnCommand 0x800AF924 (PROVEN):
1. id ∉ {1, 0x17} → fn_80078E44(player) (semantic UNKNOWN). 2. 0x106 with flag 0x15 → Effect fn_800A2DCC, consumed.
3. current behavior **vslot 4 OnCommand**. 4. `Player_HandleCommand` 0x800A7CE4 (1: move input += v, flag 0x1B;
2: pos = prevPos = +0xDC = v; 5: velocity += v and leave ground if moving away from the ground normal; …).
5. Shadow switch: **7** → ignored when flag 0x17 or 0x18 or current id == 0x1E, else player vslot 0x12 (0x800798A4) ?
Dead(0x1F) : Damage(0x1E, `InitializeArgsDamage(cmd+0x18)`); **8** → Dead; **0x108** → JumpDash(6); 0x115/0x116/0x117 → weapon
(+0x238) vslots +0x28/+0x2C/+0x30. 6. fallback: command **vslot 1 Execute**. Example `Player::StartJumpCommand::vf01`
0x8009CC1C: if action bit A0.1 → `InitializeArgsJump(cmd+0x14)` → ChangeBehavior(5).

### 4.3 Behavior id table — factory 0x80522DE8 (PROVEN; `python agent_player_behvt.py`)
| id | class (RTTI) | size | creator | CanEnter | Enter | Leave | OnCommand | **Update** |
|---|---|---|---|---|---|---|---|---|
| 0x00 | Idle | 0x20 | 80086D3C | 800870D4 | 80086FE4 | 80086F1C | 800870DC | **80086D84** |
| 0x01 | Ground | 0x40 | 80082D44 | 80083FFC | 80083EE8 | 80083DE4 | 80084004 | **80083A38** |
| 0x02 | Fall | 0x60 | 80080724 | 80081090 | 80080ED4 | 80080E58 | 80080E54 | **8008096C** |
| 0x03 | Landing | 0x24 | 80088E58 | 80089154 | 800890AC | 80089024 | 80072548 | **80088EA0** |
| 0x04 | Ottotto (ledge teeter) | 0x20 | 8008BE48 | 8008BFE0 | 8008BF70 | 8008BF44 | 80072548 | **8008BE90** |
| 0x05 | Jump | 0x44 | 80087220 | 800880D0 | 80087904 | 800877E8 | 800880D8 | **8008731C** |
| 0x06 | JumpDash | 0x28 | 800881F4 | 80088C74 | 80088948 | 800885D8 | 80072548 | **800883E8** |
| 0x07 | Sliding | 0x28 | 8008DA28 | 8008E24C | 8008E188 | 8008E108 | 8008DFFC | **8008DEC0** |
| 0x08 | HomingAttack | 0x30 | 80084F58 | 80086654 | 80085FE0 | 80085950 | 80072548 | **80085628** |
| 0x09 | HomingJump | 0x30 | 800868E0 | 80086C8C | 80086AC0 | 80086A4C | 80086A48 | **80086928** |
| 0x0A | Throw | 0x24 | 8008FD40 | 8009045C | 800901B8 | 8008FE84 | 8008FE80 | **8008FDD8** |
| 0x0B | TurnOver | 0x2C | 8009067C | 80091278 | 80090F64 | 80090C18 | 80090B8C | **800909D4** |
| 0x0C | EdgeHang | 0x50 | 8007F154 | 8007FF94 | 8007FBB8 | 8007F854 | 8008000C | **8007F810** |
| 0x0D | WallHang | 0x50 | 800914D0 | 800925F8 | 800924C4 | 80092418 | 80092168 | **80091C98** |
| 0x0E | LineHang | 0x5C | 8008A7FC | 8008BAB4 | 8008B834 | 8008B6A8 | 8008B330 | **8008B274** |
| 0x0F | SomethingHang | 0x34 | 8008E310 | 8008EC88 | 8008EB1C | 8008E934 | 8008E778 | **8008E54C** |
| 0x10 | WallJump | 0x3C | 800927A8 | 80093210 | 80092F04 | 80092BC0 | 80093218 | **800929FC** |
| 0x11 | Grind | 0x70 | 80081178 | 8008294C | 800825D0 | 800821E0 | 8008202C | **80081A94** |
| 0x12 | SpinDash | 0x58 | 8008EDA4 | 8008F9D8 | 8008F8D0 | 8008F7EC | 8008F728 | **8008F41C** |
| 0x13 | DarkSpin | 0x68 | 8007D168 | 8007D7DC | 8007D634 | 8007D588 | 8007D584 | **8007D3EC** |
| 0x14 | LightDash | 0x60 | 8008985C | 8008A650 | 8008A4B0 | 8008A400 | 8008A3FC | **80089D64** |
| 0x15 | PickupObject | 0x2C | 8008C084 | 8008CAFC | 8008C7B0 | 8008C460 | 8008C3E8 | **8008C11C** |
| 0x16 | PickupWeapon | 0x2C | 8008CE94 | 8008D7F8 | 8008D450 | 8008D144 | 80072548 | **8008CEF8** |
| 0x17 | Launch | 0x2C | 800891FC | 800897AC | 8008960C | 80089548 | 80089544 | **80089244** |
| 0x18 | Stream | 0x1C | 8008FABC | 8008FC98 | 8008FC2C | 8008FC00 | 8008FB80 | **8008FB04** |
| 0x19 | GroundAttack | 0x24 | 80084200 | 80084D60 | 80084A7C | 80084704 | 80084670 | **800842B4** |
| 0x1A | AirAttack | 0x28 | 8007A6A0 | 8007ACE4 | 8007AAF0 | 8007AA8C | 8007AA54 | **8007A774** |
| 0x1B | DriveVehicle | 0x48 | 8007DD5C | 8007EEF0 | 8007EB0C | 8007EA1C | 8007E8B8 | **8007E6D0** |
| 0x1C | ChaosBlast | 0x2C | 8007AD94 | 8007B88C | 8007B438 | 8007B08C | 8007B068 | **8007AE10** |
| 0x1D | ChaosControl | 0x3C | 8007BA94 | 8007C76C | 8007C468 | 8007C0AC | 8007C044 | **8007BB8C** |
| 0x1E | Damage | 0x28 | 8007CAE0 | 8007D0B0 | 8007CE74 | 8007CE10 | 8007D0B8 | **8007CCEC** |
| 0x1F | Dead | 0x28 | 8007D8E4 | 8007DC30 | 8007DAA4 | 8007DA44 | 8007D9F0 | **8007D92C** |
| 0x20 | ExternalControl | 0x34 | 80080218 | 80080608 | 800804A8 | 80080350 | 80080610 | **80080264** |
| 0x21 | ElectricCircuit | 0x44 | 80076400 | 80076BFC | 80076A6C | 80076874 | 80076794 | **800765B0** |
| 0x22 | Super | 0x2C | 802EB590 | 802EBAA8 | 802EBA30 | 802EB9CC | 802EB9B0 | **802EB96C** |
0x80072548 is a shared default OnCommand. RTTI classes Fly / Hide / FirstMeet exist but are not in Shadow's table (NPC use).

Observed transitions (PROVEN call sites): Ground→Idle (speed < 0.2) @0x80083DB0; Ground→Fall (not grounded) @0x80083C9C or
→Ottotto (ledge fn_800971EC) @0x80083C68; Ground→Fall on steep ground (n·up < 0.4226 = cos 65°, not fast; velocity =
normal·100) @0x80083D6C; Fall→Landing (grounded / ground sensor) @0x80080C48 / 0x80080BF4; Fall→EdgeHang (fn_8009827C)
@0x80080AD4; Jump→Landing (grounded after 10-frame grace) @0x800876C4; Jump→Fall after apex @0x80087508/0x80087544;
JumpDash→Fall after 0.4 s @0x80088564; HomingAttack→Fall (target lost / 10 s) @0x80085784 / 0x800857DC,
→HomingJump after hit @0x800856D0 / 0x80085704; Damage→Fall after 1.5 s airborne @0x8007CDAC; SpinDash→Idle after 10 s
charge @0x8008F2C4.

### 4.4 Flags
Action-enable bits +0xA0 (set in Enter, cleared in Leave; tested by the command builder) — PROVEN:
0 fast (speed > 250) · 1 can jump · 2 can air-action (homing / jump dash) · 3 can LightDash · 4 can slide (speed > 200) ·
5 can SpinDash · 6 can attack · 7 can use weapon · 8 vehicle control · 9 idle (enables idle-talk timer).

State flags +0xA8 (`python agent_player_flags.py A8 <bit>`; set/clear sites PROVEN, meaning STRONG unless noted):
0 gravity on · 1 idle tiny gravity · **2 grounded** (set by terrain collision fn_80097658 @0x80097CB8) · 4 jumping ·
5 JumpDash · 6 HomingAttack · 7 Sliding · 8 Grind · 9 SpinDash roll · 0xA ElectricCircuit · 0xB hanging · 0xC LineHang ·
0xD/0xE WallHang · 0xF Stream · **0x10 DriveVehicle** · 0x11 in Damage · 0x12 in Dead · 0x13 death-contact type 3 ·
0x14 ChaosControl active · 0x15 time-slow task active · 0x16 ChaosBlast active · 0x17 extra invulnerability (fn_800794D0,
LIKELY) · **0x18 invincible** · 0x19 weapon firing · 0x1B move input this frame · 0x1D AirAttack · 0x1E holding object ·
0x1F has weapon · 0x20 barrier/shield (LIKELY) · 0x22/0x23 Chaos Control / Chaos Blast available (setter UNKNOWN) ·
0x24 Super ring drain · **0x25 Super Shadow** · 0x33 infinite ammo (2P) · 0x38 skip rotation smoothing · 0x3B ground
speed > 150 · 0x3C parallel (2-D path) movement · 0x3E ground-type behavior (up vector follows ground).

## 5. Physics

### 5.1 Integrator — fn_80079854 → fn_800939E4 → fn_80093A18 → fn_80093BBC (PROVEN)
`fn_80093BBC(pos, vel, acc, k, dt)` integrates v' = a − k·v exactly:
- k ≤ 0 (test vs 0.0 @0x80093BFC): p += v·dt + ½·a·dt² (0.5 = 0x805F3A48), v += a·dt.
- k > 0: E = e^{−k·dt} (e = 2.71828 double 0x805F3A68 @0x80093CA8, pow wrapper fn_803B2D0C); c = a/k;
  **v ← c + (v − c)·E ; p ← p + c·dt + (v₀ − c)(1 − E)/k.**  Terminal speed = |a|/k.

Drag coefficient k chosen by fn_80093A18 (writable `.sdata`, i.e. tunables; PROVEN):
| condition | k | address | load pc |
|---|---|---|---|
| state flag 7 (Sliding) or 9 (SpinDash roll) | 1.0 | 0x805E5650 | 0x80093A74 |
| flag 2 grounded | **20.0** | 0x805E564C | 0x80093AA4 |
| flag 0x1D (AirAttack) | 10.0 | 0x805E5648 | 0x80093AD4 |
| airborne, horizontal part (⊥ up +0xF4) | **3.0** | 0x805E5640 | 0x80093B18 |
| airborne, vertical part (∥ up) | **1.0** | 0x805E5644 | 0x80093B30 |
The integrator is called from the Update of 31 behaviors (`q.py callers 0x80079854`).

### 5.2 Gravity — fn_800A7AE8 (PROVEN), only when state flag 0 is set
| condition | g | address | pc |
|---|---|---|---|
| flag 1 (idle) | −0.0625 | .sdata2 0x805F3C78 | 0x800A7C00 |
| flag 2 (grounded) | −800 | 0x805F3C7C | 0x800A7C20 |
| airborne | **−320** | 0x805F3C80 | 0x800A7C28 |
Gravity vector = g × (ground normal +0x1C8 if A0.0 fast and not flag 0xD, else up +0xF4) → +0x100; acc += it @0x800A7CC4.
Air terminal fall speed = 320 / 1.0 = **320** (derived).

### 5.3 Ground running (Ground::Update 0x80083A38, run sub-state fn_8008331C) — PROVEN constants, STRONG roles
| constant | value | address | consumer | meaning |
|---|---|---|---|---|
| min accel (+0x24) | 1000 | 0x805F3710 | fn_80082E0C 0x80082E4C | ramp floor |
| max accel base | 15000 | 0x805F3718 | 0x80082E58 → ×char scale 0x80082E74 | +0x28 = 15000 × table[id].+4 |
| carrying object | 1000 / 3000 | 0x805F3710 / 0x805F3714 | 0x80082E38 / 0x80082E3C | min / max while flag 0x1E |
| partial-stick decay | ×0.9 per frame when |stick|² < 0.81 | 0x805F373C / 0x805F3738 | 0x800834D8 / 0x800834C8 | accel decays to min |
| alignment window | t = (cosθ − 0.766)/0.234; t=0 if speed<30 | 0x805F3744 / 0x805F3748 / 0x805F3708 | 0x800835CC / 0x800835E4 / 0x800835C0 | target = min + t(max−min) |
| ramp rate | + cos⁸θ·(max−min)·0.25·dt | 0x805F374C | 0x8008360C / fmadds 0x80083630 | ≈4 s from min to max |
| accel clamp | |acc| ≤ +0x28 | – | 0x8008372C | |
| turn rate | 15 (≤150), 2 (>300), else 2+13·(300−v)/150 | 0x805F372C / 0x805F3760 / 0x805F3768, 0x805F376C, 0x805F3764 | 0x80083880..0x800838C4 | ×dt into fn_80093DE8 |
| fast bit A0.0 | speed² > 62500 (250) | 0x805F3790 | 0x80083B8C | gravity follows ground normal |
| flag 0x3B | speed² > 22500 (150) | 0x805F3758 | 0x80083BD4 | input replaced by facing |
| slide allowed A0.4 | speed > 200 | sbss 0x805EF4B4 (init fn_800841E0) | 0x80083CBC | |
| → Idle | speed < 0.2 | sbss 0x805EF4B0 | 0x80083D80 | |
| steep-slope limit | n·up < 0.4226 | 0x805F3794 | 0x80083D1C | → Fall, velocity = normal·100 (0x805F3778) |
| animation level | <12.5, <30, <100, <720 → 0..4 | 0x805F3770 / 0x805F3708 / 0x805F3778 / 0x805F3780 | 0x800838E4..0x80083974 | LIKELY walk/jog/run/dash blend |
Derived: grounded drag k = 20 ⇒ **top ground speed ≈ 15000/20 = 750 u/s** for Shadow (scale 1.0); minimum 1000/20 = 50.
Holding a weapon multiplies velocity and the accel ramp by (1 − w) every frame, w = weapon object vslot (+0x28) value
(0x80083AE4..0x80083B20; "weapon weight" LIKELY).

Per-character table 0x804F2904 (0x2C per id; PROVEN data, roles as noted):
| id | name | +0 | +4 speed scale | +8 bytes | +0xC..+0x14 | +0x18/+0x1C (sound ids, LIKELY) | +0x20..+0x28 (RGB, LIKELY) |
|---|---|---|---|---|---|---|---|
| 1 | Sonic | 2 | 1.0 | 01 01 01 00 | 0 | 0x219/0x21A | 0,0x5A,0xAA |
| 2 | Tails | 2 | 0.9 | 00 01 01 00 | 0 | 0x213/0x214 | |
| 4 | Amy | 2 | 0.6 | 00 01 01 00 | 0 | 0x217/0x218 | |
| 9 | Maria | 2 | 0.4 | 0 | 0 | 0x23/0x24 | |
| 10 | Charmy | 2 | 0 | 0 | 25, 10, 15 | | |
| **14** | **Shadow** (and 15..19) | 2 | **1.0** | 01 01 01 00 | 0 | 0x23/0x24 | 0x7F,0x40,0 |
Byte +8 ≠ 0 enables the `PlayerTarget` lookup before homing (0x800A06B0). Full dump: `agent_player_q.py` / notes.

### 5.4 Air, jump, dashes (PROVEN constants and consumers)
| mechanic | value | address | consumer pc |
|---|---|---|---|
| Fall air steering | acc += 220·input | 0x805F3698 | Fall::Update 0x80080998 |
| Fall turn | 15·dt | 0x805F369C | 0x80080A34 |
| Jump initial speed (component along jump dir replaced) | **100** (Maria 75) | jump table 0x804B1628 +0 (stride 0xC by char id) | Jump::Enter 0x80087F48 |
| Jump hold thrust | **270** u/s² along jump dir | table +4 | Jump::Update 0x800873D8 |
| Jump hold duration | **0.75 s**; EndJumpCommand (0x105) zeroes it | 0x805F3834 | Enter 0x80087A0C; Jump::vf04 0x80088100 |
| Jump air steering | 220 (110 when carrying) | 0x805F3824 / 0x805F3820 | 0x800875D4 / 0x800875CC |
| Jump turn | 6.5·dt | 0x805F3828 | 0x80087620 |
| Jump out of Sliding | velocity zeroed, + dir·200 | 0x805F383C | 0x80087CE8 |
| Landing grace | 10 frames (+0x40) | `li 0xA` 0x80087FE4 | 0x80087698 |
| carried velocity (fn_80078E8C; moving platform LIKELY) | if dot(v_c, vel) ≥ 0: pos += v_c·dt, vel += v_c | – | Jump::Enter 0x80087F68..0x80087FE0 |
| JumpDash impulse | vel += forward·180 | 0x805F3848 | JumpDash::Update 0x80088454 |
| JumpDash thrust / duration | 430 u/s² / 0.4 s | 0x805F3850 / 0x805F3840 | 0x800884AC / 0x80088440 |
| JumpDash air steering / turn | 220 / 6.5·dt | 0x805F3854 / 0x805F3858 | 0x800884DC / 0x80088508 |
| HomingAttack thrust | acc += forward·1000; velocity re-aimed along forward, speed kept | 0x805F37C0 | HomingAttack::Update 0x8008581C |
| HomingAttack steer rate | +0x28 = 8, +10 per s (units UNKNOWN) | 0x805F37C4 / 0x805F37BC | Enter 0x80086024 / 0x80085724 |
| HomingAttack timeout | 10 s → Fall | 0x805F37BC | 0x800857B4 |
| Homing search range | **150** | sbss 0x805F0CE0 (static init 0x80207274) | fn_80206B98 0x80206C98 |
| Homing search cone | 45° if |stick|² > 0.09, else 90° | 0x805F79D0 / 0x805F79D8 / 0x805F79CC | 0x80206CDC / 0x80206D98 / 0x80206C5C |
| SpinDash max charge | 10 s → Idle | 0x805F3994 | fn_8008F26C 0x8008F29C |
| SpinDash launch | speed = **150 + 850·min(charge, 1 s)** along forward | 0x805F3990 / 0x805F399C | 0x8008F354 |
| SpinDash +0x28 (roll time, LIKELY) | 2·min(charge,1) | 0x805F3980 | 0x8008F334 |
Forward / up used above = fn_80079810 (local (0,0,−1)) / fn_800797CC (local (0,1,0)) through the ground frame and heading
(fn_8007975C) — PROVEN.

### 5.5 Native behavior slices and exact limits (2026-09-30)

The native `src/PlayerBehavior.cpp` implements **bounded PROVEN** pieces of four original Update methods.
These functions do not claim to be complete behavior implementations: callers supply the results of terrain,
ledge, target, motion and other engine services. The existing `PlayerExecuter` dispatch and `PlayerPhysics`
integrator remain the state-machine/physics base. `src/PlayerPhysics.cpp` additionally holds the directly
observed force/turn slices: Fall input adds `input·220` and requests turn `15·dt` only with input flag 0x1B
and without parallel flag 0x3C (0x80080998..0x80080A40); Jump bit 1 adds `input·220` or `input·110`
while carrying (0x800875B4..0x80087604); HomingAttack adds `forward·1000` and re-aims existing velocity
to the engine-supplied forward vector without changing its speed (0x80085804..0x80085864).

| Native function | Direct DOL evidence | Recovered result | Boundary retained |
|---|---|---|---|
| `StepGroundExit` | `Ground::Update` 0x80083B7C..0x80083DC0 | squared speed >62500 sets action bit 0; >22500 and !flag 0x3C sets state 0x3B; if grounded, >40000 sets action bit 4; ungrounded ledge query chooses Ottotto (zero velocity) or Fall; below fast speed, normal·up <0.42261827 yields Fall with velocity normal·100; speed² <0.2² yields Idle | Comparisons are strict. Action bit 4 is untouched when ungrounded. World ledge query and player-position side effect remain engine hooks. |
| `StepFallLanding` | `Fall::Update` 0x80080B88..0x80080C48 | enabled sensor with bit 0, sensor-normal·velocity <0 and sensor distance below the engine's limit chooses Landing; otherwise grounded state flag 2 chooses Landing | Sensor result and distance limit come from external engine services. EdgeHang test at 0x80080A58..0x80080AF4 occurs earlier and is not reproduced here. |
| `StepJumpLandingGrace` | `Jump::Update` 0x80087680..0x8008770C | grounded and remaining frame count **already zero** chooses Landing; otherwise decrement and clamp count to zero | Jump Enter initializes 10 at 0x80087FE4. Landing does not happen on the same frame that count reaches zero. The rest of Jump Update remains outside this slice. |
| `StepHomingAttack` | `HomingAttack::Update` 0x80085648..0x80085800 | state 1→Fall, state 2/3→HomingJump variant 0/1; other states raise `+0x28` by 10·dt, lose target→Fall, and after valid target increment `+0x2C` timer; timer >10→Fall | Target usability is supplied by the original handle/bit query. Steering geometry, thrust and collision calls at 0x800857F8..0x80085878 remain outside this slice. |

`src/tests/test_player_behavior.cpp` checks the strict speed, slope, sensor-distance and 10-second boundaries,
plus transition priority and frame-grace ordering. `src/tests/test_player_physics.cpp` checks the separate
PROVEN jump-hold force at 0x80087384..0x800873F0.

## 6. Damage, death, invincibility, rings, Super

### 6.1 Taking a hit (PROVEN)
DamageCommand (id 7) → ignored while flag 0x17, flag **0x18 (invincible)**, or already in Damage → otherwise
`Player::Behavior::Damage::vf02` Enter 0x8007CE74:
- player vslot 0x10 (Player::Player::vf10 0x80076EBC): drop the held object (CastCommand to it).
- flags 0x11, 0x44; face the damage source (args+8).
- knockback: velocity = up·100 − forward·100 (0x805F35F8 @0x8007CFF4/0x8007D010); Super: velocity = −forward·700
  (sbss 0x805EF458, static init fn_8007D158 from 0x805F35FC).
- **ring loss** fn_800B8130(&player, 10) @0x8007D048: none when flag 0x24 or +0x214 bit1; flag 0x20 → Effect fn_800A2CD4
  instead; else n = min(10, rings), scatter n rings at the player (fn_8011B614), rings −= n (fn_80178ED4).
- fn_8034ED50(4, index) (controller rumble, LIKELY); Damage::Update 0x8007CCEC: airborne ≥ 1.5 s (0x805F35E8) → Fall; animation end →
  fn_8007CB28 (recover).
- **Invincibility**: Damage::Leave 0x8007CE10 → fn_8007915C(player, **2.0 s**) (0x805F35D8 @0x8007CE58). fn_8007915C keeps
  max(current, t) in +0x27C and sets flag 0x18; fn_80079098 counts it down each frame and clears the flag at 0.

### 6.2 Death (PROVEN)
- Hit with 0 rings: player vslot 0x12 = 0x800798A4 returns `rings == 0` (unless flags 0x24/0x20) → Dead instead of Damage.
- Terrain contact bits (+0xA4) 2/3 → DeadCommand (fn_8009F17C); bit 1 → DamageCommand(power 10) (fn_8009F29C).
- DeadCommand (id 8) → Dead (0x1F). `Dead::Enter` 0x8007DAA4: drop held object, weapon vslot +0x34, flags 0 and 0x12,
  animation 0x2A, clear chaos flags (fn_800795FC), SoundBase cue 0x10/0x11 (grounded/airborne), and with flag 0x13 a
  stage-specific cue (stage 500 → 0x272, stage 603 → 0x276) via Effect fn_800A2B3C (sound/voice semantics LIKELY).
  `Dead::Update` 0x8007D92C: once, when StageState(+8) == 3 && +0xC == 0 → `StageState_RequestAction(9)` @0x8007D978;
  action 9 in `StageState_DispatchAction` builds **`StageAction::DeadAction`** (RTTI, @0x80174BF8).

### 6.3 Rings (STRONG)
Rings live in the per-player record (game-state 0x8057707C + 0x728 + 0x4C·index, +0x28). Add = fn_80178ED4 → fn_8016F66C
(clamps at 0, wraps at 1000 and forwards the overflow to record+0x20).

### 6.4 Super Shadow drain (PROVEN)
`ShadowExecuter` fn_800AF6A4 (flag 0x24): accumulator +0x50 += dt; each full 1.0 s (0x805F3D28) → rings −1 (skipped when
+0x214 bit1) @0x800AF73C; when rings ≤ 0: flag 0x25 → ChangeBehavior(Dead 0x1F) @0x800AF784, else Effect fn_800A2F2C.

### 6.5 Chaos powers (partial)
- ChaosControl::Update 0x8007BB8C drains karma gauge game-state+0x70C by max(1, int(dt·max/4.5)) per frame via
  `KarmaGauge_Add` @0x8007BD28 → a full gauge lasts **4.5 s** (arithmetic PROVEN; hero/dark identity of +0x70C UNKNOWN).
- "Slow" task (ctor fn_800A4AB4, update fn_800A4830, end fn_800A492C): world time scale bss 0x8057E784 (and 0x8057E790)
  = 0.125·r with r ramping 0→1 over 0.5 s and back; drains gauge +0x70C by 2000·dt; resets both to 1.0 at the end.
  (PROVEN writes; "world time scale" STRONG — 0x8057E784 is read by ~49 world-object functions.)

## 7. Input

### 7.1 GameCube pad → game button word (PROVEN)
`PJSPeripheral::ManagerImpGc::vf03` 0x80359798 reads PADStatus (fn_80381008) and ORs the second half of each pair in table
**0x80513D48** (12 × {u16 PAD mask, u16 game bit}) when the PAD bit is held:
| GC button | A | B | X | Y | D-Up | D-Down | D-Left | D-Right | Start | Z | L | R |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| PAD mask | 0x0100 | 0x0200 | 0x0400 | 0x0800 | 0x0008 | 0x0004 | 0x0001 | 0x0002 | 0x1000 | 0x0010 | 0x0040 | 0x0020 |
| game bit | 0x0001 | 0x0002 | 0x0008 | 0x0010 | 0x0040 | 0x0080 | 0x0100 | 0x0200 | 0x0400 | 0x0020 | 0x1000 | 0x2000 |
Pad manager singleton bss 0x8056FE0C (getter fn_80009208) is updated by `GamePeripheralTask::vf01` 0x8034A534 →
fn_8040EA3C; per-pad record (0x2C): +0 held, +4 ~held, +8 pressed, +0xC released, +0x10/+0x14 stick, +0x1C/+0x20 C-stick.

### 7.2 Player input object (player+0x218, 0x88 bytes; ctor fn_800A8BA8, per-frame fn_800A8998) — PROVEN
+0x00/+0x04 stick, +0x08/+0x0C C-stick (each clamped to length ≤ 1) · +0x10 pad record · +0x14 pad index · +0x18 held ·
+0x1C ~held · **+0x20 pressed** · **+0x24 released** · +0x28 previous · +0x2C..+0x80 22 masks copied from **0x804B3C00**
(logical bit i set when record.held & mask[i]) · +0x84 idle time (reset on any input). The mask table's only reader is the
ctor → no runtime button remapping found (STRONG).
| logical bit | 0,1 | 2 | 3 | **4** | **5** | **6** | **7** | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 18 | 19 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| GC button | Start | B | A | **A** | **B** | **X** | **Y** | R | D-Left | D-Down | D-Right | D-Up | D-Down | D-Left | D-Right | Z | L | R |
(Bits 17/20/21 use game masks 0x4/0x4000/0x8000 that no GC button produces.)

### 7.3 `Player::Control::UserInput::vf03` 0x800A1D70 — buttons → commands (PROVEN control flow)
Movement: fn_800A1790 turns the stick into (x, 0, y), rotates it into the **camera** frame (fn_800940C0 → camera manager
fn_80009548 entry +0x9C; skipped with flag 0x3A), then into the player's ground frame (+0x1E4 when fast, else +0x11C),
clamps to ≤ 1 and sends `AccelerateInputCommand` (id 1) unless flag 0x2E or |v|² < 0.0001.
Fixed: B released → `EndAttackCommand`; X pressed / released → `StartActionCommand` / `EndActionCommand`;
D-Left / D-Down / D-Right pressed (only when hard-mode byte 0x8057E885 == 0) → `MissionManager_SetCurrentMission(0/1/2)`
(0x8016E220) if `MissionManager_IsMissionActivated` (0x8016E1A8), else a SoundBase call ("error" sound, LIKELY).
Context buttons (fn_800A0130 builds press/release command pairs; sent on the matching edge):
| Button (logical bit) | Condition (first match wins) | On press | On release |
|---|---|---|---|
| A (4) | vehicle: flag 0x10 and A0.8 | ControlVehicleCommand(2) | ControlVehicleCommand(3) |
| A | A0.2 and a homing target (fn_800A1A30) | **HomingAttackCommand** (0x10A) | – |
| A | A0.2, no target | **JumpDashCommand** (0x108) | – |
| A | A0.1 | **StartJumpCommand** (0x104) | EndJumpCommand (0x105) while flag 4 |
| B (5) | vehicle | ControlVehicleCommand(6) | (7) |
| B | flag 0x1F, A0.7, weapon usable | **StartUseWeaponCommand** (0x115) | EndUseWeaponCommand (0x117) |
| B | A0.4 and +0x5C == 1 | SlidingCommand (0x109) | – |
| B | A0.6 | **StartAttackCommand** (0x102) | EndAttackCommand (fixed, above) |
| X (6) | vehicle | ControlVehicleCommand(4) | (5) |
| X | flag 0x1E (holding) | ThrowCommand (0x11E) | – |
| X | controller loop-path state 4 | DarkSpinCommand (0x10F) | – |
| X | light-dash trail found (fn_800A18B0, A0.3) | **LightDashCommand** (0x110) | – |
| X | A0.4 and +0x5C == 1 | SlidingCommand (0x109) | – |
| X | A0.5 | **SpinDashCommand** (0x10E) | – |
| Y (7) | flag 0x23 | **ChaosBlastCommand** (0x113) | – |
| Y | flag 0x22 | **ChaosControlCommand** (0x114) | – |
| Y | flag 0x1F, not 0x33, not 0xB | CastWeaponCommand (0x119, drop weapon) | – |
Also: flag 0x19 while !A0.7 → CancelUseWeaponCommand (Controller 0x8009FAF0). fn_800A0130 additionally builds
UseCommand / PickupWeaponCommand / GetOnVehicleCommand / ApproachControlCommand from nearby objects (0x800A0A5C..0x800A12E8,
not fully traced). fn_8009FE58 maps chosen commands to HUD button-prompt icons (fn_8016F2D4) — LIKELY.
Logical bits 8/16/18/19 (R, Z, L) are not read by UserInput nor by any function that reads the input object via
fn_8006F9F8 or +0x218 (scan) — their consumer is UNKNOWN.
Other controls from `Player::Shadow::ShadowControl::vf01` 0x800AF0B0: id 0 `Control::UserInput`, id 1 `Control::Approach`
(ApproachControlCommand), id 5 `Control::Bukuu`. Control vtable: 0 dtor, 1, 2, **3 Update(dt)**, 4 default 0x8006D274.

### 7.4 Correction to earlier assumption — fn_80009548 is the camera manager (STRONG)
The SET range check (0x800CB9D8) was assumed to read a "player manager". Evidence that fn_80009548 returns the **camera
manager**: it is called by `SonicteamUSA::System::Camera::CameraModeStop::vf02` (0x80009344), `PJSCamera::StageCameraManager`
ctor (0x801C9BEC), `PJSCamera::CameraModeFixEye/FixLook/Free/TestPlayer::vf0x`, `PJSCamera::PJSCameraPostTask`, and by the
player only for camera-relative input (fn_800940C0 → fn_80094264 reads entry +0x9C, a camera matrix). Its object (bss
0x8056FED4, ctor fn_8000DC68) holds a 2-entry vector (fn_8000E678(…, 2)); `fn_80010244(mgr, i)` returns entry i + 8. So the
SET spawn test uses a per-viewport camera entry vector (LIKELY the tracked player position), not the player object.
(`symbols_curated.csv` already carries `CameraManager_Get` 0x80009548 / `CameraManager_GetUnitPos` 0x80010244, consistent
with this.)

## 8. Open items
- Behavior vslot 6 and Control vslots 1/2 semantics; command vslot 3.
- Consumers of logical R/L/Z bits (weapon control 0x8023A584..0x8023E028 is the prime candidate).
- Setter of state flags 0x22/0x23 (Chaos Control / Blast availability) and hero/dark identity of gauges +0x70C/+0x718.
- Terrain collision internals (fn_80097658, fn_80097DB4, fn_80098ABC mask 0x40000) and which surfaces set +0xA4 bits.
- LightDash, Grind, hangs, DriveVehicle, ChaosBlast parameters not yet extracted (Update addresses in §4.3).
- World-unit scale (metres per unit) — no evidence.

### 8.x Resolved after this document was written (integration pass)
- **Karma gauge labels (PROVEN linkage):** `ChaosControl::vf05` drains gauge 0 (`KarmaGauge_Add(g+0x70C, 0, −n)`
  @0x8007BD28) and `ChaosBlast::vf05` calls `KarmaGauge_Add(g+0x70C, 1, −10000)` @0x8007AF1C → gauge 0 = hero
  (Chaos Control), gauge 1 = dark (Chaos Blast). See COLLECTIBLES_AND_SCORING.md.
- Native reconstruction of the integrator and tuning constants: `src/PlayerPhysics.{hpp,cpp}`
  (`test_player_physics`: top ground speed 750, fall terminal 320).

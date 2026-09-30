# Player trace — Shadow (Phase 4: player gameplay mechanics)

Source: `sys/main.dol`. Tools: `gameplay/tools/q.py` + helpers `agent_player_q.py` (windowed disasm / float dump / table dump),
`agent_player_behtab.py` (factory table), `agent_player_behvt.py` (behavior vtables), `agent_player_cmds.py` (command ids),
`agent_player_flags.py` (state-flag xref), `agent_player_icall.py` (indirect-call search).

Conventions: class names from RTTI are ORIGINAL. Every other name (method/field/"semantic") is a recovered guess.
Confidence: PROVEN (direct code/data), STRONG (several consistent pieces), LIKELY (single indirect), UNKNOWN.
Vector helpers used below (all PROVEN by disassembly): `8040E048` a+=b, `8040DE20` out=a+b, `8040DDF8` out=v*s,
`8040DE88` v*=s, `8040DFE0` dot, `8040DFA0` length, `8040DEC4` normalize in place, `8040DF38` normalized copy,
`804109E0(par,perp,v,axis)` split v into component along axis / perpendicular, `800091A4` out=v*s (wrapper).

---------------------------------------------------------------------------------------------------------------------

## 1. Creation of the player (PROVEN unless noted)

```text
TStageInitalizeTask::vf01 (0x80177510, layer 6 "Manager")          task +0x28 bit0 = "spawn players" request
 └─ fn_801773CC(task, 1)  @0x801775CC                               "SpawnPlayers" (semantic)
     ├─ 1P stage  (stage-table row(fn_80177020)+8 bit2 clear):  fn_800AD680(charId=0xE, index=0, 0)   @0x80177450
     └─ 2P stage: charIds from bss 8057E89C / 8057E8A0 (clamped to 0xE..0x13) → fn_800AD680(c0,0,0), fn_800AD680(c1,1,1)
fn_800AD680(charId, playerIndex)                                    "CreatePlayerShadow" (semantic)
 ├─ fn_800AE3F0: new(0x29C) → Player::PlayerShadow::PlayerShadow(this, charId) 0x800AE378
 │    └─ Player::Player::Player 0x80077438 → Player::PlayerBase::PlayerBase 0x8007A038 (this, 1, charId, name)
 │         └─ CharInfo::CharInfo 0x801DCDA8 (vptr at +0x38)
 │    then vcall slot 0xA (vptr+0x30) = Player::PlayerShadow::vf0A 0x800AD7E0  ("Setup": builds all tasks/components)
 ├─ vcall slot 1 = PlayerBase::vf01 0x80079A1C(index): +0x54 = playerIndex
 ├─ fn_800A8B04(input(+0x218), index): bind pad = fn_8005185C()+0x11C+4*index → pad record (fn_8040EBBC, 0x2C stride)
 └─ index 0 → sbss 0x805EF630 = player+0x298 ; index 1 → sdata 0x805E5A28 = player+0x298   (player handle globals)
```
Character id table (name ptrs) 0x80547870 (fn_80228958): 0 "---", 1 Sonic … 0xD BossSonic, **0xE Shadow**, 0xF 2PYellow,
0x10 2PGunPink, 0x11 2PGunBlue, 0x12 2PCannonOrange, 0x13 2PCannonGreen. (PROVEN, data)

### 1.1 `PlayerShadow::vf0A` (0x800AD7E0) — component/task construction, in order
| pc | object | parent layer | stored at player+ | Update fn (Task slot1) |
|---|---|---|---|---|
| 800AD84C | "Player::Watch" task (0x30, fn_800B89AC, vtbl 0x805234E0, no-op Update) | **7 Controller** | 0x268 | Task::vf01 (blr) |
| 800AD878 | plain `Task` named "Shadow" (0x28) — root of the player's per-frame tasks | **13 Player** | 0x26C | blr |
| 800AD898 | "Wait" task (fn_800B88C4, vtbl 0x805234C4) under Watch — kills itself when player gone / flags 0x46&0x47 | 7 | – | 0x800B8798 |
| 800AD8A8 | `Player::Shadow::PlayerTarget` (fn_800B6A88) | – | 0x24C | (vcall slot1 each frame from Executer) |
| 800AD8C8 | `Player::Controller` (0x30) under Watch; impl 0x20 (fn_8009FD14) | 7 | 0x224 | 0x8009EFF8 |
| 800AD8F4 | `Player::CharaColliMove` (0x30) under "Shadow"; impl 0xA4 | 13 | 0x228 | 0x80094CFC → fn_80094E88 |
| 800AD900 | `Player::Shadow::ShadowExecuter` (0x58) under "Shadow" | 13 | 0x22C | 0x800AFD28 |
| 800ADEDC | `Player::Shadow::ShadowMotion` (0x868, fn_800B14B4) | 13 | 0x230 | 0x800B5708 |
| 800ADEF8 | `Player::Attack` (0x30) | 13 | 0x234 | 0x80077B44 |
| 800ADF04 | `Player::Shadow::ShadowWeapon` (0x60, fn_8023A334) | 13 | 0x238 | 0x8023B6D0 |
| 800ADF20 | `Player::Effect` (0x30) | 13 | 0x23C | 0x800A3194 |
| 800ADF2C | `Player::SoundBase` (0x40, fn_802DBD6C) | 13 | 0x240 | (see class) |
| 800ADF44 | fn_80095EC4 object (4 bytes) | – | 0x248 | – |
| 800ADF58 | `Player::Shadow::ShadowControl` (control factory) → Controller impl +0x224 via fn_8009EF94 | | | |
| 800ADF7C | `Player::Shadow::ShadowBehavior` (behavior factory) → Executer+0x38 via fn_800A8460 | | | |
| 800ADFB4 | initial control = id 0 (UserInput) via fn_8009EF70 | | | |
| 800AE000.. | 2P characters 0x10..0x13 get a weapon with 999 (0x3E7) ammo + flag 0x33 | | | |
Back-pointers +0x224..+0x240 are written by each component's ctor (`q.py off 23C W` etc.) — PROVEN.
`PlayerBase::vf0A` 0x80079BE4 (called at 800AD8A0) creates +0x250 (fn_800A87B8, 0x20: 120-entry ×0x70 history ring,
advanced each frame by fn_800A86D4 — LIKELY state history) and +0x254 (fn_800AB34C, 0x18: two CharInfo proxies copied each
frame by fn_800AB1FC — UNKNOWN purpose).

## 2. Per-frame chain (order PROVEN from task tree + scheduler; semantics as labelled)

Scene layer order (GAMEPLAY_UPDATE_PIPELINE §3): 6 Manager → **7 Controller** → **8 Command** → 9 CharaColli → 10 Landscape
→ 11 Gadget → 12 Vehicle → 14 Enemy → **13 Player** → 15 PostManager → 16 Particle → 17 Editor → 18 Camera.

```text
[layer 7] Player::Watch (no-op) → children in creation order:
    Wait task 0x800B8798                   (lifetime watchdog)
    Player::Controller::vf01 0x8009EFF8 → fn_8009F9A4(impl, dt)            "Controller_Update"
        fn_800A8998(input, dt)             sample pad → logical buttons (§3.2)
        fn_8009F330 / fn_8009F29C / fn_8009F17C / fn_8009F668               terrain-contact → DamageCommand/DeadCommand,
                                           loop-path/grind detection → SetLoopPathCommand/GrindCommand
        current Control (impl+8) → vtable slot 3 (vptr+0x14)(dt)   = Control::UserInput::vf03 0x800A1D70 for the human
            → builds CharCommands and sends them with Player vslot 2 (PlayerBase::vf02 0x80079B04 = enqueue)
[layer 8] CharCommandManagerTask::vf01 0x801DA0A0 → fn_801DA5A4: dispatch queued commands, 7 priority buckets
        → receiver's handler list (fn_801A02B4) → ShadowExecuter::OnCommand 0x800AF924 (PMF 0x80522F68 registered in ctor)
[layer 13] "Shadow" task (no-op) → children:
    CharaColliMove 0x80094CFC → fn_80094E88   displacement = pos − prevPos, effective velocity = disp/dt (+0x8C/+0x98 of impl)
    ShadowExecuter::vf01 0x800AFD28:
        fn_800AF868   flag 0x1E (holding object) checks → player vslots 0x11/0x0F
        fn_800AF7B8   karma-gauge-full effects (gs+0x70C≥+0x710 → Effect fn_800A2FC0; gs+0x718≥+0x71C → fn_800A2FE8)
        fn_800AF6A4   flag 0x24: ring drain −1 per second (acc. +0x50); rings ≤ 0 → Dead (id 0x1F) if flag 0x25
        fn_800AF60C   flag 0x15 (time-slow active) end conditions
        fn_800A8160   "Executer_Update" (base, PROVEN order):
            1 fn_800A76CC(dt)      idle/talk timer (+0x48, 20 s) → WaitTalkingCommand (id 0x132)
            2 fn_80079098(p,dt)    invincibility timer +0x27C −= dt; flag 0x18 while > 0
            3 fn_800A7AE8          up-vector frame + GRAVITY → accel(+0x24)          (§5.2)
            4 clamp |input(+0xB8)| ≤ 1
            5 current Behavior (+0x30) → vtable slot 5 (vptr+0x1C)(dt) = Behavior::Update
                 (each Update does its own accel → fn_80079854 integrate → fn_80097658 terrain collision/ground snap)
            6 fn_800A78F4(dt)      rotation (+0xC) smoothing toward target, factor min(30·dt,1); skipped if flag 0x38
            7 fn_80078E68          history ring (+0x250) advance
            8 flag 0x43 → fn_80095DC0 else fn_80095DE4 on +0x248; fn_80095D78
            9 PlayerTarget(+0x24C) vslot1 ; fn_800AB1FC(+0x254) proxies
           10 zero accel(+0x24) and input(+0xB8); clear flag 0x1B; prevPos(+0xD0) = pos; fn_80078FB8 (motion weights)
    ShadowMotion 0x800B5708 (animation), Player::Attack 0x80077B44, ShadowWeapon 0x8023B6D0, Player::Effect 0x800A3194, SoundBase
```
So: **input (layer 7) → command dispatch (layer 8) → behavior command handlers → gravity → behavior Update
(accel + integrate + terrain collision) → rotation → motion/animation → attack/weapon/effect/sound.**  (STRONG overall;
each edge PROVEN by the calls listed.) Player dt = `*(player+0x60)`: PlayerShadow ctor sets +0x60 = bss **0x8057E778**
(gs+0x18) at 0x800AE3D0, whereas CharInfo default is 0x8057E780 (gs+0x20). The "Slow" task writes world scale to
0x8057E784 (+0x24)/0x8057E790 (§8) — so the player runs on a different time base than world objects (STRONG).

## 3. Input

### 3.1 GC pad → game button word (PROVEN) — `PJSPeripheral::ManagerImpGc::vf03` 0x80359798, table 0x80513D48
| GC PAD bit | A 0x100 | B 0x200 | X 0x400 | Y 0x800 | Up 0x8 | Down 0x4 | Left 0x1 | Right 0x2 | Start 0x1000 | Z 0x10 | L 0x40 | R 0x20 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| game bit | 0x0001 | 0x0002 | 0x0008 | 0x0010 | 0x0040 | 0x0080 | 0x0100 | 0x0200 | 0x0400 | 0x0020 | 0x1000 | 0x2000 |
Pad manager singleton (bss 0x8056FE0C, getter fn_80009208) updated by `GamePeripheralTask::vf01` 0x8034A534 → fn_8040EA3C:
record (0x2C each) +0 held, +4 ~held, +8 pressed, +0xC released, +0x10/+0x14 stick, +0x1C/+0x20 C-stick (floats after fn_8040EDB0).

### 3.2 Player input object (player+0x218, 0x88 bytes; ctor fn_800A8BA8, update fn_800A8998) — PROVEN
+0x00/+0x04 stick x/y, +0x08/+0x0C C-stick x/y (each 2-vector clamped to length ≤1), +0x10 pad record ptr, +0x14 pad index,
+0x18 held (logical), +0x1C ~held, +0x20 **pressed** (held & ~prev), +0x24 **released** (prev & ~held), +0x28 prev,
+0x2C..+0x80 22 masks (logical bit i ← game mask, copied from rodata 0x804B3C00), +0x84 idle time (reset on any input).
| logical bit | 0,1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 18 | 19 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| button | Start | B | A | **A** | **B** | **X** | **Y** | R | D-Left | D-Down | D-Right | D-Up | D-Down | D-Left | D-Right | Z | L | R |
(bits 17/20/21 map to game masks 0x4/0x4000/0x8000 which no GC button produces.) Bits 8/16/18/19 (R, Z, L) are **not read by
UserInput::vf03**; their consumers were not traced (weapon control 0x8023A584.. is the likely user — UNKNOWN).

### 3.3 `Control::UserInput::vf03` 0x800A1D70 — what each button does (PROVEN control flow; command meaning = class name)
Stick: fn_800A1790 → (sx,0,sy), rotated into the **camera** frame (fn_800940C0 uses camera manager fn_80009548 entry +0x9C;
skipped if flag 0x3A) then into the player's ground frame (+0x1E4 if fast-bit A0.0 else +0x11C), clamped ≤1 →
`AccelerateInputCommand` (id 1) unless flag 0x2E / |v|²<1e-4 → handler adds to player+0xB8 and sets flag 0x1B.
Other fixed mappings: B released → `EndAttackCommand`; X pressed/released → `StartActionCommand`/`EndActionCommand`;
D-Left/Down/Right pressed (not in hard mode, bss 8057E885==0) → MissionManager(fn_8006D7D0) current mission slot 0/1/2 via
fn_8016E220 if fn_8016E1A8 (activated), else SoundBase(+0x240) vslot(+0x14)(0) (a sound — "error beep" LIKELY).
The logical mask table 0x804B3C00 has a single reader (the ctor), so no runtime button re-mapping was found (STRONG).
Context commands come from fn_800A0130 (per button,
press/release pair) and are only sent if the button edge happened:

| Button | Condition (first match) | Press command (id) | Release command |
|---|---|---|---|
| A | vehicle (flag 0x10 & A0.8) | ControlVehicleCommand(2) | ControlVehicleCommand(3) |
| A | A0.2 (air action allowed) & homing target found (fn_800A1A30) | HomingAttackCommand 0x10A | – |
| A | A0.2, no target | JumpDashCommand 0x108 | – |
| A | A0.1 (can jump) | StartJumpCommand 0x104 | EndJumpCommand 0x105 (if flag 4 jumping) |
| B | vehicle | ControlVehicleCommand(6) | (7) |
| B | flag 0x1F (weapon) & A0.7 & weapon usable | StartUseWeaponCommand 0x115 | EndUseWeaponCommand 0x117 |
| B | A0.4 (speed>200) & +0x5C==1 | SlidingCommand 0x109 | – |
| B | A0.6 | StartAttackCommand 0x102 | (EndAttack on release, above) |
| X | vehicle | ControlVehicleCommand(4) | (5) |
| X | flag 0x1E (holding object) | ThrowCommand 0x11E | – |
| X | loop-path state 4 | DarkSpinCommand 0x10F | – |
| X | light-dash line found (fn_800A18B0, A0.3) | LightDashCommand 0x110 | – |
| X | A0.4 & +0x5C==1 | SlidingCommand 0x109 | – |
| X | A0.5 | SpinDashCommand 0x10E | – |
| Y | flag 0x23 | ChaosBlastCommand 0x113 | – |
| Y | flag 0x22 | ChaosControlCommand 0x114 | – |
| Y | flag 0x1F & !0x33 & !0xB | CastWeaponCommand 0x119 (drop weapon) | – |
(fn_800A0130 also builds UseCommand/PickupWeaponCommand/GetOnVehicleCommand/ApproachControlCommand from nearby objects
— 0x800A0A5C..0x800A12E8, not fully traced.) Weapon-held B also: flag 0x19 + !A0.7 → CancelUseWeaponCommand (0x8009FAF0).
fn_8009FE58 maps the chosen commands to HUD button-prompt icons (fn_8016F2D4) — LIKELY.

Homing target search: fn_80206B98 (targeting) — range **150** (sbss 0x805F0CE0, static init fn_80207274), cone 45° when the
stick is pushed (|stick|²>0.09) else 90°; also 40°/200° limits (0x805F0CE4/E8). Values PROVEN, geometric roles LIKELY.

Controls available (ShadowControl::vf01 0x800AF0B0): id 0 `Control::UserInput` (0x1C), 1 `Control::Approach` (0x2C, used
by ApproachControlCommand — scripted walk-to), 5 `Control::Bukuu` (0x20). Control vtable: 0 dtor, 1, 2, **3 Update(dt)**,
4 (0x8006D274 default). (PROVEN slots; meaning of 1/2 UNKNOWN)

## 4. Commands and the behavior state machine

### 4.1 Commands (CharCommand, RTTI) — layout PROVEN (CharCommand ctor 0x8006E57C)
+0 vptr, **+4 u16 id**, +6/+7 u8 state bits (bit1 = handled/consumed), +8 priority bucket (0..6), +0xC links, payload from
+0x14. Allocated by `CharCommand_Alloc` 0x801DA024; sent via receiver vslot 2 (enqueue fn_801D9FAC → fn_801DA2DC).
Full id table: `agent_player_cmds.py` (99 classes). Player-relevant ids:
| id | class | id | class | id | class |
|---|---|---|---|---|---|
| 0x01 | AccelerateInputCommand | 0x07 | DamageCommand (+0x14 f32 power, +0x18 Vec3 source) | 0x08 | DeadCommand |
| 0x02 | SetPositionCommand | 0x05 | SetVelocityCommand | 0x04 | ClrVelocityCommand |
| 0x102 | StartAttack | 0x103 | EndAttack | 0x104/0x105 | Start/EndJump |
| 0x106/0x107 | Start/EndAction | 0x108 | JumpDash | 0x109 | Sliding |
| 0x10A | HomingAttack | 0x10B | HomingJump | 0x10C | SetLoopPath |
| 0x10D | Grind | 0x10E | SpinDash | 0x10F | DarkSpin |
| 0x110 | LightDash | 0x113 | ChaosBlast | 0x114 | ChaosControl |
| 0x115/0x116/0x117 | Start/Cancel/EndUseWeapon | 0x118 | PickupWeapon | 0x119 | CastWeapon |
| 0x11E | Throw (Player::) | 0x121 | Launch | 0x122 | Stream |
| 0x123 | GetOnVehicle | 0x126 | Invincible | 0x127 | IgnoreInput |
| 0x12D | SetGravity | 0x12F | ToSuper | 0x132 | WaitTalking |

### 4.2 Command handling order — `ShadowExecuter::OnCommand` 0x800AF924 (PROVEN)
1. id ∉ {1, 0x17} → fn_80078E44(player) (activity notify). 2. 0x106 + flag 0x15 → Effect fn_800A2DCC, consumed.
3. **current behavior vslot 4 (vptr+0x18) OnCommand(cmd)**. 4. `Player_HandleCommand` 0x800A7CE4 (generic: 1 input add, 2
SetPosition (pos, prevPos, +0xDC), 5 SetVelocity (vel += v; leaving ground if moving along normal), 3/0xB/0xC/0x17/0x18/0x1C/
0x132 …). 5. Shadow switch: **7 Damage** → ignored if flag 0x17 or **0x18 (invincible)** or current id==0x1E; else if player
vslot 0x12 (0x800798A4 "ShouldDieOnHit") → Dead(0x1F) else InitializeArgsDamage(cmd+0x18) → Damage(0x1E); **8 Dead** → Dead;
0x108 → JumpDash(6); 0x115/0x116/0x117 → weapon (+0x238) vslots +0x28/+0x2C/+0x30. 6. fallback cmd vslot 1 Execute(player).
Command vtable (CharCommand 0x8053E1E8): 0 dtor, **1 Execute(receiver)**, 2 debug text (e.g. StartJumpCommand::vf02 0x8009CBA8
sprintf " Dir%6.3f % 6.3f % 6.3f"), 3 UNKNOWN. Example (PROVEN): `StartJumpCommand::vf01` 0x8009CC1C: if player A0.1 (can
jump) → `InitializeArgsJump(cmd+0x14 dir)` → Executer_ChangeBehavior(id 5 Jump), mark handled. So a jump press travels:
UserInput::vf03 → StartJumpCommand(0x104) → queue → layer-8 dispatch → OnCommand (behavior vslot4 declines) → Execute → Jump.
(Step 1's fn_80078E44 = semantic UNKNOWN, called for every command except ids 1 and 0x17.)

### 4.3 Behavior object and transitions (PROVEN)
`Behavior::Behavior` 0x80076D70: +0 vptr, +4 player smart ptr (raw ptr at +4), +0xC name string, **+0x10 id**, +0x14 timer pair.
Vtable (7 slots): 0 dtor, **1 CanEnter(prevId,args)→bool**, **2 Enter(prevId,args)**, **3 Leave(nextId)**,
**4 OnCommand(cmd)**, **5 Update(dt)**, 6 UNKNOWN (default blr 0x80076DD8; no caller found).
`Executer` (0x80522B24; ctor 0x800A85C0, 0x58 for ShadowExecuter): +0x28 player smart ptr, **+0x30 current Behavior**,
+0x38 behavior Factory, +0x48 idle-talk timer (10.0), +0x4C talk counter, +0x50 (Shadow) ring-drain accumulator.
`Executer_ChangeBehavior` fn_800A82FC(exec, InitializeArgs*): prevId = cur?cur->id:0x25; new = Factory vslot1(args->id);
if new->CanEnter(prevId,args): cur->Leave(new->id); exec+0x30 = new; new->Enter(prevId,args); return 1. Behaviors call it
as `fn_800A82FC(player+0x22C, &args)`. `InitializeArgs`: +0 vptr (0x80520F18), **+4 behavior id**, subclass payload from +8.
Factory `ShadowBehavior::vf01` 0x800AE8D0: table 0x80522DE8[id] → descriptor (.rodata) word0 = creator. If flag 0x25
(Super) ids 0/2/3 are remapped to 0x22.

| id | class | size | Enter | Update | id | class | size | Enter | Update |
|---|---|---|---|---|---|---|---|---|---|
| 0x00 | Idle | 0x20 | 80086FE4 | 80086D84 | 0x12 | SpinDash | 0x58 | 8008F8D0 | 8008F41C |
| 0x01 | Ground | 0x40 | 80083EE8 | 80083A38 | 0x13 | DarkSpin | 0x68 | 8007D634 | 8007D3EC |
| 0x02 | Fall | 0x60 | 80080ED4 | 8008096C | 0x14 | LightDash | 0x60 | 8008A4B0 | 80089D64 |
| 0x03 | Landing | 0x24 | 800890AC | 80088EA0 | 0x15 | PickupObject | 0x2C | 8008C7B0 | 8008C11C |
| 0x04 | Ottotto (ledge teeter) | 0x20 | 8008BF70 | 8008BE90 | 0x16 | PickupWeapon | 0x2C | 8008D450 | 8008CEF8 |
| 0x05 | Jump | 0x44 | 80087904 | 8008731C | 0x17 | Launch | 0x2C | 8008960C | 80089244 |
| 0x06 | JumpDash | 0x28 | 80088948 | 800883E8 | 0x18 | Stream | 0x1C | 8008FC2C | 8008FB04 |
| 0x07 | Sliding | 0x28 | 8008E188 | 8008DEC0 | 0x19 | GroundAttack | 0x24 | 80084A7C | 800842B4 |
| 0x08 | HomingAttack | 0x30 | 80085FE0 | 80085628 | 0x1A | AirAttack | 0x28 | 8007AAF0 | 8007A774 |
| 0x09 | HomingJump | 0x30 | 80086AC0 | 80086928 | 0x1B | DriveVehicle | 0x48 | 8007EB0C | 8007E6D0 |
| 0x0A | Throw | 0x24 | 800901B8 | 8008FDD8 | 0x1C | ChaosBlast | 0x2C | 8007B438 | 8007AE10 |
| 0x0B | TurnOver | 0x2C | 80090F64 | 800909D4 | 0x1D | ChaosControl | 0x3C | 8007C468 | 8007BB8C |
| 0x0C | EdgeHang | 0x50 | 8007FBB8 | 8007F810 | 0x1E | Damage | 0x28 | 8007CE74 | 8007CCEC |
| 0x0D | WallHang | 0x50 | 800924C4 | 80091C98 | 0x1F | Dead | 0x28 | 8007DAA4 | 8007D92C |
| 0x0E | LineHang | 0x5C | 8008B834 | 8008B274 | 0x20 | ExternalControl | 0x34 | 800804A8 | 80080264 |
| 0x0F | SomethingHang | 0x34 | 8008EB1C | 8008E54C | 0x21 | ElectricCircuit | 0x44 | 80076A6C | 800765B0 |
| 0x10 | WallJump | 0x3C | 80092F04 | 800929FC | 0x22 | Super | 0x2C | 802EBA30 | 802EB96C |
| 0x11 | Grind | 0x70 | 800825D0 | 80081A94 | | | | | |
(all slots: `python agent_player_behvt.py`). Not in Shadow's table (NPC-only): Fly, Hide, FirstMeet.
Observed transitions (PROVEN call sites): Ground→Idle if speed<0.2 (sbss 805EF4B0), Ground→Fall when not grounded
(or Ottotto via fn_800971EC ledge test), Ground→Fall on slopes with n·up<0.4226 (cos 65°) below high speed (vel = n·100);
Fall→Landing when flag 2 (grounded) or ground sensor (+0x278) hit; Fall→EdgeHang (fn_8009827C); Jump→Landing on ground after
10-frame grace (+0x40); Jump→Fall after apex; JumpDash→Fall after 0.4 s; HomingAttack→Fall on target loss or 10 s,
→HomingJump(state 2/3) after hit; Damage→Fall after 1.5 s airborne, else anim end → fn_8007CB28; super ring-out→Dead.

## 5. Movement / physics

### 5.1 Kinematic state (CharInfo / PlayerBase fields; PROVEN by the code cited)
| off | field (semantic) | evidence |
|---|---|---|
| +0x00 | Vec3 position | integrator fn_80093BBC writes it; SetPosition handler |
| +0x0C | Vec3 rotation (Euler, radians; +0x10 = heading) | fn_800A78F4 writes/normalizes (fn_8040F9EC) |
| +0x18 | Vec3 **velocity** | integrator; SetVelocity handler; Jump/SpinDash set it |
| +0x24 | Vec3 **acceleration accumulator** (cleared every frame 0x800A8284) | gravity & behaviors add, integrator reads |
| +0x38 | vptr (CharInfo) | ctor |
| +0x54 | player index (−1 = none) | PlayerBase::vf01 |
| +0x5C | 1 for Player::Player (human-controlled kind) | Player ctor → PlayerBase(…,1,…); UserInput tests ==2 |
| +0x60 | float* dt source | CharInfo ctor / PlayerShadow ctor |
| +0x64 | character id | PlayerBase ctor; indexes tables |
| +0xA0 | u32 **action-enable bits** (§6.2) | BitTest users |
| +0xA4 | u32 terrain-contact bits (1 damage-floor, 2/3 death) | fn_8009F29C/fn_8009F17C |
| +0xA8..+0xB0 | 72-bit **state flags** (fn_80076DDC set / fn_80076DFC clr / fn_8006CEA0 test) | §6.1 |
| +0xB8 | Vec3 move input (world, |v|≤1), cleared per frame | AccelerateInput handler 0x800A7DC8 |
| +0xD0 | Vec3 previous position | 0x800A82AC |
| +0xE8 | Vec3 default up (0,1,0) ; +0xF4 current up | ctor 0x8007A168; fn_800A7AE8 |
| +0x100 | Vec3 gravity vector this frame | fn_800A7AE8 |
| +0x10C/+0x11C | up-frame quaternion / matrix | fn_800A7AE8 |
| +0x1C8 | ground normal ; +0x1D4/+0x1E4 ground-frame quat/matrix | fn_80097658 / fn_800A7AE8 |
| +0x218 | input object (§3.2) | fn_8006F9F8 |
| +0x21C | CharCommandUnit (receiver) | PlayerBase ctor |
| +0x224..+0x240 | Controller, CharaColliMove, Executer, Motion, Attack, Weapon, Effect, Sound | component ctors |
| +0x27C | f32 invincibility time | fn_80079098 / fn_8007915C |
| +0x278 | ground sensor (0x28, fn_8009397C) | Fall::Update |
| +0x290 | held object ref | Player ctor fn_8007374C; Player::vf10 |

### 5.2 Gravity — fn_800A7AE8 (PROVEN) — only if flag 0 set
| condition | g (units/s²) | const addr | consumer pc |
|---|---|---|---|
| flag 1 (set by Idle::Update) | −0.0625 | .sdata2 0x805F3C78 | 0x800A7C00 |
| flag 2 (grounded) | −800 | 0x805F3C7C | 0x800A7C20 |
| otherwise (air) | **−320** | 0x805F3C80 | 0x800A7C28 |
Direction: ground normal (+0x1C8) if A0.0 (speed>250) and !flag 0xD, else up (+0xF4). gravityVec(+0x100) → accel += (0x800A7CC4).

### 5.3 Integrator with linear drag — fn_80079854 → fn_800939E4 → fn_80093A18 → fn_80093BBC (PROVEN)
Exact solution of v' = a − k·v over dt: v ← a/k + (v − a/k)e^{−k dt}; p ← p + (a/k)dt + (v−a/k)(1−e^{−k dt})/k (k=0: plain
Euler-exact). e = 2.71828 (.sdata2 0x805F3A68, pow fn_803B2D0C). Drag k (writable .sdata, i.e. tunables):
| state | k | address |
|---|---|---|
| flags 7 (Sliding) or 9 (SpinDash roll) | 1.0 | .sdata 0x805E5650 |
| flag 2 grounded | **20.0** | 0x805E564C |
| flag 0x1D (AirAttack) | 10.0 | 0x805E5648 |
| air, horizontal (⊥ up) | **3.0** | 0x805E5640 |
| air, vertical (∥ up) | **1.0** | 0x805E5644 |
Called from 31 behavior Update functions (callers of 0x80079854). Terminal speed = |a|/k (e.g. air fall 320/1 = 320).

### 5.4 Ground running — Ground::Update 0x80083A38 / state-0 fn_8008331C (PROVEN values; roles STRONG)
- fn_80082E0C per frame: min accel +0x24 = **1000**, max accel +0x28 = **15000 × charTable[id].+4** (Shadow 1.0)
  (carrying object flag 0x1E: 1000 / 3000). Consts 0x805F3710, 0x805F3718, 0x805F3714; char table 0x804F2904 (0x2C rows).
- accelMag(+0x2C): stick |m|²<0.81 → ×0.9 per frame, floor 1000; full stick: target = lerp(min,max,t),
  t = (cos θ−0.766)/0.234 (θ = input vs facing; 0 if speed<30 or θ>40°); ramp += cos⁸θ·(max−min)·0.25·dt (≈4 s to max).
- accel(+0x24) += m·accelMag, |accel| clamped to +0x28 (0x80083730), then integrate with k=20 ⇒ top speed ≈ 15000/20 =
  **750 u/s** (Shadow); speed > 150 → flag 0x3B (input replaced by facing dir); weapon weight scales vel & accel by (1−w).
- turn rate +0x30: 15 (speed ≤150), 2 (speed >300), else 2+13·(300−speed)/150; applied by fn_80093DE8(rate·dt).
- speed²>62500 (250) → A0.0 (fast: gravity along ground normal); speed>200 (sbss 805EF4B4) → A0.4 (slide allowed);
  speed<0.2 (805EF4B0) → Idle; static init fn_800841E0 sets 0.2/200/30.
- animation blend level: <12.5 →0..1, <30 →1..2, <100 →2..3, <720 →3..4, else 4 (0x800838E0..0x80083974) — LIKELY anim.

### 5.5 Air, jump and dashes (PROVEN constants and consumers)
| mechanic | value | addr | consumer |
|---|---|---|---|
| Fall air steering accel | 220 · input | 0x805F3698 | Fall::Update 0x80080998 |
| Fall facing turn | 15·dt | 0x805F369C | 0x80080A34 |
| Jump initial speed (along jump dir; replaces that component) | **100** (Shadow; Maria 75) | table 0x804B1628 +0 (0xC/char) | Jump::Enter 0x80087F48 |
| Jump hold thrust | **270** u/s² while held | table +4 | Jump::Update 0x800873D8 |
| Jump hold max time | **0.75 s** (EndJumpCommand 0x105 zeroes it) | 0x805F3834 | Enter 0x80087A0C / Jump::vf04 0x80088100 |
| Jump air steering | 220 (110 carrying) | 0x805F3824/3820 | 0x800875D4 |
| Jump turn | 6.5·dt | 0x805F3828 | 0x80087620 |
| Jump from Sliding | vel=0 then +dir·200 | 0x805F383C | 0x80087CE8 |
| Landing grace | 10 frames (+0x40) | li 0xA @0x80087FE4 | Jump::Update 0x80087698 |
| JumpDash impulse / thrust / duration | +180 / 430 u/s² / 0.4 s | 0x805F3848 / 0x805F3850 / 0x805F3840 | JumpDash::Update 0x80088454..0x80088540 |
| HomingAttack thrust along facing | 1000 (vel re-aimed each frame, speed kept) | 0x805F37C0 | 0x8008581C |
| HomingAttack steer rate (+0x28, used ×dt by fn_800853EC per Euler axis; units UNKNOWN) | 8 + 10·t | 0x805F37C4 / 0x805F37BC | Enter 0x80086024 / Update 0x80085718 |
| HomingAttack timeout | 10 s → Fall | 0x805F37BC | 0x800857B4 |
| Homing search range | 150 | sbss 0x805F0CE0 (init 0x80207274) | fn_80206B98 0x80206C98 |
| SpinDash launch speed | 150 + 850·min(charge,1 s) | 0x805F3990 / 0x805F399C | fn_8008F26C 0x8008F354 |
| SpinDash max charge time | 10 s → Idle | 0x805F3994 | 0x8008F29C |
| Damage knockback | vel = up·100 − facing·100 (Super: −facing·700) | 0x805F35F8 ; sbss 805EF458=700 (init 0x8007D158) | Damage::Enter 0x8007CFF4 |
| Ground slide-off | vel = normal·100 | 0x805F3778 | Ground::Update 0x80083D30 |
Per-character table 0x804F2904 (0x2C/char): +0 int (2), **+4 speed scale** (Shadow 1.0, Sonic 1.0, Tails 0.9, Amy 0.6,
Maria 0.4…), +8 bytes (byte0 = 1 for Sonic/Shadow/BossSonic/2P: enables PlayerTarget homing lookup 0x800A06B0),
+0x18/+0x1C sound ids, +0x20..+0x28 RGB. Jump table 0x804B1628 (0xC/char): +0 jump speed, +4 hold thrust, +8 flag byte.

## 6. Flags

### 6.1 State flags +0xA8 (xref: `python agent_player_flags.py A8 [bit]`) — setter/clearer PROVEN, meaning STRONG unless noted
| bit | meaning (semantic) | set / clear |
|---|---|---|
| 0 | gravity enabled | most behavior Enter / Leave |
| 1 | idle-stick (tiny gravity) | Idle::Update |
| 2 | **grounded** | terrain collision fn_80097658 @0x80097CB8; cleared on jump/fall/damage |
| 4 | jumping (EndJump valid) | Jump Enter/Leave |
| 5..0x10 | JumpDash, HomingAttack, Sliding, Grind, SpinDash roll, ElectricCircuit, hanging, LineHang, WallHang(×2), Stream, **DriveVehicle** | respective Enter/Leave |
| 0x11 / 0x12 | in Damage / Dead | Damage / Dead Enter/Leave |
| 0x13 | death by contact type 3 | fn_8009F17C |
| 0x14 / 0x16 | ChaosControl / ChaosBlast behavior active | their Enter/Leave |
| 0x15 | "Slow" (time-slow) task active | fn_800A4AB4 / fn_800A492C |
| 0x17 | extra invulnerability (fn_800794D0) | LIKELY IgnoreInput/Invincible command |
| **0x18** | **invincible (timer +0x27C>0)** | fn_8007915C / fn_80079098 |
| 0x19 | weapon firing | ShadowWeapon vf08/vf09.. |
| 0x1B | move input this frame | AccelerateInput handler; cleared end of frame |
| 0x1D | AirAttack (drag 10) | AirAttack Enter/Leave |
| 0x1E | holding object | Player::vf0E / vf0F |
| 0x1F | has weapon | fn_8023A92C / fn_8023A490 |
| 0x20 | barrier/shield (no ring loss, no death on hit) | fn_8033EFA8 / fn_8033EEF8 — LIKELY shield |
| 0x22 / 0x23 | Chaos Control / Chaos Blast available (Y button) | clear fn_800795FC; setter not found via helper (UNKNOWN) |
| 0x24 | super ring drain active | fn_8027F3B8 / fn_8027E4CC |
| 0x25 | Super Shadow | same pair; remaps Idle/Fall/Landing → Super(0x22) |
| 0x33 | infinite ammo / 2P gun | PlayerShadow::vf0A |
| 0x38 | skip rotation smoothing | many Enter/Leave |
| 0x3B | high ground speed (>150) | Ground::Update |
| 0x3C | parallel (2D path) move | Start/EndParallelMoveCommand |
| 0x3E | ground-type behavior (recompute up from ground) | Ground/Idle/Landing/SpinDash/GroundAttack |

### 6.2 Action-enable bits +0xA0 (set in Enter, cleared in Leave) — PROVEN
0 speed>250 (fast) · 1 can jump · 2 can air action (homing/jumpdash) · 3 can LightDash · 4 can slide (speed>200) ·
5 can SpinDash · 6 can attack · 7 can use weapon · 8 vehicle control · 9 idle (talk timer).

## 7. Damage, death, invulnerability, rings (PROVEN unless noted)
- Ring counter: per-player record = game-state (bss 0x8057707C, getter fn_8007993C) + 0x728 + 0x4C·index (fn_80178F10);
  **rings at record+0x28**; add fn_80178ED4 → fn_8016F66C (clamp ≥0, wraps at 1000 and reports overflow to record+0x20).
- Hit: DamageCommand(7) → (§4.2) → `Damage::Enter` 0x8007CE74: player vslot 0x10 (0x80076EBC: drop held object via
  CastCommand), flags 0x11/0x44, face the source, knockback (§5.5), **ring loss fn_800B8130(&player, 10)**: skipped if flag
  0x24 or +0x214 bit1; flag 0x20 → Effect fn_800A2CD4 instead; else n = min(10, rings), scatter object fn_8011B614(pos, n),
  rings −= n. Rumble fn_8034ED50(4, index). Update: after 1.5 s airborne → Fall; anim end → fn_8007CB28.
  `Damage::Leave` 0x8007CE10: **invincibility 2.0 s** (fn_8007915C(p, 2.0) @0x8007CE5C; const 0x805F35D8).
- Death on hit: vslot 0x12 0x800798A4 returns rings==0 unless flag 0x24 or 0x20 → Dead instead of Damage.
- Other deaths: DeadCommand(8) (terrain contact bits 2/3 via fn_8009F17C; ring-out while Super fn_800AF6A4).
- `Dead::Enter` 0x8007DAA4: drop held, weapon vslot +0x34, flags 0/0x12, anim 0x2A, fn_800795FC (clear chaos flags),
  voice; `Dead::Update` 0x8007D92C: once, when StageState(+8)==3 && +0xC==0 → `StageState_RequestAction(9)` (0x801762D0).
- Invincibility timer only grows (max(current, t)); decremented in Executer step 2; blocks DamageCommand (flag 0x18).

## 8. Chaos powers (partial)
- Y → ChaosControlCommand/ChaosBlastCommand when flag 0x22/0x23 (§3.3). ChaosControl::Update 0x8007BB8C drains karma gauge
  gs+0x70C by max(1, int(dt·max/4.5)) per frame via `KarmaGauge_Add` (i.e. a full gauge lasts **4.5 s**) (PROVEN arithmetic;
  which gauge is "hero" vs "dark" UNKNOWN).
- "Slow" task (fn_800A4AB4 create / fn_800A4830 update / fn_800A492C end; parent via fn_800A34C8 in Player::Effect): ramps
  world time scale gs+0x24 (0x8057E784) and gs+0x30 = 0.125·r (r 0→1 over 0.5 s, back to 0 on end), drains gs+0x70C gauge
  2000·dt, restores both to 1.0 at end (PROVEN writes; "world time scale" STRONG — 0x8057E784 is read by ~49 world fns).

## 9. Open items
- Behavior vslot 6 caller; Control vslots 1/2; R/L/Z logical-button consumers (weapon control 0x8023A584..0x8023E028).
- Setter of flags 0x22/0x23 (chaos availability) and the hero/dark assignment of gauges +0x70C/+0x718.
- Terrain collision internals (fn_80097658, fn_80097DB4, fn_80098ABC mask 0x40000) and which surfaces set +0xA4 bits.
- Units: no evidence for metres-per-unit.
- fn_80009548 (used by SET range checks) is the **camera manager** (camera modes & StageCameraManager call it; UserInput uses
  its entry +0x9C for camera-relative input) — the SET "player position" (fn_80010244 → entry+8) is therefore a
  camera-entry vector, LIKELY the tracked player position, not read from the player object directly.

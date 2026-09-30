# GUN Soldier trace — `GunSoldier::GunSoldier` (SET id 0x0064): AI → damage → "death" → despawn → reporting

Source: `sys/main.dol` (GC). All addresses are DOL virtual addresses. Class names come from RTTI (original);
**every method name below that is not an RTTI `Class::Class`/`~Class` is a semantic guess** and is written in *italics*
or marked "(guess)". Confidence: PROVEN (direct code/data), STRONG (several consistent pieces), LIKELY (single
indirect piece), UNKNOWN.

Builds on the already-verified spawn / layout / lifecycle facts (create hook 0x80196E38, layout, vptr map
V=0x80539E64, `TEnemySetTask::Update` 0x801A6EB8, `TEnemySetBase` lifecycle 0x801A7280). Helper scripts written for
this trace: `tools/agent_enemy_vt.py` (resolve `[[comp]+off]` for GunSoldier), `tools/agent_enemy_vscan.py`
(find virtual-call sites by slot offset), `tools/agent_enemy_states.py` (AI-state singleton getters → class → requesters).

Offsets are given relative to the **subobject** named in the column header; "E+x" = complete GunSoldier object,
"S+x" = `EnemyStatusCommon` subobject (E+0x110), "AI+x" = AI subobject (E+0x360), "M+x" = move subobject (E+0x1E8),
"SB+x" = `TEnemySetBase` (E+0x250).

---------------------------------------------------------------------------------------------------------------------

## 0. Headline findings

1. **A GUN soldier does not die the way other enemies do.** When HP reaches 0 the soldier is converted in place to an
   *injured* soldier ("Hinshi" = 瀕死, near-death): `AppearType`(E+0x274) is forced to 5, weapon/shield dropped,
   AI goes `ToHinshi` → `Hinshi`, damage is ignored from then on, and **no SET "killed" call is made** (enemy type 0 is
   special-cased at 0x801A8C68 / 0x8019E848). The object stays alive until it fades out of range (PROVEN).
2. The kill **is** reported at the death edge: `EnemyStatusCommon::vf23(1)` → EnemyManager `fn_801A250C` →
   `fn_801A2930` (+0x34[team 0 = GUN]++; if killed by player: +0x40[0]++ and `EnemyDeadByPlayerCommand` to the
   player) and link-group countdown (PROVEN). Score: `ScoreCommand` for ScoreData entry 0x4F "GunSoldier"
   (0x50 "GunSoldierShield" if HaveShield) sent to the attacker at the hit that kills (PROVEN).
3. Injured soldiers can be **healed** by `RecoverCommand` (id 0x10, sent only by `HealingUnit::HealingBlast`):
   HP += amount → revive edge → kill count is **decremented** (`vf23(2)`), healer gets ScoreData 0x57
   "GunRecovery" once per SET record (record flag 0x01000000), soldier enters `Thanks` (PROVEN).
4. HP: float at S+0x7C, max at S+0x78 = per-type table 0x804CD678[type 0] = **2.0**; flinch threshold 0.5·max (PROVEN).
5. Detection is a vertical cylinder built from the raw SET misc params at spawn:
   centre = SETpos + rotY(rot.y + (180°+SearchAngle))·(0,0,SearchRange) + (0,SearchHeightOffset,0),
   radius = SearchWidth, half-height = SearchHeight; MoveRange = XZ leash radius around home (PROVEN, fn_801A6DB8).
6. SET record flag 0x8 is set **when a GunSoldier is constructed** (ctor 0x80197140 → 0x801975B0 →
   `SetSlot_MarkDestroyed` 0x800C9C44). So "0x8 = destroyed/killed" is not the whole story (PROVEN; meaning UNKNOWN).

---------------------------------------------------------------------------------------------------------------------

## 1. Per-frame chain (enemy side)

```
Task_UpdateChildren 0x8004ECAC
 └ TEnemySetTask::Update 0x801A6EB8 (Task slot 1, via thunk-992)          §5.1 despawn/fade
    └ TEnemySetBase::vf07 0x801A7280 (lifecycle, state 4 = running)
       ├ Status slot 2 → thunk 0x80198D08 → GunSoldierBase::vf0F 0x801982FC
       │    ├ EnemyStatusCommon::vf02 0x801A8A38   *StatusUpdate*: flags, death/revive edges   §3.3 §4.1
       │    └ [[E+0x18]+0x29C] = GunSoldier::vf25 0x801975DC  *WriteTempState* (STemporaryData)  §4.6
       ├ Status slot 3 (0x801A87D0)
       ├ AI slot 1 = EnemyBaseAI::vf01 0x8019EACC  *AIUpdate*                                  §2
       ├ Status slot 4 (0x801A8768), Disp slot 1 (0x801A1370), Move slot 1 EnemyMoveCommon::vf01 0x801A5B34
       ├ Disp slot 2 → thunk 0x80198D48 → GunSoldierBase::vf0B 0x80197D64 (*look-at / weapon update*, fires weapon)
       └ Status slot 5 = EnemyStatusCommon::vf05 0x801A86CC  *EndFrame*: S+0x74=S+0x75=0, S+0x84=-1 (PROVEN 0x801A86F8..0x801A8700)
```
Damage arrives *outside* this chain from the CharaColli layer callbacks (§3.1) and is consumed by the next
`StatusUpdate`.

---------------------------------------------------------------------------------------------------------------------

## 2. AI: how `EnemyBaseAI` runs `TEnemyAIState` objects

### 2.1 AI subobject fields (E+0x360)

| AI off | Meaning | Evidence | Conf |
|---|---|---|---|
| +0x18 | vptr (GunSoldier: V+0x2A4 = 0x8053A108) | ctor 0x80197124 | PROVEN |
| +0x1C | vbptr → EnemyReferer (→ E) | 0x8019F1F8 | PROVEN |
| +0x20 | f32 dt of this frame | `stfs f1,0x20(r3)` 0x8019EAE0 | PROVEN |
| +0x24 | s32 per-state sub-step counter (states write 0 on enter, ±1) | Attack 0x80187354, Standing 0x8018967C | PROVEN (use) |
| +0x28 | **StateMachine** `{+0 owner(AI*), +4 cur state, +8 prev state, +0xC u8 locked}` = AI+0x28/+0x2C/+0x30/+0x34 | fn_8019FD00; ctor zeroes 0x8019F21C | PROVEN |
| +0x38 | target tracker (target pos cached at +0x44; `fn_8019FA18` *GetTargetPos*, `fn_8019FACC` *DistToTarget* LIKELY) | fn_8019FA18 0x8019FA2C..0x8019FA40 | STRONG |
| +0x40 | u8 cleared every frame | 0x8019EB6C | PROVEN (meaning UNKNOWN) |
| +0x60 | turn helper `{+0 E, +4 AI, +8 rate}`; `fn_8019F8F8(h,dir)` *TurnTowards* (rotates E yaw ≥1°/frame, returns |Δyaw|<1°) | 0x8019F8F8 | STRONG |
| +0x6C / +0x70 | f32 state timer (decremented by dt in vf09) / its reset value | 0x8019EB70..0x8019EB7C | PROVEN |
| +0x78 | SoldierCommonAI: soldier pointer used by WaitAction states (`[+0x78]+0x280` patrol index) | 0x80189614 | STRONG |
| +0x7C | GunSoldierAI: `dynamic_cast<GunSoldierBase*>` of the enemy | GunSoldierAI ctor 0x80188748 | PROVEN |

### 2.2 State-machine primitives (PROVEN)

| Addr | Guess name | Behaviour |
|---|---|---|
| 0x8019FD00 | *SM::ChangeState(sm,new)* | if `sm+0xC` (locked) → nothing. else cur→vf01(owner), cur→vf04 (*Exit*); if new: prev=cur, cur=new, new→vf01(owner), new→vf02 (*Enter*) |
| 0x8019FC54 | *SM::RevertToPrevious* | same, with new = prev (+8) |
| 0x8019FDB8 / 0x8019FDAC | *SM::Lock / Unlock* | `sm+0xC = 1 / 0` |
| 0x8019F528 | *SM::Init(sm, owner)* | called from EnemyBaseAI ctor 0x8019F248; installs `EnemyBaseAI_None` (getter 0x8019F5E4) |

`TEnemyAIState` vtable (6 slots, `vptr+8+4i`): **[1] vf01 *BindOwner*** (`state+4 = dynamic_cast<XAI*>(owner)`,
e.g. 0x80188790) — states are **static singletons shared by all soldiers**, so the owner is re-bound before every
call; **[2] vf02 *Enter***, **[3] vf03 *Update***, **[4] vf04 *Exit***, [5] 0x8017F190 returns `"none"` (0x805F60A4, name
string). Proof of slot roles: `EnemyBaseAI::vf01` calls vf01 then vf03 each frame (0x8019EB10, 0x8019EB24); ChangeState
calls vf04 on old and vf02 on new (0x8019FD54, 0x8019FD94).

Singletons are function-local statics: getter = guard byte in .sbss + ctor + `__register_global_object` (e.g.
Standing: guard 0x805F04F8, object 0x805F04FC, 0x80188630..0x8018865C).

### 2.3 `EnemyBaseAI::vf01` 0x8019EACC (*AIUpdate*) and `vf09` 0x8019EB54 (*PreUpdate*) — PROVEN

```
vf01(ai, dt): ai+0x20 = dt; ai->vf09(); if cur: cur->vf01(owner); cur->vf03(); ai->vf0A() (0x8019EB50 = blr)
vf09(ai):
  ai+0x40 = 0; ai+0x6C -= dt
  if S.IsDead(S+0x90):                                  0x8019EB98 (status slot 0x14)
      if !S.WasDead(S+0x91): unlock; ai->vf14()          0x8019EBBC..0x8019EBE0  → GunSoldier: ToHinshi
      return
  if byte[0x8057E828] != 0: ChangeState(Idle) (inline)   0x8019EBF0 (global flag also read by Player::Control::UserInput::vf03; meaning UNKNOWN, LIKELY event/control lock)
  elif BitTest(S+0x80, 5): BitClear(5); ai->vf12()      0x8019ECA8..0x8019ECE8 → Guard (bit 5 = shield guard, §3.2)
  elif S+0x8C (flinch this frame): unlock; ai->vf13()   0x8019ED08..0x8019ED2C → WaitFinishDamaged
  elif !Move.vf06() && S+0x8E (airborne): ChangeState(WaitFinishFalling) 0x8019ED4C..0x8019EE00
```
(`EnemyMoveWalker::vf06` 0x8017B924 always returns 0; `S+0x8E = !(M+0x38 < 5.0)` from `EnemyMoveCommon::vf04`
0x801A5B1C, set at 0x801A8AE0 → *airborne*.)

### 2.4 AI virtual slots → state requests (GunSoldier vtable 0x8053A108)

| AI slot | Target | Requests state | Called from |
|---|---|---|---|
| 0x0B | EnemyBaseAI::vf0B 0x8019D8C0 | Idle | cmd 0x201 |
| 0x0C | **GunSoldierAI::vf0C 0x801885CC** *EnterInitialState* | +0x356 (rescued) → Thanks; else by AppearType(E+0x274): 0 → WaitAction_Standing, 1‑3 → WaitAction_Moving, 4 → AppearHome, 5 → Hinshi | AI vf00 0x8019EED4 (init), cmd 0x202 |
| 0x0D | **GunSoldierAI::vf0D 0x801883E0** *ReturnToWait* | as 0x0C but AppearType 4 → Standing | GoHome, Guard, WaitFinishDamaged, cmd 0x203 |
| 0x0E,0x10,0x11 | EnemyBaseAI | base Wait | cmds 0x204/0x206/0x207 |
| 0x0F | **GunSoldierAI::vf0F 0x801882B0** *OnTargetFound* | GunSoldier Caution | WaitAction states, cmd 0x205, ReleaseCommand |
| 0x12 | EnemyBaseAI::vf12 0x8019D328 | Guard | vf09 bit 5, cmd 0x208 |
| 0x13 | EnemyBaseAI::vf13 0x8019D1CC | WaitFinishDamaged | vf09 flinch, cmd 0x209 |
| 0x14 | **GunSoldierAI::vf14 0x801884E8** *OnDead* | **ToHinshi** (base would be `EnemyBaseAI_Dead`) | vf09 death edge 0x8019EBE0, cmd 0x20A |
| 0x15 | 0x8019D06C (nop) | — | cmd 0x20B |
| 0x16 | **GunSoldierAI::vf16 0x801882E8** *OnRescued* | E+0x356=1, unlock, → Thanks | RecoverCommand 0x8019E600, cmd 0x20C |

(All PROVEN from vtable slots + getter mapping by `agent_enemy_states.py`.)

### 2.5 GunSoldier / SoldierCommon state table

| State (RTTI class) | getter / singleton | vtable | Enter | Update | Exit | Transitions (Update unless noted) | Conf |
|---|---|---|---|---|---|---|---|
| SoldierCommonAI_WaitAction_Standing | inline, obj 0x805F04FC | 0x805316B4 | 0x801895F4 | 0x80189308 | 0x80189304 | end of Update: `Detect(E+0xA8, AI.vf02()=nearest player pos)` → AI.vf0F (Caution) @0x801895C0..0x801895DC | PROVEN |
| SoldierCommonAI_WaitAction_Moving | inline, obj 0x805F0508 | 0x80531688 | 0x801891C4 | 0x80188B98 | 0x80188B58 | start of Update: Detect → Caution @0x80188BDC..0x80188BF8; patrol between points (§2.7) | PROVEN / patrol STRONG |
| GunSoldierAI_Caution | 0x80186D50, 0x805F06B0 | 0x805312E0 | 0x80187878 | 0x801877D4 | 0x801877D0 | turn to target; WeaponType(E+0x278) 0/1 → ChaseTarget; else when S+0x3B (motion flag, LIKELY "motion end") → `fn_801881A0` *StartCombat* (Weapon 0/1 → ChaseTarget, 2‑6 → Attack) | PROVEN |
| GunSoldierAI_Attack (ranged) | 0x80188204, 0x805F06D4 | 0x8053125C | 0x801872B0 | 0x80186ED0 | 0x80186E80 | sub-step 0 aim (turn) → fire anim, timer 3 s; step 1 back to aim when timer <0 or motion flag with |Δyaw|>30°; **not detected → GoHome** @0x801871C8..0x801871E4 | PROVEN |
| GunSoldierAI_ChaseTarget | 0x801874BC, 0x805F06BC | 0x805312B4 | 0x80187760 | 0x801875F8 | nop | not detected → GoHome; `DistToTarget < 15` → NearAttack @0x80187680 | PROVEN |
| GunSoldierAI_NearAttack (melee) | 0x801876B4, 0x805F06C8 | 0x80531288 | 0x80187568 | 0x80187370 | nop | not detected → GoHome; after motion flag and dist > 20 → ChaseTarget @0x80187420 | PROVEN |
| GunSoldierAI_GoHome | 0x80187204, 0x805F06E0 | 0x80531230 | 0x80186DFC (home E+0x6C := SET pos E+0x48) | 0x80186C64 | nop | walk/turn to E+0x6C; Detect → Caution; XZ distance < 10 → AI.vf0D (wait) | PROVEN |
| GunSoldierAI_AppearHome | 0x801880F4, 0x805F06EC | 0x80531204 | 0x80186BDC | 0x80186B48 | nop | walk to home; XZ dist < 10 → StartCombat | PROVEN |
| GunSoldierAI_ToHinshi | 0x80188520, 0x805F068C | 0x80531364 | 0x80188078 (lock; anim param slot 0x16 → 0xB/0xC) | 0x80187EE4 | 0x80187EBC (unlock) | if `Move.vf02` (M+0x5F, LIKELY fell/out-of-bounds) → SB.vf04 + SB.vf02 (despawn, respawnable); elif motion end (S+0x3A) → unlock, Hinshi | PROVEN |
| GunSoldierAI_Hinshi | 0x80187FC0, 0x805F0698 | 0x80531338 | 0x80187DB8 | 0x80187D14 | 0x80187C84 | Enter: lock, collision mask 0x200201 (fn_8019765C), anim 9/0xA, timer rand(30,60); Update: every timer expiry play SE 0x600F (LIKELY "help" voice); leaves only via AI.vf16 (unlock) | PROVEN |
| GunSoldierAI_Thanks | 0x80188334, 0x805F06A4 | 0x8053130C | 0x80187BCC | 0x801879A8 | 0x801878E8 | Enter: S bit 0xD, lock, anim 0xD. Loops thanks anims while target in sensor; **if damaged (S+0x8A) → unlock, ChaseTarget** @0x80187B88..0x80187BAC | PROVEN |
| base Guard / WaitFinishDamaged / WaitFinishFalling / Idle | 0x8019D3D8 / 0x8019D27C / 0x8019EE18 / 0x8019D970 | — | 0x8019C474 / 0x8019BE88 / 0x8019BD40 / 0x8019CE34 | 0x8019C3D0 / 0x8019BE18 / 0x8019BCD8 / 0x8019CD6C | — | Guard & WaitFinishDamaged → AI.vf0D when motion ends; WaitFinishFalling & Idle → *RevertToPrevious* (0x8019FC54) | PROVEN |

Not used by GunSoldier (overridden): `EnemyBaseAI_Dead` (0x8019D120; its Update 0x8019BB58 calls SB.vf02 → delete) and
`EnemyBaseAI_Recover` (0x8019CFC0).

Weapon fire (STRONG): `Attack` only plays the fire motion; the shot is emitted by the **motion event callback**
`GunSoldierBase::vf18` 0x80197CF4 (Disp slot 0xE): event id 0x5000 with data==2 → `weapon[0] = E+0x358`,
`fn_801AB790(weapon)` 0x80197D44. Weapon object = `EnemyWeapon` (0x68 bytes) at E+0x350, created in
`GunSoldierBase::vf17` 0x8019802C.

### 2.6 Detection sensor and SET parameters (PROVEN)

`TEnemySetBase::vf06` 0x801A7730 → `fn_801A7608`: position/rotation from the SET slot (E+0x48 = SET position via
fn_801A7718), then `fn_801A6DB8(E+0xA8, pos, rotRad, SetSlot_GetMiscParams(slot))` @0x801A76E4, where
`fn_800CA118` returns `record+0x28` if the descriptor has a param schema (0x800CA138..0x800CA13C).
**It reads the raw record misc block, not the clamped copy at E+0x2E8.**

| SET param (misc idx) | Where read | Use | Conf |
|---|---|---|---|
| 0 MoveRange | fn_801A6DB8 0x801A6DFC → sensor+0x18, ²→+0x1C | XZ leash radius around sensor home (+0/+8): `fn_801A6D00` used by `EnemyMoveCommon::vf01` 0x801A5C44 clamps movement | PROVEN |
| 1 SearchRange | 0x801A6E2C → z of offset vector | forward offset of detection centre | PROVEN |
| 2 SearchAngle (deg) | 0x801A6DF8 | yaw of that offset = rot.y + (180+SearchAngle)·π/180 | PROVEN |
| 3 SearchWidth | 0x801A6E80 → +0x38, ²→+0x3C | detection radius (XZ) | PROVEN |
| 4 SearchHeight | 0x801A6E8C → +0x40 | detection half-height | PROVEN |
| 5 SearchHeightOffset | 0x801A6E18 | Y offset of detection centre | PROVEN |
| 6 MoveSpeedRatio | not read by GunSoldier code (raw scan of 0x8017B000–0x801B0000 for disp 0x300) | UNKNOWN | — |
| 7 HaveShield | fn_80198D90 0x80198D98 → E+0x270 (bool); clamp 0..1 at 0x80198560 | shield present; ScoreData 0x50 instead of 0x4F (GunSoldierBase::vf13 0x801982E4 reads E+0x304) | PROVEN |
| 8 WeaponType | → E+0x278 (0x80198DC8), clamp 0..6 | 0/1 melee path (ChaseTarget/NearAttack), 2‑6 ranged (Attack); anim selection 0x80197A50 | PROVEN |
| 9 AppearType | → E+0x274 (0x80198DC4), clamp 0..5 | initial state (§2.4); 4 = spawn offset by (Pos1_X, Pos1_WaitSec, Pos1_Z) @0x801988E0 and hidden until detection (GunSoldierBase::vf11 0x80198450), 5 = injured | PROVEN |
| 10‑22 Pos0..2 WaitType/WaitSec/MoveSpeedRatio/X/Z | fn_80198D90 → patrol table E+0x288 + i·0x18 {pos(home + X/Z offset), WaitSec, MoveSpeedRatio, WaitType} (0x80198DE0..0x80198EC0); mode E+0x27C = GunSoldierBase::vf0D 0x80197938 from AppearType (0→0, 1→1, 2→2, 3→3, 4/5→0); mode 2 fills points 1 and 2, mode 1 point 1; **mode 3 instead stores min(param16 "Pos1_WaitSec", MoveRange) at E+0x284 and calls fn_80198F18** (0x80198EC8..0x80198EE8, LIKELY random-wander radius) | patrol for WaitAction_Moving (per-point wait animation via GunSoldierBase::vf0C 0x80197988 on WaitType) | PROVEN (reads) / STRONG (patrol meaning). Note: param 16 is also used as a Y offset for AppearType 4 — the schema name "Pos1_WaitSec" is probably not its only meaning. |

Detection test `fn_801A6C7C(sensor, targetPos)` (PROVEN): `|target.y − sensor+0x30| < SearchHeight` **and**
`distXZ²(target, sensor+0x20) < SearchWidth²`. Callers for GunSoldier: Standing/Moving/GoHome/Attack/ChaseTarget/
NearAttack/Thanks updates and GunSoldierBase::vf0B/vf11. Target = `EnemyBaseAI::vf02` 0x8019DF7C (nearest player
position via fn_800742BC/fn_800A8E64, else own position — STRONG).

Per-type constant tables indexed by `E+0x20` (enemy type; GunSoldier = 0 because `GunSoldierBase` ctor passes
`li r5,0` at 0x80198B40 down to `Enemy::Enemy` which stores it at 0x8018F880 — PROVEN; other types: GunBigfoot 2,
BkGiant 4, BkSoldier 5, BkWorm 8, BkNinja 0xB, EggPierrot 0xD, EggShadowAndroid 0xE):
MaxHP 0x804CD678 (type0 = 2.0), team 0x804CD310 (type0 = 0), knockback 0x804CD6B4 (type0 = {50, 1.3, 1.0}),
misc 0x804CD4D4 stride 0x1C (type0 = {10,10,10,0,0,6,120}), s16 0x804CD34C (type0 = 40, used as effect id in Recover).

---------------------------------------------------------------------------------------------------------------------

## 3. Health and damage

### 3.1 Entry point: CharaColli callback (STRONG)
`EnemyStatusCommon::vf01` 0x801A9088 calls `fn_801AA010(S+8)` @0x801A90E8, which builds the body collision and binds two
member callbacks via boost::function PMFs: `{0,-1,0x801A978C}` (0x8053BCCC → slot +0x10 of the collision reaction
object, 0x801AA448) and `{0,-1,0x801A9750}` (0x8053BCD8 → slot +0x1C, 0x801AA4D8). S+8 is (LIKELY) the non-polymorphic
`EnemyCharaColliModule` — RTTI only knows it via `boost::_bi::bind_t<void, mf1<void, EnemyCharaColliModule,
SonicteamUSA::System::CharaColli::AttackCallbackParam&>>` (typeinfo 0x805E9318, referenced at 0x801A9D4C next to these
functions). Arg = `AttackCallbackParam&` ("hit"): +0 attacker collision handle (smart ptr), +0x18 attacker ref,
+0x34 hit position, +0x50 f32 damage, +0x54 s32 result code.

* `fn_801A9750` 0x801A9750 (*OnDamaged*, guess): `hit+0x54 = 5`, then **Status slot 0x1A** → thunk 0x80198CF8 →
  `GunSoldierBase::vf12` 0x80197700. PROVEN.
* `fn_801A978C` (*OnContact*, guess): if `S.vf0F` (S+0x94 > 0, "recently hit") and attacker+0x5C==1 → `hit+0x30 = 1`. LIKELY.

### 3.2 `GunSoldierBase::vf12` 0x80197700 (*TakeHit* with shield) — PROVEN control flow
```
if E+0x274 (AppearType) == 5: return                         // injured soldiers are immune     0x8019771C
E+0x356 = 0
if shield (E+0x348) present && attacker handle:
   if atk.flags(+0x40) & 0x40 && |angle(attacker→E) vs facing| < 0.5π:     // frontal block    0x80197754..0x80197780
        hit+0x54 = 5; BitSet(S+0x80, 5) (→ Guard); SE 0x6074; return (no damage)
   if !(atk.flags & 0x4):                                    // shield absorbs                  0x801977D4
        shield.TakeHit(hit) (fn_801A78C4→fn_801A8008); hit+0x54 = 1
        if shieldHP([obj]+0xC) < 1: SE 0x6075; S+0x75 = 1; release shield; E+0x270 = 0
        else: SE 0x6074; BitSet(S+0x80, 5)
EnemyStatusCommon::vf1A(S, hit)                              // damage IS still applied here     0x80197890 / 0x801978A0
E+0x357 = (S+0x84 == 0)                                      // hit-side flag copy
```
(SE = `fn_8019ADFC`/`fn_8019AD10` → sound manager fn_8005328C/fn_801D5894; "SE" naming LIKELY.)

### 3.3 `EnemyStatusCommon::vf1A` 0x801A83B0 (*ApplyHit*) and `vf1C` 0x801A8718 (*ApplyDamage*) — PROVEN
```
hit+0x54 = 1
hpBefore = S+0x7C
if S+0x76 == 0 && vf1C(hit+0x50):          // vf1C: if dmg>0 {S+0x74=1; S+0x94+=1.0; HP-=dmg; if HP<=0 {HP=0; return 1}} return 0
     S+0x76 = 1                            // killed latch (blocks further damage until revive)
     if attacker(hit+0x18): vf24 → Score_Award(attacker, GunSoldierBase::vf13 = 0x4F/0x50)   0x801A8464, fn_802067B0
          if fn_8021CCDC(attacker) (attacker+0x5C==1 && +0x50∈{1,2,3}, LIKELY "is player"): BitSet(S+0x80, 0xC)  // killed by player
     S+0x75 = 1
S.vf31 = 0.5·MaxHP; if hpBefore > thr && HP <= thr: S+0x75 = 1        // flinch threshold  0x801A84A8
if S+0x74: Disp.vf10(hit+0x18, hit+0x34)  (EnemyDispCommon::vf10 0x801A0B64, hit effect LIKELY)
S+0x84 = (angle to attacker < 0.75π) ? 0 : 1; if S+0x75||S+0x76: face attacker (yaw via fn_8019B594)
```

### 3.4 Status fields (S = E+0x110)

| S off | Meaning | Evidence | Conf |
|---|---|---|---|
| +0x08 | collision module (EnemyCharaColliModule, LIKELY); `fn_801A9918(S+8, mask)` sets collision attribute (0x100000 dead, 0x200201 injured) | 0x801A8880, 0x80197674 | STRONG |
| +0x20 | EnemyMotionSequenceModule; S+0x3A / S+0x3B motion flags (vf09/vf08; LIKELY "motion ended") | vf06 0x8017B9D0 | STRONG |
| +0x70 | current action/motion id (vf06) | 0x8017B9E4 | PROVEN |
| +0x74 | damaged this frame | vf1C 0x801A872C; cleared vf05 | PROVEN |
| +0x75 | flinch/knock this frame (kill or 50 % threshold, shield break) | 0x801A8490, 0x801A84C4 | PROVEN |
| +0x76 | killed latch | 0x801A8448; cleared on revive 0x801A8D10 | PROVEN |
| +0x78 / +0x7C | f32 MaxHP / HP | vf00 0x801A9598..0x801A959C; vf2A/vf29 getters | PROVEN |
| +0x80 | bitset (vf28 returns &S+0x80): bit2 in damage reaction, bit5 shield-guard request, bit0xC killed-by-player, bit0xD Thanks active, bit0xE (capture, LIKELY) | see refs | PROVEN (bits) / meanings as stated |
| +0x84 | hit side 0 front / 1 back / −1 none | 0x801A8528/0x801A8574, vf05 | PROVEN |
| +0x8A/+0x8B | damaged latch cur/prev | 0x801A8AE4..0x801A8AF0 | PROVEN |
| +0x8C/+0x8D | flinch cur/prev (read by AI vf09 via vf12) | 0x801A8B30..0x801A8B50 | PROVEN |
| +0x8E/+0x8F | airborne cur/prev | 0x801A8ACC..0x801A8AE0 | PROVEN |
| +0x90/+0x91 | **IsDead (HP<=0) cur/prev** (sdata 0x805E932A=0 / 0x805E932B=1) | 0x801A8BA4..0x801A8BD4 | PROVEN |
| +0x94 | f32 recent-hit timer (+1 per hit, −dt per frame, ≥0) | 0x801A8734, 0x801A8A8C | PROVEN |

HP init: `GunSoldierBase::vf0E` 0x80198340 (Status slot 0) → `EnemyStatusCommon::vf00` 0x801A9560: HP = MaxHP = Param
slot 0x19 → `EnemyParamCommon::vf19` 0x801A662C = `0x804CD678[E+0x20]` = **2.0**. If AppearType==5: HP = 0, S+0x90 =
S+0x91 = 1 (born dead, so no death edge) 0x80198368..0x80198380. PROVEN.

### 3.5 Reactions
* **Knockback** (PROVEN mechanism): in StatusUpdate when S+0x75: `Move slot 0xB = EnemyMoveCommon::vf0B` 0x801A59C8:
  M+0x5D=M+0x5E=1, velocity M+0x50 = rotY(yaw)·(0,0,±1)·50.0 (sign from S+0x84), M+0x54 (y) = 1.3·ParamVf18(=120.0).
  Values from table 0x804CD6B4/0x804CD4D4 type 0. Semantics of the constants LIKELY.
* **Flinch**: S+0x8C → AI.vf13 → WaitFinishDamaged (sets S bit 2, anim, returns to wait on motion end). PROVEN.
* **Guard**: shield hit → S bit 5 → AI.vf12 → Guard. PROVEN.
* **Invulnerability**: (a) AppearType 5 (injured) ignores all hits; (b) S+0x76 latch after the killing hit;
  (c) frontal shield block. No generic i-frame timer found (S+0x94 only feeds `vf0F`). PROVEN for (a)–(c);
  "no other" is LIKELY.

### 3.6 Message handler `EnemyBaseAI::vf08` 0x8019E058 (PROVEN)
Registered in `EnemyBaseAI::EnemyBaseAI` 0x8019F1D4 by binding PMF `{0, 0x28 (slot), 0x18 (vptr)}` at 0x8053B6B8
(@0x8019F2C8) and `fn_801A0120(port, fn)` @0x8019F33C. Messages are `CharCommand` (+4 u16 id, +7 flags bit1=handled).
Ids 0x201‑0x20C → AI slots 0xB‑0x16 (jump table 0x8053B76C). Ids 0x10‑0x1B (table 0x8053B73C), matched to
`CharCommand` subclasses by ctor ids:

| id | class | GunSoldier effect |
|---|---|---|
| 0x10 | RecoverCommand (ctor 0x801DB698, amount at +0x1C; only constructor caller fn_80108294 inside the `HealingUnit::HealingBlast` code block) | `S.vf2B` *AddHP*(msg+0x1C) @0x8019E4A8 (clamped to MaxHP, 0x8017BC08); fn_801A2294(…,0x2AA) (effect, LIKELY) + SE 0xE02F (fn_8019ADFC @0x8019E528); if !S bit 0xD and sender valid and first time for this SET record (record flag 0x01000000, test fn_800CA268 @0x8019E590 / set fn_800CA298 @0x8019E5A4): `S.vf25` → Score_Award(healer, 0x57 "GunRecovery"); then AI.vf16 → Thanks @0x8019E600 |
| 0x14 | VacuumCommand | if attacker: `S.vf24` score award + S bit 0xC when fn_8021CCDC; `S.vf23(1)` @0x8019E818 (counts as a kill); `S.vf26` = GunSoldierBase::vf14 (injure); type 0 → `SB.vf04` (despawn, respawnable) else `SB.vf03`; `SB.vf02` delete (0x8019E838..0x8019E890) |
| 0x16 | AnnounceAttackCommand | handled flag only |
| 0x17 | SetTransparentCommand | fn_8019AE60 (alpha) |
| 0x19 | GenerateCommand | → WarpWait |
| 0x1A / 0x1B | Bind / ReleaseCommand | → CaptureWait / AI.vf0F |
| 7 | DamageCommand | **not handled here** (damage uses the CharaColli path) |

---------------------------------------------------------------------------------------------------------------------

## 4. Death

### 4.1 Death edge in `EnemyStatusCommon::vf02` 0x801A8A38 (first StatusUpdate after HP hit 0) — PROVEN
Condition `S+0x90 && !S+0x91` (0x801A8BD8..0x801A8C0C). Sequence for GunSoldier:
1. `S.vf2F` (0x8017BC80 → 0) so `S.vf23(1)` @0x801A8C40 → **kill report** (§4.2).
2. `S.vf26` → thunk 0x80198CF0 → **`GunSoldierBase::vf14` 0x80198184** *BecomeInjured* (§4.4).
3. `if E+0x20 (type) == 0` → skip `TEnemySetBase::vf03`; `fn_801A8D6C(S,0,1)` (collision off, LIKELY) @0x801A8C68..0x801A8C78.
   (Any other enemy type: `SB.vf03` = SET "killed" detach, then `fn_801A8D6C(S,0,0)`.)
4. S+0x64 attachment → fn_8031AFB4 (UNKNOWN).
Same frame, `EnemyBaseAI::vf09` sees the edge → `GunSoldierAI::vf14` → **ToHinshi** (§2.5).

Revive edge `!S+0x90 && S+0x91` (0x801A8CB8..0x801A8D4C): `S.vf23(2)` (un-count), S+0x76=0, clear S bit 0xC,
`S.vf27` → `GunSoldierBase::vf15` 0x8019813C (*OnRevived*: E+0x356 = 1 "rescued", AppearType = 0, Disp.vf14(1)),
`fn_801A8D6C(S,1,1)`, fn_8031B020.

### 4.2 Kill reporting — `EnemyStatusCommon::vf23` 0x801A8998 → EnemyManager (PROVEN; manager fields per
`notes/mission_trace.md` §1.4/§4.2)
* flags = 1 (death) | 4 if S bit 0xC (killed by player) (0x801A89AC..0x801A89CC); 2 = revive.
* `fn_801A250C(EnemyManager=fn_80119560(), slot=SB.vf09 (E+0x40C), team=EnemyParamCommon::vf00 (0x804CD310[0]=0 GUN), flags)`:
  looks up the SET link id (record+0x22) in map EnemyManager+0x10; **if the link group is not registered nothing is
  counted**; flags&2 → +0x34[team]−−, group+1; flags&1 → `fn_801A2930`, group−1, and at ≤0 →
  `fn_80169460(fn_800CBA84(), link, 1)` (link-group cleared event).
* `fn_801A2930`: +0x34[team]++ (read by `Mission::EnemyMission::vf0B` 0x801F3CA4 via fn_801A24FC → mission counter);
  if flags&4: +0x40[team]++ and `new Player::Npc::EnemyDeadByPlayerCommand(team, +0x40[team])` (0x8007311C: id 0x403,
  +0x14 group, +0x18 count; its debug print 0x80072FB0 is `" Group %d Count %d"`) sent to the player; executing it
  (vf01 0x8007300C) calls `fn_802F2E20(npc, group, count)`, which picks an NPC partner line every 3rd kill (LIKELY).
* Team ids confirmed by recovery score names: team 0 → 0x57 GunRecovery, 1 → 0x68/0x6E EggRecovery, 2 → 0x62 BkRecovery
  (`EnemyStatusCommon::vf25` 0x801A8894).
* Stage-init counting (`fn_801A2BA0`, test @0x801A2C14) **skips GUN_SOLDIER records whose SET AppearType is 5**;
  the continue/restart per-link recount (`fn_801A2A30`, called via fn_801A28A8/fn_801A28DC) skips GUN_SOLDIER records
  whose `STemporaryData` state == 1 (0x801A2AB4..0x801A2ABC) or, when no temp data exists, whose AppearType is 5
  (0x801A2AE0..0x801A2AE8). I.e. injured soldiers are never counted as enemies still to defeat (PROVEN).

### 4.3 Score (PROVEN plumbing, LIKELY column meaning)
`Score_Award` 0x802067B0(attackerRef, id): id 0x27 = none; else copies 5 words of `common/ScoreData.bin` entry
(table loaded by fn_80206868 into [0x805F0CDC], 0x18-byte entries, name offset in word 5) into a `ScoreCommand`
(id 0x1C, ctor 0x801DAC54, fields +0x14..+0x24) and sends it to the attacker.

| id | name (from file) | w0 | w1 | w2 | w3 | w4 |
|---|---|---|---|---|---|---|
| 0x4F | GunSoldier | 150 | 0 | 0 | 3000 | 0 |
| 0x50 | GunSoldierShield | 200 | 0 | 0 | 3000 | 0 |
| 0x57 | GunRecovery (heal) | 0 | 0 | 200 | 0 | 3000 |

Player consumer `fn_800A7CE4` case 0x1C (0x800A7FEC): per-player score object (+0x2C): kind0 += w1, kind1 += w2,
kind2 += w0; global object (+0x70C): kind0 += w4, kind1 += w3. Given rings award w1 and Black Arms kills award
w2/w4, LIKELY w0/w3 = Dark gauge/Dark score, w2/w4 = Hero gauge/Hero score, w1 = normal score.
No ring/item drop code was found on the GunSoldier death path (UNKNOWN whether rings drop elsewhere).

### 4.4 `GunSoldierBase::vf14` 0x80198184 (*BecomeInjured*) — PROVEN
`EnemyStatusCommon::vf26` (collision mask 0x100000); drop shield (E+0x348, fn_801A7880); if weapon E+0x350: throw it
with velocity rotY(±yaw)·(0,20,10) via `fn_801AAC40` → `fn_80221F2C` → new 0x80-byte task on layer 0xB whose ctor
0x802224B0 names it **"ScatteringWeapon"** (the droppable weapon pickup, STRONG), weapon vf(delete), E+0x350 = 0,
WeaponType E+0x278 = 0; if AppearType != 5: AppearType = 5 and SE 0x600E once (0x801982A0..0x801982AC);
Disp.vf14(0).

### 4.5 Persistence across respawn — `GunSoldier::STemporaryData` (RTTI, base `SetAdapterState`) — PROVEN
Stored at SET slot+0x14 (`SetSlot_SetField14` 0x800C9D18, created in `GunSoldierBase::vf24` 0x8019886C..0x801988A4,
0xC bytes). `GunSoldier::vf25` 0x801975DC writes `state(+4)`: 1 if AppearType==5 (injured), 2 if E+0x356 (rescued),
else 0 — called every StatusUpdate (0x80198320) and from the dtor (0x80196FA0). On (re)spawn `vf24` reads it:
0 → AppearType 5 reset to 0; 1 → AppearType = 5 (spawns injured, HP 0); 2 → rescued (E+0x356 = 1 → Thanks).
If AppearType==5 after that: E+0x357 = had-shield, shield/weapon removed (0x801988A8..0x801988D0).

---------------------------------------------------------------------------------------------------------------------

## 5. Despawn / removal

### 5.1 `TEnemySetTask::Update` 0x801A6EB8 — PROVEN
```
if SB.vf00() (SB+0x14 delete request): Task flags |= 1                      0x801A6F04..0x801A6F18
alpha = E+0xEC
if slot (TEnemySetTask+0x2C = E+0x40C):
   if SetSlot_ShouldDespawn(slot): kill                                       0x801A6F38
   elif SetSlot_IsOutOfFadeRange(slot): alpha -= 2·dt; if alpha < 0: kill     0x801A6F58..0x801A6F80
   elif S.vf0E() (S+0x6D hide request): alpha = max(0, alpha − 2·dt)
   else alpha = min(1, alpha + 2·dt)
E.SetAlpha(alpha) (fn_8019AE90); SB.vf07(dt) lifecycle
```
`kill` = Task flag bit0 → `Task_UpdateChildren` deletes via slot 0 → thunk-992 0x80198D88 → `~GunSoldier`.

### 5.2 `TEnemySetBase` removal virtuals (SB = E+0x250) — PROVEN
| slot | addr | behaviour |
|---|---|---|
| 0 | 0x8017B8EC | return SB+0x14 (delete request) |
| 1 | 0x8017B8F4 | return SB+0x15 (SET already detached) |
| 2 | 0x8017B8FC | SB+0x14 = 1 (*RequestDelete*) |
| 3 | 0x801A7240 | SB+0x15 = 1; `fn_800C9EEC(slot)`: record flags |= 0x8, then fn_800C9F1C: clear 0x1, `SetSlot_Detach`, unlink from link list (fn_800CB91C→fn_800CADB0) — *killed* (not used for GunSoldier death) |
| 4 | 0x801A7200 | SB+0x15 = 1; `fn_800C9EBC(slot)`: flags |= 0x10000 (respawn-when-out-of-range), clear 0x1, detach, unlink — *despawn, respawnable*; used by GunSoldier for ToHinshi fall-out and VacuumCommand |

`SetSlot_Detach` fn_800C9FD0: clears flags 0x2 (alive) and 0x4; if flags&0x40000000 frees slot+0x14 temp data and the
misc block; slot+0xC = 0.

### 5.3 Destructor chain — PROVEN
`~GunSoldier` 0x80196F3C: restore vptrs, `GunSoldier::vf25` (write STemporaryData) @0x80196FA8, then
`~EnemyTemplate<GunSoldierBase,GunSoldierAI>` 0x80196FE0 → `TEnemySetTask::~TEnemySetTask` 0x801A7034 (if slot and
!SB.vf01 → `SetSlot_Detach(slot)`, slot = 0; then `Task::~Task`), `GunSoldierAI::~GunSoldierAI`,
`GunSoldierBase::~GunSoldierBase` (release shield 0x80198A48.., weapon E+0x350 vf(delete) 0x80198A9C,
`SoldierCommonBase` dtor), `operator delete`. `Enemy::~Enemy` also touches the EnemyManager (call to fn_80119560 at
0x8018ED8C; LIKELY unregister of the id stored at E+0x28 by `fn_801A2D6C` in `Enemy::Enemy` 0x8018F8DC — not verified).

### 5.4 SET record flag operations caused by a GunSoldier (record+0x18)

| When | Flags | Where | Conf |
|---|---|---|---|
| spawn (`SetSlot_TrySpawn`) | &= ~0x80041 | 0x800CA930 | PROVEN |
| TEnemySetTask ctor (`SetSlot_Attach`) | |= 0x2, &= ~0x4 | 0x801A7134 | PROVEN |
| **GunSoldier ctor** (`vf26` 0x801975B0 → `SetSlot_MarkDestroyed`) | |= 0x8 | 0x80197140 (only call site of slot 0x2A0, whole-DOL scan) | PROVEN |
| HP → 0 | none (type 0 skips SB.vf03) | 0x801A8C68 | PROVEN |
| first heal | |= 0x01000000 | 0x8019E5A4 | PROVEN |
| ToHinshi fell / Vacuum | |= 0x10000, &= ~0x1, &= ~0x6 | SB.vf04 | PROVEN |
| normal fade/out-of-range/ShouldDespawn removal | &= ~0x6 (dtor Detach) | 0x801A7090 | PROVEN |

LIKELY consequence (scanner not re-audited here): after a plain fade-out removal no re-arm flag (0x40/0x10000/0x800000)
is set, so `SetData_ScanSlots1P` will not respawn that record; `SB.vf04` removals get 0x40 when the player leaves the
range and are re-armed (0x800CAD40..0x800CAD68).

---------------------------------------------------------------------------------------------------------------------

## 6. Open questions / UNKNOWN
* Meaning of record flag 0x8 given it is set at construction (curated name `GunSoldier_OnDestroyedMarkSlot` for
  0x801975B0 is misleading: its only caller is the ctor).
* What the global byte 0x8057E828 represents (forces all enemy AIs to Idle; also read by player input).
* Exact damage values delivered by player attacks (hit+0x50) — not traced here.
* S+0x64 attachment (fn_8031AFB4 / fn_8031B020 / fn_8031B08C) — UNKNOWN (lock-on/marker candidate).
* MoveSpeedRatio (param 6) and per-point MoveSpeedRatio consumption — not found in GunSoldier code (UNKNOWN).
* `fn_801A978C` second CharaColli callback semantics (hit+0x30) — LIKELY contact/pre-hit.
* Consumer semantics of ScoreCommand kinds (gauge vs score) — LIKELY only.

## 7. Symbol suggestions (semantic, not original)
Superseded by the canonical list `gameplay/notes/enemy_symbols.csv` (RTTI virtuals renamed `Class::Method`, e.g.
`GunSoldier::GunSoldierBase::TakeHit`); the consolidated reference is `gameplay/enemy/GUN_SOLDIER.md`. Original list:
| Addr | Suggested name | Conf |
|---|---|---|
| 0x8019FD00 | EnemyAISM_ChangeState | PROVEN |
| 0x8019FC54 | EnemyAISM_RevertToPrevious | PROVEN |
| 0x8019FDB8 / 0x8019FDAC | EnemyAISM_Lock / Unlock | PROVEN |
| 0x801A6DB8 | EnemySensor_SetupFromSetParams | PROVEN |
| 0x801A6C7C | EnemySensor_IsInSearchArea | PROVEN |
| 0x801A6D00 | EnemySensor_IsInsideMoveRange | STRONG |
| 0x800CA118 | SetSlot_GetMiscParams | PROVEN |
| 0x80198D90 | SoldierCommon_ReadSetParams (shared with BkSoldier) | PROVEN |
| 0x801881A0 | GunSoldierAI_StartCombat | PROVEN |
| 0x80198184 | GunSoldierBase_BecomeInjured (vf14) | STRONG |
| 0x8019813C | GunSoldierBase_OnRevived (vf15) | STRONG |
| 0x80197700 | GunSoldierBase_TakeHit (vf12, shield logic) | PROVEN |
| 0x801A83B0 | EnemyStatusCommon_ApplyHit (vf1A) | PROVEN |
| 0x801A8718 | EnemyStatusCommon_ApplyDamage (vf1C) | PROVEN |
| 0x8017BC08 | EnemyStatusCommon_AddHP (vf2B) | PROVEN |
| 0x801A8998 | EnemyStatusCommon_ReportDeath (vf23) | PROVEN |
| 0x801A9750 | EnemyCharaColliModule_OnDamaged | STRONG |
| 0x801A59C8 | EnemyMoveCommon_Knockback (vf0B) | STRONG |
| 0x801975DC | GunSoldier_WriteTempState (vf25) | PROVEN |
| 0x801A2930 | EnemyManager_CountKill | PROVEN |
| 0x801A250C | EnemyManager_OnEnemyEvent | PROVEN |

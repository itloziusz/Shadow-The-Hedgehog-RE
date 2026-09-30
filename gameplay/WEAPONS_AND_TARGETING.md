# WEAPONS_AND_TARGETING — damage delivery, weapons, lock-on (second pass)

Source: `sys/main.dol` (GameCube, PAL GUPP8P). Class names are original (RTTI / MWCC typeinfo strings, including
`@unnamed@<TU>_cpp@::` names). All other names are recovered semantics. Confidence: **PROVEN** (direct code/data),
**STRONG** (several consistent pieces), **LIKELY** (single indirect piece), **UNKNOWN**.
Helpers written for this pass (read-only on the caches): `tools/agent_weapon_q.py` (grep / callargs / words / funcs /
accs / fk / dsearch), `tools/agent_weapon_catalog.py` (weapon catalog + registry, with static-initialiser overlay),
`tools/agent_weapon_vt.py` (weapon vtable fire-interval survey), `tools/agent_weapon_lock_q.py` (targeting),
`tools/agent_weapon_veh_*.py` (vehicles, see `VEHICLES.md`). Symbols: `notes/weapon_symbols.csv`.

Contents: §1 damage delivery (CharaColli attack/defense, `AttackCallbackParam`, `DamageCommand`) · §2 damage values
· §3 weapons (info block, registry, SET mapping, firing, ammo) · §4 lock-on / targeting · §5 address tables ·
§6 constants · §7 open items.

---------------------------------------------------------------------------------------------------------------------

## 0. Key results

| # | Result | Conf |
|---|---|---|
| 1 | Every hit is resolved by the CharaColli dispatcher `fn_800C6A4C`: it fires when **attacker.attackEnabled && target.damageable && attacker.level > target.defense**. The damage amount is the attacker's `CharaColliAttack+0x34` (f32 "power"), copied into `AttackCallbackParam+0x50`. | PROVEN |
| 2 | Levels: all player attacks (spin, dash, homing, Chaos Blast…) = **3**, bullets/explosions = **5**, enemy bodies = attack 1 / defense 1, player defense = **0**, many breakables defense 1, some defense **4** (only weapons break them). | PROVEN |
| 3 | Enemies subtract the power from HP directly (`EnemyStatusCommon::ApplyDamage`). The player ignores the power: its damage callback always sends `DamageCommand(power 10.0)` and Shadow's handler only uses the direction (ring loss is fixed at 10). | PROVEN |
| 4 | Damage values: homing attack 2, jump/spin 2, dash/slide 2, punch/kick combo 1, Chaos Blast 12 (radius grows 10→200 over 1.5 s), Chaos Control 2, Chaos Spear 6–20 (charge). Guns 2–6 (Shadow Rifle 32), cannons 4–16 + explosion 4–14, missiles 8/16/4, lasers 3–10, melee 1–8. | PROVEN |
| 5 | The SET param `weapon` (SET_WEAPON 0x0020, WEAPON_CONTAINER 0x000C, SECRET CONTAINER 0x003A, STICK 0x012C; range 1..71) **is** the internal weapon index = row of the weapon registry `0x8051F058` (72 × 16 B). Full table in §3.3. | PROVEN (SET_WEAPON), LIKELY (others) |
| 6 | Weapon-info block (descriptor+0x24): +0x18 **default ammo per pickup**, +0x1C **hold/animation type**, +0x20 **pickup sound id** (0x2001–0x2004 — this is the family grouping the first pass called "category"), +0x24 **hold angle** (×90°, arm IK), +0x28 **recoil**, +0x2C **fire slowdown**. The first pass's "+0x48" is info+0x24. | PROVEN (consumers) |
| 7 | Fire rate = value returned by the weapon's `Fire` virtual (slot 0x0A/0x0B): HandGun/EggGun −1 (semi-automatic), SMG/Vulcan/FlashShot 1/12 s, AutoRifle 0.1 s, AA rifle 1/6 s, HeavyShot 0.125 s, LightShot 0.5 s, RingShot 1 s, cannons 0.5 s, missiles 0.25 s. | PROVEN |
| 8 | Ammo lives in the player's game-state record (+0x1C, clamped 0..999); decremented by shots fired this frame; empty → weapon discarded; player flag 0x3F = infinite ammo, 0x33 = weapon cannot be dropped. | PROVEN |
| 9 | Weapon levels exist only for the unlockable special weapons: Katana, Satellite Laser, Vacuum Egg, Omochao Gun, Heal Cannon (Lv1/Lv2 = registry 57/58 … 65/66, chosen from save-data bits); Shadow Rifle (67) has one level. | PROVEN |
| 10 | Guns auto-aim at `PlayerTarget`'s aim target (mask 0x800, 45° cone, range = bullet range); lock-on weapons lock one target per 0.1 s inside a 20° / 300-unit muzzle cone, up to min(max locks, ammo); homing attack picks the nearest mask-0x400 target within 150 that is **below** the player. | PROVEN |

---------------------------------------------------------------------------------------------------------------------

## 1. Damage delivery

### 1.1 `SonicteamUSA::System::CharaColli::CharaColliAttack` (0x58 bytes) — PROVEN
Ctor `0x8005A968` (the `sp_counted_base_impl<CharaColliAttack*>` helpers are adjacent at 0x8005A934/0x8005B7B8).
Created per collision by `CharaColliAttack_CreateForColli 0x800C475C` and reached from the collision object through
the weak pointer at `CharaColliBaseCharInfo+0x38`.

| Off | Field | Default | Writer / reader | Conf |
|---|---|---|---|---|
| +0x00 | smart-ptr header | – | `fn_8001FC78` in ctor | PROVEN |
| +0x08 | owner (weak ref to CharInfo) | – | `fn_80052554(attack+8, owner)` in bullet/player setup; read by receivers (`fn_8005223C(param+0 → +8)`) | PROVEN |
| +0x10 | callback list "on attack" (attacker side, called first) | empty | `SetAttack` binds r6 | PROVEN |
| +0x1C | callback list "on damaged" (target side) | empty | `SetDefense` binds r5; EnemyCharaColliModule binds `0x801A9750` | PROVEN |
| +0x28 | callback list "after attack" (attacker side, reads result) | empty | `SetAttack` binds r7 | PROVEN |
| +0x34 | f32 **power** (damage) | 0.0 | `SetAttack` f1 | PROVEN |
| +0x38 | s32 **attack level** | 3 | `SetAttack` r4 | PROVEN |
| +0x3C | s32 **defense level** | 0 | `SetDefense` r4 | PROVEN |
| +0x40 | u32 **attack attribute bits** (§1.5) | 0 | `SetAttack` r5; tested by receivers (e.g. GunSoldier shield) | PROVEN |
| +0x44 | ignore list (weak refs) | empty | `fn_8005AFD4` iterates it before a pair is accepted | LIKELY |
| +0x54 | u8 attack enabled | 0 | `SetAttack` sets 1 | PROVEN |
| +0x55 | u8 damageable | 0 | `SetDefense` sets 1 | PROVEN |

`CharaColliAttack_SetAttack(attack, level r4, attr r5, onAttackCb r6, afterCb r7, power f1)` **0x80054EF8** and
`CharaColliAttack_SetDefense(attack, defense r4, onDamagedCb r5)` **0x80096E54** — both PROVEN by disassembly.

### 1.2 Hit rule — `CharaColli_DispatchAttackPair` 0x800C6A4C — PROVEN
For a colliding pair (A, B) it runs the test twice (A→B at 0x800C6BC8, B→A at 0x800C6D0C):
```text
if A.attack+0x54 && B.attack+0x55                          ; enabled / damageable
   && !CharaColliAttack_IsIgnored(A, B)  (fn_8005AFD4)
   && A.list+0x10 non-empty && B.list+0x1C non-empty       ; fn_800C4414
   && A.level(+0x38) > B.defense(+0x3C)                    ; 0x800C6C30  cmpw ; ble skip   (strict >)
      param = AttackCallbackParam()                        ; ctor 0x800C69E8
      param.power(+0x50) = A.attack+0x34                   ; 0x800C6C40/0x800C6C4C
      param+0x00 = A attack ref, +0x08 = B attack ref, +0x10/+0x14 = shape ids, +0x18 = contact point,
      param+0x24 = contact normal, +0x30 = 0 (stop), +0x58 = contact record
      invoke A.list+0x10 (attacker "on attack": fills direction etc.)
      if !param+0x30: invoke B.list+0x1C (target "on damaged": applies damage, writes result +0x54)
      if !param+0x30: invoke A.list+0x28 (attacker "after": reads result, e.g. bullet dies)
```
A one-sided variant with the same rule is `fn_800C6608` (called from the shape tests at 0x800C44AC..0x800C615C) — LIKELY.

### 1.3 `SonicteamUSA::System::CharaColli::AttackCallbackParam` (≥0x5C bytes, stack object)

| Off | Type | Meaning | Evidence | Conf |
|---|---|---|---|---|
| +0x00 | shared_ptr | attacker `CharaColliAttack` (→ +0x08 owner = attacker object) | `fn_800C694C(param, A)`; readers `fn_80053540(out, param)` in ApplyHit 0x801A83E0 and player callback 0x80095F30 | PROVEN |
| +0x08 | shared_ptr | target `CharaColliAttack` | second `fn_800C694C` 0x800C6C5C | STRONG |
| +0x10 / +0x14 | u32 | shape/contact ids of the two colliders (`[r26+8]`, `[r27+8]`) | 0x800C6C68, 0x800C6C94 | LIKELY |
| +0x18 | Vec3 | contact point (`fn_800C6FE0`, default 0) | ApplyHit uses it as hit-effect position (0x801A84D8) | STRONG |
| +0x24 | Vec3 | contact normal (`fn_800C6F6C`, default (0,1,0)) | 0x800C6CB4 | STRONG |
| +0x30 | u8 | stop further callbacks | cleared 0x800C6CD4, tested 0x800C6CDC/0x800C6CF4 | PROVEN |
| +0x34 | Vec3 | **attack direction**, written by the attacker's on-attack callback (BulletGun: normalized velocity, `fn_80055EB4`) | read by enemy ApplyHit (facing/side 0x801A84FC) and player → `DamageCommand+0x18` | PROVEN |
| +0x40..+0x48 | f32 | zeroed by ctor; no writer/reader found | ctor | UNKNOWN |
| +0x4C | f32 | attacker-supplied scalar (BulletGun writes its muzzle speed 500) | `fn_80055EB4` 0x80055EE8 | PROVEN write / consumer UNKNOWN |
| +0x50 | f32 | **power** | 0x800C6C4C; read by ApplyHit 0x801A8428 and player 0x80096144 | PROVEN |
| +0x54 | s32 | **result**, written by the damaged callback (table below) | enemy 0x801A9760/0x801A83D0, player 0x80095FBC.. | PROVEN |
| +0x58 | ptr | contact record (`+0x34` u8 "consumed", `+0x38` result copy) | BulletGun after-callback 0x80055E64 | PROVEN |

Result codes (+0x54) as written by the known receivers and read by the BulletGun after-callback (jump table 0x8051ED88):

| Code | Written by | BulletGun reaction | Meaning | Conf |
|---|---|---|---|---|
| 0 | ctor | passes through | no receiver answered | PROVEN |
| 1 | `EnemyStatusCommon::ApplyHit`, player callback | consumed | damage applied | PROVEN |
| 3 | vehicle damage callbacks when the hit destroys the vehicle (BatteryArmy 0x800E9BEC / BlackKiller 0x800EBCC0, see VEHICLES.md §6.4) | consumed | target destroyed | STRONG |
| 5 | enemy `OnDamaged` 0x801A9760 before TakeHit; GunSoldier shield block 0x8019778C; player when `fn_8009672C()==2` and attack level 3 | consumed | hit absorbed without damage | PROVEN (writes) |
| 8 | player callback (same team / invulnerable proxy / explosion-proof) | passes through | ignored | PROVEN |

### 1.4 Receivers

**Enemies** (PROVEN, see `enemy/GUN_SOLDIER.md` §6): `EnemyCharaColliModule::OnDamaged 0x801A9750` → Status slot 0x1A
(GunSoldier: `TakeHit 0x80197700`) → `EnemyStatusCommon::ApplyHit 0x801A83B0` → vslot 0x78 = `ApplyDamage 0x801A8718`:
`if power > 0: HP(+0x7C) -= power; HP ≤ 0 → 0, return 1`. No scaling. The GunSoldier shield reads the attacker's
attribute bits through `param+0` (`fn_801978F4` → `[attack]+0x40`): bit 0x40 (projectile) from the front (<90°) is
blocked (result 5); bit 0x4 bypasses the shield.

**Player** — `PlayerCharaColli_OnDamaged 0x80095F14` (bound `{0,-1,0x80095F14}` at .data 0x80521D64 by
`SetDefense(defense 0)` in `fn_80096C44`) — PROVEN:
1. result 8 (ignore) if the attacker owner is the player itself or one of its proxies (`fn_8009622C`, `fn_800961B4`
   on player+0x254), or if the attacker CharInfo+0x5C kind is 2, or kinds match the friendly-fire table
   (victim 3 ⟂ attacker 3, victim 2 ⟂ attacker 1) — kind meanings other than 1 = `Player::Player` UNKNOWN.
2. result 5 if `fn_8009672C(module) == 2` and attacker level == 3 (state meaning UNKNOWN).
3. character 0xD → `Score_Award(attacker, 0x70)`.
4. result 8 if player flag 0x21 is set and the attack has attribute 0x200 (explosion).
5. if `param.power > 0.0`: `DamageCommand(dir = param+0x34, priority 4, power = 10.0 constant 0x805F3A94)` sent to the
   player (vslot 4 = enqueue); result 1.

### 1.5 Attack attribute bits (`CharaColliAttack+0x40`)
Bits PROVEN from the `SetAttack` call sites; the role names are inferred from which attacks set them (STRONG for the
grouping, LIKELY for the label).

| Bit | Set by | Label |
|---|---|---|
| 0x001 | player combo attacks (types 0–3, 8, 9); enemy body contact (0x801AA3CC) | body / strike |
| 0x002 | player type 4 (jump, fall, homing-jump, launch) | spin |
| 0x004 | player types 5/6 (jump-dash, light-dash, spin-dash, sliding) — bypasses GunSoldier shield | dash |
| 0x008 | player type 7 (homing attack) | homing |
| 0x010 | player type 0xA (Chaos Blast) | chaos blast |
| 0x040 | `Bullet_CreateCharaColliAttack` 0x8005A428 (BulletGun, Cannon shell, Homing, Laser, ChaosSpear, ThrowObject), Ring 0x80059BD4 — frontally blocked by shields | projectile |
| 0x080 | stick/melee weapons (`fn_80068060` 0x800681B4) | melee weapon |
| 0x200 | `CannonBomb` explosion (0x80054D10), player type 0xB (Chaos Control) | explosion / blast |
| 0x400 | Torch fire (`fn_8023E7D4`) | fire |
| 0x800 | OR-ed into Homing missiles (0x800581E0) | missile |

### 1.6 Attack / defense levels (PROVEN values at the cited writers)

| Object | Attack level | Defense | Writer |
|---|---:|---:|---|
| Player body | – (attack disabled) | **0** | `fn_80096C44` 0x80096E18 |
| Player attacks (all types) | **3** | – | `PlayerAttack_ApplyType` 0x80078320 |
| Bullets, missiles, lasers, cannon shells, explosions, Ring, Chaos Spear | **5** | – | 0x8005A4BC, 0x80054D0C, 0x80059BD4 |
| Stick weapons held by the player / by others | 3 / 4 | – | `fn_80068978` 0x80068A04 / 0x80068A18 |
| Torch fire | 5 | – | 0x8023E9E8 |
| Enemy body (`EnemyCharaColliModule::Setup`) | 1 (power 1.0) | 1 | 0x801AA3A8..0x801AA3DC |
| Most gadgets/breakables (`SetDefense` callers) | – | 1 | 44 call sites (e.g. `EggCannonBullet`, `ElecPanel`, `BreakRoad`, `GunLift::vf0C`) |
| Weapon-only breakables | – | **4** | `fn_800D6C80` 0x800D6E7C, `fn_8021F7A4` 0x8021FB7C |

Consequences (from the strict `>`): enemy bodies (1) cannot hurt each other (1 > 1 false); enemy bodies hurt the
player (1 > 0); player attacks (3) hurt enemies and defense-1 gadgets but not defense-4 objects, which need a weapon
(5). Enemy projectiles (5) pass the level test against other enemies; whether receivers then filter by team is
UNKNOWN.

### 1.7 `DamageCommand` (CharCommand type 0x007) — PROVEN
Two ctors, both allocated with `CharCommand_Alloc(0x24)`: `DamageCommand::DamageCommand 0x801DBBA4(this, const Vec3*
dir r4, priority r5, power f1)` and `0x801DBC04(this, priority r4, power f1)` (direction = zero vector bss 0x805E1860).

| Off | Field | Evidence |
|---|---|---|
| +0x00 | vptr `DamageCommand` (0x8053E554) | 0x801DBBDC |
| +0x04 | u16 type = 7; +0x08 priority bucket = r5 (all senders pass 4) | `CharCommand::CharCommand` |
| +0x14 | f32 power | 0x801DBBE0 |
| +0x18 | Vec3 direction/source | `Vec3_Copy2` 0x801DBBE4 |

| Sender | Power | Direction | Conf |
|---|---:|---|---|
| `PlayerCharaColli_OnDamaged` 0x80096170 | 10.0 | `AttackCallbackParam+0x34` | PROVEN |
| `Weapon::VacuumPod::vf02` 0x802266F4 | 1.0 | normalized vector | PROVEN |
| `fn_80277D6C` 0x80277F98 (special-weapon TU, LIKELY Vacuum Egg) | 1.0 | normalized vector | PROVEN |
| `fn_80317744` 0x803177E4 | 1.0 | (0,1,0) | PROVEN |
| `PlayerController_CheckDamageContact 0x8009F29C` (terrain contact bit A4.1) → ctor 2 at 0x8009F300 | 10.0 (0x805F3B80) | zero | PROVEN |
| `Player::Behavior::Grind::Update` 0x80081BB0 → ctor 2 | 1.0 (0x805F36C0) | zero | PROVEN |
| `fn_8012C928` 0x8012C9EC (gadget) → ctor 2 | 1.0 (0x805F51C8) | zero | PROVEN |

Receiver: `ShadowExecuter_OnCommand 0x800AF924` (the only `__dynamic_cast` to the `DamageCommand` typeinfo 0x805E5B90):
ignored with flags 0x17/0x18 or while already in Damage (0x1E); `vslot 0x12 ShouldDieOnHit` → Dead (0x1F); else
`InitializeArgsDamage(cmd+0x18)` → Damage (0x1E). **+0x14 (power) is never read** by it — PROVEN; no other reader found.
Enemies do not handle type 7 (GUN_SOLDIER.md §6.8).

---------------------------------------------------------------------------------------------------------------------

## 2. Damage values

Format: `| value | address | consumer pc | meaning | confidence |`. "Power" = amount subtracted from enemy HP.

### 2.1 Player body attacks — `Player::Attack` (player+0x234) type table 0x8052124C (stride 0x14)
Row = {+0 attach id, +4 f32 radius (written by static init 0x80078C2C..0x80078C58), +8 attribute, +0xC f32 power,
+0x10 attach mode}. `PlayerAttack_ApplyType 0x80078098` → `SetAttack(level 3, row+8, power row+0xC)` at 0x80078324.

| Type | Power | Radius | Attr | Used by (caller → `Player::Attack::SetAttackType 0x80077A84`) | Conf |
|---:|---:|---:|---|---|---|
| 0–3 | 1 | 5 | 0x1 | combo strikes: `Player::Attack::SetComboAttack 0x80077AA8` event 0/1/2/3 (from motion-event handler `fn_800AA4BC`) | PROVEN values / STRONG users |
| 4 | **2** | 7 | 0x2 | Jump::Enter 0x80088038, Fall::Enter 0x80080F88.., HomingJump 0x80086C2C, Launch 0x8008971C; combo event 4 | PROVEN |
| 5 | **2** | 7 | 0x4 | JumpDash 0x80088AFC, LightDash 0x8008A560, SpinDash launch 0x8008EF9C | PROVEN |
| 6 | **2** | 8 | 0x4 | Sliding 0x8008DBE4 | PROVEN |
| 7 | **2** | 8 | 0x8 | **HomingAttack::Enter 0x80086154** | PROVEN |
| 8 / 9 | 1 / 1 | 15 / 8 | 0x1 | combo events 5 / 6 | PROVEN values |
| 10 (0xA) | **12** | 8 → grows | 0x10 | **ChaosBlast::Update 0x8007AEE0** | PROVEN |
| 11 (0xB) | 2 | 100 | 0x200 | ChaosControl::Update 0x8007BC80 | PROVEN |

| value | address | consumer pc | meaning | confidence |
|---|---|---|---|---|
| 2.0 | .data 0x805212E4 (type 7 row 0x805212D8 +0xC) | 0x80078314 | homing-attack power | PROVEN |
| 2.0 | .data 0x805212A8 (type 4 row 0x8052129C +0xC) | 0x80078314 | jump/spin power | PROVEN |
| 12.0 | 0x80521320 (type 10) | 0x80078314 | Chaos Blast power | PROVEN |
| 10 → 200 | sbss 0x805EF42C / 0x805EF430 (init 0x8007BA84/88) | 0x8007AFA0 | Chaos Blast radius r = 10 + 190·(t/1.5)^0.75 | PROVEN |
| 1.5 s | 0x805F3594 | 0x8007AF6C | Chaos Blast growth time | PROVEN |
| −10000 | li 0x8007AF14 | `KarmaGauge_Add(gs+0x70C, 1, −10000)` | Chaos Blast empties gauge 1 | PROVEN (which gauge = UNKNOWN) |

### 2.2 Chaos Spear (Super Shadow) — PROVEN
`Weapon::ChaosSpear::ChaosSpear` (a `WeaponGunBase`) is created only by `Weapon_Create_ChaosSpear 0x802F0CB8`, called
from `SuperShadow_Setup 0x8027F3B8` (via `Player::Shadow::Super::Super`), which equips it with 999 ammo and sets
player flags 0x24 (ring drain), 0x25 (Super, only when stage id 0x2C6), 0x33 (weapon locked).
- `vf02` 0x802F1504: while charging, t(+0x7C) += dt, clamped to 3.0 s (0x805FA43C); at t ≥ 2.4 s (0x805FA440) a
  one-time "charged" cue (`fn_801D5540(SoundManager_Get(), 0xF, −1, −1)`, sound call LIKELY).
- `vf0C` 0x802F1194 (trigger release): charge c = t / 3.0 (0x805FA430) → `BulletCreate_ChaosSpear 0x802F2128`.
- Bullet ctor 0x802F29A8: c ≥ 0.9995 (0x805FA484) → c = 1 and full-charge flag; **power = 6 + 14·c** (0x805FA488,
  0x805FA48C, `fmadds` 0x802F2A64) → 6..20; passed to `Bullet_CreateCharaColliAttack` 0x802F2B60 (level 5, attr 0x40).

### 2.3 Bullets per class

**`Weapon::Bullet::BulletGun`** (0x60 B; ctor 0x800566C4(this, shot, effectId r5, type r6, power f1, range f2,
speed f3)). Power → `Bullet_CreateCharaColliAttack` 0x80056850 (level 5, attr 0x40). Range (+0x40) = max travel
distance: `BulletGun_UpdateFlight 0x80056170` deletes the bullet when accumulated distance +0x38 ≥ +0x40.
Speed (+0x44) = 500 for all (sbss 0x805EF2F4, init 0x80056C2C) plus the shooter's forward speed (`fn_8005A350`).

| Bullet-type entry | Factory (power load pc) | Power | Range (+0x10) | Type | Weapon (registry idx) | Conf |
|---|---|---:|---:|---:|---|---|
| 0x8051EC54 | 0x80055C54 (0x80055C7C) | 2 | 250 | 0 | HandGun (1) | PROVEN |
| 0x8051EC68 | 0x80055BF8 (0x80055C20) | 2 | 250 | 0 | SubMachineGun (2) | PROVEN |
| 0x8051EC7C | 0x80055B9C (0x80055BC4) | 4 | 400 | 0 | AutoRifle (3) | PROVEN |
| 0x8051EC90 | 0x80055B40 (0x80055B68) | 6 | 600 | 0 | AntiAircraftRifle (4) | PROVEN |
| 0x8051ECA4 | **0x80055B40** (same factory) | 6 | 600 | 0 | Vulcan (5) | PROVEN |
| 0x8051ECB8 | 0x80055AE4 (0x80055B0C) | 2 | 250 | 0 | EggGun (7) | PROVEN |
| 0x8051ECCC | 0x80055A88 (0x80055AB0) | 2 | 250 | 1 | LightShot (8) | PROVEN |
| 0x8051ECE0 | 0x80055A2C (0x80055A54) | 2 | 250 | 1 | FlashShot (9) | PROVEN |
| 0x8051ECF4 | 0x800559D0 (0x800559F8) | 5 | 400 | 1 | HeavyShot (11) | PROVEN |
| 0x8051ED08 | 0x80055974 (0x8005599C) | 6 | 2000 | 2 | GunBattery (28, vehicle) | PROVEN |
| 0x8051ED1C | 0x80055918 (0x80055940) | 8 | 2000 | 3 | BKBattery (29, vehicle); impact spawns CannonBomb type 6 | PROVEN |
| 0x8051ED30 | 0x800558BC (0x800558E4) | 4 | 100 | 5 | Katana01 (57) | PROVEN data / firing path UNKNOWN |
| 0x8051ED44 | 0x80055860 (0x80055888) | 8 | 200 | 6 | Katana02 (58) | PROVEN data / firing path UNKNOWN |
| 0x8051ED58 | 0x80055804 (0x8005582C) | **32** | 600 | 7 | ShadowRifle (67) | PROVEN |

**`Weapon::Bullet::Cannon`** (shell; ctor 0x80053FC0(this, shot, type r5)) — per-type table 0x8051E9A0 (stride 0xC:
power, f32 +4 → shell+0x3C, effect id). Power → 0x8005414C. On hit/expiry `Cannon_Explode 0x80053138` spawns
`CannonBomb(type)` (types 7/8 instead call HealingBlast `fn_80107448(level 0/1)`).
**`Weapon::Bullet::CannonBomb`** (explosion; ctor 0x80055360) — table 0x8051EB48 (stride 0x14: power, radius
(static init 0x80055564), SE id, effect id, 0.5). `CannonBomb_CreateAttack 0x80054B04`: level 5, attr 0x200.

| Type | Weapon (idx) | Shell power | Shell +4 (speed, LIKELY) | Explosion power | Explosion radius | Conf |
|---:|---|---:|---:|---:|---:|---|
| 0 | Grenade (12) | 4 | 200 | 4 | 30 | PROVEN |
| 1 | Bazooka (13) | 8 | 300 | 8 | 50 | PROVEN |
| 2 | TankCannon (14) | 16 | 300 | 14 | 100 | PROVEN |
| 3 | BlackBarrel (15) | 4 | 200 | 4 | 40 | PROVEN |
| 4 | BigBarrel (16) | 8 | 200 | 8 | 70 | PROVEN |
| 5 | EggBazooka (17) | 8 | 300 | 8 | 50 | PROVEN |
| 6 | (no cannon factory; BKBattery bullet impact uses bomb type 6) | 99 | 9990 | 4 | 40 | PROVEN data |
| 7 / 8 | HealCannon01/02 (65/66) | 0 | 300 | 0 (heals instead) | – | PROVEN |

**`Weapon::Bullet::Homing`** (missile; ctor 0x80057E24(this, shot, kind r5, table-row r6)) — per-kind table
.rodata 0x804AD4C0 (stride 0x1C; +0x10 f32 power → `Bullet_CreateCharaColliAttack` 0x80058190; attr |= 0x800).
Steering/speed: §4.4.

| Kind | Weapon (idx) | Power | Conf |
|---:|---|---:|---|
| 0 | AntiTankMissile (18) | 8 | PROVEN |
| 1 | RocketLauncher4/8 (19/20) | 8 | PROVEN |
| 2 | WormShooter / WideWormShooter (21/22) | 8 | PROVEN |
| 3 | BigWormShooter (23) | 16 | PROVEN |
| 4 | AirBolt (vehicle, bullet-type 0x8051EEAC) | 4 | PROVEN |
| 5 / 6 / 7 | other users (0x8051EEC0 / 0x8051EED4 / 0x8051EEFC; vehicles/enemies) | 4 / 4 / 4 | PROVEN data |

**`Weapon::Bullet::Ring`** (RingShot, idx 10): `BulletCreate_Ring_RingShot 0x80059510` → `Ring::Ring(power 4.0
0x805F2EB0, 400.0, effect 0x123)`; `Ring_CreateAttack 0x80059984` level 5 attr 0x40; range 400 (0x8051EFE0, init
0x8005A348). PROVEN.

**`@unnamed@BulletLaser_cpp@::Laser`** (ctor 0x802213A0(this, shot, type r5)) — table 0x8054707C (stride 0x1C: +0 power,
+4 speed and +8 range written by static init 0x802217E0, +0xC count, effect ids). Power load `lfsx` 0x80220CCC →
`Bullet_CreateCharaColliAttack` 0x80220CDC.

| Type | Weapon (idx) | Power | Speed | Range | +0xC (reflect/split count, LIKELY) | Conf |
|---:|---|---:|---:|---:|---:|---|
| 0 | LaserRifle (25) | 3 | 1000 | 300 | 0 | PROVEN |
| 1 | Splitter (26) | 4 | 500 | 400 | 0 | PROVEN |
| 2 | Reflector (27) | 5 | 1000 | 800 | 2 | PROVEN |
| 3 | OmochaoGun01 (63) | 6 | 300 | 800 | 10 | PROVEN |
| 4 | OmochaoGun02 (64) | 10 | 300 | 800 | 10 | PROVEN |

**`Weapon::Bullet::ChaosSpear::ChaosSpear`**: 6..20 (§2.2). **`ThrowObject`** (objects launched by the vacuum
weapons, base of `ThrowContainer::Container`, `ThrowVacuumBomb::Bomb`): power 6.0 (0x805F91C8 → 0x8027DA84) — PROVEN
value, launcher link LIKELY. `BulletSatelliteLaser::SatelliteLaser` damage: UNKNOWN (not traced).

### 2.4 Melee (`Weapon::WeaponStickBase`, kind 5)
Ctor 0x80067BB4(this, owner, index r5, power f1) → stick attack component (`StickAttack_Construct 0x80068978`,
+0x48 = power, level 3 if owner is `Player::Player` (+0x5C == 1) else 4) → `StickAttack_CreateAttack 0x80068060`:
`SetAttack(level, 0x80, power)` at 0x800681B8. PROVEN.

| Weapon (idx) | Power | Power load pc | Conf |
|---|---:|---|---|
| SurvivalKnife (30), EggSpear (33), all Stick01xx–0700 (34–56 except 38) | 2 | e.g. 0x80067304 (0x805F3298) | PROVEN |
| BlackSword (31) | 4 | 0x8005DFE8 | PROVEN |
| DarkHammer (32) | 6 | 0x802F1C3C | PROVEN |
| Torch0300 (38) | 1 (+ fire attack power 0, attr 0x400) | 0x8023E0D4, 0x8023E9E0 | PROVEN |
| Katana01 / Katana02 (57 / 58) | 4 / 8 | 0x80276078 / 0x80275F84 | PROVEN |

### 2.5 Hits to defeat (derived from §2 and ENEMY_BEHAVIOR_INDEX §2.1 max HP; assumes the common
`EnemyStatusCommon::ApplyHit` path) — LIKELY
GUN Soldier (2 HP): 1 homing attack / 1 HandGun bullet. Black Arms soldier (4): 2 homing attacks / 1 AutoRifle
bullet. GUN Robot (8): 1 Bazooka direct hit (8) or its explosion. BkGiant (24): TankCannon shell + explosion = 30.
Chaos Blast (12, radius up to 200) downs every enemy type with ≤ 12 HP in one blast.

---------------------------------------------------------------------------------------------------------------------

## 3. Weapons

### 3.1 Catalog descriptor and weapon-info block
Weapons are SET-catalog descriptors (`b1A = 4`) between `WeaponDataStart` (0x2584) / `WeaponDataEnd` (0x2585). The
**info block** is at `descriptor + 0x24`; the weapon registry (§3.2) points at it. Static C++ initialisers (e.g.
0x8005D504 for Bazooka) write the name and two floats at startup, so the DOL image shows zeros there
(`agent_weapon_catalog.py` overlays them).

| Info off (desc off) | Field | Consumer (accessor → user) | Conf |
|---|---|---|---|
| +0x00 (+0x24) | `const char*` name ("HandGun" …; also copied to desc+0x00) | static init; `WeaponRegistry_GetName 0x8005C014` ← `SetWeaponTask::vf04` 0x8011F174 (re-creates the pickup when the param changes and the name is non-empty) and the label callback `fn_8011EF70` (returns "Not Used" otherwise; editor display LIKELY) | PROVEN |
| +0x04 (+0x28) | `WeaponBase* create(owner)` | `fn_8005BD1C` / `ShadowWeapon_CreateAndEquip 0x8023A5CC` | PROVEN |
| +0x08 (+0x2C) | `Weapon::Item* createItem()` (pickup) | `WeaponRegistry_CreateItem 0x8005BAFC` | PROVEN |
| +0x0C (+0x30) | optional extra factory (AA rifle 0x8005C20C, RocketLauncher4 0x80064DC4, worm shooters) | `WeaponRegistry_CreateExtra 0x8005B974` → used by `HeavyDogCtrlUnit_RocketLauncher`, `fn_800E9348`, `fn_800EB444`, `fn_80348BE0` | PROVEN (callers) / role LIKELY (non-player users) |
| +0x10 / +0x14 (+0x34/+0x38) | f32 ×2 = 1.0 / 5.0 for every weapon (static init) | `WeaponInfo_GetPickupColliSize 0x8005B9C0` → `SetWeapon::Arms` ctor 0x8011FFA8: pickup collision size (vslot 0x10(−2, f1, _, f3)), offset (0, 5, 0) | PROVEN use / LIKELY radius/height |
| +0x18 (+0x3C) | s32 **default ammo per pickup** | `WeaponInfo_GetDefaultAmmo 0x8005BAD4` ← `ShadowWeapon::AddWeapon` 0x8023B68C / `Equip` 0x8023A964 when ammo < 0 | PROVEN |
| +0x1C (+0x40) | s32 **hold/animation type** 1 one-hand, 2 knife, 3 two-hand/heavy | `WeaponInfo_GetHoldType 0x8005BAAC` ← `ShadowWeapon_Equip` 0x8023AAB4 → ShadowMotion vslot 4 | PROVEN use / LIKELY labels |
| +0x20 (+0x44) | u32, low 16 bits = **pickup sound id** 0x2001 melee / 0x2002 GUN / 0x2003 Black Arms / 0x2004 Eggman | `WeaponInfo_GetPickupSE 0x8005BA7C` ← `ShadowWeapon::AddWeapon` 0x8023B640 → `SoundManager_PlaySE 0x801D59AC` (0xFFFF = none) | PROVEN |
| +0x24 (+0x48) | f32 **hold angle factor** (×90° → ShadowWeapon+0x58, arm-IK aim offset) | `WeaponInfo_GetHoldAngle 0x8005BA54` ← 0x8023AB9C; read by `ShadowWeapon_ComputeArmIKTarget 0x8023A3C4` | PROVEN use / STRONG label |
| +0x28 (+0x4C) | f32 **recoil**: added (×direction, twice) to ShadowWeapon+0x40 on every frame a shot is fired; +0x40 decays ×0.859375/frame and offsets the arm IK target | `WeaponBase::vf07 0x8005CFFC` / `WeaponInfo_GetRecoil 0x8005BA2C` ← `ShadowWeapon::Update` 0x8023B7B8/0x8023B7E8, `fn_800B3E94` | PROVEN use / STRONG label |
| +0x2C (+0x50) | f32 **fire slowdown**: on frames with a shot, Ground velocity ×(1−w) and accel magnitude ×(1−w) | `WeaponBase::vf08 0x8005CFD8` / `WeaponInfo_GetFireSlowdown 0x8005BA04` ← `Ground::Update` 0x80083AEC | PROVEN |

The first pass's "+0x3C/+0x40/+0x44/+0x48" (descriptor-relative) are info +0x18/+0x1C/+0x20/+0x24.

### 3.2 Weapon registry 0x8051F058 (72 × 16 B) — PROVEN
Row `w` (= internal weapon index = SET `weapon` param): `+0 info*`, `+4 u8 loaded` (runtime; set by
`WeaponRegistry_MarkLoaded 0x8005B888(w)` from each weapon's resource hook, cleared by `0x8005B8A4` at
WeaponDataEnd release), `+8 s32 fire rumble pattern` (`WeaponRegistry_GetFireRumble 0x8005B95C` → `fn_8034ED50(pattern,
padIndex)` in ShadowWeapon::Update 0x8023B83C when a shot is fired; −1 = none; the same function is used with 4 on
damage and 6 on AddWeapon), `+0xC u8` (1 for rows 1–27 and 59–67, 0 for melee 30–58 and the vehicle guns 28/29;
single reader `fn_801C0A1C` in the vehicle-data region — meaning UNKNOWN).
`WeaponRegistry_CreateItem 0x8005BAFC(out, w, ammo)` returns an empty pointer unless row `w` is loaded, i.e. the
weapon's SET descriptor was enabled by the stage's setid.bin; special weapons 57–67 are always loaded by
`SpecialWeapons_LoadAll 0x80275EEC` (from the WeaponDataStart hook 0x8005E428).

**Other index-keyed tables**: bullet types `0x8051E840[w]` (§3.4) and the registry — the SET id is *not* a formula of
the index (0xC7+w for 1–11, 0xD0+w for 12–17, 0xD4+w for 18–23, 0xD8+w for 24–27, 0xDC+w for 30–56).

### 3.3 SET `weapon` param → weapon (full table)
`fn_8011F0C8` stores the SET param at `SetWeaponTask+0x44` and `WeaponRegistry_CreateItem(out, [+0x44], −1)` is called
directly with it (0x8011F468) — PROVEN for SET_WEAPON. WEAPON_CONTAINER, SECRET CONTAINER and STICK declare the
same `weapon` schema (Sint32 1..71); their spawn goes through `WeaponPickup_SpawnAtGround 0x8011ED20(w, pos)` /
`ScatteringWeapon_Spawn` (LIKELY same index).

Columns: idx · SET id · name (game string) · class · kind (§3.5) · ammo (info+0x18) · hold (+0x1C) · SE (+0x20) ·
angle (+0x24) · recoil (+0x28) · slowdown (+0x2C) · rumble (registry+8) · fire interval (§3.6) · projectile (§2.3).

| idx | SET | Name | Class | kind | ammo | hold | SE | angle | recoil | slow | rumble | interval (s) | projectile / power |
|---:|---|---|---|---:|---:|---:|---|---:|---:|---:|---:|---|---|
| 0 | – | (none) | – | | | | | | | | −1 | | |
| 1 | 00C8 | HandGun | `Weapon::HandGun` | 1 | 10 | 1 | 2002 | 0 | 0.2 | 0 | 0 | semi (−1) | BulletGun 2 |
| 2 | 00C9 | SubMachineGun | `Weapon::SubMachineGun` | 1 | 20 | 1 | 2002 | 0.4 | 0.4 | 0 | 1 | 1/12 | BulletGun 2 |
| 3 | 00CA | AutoRifle | `Weapon::AutoRifle` | 1 | 30 | 1 | 2002 | 0.4 | 0.5 | 0 | 1 | 0.1 | BulletGun 4 |
| 4 | 00CB | AntiAircraftRifle | `Weapon::AntiAircraftRifle` | 1 | 30 | 1 | 2002 | 0.667 | 0.5 | 0 | 2 | 1/6 | BulletGun 6 |
| 5 | 00CC | Vulcan | `Weapon::Vulcan` | 1 | 40 | 3 | 2002 | 0.5 | 0.3 | 0 | 2 | 1/12 | BulletGun 6 |
| 6 | – | (none) | | | | | | | | | −1 | | |
| 7 | 00CE | EggGun | `Weapon::EggGun` | 1 | 20 | 1 | 2004 | 1.0 | 0.2 | 0 | 0 | semi (−1) | BulletGun 2 |
| 8 | 00CF | LightShot | `Weapon::LightShot` | 1 | 20 | 3 | 2003 | 0 | 0.2 | 0 | 0 | 0.5 | BulletGun 2 |
| 9 | 00D0 | FlashShot | `Weapon::FlashShot` | 1 | 20 | 3 | 2003 | 0 | 0.2 | 0 | 1 | 1/12 | BulletGun 2 |
| 10 | 00D1 | RingShot | `Weapon::RingShot` | 1 | 20 | 3 | 2003 | 0 | 0.2 | 0 | 0 | 1.0 | Ring 4 |
| 11 | 00D2 | HeavyShot | `Weapon::HeavyShot` | 1 | 30 | 3 | 2003 | 0 | 0.2 | 0 | 2 | 0.125 | BulletGun 5 |
| 12 | 00DC | Grenade | `Weapon::Grenade` | 2 | 6 | 1 | 2002 | 0 | 1 | 1 | 2 | 0.5 | Cannon 4 + blast 4 |
| 13 | 00DD | Bazooka | `Weapon::Bazooka` | 2 | 6 | 1 | 2002 | 1 | 1 | 1 | 3 | 0.5 | Cannon 8 + blast 8 |
| 14 | 00DE | TankCannon | `Weapon::TankCannon` | 2 | 4 | 1 | 2002 | 0 | 1 | 1 | 3 | 0.5 | Cannon 16 + blast 14 |
| 15 | 00DF | BlackBarrel | `Weapon::BlackBarrel` | 2 | 8 | 1 | 2003 | 1 | 1 | 1 | 3 | 0.5 | Cannon 4 + blast 4 |
| 16 | 00E0 | BigBarrel | `Weapon::BigBarrel` | 2 | 8 | 1 | 2003 | 1 | 1 | 1 | 3 | 0.5 | Cannon 8 + blast 8 |
| 17 | 00E1 | EggBazooka | `Weapon::EggBazooka` | 2 | 6 | 1 | 2004 | 1 | 1 | 1 | 3 | 0.5 | Cannon 8 + blast 8 |
| 18 | 00E6 | AntiTankMissile | `Weapon::AntiTankMissile` | 3 | 6 | 1 | 2002 | 1 | 1 | 1 | 2 | 0.25/missile | Homing 8 (1 lock) |
| 19 | 00E7 | RocketLauncher4 | `Weapon::RocketLauncher` | 3 | **0** | 1 | 2002 | 1 | 1 | 0.4 | 2 | 0.25 | Homing 8 (4 locks) |
| 20 | 00E8 | RocketLauncher8 | `Weapon::RocketLauncher` | 3 | **0** | 1 | 2002 | 1 | 1 | 0.4 | 2 | 0.25 | Homing 8 (8 locks) |
| 21 | 00E9 | WormShooter | `@unnamed@WeaponWormShooter_cpp@::WormShooterBase` | 3 | **0** | 1 | 2003 | 1 | 1 | 0.4 | 2 | 0.25 | Homing 8 (2 locks) |
| 22 | 00EA | WideWormShooter | same | 3 | **0** | 1 | 2003 | 1 | 1 | 0.4 | 2 | 0.25 | Homing 8 (6 locks) |
| 23 | 00EB | BigWormShooter | same | 3 | **0** | 1 | 2003 | 1 | 1 | 0.4 | 2 | 0.25 | Homing 16 (2 locks) |
| 24 | 00F0 | VacuumPod | `Weapon::VacuumPod` | 4 | 20 | 3 | 2003 | 0.5 | 1 | 1 | 0 | – | vacuum (ThrowObject 6) |
| 25 | 00F1 | LaserRifle | (`WeaponLaserBase`, ctor 0x80221DAC) | 1 | 20 | 1 | 2002 | 0.5 | 0.4 | 0 | 0 | UNKNOWN | Laser 3 |
| 26 | 00F2 | Splitter | (`WeaponLaserBase`, 0x80225F88) | 1 | 20 | 3 | 2003 | 0.5 | 0.4 | 0 | 0 | UNKNOWN | Laser 4 |
| 27 | 00F3 | Reflector | (`WeaponLaserBase`, 0x80225A9C) | 1 | 20 | 3 | 2003 | 0.5 | 0.4 | 0 | 0 | UNKNOWN | Laser 5 |
| 28 | – | GunBattery | vehicle gun (`Vehicle::BatteryArmy`, see VEHICLES.md) | | 0 | 1 | 0 | 0 | 0.2 | 1.0 | −1 | | BulletGun 6 |
| 29 | – | BKBattery | vehicle gun (`Vehicle::BatteryBlackKiller`) | | 0 | 1 | 0 | 0 | 0.2 | 1.0 | −1 | | BulletGun 8 + blast 4 |
| 30 | 00FA | SurvivalKnife | `Weapon::Stick` | 5 | 6 | 2 | 2001 | 0 | 0 | 0 | −1 | – | melee 2 |
| 31 | 00FB | BlackSword | `Weapon::BlackSword` | 5 | 6 | 3 | 2001 | 0 | 0 | 0 | −1 | – | melee 4 |
| 32 | 00FC | DarkHammer | `Weapon::DarkHammer` | 5 | 6 | 3 | 2001 | 0 | 0 | 0 | −1 | – | melee 6 |
| 33 | 00FD | EggSpear | `Weapon::Stick` | 5 | 6 | 3 | 2001 | 0 | 0 | 0 | −1 | – | melee 2 |
| 34–37 | 00FE–0101 | Stick0100, Stick0200, Stick0201, Stick0202 | `Weapon::Stick` | 5 | 4 | 3 | 2001 | 0 | 0 | 0 | −1 | – | melee 2 |
| 38 | 0102 | Torch0300 | `@unnamed@WeaponTorch_cpp@::Torch` (stick base, ctor 0x8023F308) | 5 | 4 | 3 | 2001 | 0 | 0 | 0 | −1 | – | melee 1 + fire |
| 39–45 | 0103–0109 | Stick0301, 0302, 0400, 0401, 0402, 0403, 0404 | `Weapon::Stick` | 5 | 4 | 3 | 2001 | 0 | 0 | 0 | −1 | – | melee 2 |
| 46 | – | (none) | | | | | | | | | −1 | | |
| 47–56 | 010B–0114 | Stick0501–0504, 0600–0604, 0700 | `Weapon::Stick` | 5 | 4 | 3 | 2001 | 0 | 0 | 0 | −1 | – | melee 2 |
| 57 / 58 | – | Katana01 / Katana02 | `WeaponStickBase` (0x80276054 / 0x80275F60) | 5 | 4 / 8 | 3 | 2001 | 0 | 0 | 0 | −1 | – | melee 4 / 8 |
| 59 / 60 | – | SatelliteLaser01 / 02 | `WeaponSpecialSatelliteLaser::SatelliteLaser` (lock-on, 1 / 2 locks) | 3 | 6 / 10 | 1 | 2002 | 0 | 0 | 0 | −1 | UNKNOWN | UNKNOWN |
| 61 / 62 | – | VacuumEgg01 / 02 | (ctor 0x802776EC / 0x80277648, 0x98 B) | UNKNOWN | 20 / 30 | 3 | 2002 | 0.5 | 1 | 1 | −1 | – | vacuum |
| 63 / 64 | – | OmochaoGun01 / 02 | (0x8027674C / 0x802766B8, 0x54 B) | UNKNOWN | 10 / 20 | 1 | 2002 | 1 | 0.5 | 0 | 0 | UNKNOWN | Laser 6 / 10 |
| 65 / 66 | – | HealCannon01 / 02 | `WeaponCannonBase` (0x80275D1C / 0x80275C80) | 2 | 10 / 20 | 2 | 2002 | 1 | 1 | 1 | 3 | 0.5 | heal blast lv 0 / 1 |
| 67 | – | ShadowRifle | `WeaponGunBase` (0x8027722C) | 1 | 20 | 3 | 2002 | 0 | 0 | 0 | 2 | UNKNOWN | BulletGun **32** |
| 68–71 | – | (none; reset list 0x8051F4D8 = 0x44..0x47) | | | | | | | | | −1 | | |

Vehicle weapons (details in `VEHICLES.md` §6): BatteryArmy fires bullet type 28 (power 6) every 0.2 s, GunLift the same
bullet every 0.0625 s, BatteryBlackKiller bullet type 29 (power 8 + blast 4) with a 1.0 s cooldown, AirBolt is a
`WeaponLockonBase` (index 0x44, 6 locks, range 2000) firing Homing kind 4 (power 4); destroyed batteries drop
AntiAircraftRifle (4) / BlackBarrel (15) pickups, a destroyed WalkerCannon drops 4.

All columns PROVEN from data (registry, info blocks with static-initialiser overlay, bullet tables) except where
marked UNKNOWN; "kind" PROVEN from the base-class ctor argument (§3.5). The zero-ammo lock-on weapons (19–23): with
the default ammo (−1 → 0) the control's empty-ammo check (`ShadowWeapon_HasAmmo`) discards them immediately, so they
are LIKELY not player pickups with default ammo (they are created through the extra factory by HeavyDog/others).

### 3.4 Bullet-type table 0x8051E840[w] (5 words per entry) — PROVEN
`{+0 init(resources) — BulletType_Init 0x80051DF0 (called by each weapon's resource hook with w), +4 release —
0x80051DA8, +8 spawn(shot) — BulletType_Spawn 0x80051D5C, +0xC alt spawn — BulletType_CreateAlt 0x80051C18, +0x10 f32
range — BulletType_GetRange 0x80051BF0 (= WeaponBase::vf05, also the auto-aim range)}`.
Entries: 1–5, 7–11 BulletGun rows (§2.3), 10 → 0x8051EFD0 Ring, 12–17/65/66 → 0x8051EA24.. Cannon rows,
18–23 → 0x8051EE34.. Homing rows, 24/61/62 → 0x8054EC78 (all zero: vacuum weapons have no bullet),
25–27/63/64 → 0x80547120.. Laser rows, 28/29/57/58/67 → BulletGun rows, 59/60 → 0x8054EC30/0x8054EC44
(`BulletSatelliteLaser`), 6, 30–56, 68+ → none.

### 3.5 Class framework (RTTI PROVEN; slot roles as stated)

**`Weapon::WeaponBase`** (ctor 0x8005D21C(this, owner r4, index r5, kind r6)):
+0x00 vptr · +0x04 smart header · +0x0C owner ref · **+0x14 index** · **+0x18 kind** · +0x1C model (`fn_8006335C(model,
i)` returns locator i; 0 = muzzle) · +0x20 Vec3 muzzle position · +0x2C Vec3 muzzle direction · +0x38 subclass
(GunBase: shots this frame; LockonBase: LockonState*; StickBase: attack component).
Kind (PROVEN from the base ctors): **1** `WeaponGunBase` 0x8005ED30 (incl. `WeaponLaserBase`, ChaosSpear),
**2** `WeaponCannonBase` 0x8005E314, **3** `WeaponLockonBase` 0x8005FD1C (this, owner, maxLocks r5, index r6),
**4** `WeaponVacuumBase` 0x8023A2BC, **5** `WeaponStickBase` 0x80067BB4 (this, owner, index, power f1); 6 is accepted
by the equip switch (ControlGun) but no producer was found (UNKNOWN).

| Slot | Role | Evidence | Conf |
|---|---|---|---|
| 0 | dtor | RTTI | PROVEN |
| 1 | SetMuzzle(pos, dir) → +0x20/+0x2C | `WeaponBase::vf01 0x8005D118`; called by `ShadowWeapon_UpdateMuzzleFromHand 0x8023ADAC` | PROVEN |
| 2 | Update(dt) (GunBase: +0x38 = 0) | `ShadowWeapon::Update` 0x8023B738; 0x8005ECCC | PROVEN |
| 3 / 4 | show / hide model (`fn_80063258` / `fn_8006327C` on +0x1C) | 0x8005D08C / 0x8005D060 | LIKELY |
| 5 | GetRange = bullet-type +0x10 | 0x8005D03C | PROVEN |
| 6 | hold offset constant (x, −1, −2.703) | 0x8005D020 | LIKELY |
| 7 | GetRecoil = info+0x28 | 0x8005CFFC | PROVEN |
| 8 | GetFireSlowdown = info+0x2C | 0x8005CFD8 | PROVEN |
| 9 | returns 0 | 0x8005C204 | UNKNOWN |
| 0xA | **Fire()** → cooldown (s); negative = wait for trigger release | ControlGun 0x8023C9F0, ControlCannon 0x8023BFA0 | PROVEN |
| 0xB | **FireAt(velocity, aimPoint)** → cooldown (guns); lock-on: fire at a lock | ControlGun 0x8023C9C8 | PROVEN |
| 0xC | OnTriggerRelease → cooldown (ChaosSpear fires here) | ControlGun 0x8023CA64 | PROVEN |
| 0xD / 0xE | OnFireStart / OnFireEnd | ControlGun 0x8023C8DC / 0x8023CB2C | PROVEN |

Projectile classes: `Weapon::Bullet::{BulletBase, BulletGun, Cannon, CannonBomb, Homing, Ring, ChaosSpear::ChaosSpear}`,
`@unnamed@BulletLaser_cpp@::Laser`, `BulletSatelliteLaser::SatelliteLaser`, managed by
`Weapon::Bullet::BulletManagerTask` (layer 15). Bullet vtables have 2 slots (dtor, Update).

**Player side** — `Player::Shadow::ShadowWeapon` (0x60 B, player+0x238; `Task` vptr at +0x18):
+0x28 player · +0x30 bitset (0 trigger held, 1 cancel requested, 2 firing active, 3 shot fired this frame, 4 UNKNOWN)
· +0x34/+0x38 `shared_ptr<WeaponBase>` · +0x3C control object · +0x40 Vec3 recoil offset · +0x4C Vec3 hold offset
(weapon slot 6) · +0x58 hold angle (90°·info+0x24).

| Slot | Name | Evidence | Conf |
|---|---|---|---|
| 1 | Update | 0x8023B6D0: weapon Update, control Update, recoil/rumble on bit 3, clamp & decay (×0.859375) | PROVEN |
| 2 | AddWeapon(w, ammo) | 0x8023B620: pickup SE; same weapon → AddAmmo(ammo<0 ? info ammo : ammo); else Remove + CreateAndEquip | PROVEN |
| 3 | EquipWeaponObject(weaponRef, ammo) | 0x8023B598 (used by Super Shadow with 999) | PROVEN |
| 4 / 5 / 6 / 7 | GetWeapon / GetWeaponIndex / GetWeaponKind / GetWeaponRange | 0x8023B574 / 538 / 4FC / 4BC | PROVEN |
| 8 / 9 / 0xA | StartUse / CancelUse / EndUse (commands 0x115/0x116/0x117) | 0x8023B46C / 41C / 3D8; flag 0x19 | PROVEN |
| 0xB | ThrowWeapon unless flag 0x33 (CastWeaponCommand, empty ammo) | 0x8023B374 → `ShadowWeapon_ThrowWeapon 0x8023B08C` (spawns the pickup with the remaining ammo via `fn_80221E8C`, SE 0x3025) | PROVEN |
| 0xC | DiscardWeapon (remove without throw) | 0x8023B328 → `ShadowWeapon_RemoveWeapon 0x8023A490` | PROVEN |
| 0xD / 0xE | hide / show weapon model | 0x8023B2F0 / 0x8023B2B8 | LIKELY |
| 0x10 / 0x11 | IsFiring (bit 2) / ShotFiredThisFrame (bit 3) | 0x8023B288 / 0x8023B260 | PROVEN |

`ShadowWeapon_Equip 0x8023A92C(this, weaponRef, ammo)`: set ammo (record+0x1C), choose the control by kind (1/6
ControlGun `0x8023C260`, 2 ControlCannon `0x8023BAB8`, 3 ControlLockon `0x8023D4D4`, 4 ControlVacuum `0x8023DB54`,
5 ControlStick `0x8023D8E4`), hold type → motion, bind the arm-IK callback (`ShadowWeapon_ComputeArmIKTarget
0x8023A384`), hold angle, **player flag 0x1F** (has weapon), HUD record +0x10 = kind, +0x14 = index. Control classes
(typeinfo names PROVEN): `@unnamed@PlayerShadowWeapon{Gun,Cannon,Lockon,Stick,Vacuum}_cpp@::Control{Gun,Cannon,Lockon,
Stick,Vacuum}` (vtables 0x80548CF8 / 0x80548CB4 / 0x80548DB4 / 0x80548DD4 / 0x80548DF4; slot 1 Update).

### 3.6 Firing, fire rate, ammo (PROVEN)
`ControlGun::Update 0x8023C814` (+0x04 ShadowWeapon, +0x08 weapon, +0x10 cooldown, +0x14 shots in burst):
```text
if cooldown > 0: cooldown = max(cooldown - dt, 0)
if trigger held && !firing && cooldown <= 0: shots=0; firing=1; aim pose on; weapon.OnFireStart()
if firing && motion ready (fn_800AAA28) && cooldown == 0 && !Damage(0x11) && !Dead(0x12):
    announce to aim target (fn_8023C688)
    cooldown = PlayerTarget.HasAimTarget ? weapon.FireAt(player vel, PlayerTarget.GetAimPoint) : weapon.Fire()
if !trigger && !dmg && !dead: cooldown = weapon.OnTriggerRelease()
if weapon+0x38 (shots this frame): shots++ ; bit 3
if cancel || (!trigger && shots > 0): if firing: cooldown = max(cooldown, 0); firing=0; aim pose off; OnFireEnd()
ShadowWeapon_AddAmmo(-(weapon+0x38)) ; if !ShadowWeapon_HasAmmo → ShadowWeapon vslot 0xB (throw)
```
- A negative cooldown (HandGun, EggGun, ChaosSpear: −1.0) never counts down, so the next shot needs a trigger
  release (which resets it to 0): **semi-automatic**. Positive values are the auto-fire interval. Every frame the
  trigger is up, cooldown = `OnTriggerRelease()` (0 for all guns, 0x8005C1F4), so re-pressing is not rate-limited by
  the interval (only by the fire motion, `fn_800AAA28`).
- `ControlCannon::Update 0x8023BE44`: same trigger logic, always `Fire()` (no aim point); cannon `Fire` =
  `WeaponCannonBase::vf0A 0x8005E1A4` returns 0.5 and counts one shot.
- `ControlLockon::Update 0x8023D524`: §4.3.
- Fire intervals (returned by slot 0xA/0xB): see §6.
- **Ammo** = player game-state record +0x1C (`GameState_GetPlayerRecord(player+0x54)`), add clamps to 0..999
  (`fn_8016F31C`); `ShadowWeapon_AddAmmo 0x8023AEB4` skips decrements when player flag **0x3F** is set;
  `ShadowWeapon_HasAmmo 0x8023AE38` = flag 0x3F || ammo > 0. Flag **0x33** blocks throwing (2P gun characters and Super
  Shadow get 999 ammo + 0x33). A thrown weapon keeps its remaining ammo.

### 3.7 Pickup flow (PROVEN)
```text
SET_WEAPON create 0x8011F010 → new(0x48) SetWeapon::SetWeaponTask (layer 0xB) ctor 0x8011F3E0
   fn_8011F0C8: pos/rot/param → +0x44 = weapon index
   WeaponRegistry_CreateItem(w, ammo −1) (empty if not loaded → task kills itself 0x8011F4E4)
   new(0x1C) SetWeapon::Arms (0x8011FDC0: model, pickup collision size info+0x10/+0x14)
UserInput_BuildButtonCommands (0x800A0DD4/0x800A0E3C) → Player::PickupWeaponCommand 0x118 (+0x14 = object ref)
   PickupWeaponCommand::Execute 0x8009BD98: grounded (flag 2, !flag 3) → PickupWeapon behavior (0x16);
      else: if armed → ShadowWeapon vslot 0xB; send UseCommand (0x00D) to the object
SetWeapon command handler fn_8011F764: UseCommand from the player → AddWeaponCommand(0x11A){+0x14 = Item+8 index,
   +0x18 = Item+0xC ammo (−1)} to the player; hide the pickup (bit 0 of +0x18)
AddWeaponCommand::Execute 0x8009BAD8: armed with a different weapon → ShadowWeapon vslot 0xB (throw it);
   ShadowWeapon::AddWeapon(index, ammo); rumble 6
```
`Weapon::Item` (0x10): +4 model, +8 index, +0xC ammo (−1 default; `Weapon::Item::SetAmmo` 0x8005BD14).

### 3.8 Special weapons and levels (PROVEN)
`SpecialWeapon_SpawnUnlocked 0x8033F77C` reads save-data bits (`fn_800BC1C0(save+0x1710, bit)`): for pair k (bits 2k,
2k+1): level = −1, bit 2k → 0, bit 2k+1 → +1; weapon = table 0x8055EC98[2k + level] = {57/58 Katana,
59/60 SatelliteLaser, 61/62 VacuumEgg, 63/64 OmochaoGun, 65/66 HealCannon}; bit 10 → 67 ShadowRifle. Level 2
differs by ammo (Katana 4→8, Satellite 6→10, VacuumEgg 20→30, Omochao 10→20, HealCannon 10→20) and power
(Katana 4→8, Omochao laser 6→10, heal blast level 0→1). No level system exists for the regular weapons.

---------------------------------------------------------------------------------------------------------------------

## 4. Lock-on and targeting
(Traced by the targeting sub-investigation; spot-checked here: `PlayerTarget_SearchTargets` below-player filter
0x80206C08..0x80206C50, lock cone 0.93969 at 0x8005FDF8, ControlGun flag 0x41 at 0x8023D150 — all confirmed.)

### 4.1 Target registry — PROVEN
`TargetManager` singleton (bss 0x80576F54, getter `TargetManager_Get 0x80060180`; debug task "TargetManager" on layer 15
created by `StageManager_InitStage` 0x801785EC). `Target` (0x38 B; ctor 0x801E77A0(owner, mask)): +0 vptr (vf00 =
GetPosition → +8), +4 cached dist², +8 position, +0x14 approach direction, +0x20 interaction radius, +0x24 approach cos,
+0x28 owner, **+0x30 flags**. `Target_Create 0x801E7438` → `TargetManager_Register 0x801E7E04` (flag 0x800000);
removal by shared_ptr expiry, disable = clear bit 0.
`TargetSearchList` (0x14 B; `TargetManager_CreateSearchList 0x801E7C40`) filters with `remove_if`: keep-mask
0x801E83D0, exclude-mask 0x801E8448, exclude-target 0x801E8158, exclude-owner 0x801E7FA4, owner-kind 0x801E8358,
cone (strict `dot > cos`) 0x801E82C4, range (3D, dist² ≤ r²) 0x801E8528, approach 0x801E84C0; pick nearest (3D, strict
`<`, earliest wins ties) 0x801E879C.

| Flag | Meaning | Conf |
|---|---|---|
| 0x1 | enabled | PROVEN |
| 0x2 | approach cone active | PROVEN |
| 0x8 | light-dash target | PROVEN |
| 0x20 / 0x40 | ApproachControl targets (1) / (2) | STRONG |
| 0x80 | vacuum-able | STRONG |
| 0x400 | homing-attackable | PROVEN mask / STRONG role |
| 0x800 | gun aim target | PROVEN / STRONG |
| 0x1000 | lock-on only parts (0x1400) | STRONG |
| 0x2000 | look-at | PROVEN |
| 0x80000 | player | PROVEN |
| 0x800000 | registered | PROVEN |

Enemies register one Target per (part, mask) from `EnemyParamCommon::vf1B 0x801A65EC` (table 0x8053BB48[type];
common mask 0xC80 = vacuum + homing + gun) via `EnemyTarget_Create 0x8031B3A4` — STRONG.

### 4.2 `Player::PlayerTargetBase` / `Player::Shadow::PlayerTarget` (player+0x24C) — PROVEN
- Own Target (mask 0x80000, +0x10080; +0x2C00 in 2P stages so players can target each other) follows the player or
  its vehicle; disabled while hidden/damaged.
- Base update 0x80206F34: grind-only aim-yaw swing ±40° at ≤200°/s (constants 0x805F0CE4/0x805F0CE8 — **not**
  homing parameters, correcting player_trace §3.3).
- Impl search 0x800B6E38 each frame: nearest target in the front half-space (+0x4C), nearest 0x2000 look target,
  and — when a weapon is held and flag 0x41 (set by ControlGun/ControlVacuum) — the **aim target**: mask 0x800, 45°
  cone (0.7071) plus a ±20° horizontal wedge, range = weapon range (`ShadowWeapon::GetWeaponRange` → bullet-type
  +0x10); aim point smoothed at ≤360°/s.

**Homing attack** (`PlayerTarget_SearchTargets 0x80206B98`, called by `UserInput_BuildButtonCommands` 0x800A06B0 when
char-table byte +8 allows): keep mask 0x400, drop self; **unless Super (flag 0x25) keep only targets below the player**
(dot(−up, n̂) > 0); stick pushed (|move|² > 0.09): half-space around the move direction, range ≤ 150 (3D), approach
filter → nearest A, then within 45° → nearest B, result = B else A; stick idle: front half-space (cos 90° = 0), 150,
nearest. `fn_800A1A30` then line-of-sight checks (ray `fn_80097DB4`) before sending HomingAttackCommand (else
JumpDash). **Light dash** (`UserInput_FindLightDashTarget 0x800A18B0`): nearest target with flag 0x8 within 50 and in
sight; chaining uses a 45° cone.

### 4.3 Lock-on weapons (`Weapon::WeaponLockonBase`, `LockonTarget`, `LockonMark`) — PROVEN
`LockonState` (0x58 B, weapon+0x38; ctor 0x80060784): +0x28 vector of `shared_ptr<LockonTarget>`, +0x44 max locks,
+0x48 shots this frame, +0x4C next index, +0x50 fire timer, +0x54 lock timer. Per frame: prune dead/disabled locks
(no range re-check — locks persist), cone from muzzle locator 1 with length = range; `LockonState_Update 0x80060354`:
while locking and count < max and lock timer ≤ 0 → `LockonState_SearchNext 0x8005FD8C` (mask 0x1800, cone
cos 0.93969 = 20°, range, exclude locked, nearest to muzzle) → add lock (`LockonList_Add 0x80060AB4`: weapon vf0A
lock sound 0x201D/0x2020, `LockonTarget` + `LockonMark` reticle), lock timer 0.1 s. Volley: fire each lock via
vf0B (0.25 s apart); no locks → one unguided missile. `ControlLockon::Update 0x8023D524`: B held → locking, **stops
when lock count ≥ ammo**; B released → volley; ammo −= shots; post-volley cooldown 0.25 s.
`Weapon::LockonMark` (task "LockonMark", textures `am_tgt0`/`am_tgt1`): lock-in 0.1 s (scale 1→0.75), track, fade 0.5 s;
sends AnnounceAttackCommand kind 1 on lock / 2 on release.

| Weapon | idx | Max locks | Range | Missile kind |
|---|---:|---:|---:|---:|
| AntiTankMissile | 18 | 1 | 300 | 0 |
| RocketLauncher4 / 8 | 19 / 20 | 4 / 8 | 300 | 1 |
| WormShooter / Wide / Big | 21 / 22 / 23 | 2 / 6 / 2 | 300 | 2 / 2 / 3 |
| SatelliteLaser01 / 02 | 59 / 60 | 1 / 2 | 300 | – |
| `Weapon::AirBolt::AirBolt` (vehicle) | 0x44 | 6 | 2000 | 4 |

### 4.4 `Weapon::Bullet::Homing` flight (ctor 0x80057E24, Update 0x80057310) — PROVEN
Speed V + max(0, dot(shooterVel, dir)), then constant; steering `Homing_Steer 0x80057644` rotates the velocity toward the
target by ≤ rate·dt, rate += accel·dt. Kind 4 (AirBolt): V 500, 540°/s, +90°/s². Team 0 and not 2P: V 150, 90°/s,
+90°/s². Otherwise: V 150, 40°/s, accel sbss 0x805EF310 (no writer found → 0, UNKNOWN) and "lost" mode. After losing
the target it self-destructs after travelling 300 (+0x44). Branch labels "player 1P" / "enemy or 2P" LIKELY.

### 4.5 Gun auto-aim — PROVEN
When `PlayerTarget.HasAimTarget`, ControlGun fires with `FireAt`: direction = normalize(aimPoint − muzzle)
(`BulletShot_BuildAimedAtPoint 0x800693C0`); otherwise along the muzzle +Z (`fn_8006954C`). Team = 0 if the owner is
`Player::Player` (+0x5C == 1) else 1. Laser reflect retargets the nearest 0x800 target within 100
(`ControlGun_OnLaserReflect 0x8023C2B0`, STRONG).

---------------------------------------------------------------------------------------------------------------------

## 5. Address tables

### 5.1 Damage core
| address | semantic name | confidence | evidence |
|---|---|---|---|
| 0x800C6A4C | CharaColli_DispatchAttackPair | PROVEN | level > defense test 0x800C6C30/0x800C6D74; param build; 3 callback lists |
| 0x800C6608 | CharaColli_DispatchAttackOneWay | LIKELY | same structure; callers are shape tests |
| 0x800C69E8 | AttackCallbackParam::AttackCallbackParam | PROVEN | zeroes +0x34..+0x50, +0x54 |
| 0x800C6FE0 / 0x800C6F6C | CharaColli_GetContactPoint / GetContactNormal | STRONG | defaults 0 / (0,1,0) |
| 0x800C6920 | AttackCallbackList_Invoke | STRONG | calls fn_80078B9C if non-empty |
| 0x800C4414 | AttackCallbackList_IsEmpty | PROVEN | word == 0 |
| 0x8005A968 | CharaColliAttack::CharaColliAttack | PROVEN | defaults level 3 |
| 0x800C475C | CharaColliAttack_CreateForColli | PROVEN | new(0x58); stored via CharaColliBaseCharInfo+0x38 |
| 0x80054EF8 | CharaColliAttack_SetAttack | PROVEN | +0x54=1, +0x38, +0x34, +0x40, lists +0x10/+0x28 |
| 0x80096E54 | CharaColliAttack_SetDefense | PROVEN | +0x55=1, +0x3C, list +0x1C |
| 0x8005AFD4 | CharaColliAttack_IsIgnored | LIKELY | walks +0x44 weak list |
| 0x8005A428 | Bullet_CreateCharaColliAttack | PROVEN | SetAttack(5, 0x40, f1) |
| 0x80095F14 | PlayerCharaColli_OnDamaged | PROVEN | DamageCommand(10.0) |
| 0x80096C44 | PlayerCharaColli_Setup | STRONG | SetDefense(0, 0x80095F14) |
| 0x80078098 | PlayerAttack_ApplyType | PROVEN | table 0x8052124C |
| 0x800788A0 | PlayerAttack_SetType | PROVEN | +0x20 = type |
| 0x80077A84 | Player::Attack::SetAttackType | PROVEN | callers pass 4/5/6/7/0xA/0xB |
| 0x80077AA8 | Player::Attack::SetComboAttack | STRONG | jump table 0x80521370 |
| 0x80077A3C / 0x80078830 | Player::Attack::SetRadius / PlayerAttackImpl_SetRadius | STRONG | colli vslot 0x10(−2, r) |
| 0x801DBBA4 | DamageCommand::DamageCommand | PROVEN | (dir, prio, power) |
| 0x801A8718 | EnemyStatusCommon::ApplyDamage | PROVEN (existing) | HP −= power |

### 5.2 Weapons (see `notes/weapon_symbols.csv` for all rows)
| address | semantic name | confidence | evidence |
|---|---|---|---|
| 0x8051F058 | WeaponRegistry (data, 72×16) | PROVEN | readers 0x8005B888..0x8005C03C |
| 0x8051E840 | BulletTypeTable (data) | PROVEN | readers 0x80051BF0..0x80051DF0 |
| 0x8005BAFC | WeaponRegistry_CreateItem | PROVEN | loaded check, item+8 = w, SetAmmo |
| 0x8005BAD4 / 0x8005BAAC / 0x8005BA7C / 0x8005BA54 / 0x8005BA2C / 0x8005BA04 / 0x8005B9C0 | WeaponInfo_Get{DefaultAmmo, HoldType, PickupSE, HoldAngle, Recoil, FireSlowdown, PickupColliSize} | PROVEN | info +0x18/+0x1C/+0x20/+0x24/+0x28/+0x2C/+0x10 |
| 0x8005B95C | WeaponRegistry_GetFireRumble | PROVEN | registry +8 → fn_8034ED50 |
| 0x8023B620 | Player::Shadow::ShadowWeapon::AddWeapon | PROVEN | slot 2 |
| 0x8023A92C | ShadowWeapon_Equip | PROVEN | kind switch, flag 0x1F |
| 0x8023C814 / 0x8023BE44 / 0x8023D524 | Control{Gun,Cannon,Lockon}::Update | PROVEN | vtables 0x80548CF8 / CB4 / DB4 |
| 0x8011F010 | SetWeapon_Create (SET 0x0020 hook) | PROVEN | new(0x48) SetWeaponTask |
| 0x8011F764 | SetWeapon_OnCommand | PROVEN | UseCommand → AddWeaponCommand |
| 0x8009BAD8 | Player::AddWeaponCommand::Execute | PROVEN | slot 1 |
| 0x8033F77C | SpecialWeapon_SpawnUnlocked | STRONG | save bits → table 0x8055EC98 |

---------------------------------------------------------------------------------------------------------------------

## 6. Constants

| value | address | consumer pc | meaning | confidence |
|---|---|---|---|---|
| 10.0 | .sdata2 0x805F3A94 | 0x80096164 | power in every player DamageCommand (unused by the receiver) | PROVEN |
| 1.0 | 0x805F6928 | 0x801AA3A8 | enemy body contact power | PROVEN |
| 1 / 1 | li 0x801AA39C | 0x801AA3BC/0x801AA3C4 | enemy body attack level / defense | PROVEN |
| 3 | li 0x80078320 | SetAttack | player attack level | PROVEN |
| 5 | li 0x8005A4BC | SetAttack | projectile attack level | PROVEN |
| 0 | li 0x80096E14 | SetDefense | player defense level | PROVEN |
| −1.0 | 0x805F3060 / 0x805F7558 / 0x805FA434 | 0x8005F000 / EggGun / ChaosSpear | semi-automatic cooldown (HandGun, EggGun, ChaosSpear) | PROVEN |
| 1/12 s | 0x805F32E0 / 0x805F3328 / 0x805F301C | 0x80068D80.. / 0x80069A34.. / 0x8005E8EC.. | SMG / Vulcan / FlashShot interval | PROVEN |
| 0.1 s | 0x805F2F28 | 0x8005CE40 | AutoRifle interval | PROVEN |
| 1/6 s | 0x805F2EF4 | 0x8005C5A0 | AntiAircraftRifle interval | PROVEN |
| 0.125 s | 0x805FA474 | 0x802F1F94 | HeavyShot interval | PROVEN |
| 0.5 s | 0x805F308C | 0x8005F7C4 | LightShot interval | PROVEN |
| 1.0 s | 0x805F3104 | 0x80064AD0 | RingShot interval | PROVEN |
| 0.5 s | 0x805F300C | 0x8005E204 | cannon interval | PROVEN |
| 0.25 s | 0x805F2F08 / .sdata 0x805E4DD8 / 0x805F9EB0 | ATM / RL / AirBolt vf0B | missile interval | PROVEN |
| 500 | sbss 0x805EF2F4 (init 0x80056C2C) | 0x80055834.. | BulletGun muzzle speed | PROVEN |
| 250/400/600/2000/100/200 | 0x8051EC64.. (init 0x80056BB0) | BulletGun ctor 0x80056720 | BulletGun range (+0x40) | PROVEN |
| 2,2,4,6,6,2,2,2,5,6,8,4,8,32 | 0x805F2E50..0x805F2E64 | BulletGun factories | BulletGun power per entry (§2.3) | PROVEN |
| 4,8,16,4,8,8,99,0,0 | .data 0x8051E9A0 (+0xC stride) | 0x80054148 | Cannon shell power by type | PROVEN |
| 4,8,14,4,8,8,4,0,0 | .data 0x8051EB48 (+0x14 stride) | 0x80055460 | explosion power by type | PROVEN |
| 30,50,100,40,70,50,40 | 0x8051EB4C.. (init 0x80055564) | CannonBomb ctor 0x800553BC | explosion radius (LIKELY) by type | PROVEN value |
| 8,8,8,16,4,4,4,4 | .rodata 0x804AD4D0 (+0x1C stride) | 0x8005818C | missile power by kind | PROVEN |
| 3,4,5,6,10 | .data 0x8054707C (+0x1C) | 0x80220CCC | laser power by type | PROVEN |
| 4.0 / 400 | 0x805F2EB0 / 0x805F2EB4 | 0x80059534 | Ring power / speed | PROVEN |
| 6 + 14·c | 0x805FA488 / 0x805FA48C | 0x802F2A64 | Chaos Spear power | PROVEN |
| 3.0 / 2.4 s | 0x805FA43C / 0x805FA440 | 0x802F1548 / 0x802F1568 | Chaos Spear max charge / "charged" cue | PROVEN |
| 2 / 4 / 6 / 1 | 0x805F3298 / 0x805F2FF8 / 0x805FA458 / 0x805F83F8 | stick factories | melee power (stick / BlackSword / DarkHammer / Torch) | PROVEN |
| 4 / 8 | 0x805F8FFC / 0x805F8FF8 | 0x80276078 / 0x80275F84 | Katana Lv1 / Lv2 power | PROVEN |
| 90° | 0x805F839C | 0x8023ABA0 | hold-angle scale (× info+0x24) | PROVEN |
| 0.859375 | 0x805F83BC | 0x8023B8A4 | recoil offset decay per frame | PROVEN |
| 999 | li 0x8016F32C | ammo add | ammo clamp | PROVEN |
| 150 | 0x805F79DC → sbss 0x805F0CE0 | 0x80206C98 | homing attack range (3D) | PROVEN |
| 0.09 | 0x805F79CC | 0x80206C5C | stick threshold for directed homing search | PROVEN |
| cos 45° / cos 90° | 0x805F79D0 / 0x805F79D8 | 0x80206CDC / 0x80206D98 | homing cones | PROVEN |
| 50 | sbss 0x805EF608 | 0x800A198C | light-dash start range | PROVEN |
| 0.7071 / 0.9397 / −0.342 | 0x805F3E1C..28 | 0x800B709C.. | gun aim cone 45° + ±20° wedge | PROVEN |
| 0.93969 | 0x805F309C | 0x8005FDF8 | lock-on cone 20° | PROVEN |
| 0.1 s | 0x805F30A0 | 0x8005FE9C | lock interval | PROVEN |
| 300 / 2000 | 0x805F2E88 / 0x805F2E9C | via 0x80051BF0 | lock/aim range (ATM/RL/Worm) / AirBolt | PROVEN |

---------------------------------------------------------------------------------------------------------------------

## 7. Open items
- `AttackCallbackParam` +0x40..+0x4C consumers; enemy-side writers of result 3; meaning of the player-side result-5
  state (`fn_8009672C() == 2`) and of flag 0x21 (explosion-proof).
- Do enemy receivers filter enemy projectiles by team (level 5 > enemy defense 1)?
- Katana bullet entries (57/58) — which action fires them; SatelliteLaser damage; Vacuum weapons (DamageCommand power 1
  to the vacuumed object, `VacuumCommand`) not traced.
- Fire intervals of LaserRifle/Splitter/Reflector, OmochaoGun, ShadowRifle (unnamed classes; run
  `agent_weapon_vt.py` after naming their vtables).
- Registry +0xC byte (reader `fn_801C0A1C`) and weapon kind 6.
- WEAPON_CONTAINER / SECRET CONTAINER break → pickup path (index assumed identical to SET_WEAPON).
- Targeting: category bits 0x10000/0x20000/0x40000, sbss 0x805EF310 writer, LockonState+0x38 search override
  installer, SET id of SatelliteLaser pickups.

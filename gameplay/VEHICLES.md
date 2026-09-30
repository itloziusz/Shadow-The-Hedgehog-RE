# VEHICLES — SET_VEHICLE, vehicle classes, riding, car physics, damage (first pass)

Source: `sys/main.dol` (GUPP8P). Tools: `tools/q.py`, `tools/agent_player_q.py`, and the helpers written for this pass
`tools/agent_weapon_veh_q.py` (`funcs`, `words`, `rows`, `grep`, `tbl` = table-field tracker, `fconst`, `icalls`) and
`tools/agent_weapon_veh_setscan.py` (all SET_VEHICLE records in `files/`); `tools/agent_weapon_veh_symbols.py` prints the
recovered symbol rows (CSV, subsystem `vehicle`, curated addresses skipped).
Conventions: RTTI class names are ORIGINAL; every method/field name is a recovered semantic. Confidence:
**PROVEN** (direct code/data), **STRONG** (several consistent pieces), **LIKELY** (single indirect), **UNKNOWN**.
Virtual slot *i* is at `vptr + 8 + 4*i`; CharInfo-derived objects keep the vptr at `+0x38`.

---------------------------------------------------------------------------------------------------------------------

## 0. End-to-end chain (every edge PROVEN unless marked)

```text
SET record id 0x004F SET_VEHICLE (param Vehicle = 1..13)
 └ create hook 0x8011E508: new(0x60) SetVehicle::SetVehicleTask(layer 0xB Gadget, slot)            ctor 0x8011E934
     └ SetVehicleTask_TrySpawn 0x8011E588 → Vehicle_Spawn(type) 0x801B4C74
          registry 0x8053C890[type] = {name, create(type)}  → TVehicleBase* (new(size) + class ctor)
          boost::shared_ptr<TVehicleBase>; new(0x30) Vehicle::CharaColliMove(layer 0xC Vehicle)
          new(0x38) Vehicle::VehicleTask(layer 0xC) → new(0x30) Vehicle::Effect; vehicle Init/Setup (vslot 0xC)
       then VehicleTask+0x30 = despawn distance = (record.range+1)*100; vehicle vslot 0xA SetTransform(pos, rot)
per frame (layer 12 "Vehicle"): Vehicle::VehicleTask::Update 0x801B5ECC → vehicle vslot 0xF Update(dt)
player near a vehicle (CharInfo kind +0x50 == 0x33), X pressed:
 UserInput_BuildButtonCommands 0x800A0E8C → Player::GetOnVehicleCommand(0x123, vehicleRef) in the X slot
  → GetOnVehicleCommand::Execute 0x8009AE40 → InitializeArgsDriveVehicle(id 0x1B) → Executer_ChangeBehavior
  → DriveVehicle::CanEnter 0x8007EEF0 (vehicle has no rider) → DriveVehicle::Enter 0x8007EB0C
       TryRide 0x801B6B6C (vehicle+0x80 = player proxy), flags 0x10/0x31/0x38/0x40/0x43, A0.1, A0.7 or A0.8
  → DriveVehicle::Update 0x8007E6D0: 0.5 s jump arc to the ride point, then StartDriving 0x8007E268
       (vehicle vslot 0x10 OnRideStart) and glue the player to the ride point every frame
input while riding: stick → AccelerateInputCommand, A/X/B → ControlVehicleCommand(2..7) → DriveVehicle::OnCommand
  0x8007E8B8 forwards both to the vehicle (vehicle vslot 2 → VehicleBaseImpl command unit → vehicle vslot 0xD)
  cars (TVehicleCar) ignore those and read the pad directly (TVehicleCarMove_ReadPad 0x801BDBA0)
exit: vehicle calls Vehicle_EjectRider 0x801B63D0 → Player::StartJumpCommand to the rider → Jump behaviour
  → DriveVehicle::Leave 0x8007EA1C → Detach 0x8007DEE8 (vehicle vslot 0x11 OnRideEnd, ReleaseRider)
  car: X pressed below |speed| 25 (0x801BCCF8) or HP ≤ 0 (0x801BEEA0); vehicle object gone → DriveVehicle jumps off
destroy: HP (VehicleBaseImpl+8) ≤ 0 → SE + explosion effect + EjectRider + Vehicle_KillTask (VehicleTask flag 1)
```

---------------------------------------------------------------------------------------------------------------------

## 1. Catalog entries 0x46–0x4E (and the other vehicle descriptors)

All vehicle descriptors have **create = NULL** (`hooks=load/release/0`); they only load/release resources when the
id is enabled in `setid.bin`. Vehicles are placed exclusively through **SET_VEHICLE (0x004F)** (§2). (PROVEN,
`data/setobj_catalog.txt` + descriptor dumps below.)

### 1.1 Cars 0x46–0x49 (b1A = 1, b1C = 10) — info block = `{display name, create}` (PROVEN)

The two words after each 0x24-byte descriptor are a **vehicle-info record** `{const char* name; TVehicleBase*
(*create)(int type)}`; the vehicle registry (§2.1) points directly at `desc+0x24`.

| SET id | desc | +0x24 name | +0x28 create | load hook → (type) | release hook → (type) | registry type |
|---|---|---|---|---|---|---|
| 0x0046 "Noraml Car" (sic) | 0x8053CA50 | "Normal Car" | 0x801B8B2C | 0x801B8E70 → fn_801BFB60(1), VehicleCar_LoadResources(1) | 0x801B8C84 → VehicleCar_ReleaseResources(1) | 1 |
| 0x0047 "Open Car" | 0x8053CA7C | "Open Car" | 0x801B8B2C | 0x801B8E44 → (2) | 0x801B8C60 → (2) | 2 |
| 0x0048 "Armed Car" | 0x8053CAA8 | "Armed Car" | 0x801B8B2C | 0x801B8E18 → (3) | 0x801B8C3C → (3) | 3 |
| 0x0049 "Bike" | 0x8053CAD4 | "Bike" | 0x801B8B2C | 0x801B8DEC → (4) | 0x801B8C18 → (4) | 4 |

- `VehicleCar_Create` 0x801B8B2C(type): `new(0x174)` → `Vehicle::TVehicleCar::TVehicleCar(this, type)` 0x801B9898. PROVEN.
- `VehicleCar_LoadResources` 0x801B8CA8(type): prints `"%s::Initialize"` with the descriptor name (desc table
  0x8053CA50 + type*0x2C − 0x2C), `VehicleRegistry_MarkLoaded(type)` 0x801B4BF0, fn_801B8BC8/fn_801B9C90 (car
  singleton), then passes every id ≥ 0 of the per-car resource row **0x804CEBA8 + (type−1)*0x188** to fn_8006B634
  (row +0 = 0x60/0x63/0x62/0x5E, LIKELY model ids). `VehicleCar_ReleaseResources` 0x801B8B74 prints `"%s::Finalize"`.
  PROVEN (code) / LIKELY (field roles).

### 1.2 Non-car vehicles (b1A = 4, b1C = 0, w14 = 0x80) — no info block after the descriptor (PROVEN)

For b1A = 4 vehicles the words after the descriptor are RTTI base lists (e.g. 0x8053C428.. = typeinfo CharInfo /
Vehicle::TVehicleBase / Vehicle::Walker), **not** an info block; descriptors 0x4A/0x4B are adjacent (same TU). The
descriptor name is NULL in the DOL and written by the TU's static initialiser, which also writes the registry
record name (PROVEN, stores below).

| SET id | desc | name (static init pc) | load hook: MarkLoaded(type) + resources | registry type | info record | create | class, size |
|---|---|---|---|---|---|---|---|
| 0x004A | 0x8053C3E0 | "Walker" (0x801AF9D0) | 0x801ACDFC: type 5, fn_8020E354, res 0x2E/0x2F | 5 | 0x805E9488 | 0x801ACD84 | `Vehicle::Walker`, 0x118 |
| 0x004B | 0x8053C404 | "WalkerCannon" (0x801AF9D8) | 0x801ACD30: type 6, fn_8020E2CC, res 0x2E/0x2F | 6 | 0x805E9494 | 0x801ACCB8 | `Vehicle::Walker`, 0x118 |
| 0x004C | 0x8053C5B8 | "AirSaucer" (0x801B3034) | 0x801AFA64: type 7, res 0x16,3,0x5C,0x3F,0x3E,0x51,0x52.. | 7 | 0x805E94F0 | 0x801AF9EC | `Vehicle::AirSaucer`, 0x180 |
| 0x004D | 0x8053C74C | "AirWing" (0x801B3420) | 0x801B30C8: type 8, fn_801B45A4, res 0x53/0x54 | 8 | 0x805E9514 | 0x801B3040 | `Vehicle::BirdBase::BirdBase` ctor + vptr `Vehicle::BirdBase::AirWing` (0x801B3078), 0x120 |
| 0x004E | 0x805590BC | "AirBolt" (0x802CD150) | 0x802CC868: type 9, fn_802C9BF8, res 0x53/0x54 | 9 | 0x805ED194 | 0x802CC7F0 | `Vehicle::BirdBase::AirBolt`, 0x284 |
| 0x0018 | 0x80525960 | "BatteryArmy" (0x800EAD18) | 0x800E8CF8: type 10, res 0xC3/0xC | 10 | 0x805E636C | 0x800E8C80 | `Vehicle::BatteryArmy`, 0x1F4 |
| 0x0039 | 0x80525A40 | "BatteryBlackKiller" (0x800ECD20) | 0x800EADB4: type 11, res 0xC4/0x8 | 11 | 0x805E63A4 | 0x800EAD3C | `Vehicle::BatteryBlackKiller`, 0x1EC |
| 0x1518 "GunLift" | 0x8055D174 | (static name) | 0x80317708: types 12 **and** 13, fn_8006B59C | 12 / 13 | 0x805EE1C8 / 0x805EE1D0 ("GunLiftExpress"/"GunLiftLocal", 0x80319DF8/DFC) | 0x803176B8 / 0x8031766C | `Vehicle::GunLift`, 0x1E0, ctor arg 0 / 1 |

Markers (PROVEN): `VehicleDataStart` 0x258C (desc 0x8053D018) load hook 0x801C09F8 → fn_8006B6C8, fn_801B4C54;
`VehicleDataEnd` 0x258D (desc 0x8053D03C) load hook 0x801C0998 → `new(0x28) ResourceOneFileReq("common/vehicleResource.one")`
queued as ResourceManager slot **3** (fn_8001299C(mgr, 3, req)); release hook 0x801C096C → release slot 3 and
`VehicleRegistry_Reset` 0x801B4C0C. Car model/motion handles are taken from that slot 3 (0x801B94EC/0x801B9508).
The car load hook's first call fn_801BFB60 is an empty function (blr).

---------------------------------------------------------------------------------------------------------------------

## 2. SET_VEHICLE (0x004F) — `SetVehicle::SetVehicleTask`

### 2.1 Mapping `Vehicle` param → vehicle type (PROVEN: the param is used directly as the registry index)

`SetVehicleTask+0x34 = param Vehicle` (`SetSlot_GetParams(slot, this+0x34, 4)` @0x8011E9B4) is passed unchanged to
`VehicleRegistry_IsLoaded(type)` 0x801B4EFC and `Vehicle_Spawn(type)` 0x801B4C74, which index **0x8053C890 + type*8**.
There is no translation table. Usage count = SET_VEHICLE records over all stage files (`agent_weapon_veh_setscan.py`).

| Vehicle | registry record (addr) | name | catalog id | class (factory, size) | SET records |
|---:|---|---|---|---|---:|
| 0 | NULL | – | – | (spawn refused: record NULL) | 0 |
| 1 | 0x8053CA74 | Normal Car | 0x46 | TVehicleCar (0x801B8B2C, 0x174), car row 0 "Jeep" | 6 |
| 2 | 0x8053CAA0 | Open Car | 0x47 | TVehicleCar, row 1 "Open" | 4 |
| 3 | 0x8053CACC | Armed Car | 0x48 | TVehicleCar, row 2 "GUN" | 5 |
| 4 | 0x8053CAF8 | Bike | 0x49 | TVehicleCar, row 3 "BIKE" | 3 |
| 5 | 0x805E9488 | Walker | 0x4A | Walker (0x801ACD84, 0x118) | 5 |
| 6 | 0x805E9494 | WalkerCannon | 0x4B | Walker (0x801ACCB8, 0x118) | 11 |
| 7 | 0x805E94F0 | AirSaucer | 0x4C | AirSaucer (0x801AF9EC, 0x180) | 34 |
| 8 | 0x805E9514 | AirWing | 0x4D | BirdBase+AirWing vtable (0x801B3040, 0x120) | 4 |
| 9 | 0x805ED194 | AirBolt | 0x4E | BirdBase::AirBolt (0x802CC7F0, 0x284) | 14 |
| 10 | 0x805E636C | BatteryArmy | 0x18 | BatteryArmy (0x800E8C80, 0x1F4) | 18 |
| 11 | 0x805E63A4 | BatteryBlackKiller | 0x39 | BatteryBlackKiller (0x800EAD3C, 0x1EC) | 18 |
| 12 | 0x805EE1C8 | GunLiftExpress | 0x1518 | GunLift(…, 0) (0x803176B8, 0x1E0) | 7 |
| 13 | 0x805EE1D0 | GunLiftLocal | 0x1518 | GunLift(…, 1) (0x8031766C, 0x1E0) | 3 |

Registry entry = `{VehicleInfo* info; u8 loaded;}` (8 bytes, 14 entries). `loaded` is set by the load hooks
(`VehicleRegistry_MarkLoaded` 0x801B4BF0) and cleared for all 14 by `VehicleRegistry_Reset` 0x801B4C0C.
`VehicleRegistry_GetName` 0x801B4ED4 returns `info->name` (fallback string 0x805F6AD8). Stages using it (examples):
stg0100 Armed/Normal car, stg0202 Open car + Bike, stg0301 AirSaucer, stg0402 Walker/AirWing/AirBolt/BlackKiller,
stg0504 GunLift, stg0601 16 AirSaucers (full list: run the scan helper).

### 2.2 `Vehicle_Spawn` 0x801B4C74(type) (PROVEN)

`entry = 0x8053C890[type]`; requires `entry.loaded && entry.info && info->create`; layer = TaskManager layer 0xC;
`veh = info->create(type)`; `sp = boost::shared_ptr<TVehicleBase>(veh)` (0x801B4D6C, counted-base vtable 0x8053C90C);
`new(0x30) Vehicle::CharaColliMove(layer, sp)` 0x801B4D1C; `new(0x38) Vehicle::VehicleTask(layer, sp)` 0x801B4D38;
returns the VehicleTask.

### 2.3 `SetVehicle::SetVehicleTask` (size 0x60; bases Task, Gadget; vptr at +0x18, vtable 0x805287D4)

| Offset | Field | Evidence | Confidence |
|---|---|---|---|
| +0x00 | task name "SetVehicle" | ctor 0x8011E994 | PROVEN |
| +0x28 | SetSlot* (Gadget) | MarkRespawnable calls 0x8011E760 | PROVEN |
| +0x30 | owned `Vehicle::VehicleTask*` | TrySpawn 0x8011E5E8 | PROVEN |
| +0x34 | param `Vehicle` (type 1..13) | GetParams 0x8011E9B4 | PROVEN |
| +0x38 / +0x44 | spawn position / rotation (rad) | SetSlot_GetPosition / GetRotationRad 0x8011E998/9A4 | PROVEN |
| +0x50 | deferred-spawn handle from fn_8003A17C(pos) (engine global 0x8051DFA8) | ctor 0x8011E9CC; Update 0x8011E7AC | PROVEN (code) / UNKNOWN (meaning) |
| +0x58 | u8 spawn attempted/done | ctor 0x8011E9FC; TrySpawn 0x8011E60C | PROVEN |
| +0x59 | u8 SET link id | SetSlot_GetLinkIdByte 0x8011E9C0 | PROVEN |

Methods (PROVEN):
- `SetVehicleTask_CheckLink` 0x8011E624: link 0 → ok; else `fn_80169430(fn_800CBA84(), link)` (256×8 link table of
  the singleton 0x805EF79C, shared with Ring/Cage/HintRing…) must return 1, otherwise release the vehicle and
  `SetSlot_MarkRespawnableAndDetach`. (link table meaning: LIKELY "link group active".)
- `SetVehicleTask_TrySpawn` 0x8011E588: if CheckLink: keep an existing vehicle of the same type (fn_801B5EA8), else
  release it (fn_8011E564: VehicleTask+4 |= 1 = delete request) and, if the type is loaded, `Vehicle_Spawn`; then
  `VehicleTask+0x30 = fn_800C9D20(slot) = (record.range + 1) * 100` (despawn distance) and set +0x58.
- `Update` (vf02 0x8011E778): before spawning, wait while fn_80039F04(+0x50) is true, then TrySpawn +
  `VehicleTask_SetTransform` 0x801B60C4 (vehicle vslot 0xA with +0x38/+0x44). After spawning: drop the reference
  if the task died, or if the vehicle's vslot 0x14 is true **and it has a rider** (the SET object hands a boarded
  vehicle over); with no vehicle left → `SetSlot_MarkRespawnableAndDetach` + delete self.
- `Reset` (vf04 0x8011E690): re-read pos/rot/params/link, TrySpawn, re-place (same tail as Update).
- dtor 0x8011E8C8 → fn_8011E564: an **un-boarded vehicle is deleted together with its SET object**.

---------------------------------------------------------------------------------------------------------------------

## 3. Classes

### 3.1 Hierarchy and sizes (RTTI PROVEN; sizes from `operator_new` in the factories)

```text
CharInfo (vptr +0x38, kind +0x50)
 └ Vehicle::TVehicleBase            ctor 0x801B6FEC(this, type): CharInfo::CharInfo(this, 0x33)   vtable 0x8053C984
    ├ Vehicle::TVehicleCar           0x174  ctor 0x801B9898   vtable 0x8053CB20   (types 1–4)
    ├ Vehicle::Walker                0x118  ctor Vehicle::Walker::Walker   vtable 0x8053C448 (types 5, 6)
    ├ Vehicle::AirSaucer             0x180  vtable 0x8053C5FC (type 7)
    ├ Vehicle::BirdBase::BirdBase    vtable 0x80558F84
    │   ├ Vehicle::BirdBase::AirWing 0x120  (BirdBase ctor + vptr 0x8053C7AC written by the factory) (type 8)
    │   └ Vehicle::BirdBase::AirBolt 0x284  vtable 0x8055911C (type 9)
    ├ Vehicle::BatteryArmy           0x1F4  vtable 0x805259A4 (type 10)
    ├ Vehicle::BatteryBlackKiller    0x1EC  vtable 0x80525A84 (type 11)
    └ Vehicle::GunLift               0x1E0  vtable 0x8055D1B8 (types 12, 13)
Task-derived helpers: Vehicle::VehicleTask (0x38, vtable 0x8053C968), Vehicle::CharaColliMove (0x30, 0x8055E40C),
  Vehicle::Effect (0x30, 0x8056049C), SetVehicle::SetVehicleTask (0x60).
Held by boost::shared_ptr (RTTI only via sp_counted_base_impl): Vehicle::TVehicleCarMove (0x2F0), Vehicle::TVehicleCamera
  (0x40), VehicleArrow. Bound callbacks (boost::bind RTTI): VehicleBaseImpl(CharCommand&), TVehicleCarMove /
  AirSaucer / Walker / BirdBase / BatteryArmy / BatteryBlackKiller / GunLift (AttackCallbackParam&), cameras.
Models: Vehicle::BirdBase::BirdModelBase → Vehicle::AirWingModel / Vehicle::AirBoltModel. Weapon::AirBolt::AirBolt
  (bases Weapon::WeaponBase, Weapon::WeaponLockonBase; vtable 0x805591C0).
```
`CharInfo+0x50 = 0x33` is the **vehicle kind** that the player's interaction switch tests (0x800A0ABC). PROVEN.

### 3.2 `Vehicle::TVehicleBase` layout (common to all vehicles)

| Offset | Field | Evidence | Confidence |
|---|---|---|---|
| +0x00/+0x0C | position / rotation (CharInfo) | vf0A SetTransform 0x801B69DC; vf0B 0x801B691C | PROVEN |
| +0x30 | 2 (base) / 1 (TVehicleCar) | ctors 0x801B7078 / 0x801B9940 | UNKNOWN |
| +0x50 | kind 0x33 | CharInfo ctor arg @0x801B7004 | PROVEN |
| +0x54 | player index of the rider (−1 none) — vslot 1 | SetOccupied 0x801B66B8 / SetUnoccupied 0x801B6638 | PROVEN |
| +0x5C | 6 | ctor 0x801B7064 | UNKNOWN (player uses 1) |
| +0x60 | float* dt source: world 0x8057E780, rider's source while ridden | TryRide 0x801B6BD0 / ReleaseRider 0x801B6B34 | PROVEN |
| +0x70 | bitset: bit0 occupied, bit2/bit3 abandoned (set on ride end), bit4 = rider flag 0x2A | 0x801B65F4, 0x801B6664, 0x801B6A4C | PROVEN (bits) / LIKELY (names) |
| +0x74 | `Vehicle::Effect` task (damage smoke / explosion) | Effect ctor 0x803501F8 | PROVEN |
| +0x78 | owning `Vehicle::VehicleTask` | VehicleTask ctor 0x801B61EC | PROVEN |
| +0x80 | rider reference (player proxy 0 = PlayerBase+0x254 slot 0) | TryRide 0x801B6B6C; DriveVehicle::Enter 0x8007EBB0 | PROVEN |
| +0x88 | second rider proxy (PlayerBase+0x254 slot 1) | 0x801B6A94 ← 0x8007EC6C | PROVEN |
| +0x90 | self reference | Vehicle_Init 0x801B6D20 | PROVEN |
| +0x98 | `VehicleBaseImpl*` (0x2C) | ctor 0x801B7054 | PROVEN |

`VehicleBaseImpl` (ctor 0x801B76D8): +0 **type**, +4 **max HP**, +8 **HP** (both f32), +0xC owner vehicle
(Vehicle_Init 0x801B6D2C), +0x20 `CharCommandUnit` whose handler is `{0,-1,0x801B727C}` (PMF 0x8053C950) =
owner vslot 0xD **OnCommand**; +0x28 abandon timer (s). HP API (PROVEN): GetHP 0x801B6898, SetHP 0x801B68A4,
AddHP 0x801B68B0 (clamp to [0, max] via fn_80070588), GetMaxHP 0x801B6904, SetMaxHP 0x801B6910, RestoreHP
0x801B6888 (HP = max).

### 3.3 Vtable slot roles (slot = index; addresses per class in §3.4)

| Slot | Role | Evidence | Confidence |
|---|---|---|---|
| 1 | SetPlayerIndex (+0x54) | CharInfo::vf01 0x8007A600 | PROVEN |
| 2 | SendCommand → VehicleBaseImpl command unit (enqueue fn_801D9FAC) | 0x801B6ECC → 0x801B733C | PROVEN |
| 4, 5, 8 | forward the CharInfo query to the rider (0/false without rider) | 0x801B6E58, 0x801B6DE4, 0x801B6D60 | PROVEN (code) / UNKNOWN (query meaning) |
| 0xA | SetTransform(pos, rot) (TVehicleCar also resets the car model, HP = max) | 0x801B69DC, 0x801B9700; called by 0x801B60C4 | PROVEN |
| 0xB | GetRidePoint(outPos, outRot) — seat and VehicleArrow anchor | DriveVehicle 0x8007E428; UserInput 0x800A0FB4 | PROVEN |
| 0xC | Setup (after shared_ptr exists; sets HP for non-cars) | Vehicle_Init 0x801B6D38 | PROVEN |
| 0xD | OnCommand(CharCommand&) | 0x801B727C | PROVEN |
| 0xE | generic CharInfo hook also used by PickupObject/Throw | – | UNKNOWN |
| 0xF | Update(dt) | VehicleTask::Update 0x801B5F04 | PROVEN |
| 0x10 / 0x11 | OnRideStart(riderRef) / OnRideEnd | StartDriving 0x8007E2D0 / Detach 0x8007DF38 | PROVEN |
| 0x12 | has own controls → player A0.8 set, A0.7 (hand weapon) cleared, B sends ControlVehicleCommand 6/7 | Enter 0x8007ED64; UserInput 0x800A01B4 | PROVEN |
| 0x13 | → player flag 0x3A (stick not rotated into camera frame) | Enter 0x8007EDA0 | PROVEN |
| 0x14 | SET task releases the vehicle once it has a rider | 0x801B5E44 ← 0x8011E81C | PROVEN |
| 0x15 | abandoned-despawn timer enabled | VehicleTask::Update 0x801B5F98 | PROVEN |
| 0x16 / 0x17 | rider per-frame hook enabled / hook (DriveVehicle bit2) | 0x8007E2F4 / 0x8007E7B8 | PROVEN (call) / UNKNOWN (effect) |
| 0x18 / 0x19 / 0x1A | abandon warning = model blink on / off / is-blinking (after 25 s) — Walker 0x801AE1DC.. and AirSaucer 0x801B17E0.. toggle model blink; cars use the no-op defaults | VehicleTask::Update 0x801B6024/0x801B6058/0x801B6000 | PROVEN (calls) / STRONG (blink) |
| 0x1B, 0x1C | not called from player/vehicle-task code | – | UNKNOWN |
| 0x1D | rider motion hook (StartDriving passes motion value, Detach passes 0) | 0x8007E338 / 0x8007DF80 | PROVEN (call) / UNKNOWN |

Constant overrides (PROVEN by 2-instruction bodies): slot 0x12 true for BatteryArmy, BatteryBlackKiller, GunLift,
AirBolt and WalkerCannon (Walker::vf12 0x801ACC84 = `type == 6`); false for cars, Walker, AirSaucer, AirWing.
Slot 0x13 true for BirdBase/AirWing/AirBolt, batteries, GunLift. Slot 0x14 false only for the batteries.
Slot 0x15 true for TVehicleCar (0x801B8B24), Walker (0x801ACCB0), AirSaucer (0x801AF9E4); false for the others.

### 3.4 Vtable matrix (slots 0x0A–0x1D)

| slot | TVehicleCar | Walker | AirSaucer | BirdBase | AirWing | AirBolt | BatteryArmy | BatteryBlackKiller | GunLift |
|---|---|---|---|---|---|---|---|---|---|
| 0A | 801B9700 | 801AEDBC | 801B2198 | 802CC278 | 802CC278 | 802CC278 | 800E9E0C | 800EBEDC | 80318AAC |
| 0B | 801B9608 | 801AF580 | 801B691C | 801B691C | 801B3280 | 802CCF2C | 800E9DA8 | 800EBE78 | 80318A14 |
| 0C | 801B66EC | 801AF2F0 | 801B296C | 801B66EC | 801B31E4 | 802CCE90 | 800EA8B4 | 800EC880 | 80319098 |
| 0D | 801B9744 | 801AEE10 | 801B2370 | 802CC3D4 | 801B31C4 | 802CCDF8 | 800EA15C | 800EC13C | 803178F4 |
| 0E | 801B66E8 | 801AEE0C | 801B2254 | 802CC2C8 | 802CC2C8 | 802CC2C8 | 801B66E8 | 801B66E8 | 801B66E8 |
| 0F | 801B9688 | 801AE994 | 801B189C | 802CBF48 | 802CBF48 | 802CCDB4 | 800E9B18 | 800EBBEC | 80317DB8 |
| 10 | 801B95CC | 801AE5A0 | 801B1840 | 802CBDC8 | 802CBDC8 | 802CCD70 | 800E9804 | 800EB8EC | 80318688 |
| 11 | 801B9590 | 801AE248 | 801B1804 | 802CBB28 | 802CBB28 | 802CCD3C | 800E9474 | 800EB570 | 8031836C |
| 12 | 801B65EC (0) | 801ACC84 (type==6) | 801B65EC (0) | 801B65EC (0) | 801B65EC (0) | 802CCD34 (1) | 800E8C68 (1) | 800EAD24 (1) | 8031765C (1) |
| 13 | 801B65E4 (0) | 801B65E4 (0) | 801B65E4 (0) | 802CBB20 (1) | 802CBB20 (1) | 802CBB20 (1) | 800E8C70 (1) | 800EAD2C (1) | 80317664 (1) |
| 14 | 801B65DC (1) | 801B65DC (1) | 801B65DC (1) | 801B65DC (1) | 801B65DC (1) | 801B65DC (1) | 800E8C78 (0) | 800EAD34 (0) | 801B65DC (1) |
| 15 | 801B8B24 (1) | 801ACCB0 (1) | 801AF9E4 (1) | 801B65D4 (0) | 801B65D4 (0) | 801B65D4 (0) | 801B65D4 (0) | 801B65D4 (0) | 801B65D4 (0) |
| 16 | 801B65CC (0) | 801AE240 | 801B65CC (0) | 802CBB18 (1) | 802CBB18 (1) | 802CBB18 (1) | 801B65CC (0) | 801B65CC (0) | 801B65CC (0) |
| 17 | 801B65C4 | 801AE21C | 801B65C4 | 802CBAF4 | 802CBAF4 | 802CBAF4 | 801B65C4 | 801B65C4 | 801B65C4 |
| 18–1C | defaults 801B65C0/BC/B4/B0/AC | 801AE1DC/19C/178/138/0F8 | 801B17E0/17BC/1798/1760/1728 | defaults | defaults | defaults | defaults | defaults | defaults |
| 1D | 801B65A8 (blr) | 801AE0B0 | 801B1704 | 802CBAD0 | 802CBAD0 | 802CBAD0 | blr | blr | blr |

(slots 0–9: TVehicleBase/CharInfo for every class — dtor, 8007A600, 801B6ECC, 801DCD1C, 801B6E58, 801B6DE4, 801B6DDC,
801B6DD4, 801B6D60, 8007375C.)

### 3.5 Helper tasks

- `Vehicle::VehicleTask` (0x38; ctor 0x801B6198): +0x00 name = registry name, +0x28/+0x2C shared_ptr vehicle,
  +0x30 despawn distance (init −1.0 → never; set to (range+1)*100 by `VehicleTask_SetDespawnDistance` 0x801B60BC).
  Creates `Vehicle::Effect` and runs `Vehicle_Init` 0x801B6CF8 (flags, self ref, impl owner, vslot 0xC). PROVEN.
- `Vehicle::VehicleTask::Update` 0x801B5ECC (PROVEN):
  1. vehicle vslot 0xF Update(`*vehicle+0x60` = dt).
  2. flag bit3 && no rider && despawnDist > 0 && `Vehicle_IsFarFromCameras` 0x801B6284 (camera unit 0, and unit 1
     when bss 0x8057E812 = 2-player) at ≥ despawnDist → delete task.
  3. flag bit2 && no rider && vslot 0x15: impl+0x28 += dt; **≥ 30 s** (sdata 0x805E958C) → delete; **≥ 25 s**
     (0x805E9588) → vslot 0x18 once. With a rider: vslot 0x19 if 0x1A, timer = 0.
- `Vehicle::CharaColliMove` (0x30; update fn_803361F4): displacement pos − prev and velocity = disp/dt, the same
  pattern as `Player::CharaColliMove`. STRONG.
- `Vehicle::Effect` (0x30, impl 0x1C; update 0x80350360 → 0x80350210): damage-smoke level from HP/max:
  ≥ 0.55 → 0, ≥ 0.30 → 1, else 2 (0x805FB440/0x805FB444); effect id table **0x80512C38[type][level]**
  (16 bytes/type): cars 1–3: −1/560/567, bike 755/758, Walker 210/212, AirSaucer 746/748, AirWing/AirBolt 738/741,
  batteries & GunLift none. PROVEN (numbers are particle ids, LIKELY).

---------------------------------------------------------------------------------------------------------------------

## 4. Player entry, riding and exit

### 4.1 Boarding request (PROVEN)
`UserInput_BuildButtonCommands` 0x800A0130 asks the player's proximity object (`player+0x248`, fn_80095D00) for the
nearest interactable and switches on its `CharInfo+0x50`: **0x33 → vehicle** (0x800A0E8C). Conditions: player flags
0xB, 0x1E (holding object) and 0x10 (already driving) clear; the vehicle reference is valid and
`vehicle+0x80` (rider) is empty. Then (X slot, r10 = pressed/released pair, confirmed by the curated symbol):
`GetOnVehicleCommand(vehicleRef, priority 2)` (CharCommand_Alloc(0x1C), ctor 0x8009B04C: id 0x123, **+0x14 vehicle
shared ref**). If `UserInput+0x14` (VehicleArrow) exists: arrow shown (fn_8033BA00(arrow,1)) at vehicle vslot 0xB ride
point (fn_8033B9D8). In-vehicle (flag 0x10) the function builds only ControlVehicleCommands and returns (0x800A0314).

### 4.2 GetOnVehicleCommand → DriveVehicle (PROVEN)
`GetOnVehicleCommand::Execute` 0x8009AE40: receiver cast to player; `InitializeArgsDriveVehicle` 0x8009AF4C
(id **0x1B**, +8 vehicle ref) → `Executer_ChangeBehavior(player+0x22C)`; command marked handled.
`DriveVehicle::CanEnter` 0x8007EEF0: vehicle valid and `vehicle+0x80` empty.

### 4.3 `Player::Behavior::DriveVehicle` (0x48) — fields and Enter/Update/Leave (PROVEN)

| Offset | Field | Evidence |
|---|---|---|
| +0x1C | sub-state: 0 boarding arc, 1 seated | Enter 0x8007EE38; Boarding 0x8007DE4C |
| +0x20 | bits: 0 exit request (vehicle object expired), 1 seated (input forwarding on), 2 vehicle wants per-frame rider hook | 0x8007E714, 0x8007E2E8, 0x8007E310 |
| +0x24 | time in state (s) | Update 0x8007E6FC |
| +0x28 / +0x34 | start position / rotation of the boarding arc | Enter 0x8007EE10..0x8007EE34 |
| +0x40 | vehicle reference (also copied to **player+0x258** = current vehicle) | Enter 0x8007ECC8/0x8007ECD8 |

**Enter** 0x8007EB0C: no vehicle → Idle. `TryRide(vehicle, PlayerBase+0x254 proxy 0)` 0x801B6B6C (fails → Idle;
success → vehicle+0x80 = rider, vehicle dt source = player's). vehicle+0x88 = proxy 1. If a weapon is held and
`ShadowWeapon` vslot 6 (GetWeaponKind) returns 5 (melee, `WeaponStickBase`) → ShadowWeapon vslot 0xB (ThrowWeaponIfAllowed: a held melee weapon is thrown away on boarding; kinds PROVEN in WEAPONS_AND_TARGETING.md §3.5). Flags **set 0x10, 0x31, 0x38,
0x40, 0x43; clear 0x2** (grounded); A0 bits **set 1 (jump allowed — needed for ejection) and 7 (weapon)**; vehicle
vslot 0x12 → A0.8 set, A0.7 cleared; vslot 0x13 → flag 0x3A. Motion 0xC (blend 5.0, sdata2 0x805F3630), velocity
zeroed. HUD: player record +0x18 = vehicle type (fn_8016F2BC), record+0x44/+0x48 = HP/max HP gauge (fn_8016FA50).

**Update** 0x8007E6D0 (dt): t += dt. If the vehicle weak ref expired (fn_8007E23C) → exit bit. State 0 =
`DriveVehicle_UpdateBoarding` 0x8007DDA4: seat pose from `GetSeatPose` 0x8007E3D0 (vehicle vslot 0xB ride point +
R·(0, player+0x274 = 5.0 (static init 0x800AE70C), 0); AirSaucer (type 7) also drives two rider bones from the saucer rotation); while
t < **0.5 s** (0x805F362C) the player flies along an arc (fn_8009339C, gravity −100 × up, 0x805F3634) and slerps
rotation; at 0.5 s: snap, motion 0, `StartDriving` 0x8007E268 (vehicle vslot 0x10 OnRideStart(player ref),
bit1, bit2 if vslot 0x16, vslot 0x1D(motion), flag 0x28), pad event fn_8034ED50(6, playerIndex) (LIKELY
rumble), state 1. State 1 = `SnapToSeat` 0x8007E660 every frame. Then HUD HP gauge, vslot 0x17 + motion hook if bit2,
vehicle flag bit4 = player flag 0x2A, player velocity = (pos − prevPos)/dt, up alignment, and on the exit bit:
`Detach` + `ChangeBehavior(Jump(up))`.

**OnCommand** 0x8007E8B8: 0x001 AccelerateInput and 0x015 ControlVehicle are **re-sent to the vehicle** (new
command, priority 4, vehicle vslot 2) when seated (bit1) and consumed; 0x111/0x112 (TouchLine/TouchHang) consumed.

**Leave** 0x8007EA1C: clear flags 0x10/0x31/0x38/0x43/0x40/0x3A and A0 bits 1/7/8, motion reset, `Detach`
0x8007DEE8 (vehicle vslot 0x11 OnRideEnd, `ReleaseRider` 0x801B6AB8 (clears +0x80/+0x88, dt back to 0x8057E780),
vslot 0x1D(0), player+0x258 cleared, HUD gauge 0, flag 0x28 cleared), player record vehicle type = 0.

### 4.4 Commands (PROVEN)

| Command | Built by | Payload | Handled by |
|---|---|---|---|
| 0x123 GetOnVehicle | UserInput 0x800A0F68 (X slot) | +0x14 vehicle shared ref (0x1C bytes) | Execute 0x8009AE40 → DriveVehicle |
| 0x015 ControlVehicle | UserInput 0x800A01E4..0x800A0308 (A 2/3, X 4/5, B 6/7 when slot 0x12), UserInput::Update 0x800A1E94 (7 when input is cancelled), DriveVehicle re-send 0x8007E9D4 | +0x14 u32 sub-id (0x18 bytes) | DriveVehicle::OnCommand → vehicle vslot 0xD; cars drop it (§5) |
| 0x001 AccelerateInput | UserInput (stick); DriveVehicle re-send 0x8007E97C | +0x14 Vec3 | same |
| 0x00B Ride / 0x00C Leave | ExternalControl/LineHang/SomethingHang Enter/Leave, Player/Vehicle CharaColliMove (platform standing) | – | **not vehicle boarding**: `Vehicle_DefaultOnCommand` 0x801B6C00 drops 0xB/0xC (+0x101, 0x121, 0x122, 0x12B, 0x12C); Player_HandleCommand only consumes them (0x800A7FA0) |
| 0x104 StartJump | `Vehicle_EjectRider` 0x801B63D0 (dir or default, priority 4) | +0x14 dir | player: StartJumpCommand::Execute (A0.1 set by DriveVehicle) → Jump → DriveVehicle::Leave |

Commands a vehicle does not consume are executed **immediately on the rider** (rider vslot 3 = PlayerBase::vf03 →
command vslot 1 Execute) by `Vehicle_DefaultOnCommand`; ControlVehicle/AccelerateInput/Damage use the default
`CharCommand::vf01` (blr), so the forward is a no-op for them (no ping-pong). PROVEN.

### 4.5 Exit conditions (PROVEN unless noted)
1. The vehicle sends a `StartJumpCommand` to the rider (via `Vehicle_EjectRider`, or its own copy in the batteries /
   GunLift). **X is the exit button everywhere**: car X-press below |speed| 25 (0x801BCCF8), Walker X in stand/land/
   grounded-damage states (0x801AEB70), AirSaucer X when not on a path (0x801B2660), AirWing/AirBolt X (0x802CC568),
   batteries X (0x800E90E0 / 0x800EB160), GunLift X (0x803185FC). Also: destruction (cars, Walker, AirSaucer, birds,
   batteries), bird path end (0x802CAD7C), sub-id 1 (no sender found).
2. The vehicle object is deleted (VehicleTask killed) → DriveVehicle bit0 → Detach + Jump (0x8007E860).
3. Any other behaviour change (e.g. Damage/Dead via player commands) runs DriveVehicle::Leave → Detach.

---------------------------------------------------------------------------------------------------------------------

## 5. Cars — `Vehicle::TVehicleCar` + `Vehicle::TVehicleCarMove` (types 1–4)

### 5.1 Objects (PROVEN)
`TVehicleCar` (0x174): +0x9C first-update done, +0xA0/+0xA4 model/motion handles from the resource row (0x801B94F0/
0x801B950C), +0x168/+0x16C shared_ptr `TVehicleCarMove`, +0x170 = fn_8040FAE0(5) (LIKELY random 0..4).
`TVehicleCarMove` (0x2F0, ctor 0x801BF9B0(this, car, carIndex = type−1 via 0x801BA8F8)): +0x000 frame counter;
**+0x004..+0x11F current state record** and **+0x120..+0x23B previous-frame copy** (2 × 0x11C built by fn_803A1500 with
element ctor 0x801BFAAC; copied every update by fn_801BB16C(+0x120, +0x4) from `TVehicleCarMove_Update` → 0x801BB748).
Current record fields (CarMove offsets): +0x004 state (1 unmanned, 2 driven; SetState 0x801BF1F8), +0x008 pos, +0x014
rot (+0x018 yaw), +0x02C velocity, +0x038 signed speed, +0x044 invulnerability counter, +0x058 steer, +0x05C A pressed,
+0x05D exit request, +0x05E A held, +0x05F reverse input, +0x060 X held, +0x064/+0x068/+0x06C smoothed throttle /
reverse / brake levels, +0x070/+0x074/+0x078 spin-turn timers, +0x07C spin gauge, +0x084 (sign selects motion 3/4),
axle records at +0x0B8 / +0x0EC (contact bit +0x24, normal +0x18). Outside the records: +0x240 yaw rate, +0x244 last
velocity, +0x254 last ram damage, +0x25C car index, +0x260 pad slot, +0x264 owner car, +0x268 shared_ptr
`TVehicleCamera`, +0x280 CharaColli body, +0x284 ride point, +0x29C motion state (1 fast > ¼·top speed, 2 reverse
< −60, 3/4 spin-turn, 5 special, 6 normal), +0x2A8 camera target, +0x2B4 spin finished, +0x2B5 start-up flag.
Update chain (`TVehicleCar::Update` 0x801B9688): first frame `FirstUpdate` 0x801BF8CC (SetState 1 + Reset) →
`TVehicleCarMove_Update` 0x801BEC50(dt) → 0x801BE348 → base SetTransform(+0x08, +0x14).
`OnRideStart` 0x801B95CC = SetOccupied + SetState(2, 0): pad slot = GamePeripheral(0x80576DF4)+0x11C[0] (**player 0's
pad is hard-wired**, r5 = 0 @0x801B95EC), new TVehicleCamera(target +0x2A8, rot +0x14, offset row+0x88), collision
body with attack callbacks. `OnRideEnd` 0x801B9590 = SetUnoccupied + SetState(1).

### 5.2 Input — cars read the pad themselves (PROVEN, `TVehicleCarMove_ReadPad` 0x801BDBA0)
Logical pad record (0x28 bytes at 0x80576DF4+0x28+slot*0x28: +0 held, +8 pressed, +0x10 stick X, +0x14 stick Y;
game bits A = 0x1, X = 0x8):
| Input | Effect | pc |
|---|---|---|
| stick X | steer +0x58 → stickX + (old − stickX)·8/9 (8 @0x805E964C) | 0x801BDBC0 |
| A held | throttle +0x64 → 1 with k = 18/19 (0x805E9650), released → 0 with k = 6/7 (0x805E9654) | 0x801BDBFC |
| A pressed | +0x5C (cancels spin, 0x801BD9C0) | 0x801BDC7C |
| X held | brake +0x6C → 1 (18) / 0 (6) | 0x801BDCC4 |
| stick Y > 0.45 (0x805F6B78) | reverse +0x68 → 1 (18) / 0 (6) | 0x801BDD48 |
| **X pressed and \|speed\| < 25** (0x805E963C) | +0x5D → `TVehicleCarMove_ExitCar` 0x801BA948 = EjectRider + SetState(1) + unmanned collision body | 0x801BDDD8, 0x801BCCF8 |

The rider's ControlVehicle/AccelerateInput commands reach the car but `TVehicleCarMove_OnCommand` 0x801BE124 only
handles 3 SetAngle, 4 ClrVelocity, 5 SetVelocity (×0.5); 7 Damage is not consumed (→ rider no-op). B while driving =
the player's own weapon (StartUseWeapon, A0.7 stays set for cars). No car-owned weapon: no call to any Weapon/Bullet
class in 0x801B8B00–0x801C1100 (grep) — the "Armed Car" differs only by its parameter row. PROVEN (code) / STRONG (conclusion).

### 5.3 Car parameter table 0x8053CBF8 (4 rows × 0xAC, row = type−1)

Values (Jeep / Open / GUN / BIKE); consumer pcs from `agent_weapon_veh_q.py tbl 8053CBF8 AC …`.
| Row off | Jeep | Open | GUN | BIKE | consumer pc | meaning | confidence |
|---|---|---|---|---|---|---|---|
| +0x00 | "Jeep" | "Open" | "GUN" | "BIKE" | – | row name | PROVEN |
| +0x04 | 6 | 6 | 10 | 4 | 0x801BBA78 | ram damage base (int) | STRONG |
| +0x08 | 70 | 50 | 200 | 40 | 0x801BB0F4/0x801BB114 (Reset: SetMaxHP+SetHP), 0x801BB460 | **max HP** | PROVEN |
| +0x0C | 3.0 | 2.4 | 3.0 | 1.5 | 0x801BD240 (normal), 0x801BCE48 (spin) | yaw rate at/above +0x1C speed (×steer/30) | STRONG |
| +0x10 | 24 | 19.2 | 24 | 14 | 0x801BD244 | yaw rate at speed 0 (lerp) | STRONG |
| +0x14 | −0.1 | −0.1 | −0.1 | −0.1 | 0x801BC864 | UNKNOWN | UNKNOWN |
| +0x18 | 450 | 600 | 450 | 700 | 0x801BD800, 0x801BBA98, 0x801BD168 | **top speed** for throttle; ram-damage speed scale | STRONG |
| +0x1C | 180 | 180 | 135 | 150 | 0x801BD228 | speed where yaw rate reaches +0x0C | STRONG |
| +0x20 | 360 | 450 | 230 | 350 | 0x801BD814 | forward acceleration (× throttle) | STRONG |
| +0x24 | 180 | 225 | 115 | 200 | 0x801BD7B0 | reverse acceleration (× reverse level, below speed 25) | STRONG |
| +0x28 / +0x2C | 1 / 1.5 | same | same | 1 / 1.3 | 0x801BD354 / 0x801BD33C | axle velocity-approach rates (grounded) | LIKELY |
| +0x30 | 9 | 9 | 9 | 9 | 0x801BD388 | extra grip × brake level (X) | STRONG |
| +0x34 | −127 | −127 | −127 | −127 | 0x801BD85C/0x801BD8B4 | **gravity** (accel (0, v, 0) every frame) | STRONG |
| +0x38 | −9 | −9 | −9 | −9 | 0x801BC50C/0x801BC53C | UNKNOWN | UNKNOWN |
| +0x54 | 1 | 1 | 1 | 1 | 0x801BC8D8 | UNKNOWN | UNKNOWN |
| +0x58 | 1.52 | 1.3 | 1.52 | 1.52 | 0x801BCE50 | spin-turn yaw multiplier (yaw −= steer·+0x0C·+0x58·dt·+0xA8) | STRONG |
| +0x5C | 360 | 360 | 360 | 12 | 0x801BCEA8 | spin-turn forward push × throttle | STRONG |
| +0x64 | 150 | 180 | 200 | 150 | 0x801BDA98/0x801BDB48 | spin-turn max speed | LIKELY |
| +0x68 | 0.2 | 0.2 | 0.2 | 0.2 | 0x801BDA48 | spin arm time (s) | LIKELY |
| +0x6C | 60 | 60 | 60 | 60 | 0x801BD9EC | spin settles while \|CarMove+0x84\| < this (or speed < 25) | LIKELY |
| +0x70 | 1 | 1 | 1 | 0.5 | 0x801BDA18 | settle time that ends the spin (s) | LIKELY |
| +0x74 / +0x78 | 0.03 / 0.075 | same | same | 0.04 / 0.1 | 0x801BDA80 / 0x801BD9A4.. | spin gauge max / rate | LIKELY |
| +0x7C | 1.75 | 1.75 | 1.75 | 1.75 | 0x801BF304, 0x801BAE7C | collision-body size factor | LIKELY |
| +0x80 | →(0,16,0) | (0,16,0) | (0,18,0) | (0,16,0) | – | Vec3 (bss 0x80582140.., static init 0x801BFD84) | UNKNOWN |
| +0x84 | →(6,5,15) | (6,5,15) | (8,8,22) | (3,5,15) | 0x801BB4E4.., 0x801BACCC | Vec3 used by damage/collision code | UNKNOWN |
| +0x88 | →(0,2,65) | (0,2,60) | (0,10,75) | (0,2,45) | 0x801BF294 | **camera offset** (TVehicleCamera+0x20, length → +0x2C) | STRONG |
| +0x8C..+0xA0 | 13,12,9.5,−1.5,−3e-4,0.1 | 13,12,9.5,… | 20,12,10.5,… | 13,8,5.5,7,0,0.6 | 0x801BEAB0, 0x801BAB74.., 0x801BC1B8.. | UNKNOWN (body/suspension) | UNKNOWN |
| +0xA4 | 0 → **45°** (0.785 rad) | 45° | 45° | **60°** | 0x801B9F10 | max drivable slope (axle grounded if n·up ≥ cos) — value written by static init 0x801BFED0..0x801BFEDC | PROVEN |
| +0xA8 | 0.007 | 0.007 | 0.007 | 0.01 | 0x801BCE54 | spin-turn yaw step factor | STRONG |

**Drive step** (`TVehicleCarMove_Drive` 0x801BCCAC, driven and unmanned): exit check; body frame R from rotation;
`fn_801BD8FC` spin-turn timers. If the previous-frame spin timer (+0x18C = copy of +0x070) > 0 → **spin-turn mode**:
yaw −= steer·row+0x0C·row+0x58·dt·row+0xA8, A adds forward·row+0x5C·throttle plus up·row+0x34, axle grip
(0x801BD294 ×2), motion 3/4. Otherwise **normal mode**: per-axle acceleration `TVehicleCarMove_GetAxleAccel`
0x801BD694 = A: forward·row+0x20·throttle while −25 < speed < row+0x18; reverse input: −forward·row+0x24·level while
speed < 25; always + (0, row+0x34, 0) gravity; projected on the ground plane when the axle is on drivable ground
(slope ≤ row+0xA4); yaw rate `TVehicleCarMove_GetYawRate` 0x801BD1F8 = −steer·lerp(row+0x10 → row+0x0C over
|v| ∈ [0, row+0x1C])/30; axle integration 0x801BD468 ×2; motion 1/2/6. Spin-turn arming (`fn_801BD8FC`): after

coasting ≥ row+0x68 s at speed ≤ row+0x64 without throttle, holding A with |stick X| > 0.4 (0x805F6B74) starts it; it ends
after row+0x70 s, on A press, or when the gauge/speed conditions fail (+0x2B4 = 1). (branch structure PROVEN, gameplay
name "spin-turn" LIKELY).

Other car constants: `ram damage = int(row+4 · 2|v| / row+0x18) + (|v| > 0.2·25 ? 1 : 0)` (`TVehicleCarMove_GetRamDamage`
0x801BBA38, 2.0 @0x805F6B48, 0.2 @0x805F6B4C; only in state 2); motion state thresholds −60 (0x805F6B5C) and
0.25·row+0x18 (0x805F6B60); yaw rate scale 1/30 (0x805F6B64); SetVelocity scale 0.5 (0x805E9648).

### 5.4 Car damage and destruction (PROVEN)
Collision body (`CarMove+0x280`) callbacks registered in SetState(2) (PMFs 0x8053CEA8/B4/C0):
- `OnDamaged` 0x801BB994 (being hit): unless invulnerable (+0x44 ≠ 0 → result 5), HP −= hit+0x50; alive → result 1 +
  SE 0x202F, dead → result 3, HP = 0; invulnerability counter **+0x44 = 60**, decremented once per update in
  0x801BBC24, model hidden on odd counts (blink, 0x801BC5C4).
- `OnAttack` 0x801BB8F4 (car hits something): hit+0x50 = ram damage (+0x254).
- `OnAttackResult` 0x801BB858: if the victim answered 1 (damaged but alive), the car loses the same damage (clamped 0).
- `DamageCommand` (id 7) has **no effect** on cars (not consumed, rider no-op). Walker handles it (§6).
- Damage level for effects: `2 − min(int(HP/max/⅓), 2)` → +0x3C (0x801BB45C..0x801BB4A8).
- **Death** (`TVehicleCarMove_Update` 0x801BEE10): HP ≤ 0 → SE 0x202E, explosion at the car matrix (Effect task
  fn_8034FFD8), `Vehicle_EjectRider(car, 0)`, `Vehicle_KillTask`.
- Repair: `Player::GetItemCommand` item 5 (0x8009B970): if the player's current vehicle (+0x258) is alive →
  `Vehicle_RestoreHP` + SE 0xE020. PROVEN.

---------------------------------------------------------------------------------------------------------------------

## 6. Other vehicles (Walker, AirSaucer, BirdBase/AirWing/AirBolt, batteries, GunLift)

All of them handle rider input in their vslot 0xD OnCommand (ControlVehicleCommand sub-ids from §4.4); Walker,
AirSaucer and BirdBase fall back to `Vehicle_DefaultOnCommand` 0x801B6C00, the batteries and GunLift drop unhandled
commands. Sub-id 1 = eject in every class that handles it, but **no sender of sub-id 1 was found** (only 2..7 are built).
Sections 6.1–6.5 were traced with three read-only sub-investigations; the key constants were re-read by hand.

### 6.1 `Vehicle::Walker` (type 5 "Walker", type 6 "WalkerCannon")

Construction (PROVEN): ctor 0x801AF738: +0x9C cannon (0), +0xA0 model (0), +0xA4 CharaColli wrapper (fn_801B525C →
new(0x38)), +0xA8 ground mover new(0x28) fn_80334558 (fn_803342F4 = grounded), +0xAC ground ray sensor (as
Player+0x278), +0xD4 state, +0xD8 sub-state, +0xDC state timer, +0xE0 move accel, +0xE4/+0xE8 drags, +0xEC up,
+0xF8 stick accumulator, +0x104 bits (0 gravity, 2 jump-hold, 3 hover, **4 exit request (X)**, 5 A on ground, 6 A in
air, 7 teleported), +0x108 invulnerability timer, +0x10C hit history. Setup vslot 0xC 0x801AF2F0: **max HP = HP = 8.0**
(0x805F69B4), or **40.0** (0x805F69C0) when the two-player byte 0x8057E812 is set; model via fn_801ACF34 and, **only for
type 6**, the cannon turret +0x9C (new(4) fn_80348820 → impl new(0x1C4) fn_80348E94; no RTTI). Ride point = model node 1
(0x801AF580). OnRideStart 0x801AE5A0: state 0, collision callbacks, **SE 0x204A**, cannon enabled; OnRideEnd 0x801AE248.

| Input (OnCommand 0x801AEE10) | Effect | pc |
|---|---|---|
| AccelerateInput | +0xF8 += stick | 0x801AEE4C |
| **A (2)** | grounded → bit5 (jump), airborne → bit6 (hover) | 0x801AEF68 |
| A release (3) | clear bits 2, 3 (end jump-hold / hover) | 0x801AEF98 |
| **X (4)** | bit4; in states 0 (stand), 3 (land) and 4 (damage, if grounded) → `Vehicle_EjectRider` | 0x801AEFB4, 0x801AEB70 |
| **B (6/7)** | cannon trigger on/off (fn_80348714 / fn_803486E8) — WalkerCannon only (+0x9C null for Walker) | 0x801AEFD0 |
| sub 1 | EjectRider | 0x801AEF54 |
| 7 DamageCommand | `Walker_ApplyDamage` 0x801ADB7C(power, pos) | 0x801AEF0C |

States (+0xD4; Update 0x801AE994): 0 stand/walk 0x801AD93C, 1 jump 0x801AD75C, 2 fall 0x801AD6D8, 3 land 0x801AD64C,
4 damage 0x801AD5C8, 5 hover 0x801AD4F8 (A held in the air).

| value | address | consumer pc | meaning | confidence |
|---|---|---|---|---|
| −4000 / −475 | 0x805F6970 / 0x805F6974 | 0x801AD23C / 0x801AD244 | gravity along up (grounded / airborne) | PROVEN |
| 2400 | 0x805F6994 | 0x801AD978 | move acceleration | PROVEN |
| 25 / 2.25 | 0x805F6988 / 0x805F698C | 0x801AD31C | horizontal / vertical drag (top speed ≈ 2400/25 = 96, derived) | PROVEN / LIKELY (derived) |
| 400 / 200 | 0x805F6980 / 0x805F6998 | 0x801AD804 / 0x801AD80C | jump impulse Walker / WalkerCannon | PROVEN |
| 500 / 250, 1.25 s | 0x805F699C / 0x805F69A0, 0x805F69A4 | 0x801AD864.., 0x801AD88C | jump-hold thrust, max hold time | PROVEN |
| 400, drags 300 / 7 | 0x805F6980, 0x805F6968 / 0x805F6984 | 0x801AD2EC / 0x801AD308 | hover up-thrust while falling | PROVEN |
| 125°/s | 0x805F6960 | 0x801AD098 | yaw turn rate | PROVEN |
| 30° | 0x805F6978 | 0x801AD260 | ground normal accepted as up within 30° | PROVEN |
| 300, 0.05 | 0x805F6968, 0x805F696C | 0x801AD144 | SetLookAtPositionCommand to the rider (point 300 ahead) | PROVEN |
| 200 | 0x805F6998 | 0x801AEC08 | length of the downward ground ray (not gravity) | STRONG |
| 7.0 / 10.0 | sbss 0x805F0824 / 0x805F0828 (static init 0x801AF9B0) | 0x801ADEF0.. | collision-shape radius terms | LIKELY |

**WalkerCannon cannon** (PROVEN): fire loop fn_80348BE0: while the trigger bit is set and the cooldown is 0 → reload
**0.25 s** (0x805FB388), 4 muzzles in rotation, bullet table index **4** → `Weapon::Bullet::BulletGun` (new(0x60),
6.0 @0x805F2E5C, effect 0xE6), rider rumble 2.
**Damage** (PROVEN): `Walker_ApplyDamage` 0x801ADB7C: power < 1 ignored; hit effect 0x17; if +0x108 ≤ 0 → state 4,
AddHP(−power), invulnerability **2.0 s** (0x805F69AC), SE 0x2052, rumble 4. CharaColli callback 0x801AD9BC: blocks hits
from above while falling (result 5), else damage +0x50 → result 3 dead / 1 alive. Stomp attack 0x801ADAA0: damage 4.0
(0x805F69A8) to targets below while falling. **Destroyed** (Update 0x801AECD0): SE 0x2053, explosion effect,
WalkerCannon drops `ScatteringWeapon` weapon **4** (AntiAircraftRifle pickup, per the battery trace) at pos+(0,20,0),
EjectRider, Vehicle_KillTask.

### 6.2 `Vehicle::AirSaucer` (type 7)

Construction (PROVEN): ctor 0x801B2C64: +0x9C ground mover, +0xA0 path follower (fn_803353A0), +0xCC model, +0xD0
CharaColli wrapper, +0xD4 attack collision, +0x10C stick accumulator, +0x118.. Euler, +0x144 bits (1/2 on a path,
4 jump request, 5 boost), +0x154 speed factor, +0x15C invulnerability, +0x160 loop SE, +0x17C state. Setup 0x801B296C:
**max HP = HP = 10.0** (0x805F6A00). Vslot 0x12 = 0: **no own weapon — B uses the rider's hand weapon**. OnRideStart /
End start/stop loop **SE 0x2043**. The rider pose is special-cased for type 7 in `DriveVehicle_GetSeatPose` 0x8007E4AC.

| Input (OnCommand 0x801B2370, table 0x8053C698) | Effect |
|---|---|
| AccelerateInput | +0x10C += stick (yaw rate 250°/s, 0x805F6A2C) |
| **A (2)** | if the contacted object is CharInfo kind 0x37 → `UseCommand` to it; else jump request (bit4) |
| **X (4)** | on a path → boost (+50, cooldown 0.5 s); otherwise **eject** (0x801B2660) |
| sub 1 | eject (0x801B2504) |
| 3, 5, 6, 7 | unhandled → Vehicle_DefaultOnCommand (B edges become no-ops on the rider) |
| 7 DamageCommand | `AirSaucer_ApplyDamage` 0x801AFDBC |

States (+0x17C, table 0x8053C67C): 0 unridden (vel ×0.7 per grounded frame), 1 ridden, 2 take-off (+100 along the ground
normal, SE 0x2044), 3 airborne (second jump +100, SE 0x2045), 4 airborne without jump, 5/6 path-follow (jump off → 2).

| value | address | consumer pc | meaning | confidence |
|---|---|---|---|---|
| 270 / −1300 / 0.98 / 0.75 | 0x805F6A4C..0x805F6A58 | 0x801B0F54 | grounded: input accel / gravity / horizontal / vertical drag | PROVEN |
| 220 / −320 / 3 / 1 | 0x805F6A5C..0x805F6A64, 0x805F69D4 | 0x801B0F68 | airborne: same four | PROVEN |
| 100 | 0x805F69EC | 0x801B1AB4, 0x801B1B1C | jump impulse (both jumps) | PROVEN |
| 50° | 0x805F6A70 | 0x801B1E0C | max body tilt | STRONG |
| 40 / 100, cap 360, min 10 | 0x805F69F8 / 0x805F69EC, 0x805F69FC, 0x805F6A00 | 0x801B0474.. | path speed gain downhill / loss uphill | PROVEN |
| 7.25 / 7.0 / 8.0 | sbss 0x805F084C / 0x805F0850 / 0x805F0854 (static init 0x801B3014) | 0x801B05D8 / 0x801B0814 | path hover height / attach radius | LIKELY |

**Damage** (PROVEN): `AirSaucer_ApplyDamage` 0x801AFDBC: dmg ≥ 1 and not invulnerable → HP −= floor(dmg),
invulnerability **2 s**, SE 0x2048, rumble 4; 0.3 < dmg < 1 → rumble 1. Ram attack 0x801AFCE0 =
floor(min(speed/100, 2)·facing); wall self-damage 0x801AFB58 = factor·min(speed/200, 3)·facing. **Destroyed** (Update
0x801B1FDC): SE 0x2049, explosion effect, EjectRider (0x801B2038), Vehicle_KillTask; no weapon drop.

### 6.3 `Vehicle::BirdBase::BirdBase` → AirWing (type 8) / AirBolt (type 9)

Construction (PROVEN): `BirdBase::BirdBase(this, type, params*)` 0x802CC640 → TVehicleBase ctor; +0xF4 params,
+0x104 path follower (fn_801B480C, new(0x7C)), +0x108 collider set, +0x10C body collider, +0xA0 model, +0xFC state,
+0xF8 sub-state, +0x118 invulnerability timer. AirWing factory passes params **0x8053C6F0**, `AirBolt::AirBolt`
0x802CD0C4 passes **0x80559060** (identical values; +0 = 15 and +4 = 14 are written by the static inits
0x801B3404 / 0x802CD134). AirBolt adds +0x120 fire bits, +0x124 7 muzzle matrices, +0x274 shared_ptr
`Weapon::AirBolt::AirBolt`, +0x27C fire state, +0x280 cooldown. Models: new(8) AirWingModel / AirBoltModel
(0x801B311C / 0x802CCC8C) via `BirdModelBase` 0x802C96B0.

| Slot | AirWing / AirBolt | Role |
|---|---|---|
| 0xC | 0x801B31E4 / 0x802CCE90 | **max HP = HP = 14.0** (0x805F6A90) / **20.0** (0x805F9E98); model; body collider 0x802CB2B8 |
| 0xA | 0x802CC278 | SetTransform + model transform + previous pos |
| 0xB | 0x801B3280 / 0x802CCF2C | ride point = model node 0 |
| 0xF | 0x802CBF48 (AirBolt 0x802CCDB4 adds the weapon) | Update: states 0 idle, 1 boarding (camera, attach path, → 2 when < 10 from the path), 2 flying (0x802CB768), 3 abandoned (speed 500), 4 dying (kill VehicleTask) |
| 0x10 / 0x11 | 0x802CBDC8 / 0x802CBB28 (AirBolt 0x802CCD70 / 0x802CCD3C create/release the weapon, 0x802CC958) | ride start: state 1, ride collider; ride end: state 3 |

Flight (PROVEN code, per-field confidence from the sub-report): the bird follows a path (`+0x104`) with a local offset;
stick (AccelerateInput, stored at +0xE0) gives yaw `+= dt·(−x·params+0x24)` and pitch `+= dt·(z·params+0x20)`, clamped
to ±params+0x2C / ±params+0x28 degrees; A (sub 2/3) sets/clears **boost** (+0x9C bit2): speed ramps between
params+0x0C and +0x10 in params+0x14 / params+0x18 s; X (sub 4) ejects (0x802CC568); end of a non-looping path ejects
(0x802CAD7C); no input for params+0x58 s far from the path enables auto-return.

| value | params off (0x80559060 / 0x8053C6F0) | consumer pc | meaning | confidence |
|---|---|---|---|---|
| 15 | +0x00 | 0x802CC13C | collision sweep radius | LIKELY |
| 14 | +0x04 | 0x802CBE44 | ride-collider radius | LIKELY |
| 500 | +0x0C | 0x802CB7D0 | cruise (min) speed | PROVEN |
| 1900 | +0x10 | 0x802CB7A4 | boost (max) speed | PROVEN |
| 0.125 / 2 | +0x14 / +0x18 | 0x802CB7AC / 0x802CB7D8 | accelerate / decelerate time (s) | PROVEN |
| 1 / 2 | +0x20 / +0x24 | 0x802C9F58 / 0x802C9F30 | pitch / yaw rate (rad/s) | STRONG |
| 30 / 60 | +0x28 / +0x2C | 0x802CA088 / 0x802CA078 | pitch / yaw limit (deg) | PROVEN |
| 10 / 130 | +0x34 / +0x38 | 0x802CBA58 / 0x802CAC40 | near-path distance / max offset from path | PROVEN |
| 100, 2 | +0x40, +0x44 | 0x802CB0DC, 0x802CB0EC | ram damage = floor(min(\|v\|/100, 2)·facing) | PROVEN |
| 200, 3, 2 | +0x48, +0x4C, +0x50 | 0x802CAFCC, 0x802CAFDC, 0x802CC378 | impact self-damage scale / cap / multiplier | PROVEN |
| 2 | +0x54 | 0x802CB200 | invulnerability after a hit (s) | PROVEN |
| 1 | +0x58 | 0x802CB860 | idle time before auto-return (s) | PROVEN |

**AirBolt weapon** (PROVEN unless noted): sub 6/7 (B press/release) set/clear +0x120 bit0 (0x802CCDF8);
`Weapon::AirBolt::AirBolt` 0x802CD610 = `WeaponLockonBase(this, owner = vehicle+0x88 rider proxy, 6, 0x44)` → internal
weapon index **0x44**, **max 6 locks**; lock search radius **2000** (0x802CD470) around muzzle matrix 6; lock SE 0x2020
(vf0A 0x802CD384, STRONG); fire (vf0B 0x802CD3B4) spawns **`Weapon::Bullet::Homing`** (new(0x6C), speed/param 2000 via
0x8051EEAC → fn_80056DA4), muzzle SE 0x201E, **0.25 s between missiles** (0x805F9EB0); fire FSM 0x802CCAE0: hold B
= lock, release = fire all locks, 0.25 s cooldown (0x805F3098). Generic lock interval 0.1 s (0x805F30A0).

**Damage** (PROVEN): ride collider callback 0x802CAEE8 → `BirdBase_ApplyDamage` 0x802CB16C(power): power ≥ 1 and not
invulnerable → AddHP(−floor(power)), invulnerability 2 s, SE 0x205A, pad code 4; 0.3 ≤ power < 1 → pad code 1 only;
`DamageCommand` (id 7) also → 0x802CB16C. Ram callback 0x802CB0AC; recoil 0x802CB050. HP ≤ 0 in flight (0x802CB768):
SE 0x205B, EjectRider (0x802CB91C), state 4 → VehicleTask killed.

### 6.4 `Vehicle::BatteryArmy` (type 10) / `Vehicle::BatteryBlackKiller` (type 11) — stationary gun turrets

Same design, BlackKiller fields shifted by −4 (PROVEN from the sub-report's disassembly, key values re-checked).
ctor 0x800EAABC (Army): +0x1C4 state, +0x1C8 bits (bit0 fire), +0x1CC fire timer, +0x1D0 invulnerability, +0x1B8 pitch,
+0x1BC yaw, +0x1DC CameraModeUser, +0x1E4 camera handle, +0x1EC reticle effect (0xCC; BlackKiller 0x17B); CharInfo
+0x5C = 4. Setup (vslot 0xC 0x800EA8B4 / 0x800EC880): **max HP = HP = 10.0** (0x805F4548 / 0x805F4584), model
resource 0xC3 / 0xC4, damage PMF {0,-1,0x800E9BEC} (0x80525948) / {0,-1,0x800EBCC0} (0x80525A28). Ride point (vslot 0xB)
= turret node 0x7D3. Ride start (0x800E9804 / 0x800EB8EC) builds a first-person camera: eye = node 0x7E4 (+0.5 y for
Army), target = muzzle + 200·forward (Army) / 55·forward (BlackKiller, 0x805F4578).

| Input (OnCommand 0x800EA15C / 0x800EC13C) | Effect |
|---|---|
| AccelerateInput | pitch += 20·dt·stick.z, yaw −= 40·dt·stick.x (deg); pitch clamped to [−20°, 60°], yaw free; loop SE 0x1013 / 0x1016 |
| sub 1, **sub 4 (X)** | eject: own `StartJumpCommand` to the rider (0x800E90E0 no direction / 0x800EB160 dir = Ryaw·(0,1,1)) — not Vehicle_EjectRider |
| **sub 6 (B press)** / 7 | set / clear fire bit (BlackKiller: 7 is a no-op) |
| 2, 3, 5 | ignored; unhandled commands are dropped (no Vehicle_DefaultOnCommand) |

Firing (PROVEN): Army 0x800E9348 while B held every **0.2 s** (0x805F4558): muzzle node 0x7D0, bullet type **0x1C** →
`Weapon::Bullet::BulletGun(desc, 0xE6, 2)` (power 6.0, range 2000, muzzle speed 500) via table 0x8051E840; BlackKiller
0x800EB444 one shot per press, **1.0 s** cooldown (0x805F458C), bullet type **0x1D** → BulletGun(desc, 0x130, 3) (power 8.0;
its impact spawns a CannonBomb type 6: power 4, radius 40). f1 = damage is PROVEN (BulletGun ctor → `Bullet_CreateCharaColliAttack`
0x80056850, level 5; WEAPONS_AND_TARGETING.md §2.3). Registry rows 28 "GunBattery" / 29 "BKBattery" are these guns.

Damage callbacks 0x800E9BEC / 0x800EBCC0: only in state 1 and not invulnerable (else result 5); `AddHP(−hit+0x50)`; HP ≤ 0
→ `Score_Award(attacker, 0x12 / 0x13)`, result 3; otherwise (damage > 0.9 for Army / > 0 for BlackKiller) result 1,
invulnerability 2.0 s, camera shake + hit flash (LIKELY). Next Update (0x800E8EDC / 0x800EAF8C): HP ≤ 0 → eject,
state 2 → debris (GadgetCrash3D, resource 0xC / 0x8), effect 0xA9 / 0xB9, **ScatteringWeapon drop 4 (AntiAircraftRifle)
/ 0xF (BlackBarrel)**, Vehicle_KillTask. No SE and no respawn.

### 6.5 `Vehicle::GunLift` (types 12 "GunLiftExpress" / 13 "GunLiftLocal") — rail lift with a gun

ctor 0x80319B14(this, variant): +0x9C variant (0 Express, 1 Local), +0xA0 = &0x8050D878[variant] (4 float* params),
+0xA4 path follower, +0x114/+0x11C/+0x124 body/turret/gun models, +0x154 CameraModeUser, +0x15C bits (0 fire, 2
accelerate), +0x160 pitch, +0x164 yaw, +0x168 speed, +0x16C fire timer. Setup 0x80319098: SetHP(1.0) only — **the lift
cannot be destroyed** (no GetHP/AddHP/kill in the TU, PROVEN by the sub-agent's region scan).
Input (OnCommand 0x803178F4, jump table 0x8055D238): stick → pitch/yaw ±1.5° per command (no dt), pitch [−10°, 60°];
**A (2/3) = accelerate**, **X (4) and sub 1 = eject** (own StartJumpCommand, 0x803185FC), **B (6/7) = fire**.
Movement (Update 0x80317DB8, only while occupied): A held → speed += p1·dt up to p0; else decays to cruise p2 (−p1) or
rises to it (+p3); stop only at the path end (SE 0x7636).

| value (Express / Local) | address | consumer pc | meaning | confidence |
|---|---|---|---|---|
| 400 / 200 | 0x805FAAF4 / 0x805FAAE4 | 0x80317E38 | p0 max speed with A | PROVEN |
| 200 / 100 | 0x805FAAF0 / 0x805FAAE0 | 0x80317E24, 0x80317E6C | p1 A acceleration / deceleration | PROVEN |
| 100 / 50 | 0x805FAAEC / 0x805FAADC | 0x80317E50 | p2 cruise speed | PROVEN |
| 50 / 50 | 0x805FAAE8 / 0x805FAAD8 | 0x80317E98 | p3 acceleration to cruise | PROVEN |
| 0.0625 s | 0x805FAB20 | 0x803182B0 | fire interval while B held (bullet type 0x1C, SE 0x200B) | PROVEN |
| (0, 2.92, −55.491) | bss 0x80586000 (static init 0x80319DBC) | 0x803180FC | muzzle offset in gun frame | PROVEN |
| (0, 13.039, 0.3) | bss 0x8058600C | 0x8031785C | camera eye offset | PROVEN |

Hits on turret/gun (PMF {0,-1,0x80317744}) while occupied: result 1 and a **`DamageCommand(1.0, (0,1,0))` is forwarded to
the rider** (the player takes the damage); unoccupied: result 5.

---------------------------------------------------------------------------------------------------------------------

## 7. HP / destruction / respawn summary

| Vehicle (type) | Max HP | Where set (pc / constant) | Damage in | Invulnerability | Death |
|---|---:|---|---|---|---|
| Normal Car / Open / Armed / Bike (1–4) | 70 / 50 / 200 / 40 | Reset 0x801BB110, row+8 of 0x8053CBF8 | CharaColli 0x801BB994 (+ recoil 0x801BB858); DamageCommand ignored | 60 update ticks (blink) | SE 0x202E, explosion, eject, kill (0x801BEE10) |
| Walker (5) / WalkerCannon (6) | 8 (40 in 2P) | 0x801AF31C.. / 0x805F69B4, 0x805F69C0 | CharaColli 0x801AD9BC, DamageCommand → 0x801ADB7C | 2.0 s | SE 0x2053, explosion, (6: weapon drop 4), eject, kill |
| AirSaucer (7) | 10 | 0x801B2984 / 0x805F6A00 | CharaColli 0x801AFAF0, wall impacts, DamageCommand → 0x801AFDBC | 2 s | SE 0x2049, explosion, eject, kill |
| AirWing (8) / AirBolt (9) | 14 / 20 | 0x801B31FC / 0x802CCEA8 (0x805F6A90 / 0x805F9E98) | ride collider 0x802CAEE8, impacts, DamageCommand → 0x802CB16C | 2 s | SE 0x205B, eject, state 4 → kill |
| BatteryArmy (10) / BlackKiller (11) | 10 / 10 | 0x800EA8B4 / 0x800EC880 | CharaColli 0x800E9BEC / 0x800EBCC0 (Score_Award 0x12 / 0x13 on kill) | 2 s | debris, drop 4 / 0xF, eject, kill; no SE |
| GunLift (12, 13) | – (HP 1, never read) | 0x803190D8 | hits forwarded to the rider as DamageCommand(1.0) | – | indestructible |

The HUD vehicle gauge is the player record +0x44 (HP) / +0x48 (max), written by DriveVehicle (§4.3).

Respawn: a destroyed or despawned vehicle is not re-created by itself. The SET object is marked respawnable
(`SetSlot_MarkRespawnableAndDetach`) when it gives the vehicle away or loses it (0x8011E760/0x8011E858), so a new
SetVehicleTask (and vehicle) is created the next time the slot is spawned by the SET scanner. LIKELY (flag 0x10000
semantics taken from the enemy trace; the rescan path was not re-traced for vehicles).

---------------------------------------------------------------------------------------------------------------------

## 8. Open items

- **ControlVehicleCommand sub-id 1** (eject in Walker, AirSaucer, BirdBase, batteries, GunLift): no constructor call with 1
  was found (all 8 call sites use 2..7); the sender, if any, is UNKNOWN.
- Car pad slot is always player 0's (`SetState(2, 0)` @0x801B95EC) — how cars behave for player 2 in 2P mode is UNKNOWN.
- `TVehicleCarMove` rows +0x14, +0x38, +0x54, +0x80, +0x84, +0x8C..+0xA0 and the collision-body helper
  0x801BABF4 (flags 0x200 driven / 0x203 unmanned) — roles UNKNOWN; "spin-turn" is a behavioural label (LIKELY).
- Vehicle virtual slots 4/5/8 (rider-delegated CharInfo queries), 0xE, 0x16/0x17, 0x1B/0x1C, 0x1D — semantics UNKNOWN.
- TVehicleBase +0x30 (2, car 1), +0x5C (6; batteries 4) and the rider flag 0x2A mirrored into vehicle bit4 — UNKNOWN.
- SetVehicleTask +0x50 deferred-spawn handle (fn_8003A17C / fn_80039F04, engine global 0x8051DFA8) — UNKNOWN meaning.
- Link gate table (singleton 0x805EF79C, fn_80169430) — "link active" is LIKELY, the writer was not traced.
- AttackCallbackParam result codes: 1 = hit and alive, 3 = destroyed, 5 = ignored/invulnerable are consistent in every
  vehicle (STRONG) but not tied to a documented enum; +0x30 reject flag, +0x4C, +0x58 fields partly UNKNOWN.
- (resolved in WEAPONS_AND_TARGETING.md) BulletGun floats = power 6.0 / 8.0, range 2000, speed 500; `Weapon::Bullet::Homing`
  kind 4 = AirBolt missile (power 4, speed 500, 540°/s); ShadowWeapon vslot 6 value 5 = melee weapon kind.
- CharInfo kind 0x37 (AirSaucer A → UseCommand) and 0x74 (blocks bird steering) — object classes UNKNOWN.
- fn_8034ED50 / fn_80169F94 pad codes (1, 2, 4, 6, 10) are LIKELY rumble patterns (peripheral code region) — not proven.
- Particle ids in 0x80512C38 and the explosion effect ids — asset names UNKNOWN.
- VehicleDataStart hook fn_801B4C54 → fn_801C0E74 and fn_8006B6C8 — not traced.

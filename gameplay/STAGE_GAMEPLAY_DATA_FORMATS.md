# STAGE_GAMEPLAY_DATA_FORMATS — stage object placement (SET) data

Scope: how enemies, gadgets, rings, mission objects, triggers etc. are described **outside** the DOL and how
the DOL turns them into gameplay objects. Tools: `tools/setparse.py` (parser + validator),
`tools/setobj_catalog.py` (schema extraction), outputs `data/setobj_catalog.{json,txt}`.

## 1. Files and load order (PROVEN — `SetData_LoadStage` 0x800CBC54)

| Step | File | Loader call | Source bit OR / AND-NOT mask | Evidence |
|---|---|---|---|---|
| 1 | `setid.bin` | fn_800CB044 → `SetData+0x0C/+0x10` | — | string 0x804B77E8 |
| 2 | (catalog 0x8052C1A0 marked for this stage) | fn_800CA7F0(set, table, stageIdx) | — | |
| 3 | `stg%04d/stg%04d_cmn.dat` | fn_800CB1A4 | none (file already carries 0x1000) | 0x804B77C0 |
| 4a | `stg%04d/stg%04d_hrd.dat` if bss 0x8057E885 ≠ 0 | fn_800CB0EC(set, name, 0x4000, 0xB000) | +0x4000, clear 0xB000 | 0x804B7790 |
| 4b | `stg%04d/stg%04d_nrm.dat` otherwise | fn_800CB0EC(set, name, 0x2000, 0xD000) | +0x2000, clear 0xD000 | 0x804B77A8 |
| 5 | `stg%04d/stg%04d_ds1.dat` | fn_800CB0EC(set, name, 0x8000, 0x7000) | +0x8000, clear 0x7000 | 0x804B7778 |
| 6 | catalog resource hooks | fn_800CA780 → desc+0x04 for each used type | | |
| 7 | `SetData+0x14 = 0x8052C1A0`; fn_800CA6B4 (slot table build); `SetManager+4 = 1` | | | |

So `_cmn` (common) objects are always present; exactly one of `_nrm`/`_hrd` is added (hard mode flag
0x8057E885 = byte +0x125 of the game-state block 0x8057E760); `_ds1` is always added. Bits 12–15 of the
record flag words record which file a record came from (cmn 0x1000, nrm 0x2000, hrd 0x4000, ds1 0x8000).
The meaning of "ds1" beyond "always loaded third" is UNKNOWN.

## 2. SET layout file (`*.dat`) — little-endian (PROVEN)

`fn_800CB1A4`: memcmp(file, "sky2", 4); count = u32 @+4; misc size = u32 @+8; records copied from +0x0C;
byte-swapped with format string **`"ffffffiisccii"`** (0x804B7768, fn_80043C80); misc block swapped as u32s
(fn_80043D34). After loading, `rec+0x18 = rec+0x1C` for every new record, and `rec+0x28` is rewritten to point
at the record's misc block (running pointer; block lengths from `rec+0x24`).

```c
struct SetFileHeader {          // 12 bytes, LE
    char magic[4];              // "sky2"
    u32  count;                 // number of SetRecord
    u32  miscSize;              // bytes of parameter data after the records
};
struct SetRecord {              // 0x2C bytes; format "ffffffiisccii"
    f32  pos[3];                // +0x00 world position (SetSlot_GetPos 0x800CA2FC)
    f32  rot[3];                // +0x0C rotation in DEGREES (SetSlot_GetRotRad 0x800CA2BC multiplies by pi/180)
    u32  runtimeFlags;          // +0x18 overwritten with +0x1C at load; SetFlag bits below
    u32  flags;                 // +0x1C authored flags; bit0 = spawnable, bits12-15 = source file
    u16  id;                    // +0x20 object type id (SetObjDesc.id)
    u8   link;                  // +0x22 link/group id (0..255) -> SetData link lists (fn_800CAE24)
    u8   range;                 // +0x23 activation radius in units of 100 (fn_800CA970)
    u32  miscLen;               // +0x24 bytes of params (== 4 * param count)
    u32  miscPtr;               // +0x28 runtime pointer to params (file value ignored)
};
```

**Validation (PROVEN):** `python tools/setparse.py` parses all 130 stage SET files (26 567 records);
26 557 records have `miscLen == 4 × paramCount(catalog)`. The 10 mismatches (SETCOLLISION ×2, HINT
COLLISION, HRD_CONTAINER, BK_WORM ×2, BLACK_BULL_0210, BK_SOLDIER ×3) are handled by the game's own check:
`SetSlot_GetParams(slot, dst, size)` (0x800CA1A4) copies only when `size == miscLen`, otherwise sets
`runtimeFlags |= 0x20000000` (error) — so those objects run with default parameters.

### Runtime flag bits (`SetRecord+0x18`)

| Bit | Meaning | Evidence | Confidence |
|---|---|---|---|
| 0x00000001 | enabled / spawnable (copied from authored flags) | fn_800CAA2C skips records without it | PROVEN |
| 0x00000002 | instance alive (object attached) | set by `SetSlot_Attach` 0x800CA0B8; fn_800CA8D8 won't respawn | PROVEN |
| 0x00000004 | despawn requested | cleared by Attach; tested by `SetSlot_ShouldDespawn` 0x800C9E4C | STRONG |
| 0x00000008 | written by `SetSlot_SetFlag8` 0x800C9C44 (GunSoldier at spawn; PowerDeviceCage/Naked in update) and `SetSlot_MarkKilledAndDetach` 0x800C9EEC; checkpoint save maps it to 0x200. The scanner preserves 0x8 when 0x40 explicitly rearms a slot; it is not a direct spawn veto. | PROVEN (writes/reads); full gameplay meaning UNKNOWN beyond checkpoint persistence |
| 0x00000040 | out-of-range after destruction → eligible to re-arm | fn_800CAA2C | LIKELY |
| 0x00000200 | record had flag 0x8 when a checkpoint was saved; skipped by the continue re-arm | fn_800CA480 (set), fn_800CA4B4 (test) — CHECKPOINTS_AND_RESTART.md | PROVEN (code) |
| 0x00000400 | (authored, +0x1C) record comes back disabled after a continue | fn_800CA4B4 | PROVEN (code) |
| 0x01000000 | GunSoldier "recovery score already paid" (per record) | enemy/GUN_SOLDIER.md | STRONG |
| 0x02000000 | preserved across continue | fn_800CA4B4 | PROVEN (code), meaning UNKNOWN |
| 0x00000800 | ignore range for fade-out test | fn_800C9D54 | PROVEN (code) |
| 0x00001000/2000/4000/8000 | source file cmn/nrm/hrd/ds1 | fn_800CB0EC masks | PROVEN |
| 0x00008000 (with sbss 805EF774 bit0) | exempt from global despawn | fn_800C9E4C | PROVEN (code) |
| 0x00010000 | respawn-when-out-of-range | fn_800CAA2C → sets 0x40 when no player in range | LIKELY |
| 0x00020000 | creation blocked | fn_800CA8D8 | PROVEN (code) |
| 0x00080000 / 0x00000800 | always "in range" | fn_800CA970 mask 0x80800 | PROVEN |
| 0x00800000 | pending re-enable | fn_800CAA2C | LIKELY |
| 0x20000000 | param size mismatch | fn_800CA1A4 | PROVEN |

**Native normal-death path (PROVEN, stage-owned slots):** `SetData::MarkKilledAndDetach`
follows `SetSlot_MarkKilledAndDetach` 0x800C9EEC → `SetSlot_DisableAndDetach`
0x800C9F1C → `SetSlot_Detach` 0x800C9FD0. It sets 0x8, clears 0x1/0x2/0x4,
clears the attached object and removes the slot from its link group. `test_set_task`
checks that the next scan cannot spawn the killed record; `stage_sim_stg0100_no_respawn`
locks the GUN Beetle integration count. `test_set_task` also proves an explicit
0x40 re-arm preserves flag 0x8 and enables the record again (scanner branch
0x800CACA4..0x800CACF4). The original's separate dynamic-slot
0x40000000 allocation cleanup is outside the native stage-owned SET buffer path.

## 3. `setid.bin` — per-stage object enable masks (PROVEN)

`u32 zero; u32 count; count × { u32 id; u32 mask[2]; }` (little-endian on disc; 12-byte entries, first entry at
+8). `fn_800CA7F0(set, table, stageIndex)`: for each entry whose bit `(31 - stageIndex % 32)` of
`mask[stageIndex / 32]` is set, finds the catalog descriptor with the same id and sets `desc->flags |= 1`;
`fn_800CA780` then runs the descriptor's resource-load hook. This is how per-stage object resources
(e.g. `enemy/GunSoldierData_%s.one`) are loaded only where needed.
(The byte order of `setid.bin` on the GC disc: first words read `00000000 2E010000 80250000` → count 0x12E,
first id 0x2580 → **little-endian**, like the .dat files.)

### 3.1 Stage number → stage index (STRONG, data-derived)

`SetData_LoadStage(mgr, stageNumber = [0x8057E808], stageIndex = [0x8057E80C])` (call at 0x8017871C).
For each stage the unique mask bit whose enabled-type set contains every object id used by that stage's
SET files (`tools/setparse.py` + setid.bin) gives:

| Stage | Index | Stage | Index | Stage | Index | Stage | Index |
|---|---:|---|---:|---|---:|---|---:|
| 0100 | 5 | 0400 | 12 | 0600 | 22 | 0510 | 33 |
| 0200 | 6 | 0401 | 13 | 0601 | 23 | 0610 | 35 |
| 0201 | 7 | 0402 | 14 | 0602 | 24 | 0611 | 36 |
| 0202 | 8 | 0403 | 15 | 0603 | 25 | 0612 | 37 |
| 0300 | (9, no exact match) | 0404 | 16 | 0604 | 26 | 0613 | 38 |
| 0301 | 10 | 0500 | 17 | 0700 | 27 | 0710 | 44 |
| 0302 | 11 | 0501 | 18 | 0210 | 28 | 0802 | 47 |
| | | 0502 | 19 | 0310 | 29 | | |
| | | 0503 | 20 | 0410 | 30 | | |
| | | 0504 | 21 | 0411/0412 | 31/32 | | |

The sequence is monotonic in story-stage order, then bosses — consistent with a stage table in the DOL
(to be located; the mission investigation may find it). Ambiguous: 0614–0618, 0800/0801 (small object sets).

## 4. Object type catalog inside the DOL (PROVEN)

`0x8052C1A0` = NULL-terminated array of `SetObjDesc*` (286 entries, dump: `data/setobj_catalog.txt`).

```c
struct SetObjDesc {             // 0x24 bytes
    const char* name;           // +0x00 e.g. "GUN_SOLDIER"
    void (*loadResources)();    // +0x04 called by fn_800CA780 when id is enabled in setid.bin
    void (*releaseResources)(); // +0x08 (GUN_SOLDIER: ResourceManager slot 7 release)
    void (*create)(SetSlot*);   // +0x0C called by SetSlot_TrySpawn 0x800CA8D8
    u32  unk10, unk14;          // +0x10/+0x14 (0x80, 0x200, 0x800 seen) UNKNOWN
    u16  id;                    // +0x18
    u8   unk1A;                 // +0x1A 1 = gadget, 2 = enemy, 4 = ? (LIKELY category)
    u8   flags;                 // +0x1B bit0 = used in current stage (runtime)
    u8   unk1C;                 // +0x1C 10 gadgets, 5 enemies (LIKELY category/priority)
    SetParamDesc* params;       // +0x20 array terminated by type == 0
};
struct SetParamDesc {           // 0x28 bytes — editor schema, 4 bytes of data per param
    u32  type;                  // +0x00 1 Sint32, 2 Uint32, 3 Hex, 4 Single (f32) — names PROVEN by the
                                //       DOL's own "Sample" object (0x2586) whose params are named
                                //       "Single","Hex","Uint32","Sint32" with types 4,3,2,1
    const char* name;           // +0x04 e.g. "SearchRange"
    s32  i[3];                  // +0x08 ints: {default?, min, max} for int params
    f32  f[3];                  // +0x14 floats: {default, min, max} for float params
    void* accessor;             // +0x20 (0x800CC008 generic)
    u32  pad;
};
```
Examples: `0x0001 SPRING {level f32 [0..15], nocontrol time (sec) [0..3600]}`,
`0x0010 RING {type, num[1..128], param0, param1, ghost}`, `0x0014 GOALRING {color}`,
`0x0064 GUN_SOLDIER` (23 params: MoveRange, SearchRange, SearchAngle[-360..360], SearchWidth,
SearchHeight, SearchHeightOffset, MoveSpeedRatio, HaveShield, WeaponType[0..6], AppearType[0..5],
Pos0..2 patrol points with WaitType/WaitSec/MoveSpeedRatio), `0x2595 MissionClearCollision`.

Id ranges (from catalog): 0x0001–0x0062 common gadgets; 0x0064–0x0093 enemies; 0x00B4–0x00BE bosses;
0x012C STICK; 0x0190–0x019C characters/NPCs (SET_CHARACTOR, Sonic, Tails, … DoomsEye); 0x03E8.. and up
stage-specific gadgets (Westopolis 0x03E8 CityChaosEmerald, Digital Circuit 0x07D0.. ELEC_*, …);
0x2580–0x2598 marker/utility objects (GadgetCommonDataStart/End, WeaponDataStart/End, VehicleDataStart/End,
EnemyEntryStart/End, ART GADGET, PARTICLE, SET_SE_LOOP, MissionClearCollision, HINT).

## 5. From record to object (PROVEN)

```text
SetData_LoadStage (0x800CBC54)       records → SetData+0x41C (≤0x800), slots → SetData+0x18 (0x800 × 0x20)
SetManagerTask::Update (0x800CB794)  → SetManager_Update (0x800CB97C) each frame
   → SetData_ScanSlots (0x800CAC08 / 2P 0x800CAA2C) with viewpoint = camera unit position
       (CameraManager_Get 0x80009548 → CameraManager_GetUnitPos 0x80010244(mgr, i); PROVEN — NOT the player position)
       → SetSlot_TrySpawn (0x800CA8D8): in range → desc->create(slot)
             e.g. GUN_SOLDIER create 0x80196E38 → new(0x418) GunSoldier::GunSoldier(this,1,layer Enemy,slot)
object ctor/init: SetSlot_Attach(slot, obj) (0x800CA0B8), SetSlot_GetPos/GetRotRad, SetSlot_GetParams(slot,&p,size)
object spawn/update: SetSlot_SetFlag8(slot) (0x800C9C44) → flag 0x8 (meaning UNKNOWN)
per-frame:        TEnemySetTask::Update checks SetSlot_ShouldDespawn / SetSlot_OutOfFadeRange
```

## 6. SetData object (the big loader state, pointed to by `SetManager+0`)

| Offset | Size | Field | Evidence | Confidence |
|---|---:|---|---|---|
| +0x000 | 4 | record count | fn_800CB1A4 | PROVEN |
| +0x004 | 4 | used-descriptor count | fn_800CA780 | PROVEN |
| +0x008 | 4 | alive-object count (recounted per scan) | fn_800CAA2C | STRONG |
| +0x00C | 4 | setid.bin entry count | fn_800CA7F0 | PROVEN |
| +0x010 | 4 | setid.bin entries* | fn_800CA7F0 | PROVEN |
| +0x014 | 4 | SetObjDesc** table (0x8052C1A0) | fn_800CBC54, fn_800CA8D8 | PROVEN |
| +0x018 | 4 | SetSlot[0x800]* (0x20 each) | fn_800CAF38 | PROVEN |
| +0x01C | 0x400 | link-group list heads [256] | fn_800CAE24 | PROVEN |
| +0x41C | 4 | SetRecord[0x800]* (0x16000 bytes) | fn_800CAF38 memset 0x16000 | PROVEN |
| +0x420 | 4 | misc parameter buffer* | fn_800CB1A4 | PROVEN |
| +0x424 | 4 | misc bytes used | fn_800CB1A4 | PROVEN |
| +0x428 | ? | bitfield (bit0 tested in 2P scan) | fn_800CAA2C | UNKNOWN meaning |

SetSlot (0x20): `+0 SetRecord*`, `+4 prev`, `+8 next` (link-group list), `+0xC attached object`,
`+0x10 descriptor index (s32, <0 none)`, `+0x14` (set by fn_800C9D18, UNKNOWN), rest UNKNOWN.

## 7. Other per-stage files (not yet analysed)

`stgXXXX_cam.dat` (camera, loaded by fn_801C873C), `stgXXXX_light.bin` (lighting), `stgXXXX_dat.one`,
`stgXXXX_gdt.one` (gadget models), `stg_cmn_gdt.one` (loaded by GadgetCommonDataEnd hook fn_8015DC38),
`stgXXXX_NN.one` (landscape blocks), `enemy/*Data*.one`, `enemy/EnemyPath.one`, `common/ScoreData.bin`,
`common/WeaponResource.one`, `common/vehicleResource.one`. Path data (`PathObject`, `EnemyPath.one`) is
used by flyer enemies (`EnemyAIState_FlyerCommonAI_MoveOnPath`) — UNKNOWN format.

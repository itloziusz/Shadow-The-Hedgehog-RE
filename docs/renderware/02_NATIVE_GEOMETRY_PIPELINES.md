# 02 — RwGC native geometry pipelines (world sectors, atomics, skinning, MatFX, materials, lights)

Shadow the Hedgehog, GameCube PAL `GUPP8P`. Lane "geometry" of the RENDERER_RE reverse-engineering pass.
Companion data: `02_geometry_formats.csv` (every struct field / enum / record in this document, one row each).

**Purpose.** This is an implementable specification of how RenderWare 3.7's GameCube driver ("RwGC")
turns `RpWorldSector` and `RpAtomic` objects into GX calls in this game, so that a native DX12 renderer can
replace everything *below* `RxPipelineExecute` while the game (and RW's scene/visibility code above it)
keeps running as guest code.

**Confidence tags.** CONFIRMED = read in code **and** observed at runtime (hook counts, memory dump, or asset
bytes). CODE = read in code only. INFERRED = derived, not directly observed. Addresses are guest PPC;
`recomp_NNN.cpp:line` is the translated body in `GekkoForge/build-native/generated/`.

**Evidence base (this lane).**
- Generated-code citations are the `// 0xADDR` header line of the function in the current `build-native/generated` tree (the line numbers in `RECOMP_VISUAL_MAP/db/functions.csv` are stale).
- Disassembly of `sys/main.dol` via `GekkoForge/extract/functions.jsonl` (note: that file mis-decodes `fcmpo`
  as `fadd`; every float compare below was re-read with that in mind).
- Four headless runs of a private copy of `scratch/stage-boot-2026-09-16/exe`, `--boot-stage stg0604`
  (Final Haunt), budgets 40e9 and 45e9, each pair with ~60-80 `--watch` entry hooks. Per-frame numbers are
  the 40e9→45e9 delta divided by the 616 `GXCopyDisp` calls in that window. The two run pairs gave
  **identical** deltas for every shared hook (deterministic).
- A full MEM1 dump at 45e9 (`--dump 0x80000000 0x1800000`), decoded offline (pipelines, resource entries,
  display lists, vertex arrays, RpSkin, RpWorld, materials).
- The disc assets: a `.one`/PRS unpacker and RW chunk walker written for this lane
  (`RENDERER_RE/_work/geometry/tools/one.py`, `rg1.py`, `census.py`, `matcensus.py`).
- Scratch probes and raw outputs: `RENDERER_RE/_work/geometry/` (`probe*.py`, `runs/*.hits`, `runs/probe3.txt`,
  `runs/rg1_census_all.txt`, `perframe2.txt`).

---

## 0. Executive summary

1. **There are 8 RxPipelines and every one is a single "AllInOne" node** whose 16-byte private data holds
   four callbacks `{instance, reinstance, lighting, render}`. The "runtime-registered roots with no static
   refs" of the older docs (0x80452B70, 0x804A62D0, …) are these callback slots; they are materialised with
   `lis/addi` in the pipeline constructors. The live pipeline structs were dumped (§2.1). CONFIRMED.
2. **Two node bodies do all the work**: `0x80464AAC` (atomic, "GameCubeAtomicAllInOne.csl") and `0x80464FF4`
   (world sector, "GamerCubeWorldSectorAllInOne.csl"). They call instance-or-reinstance, then lighting, then
   render, passing a 44-byte on-stack `RxGameCubeInstanceData` (§2.3). CONFIRMED.
3. **World geometry is pre-instanced on disc** (`*.RG1` inside the stage `.one` archives, Native Data PLG
   chunk 0x510, platform 6). **Models (`*.DFF`) are not**; they are instanced in memory at first render by
   `0x8046D124` (static) / `0x80451988`/`0x8046D124` (skinned). Both produce the *same* in-memory layout:
   `RwResEntry` + big-endian header + attribute arrays + one GX display list per `RpMesh` (§3). CONFIRMED.
4. **Every attribute is indexed** (`GXSetArray` + `GX_INDEX8/16`), display lists contain only
   `GX_TRIANGLESTRIP (0x98)` on Final Haunt, and are padded to 32 bytes. World sectors use per-attribute
   deduplicated indices (mixed 8/16-bit); instanced models use one shared vertex index for all attributes.
   CONFIRMED (dump + disc census of 3,092 native sectors).
5. **Vertex formats**: world sectors = POS f32, CLR0 RGBA8, TEX0 (+TEX1) f32, **no normals anywhere on the
   disc**. Models use a game-supplied `RpGameCubeVtxFmt` (created by game code `0x802E3034`): POS f32,
   **NRM s8 frac 6** (`trunc(n*64)`), **CLR0 RGBA4** (`c>>4` per channel), TEX f32. CONFIRMED (byte compare of
   instanced arrays against the platform-independent geometry still in RAM).
6. **Matrices**: one position matrix per draw (`GX_PNMTX0`), `M = LTM × View(0x805E428C)`, then an axis flip
   `diag(-1, 1, -1)`; the normal matrix is the same 3×3 (no inverse-transpose). World sectors get the view
   matrix only. CONFIRMED.
7. **Skinning on Final Haunt is 100% CPU skinning**: all 5 live skins have `maxNumWeightsForVertex = 2`, which
   forces the CPU path (positions and s8 normals rewritten in the instanced arrays every frame, drawn with the
   atomic LTM). The GX matrix-palette path (PNMTXIDX, ≤10 bones per split mesh) exists but is not live here.
   CONFIRMED.
8. **Two independent material/lighting implementations**: the default pipelines (sector, atomic, skin) use the
   TEV/channel "brain" `0x80466624` (1- or 2-stage TEV, material color in TEV REG0/REG1 or channel
   registers); the MatFX pipelines use `0x804A55CC` + `0x804A3CE0` (always 1-stage `GXSetTevOp`, material
   color in channel registers, extra passes for effects). Decision tables for both are given exactly (§9, §10).
9. **MatFX live on Final Haunt**: ENVMAP (25.1 meshes/frame, additive second pass, normal-driven texgen) and
   UVTRANSFORM (6 meshes/frame). DUALTEXTURE, DUALUVTRANSFORM, BUMPMAP, BUMPENVMAP and the GCN native-shader
   material plugin (0x129) have **0 hits** on Final Haunt (dual/dualUV/bumpenv exist in other stages' data;
   0x129 exists in no asset on the disc). CONFIRMED.
10. **Final Haunt per frame (in-stage window)**: 8 world sectors drawn (15 sector callbacks, 7 empty), 83.1
    atomic pipeline executions (33 default, 30.1 MatFX, 2 skin, 2 skin+MatFX, 16 PTank-ID effect atomics),
    4 CPU-skinned atomics, 176.3 `GXCallDisplayList` = **110,447 display-list bytes/frame**, 21.1 atomics
    collect world lights, 18.1 `GXLoadLightObjImm`. Lit world: **no** (the game clears `rpWORLDLIGHT` on the
    stage worlds). About 9 distinct TEV/channel configurations are live (§11.3). CONFIRMED.
11. **Game logic sits between the camera and the pipeline**: per-atomic game render callbacks
    (`0x801645D0` 53/frame, `0x8003851C`, `0x800386EC`) switch the stage light record, edit material
    color/UV state and toggle `rpGEOMETRYMODULATEMATERIALCOLOR` *immediately before* each
    `RpAtomicRender`. A native renderer must snapshot RW state per submitted atomic, never per frame. CONFIRMED.

---

## 1. Layer map and the cut line

```
GUEST (stays):  task/render levels → RpWorldRender / RpClumpRender (RW scene traversal, frustum culling)
                → atomic->renderCallBack  (game wrappers 0x801645D0 / 0x8003851C / 0x800386EC → 0x80456F04)
                → world->renderCallBack   (0x80460B60, per sector)
============================ CUT LINE: RxPipelineExecute(pipeline, object, TRUE) 0x8049C6C4 ============
REPLACE:        AllInOne node body 0x80464AAC (atomic) / 0x80464FF4 (sector)
                  ├─ instance / reinstance callback  → RwResEntry instance data (arrays + display lists)
                  ├─ lighting callback               → GX light objects + RxGameCubeInstanceData lighting state
                  └─ render callback                 → SetArrays, matrices, TEV/channels, texture bind, DL submit
                GX SDK 0x80393754-0x8039AE9C → FIFO
```

| Function | Address | Generated | Role | Conf. |
|---|---|---|---|---|
| RpAtomic default render cb | 0x80456F04 | recomp_021.cpp:150551 | `pipeline = atomic+0x6C ? : globals+60`; `RxPipelineExecute(pipeline, atomic, 1)` | CONFIRMED (83.1/frame) |
| RpWorldSector render cb | 0x80460B60 | recomp_019.cpp:157661 | skip if `u16 sector+0x84 (numPolygons) == 0`; `pipeline = sector+0x7C ? : world+0x6C ? : globals+64`; execute | CONFIRMED (15/frame, 8 non-empty) |
| RxPipelineExecute | 0x8049C6C4 | — | RW PowerPipe executor (device lane) | CODE |
| Atomic AllInOne node body | 0x80464AAC | recomp_016.cpp:169154 | §2.2 | CONFIRMED (83.10/frame) |
| Sector AllInOne node body | 0x80464FF4 | recomp_028.cpp:169533 | §2.2 | CONFIRMED (8.00/frame) |

`globals` = `RwEngineInstance([0x805F265C]) + [0x805F2738]` (RpWorld module globals). At runtime: `+60` current
default atomic pipeline, `+64` current default sector pipeline, `+84`/`+88` the driver's original defaults
(both pairs point to the same objects on Final Haunt). `RWSRCGLOBAL(curWorld)` is `[RwEngineInstance+4]`
(read by every lighting callback). CONFIRMED (dump).

**Game callbacks above the cut** (not re-implemented, but they mutate what is below):
`0x801645D0` (53.0/frame), `0x8003851C` (3.1/frame), `0x800386EC` (2.0/frame). Each fetches per-atomic display
params (`0x80037D84`/`0x80037D2C`), calls `StageLight_SetCurrentRecord 0x80048308` (rewrites RpLight colors),
runs `RpGeometryForAllMaterials 0x8045A138` with a material callback, sets or clears geometry flag `0x40`
(MODULATEMATERIALCOLOR), then tail-calls the saved original callback (0x80456F04). CONFIRMED (code + counts).

---

## 2. Pipelines, nodes and the per-draw instance record

### 2.1 The eight live pipelines (runtime dump at 45e9)

`RxPipeline` as used: `+0x04 u32 numNodes`, `+0x08 RxPipelineNode* nodes` (40 bytes each), `+0x2C u32 pluginId`,
`+0x30 u32 pluginData (sub-id)`. `RxPipelineNode`: `+0x00 RxNodeDefinition*`, `+0x14 void* privateData`.
Private data (16 bytes) = `{+0 instanceCB, +4 reinstanceCB, +8 lightingCB, +12 renderCB}`; setters
`0x80463C40/4C/58/64` (recomp_044.cpp:173505, recomp_045.cpp:168018, recomp_046.cpp:158939, recomp_047.cpp:163547).
Node definitions: atomic `0x8056E98C` (name `"GameCubeAtomicAllInOne.csl"`, body 0x80464AAC,
pipelineNodeInit 0x80464D30), sector `0x8056E9F4` (`"GamerCubeWorldSectorAllInOne.csl"`, body 0x80464FF4,
init 0x804651D8); `pipelineNodePrivateDataSize = 0x10`.

| Pipeline (object @ dump) | Global | pluginId / sub | Built by | instance | reinstance | lighting | render | Per frame (FH) |
|---|---|---|---|---|---|---|---|---|
| Default atomic `0x80784980` | worldglobals+60/+84 | 0x2 / 0 | 0x804642E8 (recomp_009.cpp:158500) | 0x804649FC | 0x80464890 | 0x80464478 | 0x80463964 | 33 |
| Default sector `0x807854C0` | worldglobals+64/+88 | 0x2 / 0 | 0x80464228 (recomp_007.cpp:161630) | 0x80464F60 | — | 0x80464E44 | 0x80463964 | 8 |
| MatFX atomic `0x80786100` | [0x805F2764] | 0x120 / 1 | 0x804A64E4 (recomp_038.cpp:161962) | 0x804A60A8 | 0x804A6174 | 0x80464478 | 0x804A62D0 | 30.1 |
| MatFX sector `0x80786160` | [0x805F2768] | 0x120 / 2 | 0x804A64E4 | 0x804A6254 | — | 0x80464E44 | 0x804A62D0 | 0 |
| Skin atomic `0x80786000` | [0x8056E7C8+40] | 0x116 / 1 | 0x80452DB0 (recomp_055.cpp:166889) | 0x8044E974 | 0x8044EA8C | 0x80464478 | 0x8044EEC4 | 2 |
| Skin+MatFX atomic `0x80786060` | [0x8056E7C8+44] | 0x116 / 2 | 0x80452DB0 | 0x80452ABC | 0x80452B70 | 0x80464478 | 0x80452C1C | 2 |
| PTank-ID atomic `0x807865C0` | [0x805F24F8] | 0x12F / 0 | 0x8044960C (recomp_020.cpp:146509) | — | — | 0x80449D00 | 0x80449D54 | 16 |
| DMorph atomic `0x80786560` | [0x805F24E4] | 0x122 / 1 | 0x80446C88 (recomp_037.cpp:149053) | 0x80446984 | 0x80445ECC | 0x80464478 | 0x80463964 | 0 |

Builders: atomic pipes via `0x804643AC(pluginId, subId, inst, reinst, light, render)` (recomp_011.cpp:162437),
sector pipes via `0x80464D78(...)` (recomp_025.cpp:173704); both `RxPipelineCreate 0x8049C7BC`, store
pluginId/subId at +44/+48, lock, add the AllInOne fragment, unlock, find the node, set the 4 callbacks.
Selection: `atomic+0x6C`, `sector+0x7C`, `world+0x6C` pipeline pointers (all world/sector pointers NULL on
Final Haunt → default sector pipeline). CONFIRMED.

Per-frame check: `0x80463964` 41 = 33 atomics + 8 sectors; node-body atomics 83.1 = 33 + 30.1 + 2 + 2 + 16.

### 2.2 AllInOne node algorithm

**Atomic body `0x80464AAC`** (r3 = node, r4 = packet → atomic, private = node+0x14):

```
geom = atomic->geometry (+0x18); flags = geom->flags (+0x08)
inst = { repEntry, meshHeader = geom+0x54, flags }            // 12 of the 44 bytes, rest filled later
if (flags & 0x01000000 /*rpGEOMETRYNATIVE*/) {
    inst.repEntry = geom->repEntry (+0x58); reinstanceCB(atomic,&inst) if set
} else {
    if (geom->numVertices(+0x14) == 0 || meshHeader->numMeshes(u16 +4) == 0) return
    inst.repEntry = (geom->numMorphTargets(+0x18) == 1) ? geom->repEntry(+0x58) : atomic->repEntry(+0x14)
    if (repEntry && u16 repEntry+26 (header+2 serial) != u16 meshHeader+6 (serialNum)) { RwResourcesFreeResEntry 0x8048176C; repEntry = 0 }
    if (repEntry) { reinstanceCB; move repEntry to head of the resource-arena LRU list }
    else          { instanceCB(atomic,&inst); reinstanceCB(atomic,&inst) }
}
lightingCB(atomic,&inst); renderCB(atomic,&inst); return 1
```

**Sector body `0x80464FF4`**: identical shape; `flags = curWorld->flags (+0x08)`, `repEntry = sector+0x34`,
`meshHeader = sector+0x78`, skip if `u16 sector+0x82 (numVertices) == 0`. NATIVE worlds never instance.
CONFIRMED (sector instance callbacks 0 hits; native stream reads 8 at load).

### 2.3 `RxGameCubeInstanceData` (44 bytes on the node's stack, passed as r4 to every callback)

| Off | Type | Field | Writer |
|---|---|---|---|
| +0x00 | RwResEntry* | repEntry (instance data, §3) | node body / instance cb |
| +0x04 | RpMeshHeader* | meshHeader | node body |
| +0x08 | u32 | geometry (or world) flags | node body |
| +0x0C | f32 ×3 | accumulated ambient RGB (0..1, clamped to 1.0) | lighting cb |
| +0x18 | f32 | ambient alpha (= 1.0) | lighting cb |
| +0x1C | s32 | hasAmbient (1 if any ambient light was added) | lighting cb |
| +0x20 | u32 | GX light mask (bit i = GX_LIGHTi loaded) | lighting cb |
| +0x24 | s32 | number of GX lights loaded (max 8) | lighting cb |
| +0x28 | void* | node private data (callbacks) | node body |

CONFIRMED (code: 0x80464478 / 0x80464E44 initialise +0x0C..+0x24; 0x80463964 reads +0x1C/+0x20).

---

## 3. Instance data (the in-memory GC native geometry)

### 3.1 Memory layout

```
RwResEntry (24 B, native order)          +0x00 link.next  +0x04 link.prev  +0x08 u32 size
                                         +0x0C owner (geometry, atomic or sector)  +0x10 RwResEntry** ownerRef
                                         +0x14 destroyNotify (0x804638DC static/sector, 0x8044DE14 skinned)
header (starts at entry+24, big-endian)  +0x00 u16 drawSyncStamp   +0x02 u16 meshSerial
                                         +0x04 u32 hdrFlags (bit0 = vertex colors carry alpha)
                                         +0x08 u32 numAttributes (N)
                                         +0x0C attribute record × N   (8 B each)
                                         +0x0C+8N mesh DL record × numMeshes (8 B each)
data (32-byte aligned after header)      display lists (mesh order), then attribute arrays (attribute order)
```

Attribute record (8 bytes): `+0 u8* array` (relocated pointer), `+4 u8 gxAttr` (9 POS, 10 NRM, 11 CLR0,
13 TEX0, 14 TEX1; informational only), `+5 u8 stride` (bytes per element), `+6 u8 indexType` (`GX_INDEX8=2`,
`GX_INDEX16=3`), `+7 u8 pad=0`. Mesh DL record: `+0 u8* displayList`, `+4 u32 sizeBytes`.
Attribute order is fixed: POS, [NRM or NBT], [CLR0], [TEX0..TEXn]. CONFIRMED (0x80463C80, dump).

### 3.2 Array/descriptor binding — `RwGC_SetArrays 0x80463C80` (recomp_050.cpp:153313)

Args: `r3 = RpGameCubeVtxFmt*` (NULL → default fmt `0x805E2748`), `r4 = inst`. `hdr = repEntry+24`.

| Step | Condition | GX calls |
|---|---|---|
| 1 | always | `GXClearVtxDesc()` |
| 2 | always | `GXSetVtxDesc(GX_VA_POS, attr0.indexType)`; `GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, fmt.posType, fmt.posFrac)`; `GXSetArray(GX_VA_POS, attr0.array, attr0.stride)` |
| 3a | flags&0x10 and fmt.nbt≠0 | `GXSetVtxDesc(GX_VA_NBT, attr1.indexType)`; `GXSetVtxAttrFmt(0, GX_VA_NBT, GX_NRM_NBT, fmt.nrmType, 0)`; `GXSetArray(GX_VA_NBT, …)` |
| 3b | flags&0x10, nbt=0 | same with `GX_VA_NRM`, `GX_NRM_XYZ` |
| 4 | flags&0x08 | `GXSetVtxDesc(GX_VA_CLR0, idx)`; `GXSetVtxAttrFmt(0, GX_VA_CLR0, fmt.colType>2 ? GX_CLR_RGBA : GX_CLR_RGB, fmt.colType, 0)`; `GXSetArray(GX_VA_CLR0, …)` |
| 5 | flags&0x84 | for each remaining attribute k (k = 0.., until N): `GXSetVtxDesc(GX_VA_TEX0+k, idx)`; `GXSetVtxAttrFmt(0, GX_VA_TEX0+k, GX_TEX_ST, fmt.texType[k], fmt.texFrac[k])`; `GXSetArray(GX_VA_TEX0+k, …)` |

The GX attribute id comes from the position in the record list, not from the stored `gxAttr` byte.
The skin render callbacks add `GXSetVtxDesc(GX_VA_PNMTXIDX, GX_DIRECT)` afterwards when (and only when) the
matrix-palette path is used (§6). Everything uses `GX_VTXFMT0`. CONFIRMED.

### 3.3 `RpGameCubeVtxFmt` (24 bytes)

| Off | Type | Field | Default (0x805E2748, init 0x80463FFC) | Game model fmt (0x80571BD0, game 0x802E3034) |
|---|---|---|---|---|
| +0 | u8 | posType (GXCompType) | 4 GX_F32 | 4 GX_F32 |
| +1 | u8 | nrmType | 4 GX_F32 | 1 GX_S8 |
| +2..+9 | u8×8 | texType[0..7] | 4 | 4 |
| +10 | u8 | colType (GXCompType colour: 0 RGB565,1 RGB8,2 RGBX8,3 RGBA4,4 RGBA6,5 RGBA8) | 5 RGBA8 | 3 RGBA4 |
| +11 | u8 | unknown (always 1) | 1 | 1 |
| +12 | u8 | posFrac | 0 | 6 (ignored for F32) |
| +13 | u8 | nbt flag (normals as NBT) | 0 | 0 |
| +14..+21 | u8×8 | texFrac[0..7] | 0 | 6 (ignored for F32) |
| +22 | u16 | refCount | 1 | 0x0208 (shared) |

API: `SetPosition 0x80463FCC(fmt,type,frac)`, `SetNormal 0x80463FD8(fmt,type,nbt)`,
`SetTexCoords 0x80463FE4(fmt,index1based,type,frac)`, `SetColor 0x80463FF4(fmt,type)`, `Create 0x8046406C`,
`Destroy 0x80464110`, `RpGameCubeGeometrySetVtxFmt 0x80464154` (refcounted). Plugin id 0x511 registered by
`0x80463F24`: engine (default fmt), geometry offset `[0x805F25C8]=0x60`, world offset `[0x805F25CC]=0x70`.
The MatFX pipeline owns a third fmt `[0x805F2760]` (F32 normals as NBT) used only for bump-mapped geometry.
Game code `0x802E3034` (recomp_006.cpp:100432) builds a pair of formats and `0x802E2F84` assigns one to every
atomic's geometry — this is above the cut line but it decides the quantisation the instanced data has.
CONFIRMED (dump bytes `040104040404040404040301060006000000000000000208`).

### 3.4 Display-list encoding

- Primitive record: `u8 opcode` (`0x98` GX_TRIANGLESTRIP; `0x90` GX_TRIANGLES when the RW mesh header is not
  a tristrip; fmt index 0), `u16 BE vertexCount`, then per vertex the index tuple in attribute order:
  `[u8 PNMTXIDX]` (palette skins only) + one index per attribute, width 1 or 2 bytes (BE) by that attribute's
  `indexType`. DLs are padded with `0x00` (GX NOP) to a multiple of 32 bytes.
- Pre-instanced world sectors: several strips per mesh (strips separated by new `0x98` records), indices
  deduplicated **per attribute** (e.g. `POS u16, CLR0 u8, TEX0 u8`).
- Runtime-instanced models (`0x8046D124`): exactly **one** primitive per mesh (the RW mesh index list, already
  a single degenerate-joined strip), vertex tuple repeats the same RW vertex index for every attribute; arrays
  have `numVertices` entries in RW vertex order. Index width per attribute: `numVertices < 255 → GX_INDEX8`,
  else `GX_INDEX16` (`cmplwi r29,255` at 0x8046CF80). DL size = `align32(3*numPrims + numIndices*tupleBytes)`
  (`0x8046C0E4`).
- Census: Final Haunt RG1 files (46 files, 100 sectors) and the whole disc (1,625 RG1 files, 3,092 sectors):
  11,358 / 411,955 primitives, all `0x98`; no NRM attribute in any sector. CONFIRMED.

### 3.5 Quantisation performed by the in-game instancer (model data)

Compared array bytes against the platform-independent geometry still resident in RAM (probe9):

| Attribute | Rule | Result |
|---|---|---|
| POS (F32) | copy | identical for all vertices |
| NRM (S8, frac 6) | `(s8) trunc(n * 64.0)` (C cast toward zero) | 72/72, 297/297, 285/285 components match |
| CLR0 (RGBA4, u16 BE) | `(r>>4)<<12 \| (g>>4)<<8 \| (b>>4)<<4 \| (a>>4)` | 24/24, 99/99 match |
| TEX (F32) | copy | identical |

GX dequantises RGBA4 as `v*17` (4→8 bit replication) and S8/frac6 as `v/64`. `hdrFlags bit0` is set when the
colour format is RGBA (`colType>2`) or, without a fmt, when any prelit alpha < 255 (0x8046D324-0x8046D3A8).
CONFIRMED.

### 3.6 `drawSyncStamp` / `meshSerial`

`header+0` is written with `[0x805EEFF0]` (u16 CPU-side draw-sync token, wraps modulo 0xE000) by every render
callback. Before rewriting arrays in place, reinstance (`0x80464890`) sends `GXSetDrawSync(token)` and polls
`0x8049B7DC` (compares against `GXReadDrawSync` stored at `[0x805F2720]`) until Flipper has consumed the old
buffer. `header+2` must equal `RpMeshHeader.serialNum` or the entry is freed and re-instanced. CONFIRMED (code);
drawSync is purely a CPU/GPU-concurrency device.

---

## 4. Where instance data comes from (byte-level)

### 4.1 `.one` archive (`One Ver 0.60`) and PRS

| Off | Type (LE) | Field |
|---|---|---|
| 0x00 | u32 | 0 |
| 0x04 | u32 | archive size − 12 |
| 0x08 | u32 | RW library stamp `0x1C020037` (3.7.0.2) |
| 0x0C | char[12] | `"One Ver 0.60"` |
| 0x18 | u32 | 0 |
| 0x1C | u32 | fileCount |
| 0x20 | 0x90 B | block name (`"blk01"`, 0xCD padding) |
| 0xB0 + 0x38·i | char[44] | file name |
| +0x2C | u32 | uncompressed size |
| +0x30 | u32 | data offset − 12 (file offset = value + 12) |
| +0x34 | u32 | 1 |

Each file's bytes run to the next file's offset and are Sega **PRS** compressed. Control bits are consumed
LSB-first from a control byte refilled every 8 bits:
- bit `1`: copy one literal byte;
- bits `0,1`: long copy — read `a`, `b`; if `a==0 && b==0` the stream ends; `offset = ((b<<8|a)>>3) − 0x2000`;
  `len = a&7`; if `len == 0` then `len = next byte + 1` else `len += 2`;
- bits `0,0`: short copy — `len = (bit<<1 | bit) + 2` (2-5), `offset = next byte − 256`;
- a copy reads `len` bytes from `output[end + offset]` one at a time (overlap allowed).
Implementation verified by exact size match on every Final Haunt archive (tools/one.py). Loader in guest:
`0x8004CE1C`/`0x8004CD38` (asset-trace comments in shadow_main.cpp). CONFIRMED (bytes).

### 4.2 Pre-instanced world: `stgNNNN_KK_<G|OS|D|A>_.RG1`

RW 3.7 stream (little-endian chunk framing), tree for `STG0604_01_G_.RG1`:

```
0x29 ChunkGroupStart ("stg0604_01_g_")
0x0B World
  0x01 Struct (64 B): rootIsWorldSector=1, invWorldOrigin(0,0,0), numTriangles 346, numVertices 446,
                      numPlaneSectors 0, numWorldSectors 1, colSectorSize 0,
                      format 0x4101002D (TRISTRIP|TEXTURED|PRELIT|LIGHT, texSets=1 in byte 2, NATIVE 0x01000000,
                      SECTORSOVERLAP 0x40000000), bbox sup/inf
  0x08 MaterialList → 0x07 Material (Struct 28 B: flags, RGBA, unused, textured, ambient/specular/diffuse)
                      → 0x06 Texture (filter/addressing, name, mask)
  0x09 AtomicSector
     0x01 Struct (44 B): matListWindowBase, numTriangles, numVertices, bbox inf/sup, collSectorPresent, unused
                          — NO vertex/triangle/UV/prelit arrays follow (native)
     0x03 Extension
        0x50E BinMesh (Struct-less): u32 flags(1=tristrip), u32 numMeshes, u32 totalIndices,
              per mesh {u32 numIndices, u32 matIndex} — NO index data (native)
        0x510 Native Data PLG
           0x01 Struct: u32 LE platform = 6 (GameCube), u32 LE headerSize, u32 LE dataSize,
                        headerSize bytes: the §3.1 header verbatim (big-endian, offsets relative),
                        dataSize bytes: DLs + arrays (offsets in the header are relative to data start)
        0x11D CollisionPLG, 0x11F UserData
0x2A ChunkGroupEnd
```

Reader: plugin 0x510 registered at `0x80462FD4-0x80463048` for geometry (read `0x80462C8C` →
`0x804661D0`) and world sector (read `0x80462D14` → `0x804662B0`); both call
**`NativeDataReadCommon 0x80465AE4`** (recomp_040.cpp:163934, continues at 0x80465C10 recomp_041.cpp:150679):
1. `RwStreamFindChunk(rwID_STRUCT)`; library version must be in `[0x35000, 0x37002]`.
2. read `platform` (must be 6), `headerSize`, `dataSize`.
3. `RwMalloc(headerSize + dataSize + 55, 0x3050D)`; header read at `entry+24`; data read at
   `align32(entry+24+headerSize)`.
4. relocate: `attr[i].array += dataBase`; `mesh[j].dl += dataBase` (numMeshes from the mesh header).
5. fill RwResEntry `{0,0,size=chunkSize,owner,ownerRef,destroyNotify 0x804638DC}`; `header.stamp = [0x805EEFF0]`;
   `DCFlushRange`; `GXInvalidateVtxCache`.

In the file, `header+2` already carries the mesh serial (e.g. `0x08F0` = 2288, matching the runtime mesh
header). Runtime: 8 native sector reads on Final Haunt, 0 geometry native reads. CONFIRMED.

**Stage world facts (runtime dump):** RpWorld layout `+0x08 flags`, `+0x10 RpMaterialList`, `+0x1C rootSector`,
`+0x20 numTexCoordSets`, `+0x34 positioned-light list`, `+0x3C direct-light list`, `+0x68 sectorRenderCB
(0x80460B60)`, `+0x6C pipeline`, `+0x70 VtxFmt plugin`. Loaded world flags are `0x4101000D`: the game has
**cleared rpWORLDLIGHT (0x20)** that the file carries, so sectors collect no lights. World VtxFmt is NULL →
default fmt → positions in world space and no origin matrix. CONFIRMED.

### 4.3 Models: `*.DFF` (not pre-instanced)

Plain RW 3.7 clumps: Geometry Struct + prelit/UV/triangles/morph targets, BinMesh **with** indices, MatFX
(0x120) on atomics and materials, Skin (0x116) with weights and inverse bind matrices, UVAnim dictionaries.
Disc census: 0 Native Data PLG chunks in any DFF; 0 `rwID_GCNMATPLUGIN` (0x129) chunks anywhere.
Instancing happens at the first render (`instanceCB`): `0x804649FC` → `0x8046D124` (recomp_057.cpp:167591)
for static geometry, `0x8044E974` → `0x8046D124` (multi-weight skin) / `0x80451988` (palette skin) for
skins. Alternative instancer `0x8046C7EC` is used only when the geometry has
`rpGEOMETRYNATIVEINSTANCE (0x02000000)` and `[0x805EEE88] == 1`; never on Final Haunt.

`0x8046D124` algorithm: build a GX vertex-descriptor context (`0x8046CF14`, attr order and index widths per
§3.4), size = header(12+8N) + 8·numMeshes + Σ DL sizes + arrays, allocate with
`RwResourcesAllocateResEntry 0x804818C8`, zero it, stamp, `meshSerial = meshHeader.serialNum`, set hdrFlags bit0
(§3.5), write one DL per mesh (`0x8046F5E4`, opcode 0x98 if `geom.flags & rpGEOMETRYTRISTRIP` else 0x90),
write arrays (`0x80470258`, quantising per the fmt), fill attribute records (`0x8047071C`), `DCFlushRange`,
`GXInvalidateVtxCache`. CONFIRMED (code + dump + byte compare).

Disc census of platform-independent data (all stage + character archives, 7,300 geometries): material MatFX
effect types `ENVMAP 1486, DUALTEXTURE 514, UVTRANSFORM 391, DUALUVTRANSFORM 75, BUMPENVMAP 2`;
geometry format low bits dominated by `0x3F` (5,018), `0x73` (841), `0x7B` (438), `0x0F` (428), `0xBB` (353).
Final Haunt subset: `ENVMAP 295, UVTRANSFORM 21`. CONFIRMED (assets).

---

## 5. Matrices

### 5.1 `RwGC_LoadPosNrmMtx 0x804960FC(RwMatrix* ltm_or_NULL, bool normals)` (recomp_063.cpp:155337)

```
V = RwMatrix @ 0x805E428C            // camera view matrix, written by the camera lane
M = ltm ? RwMatrixMultiply(ltm, V) : V   // 0x8047FC08; RW row-vector order: v' = v·LTM·V
GXMtx G (3 rows × 4):
  row0 = ( -M.right.x, -M.up.x, -M.at.x, -M.pos.x )
  row1 = (  M.right.y,  M.up.y,  M.at.y,  M.pos.y )
  row2 = ( -M.right.z, -M.up.z, -M.at.z, -M.pos.z )
GXLoadPosMtxImm(G, GX_PNMTX0); if (normals) GXLoadNrmMtxImm(G, GX_PNMTX0); GXSetCurrentMtx(GX_PNMTX0)
```

i.e. GX eye space = `diag(-1, 1, -1)` × RW camera space (a 180° rotation about Y; handedness preserved). The
normal matrix is the upper 3×3 of the same matrix, not inverse-transposed and not renormalised. CONFIRMED
(code; 91.1 calls/frame = every geometry draw; `GXLoadNrmMtxImm` 67.1/frame = every non-PTank atomic).

### 5.2 Callers

- Default / MatFX render, atomics: `ltm = RwFrameGetLTM 0x804882E8(atomic->frame)`, normals = `flags & 0x10`.
- Sectors: `ltm = NULL` unless the world has a VtxFmt with `posType != GX_F32`, in which case an origin matrix
  is built: identity rotation, `pos = inf − fmod(inf, 1/2^posFrac)` with `inf = sector+0x6C` (tight bbox
  minimum); not live on the disc (all worlds F32). CODE.
- Skin CPU path: `ltm = atomic LTM`. Skin palette path: per-bone matrices (§6.3), no view multiply at load time
  because the palette already includes the view.
- PTank-ID pipeline: `ltm = NULL` (particles emitted in world space).

---

## 6. Skinning (RpSkin, plugin 0x116)

### 6.1 Structures

Skin plugin offsets `[0x8056E7C8+4] = atomic offset 0x7C` (→ `RpHAnimHierarchy*`), `[+8] = geometry offset 0x68`
(→ `RpSkin*`), `[+12] = 0x8078B8C0` skin matrix cache (64-byte `RwMatrix` × bones), `[+40]/[+44]` pipelines.
`RpSkinGeometryGetSkin = 0x8044DCFC`.

| RpSkin off | Type | Field (observed use) | Live values (5 skins) |
|---|---|---|---|
| +0x00 | u32 | numBones | 0x1E, 7, 7, 10, 5 |
| +0x04 | u32 | numUsedBones (loop bound) | 0x1A, 7, 7, 9, 4 |
| +0x08 | u8* | usedBoneIds | |
| +0x0C | RwMatrix* | inverse bind (skin→bone) matrices, 64 B each | |
| +0x10 | u32 | maxNumWeightsForVertex | **2 for all** |
| +0x14 | u32* | vertexBoneIndices | |
| +0x18 | RwMatrixWeights* | vertexBoneWeights | |
| +0x1C | void* | saved bind-pose POS array (CPU path) | set |
| +0x20 | void* | saved bind-pose NRM array (CPU path) | set |
| +0x2C | u32 | platform type (3 forces CPU path) | 2 or 1 |
| +0x34 | u32 | split boneLimit (≠0 → per-mesh bone RLE) | 0 |
| +0x40 | u8* | meshBoneRLECount: per mesh `{u8 start, u8 count}` | |
| +0x44 | u8* | meshBoneRLE: `{u8 firstBone, u8 numBones}` | |

### 6.2 Path selection (identical test in instance, reinstance, render, matrix build)

`cpu = (skin.maxNumWeightsForVertex > 1) || (skin+0x2C == 3)`.

- **CPU path** (live): instance with `0x8046D124` (normal model layout). Reinstance `0x8044EA8C`
  (recomp_015.cpp:151267) every frame: on first use it moves the instanced POS/NRM array pointers into
  `skin+0x1C/+0x20` (or the per-atomic plugin when `atomicplugin byte0 == 1`), allocates new output arrays
  (`0x8044DFB8`), computes bone matrices `0x8044E394`, then skins positions+normals with `0x8044E738`
  (or positions only `0x8044E5A4`) straight into `header.attr[POS/NRM].array`, quantising normals per the
  fmt. Render `0x8044EEC4` (recomp_018.cpp:161874) loads the atomic LTM (§5.1) and draws normally.
  Final Haunt: 4 atomics/frame, `0x8044E738` 4/frame, `0x8044E5A4` 0. CONFIRMED.
- **Palette path** (not live on FH): instance `0x80451988`; render adds `GXSetVtxDesc(GX_VA_PNMTXIDX,
  GX_DIRECT)`; per-vertex `u8 PNMTXIDX` in the DL selects `GX_PNMTX0 + 3·slot`. Without split data it loads
  `usedBone[i]` into slot i (`id = 3i`) once per atomic; with split data (skin+0x34 ≠ 0) it reloads, before each
  mesh's DL, the RLE bone ranges of that mesh into consecutive slots 0,3,6,…. The SDK's GC instancer limit is
  10 bones per split mesh (GX_PNMTX0..9). CODE.

### 6.3 Bone matrix formulas (`0x8044E394(out, skin, LTM, hierarchy)`, recomp_011.cpp:157448)

With `base = cpu ? invert(LTM) : View(0x805E428C)`:
- hierarchy flag `0x2` (NOMATRICES — use frames): `out[b] = invBind[b] × LTM(hier.nodeInfo[b].frame) × base`.
- flag `0x4000` (LOCALSPACEMATRICES): cpu → `out[b] = invBind[b] × hier.matrices[b]`;
  palette → `out[b] = invBind[b] × hier.matrices[b] × (LTM × View)` (via `0x80451DBC`).
- otherwise (global matrices): `out[b] = invBind[b] × hier.matrices[b] × base` (`0x80451DBC`).
Palette load uses the same axis flip as §5.1 (`0x8044EC9C`). CODE.

---

## 7. Other atomic pipelines

- **DMorph (0x122)** and **morph-target interpolation** (`0x80465864`, atomic interpolator dirty bit
  `atomic+0x4C bit0`): 0 hits on Final Haunt; 6 DMorph chunks exist on the disc. Reinstance rewrites arrays
  in place under the draw-sync wait (§3.6). CODE.
- **PTank-ID pipeline (plugin 0x12F)**, 16 atomics/frame: lighting `0x80449D00` calls the atomic lighting cb
  only when the geometry has normals, render `0x80449D54` (recomp_029.cpp:151918) uses **direct** (non-indexed)
  attributes, a NULL world matrix, its own channel logic from plugin flags `[atomic+[0x805F24F0]]+0x44`,
  `GXBegin(GX_QUADS, GX_VTXFMT0, 4·n)` and an emitter callback at plugin `+0x28` (with the PPC quantisation
  SPRs 918/919 set for fast `psq_st` writes). Owned by the effects lane; listed here because it is an RpAtomic
  pipeline and the older "no PTank code" negative result does not hold for the plugin id. CONFIRMED (hits) /
  CODE (layout).
- **GCN material plugin (0x129)**: hook table `0x805E4398` entry 6 = `{6, pluginId 0x129, offset 0x20, size 0xC}`
  (dump); `0x804AA118(material,6)` returns `[material+0x20]`; when non-NULL the MatFX mesh function uses the
  data-driven TEV program `0x804A7A58 → 0x804A7334/0x804A7AAC` (blob layout in RECOMP_VISUAL_MAP/07 §TEV_PROFILE_05).
  0 hits; no asset carries the plugin. CONFIRMED absent.

---

## 8. Lighting (RpLight → GX light objects)

### 8.1 Lighting callbacks

**Atomic `0x80464478`** (recomp_012.cpp:160378; 67.1/frame):
```
inst.amb = (0,0,0); inst.ambA = 1.0; hasAmbient = 0; mask = 0; numLights = 0
if (geom.flags & 0x20 /*LIGHT*/) && curWorld:
    UploadDirectLights(mask=rpLIGHTLIGHTATOMICS(1), inst)                 // 0x8046521C
    ++engine.lightFrame (u16 [RwEngineInstance]+0x0A)
    for tie in atomic->worldSectorTies (list @ atomic+0x64): sector = tie+8
        for lt in sector->lights (list @ sector+0x40): light = lt+8
            if light && light.lightFrame(u16 +0x3C) != engine.lightFrame && (light.flags(+2) & 1):
                light.lightFrame = engine.lightFrame
                if |sphere.center − LTM(light).pos|² < (sphere.radius + light.radius(+0x14))²:
                    UploadPositionedLight(light, inst)                    // 0x80465438
if hasAmbient: clamp each inst.amb channel to ≤ 1.0
```
(`sphere = RpAtomicGetWorldBoundingSphere 0x80457720`.)
**Sector `0x80464E44`** (recomp_026.cpp:154979): same init; if `curWorld.flags & 0x20`:
`UploadDirectLights(2 = rpLIGHTLIGHTWORLD)`, then every light tied to the sector with `flags & 2` (no sphere
test, no lightFrame dedupe). Final Haunt: world flag cleared → sectors are unlit. CONFIRMED.

### 8.2 `UploadDirectLights 0x8046521C(mask, inst)` (recomp_034.cpp:174843; 21.1/frame)

For each light on `curWorld+0x3C` with `light.flags & mask`:
- **type 1 (directional)**: `i = inst.numLights`; `d = RwV3dTransformVector(LTM(light).at, View)`;
  attenuation: plugin `[light+[0x805F25D8]]` (28 B `{u32 custom; f32 a0,a1,a2,k0,k1,k2}`): custom ?
  `GXInitLightAttn(a0..k2)` : `GXInitLightAttn(1,0,0, 1,0,0)`;
  `GXInitLightPos(obj, d.x·2^20, −d.y·2^20, d.z·2^20)` (constant −1048576.0 applied to (−x, y, −z));
  `GXInitLightColor(obj, {trunc(r·255), trunc(g·255), trunc(b·255), 0})` from `light+0x18/1C/20`;
  `GXLoadLightObjImm(obj, 1<<i)`; `mask |= 1<<i; numLights++`. **No 8-light check here.**
- **any other type** (ambient): `inst.amb += light.color.rgb; hasAmbient = 1`.
GX light objects live in `0x805E2760 + 64·i`.

### 8.3 `UploadPositionedLight 0x80465438(light, inst)` (recomp_035.cpp:159310; 0 on FH)

Only if `numLights < 8`: colour as above; `p = RwV3dTransformPoint(LTM.pos, View)`;
`GXInitLightPos(−p.x, p.y, −p.z)`; then by type (`light+1`):
`0x80` point → custom ? `GXInitLightAttn(…)` : `GXInitLightAttnA(1,0,0)` + `GXInitLightDistAttn(radius·0.5, 0.5,
GX_DA_MEDIUM)`; `0x81` spot / `0x82` soft spot → `GXInitLightDir(−a.x, a.y, −a.z)` (transformed `at`), custom ?
`GXInitLightAttn` : `GXInitLightSpot(coneAngle·57.29578, GX_SP_FLAT | GX_SP_COS)` +
`GXInitLightDistAttn(radius·0.5, 0.5, GX_DA_MEDIUM)`; other types load with no attenuation set.
Then load and update mask/count. CODE.

---

## 9. Default material path — `RwGC_MeshMaterialLightTev 0x80466624`

Generated: 0x80466624 recomp_055.cpp:168059; render callback 0x80463964 recomp_043.cpp:199460.
Used by render callbacks `0x80463964` (sectors + default atomics + DMorph), `0x8044EEC4` (skin).
Called **once per object** (not per mesh): `fn = 0x80466624(flags, inst.lightMask, inst.hasAmbient,
header.hdrFlags & 1)`; then per mesh: `RwGC_TextureBind 0x8049AC88(material.texture, GX_TEXMAP0)`;
`RwGC_AlphaTestZCompLoc 0x804987C8(opaque)`; `if (fn) fn(&inst.amb, &material.color, material.surfaceProps.ambient)`;
`GXCallDisplayList(dl, size)`.

`opaque = !(raster && (rasterPlugin[+0x14] & 1))` where `rasterPlugin = raster + [0x805F2700]` and raster =
`texture->raster`; untextured → `opaque = 1`. `0x804987C8(1)`: `GXSetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,
GX_ALWAYS,0)` + `GXSetZCompLoc(GX_TRUE)`; `(0)`: `GXSetAlphaCompare` from the RW render-state cache
(`0x805E42D0 +0x40..+0x4D`) + `GXSetZCompLoc(GX_FALSE)` (cached on `+0x3C`). A NULL texture binds the
raster-less "Im texture" `[0x805F270C]` (RwTexture created at device reset, 0x80786620 in the dump; see 01_RWGC_DEVICE_RASTER_CAMERA_IM2D.md). (Texture/device lanes own those functions.) CONFIRMED.

### 9.1 Inputs

`T = flags & 0x84` (textured / textured2), `P = flags & 0x08` (prelit), `M = flags & 0x40` (modulate),
`L = lightMask != 0`, `X = hasAmbient == 1`, `H = hdrFlags & 1` (vertex alpha).
Constants: `K1 = 0xFFFFFFFF` (`0x805FC250`), `K2 = 0x000000FF` (`0x805FC254`).
Channel-control variables are written as `(enable, ambSrc, matSrc)` with `GX_SRC_REG=0`, `GX_SRC_VTX=1`.

### 9.2 Decision table (exact)

| Case | Condition | Colour registers set here | COLOR0 (enable,amb,mat) | ALPHA0 (enable,amb,mat) | Stages | Continuation |
|---|---|---|---|---|---|---|
| A-L | T P X, L | `ChanMatColor(COLOR0A0,K1)`; `!M: TevColor(REG0,K1)`; `!H: ChanAmbColor(ALPHA0,K2)` | (1, VTX, REG) | H ? (1,VTX,REG) : (0,REG,REG) | 2 | M ? 0x804662DC : 0x804663C0 |
| A-U | T P X, !L | `!M: TevColor(REG0,K1)`; `!H: ChanMatColor(ALPHA0,K2)` | (0, REG, VTX) | H ? (0,REG,VTX) : (0,REG,REG) | 2 | M ? 0x804662DC : 0x804663C0 |
| B-L-M-P | T !(P X), L, M, P | `!H: ChanAmbColor(ALPHA0,K2)` | (1, VTX, REG) | H ? (1,VTX,REG) : (0,REG,REG) | 1 | 0x8046656C |
| B-L-M | T, L, M, !P | `ChanAmbColor(ALPHA0,K2)` | (1, REG, REG) | (0, REG, REG) | 1 | 0x804664D0 |
| B-L-P | T !(P X), L, !M, P | `ChanMatColor(COLOR0A0,K1)` | (1, VTX, REG) | H ? (1,VTX,REG) : (0,REG,REG) | 1 | none |
| B-L | T, L, !M, !P | `ChanMatColor(COLOR0A0,K1)` | (1, REG, REG) | (0, REG, REG) | 1 | 0x80466448 |
| B-U-M-P | T !(P X), !L, M, P | (as B-L-M-P) | (1, VTX, REG) | H ? (1,VTX,REG) : (0,REG,REG) | 1 | 0x8046656C |
| B-U-M | T, !L, M, !P | (as B-L-M) | (1, REG, REG) | (0, REG, REG) | 1 | 0x804664D0 |
| B-U-P | T !(P X), !L, !M, P | `!H: ChanMatColor(ALPHA0,K2)` | (0, REG, VTX) | H ? (0,REG,VTX) : (0,REG,REG) | 1 | none |
| B-U | T, !L, !M, !P | `ChanMatColor(ALPHA0,K2)` | (0, REG, REG) | (0, REG, REG) | 1 | 0x8046659C |
| C-L-P | !T, L, P | `ChanMatColor(COLOR0A0,K1)`; `!M: TevColor(REG0,K1)` | (1, VTX, REG) | H ? (1,VTX,REG) : (0,REG,REG) | 1 | M ? 0x804662DC : 0x804663C0 |
| C-L | !T, L, !P | `ChanMatColor(COLOR0A0,K1)`, `ChanAmbColor(COLOR0A0,K2)`; `!M: TevColor(REG0,K1)` | (1, REG, REG) | (0, REG, REG) | 1 | M ? 0x804662DC : 0x804663C0 |
| C-U-P | !T, !L, P | `!H: ChanMatColor(ALPHA0,K2)`; `!M: TevColor(REG0,K1)` | (0, REG, VTX) | H ? (0,REG,VTX) : (0,REG,REG) | 1 | M ? 0x804662DC : 0x804663C0 |
| C-U | !T, !L, !P | `ChanMatColor(COLOR0A0,K2)`; `!M: TevColor(REG0,K1)` | (0, REG, REG) | (0, REG, REG) | 1 | M ? 0x804662DC : 0x804663C0 |

Always afterwards: `GXSetNumChans(1)`; `GXSetChanCtrl(GX_COLOR0, en, amb, mat, lightMask, GX_DF_CLAMP, GX_AF_SPOT)`;
`GXSetChanCtrl(GX_ALPHA0, en, amb, mat, 0, GX_DF_NONE, GX_AF_NONE)`; `GXSetChanCtrl(GX_COLOR1 / GX_ALPHA1,
0, REG, REG, 0, GX_DF_NONE, GX_AF_NONE)`; `GXSetNumTevStages(n)`; stage 0 ops
`GXSetTevColorOp/AlphaOp(0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV)`.

TEV programs (`out = d + (1−c)·a + c·b`, clamped):

| Program | Used by | Stage 0 | Stage 1 | TexGen / order |
|---|---|---|---|---|
| **TX1** | cases B-* | C `(ZERO, RASC, TEXC, ZERO)` = RASC·TEXC; A `(ZERO, RASA, TEXA, ZERO)` = RASA·TEXA | — | `GXSetNumTexGens(1)`; `TexCoordGen2(TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, FALSE, GX_PTIDENTITY)`; order0 `(TEXCOORD0, TEXMAP0, COLOR0A0)` |
| **TX2** | cases A-* | C `(ZERO, RASC, C0, C1)` = C1 + C0·RASC; A `(ZERO, RASA, A0, A1)` = A1 + A0·RASA; order `(NULL, NULL, COLOR0A0)` | C `(ZERO, CPREV, TEXC, ZERO)`; A `(ZERO, APREV, TEXA, ZERO)`; ops ADD/clamp/PREV; order `(TEXCOORD0, TEXMAP0, COLOR_NULL)` | 1 texgen as above |
| **UT1** | cases C-* | C1 + C0·RASC / A1 + A0·RASA | — | `GXSetNumTexGens(0)`; order0 `(NULL, NULL, COLOR0A0)` |

### 9.3 Continuations (called per mesh; args `amb = &inst.amb (f32×3)`, `col = &material.color (RGBA8)`, `ka = surfaceProps.ambient`)

All float→byte conversions are `fctiwz` (truncate) followed by taking the low byte (no saturation).

| Fn | Generated | GX writes |
|---|---|---|
| 0x804662DC | recomp_049.cpp:132987 | `TevColor(REG0, col)`; `TevColor(REG1, {col.r·amb.r·ka, col.g·amb.g·ka, col.b·amb.b·ka, 0})` |
| 0x804663C0 | recomp_050.cpp:154297 | `TevColor(REG1, {amb.r·255·ka, amb.g·255·ka, amb.b·255·ka, 0})` |
| 0x80466448 | recomp_051.cpp:149531 | `ChanAmbColor(GX_COLOR0, {amb·255·ka, 0})` |
| 0x804664D0 | recomp_052.cpp:165239 | `ChanMatColor(GX_COLOR0A0, col)`; `ChanAmbColor(GX_COLOR0, {amb·255·ka, 0})` |
| 0x8046656C | recomp_053.cpp:171233 | `ChanMatColor(GX_COLOR0A0, col)` |
| 0x8046659C | recomp_054.cpp:166797 | `ChanMatColor(GX_COLOR0, {amb·255·ka, 0})` |

These are genuine per-material colour writers (not "dispatch-table sub-functions" as earlier docs guessed); they
are returned as a function pointer. CONFIRMED (hits per frame: 0x804662DC 2, 0x804663C0 5, 0x80466448 4,
others 0).

---

## 10. MatFX path — render `0x804A62D0`, mesh `0x804A55CC`

### 10.1 Render callback `0x804A62D0` (recomp_037.cpp:163032; 30.1/frame)

Atomic: `ltm = LTM`, fmt = geometry fmt; sector: world fmt / origin matrix as §5.2. `RwGC_SetArrays(fmt, inst)`;
`RwGC_LoadPosNrmMtx(ltm, flags&0x10)`; clear the MatFX state cache `0x805E4388 = {chanValid, lastMaterial,
tevValid} = {0,0,0}`; for each mesh `j`: `0x804A55CC(&mesh[j], &dlRecord[j], object, ltm, inst)`. The skin+MatFX
render `0x80452C1C` (recomp_054.cpp:164731) does the same after its skin matrix handling (it clears the cache
with `0x804A3F58`). CONFIRMED.

### 10.2 Mesh function `0x804A55CC` (recomp_033.cpp:166951; 35.19 meshes/frame)

```
if (0x804AA118(material, 6) /*GCN mat plugin*/) → native shader path 0x804A7A58 (not live)
fx = [material + [0x805F2750] (=0x1C)]          // rpMatFXMaterialData*
switch (fx ? fx->flags(+0x30) : 0):
  1 BUMPMAP        → 0x804A4E00(mesh, dl, ltm, inst, 0)
  2 ENVMAP         → 0x804A48DC(mesh, dl, ltm, inst)          25.1 / frame
  3 BUMPENVMAP     → 0x804A4E00(..., 1)
  4 DUALTEXTURE    → 0x804A3F74(mesh, dl, inst)
  5 UVTRANSFORM    → inline base pass + texture matrix         6 / frame (GXLoadTexMtxImm delta)
  6 DUALUVTRANSFORM→ 0x804A4358(mesh, dl, inst)
  0 / NULL / other → inline base pass                          ~4.1 / frame
```

**Channel block** (identical copy in every branch; executed when `chanValid == 0`, then `chanValid = 1`):

| Condition | GX |
|---|---|
| numLights > 0 | if `!P && !X`: `ChanAmbColor(COLOR0A0, 0x000000FF)`; `NumChans(1)`; `ChanCtrl(GX_COLOR0A0, 1, P?VTX:REG, REG, lightMask, GX_DF_CLAMP, GX_AF_SPOT)`; if `!M`: `ChanMatColor(COLOR0A0, 0xFFFFFFFF)` |
| no lights, P, M | `NumChans(1)`; `ChanCtrl(COLOR0A0, 1, VTX, REG, 0, GX_DF_NONE, GX_AF_NONE)` |
| no lights, P, !M | `ChanCtrl(COLOR0A0, 0, REG, VTX, 0, NONE, NONE)` |
| no lights, !P | if `!X && !M`: `ChanMatColor(COLOR0A0, 0x000000FF)`; `ChanCtrl(COLOR0A0, 0, REG, REG, 0, NONE, NONE)` |

Note `GX_COLOR0A0` (colour and alpha share the setting) — unlike §9, alpha is lit together with colour here.
Constants at `0x805FC4C8` (0xFFFFFFFF) and `0x805FC4CC` (0x000000FF).

**Material colour `0x804A3CE0(inst, material)`** (recomp_024.cpp:150092; runs when `material != lastMaterial`):

| Condition | GX |
|---|---|
| lights > 0 | `X && !P`: `ChanAmbColor(COLOR0A0, {255·ka·amb.rgb, 0xFF})`; `M`: `ChanMatColor(COLOR0A0, col)` |
| no lights, P | `M`: `ChanMatColor(COLOR0A0, col)` |
| no lights, !P, X | `M`: `ChanMatColor(COLOR0A0, {col.rgb·ka·amb.rgb, col.a})`; `!M`: `ChanMatColor(COLOR0A0, {255·ka·amb.rgb, 0xFF})` |
| no lights, !P, !X | `M`: `ChanMatColor(COLOR0A0, {0,0,0,col.a})` |

**Base TEV** (when `tevValid == 0`, then `tevValid = 1`): `NumTevStages(1)`; textured geometry (`flags&0x84`):
`NumTexGens(1)`, `TexCoordGen2(0, MTX2x4, TEX0, GX_IDENTITY, 0, PTIDENTITY)`, `TevOrder(0, TEXCOORD0, TEXMAP0,
COLOR0A0)`, `GXSetTevOp(0, GX_MODULATE)`; untextured: `NumTexGens(0)`, `TevOrder(0, NULL, NULL, COLOR0A0)`,
`GXSetTevOp(0, GX_PASSCLR)`. Then texture bind + alpha test (as §9) + `GXCallDisplayList`.
Blend, Z and cull are **not** touched except where stated below (they come from the RW render-state cache).

### 10.3 Effects

**UVTRANSFORM (5)**: after base TEV, if `fx.data[0].uvAnim.baseTransform (+0x00)` is non-NULL:
`GXLoadTexMtxImm({m.right.x, m.up.x, 0, m.pos.x, m.right.y, m.up.y, 0, m.pos.y}, GX_TEXMTX0, GX_MTX2x4)`,
`TexCoordGen2(0, MTX2x4, TEX0, GX_TEXMTX0, 0, PTIDENTITY)`, `tevValid = 0`. The matrix is animated by
`RpMaterialUVAnimApplyUpdate 0x8045567C` (5 calls/frame on FH). CONFIRMED (counts).

**ENVMAP (2)** `0x804A48DC` (recomp_030.cpp:174766) + continuation `0x804A4B7C` (recomp_031.cpp:180922):
1. base pass as §10.2 but alpha test forced off (`0x804987C8(1)`).
2. env pass: save `SRCBLEND(10)`/`DESTBLEND(11)` via `RwEngineInstance+36`; set `SRCBLEND = fx.env.fbAlpha(+0x0C) ?
   rwBLENDDESTALPHA(7) : rwBLENDONE(2)`, `DESTBLEND = rwBLENDONE(2)` via `+32`;
   `ChanCtrl(COLOR0A0, 0, REG, REG, 0, GX_DF_CLAMP, GX_AF_NONE)`; `ChanCtrl(COLOR1A1, 0, …)`;
   `ChanMatColor(COLOR0A0, coef<1 ? {k,k,k,255} with k=trunc(coef·255.9) : {255,255,255,255})` (coef = +0x08);
   `NumTexGens(1)`, `TexCoordGen2(0, MTX2x4, GX_TG_NRM, GX_TEXMTX0, 0, PTIDENTITY)`, `NumTevStages(1)`,
   `GXSetTevOp(0, MODULATE)`, `TevOrder(0, TEXCOORD0, TEXMAP0, COLOR0A0)`; invalidate all three caches;
   bind `fx.env.texture (+0x04)` on TEXMAP0;
   `E = fx.env.frame(+0x00) ? orthonormalize(invert(LTM(frame))) : View`; if `ltm`: `E = orthonormalize(ltm × E)`;
   `GXLoadTexMtxImm({−½E.right.x, −½E.up.x, −½E.at.x, ½,  −½E.right.y, −½E.up.y, −½E.at.y, ½}, GX_TEXMTX0,
   MTX2x4)`; `GXCallDisplayList` (same DL); restore blend states.
   So `u = ½ − ½·(n·E_x)`, `v = ½ − ½·(n·E_y)` with `n` the model-space vertex normal (s8/64). CONFIRMED (25.1/frame).

**DUALTEXTURE (4)** `0x804A3F74` + `0x804A4240`: base pass; save blend; set `SRCBLEND/DESTBLEND` from
`fx.dual.srcBlend(+0x04)/dstBlend(+0x08)`; if `flags&0x80`: `TexCoordGen2(0, MTX2x4, GX_TG_TEX1, IDENTITY)`,
`tevValid=0`; bind `fx.dual.texture(+0x00)`; alpha test from its raster; same DL; restore. CODE.

**DUALUVTRANSFORM (6)** `0x804A4358` + `0x804A47F4`: pass 1 with `data[0].baseTransform` texmtx on TEX0; pass 2
with `data[0].dualTransform (+0x04)` (on TEX1 if `flags&0x80`, else TEX0; identity if NULL), blend from
`data[1].dual {texture +0x18, src +0x1C, dst +0x20}`. CODE.

**BUMPMAP (1) / BUMPENVMAP (3)** `0x804A4E00` (recomp_032.cpp:191538): channels + material colour as §10.2;
bind `fx.bump.bumpedTexture(+0x04)` on TEXMAP0; alpha test off; `s = −invBumpWidth(+0x10)·coef(+0x0C)`;
`B = frame(+0x00) ? ortho(invert(LTM)) : View`, `× ltm`; `TEXMTX0 = {s·B.right.x, s·B.up.x, s·B.at.x, 0,
s·B.right.y, s·B.up.y, s·B.at.y, 0}`. Bump-env additionally binds `data[1].env.texture(+0x1C)` on TEXMAP1 and loads
`GX_TEXMTX1 = env matrix (−½…, ½)` from `data[1].env.frame(+0x18)`.
Texgens: c0 = TEX0/IDENTITY, c1 = NRM/TEXMTX0, [c2 = NRM/TEXMTX1].
Stages:

| St | Order | Colour | Alpha | Other |
|---|---|---|---|---|
| 0 | (c0, map0, COLOR0A0) | TEXC·RASC → **TEVREG0** | (1 − TEXA)·KONST, KASEL_1 → PREV | |
| 1 | (c1, map0, COLOR_NULL) | C0·APREV | TEXA | `GXSetTevIndirect(1, IND0, ITF_8, ITB_NONE, ITM_OFF, ITW_OFF, ITW_OFF, addPrev=TRUE, utcLod=FALSE, ITBA_OFF)` |
| 2 | bump: (NULL,NULL,COLOR0A0); env: (NULL,NULL,NULL) | CPREV + C0·APREV | bump: RASA; env: fbAlpha(+0x24) ? KONST·APREV : KONST, KASEL_K0_A, `KColor0.a = coef<1 ? trunc(coef·255.9) : 255` | |
| 3 | env only: (c2, map1, COLOR0A0) | CPREV + TEXC·APREV | RASA | |

`NumTevStages(3 or 4)`; `GXCallDisplayList`; `GXSetTevDirect(1)`. CODE (0 hits on FH).

---

## 11. Final Haunt measurements

### 11.1 Per frame (window 40e9→45e9, 616 frames, deterministic across two run pairs)

| Quantity | /frame | Source hook |
|---|---|---|
| World sector render callbacks / non-empty sectors drawn | 15 / 8.00 | 0x80460B60 / 0x80464FF4 |
| Atomic pipeline executions | 83.10 | 0x80456F04 = 0x80464AAC |
| … default atomic / MatFX / skin / skin+MatFX / PTank-ID | 33 / 30.1 / 2 / 2 / 16 | render cbs |
| CPU-skinned atomics (`0x8044E738`) / palette-skinned | 4 / 0 | |
| Instancing calls in window (all pipelines) | 0 | 0x8046D124 etc. (all instancing happened before 40e9: 49 calls of 0x8046D124 for static and CPU-skinned geometry, 6 skin + 4 skin+MatFX instance callbacks, 36 MatFX instance callbacks) |
| `0x80466624` calls (objects) / MatFX meshes | 43 / 35.19 | |
| ENVMAP meshes (2 DL passes) / UVTRANSFORM meshes / other MatFX meshes | 25.1 / 6 / 4.09 | |
| `GXCallDisplayList` (all are RW geometry pipelines) | 176.29 | 16 static call sites, all in RwGC |
| … sectors (73 meshes) / default atomics / skin / MatFX base / env pass | 73 / 41 / 2 / 35.19 / 25.1 | derived (sum = 176.29) |
| Display-list bytes submitted | **110,447** (sectors 35,776) | host `display_list_bytes` delta |
| `RwGC_LoadPosNrmMtx` / `GXLoadNrmMtxImm` | 91.1 / 67.1 | |
| Atomics collecting world lights / `GXLoadLightObjImm` / positioned lights | 21.1 / 18.1 / 0 | |
| `GXLoadTexMtxImm` | 31.1 | |
| `RwCameraBeginUpdate` (all passes, all systems) | 32 | |

### 11.2 Resident instance data (dump at 45e9)

| Kind | Entries | DL bytes | Notes |
|---|---|---|---|
| World sectors (native, RG1) | 8 | 35,776 | 1-16 meshes each, 1-54 strips per mesh, POS idx16/idx8, CLR0 idx8, TEX0 idx8/16 |
| Static model geometries | 20 | 8,224 | 1-2 meshes, single strip, all indices idx8 (≤231 verts) |
| Skinned geometries | 5 | 19,232 | CPU path; 1382-vertex model uses idx16 (13,184-byte DL mesh) |

Disc-wide pre-instanced world data: 3,092 sectors, 15.1 MiB of display lists, 37.96 MiB of sector data.

### 11.3 Live TEV/channel configurations (geometry only)

| Id | Pipeline / case | Colour equation (per pixel) | Texgen | /frame | Evidence |
|---|---|---|---|---|---|
| G1 | default B-U-P, H=1 (sectors + prelit unlit models, flags 0x0D/0x1F) | `C = vtx.rgb·tex.rgb`, `A = vtx.a·tex.a` | TEX0 | 32 (8 sectors + 24 atomics) | 43 − other cases |
| G2 | default B-L (flags 0x37) | `C = K1·clamp(amb·ka + ΣN·L)·tex`, `A = tex.a` | TEX0 | 4 | cont 0x80466448 |
| G3 | default A-L (flags 0x3F, hasAmbient; A-U if no directional was loaded for it) | `C = (amb·ka + clamp(vtx + ΣN·L))·tex`, `A = vtx.a·tex.a` (2 stages, all normalised) | TEX0 | 1 | TevColorIn 44 − 43 |
| G4 | default C-U-P, !M (flags 0x1B) | `C = 0 + K1·vtx` (REG1 = 0 unlit) | none | 4 | cont 0x804663C0 5 − 1 |
| G5 | skin C-L, M (flags 0x73) | `C = col·amb·ka + col·clamp(K2 + ΣN·L)`, `A = col.a` | none | 2 | cont 0x804662DC |
| M1 | MatFX base, textured, channel per §10.2 (lit 0x77 / prelit 0x1F) | `GX_MODULATE` | TEX0 | ≈29.2 | 35.19 − 6 |
| M2 | MatFX UVTRANSFORM base | `GX_MODULATE` | TEX0 × TEXMTX0 | 6 | texmtx delta |
| M3 | MatFX ENVMAP pass 2 | `C = coef·envtex`, blend ONE/ONE (or DESTALPHA/ONE) | NRM × TEXMTX0 | 25.1 | 0x804A48DC |
| P1 | PTank-ID effects | MODULATE / PASSCLR, direct vertices | TEX0 | 16 | 0x80449D54 |

Counting distinct combiner programs: 4 (TX1 ≡ MODULATE, TX2, UT1, PASSCLR-equivalent); with channel and
texgen variants about 9 full configurations are live. Channel variants inside M1 (lit vs prelit) are not
separately counted by hooks (INFERRED from the atomics' geometry flags in the dump: MatFX atomics carry flags
0x77, 0x3F and 0x1F).

---

## 12. Re-implementation notes (the cut line)

**Intercept.** Replace the four callbacks of all eight AllInOne nodes (or the two node bodies) — i.e. the whole
contents of `RxPipelineExecute` for pipeline plugin ids 0x2, 0x116, 0x120, 0x122 (0x12F belongs to the effects
lane). Keep `RpWorldRender`, `RpClumpRender`, the atomic/sector render callbacks (guest and game), frustum
tests and the game callbacks above; they decide *what* is drawn and mutate light/material/flag state per atomic.

**Must reproduce (semantics that game data depends on):**
1. Per submitted object, read `geometry/world flags`, material colour, `surfaceProps.ambient`, texture and
   MatFX data **at submit time** (game callbacks change them between atomics of the same frame).
2. Vertex data decoding exactly as instanced: s8/64 normals, RGBA4 colours `(v<<4)|v` per channel for models,
   RGBA8 for worlds; UVs as f32. To match console output byte-for-byte, quantise from the platform-independent
   geometry with the §3.5 rules (or read the instanced arrays). World sectors must be read from the RG1 native
   blob (no PI copy exists).
3. The transform convention (§5.1) including `diag(−1,1,−1)`, non-normalised normal matrix, and directional
   lights at `2^20 · (x,−y,z)` of the view-space `at` vector.
4. Lighting: ambient accumulation clamp to 1.0, `GX_DF_CLAMP` diffuse, alpha channel rules per §9.2/§10.2,
   8-light cap (positioned only), sphere test for positioned lights, `GX_DA_MEDIUM` with `ref = radius·0.5`.
5. The exact colour paths: TEV REG0/REG1 vs channel material/ambient registers, truncation (`fctiwz`, low
   byte) of all float→byte colour products, the 2-stage prelit+ambient program, and MatFX's use of
   `GX_COLOR0A0` (alpha lit with colour).
6. Draw order inside an object: meshes in `RpMeshHeader` order; MatFX multi-pass effects re-draw the same
   mesh immediately (env pass right after its base pass) with the blend states listed; blend/Z/cull/fog/alpha
   compare otherwise come from the RW render-state cache at the time of the draw.
7. Alpha test / Z-compare location per mesh: textures whose raster plugin flag bit0 is set get the RW
   alpha-test state and late Z; others get "always" and early Z (§9).
8. Skinning results: CPU path bone math of §6.3 (including `invBind × bone × invert(LTM)` and drawing with the
   atomic LTM). A GPU implementation is fine as long as it matches that math and the s8 normal quantisation
   (the guest quantises skinned normals too).
9. UV transform and env/bump texture matrices exactly as §10.3, texgen from the model-space (quantised) normal.

**Can be dropped (exist only for GX/Flipper limits):**
- Display lists, 32-byte padding, index-width selection, per-attribute dedup: convert once per resource entry to
  a native VB/IB (one draw per mesh; restart strips or convert to lists).
- `drawSyncStamp`, `GXSetDrawSync`/`GXReadDrawSync` waits, `DCFlushRange`, `GXInvalidateVtxCache`.
- `GX_VTXFMT0`/VAT fractional bits (keep only the decoded values), the RGBA4/S8 storage itself (decode to float).
- The MatFX state cache `0x805E4388` and the 0x80466624 "call once per object" optimisation (compute state per
  mesh natively; results are identical because the inputs are per object except colour, which is per mesh).
- Matrix-palette splitting to 10 bones and the RLE reload order (only the resulting per-vertex matrix matters).
- The resource-arena LRU/eviction of instance data (keep it alive while the geometry lives; re-instance when
  `meshSerial` changes or the geometry is locked).

**Keep byte-for-byte (data formats a loader must parse):** the `.one`/PRS container (§4.1), RW 3.7 chunk
streams, the Native Data PLG struct and header (§4.2, §3.1), BinMesh without indices, and the RpSkin / MatFX /
VtxFmt / light plugin layouts listed in the CSV.

---

## 13. Corrections to existing documents

1. RECOMP_VISUAL_MAP/06 and /07: **0x804A55CC is the MatFX *mesh* function, not the world-sector callback**;
   world sectors use `0x80463964` (default) and would use `0x804A62D0` only with the MatFX sector pipeline
   (0 on FH). 0x804A62D0 is the MatFX render callback for atomics *and* sectors.
2. /06 flavour-node roles were wrong: `0x804A3F74` = DUALTEXTURE, `0x804A4358` = DUALUVTRANSFORM,
   `0x804A48DC` = ENVMAP, `0x804A4E00` = BUMPMAP/BUMPENVMAP (not env map), UVTRANSFORM is inline in 0x804A55CC.
   `0x804A4B7C`, `0x804A4240`, `0x804A47F4` are the second-pass continuations of env / dual / dual-UV
   (bctr-split tails), not independent nodes.
3. /07 TEV_PROFILE_04 describes the bump/bump-env program (§10.3), not the env map; the env map is a 1-stage
   additive second pass with `GX_TG_NRM × TEXMTX0`.
4. /06 and /20: **0x805E4398 is not a hook table** but the GC plugin registry for native material data;
   entry 6 = GCN material plugin 0x129 at material+0x20.
5. /02, /06, /20: the "runtime-registered roots" are the AllInOne private-data callbacks; `0x80452B70` is the
   skin+MatFX **reinstance** callback, `0x80452ABC` its instance callback, `0x804A60A8/6174/6254` MatFX
   instance/reinstance/sector-instance callbacks. They are materialised by `lis/addi` in the constructors.
6. /02 §5: `0x804662DC/0x804663C0/0x80466448/0x804664D0/0x8046656C/0x8046659C` are per-material continuations
   returned by `0x80466624` (§9.3); `0x80449D54` is the PTank-ID atomic render callback, not a skin/morph
   "lit pipeline"; `0x8044960C` creates pipeline plugin 0x12F.
7. /02 view matrix: the lighting and matrix code read `0x805E428C` (not `0x805E429C`).
8. /20 negative result "no RW PTank": plugin-id 0x12F pipeline and its render callback exist and are live
   (16 atomics/frame); only the RpPTank *API/strings* are absent.
9. /05: `0x80463C80` is called by four render callbacks plus the MatFX and skin+MatFX ones; the attribute list is
   in the instance header at `repEntry+24+0x0C`, 8 bytes per record with the pointer first (the old
   "+0x0C arrayPtr, +0x11 stride, +0x12 vcdType" offsets are header-relative offsets of record 0).
10. GLOBAL_GEOMETRY_PIPELINE_AUDIT §4 "the game does use the matrix palette": the code exists, but on Final
    Haunt every skin has `maxNumWeightsForVertex = 2` and is CPU-skinned (0 palette draws).
11. RW37SDK/SDK_USABLE_PARTS.md "Shadow's DFFs are NOT pre-instanced" is true for DFFs, but **stage worlds are**
    (`*.RG1`, Native Data PLG platform 6) — 3,092 native sectors on the disc.
12. `RECOMP_VISUAL_MAP/db/functions.csv` `file:line` columns (and every RECOMP_VISUAL_MAP doc citation derived from
    them) are stale against the current `build-native/generated` tree (e.g. 0x80463964 is now
    recomp_043.cpp:199460, not :153385); the function-to-TU assignment is unchanged. Locate bodies by the
    `// 0xADDR` header comment.

---

## 14. Open questions

1. `RpGameCubeVtxFmt+0x0B` (always 1) — meaning not established.
2. Channel-variant split of MatFX base passes per frame (lit vs prelit) is inferred from geometry flags, not
   hooked; a per-branch counter inside 0x804A55CC would settle it.
3. The palette-skin instancer `0x80451988` output (PNMTXIDX value encoding `3·slot` is inferred from the
   render loop) — no live palette skin on Final Haunt to dump; needs a stage/character with single-weight skins.
4. Exact normal quantisation inside the CPU skinning writer `0x8044E738` (assumed identical to the instancer's
   truncation) — verify with a second dump comparing re-skinned normals against float math.
5. Which system writes the view matrix `0x805E428C` for each of the 32 camera passes per frame (camera/device lane).
6. `0x804987C8` render-state cache fields `0x805E42D0+0x40..+0x4D` (alpha-test function/ref pairs) — device lane.
7. PTank-ID plugin layout (`0x805F24F0` offset, flags +0x44, emitter +0x28, constant colour +0xC4) — effects lane.
8. The native-shader material blob (0x129) is supported by code but unused by any asset; confirm no runtime
   creator exists (no writer to material+0x20 found in this pass).

# 01 — The RwGC device contract: engine globals, standard functions, render states, rasters/textures, cameras, EFB copies, Im2D/Im3D

Lane: **device**. Game: Shadow the Hedgehog, GameCube PAL (GUPP8P), RenderWare Graphics 3.7 GameCube driver ("RwGC").
Companion data file: `RENDERER_RE/01_rwgc_device_slots.csv` (166 rows: every RwGlobals slot, every stdFunc id, every
fpSystem request, every RwRenderStateSet case, the driver-private entry points, the RW core wrappers and the GX sinks, each
with address, generated file:line, semantics, GX meaning, Final Haunt per-frame count and confidence).

Scratch evidence (all reproducible): `RENDERER_RE/_work/device/` — `dis.py` (annotated disassembly), `rd.py` (static DOL
reader), `scan_devcalls.py` + `devcall_sites.csv` (static census of every call through the engine pointer),
`txd_census.py` / `one_txd_census.py` (+ CSVs: native texture census of the whole disc, PRS-decompressing the ONE
archives), `mem.py` / `decode.py` (MEM1 reader for the Run-C savestate), `watches.txt`, `runs/` (logs, JSON reports,
`delta.txt`, `A40_first8.txt`).

Confidence tags: **CONFIRMED** = read in code and observed at runtime (hook counts, first-hit register samples, or live
memory); **CODE** = read in code only; **INFERRED** = GX/RW semantics or reasoning without direct observation.
Generated line numbers are from the CURRENT corpus (`GekkoForge/build-native/generated/fnindex.txt`, `_body` line + 1).

---

## 0. Measurement set-up (what "per frame on Final Haunt" means here)

Three headless runs from a private copy of `scratch/stage-boot-2026-09-16/exe/ShadowTheHedgehog.exe`, save-data recreated
from the seed before each run, navigation only by `--boot-stage stg0604`:

| Run | Budget | Instrumentation | Used for |
|---|---:|---|---|
| A40 | 40e9 | 102 `--watch` hooks (all device fps, all stdFuncs, all RenderStateSet case handlers, driver internals, GX sinks) + `--dump` of driver globals | totals at 40e9, first 8 register samples per hook |
| B50 | 50e9 | same hooks + dumps of RwGlobals, glare camera-texture rasters, Im texture, white raster | totals at 50e9, live object layouts |
| C45 | 45e9 | `--contexts 0x8048652C` (distinct RwCamera* passed to RwCameraBeginUpdate) + `--save-state` | full MEM1 image in-stage (cameras, rasters, caches) |

**Per-frame numbers = (B50 − A40) / (ShowRaster hits B50 − A40) = Δ / 1232 frames**, i.e. a 10e9-instruction window of
steady in-stage Final Haunt gameplay (no rasters created or destroyed, no native texture reads in the window). The counts
reconcile exactly (e.g. GXSetViewport 129 = 32 camera begins + 89 Im2D begins + 8 clear quads; GXSetProjectionv 218 = 32 +
2×89 + 8; GXLoadPosMtxImm 188.1 = 91.1 world-matrix loads + 89 Im2D + 8 clears), which also proves that every hook fired on
every call, including the jump-table case fragments (the recompiler dispatches `bctr` targets through `fn_X` thunks that run
`try_hle`).

---

## 1. Executive summary

1. **0x805F265C is not the device.** It is `RwEngineInstance` (r13+24924), a pointer to the heap-allocated `RwGlobals` at
   **0x8060E340** (live, stable across runs). The RW `RwDevice dOpenDevice` is embedded at RwGlobals+0x10; the static
   template the driver registers is **0x8056F50C**. Every "device +0x20" citation in older docs is an RwGlobals offset.
   (CONFIRMED)
2. **Standard functions**: `0x8051D400` is a 27-entry `{u32 id, fn}` table (ends at 0x8051D4D8) copied into
   `RwGlobals.stdFunc[29]` (+0x48) by fpSystem request 11. Ids 0 and 22 are absent and hold the FALSE stub 0x80494CCC.
   All 29 live slots were dumped from MEM1. (CONFIRMED)
3. **Im3D does not use the device.** `fpIm3D*` (+0x38..+0x44) are NULL in the live globals; Im3D runs through two RxPipeline
   nodes (`ImmInstance.csl` 0x804A0B4C, `SubmitNoLight.csl` 0x804A0B68). Final Haunt in-stage Im3D usage: **0**
   (49,912 calls happen before 40e9, during boot/menus). (CONFIRMED)
4. **Im2D is one entry**: only `fpIm2DRenderPrimitive` (0x80491E80) is live — **89 calls/frame**, 87 textured. Line,
   triangle and indexed variants: 0 hits. Game code uses TRISTRIP almost exclusively (16 of 19 static sites; 2 POLYLINE, 1 indexed TRILIST). (CONFIRMED)
5. **RwRenderStateSet (0x80498954)** is a 31-way jump table over a private cache at **0x805E42D0** (0x50 bytes). It emits GX
   only on change for z/blend/cull/fog, never for texture states (those are written into a hidden "Im texture"
   0x80786620 and applied at the next Im2D/Im3D primitive). **615 calls/frame**; SRCBLEND/DESTBLEND 136 each. (CONFIRMED)
6. **No-op states on GC**: VERTEXALPHAENABLE (12), TEXTUREPERSPECTIVE, SHADEMODE (only reports GOURAUD), BORDERCOLOR,
   FOGDENSITY, all stencil states. ~40 calls/frame land on these. **Blending is always `GX_BM_BLEND`**; "opaque" is
   ONE/ZERO. **Z test off = compare ALWAYS with the compare still enabled** (z-write keeps working). (CONFIRMED)
7. **Cull mapping flips winding**: RW CULLNONE/BACK/FRONT (1/2/3) → `GXSetCullMode(value−1)` = NONE/FRONT/BACK.
   (CONFIRMED)
8. **Alpha test is a driver extension, not RW state**: `0x804987C8(mode)` toggles alpha-test+late-Z vs no-alpha-test+early-Z;
   `0x80498F74(comp0,ref0,op,comp1,ref1)` stores a full GXSetAlphaCompare but **re-emits the previous compare** when the test
   is enabled — the new compare reaches GX only at the next mode change. Runtime-observed (GREATER/1 emitted while 200 was
   cached). A byte-exact renderer must track *emitted* GX state, not the RW cache. (CONFIRMED)
9. **Texture bind chokepoint 0x8049AC88** (273 calls/frame): re-runs `GXInitTexObj[CI]`/`GXInitTexObjLOD` only when the
   texture is dirty or its filterAddressing low 16 bits changed (36 re-inits/frame), loads the TLUT into the slot with the
   **same index as the texmap**, and always calls `GXLoadTexObj`. Palettised textures cannot use trilinear
   (RW filter 5/6 → `GX_LIN_MIP_NEAR`). (CONFIRMED)
10. **Native textures on this disc**: 10,164 textures (1,735 loose TXDs + 8,429 in 379 PRS-compressed ONE archives), every one
    platform 6, **1 mip level, filterAddressing 0x1101 (NEAREST/WRAP/WRAP), aniso 0, LOD bias 0**, formats **C4, C8, CMPR,
    RGB565, RGB5A3** only (TLUT RGB565 or RGB5A3). RGBA8 exists only as camera-texture rasters (EFB copies). Final Haunt
    in-stage GX census: CMPR (20 maps), C4 (83), C8 (6), RGBA8 (4 copy targets), RGB565 (1 = the white 4×4). (CONFIRMED)
11. **Cameras**: one RwCamera in-stage (0x80786880, frame buffer raster 0x80786A60 = 640×480 `rwRASTERTYPECAMERA`,
    DONTALLOCATE, no Z raster). **32 camera begin/end pairs per frame.** Begin builds the GX view matrix =
    inverse(LTM) × viewOffset shear, flips X and Z when loading it (RW right-handed/+Z forward → GX −Z forward), loads a
    perspective (or ortho for rwPARALLEL) projection from recipViewWindow/near/far, and sets viewport = scissor = camera raster
    rectangle. End only clears a pointer. (CONFIRMED)
12. **Clears are geometry, not copy-clears**: `[0x805F2688]` (clear-on-copy flag) is 0, so RwCameraClear draws a full-rect
    `GX_QUADS` at z=0.99999994 with blend ONE/ZERO (**8 clears/frame**: 1 main IMAGE|Z + 7 glare-ladder IMAGE). All
    `GXCopyDisp`/`GXCopyTex` run with clear = FALSE. (CONFIRMED)
13. **EFB copies**: 9 `GXCopyTex`/frame, all through `0x80496208` (PreGlare 1 + PostGlare capture 1 + ladder 7), copy filter
    disabled during the copy, destination = parent camera-texture raster (RGBA8). **Present**: 1 `GXCopyDisp`/frame through
    stdFunc[20] 0x80495CBC, which **ignores its raster argument** and alternates two XFBs (0x8064E900 / 0x806E4900) under a
    GXDrawSync token queue. (CONFIRMED)
14. **Render mode in-stage** (0x805E4250): EURGB60 interlaced, fbWidth 640, efbHeight 480, xfbHeight 480, no AA, no field
    rendering, vfilter {8,8,10,12,10,8,8}; two-pass half-EFB mode `[0x805F2698]` = 0; pixel format RGB8_Z24 (no destination
    alpha). (CONFIRMED)

---

## 2. Engine and device objects

### 2.1 `RwEngineInstance` → `RwGlobals` (0x8060E340)

`0x805F265C` (sdata, r13+24924) holds the RwGlobals pointer; the pointer is written by RwEngineInit/Open
(0x804871BC / 0x80487254 / 0x804872E8 / 0x80487548). Layout = RW 3.7 release `RwGlobals` (no RWDEBUG fields); every value
below is from the Run-C MEM1 image (identical in the B50 dump).

| Off | Field | Live value / meaning | Conf |
|---|---|---|---|
| +0x000 | `void *curCamera` | set by 0x804863A4 (camera begin), NULL after 0x80486370 | CONFIRMED |
| +0x004 | `void *curWorld` | set by RpWorld begin override 0x80461B6C | CONFIRMED |
| +0x008 | `u16 renderFrame` | incremented per camera begin (0x80461B6C) | CONFIRMED |
| +0x00A | `u16 lightFrame` | light dedup stamp (RpLight+0x3C compares) | CODE |
| +0x010 | `RwDevice dOpenDevice` (0x38 bytes) | see 2.2 | CONFIRMED |
| +0x048 | `RwStandardFunc stdFunc[29]` | see 2.3 | CONFIRMED |
| +0x0BC | `RwLinkList dirtyFrameList` | frame sync list | CODE |
| +0x0C4 | `RwStringFunctions` (17 fps, to +0x107) | 0x803AA248 sprintf … | CONFIRMED |
| +0x108 | `rwmalloc` | 0x80050E28 (game trampoline 0x80049BB8 adds 0x32 to the hint) | CONFIRMED |
| +0x10C | `rwfree` | 0x80050EF0 (trampoline 0x80049B5C) | CONFIRMED |
| +0x110 / +0x114 | `rwrealloc` / `rwcalloc` | 0x80050D3C / 0x80050DDC | CONFIRMED |
| +0x118 / +0x11C | `memoryAlloc` / `memoryFree` (free lists) | 0x80486C1C / 0x80486C4C | CONFIRMED |
| +0x120 | `RwMetrics *metrics` | NULL | CONFIRMED |
| +0x124 | `engineStatus` | 3 = rwENGINESTATUSSTARTED | CONFIRMED |
| +0x128 | `resArenaInitSize` | 0x40000 | CONFIRMED |

### 2.2 `RwDevice` (static template 0x8056F50C → RwGlobals+0x10)

| RwGlobals off | Field | Address / value | Generated | Final Haunt / frame | Conf |
|---|---|---|---|---:|---|
| +0x10 | gammaCorrection | 1.0f | — | — | CONFIRMED |
| +0x14 | fpSystem | **0x80494EDC** | recomp_038.cpp:161015 | 2.00 | CONFIRMED |
| +0x18 | zBufferNear | 0.0f (RwIm2DGetNearScreenZ) | — | — | CONFIRMED |
| +0x1C | zBufferFar | 0x3F7FFFFF = 0.99999994 (RwIm2DGetFarScreenZ) | — | — | CONFIRMED |
| +0x20 | fpRenderStateSet | **0x80498954** | recomp_030.cpp:174157 | 615.39 | CONFIRMED |
| +0x24 | fpRenderStateGet | **0x804984C4** | recomp_005.cpp:158632 | 181.19 | CONFIRMED |
| +0x28 | fpIm2DRenderLine | 0x80491D0C | recomp_042.cpp:170817 | 0 | CONFIRMED |
| +0x2C | fpIm2DRenderTriangle | 0x80491B08 | recomp_041.cpp:155542 | 0 | CONFIRMED |
| +0x30 | fpIm2DRenderPrimitive | **0x80491E80** | recomp_043.cpp:203657 | 89.00 | CONFIRMED |
| +0x34 | fpIm2DRenderIndexedPrimitive | 0x8049245C | recomp_044.cpp:177693 | 0 | CONFIRMED |
| +0x38..+0x44 | fpIm3DRenderLine/Triangle/Primitive/IndexedPrimitive | **NULL** (static and live) | — | — | CONFIRMED |

`0x804960F0` returns &0x8056F50C (`_rwDeviceGetHandle`; callers REGISTER 0x804950A0 and 0x80487254).

### 2.3 Standard-function table 0x8051D400 → `stdFunc[]`

Installer: fpSystem request 11 (`rwDEVICESYSTEMSTANDARDS`, 0x804954F8) walks `{id, fn}` pairs; first-hit sample
`r3=0x0B r4=0x8060E388 (=RwGlobals+0x48) r6=0x1D (29)`. RW API wrappers call `RWSRCGLOBAL(stdFunc[id])` as
`fn(pOut, pInOut, nI)`.

| id | RW name | Driver fn | Generated | / frame | Semantics (GX meaning) |
|---:|---|---|---|---:|---|
| 0 | NASTANDARD | 0x80494CCC | recomp_035.cpp:170658 | — | `return FALSE` stub (not in table) |
| 1 | CAMERABEGINUPDATE | 0x804956F4 | recomp_059.cpp:147172 | 32.00 | §5.2 |
| 2 | RGBTOPIXEL | 0x80492BFC | recomp_047.cpp:169807 | 0 | pack RwRGBA for raster format |
| 3 | PIXELTORGB | 0x80492F10 | recomp_051.cpp:152716 | 0 | unpack |
| 4 | RASTERCREATE | 0x80497CA8 | recomp_060.cpp:151869 | 0 (358 total) | §4.4 |
| 5 | RASTERDESTROY | 0x80498058 | recomp_063.cpp:155889 | 0 (74 total) | §4.4 |
| 6 | IMAGEGETRASTER | 0x80493A40 | recomp_011.cpp:167443 | 0 | RwImage from raster |
| 7 | RASTERSETIMAGE | 0x80494614 | recomp_023.cpp:153588 | 0 (1 total) | raster from RwImage |
| 8 | TEXTURESETRASTER | 0x8049819C | recomp_001.cpp:150315 | 29.10 | `tex->raster=r4; texExt+0x20 = 0x01000000` (dirty) |
| 9 | IMAGEFINDRASTERFORMAT | 0x804948D8 | recomp_031.cpp:178987 | 0 | |
| 10 | CAMERAENDUPDATE | 0x80495CAC | recomp_060.cpp:151807 | 32.00 | `[0x805F26F8]=NULL; return TRUE` — no GX |
| 11 | SETRASTERCONTEXT | 0x8049A570 | recomp_006.cpp:154051 | 0 | `[0x805F2710]=raster` |
| 12 | RASTERSUBRASTER | 0x804981B8 | recomp_002.cpp:178725 | 2.00 | copy stride/depth/cType/cFormat, cpPixels=0 |
| 13 | RASTERCLEARRECT | 0x8049A508 | recomp_004.cpp:153226 | 0 | 0x8049A124 on context raster |
| 14 | RASTERCLEAR | 0x8049A52C | recomp_005.cpp:158748 | 0 | same, full rect |
| 15 | RASTERLOCK | 0x80496CA4 | recomp_034.cpp:185891 | 0 (3 total) | §4.4 |
| 16 | RASTERUNLOCK | 0x80497104 | recomp_046.cpp:161378 | 0 (3 total) | §4.4 |
| 17 | RASTERRENDER | 0x80499A9C | recomp_056.cpp:152186 | 0 | §6.1 |
| 18 | RASTERRENDERSCALED | 0x80499AEC | recomp_058.cpp:152554 | 0 | §6.1 |
| 19 | RASTERRENDERFAST | 0x80499AC4 | recomp_057.cpp:174661 | 0 | §6.1 |
| 20 | RASTERSHOWRASTER | **0x80495CBC** | recomp_061.cpp:156814 | 1.00 | §6.3 present |
| 21 | CAMERACLEAR | **0x80495638** | recomp_058.cpp:152189 | 8.00 | §5.6 |
| 22 | HINTRENDERF2B | 0x80494CCC | — | — | not provided |
| 23 | RASTERLOCKPALETTE | 0x804972B8 | recomp_048.cpp:172471 | 0 | |
| 24 | RASTERUNLOCKPALETTE | 0x80497388 | recomp_049.cpp:141373 | 0 | |
| 25 | NATIVETEXTUREGETSIZE | 0x8049A57C | recomp_007.cpp:167211 | 0 | |
| 26 | NATIVETEXTUREREAD | **0x8049A8DC** | recomp_009.cpp:161831 | 0 (335 total) | §4.7 |
| 27 | NATIVETEXTUREWRITE | 0x8049A604 | recomp_008.cpp:162134 | 0 | |
| 28 | RASTERGETMIPLEVELS | 0x804966D8 | recomp_021.cpp:161910 | 0 | ext+0x32+1, or log2(max(w,h))+1 if 0xFF and MIPMAP |

Table order in the DOL: 1,10,21,20,2,3,7,6,5,4,9,8,15,16,23,24,14,13,17,18,19,11,12,25,27,26,28.

### 2.4 fpSystem requests (0x80494EDC, jump table 0x8056F494)

0 OPEN 0x80495124 · 1 CLOSE 0x804951D8 · 2 START **0x80495214** (GXInit + FIFO, VI/render-mode programming, initial
viewport/scissor/DispCopySrc/Dst, copy filter from render mode, two-pass decision, `GXSetPixelFmt([0x805F268C])`) · 3 STOP
0x804954CC · 4 REGISTER 0x804950A0 (copies 0x8056F50C into RwGlobals+0x10; sample `r3=4 r4=0x8060E350`) · 5 GETNUMMODES
0x80494F34 · 6 GETMODEINFO 0x80494F44 · 7 USEMODE 0x80494F18 · 8 FOCUS 0x80495098 · 9 INITPIPELINE 0x804954F0 · 10 GETMODE
0x80495088 · 11 STANDARDS 0x804954F8 · 12–16, 20, 21 unsupported 0x8049561C · 17 FINALIZESTART 0x804954B4 (→ render-state
reset 0x804981E8) · 18 INITIATESTOP 0x804954C0 · 19 GETMAXTEXTURESIZE 0x804955FC · 22 GETID 0x8049560C. (CODE; START,
REGISTER, STANDARDS, FINALIZESTART CONFIRMED by first-hit samples.) In-stage the device sees 2 system calls/frame (mode
queries).

Two-pass ("half EFB") mode: START sets `[0x805F2698]=1` and `[0x805F26F0]=xfbHeight/2` only when the render mode has AA and
`xfbHeight == viHeight`; otherwise DispCopyYScale = xfbHeight/efbHeight and the flag is 0. Live value 0 → every two-pass
branch in camera begin, Im2D end, clear quad and ShowRaster is dead on this configuration. (CONFIRMED)

### 2.5 Driver globals (live values in-stage)

| Address | Meaning | Live |
|---|---|---|
| 0x805F2688 | clear-on-copy flag: nonzero → RwCameraClear uses GXSetCopyClear and copies pass `clear` | **0** |
| 0x805F268C | default EFB pixel format (GX_PF_*) | 0 = RGB8_Z24 |
| 0x805F2690 | current pixel format cache | 0 |
| 0x805F2698 | two-pass half-EFB mode | 0 |
| 0x805F26A0 / A4 | XFB 0 / XFB 1 | 0x8064E900 / 0x806E4900 |
| 0x805F26A8 | XFB for the next GXCopyDisp | alternates |
| 0x805F26B0/B4/B8 | present queue head / tail / count (ring 0x805E4238, 8-byte entries {xfb, fifo wr ptr}; size [0x805EEFE4]) | |
| 0x805EEFF0 | u16 GXDrawSync token, wraps at 0xE000 | 0x17DD at 50e9 |
| 0x805F26F0 | two-pass split line | 0 |
| 0x805F26F4 | `GXRenderModeObj*` | 0x805E4250 |
| 0x805F26F8 | driver current camera (begin sets, end clears) | NULL between frames |
| 0x805F2700 | RwRaster plugin offset (extension) | **0x34** |
| 0x805F2708 | white 4×4 RGB565 raster (bound when texture == NULL) | 0x807866E0 |
| 0x805F270C | "Im texture": `RwTextureCreate(NULL)` used by render states 1–4, 9 | 0x80786620 |
| 0x805F2710 | raster context (SETRASTERCONTEXT) | NULL |
| 0x805F2718 | RwTexture plugin offset (extension) | **0x58** |
| 0x805F2720 | initial draw-sync token | |
| 0x805F2748 | Im3D stash pointer written by ImmInstance.csl | |
| 0x805E4218 | saved projection (Im2D begin/end) | persp {1.5156, 2.0, near 1, far 20000.13} |
| 0x805E4250 | render mode (§5.7) | |
| 0x805E428C | GX view matrix (RwMatrix, flags 0x00020003) | |
| **0x805E42D0** | render-state cache (§3.2) | |
| 0x8056F4F0 | GXSetProjectionv block for cameras (7 f32) | |
| 0x8056F13C / 0x8056F158 | Im2D ortho projection / Im2D position matrix | |
| 0x8056FBD0 / 0x8056FBEC | clear-quad ortho projection / position matrix | |
| 0x8056FC90 / 0x8056FCC8 | filter → (min,mag) table / address → wrap table | |
| 0x8056FCDC | `RwTexture *bound[8]` (written by bind, never read for caching) | |
| 0x8056FA18 / 0x8056FA28 | RwFogType → GXFogType / RwBlendFunction → GXBlendFactor | |
| 0x8056F120, 0x8056FD00 | RwPrimitiveType → GXPrimitive (Im2D / Im3D copies) | |

### 2.6 Who calls the device (static census, `scan_devcalls.py`)

Calls through `[r13+24924]` + `lwz rX, OFF` + `bctrl` (lower bound — functions that keep the engine pointer in a
non-volatile register across `bl` are not counted; e.g. the Effect::RW3 library at 0x8042–0x8043xxxx):

| Offset | Slot | Sites / functions |
|---|---|---|
| +0x20 | RenderStateSet | 191 / 45 |
| +0x24 | RenderStateGet | 65 / 37 |
| +0x30 | Im2DRenderPrimitive | 18 / 13 (primType 4 ×16, 2 ×2) |
| +0x34 | Im2DRenderIndexedPrimitive | 1 / 1 (0x80479A28, TRILIST) |
| +0x14 | fpSystem | 5 / 5 (RW core only) |
| +0x4C..+0xB8 | stdFunc[1..28] | 1 site each, all inside RW core wrappers (table in CSV) |
| +0x108/+0x10C | rwmalloc / rwfree | 159 / 245 sites |

Game code also calls **driver-private functions directly** (not through RW): `0x80496208` (effects), `0x804987C8` and
`0x80498F74` (RenderLevel payload 0x80046D80 fragments), `0x8049B568` (0x80038BB0, 0x80039550), and reads the private cache
byte `0x805E431D` (fn_80046F6C/fn_80047084 pass it back as ref1). These are part of the contract (§9).

---

## 3. Render states

### 3.1 Dispatch

`RwRenderStateSet(state r3, value r4)` 0x80498954: `if (state > 30) return FALSE;` then `bctr table_0x8056FAD4[state]`
with `r31 = value`, `r6 = result`; every case ends at the common exit 0x80498F58 (`return r6`). Cases are their own
recompiled entries (hookable). `RwRenderStateGet` 0x804984C4 uses table 0x8056FA58 and returns cached values.

### 3.2 Render-state cache `S = 0x805E42D0` (BSS)

| Off | Type | Meaning | Reset (0x804981E8) | In-stage sample |
|---|---|---|---|---|
| +0x00 | u32 | ZWRITEENABLE | 1 | 0 |
| +0x04 | u32 | ZTESTENABLE | 1 | 0 |
| +0x08 | u32 | current GX compare (3 LEQUAL / 7 ALWAYS) | 3 | 7 |
| +0x0C | u32 | CULLMODE (RW value) | 2 (BACK) | 1 |
| +0x10 | u32 | FOGENABLE | 0 | 0 |
| +0x14 | u32 | FOGTYPE (RW) | 1 LINEAR | 1 |
| +0x18 | u32 | FOGCOLOR as passed (A<<24\|R<<16\|G<<8\|B) | 0 | 0 |
| +0x1C | GXColor | fog color bytes R,G,B,A | 0,0,0,0 | |
| +0x20 | u32 | fog end-z override flag (only reset writes it: always 0) | 0 | 0 |
| +0x24 | f32 | fog startz (camera fogPlane +0x88) | 5.0 | |
| +0x28 | f32 | fog endz (camera far +0x84) | 10.0 | 20000.13 |
| +0x2C | f32 | fog nearz (camera near +0x80) | 0.05 | 1.0 |
| +0x30 | f32 | fog farz (camera far) | 10.0 | 20000.13 |
| +0x34 | u32 | SRCBLEND (RW) | 5 | 5 |
| +0x38 | u32 | DESTBLEND (RW) | 6 | 6 |
| +0x3C | u32 | alpha-test mode: 1 = test off + early Z, 0 = cached compare + late Z | 1 | 1 |
| +0x40 | u32 | comp0 (GXCompare) | 4 GREATER | 7 |
| +0x44 | u32 | comp1 | 7 ALWAYS | 7 |
| +0x48 | u32 | op (GXAlphaOp) | 0 AND | 0 |
| +0x4C | u8 | ref0 | 0 | 0 |
| +0x4D | u8 | ref1 | 0 | 0 |
| +0x4E | u8 | "full compare set" flag (0x80498F74) | 0 | 1 |

### 3.3 Per-state specification

`/f` = Final Haunt calls per frame (handler hits). GX calls are emitted only where stated.

| # | State | Handler (generated) | /f | Accepted values → effect |
|---:|---|---|---:|---|
| 0 | NA | 0x80498F58 (recomp_051.cpp:153647) | — | FALSE |
| 1 | TEXTURERASTER | 0x80498C18 (recomp_039.cpp:147833) | 35 | if `value != ImTex->raster`: `TEXTURESETRASTER(ImTex, value)` (dirty). **No GX**; applied at next Im prim (§7.4). NULL allowed (untextured Im). |
| 2 | TEXTUREADDRESS | 0x80498B6C (recomp_035.cpp:170716) | 2 | 4 (BORDER) → FALSE. Else `ImTex.filterAddressing = (fa & ~0xFF00) \| v<<8 \| v<<12`. |
| 3 | TEXTUREADDRESSU | 0x80498BA4 (recomp_036.cpp:144577) | 12 | BORDER → FALSE; bits 8–11 |
| 4 | TEXTUREADDRESSV | 0x80498BD0 (recomp_037.cpp:162586) | 12 | BORDER → FALSE; bits 12–15 |
| 5 | TEXTUREPERSPECTIVE | 0x80498E48 | 0 | returns value; nothing |
| 6 | ZTESTENABLE | 0x80498CA4 (recomp_041.cpp:156541) | 49 | on change: on → `GXSetZMode(TRUE, GX_LEQUAL, S+0x00)`; off → `GXSetZMode(TRUE, GX_ALWAYS, S+0x00)`; S+0x08 = func |
| 7 | SHADEMODE | 0x80498E30 (recomp_045.cpp:180712) | 2 | returns `value == 2 (GOURAUD)`; nothing |
| 8 | ZWRITEENABLE | 0x80498C38 (recomp_040.cpp:169827) | 48 | on change: `GXSetZMode(TRUE, S+0x08, value!=0)` |
| 9 | TEXTUREFILTER | 0x80498BFC (recomp_038.cpp:161172) | 43 | `ImTex.filterAddressing` low byte = value (no validation; 0 behaves as NEAREST) |
| 10 | SRCBLEND | 0x80498D38 (recomp_042.cpp:171553) | 136.19 | valid {1,2,5,6,7,8,9,10}; on change `GXSetBlendMode(GX_BM_BLEND, M[src], M[S+0x38], GX_LO_CLEAR)` |
| 11 | DESTBLEND | 0x80498DC0 (recomp_043.cpp:206599) | 136.19 | valid {1..8}; on change `GXSetBlendMode(GX_BM_BLEND, M[S+0x34], M[dst], GX_LO_CLEAR)` |
| 12 | VERTEXALPHAENABLE | → 0x80498F58 | ~40 (all no-op states) | **no-op, returns FALSE** |
| 13 | BORDERCOLOR | 0x80498E40 | 0 | FALSE |
| 14 | FOGENABLE | 0x80498990 (recomp_031.cpp:179647) | 51 | §3.6 |
| 15 | FOGCOLOR | 0x80498A80 (recomp_032.cpp:189817) | 1 | §3.6 |
| 16 | FOGTYPE | 0x80498AF8 (recomp_033.cpp:166059) | 1 | only 1 (LINEAR) accepted; §3.6 |
| 17 | FOGDENSITY | 0x80498B64 | 0 | FALSE |
| 18–19 | (legacy gap) | 0x80498F58 | — | FALSE |
| 20 | CULLMODE | 0x80498E50 (recomp_048.cpp:172915) | 47 | on change `GXSetCullMode(value − 1)` |
| 21–28 | STENCIL* | 0x80498F58 | — | FALSE (no stencil) |
| 29 | ALPHATESTFUNCTION | 0x80498E78 (recomp_049.cpp:141800) | 0 | §3.5 |
| 30 | ALPHATESTFUNCTIONREF | 0x80498F18 (recomp_050.cpp:162669) | 0 | §3.5 |

Blend factor map `M` (0x8056FA28, index = RwBlendFunction):

| RW | 1 ZERO | 2 ONE | 3 SRCCOLOR | 4 INVSRCCOLOR | 5 SRCALPHA | 6 INVSRCALPHA | 7 DESTALPHA | 8 INVDESTALPHA | 9 DESTCOLOR | 10 INVDESTCOLOR | 11 SRCALPHASAT |
|---|---|---|---|---|---|---|---|---|---|---|---|
| GX value | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 2 | 3 | (rejected) |
| as src | ZERO | ONE | *rejected* | *rejected* | SRCALPHA | INVSRCALPHA | DSTALPHA | INVDSTALPHA | DSTCLR | INVDSTCLR | — |
| as dst | ZERO | ONE | SRCCLR | INVSRCCLR | SRCALPHA | INVSRCALPHA | DSTALPHA | INVDSTALPHA | *rejected* | *rejected* | — |

GX value 2/3 means DSTCLR/INVDSTCLR in the source slot and SRCCLR/INVSRCCLR in the destination slot (GX aliases). Blend
op is always add (GX_BM_BLEND, logic op CLEAR ignored); there is no subtract, no logic op and no "blend off" anywhere in the
driver. Measured GX blend pairs over the whole route (src/dst GX values): 4/5 (dominant), 4/1 additive, 1/1, 1/0 (clears and
opaque), 0/3. (CONFIRMED)

Cull map: RW 1 NONE → `GX_CULL_NONE`(0); RW 2 BACK → `GX_CULL_FRONT`(1); RW 3 FRONT → `GX_CULL_BACK`(2). Samples:
`CULLMODE r4=1 → GXSetCullMode(0)`, `r4=2 → GXSetCullMode(1)`. GX front faces are clockwise in screen space while RW
front faces are counter-clockwise, so "cull RW back" = "cull GX front". (CONFIRMED)

### 3.4 Z semantics

Z compare is **always enabled** on GX. ZTEST off means compare ALWAYS; z-write remains independently controlled by
ZWRITEENABLE. ZCompLoc is driven by the alpha-test mode (§3.5), never by a render state. Measured: 49 ZTEST + 48 ZWRITE
handler hits → 28 `GXSetZMode`/frame (changes only; 16 more come from the clear quads). (CONFIRMED)

### 3.5 Alpha test: RW states vs the GC extension API

Three driver functions own `GXSetAlphaCompare` + `GXSetZCompLoc`:

* **0x804987C8 `SetAlphaTestMode(mode)`** (recomp_027.cpp:165077; 194.19/f; 17 callers incl. the RenderLevel payload and the
  pipelines 0x80449D54/0x8044EEC4/0x80463964 and matfx 0x804A3F74…0x804A55CC): if `mode != S+0x3C`:
  mode 1 → `GXSetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0)`, `GXSetZCompLoc(GX_TRUE)`;
  mode 0 → `GXSetAlphaCompare(S+0x40, S+0x4C, S+0x48, S+0x44, S+0x4D)`, `GXSetZCompLoc(GX_FALSE)`; store mode.
* **0x80498854 `BindImTextureAndAlphaMode()`** (recomp_028.cpp:189067; 87/f): if `ImTex->raster`: bind it to TEXMAP0 via
  0x8049AC88, then `mode = !(parentRasterExt+0x14 & 1)` (texture without alpha → mode 1) and apply as above.
* **0x80498F74 `SetAlphaCompare(comp0, ref0, op, comp1, ref1)`** (recomp_052.cpp:173504; 19/f; callers are the six
  fragments of 0x80046D80): `if (S+0x3C != 1) GXSetAlphaCompare(OLD S+0x40, OLD S+0x4C, OLD S+0x48, OLD S+0x44, OLD S+0x4D);`
  then store the new five values and set S+0x4E = 1.
  **The previous compare is re-emitted, the new one is only cached.** First-hit samples show exactly that:
  `GXSetAlphaCompare lr=0x80498FC8 r3=4 r4=1` followed by `r3=4 r4=0xC8` — the value emitted by each call is the one stored by
  the call before. Since 0x80046D80 always follows it with `SetAlphaTestMode(m)`, the new compare reaches GX only when
  the mode actually changes (1→0); consecutive mode-0 levels keep the older reference.
* RW states 29/30 (`ALPHATESTFUNCTION` = value−1 as GXCompare; `...REF` = ref0 byte) write the cache and emit only if
  mode ≠ 1 (and only on change). Special case: once 0x80498F74 has run (S+0x4E ≠ 0), ALPHATESTFUNCTION unconditionally
  resets op/comp1/ref1 to AND/ALWAYS/0 in the cache and emits `(value−1, ref0, AND, ALWAYS, 0)` if mode ≠ 1 (S+0x4E stays
  set). Not called in-stage (0 hits).

RenderLevel payload 0x80046D80 (render-order lane) uses the extension API as follows (CODE, values observed):
flag 0x40 → SRCALPHA/INVSRCALPHA, ZTEST 1, ZWRITE 0, compare ALWAYS, mode 1; flag 0x80 → SRCALPHA/ONE, ZTEST 1, ZWRITE 0,
mode 1; flag 0x100 → INVDESTCOLOR/ZERO, mode 1; flag 0x400 → SRCALPHA/INVSRCALPHA, ZTEST 0, ZWRITE 0, mode 1; flag 0x800 →
SRCALPHA/INVSRCALPHA, ZWRITE 1, ZTEST 1, compare (GREATER, 200, AND, ALWAYS, S+0x4D), mode 0; default → SRCALPHA/INVSRCALPHA,
ZTEST 1, ZWRITE 1, compare (GREATER, 1, AND, ALWAYS, S+0x4D), mode 0.

### 3.6 Fog

GXSetFog(type, startz, endz, nearz, farz, GXColor) — 38 GX calls/frame from 51 FOGENABLE + 1 FOGCOLOR + 1 FOGTYPE.

* **FOGENABLE on** (only if S+0x10 == 0): if `curCamera`: `S+0x28 = cam.far` (override flag S+0x20 is always 0),
  `S+0x24 = cam.fogPlane (+0x88)`, `S+0x2C = cam.near (+0x80)`, `S+0x30 = cam.far (+0x84)`; then
  `GXSetFog(T[S+0x14], S+0x24, S+0x28, S+0x2C, S+0x30, S+0x1C)`, S+0x10 = 1.
* **FOGENABLE off** (only if on): `GXSetFog(GX_FOG_NONE, 5.0, 10.0, 0.05, 10.0, S+0x1C)`, S+0x10 = 0.
* **FOGCOLOR** (on change): color bytes R=(v>>16), G=(v>>8), B=v, A=(v>>24) into S+0x1C; then
  `GXSetFog(T[S+0x14], cam.fogPlane, cam.far, cam.near, cam.far, color)` **without checking FOGENABLE and without a NULL check
  on curCamera** — setting a fog colour re-arms linear fog on GX even while RW fog is off, until the next FOGENABLE(0) change.
* **FOGTYPE**: only 1 accepted; same emission as FOGCOLOR.
* Type map T (0x8056FA18): 0 NONE→0, 1 LINEAR→`GX_FOG_LIN`(2), 2 EXP→4, 3 EXP2→5. Only LINEAR is ever used.
Camera fields are read at the moment of the state call, not at draw time. (CONFIRMED: `GXSetFog lr=0x80498A2C r3=2`,
`lr=0x80498A70 r3=0`; FOGCOLOR/FOGTYPE callers 0x8016AC38/0x8016AC50 = fog bank loader.)

### 3.7 Texture states through the "Im texture"

States 1–4 and 9 edit `ImTex = [0x805F270C]` (an `RwTexture` created with no raster at reset). Nothing reaches GX until an
Im2D primitive (0x8049148C), an Im3D SubmitNoLight node (0x804A0B68) or a raster render (0x80499394) calls 0x80498854,
which binds ImTex to GX_TEXMAP0 through the normal bind (§4.6). In-stage end state: filterAddressing 0x3302 (LINEAR, CLAMP,
CLAMP). Observed values: TEXTUREFILTER {0,1,2}, TEXTUREADDRESS 3, U/V 3 (CSD 0x803645E0/0x80364600, PostGlare 0x80430B08).

### 3.8 RenderStateGet (0x804984C4)

Returns TRUE and the cached value for 1 (ImTex raster), 2 (U if U==V else FALSE), 3, 4, 6, 8, 9, 10, 11, 14, 15, 16, 20;
5 → 1; 7 → 2 (GOURAUD); 29 → `comp0+1` if op==AND and comp1==ALWAYS else 0; 30 → ref0. FALSE for 0, 12, 13, 17–19, 21–28.
DrawBlur and friends save/restore states through it (181/f). Note VERTEXALPHAENABLE Get returns FALSE, so save/restore of
that state is a no-op round trip.

### 3.9 Defaults (0x804981E8, FINALIZESTART)

Cache as in §3.2; GX: `GXSetZMode(1,LEQUAL,1)`, `GXSetZCompLoc(1)`, `GXSetAlphaCompare(ALWAYS,0,AND,ALWAYS,0)`,
`GXSetBlendMode(BLEND,SRCALPHA,INVSRCALPHA,CLEAR)` ×2, `GXSetCullMode(GX_CULL_FRONT)`, `GXSetChanCtrl` all four channels
lighting off (REG/REG), `GXSetChanMatColor` all 0xFFFFFFFF, `GXSetColorUpdate(1)`, `GXSetAlphaUpdate(1)`,
`GXSetCopyClear({255,255,255,255}, 0xFFFFFF)`, `GXSetCurrentMtx(0)`, bound-texture table cleared (0x8049AC18), ImTex =
`RwTextureCreate(NULL)` with filterAddressing 0x1102 (LINEAR, WRAP, WRAP), white raster = `RwRasterCreate(4,4,16,
TEXTURE|565)` locked and memset 0xFF.

### 3.10 Re-implementation notes (render states)

* Keep the RW-level cache (0x805E42D0 layout) observable: game code reads S+0x4D directly and DrawBlur restores through Get.
* Blend: always enable blending; ONE/ZERO is opaque. Destination-alpha factors read 1.0 because the EFB is RGB8_Z24
  (INFERRED from GX pixel-format semantics; only SRCALPHA/INVSRCALPHA/ONE/ZERO/INVSRCCLR occur).
* Depth: `DepthEnable=TRUE` always; ZTEST off → `DepthFunc=ALWAYS` (a D3D12 DepthEnable=FALSE would also kill writes and
  diverge). LEQUAL otherwise.
* Cull: map RW BACK to "cull clockwise-in-screen" after reproducing GX's projection/viewport flip (§5.3); do not "fix" it.
* Alpha test: emulate `GXSetAlphaCompare(comp0,ref0,op,comp1,ref1)` in the pixel shader exactly (two compares + AND/OR/XOR/
  XNOR), and apply the **emitted** compare, including the one-call lag of 0x80498F74. Early/late Z (ZCompLoc) matters only
  when the alpha test discards: late Z = depth written only for surviving fragments; with a discard-based shader this is the
  D3D default, so ZCompLoc(TRUE) with a non-discarding compare needs no special handling.
* Fog: GX linear fog on range [startz, endz] in eye-space depth computed from `nearz/farz` (GX formula), with the quirk that
  FOGCOLOR/FOGTYPE re-arm fog. Only LINEAR occurs.
* Droppable: stencil states, TEXTUREPERSPECTIVE, SHADEMODE, BORDERCOLOR, FOGDENSITY, VERTEXALPHAENABLE (they have no effect on
  GC and the game's output depends on them having none).

---

## 4. Rasters and textures

### 4.1 `RwRaster` (base 0x34) + RwGC extension (0x34 at +0x34, `[0x805F2700]`)

Base per RW 3.7 (`parent, cpPixels, palette, width, height, depth, stride, nOffsetX(i16), nOffsetY(i16), cType, cFlags,
privateFlags, cFormat (=format>>8), originalPixels, originalWidth, originalHeight, originalStride`). Extension, registered by
0x80496440 (`RwRasterRegisterPlugin(0x34, 0x40C)`):

| Ext off | Raster off | Type | Meaning | Written by |
|---|---|---|---|---|
| +0x00 | +0x34 | GXTlutObj (12) | TLUT object for palettised rasters (`GXInitTlutObj(ext, palette, tlutFmt, 1<<depth)`) | create |
| +0x0C | +0x40 | u32 | **GXTexFmt** (255 = unset) | 0x80497458 / TXD read |
| +0x10 | +0x44 | u32 | **GXTlutFmt** (255 = none) | same |
| +0x14 | +0x48 | u32 | bit0 = has alpha (drives alpha-test mode and camera pixel format) | same |
| +0x18 | +0x4C | ptr | allocation block (rwfree target) | create |
| +0x1C | +0x50 | ptr | **pixel data** (block rounded up to 32) — the GX image pointer | create |
| +0x20 | +0x54 | ptr | palette (after the image) | create |
| +0x24 / +0x28 | +0x58 / +0x5C | | lock bookkeeping | lock |
| +0x2C | +0x60 | ptr | GXTexRegion* for preloaded textures (never set here) | — |
| +0x30 | +0x64 | u16 | last GXDrawSync token that used the raster (bind stamps it; destroy waits for it) | bind |
| +0x32 | +0x66 | u8 | mip levels − 1 (255 = unknown) | create |
| +0x33 | +0x67 | u8 | 0xFF | create |

Live examples (Run B/C):

| Raster | Parent | w×h | depth | cType | cFlags | cFormat | ext fmt | pixels |
|---|---|---|---:|---:|---|---|---|---|
| main camera fb 0x80786A60 | self | 640×480 | 32 | 2 CAMERA | 0x80 | 0x06 (888) | RGBA8 (6), alpha 0 | NULL (it is the EFB) |
| glare/scene save 0x809ED600 | self | 640×480 | 32 | 5 CAMERATEXTURE | 0 | 0x05 (8888) | RGBA8, alpha 1 | 0x80C1EE60 |
| its sub 0x809ED560 | 0x809ED600 | 640×480 off (0,0) | 32 | 5 | 0x80 | 0x05 | (from parent) | — |
| glare 0x809ED740 | self | 640×480 | 32 | 5 | 0 | 0x05 | RGBA8 | 0x80D4AEA0 |
| ladder 0x809ED7E0 | self | 128×128 | 32 | 5 | 0 | 0x05 | RGBA8 | 0x809F8380 |
| ladder 0x809ED880 | self | 64×64 | 32 | 5 | 0 | 0x05 | RGBA8 | 0x80A083C0 |
| white 0x807866E0 | self | 4×4 | 16 | 4 TEXTURE | 0 | 0x02 (565) | RGB565, alpha 0 | 0x80786780 |

### 4.2 `RwTexture` (base 0x58) + extension at +0x58 (`[0x805F2718]`)

Base: `raster +0x00, dict +0x04, lInDictionary +0x08, name[32] +0x10, mask[32] +0x30, filterAddressing +0x50
(bits 0–7 filter, 8–11 addrU, 12–15 addrV), refCount +0x54`. Extension:

| Ext off | Tex off | Meaning |
|---|---|---|
| +0x00..+0x1F | +0x58 | `GXTexObj` (SDK internal: mode0, mode1, image0, image3, userData, fmt, tlutName, loadCnt, loadFmt, flags) |
| +0x20 | +0x78 | u32: bit 0x01000000 = dirty (set by TEXTURESETRASTER; forces GXInitTexObj on next bind); bit 0x02000000 = "TLUT/texture preloaded" (never set in this game: GXLoadTexObjPreLoaded count == GXLoadTexObj count, the SDK's internal call); low 16 bits = filterAddressing low 16 bits last applied |

### 4.3 Raster types and format selection (0x80497458, recomp_050.cpp:159669)

`cType = flags & 7` (0 NORMAL, 1 ZBUFFER, 2 CAMERA, 3 invalid, 4 TEXTURE, 5 CAMERATEXTURE); `cFlags = flags & 0xF8`
(0x80 DONTALLOCATE, 0x40 PALETTEVOLATILE). For NORMAL/TEXTURE/CAMERATEXTURE (CAMERATEXTURE may not be palettised):

| RW pixel format (flags & 0x0F00) | + PAL4 (0x4000) | + PAL8 (0x2000) | no palette | ext+0x14 alpha |
|---|---|---|---|---|
| 0x0100 C1555 | C4 + TLUT RGB5A3, depth 4 | C8 + RGB5A3, depth 8 | RGB5A3, depth 16 | 1 |
| 0x0200 C565 | C4 + RGB565 | C8 + RGB565 | RGB565, 16 | 0 |
| 0x0300 C4444 | C4 + RGB5A3 | C8 + RGB5A3 | RGB5A3, 16 | 1 |
| 0x0500 C8888 | error | error | RGBA8, 32 | 1 |
| 0x0600 C888 | error | error | RGBA8, 32 | 0 |
| 0x0000 default | by depth (jump table 0x8056F96C) | | | |
| 0x0400 LUM8, 0x0700/0x0800/0x0900 | error | | | |

For ZBUFFER/CAMERA: default format → RGBA8/888 (depth 32) when `renderMode.aa == 0` (this game), RGB565/565 (depth 16)
with AA. `cFormat` is rewritten with the chosen format.

Formats that never come from 0x80497458 but do come from TXD files: **CMPR (14)** with raster flags 0x0004 (format 0) — the
native reader sets ext+0x0C/0x10/0x14 directly from the file (§4.7).

### 4.4 Create / destroy / lock / unlock / sub-raster

* **RASTERCREATE 0x80497CA8** (r4 raster, r5 flags): init ext (0x0C/0x10 = 255, rest 0, token = [0x805F2720], mips = 255),
  select format; `width==0 || height==0` → DONTALLOCATE; CAMERA/ZBUFFER → DONTALLOCATE, **no memory**; DONTALLOCATE flag
  → return. Otherwise levels = MIPMAP ? log2(max(w,h))+1 : 1, size = Σ per-level size, palette adds `(1<<depth)*2`,
  `rwmalloc(size+31(+palette), 0x30411)`, pixels = (block+31)&~31, palette after pixels + `GXInitTlutObj`; CAMERATEXTURE
  pixel memory is zero-filled (0x80372508). Sample flags: 0x204 (white raster), 0x2284 (TXD PAL8|565|DONTALLOCATE|TEXTURE),
  0x2 (CAMERA), 0x505 (camera textures). 358 creations before the stage window, 0 in-stage. (CONFIRMED)
* Per-level size (0x80496478 → table 0x8056F5B8 by depth, each rounded up to 32 bytes): depth 4 → `A8(w)·A8(h)/2`;
  depth 8 → `A8(w)·A4(h)`; depth 16 → `A4(w)·A4(h)·2`; depth 32 → `A4(w)·A4(h)·4` (A8/A4 = round up to 8/4); w,h of level
  L = max(1, dim>>L). (CODE)
* **RASTERDESTROY 0x80498058**: only top-level non-DONTALLOCATE NORMAL/TEXTURE/CAMERATEXTURE; if ext token == current
  token, `GXSetDrawSync(token)` and advance; wait until the GPU has passed the token (0x8049B7DC); unbind from ImTex; `rwfree`.
* **RASTERLOCK 0x80496CA4** (r4 raster, r5 mode, level = mode>>8): NORMAL/TEXTURE/CAMERATEXTURE only (**CAMERA/ZBUFFER → error:
  the EFB cannot be read back through RW**); returns ext+0x1C + Σ sizes of lower levels and sets width/height/stride to the
  level. **UNLOCK 0x80497104**: format fix-up 0x8049677C, `DCFlushRange`, `GXInvalidateTexAll`, restore dimensions, re-dirty
  textures (0x8048E310). 3 lock/unlock pairs total (white raster, RasterSetImage), 0 in-stage.
* **RwRasterSubRaster 0x8048AE14 + stdFunc[12] 0x804981B8**: sub must be DONTALLOCATE; `sub.w,h = rect.w,h`;
  `sub.nOffset = parent.nOffset + rect.xy`; copy stride/depth/cType/cFormat; cpPixels = 0; sub.parent = parent's parent.
  2/frame (glare split-screen offset rasters rebuilt each frame). GX never sees sub-rasters except as viewport/scissor
  offsets (camera) and texture-copy source/destination offsets (§6.2).

### 4.5 Mip levels

Every texture on the disc has 1 level (§4.7); camera-texture copies use `GXSetTexCopyDst(..., mipmap=halfRes)` only for
the half-resolution box-filtered copy (blur history, inactive in the measured window). `ext+0x32` is therefore 0 for all
live textures and every `GXInitTexObjLOD` gets `max_lod = 0`. (CONFIRMED)

### 4.6 Texture bind chokepoint 0x8049AC88 `RwGameCubeTextureBind(RwTexture *tex, GXTexMapID map)` (recomp_013.cpp:183295)

273.29 calls/frame (87 from Im2D via 0x80498854; the rest from the atomic/world/skin pipelines 0x80449D54, 0x8044EEC4,
0x80463964, matfx 0x804A3F74…0x804A55CC, EMBM 0x804A7AAC).

```
if (tex == NULL) { tex = ImTex; TEXTURESETRASTER(tex, whiteRaster4x4); }          // dirty
obj  = tex + 0x58 (GXTexObj);  flags = tex+0x78
r    = tex->raster;  P = r->parent;  PE = P + 0x34
PE.token = drawSyncToken                                                           // u16 at P+0x64
pal  = (r->cFormat & 0x60) != 0                                                    // PAL4|PAL8
fa   = tex->filterAddressing
wrapS = WRAP[(fa>>8)&0xF]; wrapT = WRAP[(fa>>12)&0xF]      // 0x8056FCC8: 0→CLAMP 1→REPEAT 2→MIRROR 3→CLAMP 4→CLAMP
f = fa & 0xFF; if (pal && (f==5||f==6)) f = 4             // CI textures cannot use GX_LIN_MIP_LIN / NEAR_MIP_LIN
(min,mag) = FILT[f]                                        // 0x8056FC90: 0:(NEAR,NEAR) 1:(NEAR,NEAR) 2:(LINEAR,LINEAR)
                                                           // 3:(NEAR_MIP_NEAR,NEAR) 4:(LIN_MIP_NEAR,LINEAR)
                                                           // 5:(NEAR_MIP_LIN,NEAR) 6:(LIN_MIP_LIN,LINEAR)
mip  = (r->cFormat & 0x80) != 0
if (flags & DIRTY):
    pal ? GXInitTexObjCI(obj, PE.pixels, r->w, r->h, PE.gxFmt, wrapS, wrapT, mip, tlutName=map)
        : GXInitTexObj  (obj, PE.pixels, r->w, r->h, PE.gxFmt, wrapS, wrapT, mip)
    GXInitTexObjLOD(obj, min, mag, 0.0, (f32)PE.mipsMinus1, 0.0, TRUE, TRUE, GX_ANISO_1)
elif ((fa & 0xFFFF) != (flags & 0xFFFF)) or (pal && GXGetTexObjTlut(obj) != map):
    same Init, but LOD bias / bias clamp / edge LOD / max aniso are read back from obj first and preserved
flags = (flags & 0xFFFF0000 & ~DIRTY) | (fa & 0xFFFF)
if (pal) GXLoadTlut(PE /*GXTlutObj*/, map)                 // TLUT index == texmap index
GXLoadTexObj(obj, map)                                     // always, no bind cache
bound[map] = tex                                            // 0x8056FCDC, write-only
```

Measured: GXInitTexObj 23.1/f + GXInitTexObjCI 13/f (re-inits), GXLoadTlut 187.19/f, GXLoadTexObj 273.29/f. Note the
image pointer is the **parent's** pixels with the **sub-raster's** width/height (sub-raster textures are not offset).
`0x8049B568 SetTextureLOD(tex, biasClamp, edgeLOD, maxAniso, f1=lodBias)` re-inits with explicit LOD parameters (TXD read and
game 0x80038BB0/0x80039550). (CONFIRMED)

### 4.7 Native texture format (GameCube platform 6) — reader 0x8049A8DC

Chunk headers are standard little-endian RW stream headers; the native payload is **raw big-endian** memory:

```
TEXTURENATIVE (0x15) > STRUCT (0x01), version must be 0x35000..0x37002 (disc: 0x1C020037)
+0x00  u32 platformId            = 6
+0x04  u32 filterAddressing      (low 16 bits used: filter | U<<8 | V<<12)
+0x08  u32 maxAniso              (GXAnisotropy)
+0x0C  u32 biasClamp             (GXBool)
+0x10  u32 edgeLOD               (GXBool)
+0x14  f32 lodBias
+0x18  char name[32]
+0x38  char mask[32]
+0x58  u32 rasterFlags           (cType | format; reader ORs 0x80 DONTALLOCATE)
+0x5C  u16 width, u16 height
+0x60  u8 depth, u8 numLevels, u8 gxTexFmt (→ext+0x0C), u8 gxTlutFmt (→ext+0x10)
+0x64  u32 hasAlpha              (→ext+0x14 = value != 0)
+0x68  [palette: (1<<depth)*2 bytes of big-endian u16 in gxTlutFmt]   if rasterFlags & 0x6000
       u32 dataSize
       dataSize bytes: GX texture image, all levels concatenated, GX block layout, big-endian
```
Reader: `RwRasterCreate(w,h,depth,flags|0x80)`, set ext fmt/tlut/alpha, allocate with numLevels (0x80497A70), clear
DONTALLOCATE, read palette + DCFlushRange, temporarily clear AUTOMIPMAP (cFormat bit 0x10), read image, DCFlushRange,
`GXInvalidateTexAll`, `RwTextureCreate`, set filterAddressing low 16 bits, name, mask, `SetTextureLOD(biasClamp, edgeLOD,
maxAniso, lodBias)`. (CONFIRMED by disc census: 10,161 of 10,164 image sizes match the GX block-size formula exactly.)

Disc census (`txd_census.csv`, `one_txd_census.csv`):

| Source | Textures | GX formats | TLUT | Levels | filterAddressing | aniso/biasClamp/edgeLOD/lodBias |
|---|---:|---|---|---|---|---|
| 701 loose .txd | 1,735 | C4 1138, C8 584, RGB5A3 12, RGB565 1 | RGB5A3 966, RGB565 756 | all 1 | all 0x1101 | 0 / 1 / 1 / 0.0 |
| 379 TXDs in .one (PRS) | 8,429 | C4 5960, CMPR 1106, C8 933, RGB565 291, RGB5A3 139 | RGB565 6390, RGB5A3 503 | all 1 | all 0x1101 | 0 / 1 / 1 / 0.0 |
| stg0604 archives only | 135 | C4 107, CMPR 25, RGB5A3 2, C8 1 | | 1 | 0x1101 | |

Runtime GX census (host counters, whole route to 50e9: sampled draws / distinct maps): CMPR 3,729,994/20, C4 2,087,799/83,
C8 307,683/6, RGBA8 138,092/4 (camera textures), RGB565 12,547/1 (white). Filters used at draw time come from game-side
`RwTextureSetFilterMode` calls/material setup (the files all say NEAREST). (CONFIRMED)

### 4.8 Re-implementation notes (rasters/textures)

* Decode C4/C8 (with RGB565/RGB5A3 TLUTs), CMPR, RGB565, RGB5A3 at load (or on first bind) into host textures; key the host
  texture by (parent raster, dirty generation). No other formats are needed for this game's data.
* Honour `rwRASTERTYPECAMERA` = "the backbuffer", `rwRASTERTYPECAMERATEXTURE` = CPU-visible texture memory that is filled
  only by explicit EFB copies (§6); RASTERLOCK on CAMERA must keep failing.
* Filter/address semantics must follow FILT/WRAP exactly, including 0 → NEAREST, BORDER → CLAMP and the CI trilinear
  demotion (irrelevant here: no mips).
* Droppable: TLUT slot == texmap coupling, GXTexObj storage in the texture extension, draw-sync token waits on destroy,
  DCFlushRange/GXInvalidateTex*, the 32-byte rounding, preloaded-region flag. Must stay byte-for-byte: the TXD stream layout
  (game data), texture/raster struct offsets the game touches (filterAddressing +0x50, raster width/height/offsets), the
  ImTex/white-raster behaviour for NULL textures.

---

## 5. Cameras

### 5.1 `RwCamera` as used (main camera 0x80786880, holder `[0x8056FF1C]`)

RW 3.7 base layout (0x184) + plugins. Live in-stage values:

| Off | Field | Value |
|---|---|---|
| +0x00 | RwObjectHasFrame (type 4) | frame 0x807875E0 |
| +0x14 | projectionType | 1 rwPERSPECTIVE |
| +0x18 / +0x1C | beginUpdate / endUpdate | 0x80461B6C (RpWorld) / 0x80474EEC |
| +0x20 | viewMatrix (RW internal) | |
| +0x60 | frameBuffer | 0x80786A60 (640×480 CAMERA) |
| +0x64 | zBuffer | NULL (GX has an implicit Z buffer) |
| +0x68 / +0x70 | viewWindow / recipViewWindow | (0.6158, 0.5421) / (1.6239, 1.8446) at save time (varies with FOV) |
| +0x78 | viewOffset | (0, 0) |
| +0x80 / +0x84 / +0x88 | near / far / fogPlane | 1.0 / 20000.13 / ~0 |
| +0x184 | RpWorld plugin: world, …, saved begin 0x804863A4 (+0x194), saved end 0x80486340 (+0x198) | |

Only this camera is begun in-stage (`--contexts 0x8048652C` saw 4 distinct pointers over the whole run; the other three are
boot/menu objects since freed). (CONFIRMED)

### 5.2 Begin / end chain (32 pairs per frame)

`RwCameraBeginUpdate 0x8048652C` → `cam->beginUpdate` 0x80461B6C (`curWorld = plugin world; renderFrame++`) → core default
0x804863A4 (`curCamera = cam`; frame/frustum sync 0x8048C3A4; `stdFunc[1](NULL, cam, 0)`; then 0x8049C198) →
**0x804956F4**. End: `0x80486504` → 0x80474EEC → 0x80486340 (`stdFunc[10]` 0x80495CAC: driver camera = NULL) → 0x80486370
(`curCamera = NULL`). End update performs **no GX work, no copy, no flush**. (CONFIRMED)

### 5.3 CAMERABEGINUPDATE 0x804956F4 — exact GX programming

1. `GXSetCurrentGXThread()`; `[0x805F26F8] = cam`.
2. View matrix V (RwMatrix at 0x805E428C, flags 0x00020003) = `RwMatrixInvert(RwFrameGetLTM(cam->frame))` then
   `RwMatrixTransform(V, S, rwCOMBINEPOSTCONCAT)` with the view-offset shear
   `S = {right(1,0,0), up(0,1,0), at(−ox, oy, 1), pos(ox, −oy, 0)}` (ox,oy = viewOffset). Nothing is loaded to GX here.
3. Projection block P (0x8056F4F0, `GXSetProjectionv` format `{type, m00, m02, m11, m12, m22, m23}`):
   `m00 = recipViewWindow.x (+0x70)`, `m11 = recipViewWindow.y (+0x74)`, `m02 = m12 = 0` (static),
   * perspective: `type = 0`, `m22 = −near/(far−near)`, `m23 = far·m22` ( = −far·near/(far−near));
   * rwPARALLEL: `type = 1`, `m22 = −1/(far−near)`, `m23 = far·m22`.
   In-stage sample: `{0, 1.51562, 0, 2.0, 0, −5.0002e−5, −1.00005}` → near 1.0, far 20000.13.
4. Camera raster `R = cam->frameBuffer`:
   * `R.cType & 5` (ZBUFFER/TEXTURE/CAMERATEXTURE): `GXSetPixelFmt(parentExt.alpha ? GX_PF_RGBA6_Z24 : GX_PF_RGB8_Z24,
     GX_ZC_LINEAR)` if different from [0x805F2690]; `GXSetViewport(R.nOffsetX, R.nOffsetY, R.width, R.height, 0, 1)`;
     `GXSetScissor(same)`; `GXSetScissorBoxOffset(0,0)`. **Not reached in-stage** (GXSetPixelFmt: 0 calls in the window).
   * otherwise (CAMERA, the EFB): pixel format back to `[0x805F268C]` if needed; single-pass mode:
     `GXSetViewport[Jitter if field rendering](R.nOffsetX, R.nOffsetY, R.width, R.height, 0.0, 1.0)` and
     `GXSetScissor(R.nOffsetX, R.nOffsetY, R.width, R.height)`. (Two-pass branches double y/height and clamp the scissor at
     [0x805F26F0] ± 2 with a scissor box offset — dead here.)
5. The object transform is loaded later, per draw, by **0x804960FC `SetTransform(RwMatrix *world, RwBool normals)`**
   (recomp_063.cpp:155339; 91.1/f): `M = world ? world×V : V` (RW row-vector product), then GX 3×4 rows
   `row0 = (−M.right.x, −M.up.x, −M.at.x, −M.pos.x)`, `row1 = (M.right.y, M.up.y, M.at.y, M.pos.y)`,
   `row2 = (−M.right.z, −M.up.z, −M.at.z, −M.pos.z)`; `GXLoadPosMtxImm(row, GX_PNMTX0)`; if `normals`
   `GXLoadNrmMtxImm(row, GX_PNMTX0)` (same 3×3, no inverse-transpose); `GXSetCurrentMtx(0)`.
   The X/Z negation is the RW(+Z forward, right-handed with +X screen-left) → GX(−Z forward) change of basis.

Equivalent D3D-style clip transform (INFERRED from GX XF semantics; matches the host translation): with GX eye position
`e = row·[x y z 1]` and GX clip `c = (m00·e.x + m02·e.z, m11·e.y + m12·e.z, m22·e.z + m23, −e.z)` (perspective) or
`(m00·e.x + m02, m11·e.y + m12, m22·e.z + m23, 1)` (ortho — in the `GXSetProjectionv` block the 3rd and 5th values are
column-3 translations for ortho and column-2 terms for perspective), GX window depth is `z01 = 1 + c.z/c.w` and window
`x = (c.x/c.w)·w/2 + x0 + w/2`, `y = −(c.y/c.w)·h/2 + y0 + h/2`. A D3D projection producing identical depth is
`m22' = m22 − 1, m23' = m23` (perspective) or `m22' = m22, m23' = m23 + 1` (ortho).

### 5.4 Render mode and the EFB

In-stage `GXRenderModeObj` 0x805E4250: viTVmode 0x14 (EURGB60 interlaced), fbWidth 640, efbHeight 480, xfbHeight 480,
viXOrigin 40, viYOrigin 0, viWidth 640, viHeight 480, xFBmode 1 (DF), field_rendering 0, aa 0, sample pattern all 6,
vfilter {8,8,10,12,10,8,8}. Boot (PAL 50 Hz, before the 50/60 screen) used 528-line scissors (0x210). The camera frame
buffer raster is 640×480 at offset (0,0): **viewport = scissor = the whole 640×480 EFB**. EFB pixel format RGB8_Z24.
(CONFIRMED)

### 5.5 CAMERACLEAR 0x80495638 → ClearQuad 0x80499E58 (8/frame)

`RwCameraClear(cam, RwRGBA *color, mode)` → stdFunc[21]. `[0x805F2688] == 0`, so:
rect = camera raster `(nOffsetX, nOffsetY, width, height)` → `0x80499E58(R, rect, color, mode)`:
* mode & IMAGE: color = *color; `GXSetZMode(TRUE, GX_ALWAYS, mode & Z)`.
* Z only: color = (0,0,0,255); `GXSetZMode(TRUE, GX_ALWAYS, TRUE)`; `GXSetColorUpdate(FALSE)`.
* neither: return.
* `GXSetChanMatColor(GX_COLOR0A0, color)`, `GXSetBlendMode(BLEND, ONE, ZERO, CLEAR)`, `GXSetCullMode(NONE)`,
  `SetAlphaTestMode(1)`, fog off if on (0x804986B8), 2D setup 0x80499B14 (VtxDesc POS XYZ F32 only, 0 texgens, 1 TEV stage
  PASSCLR, COLOR0A0 material from register; screen raster: viewport = render-mode fb, scissor 0,0,fbW,xfbH, projection
  ortho `2/fbW, −2/xfbH` (0x8056FBD0), PosMtx 0x8056FBEC = translate(0.5,0.5)·scaleZ(−1); CAMERATEXTURE raster: viewport
  640×528, scissor 640×528, projection `2/639, −2/527`), `GXBegin(GX_QUADS, VTXFMT0, 4)` with (x,y), (x,y+h), (x+w,y+h),
  (x+w,y) at z = 0.99999994;
* restore: fog on if it was, `GXSetBlendMode` from cache (0x80498910), `GXSetZMode(TRUE, S+0x08, S+0x00)`,
  `GXSetCullMode(S+0x0C − 1)`, `GXSetColorUpdate(TRUE)`. Alpha-test mode stays 1; projection and viewport stay as set.
Callers: 0x801D032C (frame start, color table 0x8057E760, mode 3 = IMAGE|Z), glare ladder 0x8042F258 ×2 and 0x804302EC ×2
(mode 1), 0x802BE9D4 and 0x80306D80 (mode 1). Final Haunt: 1 main + 7 ladder clears per frame. (CONFIRMED)

### 5.6 Re-implementation notes (cameras)

* Intercept at **stdFunc[1]/[10]/[21]** and **0x804960FC**; reproduce view = inverse(LTM)×shear with the X/Z flip, the GX
  projection from recipViewWindow/near/far, and viewport = scissor = frame-buffer raster rect.
* Clear = draw: a native clear must write colour and depth = 0.99999994 (0xFFFFFE in 24-bit), respect the camera raster rect,
  and must **not** disturb the "last emitted" alpha-test/viewport/projection state that later draws inherit (the quad leaves
  alpha mode 1 and its ortho projection/viewport in place until the next camera begin or Im2D).
* Droppable: GXSetCurrentGXThread, two-pass half-EFB and field-rendering branches (dead in this configuration), pixel
  format toggling (never reached in-stage), the driver's current-camera global.

---

## 6. RwRasterRender family, EFB copies and present

### 6.1 RasterRender / Scaled / Fast (stdFunc 17/18/19)

All three call 0x80499924(src, rect-or-pos, scaled, masked): only a CAMERA (2) context raster [0x805F2710] is supported
(every other context type logs an error); source NORMAL/TEXTURE/CAMERATEXTURE → 0x80499394 (2D textured quad with ImTex
binding + BlendModeDirect), source type ≥ 6 or 3 → returns TRUE without drawing, ZBUFFER/CAMERA source → error.
**0 calls on the whole route** (no RwRasterPushContext/RenderFast users). (CONFIRMED) Droppable.

### 6.2 Camera-texture copy 0x80496208 `CopyEFBToCameraTexture(RwRaster *dst, RwBool halfRes)` (recomp_000.cpp:134733)

Called directly by game/effect code (SaveScreen 0x8042E8A4, glare ladder 0x8042F258 / 0x804302EC, PostGlare 0x80430980,
PreGlare 0x80430DE0, shimmer 0x804382F8). 9/frame on Final Haunt.

```
P = dst->parent; PE = P + 0x34
GXSetCopyFilter(GX_FALSE, NULL, GX_FALSE, NULL)
halfRes ? GXSetTexCopySrc(2*dst.nOffsetX, 2*dst.nOffsetY, 2*dst.width, 2*dst.height)
        : GXSetTexCopySrc(dst.nOffsetX, dst.nOffsetY, dst.width, dst.height)
GXSetTexCopyDst(P.width, P.height, PE.gxFmt, /*mipmap(box 2x2)*/ halfRes)
offset = by P.depth (table 0x8056F544): 4:(A8(P.w)*oy + 8*ox)/2  8:A8(P.w)*oy + 4*ox
                                        16:(A4(P.w)*oy + 4*ox)*2  32:(A4(P.w)*oy + 4*ox)*4    (others: error)
GXCopyTex(PE.pixels + offset, clear = [0x805F2688] /* 0 */)
GXPixModeSync()
GXSetCopyFilter(rmode.aa, rmode.sample_pattern, GX_TRUE, rmode.vfilter)   // restore the XFB deflicker filter
PE.region ? GXInvalidateTexRegion(PE.region) : GXInvalidateTexAll()
```
All in-stage destinations are RGBA8 parents at offset (0,0) (640×480 ×2, 128×128, 64×64); the x-term of the offset formula
is therefore never exercised. Because the EFB is RGB8_Z24, the copied alpha is opaque (INFERRED; the host models this with
its "opaque copy alpha" rule). First-hit sources: 0x80430FB0 (PreGlare), 0x80430B5C (PostGlare), 0x8042F420 / 0x804305AC /
0x80430814 / 0x8042F7EC (ladder). (CONFIRMED)

**Render-to-texture model**: there is no off-screen render target. Effects render into the **EFB** (camera = the main
640×480 camera; ladder passes draw with viewport/scissor inherited from the camera and Im2D ortho over 640×480, into the
top-left 128×128 / 64×64 region), then copy that region into a CAMERATEXTURE raster with 0x80496208, then clear it
(mode-1 RwCameraClear) or repaint over it. CAMERATEXTURE rasters are only ever *sampled* textures.

### 6.3 Present: RASTERSHOWRASTER 0x80495CBC (1/frame)

`RwRasterShowRaster(raster, dev, flags)` 0x8048AD94 → stdFunc[20]. The raster argument is **not used** (r31 holds the
interrupt level):
```
level = OSDisableInterrupts()
if (queue full) OSSleepThread(0x805F26D8)          // wait for a retrace to consume an entry
OSRestoreInterrupts(level)
// single-pass ([0x805F2698]==0):
GXFlush(); GXGetFifoPtrs(GXGetCPUFifo(), &rd, &wr)
disable ints; ring[tail] = {xfb=[0x805F26A8], wr}; tokens[count] = token; tail=(tail+1)%N; count=(count+1)%N; restore
if (!breakpointArmed) { armed=1; GXEnableBreakPt(wr) }
GXCopyDisp([0x805F26A8], clear = [0x805F2688] /* 0 */)     // DispCopySrc/Dst/YScale/filter programmed at START
GXSetDrawSync(token); token = (token+1) % 0xE000
GXFlush()
[0x805F26A8] = ([0x805F26A8] == XFB0) ? XFB1 : XFB0
```
Two-pass mode would copy the top and bottom EFB halves on alternate calls (GXSetDispCopySrc(0,2,fbW,efbH−2), clamp flags,
dest offset) — dead here. GXCopyDisp samples: `r3=0x806F3900 / 0x8064E900` alternating, `r4=0`. (CONFIRMED)

### 6.4 Re-implementation notes (copies/present)

* Model the EFB as the single colour+depth render target; implement 0x80496208 as "copy rect → texture of parent raster"
  (with optional 2×2 box downsample) and 0x80495CBC as "resolve EFB (+ the render-mode vfilter deflicker, which the
  console applies at GXCopyDisp) → present". Do not add clears to copies (clear flag is 0).
* The guest reads nothing back from the copy destinations on the CPU (no locks in-stage); a GPU-resident copy is sufficient
  as long as the raster's pixel pointer is never read by guest code (host counters: 0 lazy copies read from MEM1 on this
  route).
* Droppable: XFB alternation, present ring/OSSleepThread, GXDrawSync tokens (they only pace the CPU against the GPU),
  GXPixModeSync, GXInvalidateTex*, copy filter save/restore mechanics (keep the filter *values*).

---

## 7. Im2D and Im3D

### 7.1 Im2D vertex (GameCube layout, 0x18 bytes) — CONFIRMED from the submit loops and DrawBlur's B+0x14 quad

| Off | Type | Field |
|---|---|---|
| +0x00 | f32 | x (EFB pixels, before the +0.5 matrix offset) |
| +0x04 | f32 | y |
| +0x08 | f32 | z (screen depth 0..1; RwIm2DGetNearScreenZ = 0.0, Far = 0.99999994) |
| +0x0C | u8 ×4 | R, G, B, A (RwRGBA order) |
| +0x10 | f32 | u |
| +0x14 | f32 | v |

There is no rhw / recipZ field: Im2D on GC is affine (orthographic, no perspective-correct UVs).

### 7.2 Primitive mapping (0x8056F120; Im3D copy 0x8056FD00)

| RwPrimitiveType | 1 LINELIST | 2 POLYLINE | 3 TRILIST | 4 TRISTRIP | 5 TRIFAN | 6 POINTLIST |
|---|---|---|---|---|---|---|
| GXPrimitive | 0xA8 GX_LINES | 0xB0 GX_LINESTRIP | 0x90 GX_TRIANGLES | 0x98 GX_TRIANGLESTRIP | 0xA0 GX_TRIANGLEFAN | 0xB8 GX_POINTS (accepted by the map, rejected by the submitters: type ≥ 6 → error) |

`GXBegin(prim, GX_VTXFMT0, (u16)n)`; LINELIST emits ⌊n/2⌋·2 vertices, TRILIST ⌊n/3⌋·3, others n. Indexed: `u16`
`RwImVertexIndex`, vertices looked up on the CPU, `GXBegin(prim, 0, numIndices)`.

### 7.3 Per-primitive GX state: Im2DBegin 0x8049148C / Im2DEnd 0x80491774

Begin (every Im2D call, 89/f):
```
GXClearVtxDesc()
GXSetVtxDesc(GX_VA_POS, GX_DIRECT);  GXSetVtxAttrFmt(VTXFMT0, POS, GX_POS_XYZ, GX_F32, 0)
GXSetVtxDesc(GX_VA_CLR0, GX_DIRECT); GXSetVtxAttrFmt(VTXFMT0, CLR0, GX_CLR_RGBA, GX_RGBA8, 0)
GXSetNumTevStages(1); GXSetNumChans(1)
GXSetChanCtrl(GX_COLOR0A0, lighting off, amb REG, mat VTX, 0, NONE, AF_NONE)
GXSetChanCtrl(GX_COLOR1A1, off, REG, REG, 0, NONE, AF_NONE)
if (ImTex->raster):                                           // textured
    GXSetVtxDesc(GX_VA_TEX0, DIRECT); GXSetVtxAttrFmt(VTXFMT0, TEX0, GX_TEX_ST, GX_F32, 0)
    GXSetTevOp(GX_TEVSTAGE0, GX_MODULATE); GXSetNumTexGens(1)
    GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, FALSE, GX_PTIDENTITY)
    GXSetTevOrder(0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0)
    0x80498854()                                              // bind ImTex + alpha-test mode from texture alpha
else:
    GXSetNumTexGens(0); GXSetTevOrder(0, NULL, NULL, GX_COLOR0A0); GXSetTevOp(0, GX_PASSCLR)
GXSetViewport(0, 0, rmode.fbWidth, rmode.xfbHeight, 0.0, 1.0)   // Jitter variant if field rendering
GXGetProjectionv(save 0x805E4218)
proj 0x8056F13C = {ORTHO, 2/fbWidth, −1, −2/xfbHeight, +1, −1, −1}; GXSetProjectionv
posMtx 0x8056F158 = [1 0 0 cam.fb.nOffsetX+0.5; 0 1 0 cam.fb.nOffsetY+0.5; 0 0 −1 0]; GXLoadPosMtxImm(PNMTX0); GXSetCurrentMtx(0)
```
End: if the current camera's frame buffer is a sub-raster, restore viewport and scissor to its rect (two-pass variants
omitted); `GXSetProjectionv(saved)`. The Im2D path does **not** touch blend, Z, cull, fog or alpha compare beyond
0x80498854 — those come from the render-state cache as last emitted.

Resulting mapping (640×480): `x_clip = 2(x+0.5)/640 − 1`, `y_clip = 1 − 2(y+0.5)/480`, depth01 = z. The scissor is the
camera's (full EFB in-stage). Viewport is always the full render-mode frame even when the camera raster is smaller, so Im2D
coordinates are always EFB pixel coordinates. The +0.5 places integer vertex coordinates on GX pixel centres: full-screen
quads authored as (0,0)–(640,480) cover [0.5, 640.5]×[0.5, 480.5], which at 1× contains every pixel centre (the host
documents the same fact at renderer.cpp:3350). (CONFIRMED)

### 7.4 Im2D producers

Static sites of `+0x30`: 0x800366D4, 0x8003972C, 0x800448F8 ×2, 0x8006227C ×2, 0x800BCCDC, 0x802E51B0 (POLYLINE),
0x802E5448 (POLYLINE), 0x80306388, 0x803436A0, 0x80343768, 0x80343FE0, 0x80344150 ×4, 0x80346A94; `+0x34`: 0x804797B0.
Plus the effect library (DrawBlur 0x8042E2B8, SaveScreen, glare ladder quad builder 0x80430850, screen color, shimmer,
CSD 2D renderer 0x80364xxx), which keeps the engine pointer in a saved register (not in the static count). Runtime: 89/f,
87 textured.

### 7.5 Im3D (pipeline-based)

Nodes (created by 0x804A08FC `_rwIm3DCreatePlatformTransformPipeline` and 0x804A0A6C `...RenderPipelines`, registered
for prim types 3, 5, 4, 1, 2):
* **ImmInstance.csl** 0x804A0B4C: `[0x805F2748] = *(param)` (the Im3D stash).
* **SubmitNoLight.csl** 0x804A0B68 (recomp_028.cpp:189466): stash fields `+0x00 u16 numVerts`, `+0x04 RwIm3DVertex*`,
  `+0x0C flags (bit0 rwIM3D_VERTEXUV)`, `+0x10 RwMatrix *ltm`, `+0x30 primType`, `+0x34 u16 *indices (NULL = not indexed)`,
  `+0x38 numIndices`. GX: `SetTransform(ltm, FALSE)` (§5.3 step 5), same VtxDesc/TEV/channel set-up as Im2D (POS, CLR0,
  optional TEX0 + 0x80498854), `GXBegin(map[prim], VTXFMT0, count)`, vertices from:

| Off | Type | Field (RwIm3DVertex, 0x24 bytes) |
|---|---|---|
| +0x00 | f32 ×3 | object-space position |
| +0x0C | f32 ×3 | normal (ignored: NoLight) |
| +0x18 | u8 ×4 | R, G, B, A |
| +0x1C | f32 | u |
| +0x20 | f32 | v |

Projection is the camera's (perspective); no viewport change. Runtime: 49,912 node executions before 40e9 (boot, title,
menus), **0 in the Final Haunt window**. (CONFIRMED)

### 7.6 Re-implementation notes (Im2D/Im3D)

* Replace `fpIm2DRenderPrimitive` (and for safety the other three Im2D fps) with a host immediate batch: 24-byte vertex,
  affine UV, colour modulate or pass-colour, EFB-pixel ortho with the +0.5 offset and the camera raster's nOffset, depth =
  z, blend/Z/cull/fog/alpha from the emulated GX state, texture = ImTex (NULL → untextured, not white).
* Im3D: implement the SubmitNoLight semantics (36-byte vertex, ltm×view, no lighting) if menus/boot are in scope; it is not
  needed for in-stage frames.
* Droppable: vertex-format descriptor programming, GXBegin count truncation, projection save/restore mechanics.

---

## 8. Final Haunt per-frame census (Δ 40e9→50e9 / 1232 frames)

| Group | Entry | / frame |
|---|---|---:|
| Present | RwRasterShowRaster → stdFunc[20] 0x80495CBC; GXCopyDisp | 1 |
| Cameras | RwCameraBeginUpdate / EndUpdate (all → stdFunc[1]/[10]) | 32 / 32 |
| Clears | RwCameraClear → stdFunc[21] → ClearQuad 0x80499E58 | 8 |
| EFB copies | 0x80496208 → GXCopyTex; GXSetCopyFilter | 9; 18 |
| Sub-rasters | RwRasterSubRaster → stdFunc[12] | 2 |
| Render states | RwRenderStateSet / Get | 615.39 / 181.19 |
| | SRCBLEND, DESTBLEND | 136.19 each |
| | FOGENABLE, ZTEST, ZWRITE, CULLMODE, TEXTUREFILTER, TEXTURERASTER | 51, 49, 48, 47, 43, 35 |
| | TEXTUREADDRESSU/V, TEXTUREADDRESS, SHADEMODE, FOGCOLOR, FOGTYPE | 12/12, 2, 2, 1, 1 |
| | no-op states (VERTEXALPHAENABLE etc.) | ≈40 |
| Alpha extension | SetAlphaTestMode 0x804987C8 / SetAlphaCompare 0x80498F74 | 194.19 / 19 |
| Textures | TextureBind 0x8049AC88 (GXLoadTexObj) / TEXTURESETRASTER / GXInitTexObj / GXInitTexObjCI / GXLoadTlut | 273.29 / 29.1 / 23.1 / 13 / 187.19 |
| Transforms | SetTransform 0x804960FC / GXLoadPosMtxImm / GXSetProjectionv | 91.1 / 188.1 / 218 |
| Im2D | fpIm2DRenderPrimitive (= Im2DBegin = Im2DEnd) / ImTex bind | 89 / 87 |
| Im3D | SubmitNoLight node | 0 |
| GX state sinks | GXSetBlendMode / GXSetZMode / GXSetZCompLoc / GXSetAlphaCompare / GXSetCullMode / GXSetFog | 162.39 / 28 / 40 / 47 / 26 / 38 |
| | GXSetViewport / GXSetScissor / GXBegin (all callers) | 129 / 40 / 113 |
| Device | fpSystem | 2 |
| Zero | Im2D line/triangle/indexed, stdFunc 2,3,4,5,6,7,9,11,13–19,23–28, GXSetPixelFmt, GXSetViewportJitter, ALPHATESTFUNCTION(REF) | 0 |

---

## 9. Cut line (what a native renderer must intercept here)

**Intercept (replace) — the complete device surface the game uses in-stage:**

| Entry | Why |
|---|---|
| RwGlobals+0x20 fpRenderStateSet 0x80498954, +0x24 fpRenderStateGet 0x804984C4 | all RW-level state |
| RwGlobals+0x30 fpIm2DRenderPrimitive 0x80491E80 (+0x28/+0x2C/+0x34 for completeness) | all 2D/effect quads |
| stdFunc[1] 0x804956F4, stdFunc[10] 0x80495CAC | camera → view/projection/viewport/scissor |
| stdFunc[21] 0x80495638 | clears |
| stdFunc[20] 0x80495CBC | present |
| stdFunc[4]/[5]/[8]/[12]/[15]/[16]/[26] | raster/texture lifetime and TXD decode (host texture creation) |
| 0x8049AC88 TextureBind, 0x804960FC SetTransform | called by the RwGC pipelines (other lanes) — shared chokepoints |
| 0x804987C8 SetAlphaTestMode, 0x80498F74 SetAlphaCompare, 0x80498854 ImTex bind | GC extension API called by game code |
| 0x80496208 CameraTextureCopy | EFB→texture copies called by effect code |
| 0x8049B568 SetTextureLOD | called by game code |
| ImmInstance/SubmitNoLight nodes 0x804A0B4C / 0x804A0B68 | Im3D (boot/menus only) |

**Must reproduce exactly (game data or visible output depends on it):** RwGlobals/RwRaster/RwTexture/RwCamera field offsets
the game reads or writes (engine pointer slots, filterAddressing, raster size/offsets/parent, camera near/far/fogPlane/
viewWindow, cache byte 0x805E431D); TXD native stream layout; RW enum semantics including rejected values and no-op
states; blend/cull/Z/fog mappings; the emitted-state lag of 0x80498F74 and the FOGCOLOR fog re-arm; Im2D +0.5 pixel-centre
convention and EFB-pixel ortho; clear-by-quad at z=0.99999994 with the side effects listed in §5.5; copy rectangles and
2×2 box downsample; RGB8 EFB (opaque copies, dst alpha = 1); texture filter/wrap tables; one-level textures.

**Droppable (exist only for GX/Flipper):** FIFO/draw-sync tokens and waits, XFB ring and OSSleepThread pacing, breakpoints,
DCFlushRange/GXInvalidateTex*, GXTexObj/GXTlutObj storage and TLUT-slot coupling, pixel-format toggling, two-pass half-EFB
and field-rendering branches, GXSetCurrentGXThread, vertex-format descriptor programming, memory rounding, preloaded regions,
RASTERRENDER family (unused), stencil/perspective/shade/border/density states.

---

## 10. Corrections to existing documentation

1. **0x805F265C is `RwEngineInstance` (pointer), not the device object**; RwGlobals lives at 0x8060E340 and the device at
   RwGlobals+0x10 (static template 0x8056F50C). Offsets "+0x20 RwRenderStateSet / +0x24 Get / +0x30 Im2D" in docs 00, 07, 10,
   11, 14, 19 are correct as RwGlobals offsets, but "device (0x805F265C)" is a mislabel.
2. **RwGlobals+0x108/+0x10C are `rwmalloc`/`rwfree`**, not "game-range state thunk slots": 0x80049B5C (90 callers) is an RwFree
   trampoline and 0x801DA004 a free wrapper (doc 03 table row "RenderStateGet wrapper 0x801DA004 → 0x80049B5C" and doc 19's
   "+0x108/+0x10C" description are wrong; 0x801DA024 is the matching alloc wrapper, hint 2+0x32).
3. **Standard-function table 0x8051D400 is complete at 27 entries, ending at 0x8051D4D8** (ids 25/27/26/28 are the last four;
   ids 0 and 22 are absent → stub 0x80494CCC). Resolves doc 20 open question "may continue past 0x8051D4B0".
4. **RwGameCubeCameraClear does not use GXSetCopyClear in this game** (doc 14 row 0x80495638, doc 11 §fog/backdrop wording
   "GXSetCopyClear + Z-fill quad; EFB clear is realized by the next copy"): `[0x805F2688]` = 0, so the clear is the
   ClearQuad only (colour and/or depth), and GXCopyDisp/GXCopyTex are issued with clear = FALSE. The backdrop is the clear
   quad's colour.
5. **VERTEXALPHAENABLE is a no-op on GC** (jump-table entry = common exit returning FALSE). Doc 10's DrawBlur description
   "sets VERTEXALPHA=1" has no GX effect; blending is always enabled.
6. **ZTEST=0 does not disable the Z compare** (GXSetZMode(TRUE, GX_ALWAYS, zwrite)); any description implying "depth test
   off" for the GX translation must keep writes.
7. **Alpha-test "policy" 0x804987C8 semantics**: mode 1 = alpha test OFF + early Z, mode 0 = cached compare + late Z (doc 07
   §alpha compare / master index "alpha-test/ZCompLoc policy"). 0x80498F74 re-emits the previous compare (lag).
8. **ShowRaster 0x80495CBC ignores its raster argument**, has no GXSetCopyClamp/GXSetDispCopySrc in the single-pass path
   (those calls belong to the dead two-pass branch; doc 14 row lists them as part of the per-frame present).
9. **Im3D render callback 0x804A0B68 is not an in-stage path** (0 hits on Final Haunt; 49,912 at boot/menus), and the
   device `fpIm3D*` slots are NULL; doc 20/21 rows present it as a live pipeline.
10. **Camera end update performs no GX work** (stdFunc[10] only clears 0x805F26F8); there is no per-camera EFB copy.
11. **Render mode in-stage is 640×480** (xfbHeight 480, EURGB60); "640×528" is the boot PAL50 mode and the host's internal EFB
    size. The clear-quad 2D setup hard-codes 640×528 (projection 2/639, −2/527) only for CAMERATEXTURE framebuffers, which the
    game never begins in-stage.
12. **All generated line numbers in RECOMP_VISUAL_MAP docs/CSVs are stale** against the current corpus (e.g. 0x80498954 is
    recomp_030.cpp:174157, not :148126; 0x80496208 recomp_000.cpp:134733, not :115845; 0x80495CBC recomp_061.cpp:156814, not
    :138855). `generated/fnindex.txt` is authoritative.
13. **Texture formats**: the disc contains no I4/I8/IA4/IA8/C14X2/RGBA8 textures and no mipmaps; every TXD stores
    filterAddressing 0x1101. RGBA8 appears only as camera-texture copy targets.
14. `RwGameCubeCameraTextureCopy` restores the copy filter from the render mode (vfilter) after every texture copy, which is
    why the XFB deflicker filter is active only for GXCopyDisp (consistent with memory note "copy filter dropped").

---

## 11. Open questions

1. The exact request ids behind the 2 fpSystem calls per frame in-stage (first-hit samples are boot-time: 0x0A GETMODE,
   0x06 GETMODEINFO, 0x16 GETID).
2. Per-value distributions of dynamic RenderStateSet values in-stage (the static census gives constant pairs; `--watch`
   exposes only the first 8 register samples). An observe-only per-(state,value) counter would close this.
3. Which in-stage callers produce the 89 Im2D primitives and 35 TEXTURERASTER changes (CSD HUD vs effect quads) — belongs to
   the 2D/effects lanes; needs a caller histogram.
4. GX pixel-centre/sub-pixel precision (1/12 vs 1/16) and the exact rasterisation of the clear quad at z=0.99999994 versus
   copy-clear 0xFFFFFF — relevant only to byte-identity gates.
5. 0x8049677C (unlock-time format conversion) and 0x8049A124 (raster clear) were not decoded in detail (0 in-stage uses).
6. Whether any code path sets the texture-extension bit 0x02000000 (preloaded TLUT/region); no writer was found and runtime
   counts show none.

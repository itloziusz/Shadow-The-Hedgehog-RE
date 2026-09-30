# 03 - Frame render graph and screen-space passes

Shadow the Hedgehog, GameCube PAL (disc GUPP8P), RenderWare 3.7 GameCube driver ("RwGC").
Lane "graph": the per-frame pass list, every EFB copy, clear, Z-buffer lifetime, 2D/fade passes,
split screen and present, written as a render-pass specification for a native DX12 renderer.

Companion data: `03_frame_sequence_final_haunt.csv` (46 rows, one per pass/sub-pass, with the
measured Final Haunt per-frame counts, state, inputs, outputs, copy rectangles and formats).

Evidence tags: **CONFIRMED** = code read and runtime-observed in this lane's runs; **CODE** = read in
code only; **INFERRED** = reasoned, not verified. Generated sources are
`GekkoForge/build-native/generated/recomp_NNN.cpp:line` (function start line from
`RECOMP_VISUAL_MAP/db/functions.csv`). PPC listings were taken straight from `sys/main.dol` with a
capstone disassembler (the extractor's `functions.jsonl` truncates bodies at every `bctrl`).
Built on `RECOMP_VISUAL_MAP/14_RECOMP_RENDER_ORDER_MAP.md` and `10_RECOMP_POSTPROCESS_MAP.md`; where
this document disagrees with them, section 11 lists the correction and the evidence.

---

## 0. How the numbers were obtained

Private exe copy `RENDERER_RE/_work/graph/exe/ShadowTheHedgehog.exe` (from
`GekkoForge/scratch/stage-boot-2026-09-16/exe`), save-data recreated from the seed before every run,
headless, `--no-audio --no-call-trace --service-interval 65536`, navigation only with `--boot-stage`.
Four runs, all with the same 61 `--watch` hooks (hit counters; observe-only):

| run | stage | budget | extra diagnostics |
|---|---|---|---|
| fhA | stg0604 Final Haunt | 44e9 | `--dump` of static objects |
| fhB | stg0604 Final Haunt | 46e9 | `--internal-scale 1 --effect-geometry --hud-geometry --hud-geometry-frames 5500-5502 --screenshot-efb-native --native-efb-frame 5501`, dumps of E/G/B/C/S/LF, rasters, rmode, engine, level tasks |
| lavaA | stg0602 Lava Shelter | 44e9 | - |
| lavaB | stg0602 Lava Shelter | 46e9 | same as fhB plus a 32 KiB dump of the task heap (runtime render-tree walk) |

**Per-frame counts = (B - A) / frames.** Both pairs are deterministic prefixes of the same boot:
the render-tree group execs advanced by exactly 247 (FH: 247/248 boundary) and the XFB submissions by
exactly 247 in both stages, so every count below is "per displayed in-stage frame" over 247 frames
of gameplay (Shadow idle after the stage intro). Raw data: `RENDERER_RE/_work/graph/runs/`.
Nothing in this lane reports wall-clock time.

---

## 1. Executive summary

The game has no explicit render loop. One scheduler pump per frame walks a task tree in preorder;
the render part of that tree is **Layer5 "Render" -> 4 RenderAttach groups -> 23 RenderLevel
containers**, and the preorder of that tree *is* the pass order. Everything is drawn into a single
640x480 region of the 640x528 EFB (pixel format RGB8_Z24, no destination alpha). "Render targets"
are faked with `GXCopyTex` into RGBA8 camera-texture rasters, and several passes paint scratch
images into the top-left corner of the same EFB, copy them out, and wipe the screen afterwards.

Final Haunt, one in-stage frame (all CONFIRMED by counts):

```
PreRender   RenderStart: clear-by-quad colour (0,0,0) + Z far                     1 clear
Div (x1 in 1P, x2 per viewport in split screen)
  OpeqWorld  8 LandDispManager world classes (+ FullScreenShimmer child of land N)
  Opeq       CameraRender(0x20) objects, EffectDrawOpaq (bucket 0), TFogSet
  Punch      CameraRender(0x800) EffectDrawPunch (bucket 1)
  TransWorld 3 land classes;  Trans CameraRender(0x40) objects + EffectDrawTrans
  AddWorld   2 land classes;  Add   CameraRender(0x80) EffectDrawAdd + EffectDrawSub
  GlareWorld PreGlare: copy scene -> 640x480 tex, clear colour only (Z kept)      1 copy 1 clear
             lens flare (pass 1), EffectDrawGlare (bucket 5), land G/GL, lens flare (pass 0)
  Glare      CameraRender(0x80) (+ stage gimmicks)
  PostGlare  PostGlare: copy glare source -> 640x480 tex                          1 copy
             ladder: 128 capture, 128 H/V, 64 capture, 64 H/V, merge, composites  7 copies 6 clears(*)
             add saved scene back ONE/ONE
             DrawBlur (temporal/zoom composite of 320x240 history; gated off)
             EffectDrawShimmer (bucket 6; dormant: E+0x4C == NULL)
  Sprite3D   CameraRender(0x20) world-space sprites;  Last
Static
  PostEffect SaveScreen (history capture, gated off), DrawScreenColor (gated off)
  Sprite     Set2D, CameraRender(0x418), HUD (58 Im2D prims, 2 CSD projects)
  OnFade     FadeDraw (idle); Gindows SaveMessageDraw; SpriteLast Reset2D
End
  PostRender RenderStop -> RwCameraShowRaster -> GXCopyDisp (deflicker) -> XFB      1 XFB copy
(*) 6 of the 8 per-frame clears are inside the ladder; PreGlare and RenderStart own the other two.
```

Measured totals per frame, Final Haunt / Lava Shelter: GXCopyTex **9 / 10**, GXCopyDisp **1 / 1**,
clears **8 / 8**, RwCameraBeginUpdate **32 / 30**, Im2D primitives **89 / 86**, GXBegin **113 / 120**,
GXCallDisplayList **176.3 / 201.3**, GX primitives decoded **1061 / 650**, triangles **9021 / 7273**.
Lava Shelter's extra copy is the full-screen heat shimmer (active there, gated off in Final Haunt).

---

## 2. Frame skeleton (the render task tree)

### 2.1 Chain above the tree (CODE, one level of CONFIRMED)

`fn_801781B8` (GameFrame_Tick) -> `fn_801EB624` (recomp_057.cpp:39381: kill pass `fn_8004EECC`,
dt = `[0x805F7198]`, `fn_8004ECAC` over the root, `fn_801EB4A4`). `fn_8004ECAC` (recomp_041.cpp:4831)
executes a task's exec `[vt+0x0C](task, f1=dt)` then recurses into its children (first child +0x14,
next sibling +0x0C), so **order = parent first, then children in creation (append) order**.
Pause byte `0x805EF27C`: only tasks with flag `0x100` run while paused; every render task below has it
(dump) except `TFogSet` (flags 0x0000). Observation: a `--watch` on `0x801EB624` fired once in a 44e9
run while every render-tree exec fired once per frame; the tree is certainly pumped per frame, the
entry that does it is left as an open question (section 12).

### 2.2 Runtime tree (CONFIRMED - walked from the lavaB heap dump; FH differs only in stage tasks)

Level descriptor table `0x804D24EC` (23 x 0x14 `{name, stateEnable, stateFlags, camMode, group}`) is
confirmed byte-for-byte as in doc 14. The runtime child lists (task name at +0x00, exec from vtable
+0x0C):

```
RenderAttach_Start            exec 0x801E5100  camMgr+0x20 = V+0x60 (split mode)
  RenderLevel_PreRender
    RenderStart               0x801D032C  begin-of-frame clear (section 4.1)
    EffectDeleteResourceTask  0x801D2E78
RenderAttach_Div              exec 0x801E4EF0  viewport loop (section 7)
  RenderLevel_OpeqWorld
    LandDispManager_N         0x8004620C
      FullScreenShimmerDraw   0x801D25F0  (child of the land task, section 4.3)
    LandDispManager_D, _OS, _OSL, _O, _OW, _OL, _P
  RenderLevel_Opeq
    CameraRender              0x801D04D4  (state payload, flags 0x20)
    RenderUnit                0x8004E390  (generic object draw callback)
    EffectDrawOpaq            0x801D2B14
    TFogSet                   0x8016A904
  RenderLevel_PunchWorld
    CameraRenderClose         0x801D0418
  RenderLevel_Punch
    CameraRender (0x800), EffectDrawPunch 0x801D2B90
  RenderLevel_TransWorld
    CameraRenderClose, LandDispManager_AF, _A, _AL
  RenderLevel_Trans
    CameraRender (0x40), RenderUnit, EffectDrawTrans 0x801D2C0C, [stage task]
  RenderLevel_AddWorld
    CameraRenderClose, LandDispManager_K, _M
  RenderLevel_Add
    CameraRender (0x80), EffectDrawAdd 0x801D2C88, EffectDrawSub 0x801D2D04
  RenderLevel_GlareWorld
    CameraRenderClose, PreGlare 0x801D266C, DrawGlobalLensFlare 0x801D2928,
    EffectDrawGlare 0x801D2D80, LandDispManager_G, _GL, [stage-heap task: lens flare pass 0]
  RenderLevel_Glare
    CameraRender (0x80)
  RenderLevel_PostGlare
    CameraRenderClose, PostGlare 0x801D26E8, DrawBlur 0x801D27FC, EffectDrawShimmer 0x801D2DFC
  RenderLevel_Sprite3D
    CameraRender (0x20), [stage task]
  RenderLevel_Last
    CameraRenderClose
RenderAttach_Static           exec 0x801E4E40  layout 0 + camera 0 (full screen)
  RenderLevel_PostEffect
    SaveScreen 0x801D2764, DrawScreenColor 0x801D2878          (no CameraRender payload)
  RenderLevel_Sprite
    CameraMatrix->Set2D 0x801D019C, CameraRender (0x418),
    RenderLevel_SpriteBack (empty), RenderLevel_SpriteMiddle (empty),
    RenderLevel_SpriteFront { MessageTask 0x801FB5AC, RenderUnit, RenderUnit },
    RenderUnit x5 (HUD)
  RenderLevel_OnFade
    CameraRenderClose, CameraRender (0x418), FadeDrawTask 0x80044E08  (+StageTitleTask when loading)
  RenderLevel_Gindows
    CameraRenderClose, CameraRender (0x418), SaveMessageDrawTask 0x802E61AC
  RenderLevel_SpriteLast
    CameraRenderClose, CameraMatrix->Reset2D
RenderAttach_End              exec 0x801E50B4 (blr)
  RenderLevel_PostRender
    RenderStop                0x801D0250  present (section 4.14)
```

Facts a renderer depends on:

* **Payload names**: the SetRenderState payload is the task named `CameraRender`
  (`0x801D04D4`, recomp_027.cpp:46109): `RpWorldAddCamera(world,cam)` (`0x80463180`), then
  **`RwCameraBeginUpdate(cam)`** (`0x8048652C`), then the level flags through `fn_80046D80`, then
  world lights. `CameraRenderClose` (`0x801D0418`, recomp_025.cpp:38661) is
  **`RwCameraEndUpdate`** + `RpWorldRemoveCamera`. The Close is the first child of the *next* level,
  so every state level is one camera-update bracket. **Close does not reset render states** - the
  last level's states stay latched into whatever runs next (this matters for SaveScreen, 4.11).
* **Levels without a payload** (OpeqWorld, TransWorld, AddWorld, GlareWorld, PostGlare, PostEffect)
  run their tasks outside any camera bracket; each of those tasks brackets the camera itself.
* **RenderUnit** (`0x8004E390`, recomp_010.cpp:10797) is the generic draw-callback task: exec calls
  `[[task+0x28]+0x10]`. It is created by the level attach helpers `fn_8004E290` / `fn_8004DFF4` /
  `fn_8004E318` (INFERRED from the 60+ attach sites that pass `RenderTaskSystem+off` to them; the
  runtime tree shows RenderUnit tasks exactly in Opeq, Trans, Sprite and SpriteFront). Game objects
  and HUD widgets draw through these; they are above the cut line.
* Inside `Sprite`, the three sub-levels run **before** the RenderUnits attached directly to Sprite,
  so "SpriteFront" is not the front-most HUD plane.

### 2.3 World classes (CONFIRMED)

`fn_800460C0` (recomp_060.cpp:6331, called from `fn_801783D4` at `0x80178594`) creates 15
`LandDispManager_*` tasks from table `0x8051E328` (stride 0x18: `+0x00 ptr to an sdata slot,
+0x04 name, +0x08 flags, +0x0C level index, +0x10 global RpWorld (runtime), +0x14 task pointer
(runtime, written by fn_8004625C)`). Exec `0x8004620C` -> `fn_800471BC(ctx, camera, index)`
(recomp_029.cpp:6394): for each visible RpWorld of that class (up to 32 section worlds + one global),
`RpWorldAddCamera` -> `RwCameraBeginUpdate` -> `fn_80046D80(ctx, flags, world)` ->
`RpWorldRender` (`0x80460FAC`) -> `RwCameraEndUpdate` -> `RpWorldRemoveCamera`. Measured: 15 execs per
frame in both stages.

| class | flags | level | decoded state (fn_80046D80, table 3.6) |
|---|---|---|---|
| N, D | 0x28 | OpeqWorld | opaque default arm, LINEAR |
| OS, O | 0x2C | OpeqWorld | opaque, LINEAR, fog |
| OSL, OL | 0x2D | OpeqWorld | opaque, LINEAR, fog, lights |
| OW | 0x3C | OpeqWorld | opaque, LINEAR, fog, cull none |
| P | 0x80C | OpeqWorld | punch (alpha > 200, Z write), LINEAR, fog |
| AF, A | 0x5C | TransWorld | alpha blend, Z test no write, LINEAR, fog, cull none |
| AL | 0x4D | TransWorld | alpha blend, LINEAR, fog, lights |
| K | 0x9C | AddWorld | additive, LINEAR, fog, cull none |
| M | 0x11C | AddWorld | colour invert blend (INVDESTCOLOR, ZERO), LINEAR, fog, cull none |
| G | 0x98 | GlareWorld | additive, LINEAR, cull none, no fog |
| GL | 0x99 | GlareWorld | additive, LINEAR, cull none, lights |

`RenderLevel_PunchWorld` has no world class: punch-through landscape is drawn in OpeqWorld.

---

## 3. Mechanics shared by every pass (the RW primitives a native renderer must honour)

### 3.1 Camera begin: viewport, scissor, projection (CONFIRMED)

`RwCameraBeginUpdate` = `[cam+0x18]` -> device `RwGameCubeCameraBeginUpdate` `0x804956F4`
(recomp_059.cpp:134190). It stores the current camera at `0x805F26F8`, builds the view matrix,
loads `GXSetProjectionv` (perspective: `p1=cam+0x70 recipViewWindow.x`, `p3=+0x74`,
`p5=-near/(far-near)`, `p6=far*p5`; parallel: `p0=1`, `p5=-1/(far-near)`), then from the frame-buffer
raster `fb=[cam+0x60]`:

* **viewport = (fb.offX, fb.offY, fb.w, fb.h, near 0.0, far 1.0)**, **scissor = same rect**,
  scissor box offset (0,0). In 1P the main camera's fb is 640x480 at (0,0).
* If `fb` is a camera-texture raster (type 5) the EFB pixel format is switched to RGBA6_Z24 when the
  raster's extension bit 0 is set. Measured **0** `GXSetPixelFmt` calls in-stage: never used.
* Field-rendering and "rmode AA" variants exist (`0x805F2698`); PAL60 frame mode takes the plain path.

`RwCameraEndUpdate` = `0x80495CAC`: clears `0x805F26F8`. Per frame: 32 (FH) / 30 (Lava).

### 3.2 Clear = a quad, never a copy-clear (CONFIRMED)

`RwCameraClear(cam, rgba, mode)` = `0x8048680C` -> std fn 0x15 `0x80495638` (recomp_058.cpp:142490).
Because the GX copy-clear flag byte `0x805F2688` is 0 (dump), it **always** draws
`fn_80499E58(fbRaster, rect=fb (x,y,w,h), rgba, mode)` (recomp_060.cpp:134007):

| mode | colour | Z |
|---|---|---|
| 1 IMAGE | material colour = rgba, ColorUpdate on | ZMode(test on, ALWAYS, **no write**) |
| 2 Z | ColorUpdate off | ZMode(on, ALWAYS, write) |
| 3 IMAGE+Z | rgba | ZMode(on, ALWAYS, write) |

Common: `GXSetBlendMode(BLEND, ONE, ZERO)`, cull none, alpha test off (`fn_804987C8(1)`), fog forced
off and restored, `fn_80499B14` Im2D setup (viewport (0,0,640,480), scissor (0,0,640,480), ortho,
position matrix with +0.5 translation), one `GX_QUADS` of 4 vertices at **z = 0.99999994**
(`0x805FC488`) which the Im2D projection maps to depth ~0xFFFFFE (far), then state restore.
Measured on the effect-geometry rows: clears have `blend 1/0, ztest 1, zfunc 7, zwrite 0`.
Per frame: 8 in both stages (RenderStart 1, PreGlare 1, ladder 6).

Native: a render-target clear of colour (and depth to far) over the camera rect. Only the rect
matters; the quad is a Flipper-era idiom.

### 3.3 Im2D primitive pipeline (CONFIRMED)

Device function table (engine `0x8060E340` dump): `+0x20 fpRenderStateSet 0x80498954`,
`+0x24 fpRenderStateGet 0x804984C4`, `+0x28 Im2DRenderLine 0x80491D0C`,
`+0x2C Im2DRenderTriangle 0x80491B08`, **`+0x30 Im2DRenderPrimitive 0x80491E80`**,
`+0x34 Im2DRenderIndexedPrimitive 0x8049245C` (0 hits in-stage); `+0x18 zBufferNear = 0.0`,
`+0x1C zBufferFar = 0.99999994`. Every screen pass uses `Im2DRenderPrimitive(4 = TRISTRIP, verts, n)`.

`fn_8049148C` (recomp_039.cpp:126973), called per primitive:

* vertex format: POS xyz f32, CLR0 RGBA8, TEX0 st f32 (only when a texture raster is bound).
* **`RwIm2DVertex` layout, stride 0x18**: `+0x00 x, +0x04 y, +0x08 z, +0x0C RGBA (u8 x4),
  +0x10 u, +0x14 v`.
* TEV: textured -> 1 stage `GX_MODULATE` (texture x vertex colour, alpha too); untextured ->
  `GX_PASSCLR`. Lighting off, material source = vertex.
* viewport **(0, 0, rmode.fbWidth=640, rmode.xfbHeight=480, 0, 1)** - the whole screen, not the
  camera sub-rect. Scissor is **not** touched (the camera bracket's scissor applies).
* projection: ortho `{1, 2/640, -1, -2/480, 1, -1, -1}` (array `0x8056F13C`).
* position matrix `0x8056F158` = `[1 0 0 fb.offX+0.5; 0 1 0 fb.offY+0.5; 0 0 -1 0]`:
  **Im2D coordinates are relative to the current camera frame-buffer origin and shifted +0.5
  pixel**. With vertex x = integer, the edge sits on a pixel centre.
* depth: z_clip = z - 1, so z = 0 is depth 0 (nearest) and z = 0.99999994 is far. All effect quads
  below use z = 0.

Render-state ids used by the screen passes (jump table `0x8056FAD4`, CONFIRMED by the calls):
1 TEXTURERASTER, 2 TEXTUREADDRESS, 6 ZTESTENABLE (1 -> GX_LEQUAL), 7 SHADEMODE, 8 ZWRITEENABLE,
9 TEXTUREFILTER (1 NEAREST, 2 LINEAR, 0 = raster default), 10 SRCBLEND, 11 DESTBLEND,
12 VERTEXALPHAENABLE (**no-op on this driver**: table slot 12 is the default), 14 FOGENABLE,
20 CULLMODE (1 none, 2 back). RW blend enum -> GX factor: ZERO 1->0, ONE 2->1, SRCALPHA 5->4,
INVSRCALPHA 6->5, INVDESTCOLOR 10->INVDSTCLR.

### 3.4 EFB -> texture copy (CONFIRMED)

`RwGameCubeCameraTextureCopy` `fn_80496208(raster, halve=0)` (recomp_000.cpp:115845); all 4 GXCopyTex
sites in the DOL are its format fragments `0x804962E4/0x8049630C/0x80496330/0x80496358`.

1. `GXSetCopyFilter(aa 0, pattern 0, vf 0, 0)` - identity (no deflicker on texture copies).
2. `GXSetTexCopySrc(raster.offX, raster.offY, raster.w, raster.h)` (the sub-raster's own rect; the
   `halve` arm doubles it and is never taken).
3. `GXSetTexCopyDst(parent.w, parent.h, ext.gxFormat, halve)`, destination memory =
   `parent.ext.pixels + tileOffset(sub-raster offset)`.
4. `GXCopyTex(dest, clear = byte 0x805F2688 = 0)` - **never clears the EFB**.
5. `GXPixModeSync`, restore the display copy filter from rmode, `GXInvalidateTexRegion/All`.

All camera-texture rasters are RW format `0x505` = 8888 camera texture -> **GX_TF_RGBA8** (dump:
`ext+0x0C = 6`). The EFB is RGB8, so **every copy has alpha = 0xFF**; passes that blend with
SRCALPHA multiply by vertex alpha only. A native renderer that reuses an RGBA backbuffer must force
sampled alpha to 1 for these textures.

### 3.5 The screen-effect facade and its rasters (CONFIRMED by dump, stg0604 and stg0602 identical)

Facade `E = 0x8057798C` (accessor `fn_800A35D8`). Sub-objects and the textures they share:

| object | address (heap) | raster(s) | size / format | producers (copy sites) | consumers |
|---|---|---|---|---|---|
| E+0x34 | 0x809ED560 sub of 0x809ED600, pixels 0x80C1EE60 | "screen A" | 640x480 RGBA8 | PostGlare capture 0x80430B58; SaveScreen copy1 0x8042E8C0; FS shimmer 0x80436EEC; ScreenColor 0x80439124 | ladder capture/composite; SaveScreen stomp; shimmer mesh; ScreenColor multiply |
| E+0x38 | 0x809ED6A0 sub of 0x809ED740, pixels 0x80D4AEA0 | "screen B" | 640x480 RGBA8 | PreGlare 0x80430FAC | PostGlare scene add-back |
| G = E+0x3C | 0x809E2F98 (260 B) | G+0x08 = E+0x34, G+0x0C = E+0x38, G+0x10 = 0x809ED7E0, G+0x18 = 0x809ED880 | 128x128, 64x64 RGBA8 (pixels 0x809F8380, 0x80A083C0) | ladder sites 0x8042F41C, 0x804305A8, 0x80430810, 0x8042F630/0x8042F7E8/0x8042F9C4, 0x8042FF40 | ladder |
| B = E+0x40 | 0x809E30A8 (316 B) | B+0x74 = 0x809ED920 sub of 0x809ED9C0 | 320x240 RGBA8 | SaveScreen copy2 0x8042E968 | DrawBlur |
| C = E+0x44 | 0x809E31F0 (120 B) | C+0x10 = E+0x34 | - | - | - |
| S = E+0x48 | 0x809E3308 (212 B) | S+0x04 = E+0x34 | - | - | - |
| E+0x4C | **NULL** in both stages | - | - | atomic shimmer 0x80438370/0x80438460/0x80438B18 never reached | - |
| LF = E+0x50 | 0x809E3270 (144 B) | textures from LENSFLARE.TXD | - | - | - |

G+0x14 / G+0x1C (offset 128/64 sub-rasters) are NULL in 1P; they exist only for a split-screen
viewport whose origin is not (0,0) (4.6).

**Native**: E+0x34 is one scratch texture reused by four independent passes in the same frame; there
is no cross-pass dependency on its content except "the pass that wrote it last". Give each pass its
own target.

### 3.6 Level state payload `fn_80046D80(ctx, flags, world)` (CONFIRMED, recomp_022.cpp:4224)

Exactly one blend arm is taken, in this priority:

| bit | SRC/DST | ZTEST | ZWRITE | alpha compare (GXSetAlphaCompare) | Z compare location |
|---|---|---|---|---|---|
| 0x40 trans | SRCALPHA / INVSRCALPHA | 1 | 0 | ALWAYS (off) | before texture |
| 0x80 add | SRCALPHA / ONE | 1 | 0 | off | before texture |
| 0x100 invert | INVDESTCOLOR / ZERO | unchanged | unchanged | off | before texture |
| 0x800 punch | SRCALPHA / INVSRCALPHA | 1 | 1 | GREATER 200 | after texture |
| 0x400 sprite | SRCALPHA / INVSRCALPHA | 0 | 0 | off | before texture |
| none of the above (e.g. 0x20) | SRCALPHA / INVSRCALPHA | 1 | 1 | GREATER 1 | after texture |

Modifiers: `0x08` -> TEXTUREFILTER = LINEAR else 0 (raster default); `0x10` -> CULLMODE = NONE else
BACK; `0x04` -> FOGENABLE = `ctx+0x1809` (stage fog) else off; `0x01` -> world lights (only when a
world is passed). ZTEST 1 maps to GX_LEQUAL (measured zfunc 3).

Levels: Opeq 0x20, Punch 0x800, Trans 0x40, Add 0x80, Glare 0x80, Sprite3D 0x20, Sprite/OnFade/Gindows
0x418 (sprite arm + cull none + LINEAR). Effect elements then override per draw through the same
`fpRenderStateSet`.

---

## 4. Pass-by-pass specification (frame order)

Row numbers refer to the CSV.

### 4.1 RenderStart - frame clear (CSV 1) CONFIRMED

`0x801D032C` (recomp_022.cpp:41277): `fn_80378A08`; `fn_804040AC` when `[0x805EF150] != 0` (purpose
not traced);
`RwCameraClear(mainCamera, &V[0], 3)` with `V = 0x8057E760` (viewport manager; `V+0x00` RGBA = clear
colour; measured `(0,0,0,0xE8)` in both stages; the alpha is lost in RGB8); profiling timer start.

* Writes: EFB colour rect (0,0,640,480) = clear RGB; Z = far. EFB rows 480..527 are never drawn.
* Native: clear colour RT to `V+0x00.rgb`, depth to 1.0. The clear colour is the only "sky" when no
  sky geometry is drawn (doc 14 negative result stands).

### 4.2 World and object levels (CSV 4, 6-14) CONFIRMED

Standard forward passes, each bracketed by `CameraRender`/`CameraRenderClose` or by the land task.
Viewport and scissor = camera frame buffer. Order inside the Div group:
OpeqWorld -> Opeq -> PunchWorld -> Punch -> TransWorld -> Trans -> AddWorld -> Add -> GlareWorld ->
Glare -> PostGlare -> Sprite3D -> Last. Effect buckets: all seven wrappers call the single walker
`fn_804261C0(E, bucket)` (recomp_026.cpp:131488) over `E+0x14`, filter `elem+0x30 == bucket`, call
`[elemVt+0x10]` (with a re-entrancy bit 0x8 in `elem+0x18`). Measured 7 walker calls per frame;
element draws through the vertex-stream pipeline callback `0x80449D54`: 16 (FH) / 25.8 (Lava) per
frame. Transparent sorting is not done by the render graph: order = list order.

`TFogSet` (`0x8016A904`) runs after the Opeq effects each frame, so fog programmed there applies to
PunchWorld onward in this frame and to OpeqWorld next frame (CODE for the consequence).

### 4.3 Full-screen heat shimmer (CSV 5) CONFIRMED

Task `FullScreenShimmerDraw` exec `0x801D25F0` -> `fn_8043803C` -> `fn_804369CC`
(recomp_063.cpp:128102; fragment `0x80436A64` recomp_000.cpp:108983). **Parent: the
`LandDispManager_N` task** (`fn_801D2204` is passed `[0x8051E328+0x14]`, the task pointer stored by
`fn_8004625C`; runtime tree confirms). It therefore executes right after the "N" landscape class and
**before** every other world class, object and effect.

Gates: `S+0x0C` enable, `S+0xA4/+0xA8/+0xAC/+0xB0` buffers, `S+0x04` raster. Then:

1. Re-point `S+0x04` (= E+0x34) at the camera viewport rect; build UV rect with +0.5 texel.
2. If raster size changed (`S+0xCC/+0xD0` cache) rebuild grid `fn_804374F0` and mesh
   `fn_80437178(S, 0)`.
3. Fill vertex colours RGB 255, A = `S+0x0D`; UVs from the undisplaced grid position
   (`u = u0 + du*x/W`, `v = v0 + dv*y/H`), serpentine strip rows.
4. Save states 10,11,9,12,6,8,14,20.
5. **Copy** EFB (viewport rect) -> E+0x34 parent (640x480 RGBA8), copy site `0x80436EEC`.
6. Cam begin; TEXTURERASTER = E+0x34 parent; SRCALPHA/INVSRCALPHA; ZTEST 0; ZWRITE 0; FOG 0;
   **NEAREST**; CULL NONE; `Im2DRenderPrimitive(TRISTRIP, [S+0xA4], [S+0xB4])`; end; restore.

Mesh animation (`fn_80437178`, recomp_001.cpp:117543; called with dt by `FullScreenShimmerDrawSub`
`0x801D2564` on scheduler Layer16, i.e. game-side animation): phases `S+0xBC += dt` (wrapped by
`S+0x1C`) and `S+0xC0 += dt` (wrapped by `S+0x34`);
`rowOff[r] = S+0x30 * sin(2*pi*r*(H/rows)/S+0x38 + 2*pi*phaseA/S+0x1C)` (rows = `S+0x2C`),
`colOff[c] = S+0x18 * sin(2*pi*c*(W/cols)/S+0x20 + 2*pi*phaseB/S+0x34)` (cols = `S+0x14`);
vertex position = grid point + gridWeight * (rowOff, colOff), gridWeight = grid entry +8 from
`fn_804374F0` (CODE; weights keep the border fixed - INFERRED). Positions move, UVs do not.

Measured, Lava Shelter: cols = rows = 16, 289 grid points, **544 strip vertices**, vertex alpha 255,
params `(1.3, 1.6, 180.0, 0, 1.0)` and `(1.5, 1.5, 160.0, 0, 1.0)`; one copy + one strip per frame
(effect-geometry: 256+256+32 host-split vertices, NEAREST, blend 4/5, screen 0.5..640.5 x 0.5..480.5,
v range 0.001..1.001). Final Haunt: `S+0x0C = 0`, gated.

Intended effect: refractive heat haze that bends **only the landscape layer drawn before it** (land
N); characters, objects, effects and later land classes stay sharp on top. Native: after land N,
resolve the colour target to a texture and redraw the full viewport with a UV displacement (either
the same 16x16 displaced mesh or a pixel shader evaluating the two sine families), alpha-blended by
`S+0x0D`. The half-resolution-free NEAREST sampling is a GX choice; LINEAR at native resolution keeps
the intent.

### 4.4 GlareWorld - PreGlare (CSV 15) CONFIRMED

`0x801D266C` -> `fn_80431968` -> `fn_80430D64` (recomp_034.cpp:137335). Gate: `G+0x08 != 0 &&
G+0xE0 && (G+0xE1 || G+0xE2)`.

1. `G+0xDC` = camera (EffectSystemRW3 vfunc +0x24), `G+0x00` = its frame buffer, `G+0x04` =
   `cam+0x64`.
2. `RwRasterSubRaster(G+0x08, parent, viewportRect)` and the same for `G+0x0C`: both 640x480
   camera textures now alias the viewport.
3. UV rows (u at `G+0xA8+8*row`, v at `G+0xC0+8*row`): row 0 = `(off+0.5)/parent .. (off+size+0.5)
   /parent` (half texel, measured 0.00078..1.00078 x 0.00104..1.00104); rows 1, 2 = 0..1.
4. **Copy** EFB viewport -> `G+0x0C` parent = E+0x38 (640x480 RGBA8), site `0x80430FAC`: the
   finished opaque/punch/trans/add scene.
5. Split screen only (viewport origin != 0): (re)create `G+0x2C` parent `(x+128)x(y+128)` with
   `G+0x14` = 128x128 sub at (x,y), and `G+0x34`/`G+0x1C` for 64; rows 1/2 become
   `(x+0.5)/(x+128) .. 1`; `G+0x3C = G+0x40 = 1`. 1P: `G+0x3C = G+0x40 = 0`.
6. `RwCameraClear(cam, (0,0,0,0), IMAGE)`: **colour only, Z kept**.
7. If the camera is attached to a world, `RpWorldRemoveCamera`.

Per frame (FH): 1 copy, 1 clear. Everything drawn next into GlareWorld/Glare lands on black and is
depth-tested against the untouched scene Z: this is the glare source image.

### 4.5 Glare sources (CSV 16-20) CONFIRMED

In child order: `DrawGlobalLensFlare` (pass 1), `EffectDrawGlare` (bucket 5; wrapper `0x80027640`
brackets `RwCameraBeginUpdate/EndUpdate` itself because GlareWorld has no payload),
`LandDispManager_G` and `_GL` (additive, Z test, no Z write), the second lens-flare task (pass 0),
then the Glare level's `CameraRender` (0x80 additive) plus any stage gimmick attached to rs+0x28
(static attachers: `ElecSearchLight 0x80155B10`, `fn_802659D4`, `fn_802667A8`; none alive in
stg0602).

**Lens flare** `fn_80431E38(LF, pass)` (recomp_050.cpp:128854). Gate `LF+0x04`; return if both
hidden flags `LF+0x8C` and `LF+0x8D` are 1. Saves 7 states; cam begin; SRCALPHA/**ONE**;
VERTEXALPHA; ZWRITE 0; FOG 0; FILTER NEAREST.
* Element 0 (sun sprite) when `LF+0x8C == 0` and `table[0].pass(+0x20) != pass`:
  **ZTEST 1**, texture `[LF+0x1C][0]`, quad `[LF+0x28]`.
* Elements 1..`LF+0x24`-1 when `LF+0x8D == 0` and `elem.pass == pass`: ZTEST 0, texture
  `[LF+0x1C][i]`, quad `[LF+0x28] + 0x60*i` (element table stride 0x24 at `[LF+0x18]`).
* Second bracket: when `LF+0x8D == 0` and pass 0: untextured quad at `LF+0x2C`, ONE/ONE (screen wash).
Update `fn_80432360` (once per frame from the pass-1 task) sets `8C = 8D = 1`, projects the authored
sun direction, clears `8D` when the projected sun lies inside the flare-chain bound and clears `8C`
when it lies inside `[-0.5, 1.5] x` the screen half-extents on both axes (constants
`0x8051C540 = 0.5`, `0x8051C54C = 1.5`, tests at `0x804324D8..0x80432534` and
`0x804329E4..0x80432A3C`).
Both stages: `LF+0x04 = 0`, 2 draw calls per frame that return immediately.
Intent: sun disc occluded by scene depth (hardware Z test, no readback), flare ghosts along the
sun-centre axis, additive wash - all of it drawn into the glare source, so it blooms.

### 4.6 PostGlare - glare ladder and composite (CSV 22-33) CONFIRMED

`0x801D26E8` -> `fn_8043193C` -> `fn_80430904` (recomp_032.cpp:138021), ladder
`fn_8042F258` (recomp_016.cpp:130024), blur half pair `fn_804302EC` (recomp_028.cpp:136077), quad
builder `fn_80430850` (recomp_031.cpp:134531). Gates as PreGlare.

**Parameters** (G+0xE0..0xFC, written only by `fn_80431494`; values live in both stages):

| field | meaning | FH / Lava |
|---|---|---|
| E0 | enable | 1 |
| E1 | level A (128) enable | 1 |
| E2 | level B (64) enable | 1 |
| E3 | 1 = composite A and B separately; 0 = merge B into A | 0 |
| E4 | 1 = level B samples the full-res source even when A is on | 0 |
| E5 | level A capture intensity byte | 0x20 |
| E6 | level B capture intensity byte | 0x30 |
| E8 / EC | spread A / B (level texels) | 1.0 / 1.0 |
| F0 / F4 | extra H / V taps level A | 2 / 2 |
| F8 / FC | extra H / V taps level B | 4 / 4 |

**Quad builder** `BuildQuad(G, x0, y0, w, h, u0, v0, u1, v1)` writes 4 `RwIm2DVertex` at G+0x48 (tri
strip TL, BL, TR, BR). It **shifts the image by (x0, y0) and clips to [0,w] x [0,h]**, trimming the
UV range proportionally (`x0 >= 0`: x in [x0, w], `u1 -= du*x0/w`; `x0 < 0`: x in [0, w+x0],
`u0 -= du*x0/w`), so a tap outside the rectangle contributes **nothing** (zero border, not clamp).
Vertex colour bytes `G+0x54..` (RGBA) are set by the ladder; z stays 0.

**Sequence** (1P, E3 = 0, E1 = E2 = 1, E4 = 0 - the measured configuration):

| step | draws (Im2D) | state | source -> destination |
|---|---|---|---|
| P0 | save 8 states; set ZTEST 0, ZWRITE 0, TEXTUREADDRESS CLAMP, SHADEMODE GOURAUD, FILTER LINEAR (all later steps inherit); `cam->zBuffer = NULL` | - | - |
| P1 | copy | - | EFB viewport (glare source) -> E+0x34 parent, site 0x80430B58 |
| L1 A capture | 1 quad, x0=y0=-0.5, 128x128, UV row 0 | ONE/ZERO, colour RGB = trunc(E5 x 0.95) = 30, A 255 | E+0x34 -> EFB corner -> copy (0,0,128,128) -> G+0x10 |
| L2 A horizontal | clear (0,0,0,255) full viewport; 1+F0 = 3 taps | ONE/ONE, colour white | G+0x10 -> corner -> copy -> G+0x10 |
| L3 A vertical | clear; 1+F4 = 3 taps | ONE/ONE | G+0x10 -> G+0x10 |
| L4 B capture (cascade arm) | 1 quad 64x64, UV row 1 | ONE/ZERO, colour = E6 raw = 48 | G+0x10 -> copy (0,0,64,64) -> G+0x18 |
| L5 B horizontal | clear; 1+F8 = 5 taps | ONE/ONE | G+0x18 -> G+0x18 |
| L6 B vertical | clear; 1+FC = 5 taps | ONE/ONE | G+0x18 -> G+0x18 |
| L7 wipe | clear (0,0,0,0) colour only (site 0x8042FA30) | - | - |
| L8 merge | A at 128x128 (row 1) + B upscaled into 128x128 (row 2) | ONE/ONE LINEAR | copy (0,0,128,128) -> G+0x10; wipe (0,0,0,0) (0x8042FF50) |
| L9 | A full viewport (x0=y0=0, size 640x480, row 1) | ONE/ONE LINEAR | G+0x10 -> EFB |
| L10 | glare source full viewport (row 0) | ONE/ONE LINEAR | E+0x34 -> EFB |
| P2 | saved scene full viewport (row 0); restore 8 states | ONE/ONE LINEAR | E+0x38 -> EFB |

**Tap kernel** (one blur half, `count = F0/F4/F8/FC`): draw the source once at offset
`-0.5`, then for `i = 0..count-1` at offset `((i/2)+1) * s_i * spread - 0.5` along the half's axis
(fixed axis -0.5), with `s_i` starting +1 and alternating. With the Im2D +0.5 translation the net
integer offsets are `{0, +1, -1, +2, -2, ...} x spread` destination-level texels, each tap sampled
exactly at a texel centre (so LINEAR acts as point sampling when spread is integral). Each tap
**adds the full texture value** (unnormalised sum, 8-bit saturation). Horizontal and vertical halves
are the same instructions with the axis swapped; `count = 0` skips that half including its clear and
copy.

Other configurations (CODE): E4 = 1 or E1 = 0 -> level B captures from E+0x34 with row 0 and
colour trunc(E6 x 0.95) (copy sites 0x8042F630 / 0x8042F9C4 instead of 0x8042F7E8). E3 = 1 -> no
merge; composite A (row 1) then B (row 2) full viewport. E3 = 0 with only E2 -> composite B full
viewport. The unblurred source (L10) and the scene (P2) are always added.

Measured on Final Haunt and Lava Shelter (effect-geometry, frames 5500-5502, identical every frame):
23 BuildQuad draws, 6 ladder clears, 8 copies (the P1 source copy plus the 7 ladder copies L1, L2,
L3, L4, L5, L6, L8), tap footprints exactly as above (128 level x ranges [0.5,128], [1,128.5], [0.5,127];
64 level adds [2,64.5] and [0.5,62]), composites over 0.5..640.5 x 0.5..480.5 with UVs 0..1 (A) and
0.0008..1.0008 (source and scene).

**Resulting image** (all 8-bit saturating):

```
lvlA   = Hv( Hh( round(E5*0.95)/255 * S640->128(src) ) )          separable sums, 1+F0 / 1+F4 taps
lvlB   = Hv( Hh( E6/255 * S128->64(lvlA) ) )                      cascade arm
lvlA'  = lvlA + Up64->128(lvlB)                                   bilinear, UV 0..1
final  = scene + Up128->viewport(lvlA') + src                     src = glare sources on black
```

`S640->128` is one bilinear tap per destination texel at UV row 0 (a sparse 5:1 x 3.75:1
decimation), `Up` is one bilinear tap per destination pixel with UV 0..1 over the quad.

Intended effect: an additive bloom of everything flagged "glare" (bucket-5 emitters, G/GL landscape,
glare-level gimmicks, lens flares), occluded by scene depth, at two blur radii, with the sources
themselves also added on top of the scene. Resolution-independent re-implementation:
* render glare sources into their own RT (cleared to black, scene depth attached, depth write off)
  instead of PreGlare copy + clear + P1 copy + P2 add-back (P2 is only there to undo the clear);
* level A target = viewport / (5, 3.75) (128x128 at 640x480; anisotropic on purpose because the GC
  used square power-of-two textures), level B = half of A;
* keep the unnormalised tap sums and the byte intensities (E5 x 0.95 truncated, E6 raw in the cascade
  arm, E6 x 0.95 otherwise) - the authored bytes compensate for the gain;
* keep the tap spacing in level texels (`spread`), zero contribution outside the level rectangle,
  and saturate at 1.0 after each additive draw (min(1, a+b+c) is order independent, so the final
  add can be done in any order but must clamp);
* the corner-of-EFB work area, the opaque-black pre-clears, the two transparent wipes and every
  copy exist only because Flipper has one render target.
At internal scales above 1 a host that keeps the guest geometry must sample one guest texel per tap
(see GekkoForge `--no-effect-footprint-filter` notes); a native renderer with real level RTs does
not have that problem.

### 4.7 DrawBlur - temporal + zoom composite (CSV 34) CONFIRMED (code), runtime values from earlier sessions

`0x801D27FC` -> `fn_8042F16C` -> `fn_8042E254` (recomp_000.cpp:107294). Gates `B+0x00` enable,
`B+0x01` alpha, `B+0x78` history-valid.

* Save states 10,11,12,6,8,14,9. Frame buffer `fb = cam+0x60`, parent `PW x PH` (640x480).
* UV: `u0=(offX+0.5)/PW, u1=(offX+fb.w+0.5)/PW, v0=(offY+0.5)/PH, v1=(offY+fb.h+0.5)/PH`
  (normalised against the **full-resolution** parent, applied to the 320x240 history -> a quarter
  history texel bias).
* Vertices at `B+0x14` (stride 0x18): RGB 255, A = `B+0x01`; with `k = B+0x04 - 1`:
  x in `[-k*w/2, w + k*w/2]`, y in `[-k*h/2, h + k*h/2]` (zoom about the viewport centre; neutral 1.0).
* Cam begin; TEXTURERASTER = `B+0x74` parent (320x240 RGBA8); SRCALPHA/INVSRCALPHA; ZTEST 0;
  ZWRITE 0; FOG 0; **NEAREST**; TRISTRIP 4; restore; end.

Runtime: gated in both headless runs (`B+0x00 = 0`, `B+0x01 = 0x80`, zoom 1.0, `B+0x78 = 0`).
Active-case geometry measured in the motion-blur investigation (Final Haunt spring,
`GekkoForge/docs/motion-blur-2026-09-14/draw-path.md` section 1, `MOTION_BLUR_ROOT_CAUSE_AND_FIX_PLAN.md`):
composite corners -15.5..656.5 at zoom `1+16/320`, UVs 0.00078..1.00078 / 0.00104..1.00104,
NEAREST, SRCALPHA/INVSRCALPHA, depth off.

Intended effect: a feedback motion trail. Each frame the previous finished frame (captured by
SaveScreen after this composite, so trails accumulate geometrically with factor alpha/255) is blended
over the current frame, optionally scaled outward from the centre (zoom > 1 -> radial streaks during
speed bursts / Chaos Control). Native: full-resolution LINEAR history RT, same alpha and zoom maths;
320x240 NEAREST is a GameCube memory/fill-rate compromise and visibly blocky.

### 4.8 EffectDrawShimmer (CSV 35) CONFIRMED dormant

Bucket 6 walker. The only known bucket-6 draw virtual, `Effect::RW3::EffectShimmerRW3::Draw`
`0x80028BA4`, calls `fn_80438C64`, which returns immediately when `E+0x4C == 0`. E+0x4C is NULL in
both stage dumps and `0x80028BA4` / `0x80438168` had 0 hits. The per-element refraction path (EFB copy
+ `RpAtomicRender` of an effect mesh with the screen texture, copy sites 0x80438370/0x80438460/
0x80438B18) is not exercised in retail play on these stages; no writer of E+0x4C is known.

### 4.9 Sprite3D and Last (CSV 36-37) CONFIRMED

`CameraRender(0x20)`: Z test **and write** on. The scene Z buffer is still intact here (PreGlare and
the ladder cleared colour only, every screen pass ran with Z test/write off), so world-space sprites
are correctly occluded. `Last` only closes the bracket and leaves ZTEST 1 / ZWRITE 1 / LEQUAL latched.

### 4.10 RenderAttach_Static (CSV 38) CONFIRMED

`0x801E4E40`: `V+0x6C = V+0x70 = 0`, `fn_8027211C(camMgr, 0)` (layout 0 = full 640x480),
`fn_80271F9C(camMgr, V+0x64[0])`. Everything below draws once per frame over the full screen.

### 4.11 PostEffect - SaveScreen (CSV 39) CONFIRMED (code), active case measured earlier

`0x801D2764` -> `fn_8042F140` -> `fn_8042E838` (recomp_002.cpp:130559) + fragment `0x8042E8A4`
(recomp_003.cpp:142894).

1. `B+0x78 = 0` **unconditionally** (so any frame with the blur off invalidates the history).
2. Gates `B+0x00 && B+0x01` and the camera.
3. **Copy** EFB camera fb (0,0,640,480) -> `[E+0x34]` parent 640x480 RGBA8 (site `0x8042E8C0`).
4. Cam begin; TEXTURERASTER = that parent; SRC ONE, DST ZERO; `Im2D TRISTRIP B+0x7C`:
   **stomp quad** x 0..320, y 0..240, UV 0.00078..1.00078 x 0.00104..1.00104 (dump), z 0; end.
   Z test/write and texture filter are **not set** - inherited from Sprite3D (LEQUAL + write;
   measured NEAREST).
5. **Copy** (0,0,320,240) -> `B+0x74` parent 320x240 RGBA8 (site `0x8042E968`).
6. Cam begin; `Im2D TRISTRIP B+0xDC`: **restore quad** x 0..640, y 0..480 same UVs, ONE/ZERO; end.
   SRC/DST/TEXTURERASTER are not restored afterwards.
7. `B+0x78 = 1`.

Side effect when active: depth 0 is written over the whole screen (both quads pass LEQUAL at z 0).
Nothing after it depth-tests in normal play (HUD levels are ZTEST 0).

Intended effect: capture "the finished 3D frame without HUD" as next frame's blur history.
Native: copy/downsample the colour RT into the history RT (no stomp, no restore, no depth write).
The stomp/restore pair exists only to produce a half-size copy with a single render target.

### 4.12 PostEffect - DrawScreenColor (CSV 40) CONFIRMED (gating) / CODE (modes)

`0x801D2878` -> `fn_80439998` -> `fn_80438C90` (recomp_028.cpp:137695) + fragment `0x80438D00`.
Gate `(C+0x00 || C+0x01) && C+0x10`. Block (`fn_804396D4` copies 11 bytes):
`+0x00 u8 tint enable, +0x01 u8 invert enable, +0x02 u8 (not read here), +0x03..0x06 RGBA multiply
colour, +0x07..0x0A RGBA overlay colour`. Quad `C+0x18` = full viewport (0..w, 0..h), UVs of the
viewport rect + 0.5 texel. States saved 10,11,12,6,8,14,9 and restored.

* `C+0x00` and multiply RGB != (255,255,255): **copy** EFB -> E+0x34 parent (site `0x80439124`);
  vertex colour = multiply RGBA; TEXTURERASTER = copy; ONE/ZERO; ZTEST 0 ZWRITE 0 FOG 0 NEAREST.
  Result = screen x colour (TEV modulate).
* `C+0x00` and overlay alpha `C+0x0A != 0`: vertex colour = overlay RGBA; no texture;
  SRCALPHA/INVSRCALPHA. Result = lerp(screen, colour, a).
* `C+0x01`: vertex colour white; no texture; **SRC INVDESTCOLOR, DST ZERO**. Result = 1 - screen.

Both stages: `C = {0,0,0, ff,ff,ff,ff, 0,0,0,0}`: gated. Order relative to HUD: after SaveScreen,
before Sprite, so flashes/tints affect the 3D image and the blur history of the next frame is captured
before the tint.
Native: multiply = blend (DEST_COLOR, ZERO) or (ZERO, SRC_COLOR) with the colour as a constant - the
copy is a GX workaround; overlay and invert are ordinary blend states.

### 4.13 2D: Sprite, OnFade, Gindows, SpriteLast (CSV 41-45) CONFIRMED (tree, counts) / CODE (camera)

* `CameraMatrix->Set2D` (`0x801D019C` with payload byte 1 -> `fn_80271E5C`, recomp_050.cpp:54257):
  layout 0, saves the camera frame LTM into `camMgr+0x38..0x64` and the view window into
  `camMgr+0x30/0x34`, sets the view window to the global at `[0x805F1200]` if different. It does
  **not** load an orthographic projection; screen-space HUD uses Im2D, which loads its own ortho per
  primitive (3.3). `Reset2D` (`fn_80271D50`) restores frame and view window.
* `CameraRender(0x418)`: SRCALPHA/INVSRCALPHA, Z test/write off, alpha test off, cull none, filter
  LINEAR; Im2D TEV modulate; viewport full screen; scissor = camera fb.
* Final Haunt HUD per frame: 58 Im2D primitives (33 glyph quads, 21 CSD quads, 4 prompt-sprite quads;
  2 `CsdBase::Draw` -> `Chao::CSD::Project::Render 0x8036E794` per frame). Layout, anchoring and
  widget identity are documented in `GekkoForge/docs/2D_LAYOUT_ARCHITECTURE.md`.
* `OnFade`: `FadeDrawTask` (`0x80044E08`) -> `Fade::Draw 0x800448C8/0x800448F8`: ten untextured
  64x480 canvas-unit tri-strips with per-vertex alpha, optional 109x135 badge (2D doc section 4.3);
  0 draws in-stage (state 0). `StageTitleTask` attaches here while loading. The fade therefore covers
  the 3D frame and the HUD, but not the stage title card (appended later) or Gindows.
* `Gindows`: `SaveMessageDrawTask` (memory-card message windows).

### 4.14 RenderStop - present (CSV 46) CONFIRMED

`0x801D0250` (recomp_019.cpp:44144) -> `fn_80051B18` -> `fn_800514A0` ->
`RwCameraShowRaster(cam, NULL, 0)` `0x80486858` -> `RwRasterShowRaster 0x8048AD94` -> std fn 0x14
`0x80495CBC` (recomp_061.cpp:138855). **The XFB copy happens here, at the end of the render subtree,
every frame** - not at the next frame's clear.

Frame (non-field) arm: wait on the VI queue (`0x805F26B0/B4`, depth `[0x805EEFE4]`) if full;
`GXFlush`; record `{xfb, fifo ptr, token}`; first frame `GXEnableBreakPt`; **`GXCopyDisp(xfb, clear 0)`**;
`GXSetDrawSync(token)`; token++; `GXFlush`; swap between the two XFBs (`0x805F26A0/A4`).

Display-copy state (set once by `0x80495214`, recomp_049.cpp:134397, from rmode at `0x805E4250`):
rmode = `EURGB60 interlaced (viTVmode 0x14)`, fb 640, efbHeight 480, xfbHeight 480, viWidth 640,
viHeight 480, viXOrigin 40, xFBmode DF, field_rendering 0, aa 0, sample pattern all 6, vfilter
`{8,8,10,12,10,8,8}`. So: `GXSetDispCopySrc(0,0,640,480)`, `GXSetDispCopyDst(640,480)`,
`GXSetCopyFilter(aa 0, pattern, vf on, vfilter)` = **vertical 1/4, 1/2, 1/4 deflicker**,
`GXSetDispCopyYScale(1.0)`, `GXSetFieldMode(0, 0)`, `GXSetPixelFmt(RGB8_Z24, ZC_LINEAR)`,
`GXSetDispCopyGamma(1.0)`. Measured 1 GXCopyDisp per frame.

Native: present the colour RT. The deflicker exists for interlaced CRT output; keep it only as an
optional look setting. The 50 Hz rmode (efbHeight/xfbHeight differ) was not captured (section 12).

---

## 5. Z buffer and alpha lifetime across the frame

| point in frame | Z buffer | colour buffer |
|---|---|---|
| RenderStart | cleared to far | clear colour |
| world/object/effect levels | tested and written per flags | scene |
| FS shimmer (after land N) | untouched (ztest/zwrite off) | land N distorted in place |
| PreGlare | **kept** | scene copied to E+0x38, then black |
| glare sources | tested, not written (additive) | glare sources on black |
| ladder / PostGlare | untouched | scratch corner, then source + bloom, then + scene |
| DrawBlur | untouched | history blended over |
| Sprite3D | tested and written (scene Z still valid) | sprites |
| SaveScreen (when active) | **overwritten with 0** (inherited LEQUAL + write, z 0) | repainted identically |
| DrawScreenColor | untouched | tinted/overlaid/inverted |
| HUD, fade, windows | untouched (ZTEST 0) | 2D |
| RenderStop | - | XFB copy with deflicker |

Destination alpha: the EFB is RGB8_Z24 all frame; `GXSetDstAlpha` has no callers; every texture
copy reads alpha 255. No pass depends on EFB alpha.

---

## 6. Per-frame measured sequence (summary; full rows in the CSV)

| pass | FH copies | FH clears | FH Im2D | Lava difference |
|---|---|---|---|---|
| RenderStart | 0 | 1 | 0 | - |
| Opaque/punch/trans/add world + objects + buckets 0-4 | 0 | 0 | 0 | more display lists (201 vs 176) and element draws (25.8 vs 16) |
| FS shimmer | 0 (gated) | 0 | 0 | +1 copy, +1 strip (544 verts) |
| PreGlare | 1 | 1 | 0 | same |
| glare sources (lens flare x2 gated, bucket 5, land G/GL) | 0 | 0 | 0 | same |
| PostGlare + ladder + composites | 8 | 6 | 23 | same (identical ladder parameters) |
| DrawBlur / EffectDrawShimmer | 0 | 0 | 0 | same (gated / dormant) |
| Sprite3D | 0 | 0 | 0 | stage objects |
| SaveScreen / DrawScreenColor | 0 / 0 | 0 | 0 | same (gated) |
| HUD / fade / windows | 0 | 0 | 58 (+8 fully clipped, not in any CSV) | 54 |
| RenderStop | XFB 1 | 0 | 0 | same |
| **total** | **9 + XFB** | **8** | **89** | **10 + XFB, 8, 86** |

Other per-frame counters (FH / Lava): GXBegin 113 / 120 (= Im2D + 8 clear quads + vertex-stream
effect draws), GXCallDisplayList 176.3 / 201.3, `0x80463964` 41 / 14, `0x8044EEC4` 2 / 2,
`0x80457C14` 9 / 6, Im3D `0x804A0B68` 0 / 0.23, GXSetViewport 129 / 124 (Im2D setup + clears + camera
begins), GXSetScissor 40 / 38 (camera begins + clears), camera begins 32 / 30, host draw batches 30.9 /
31.1, triangles 9021 / 7273, GX primitives decoded 1061 / 650, fog draws 801 / 532, lit draws 21 / 446.

Native EFB captures at XFB copy 5501 (640x528, rows 480..527 black):
`RENDERER_RE/_work/graph/runs/fhB_efb_5501.png`, `lavaB_efb_5501.png`.

---

## 7. Split screen (CONFIRMED code, not exercised)

Viewport manager `V = 0x8057E760`: `+0x00` clear colour, `+0x60` split mode, `+0x64[]` camera ids,
`+0x6C` layout index, `+0x70` viewport ordinal. `RenderAttach_Div` exec `0x801E4EF0`:

* mode 0: layout 0, camera 0; children run once (measured).
* mode 1: layout 1 + camera 0, **explicitly runs `fn_8004ECAC` on its own children**, then layout 2 +
  camera 1 and returns so the executor runs the children a second time.
* mode 2: same with layouts 3 and 4. There is no four-viewport path (doc 14 "4P" is wrong).

Layouts are selected with `fn_8027211C(camMgr, n)` (which re-points the camera frame buffer to the
layout's sub-raster; the layout geometry itself was not decoded - INFERRED top/bottom vs left/right
halves) and cameras with `fn_80271F9C`. Consequences per pass: every world level, PreGlare/ladder,
DrawBlur, shimmer and lens flare run once per viewport inside that viewport's sub-raster (ladder uses
the offset 128/64 sub-rasters when the origin is not (0,0); DrawBlur maps the full-screen history
through the viewport's UV rect); Static-group passes (SaveScreen, DrawScreenColor, HUD, fade) run
once on the full screen. Native: per-viewport scene targets (or scissored viewports) for the Div group,
one full-screen post/HUD stage.

---

## 8. GameCube-limitation passes vs visual intent

| mechanism | why it exists | native replacement | must be kept exactly |
|---|---|---|---|
| clear-by-quad | GX has no clear call; copy-clear flag unused | RT clear | clear colour `V+0x00.rgb`, depth far, camera rect |
| PreGlare scene copy + colour clear, PostGlare scene add-back | one render target | glare-source RT with shared scene depth | depth test of glare sources against the scene; final `scene + bloom + sources` saturating |
| PostGlare source copy | same | RT is already a texture | - |
| ladder corner rendering, 6 copies, opaque/transparent wipes | same | 2 small RTs per level (or ping-pong) | level sizes relative to viewport, byte intensities, unnormalised tap sums, spread, zero border, saturation, merge/composite arm logic E1..E4 |
| SaveScreen copy + stomp + copy + restore | half-size copy with one RT | history RT update | capture point (after Sprite3D, before tint and HUD), B+0x78 handshake semantics |
| 320x240 NEAREST history | memory/fill rate | full-res LINEAR history (quality) or 320x240 NEAREST (parity) | alpha `B+0x01`, zoom `B+0x04` about viewport centre |
| DrawScreenColor multiply via copy | no constant-colour multiply blend used | multiply blend | the three modes and their order |
| FS shimmer copy | framebuffer read | colour resolve after land N | pass position (child of land N), sine parameters, vertex alpha |
| sub-raster offsets (G+0x14/0x1C, fb sub-rasters) | copies land at raster offsets | per-viewport targets | viewport rects |
| XFB deflicker 1/4-1/2-1/4 | interlaced TV | none (optional filter) | - |
| RGBA6 pixel-format switch for camera-texture targets | - | none | never used in-stage |

Visual intent that must be reproduced: glare bloom (two radii, depth-occluded sources), temporal
motion trail and zoom streak, heat haze on the landscape layer, lens flare with depth-occluded sun,
screen tint / flash overlay / colour inversion, fades, HUD.

---

## 9. Cut-line implications

1. **Keep above the line (guest)**: the whole task tree, level/attach order, `LandDispManager` and
   RenderUnit callbacks, EffectManager element update and bucket membership, parameter drivers
   (`EffectGlobalParamPJS` lerp `0x80439A8C`, `PlayerEffect::Slow`, `SetGlareParams 0x80431910`,
   blur/screen-colour/shimmer/lens-flare setters), `FullScreenShimmerDrawSub` phase advance
   (`0x80437178` writes `S+0xBC/0xC0` and the vertex buffer), lens-flare update `0x80432360`
   (computes `LF+0x8C/0x8D` and vertices from the camera).
2. **Intercept options for the screen passes** (they are guest effect code written entirely in RW
   calls):
   * (a) faithful RW driver: implement Im2D (3.3), camera begin/end (3.1), RwCameraClear (3.2) and
     `RwGameCubeCameraTextureCopy` (3.4) as RT operations and let the guest drive all 9-10 copies per
     frame. Byte-comparable at 640x480; at higher resolution the ladder taps and 320x240 NEAREST
     history need footprint-aware sampling.
   * (b) replace the worker bodies `0x80430D64` (PreGlare), `0x80430904` + `0x8042F258`
     (PostGlare + ladder), `0x8042E254` (DrawBlur), `0x8042E838` (SaveScreen), `0x80438C90`
     (DrawScreenColor), `0x804369CC` (FS shimmer draw), `0x80431E38` (lens flare draw) with native
     passes that read the parameter blocks (G+0xE0..0xFC, B+0x00..0x0F, C+0x00..0x0A, S+0x0C..0x40,
     S+0xA4/0xB4, LF+0x04..0x8D) at the same tree positions. These workers write no game-visible
     state except: `B+0x78` (read only by DrawBlur), `G+0x00/0x04/0x38/0x3C/0x40/0xA8..0xDC`, the
     embedded vertex quads, `S+0xCC/0xD0`, `cam+0x64 = NULL` in PostGlare, and camera attach/detach
     from the world in PreGlare. A native pass must still leave the camera detached from the world
     the way PreGlare does if later guest code depends on it (not observed; INFERRED harmless).
3. **Present**: intercept `RwGameCubeRasterShowRaster 0x80495CBC` (std fn 0x14); the VI queue
   throttle and draw-sync token are pacing, not rendering.
4. **Ordering invariants the native frame graph must keep**: FS shimmer right after land N; PreGlare
   before lens flare / bucket 5 / land G,GL / Glare level; ladder then scene add-back then DrawBlur
   then bucket 6 then Sprite3D (with scene depth intact); SaveScreen after Sprite3D and before
   DrawScreenColor; HUD after DrawScreenColor; fade after HUD; title card and Gindows after fade.
5. **Can be dropped**: every GXCopyTex listed in 3.5 as a mechanism (not as a semantic), the ladder
   wipes, SaveScreen stomp/restore and its depth write, XFB deflicker, RGBA6 path, the unused
   `halve` copy arm, EFB rows 480-527.

---

## 10. Final Haunt specifics

stg0604 in-stage (Shadow idle, frames ~5500): glare ladder active with E0..E6 = 1,1,1,0,0,0x20,0x30,
spread 1/1, taps 2/2/4/4; lens flare, blur, SaveScreen, screen colour, FS shimmer all gated off;
bucket-6 refraction dormant; 1P. The speed-burst blur (springs, dash pads) was not triggered by the
headless route; its measured geometry is quoted in 4.7 from the motion-blur investigation.
Lava Shelter adds the heat shimmer (4.3) with otherwise identical post-processing parameters.

---

## 11. Corrections to existing documents

1. **doc 14 section 3 / 7, "EFB clear is realized by the next copy" and "(RW device) camera clear
   next frame"**: the frame clear is `RenderStart` (PreRender) calling `RwCameraClear(cam, V+0x00,
   IMAGE|Z)` at the **start** of each frame, implemented as a depth-ALWAYS quad; GX copies never clear
   (`0x805F2688 = 0`). The XFB copy is issued by `RenderStop` in PostRender through
   `RwCameraShowRaster`, not by a later device call.
2. **doc 14 "RenderLevel_Glare has no children - reserved"**: it has a live `CameraRender` (0x80) and
   static attachers read rs+0x28 (`ElecSearchLight 0x80155B10`, `fn_802659D4`, `fn_802667A8`). Anything
   drawn there becomes a glare source.
3. **doc 14 section 3 world levels**: concrete contents are 15 `LandDispManager_*` classes from table
   `0x8051E328` (section 2.3); `RenderLevel_PunchWorld` holds no landscape - punch landscape (`_P`,
   flags 0x80C) is drawn in OpeqWorld.
4. **doc 14 / doc 10 "FullScreenShimmerDraw: caller-supplied parent"**: the parent is the
   `LandDispManager_N` task; the shimmer captures and distorts only land N (runtime tree + code).
5. **doc 14 "CameraMatrix(1) payload = enter 2D ORTHO"**: `Set2D` saves the camera frame and view
   window and sets a fixed view window; it loads no ortho projection (Im2D does, per primitive).
6. **doc 14 section 5 "split mode 2 = 4P"**: mode 2 is a second two-viewport arrangement (layouts 3/4).
7. **doc 14 section 4 flag table ("0x20 opaque-ish", "0x800 alpha-test block", "0x418 ztest bits")**:
   exact decode in 3.6 (default arm = alpha test GREATER 1 with Z write; punch = GREATER 200; 0x418 =
   no Z, cull none, LINEAR; plus 0x100 invert arm and the 0x01/0x04/0x08/0x10 modifiers).
8. **doc 10 section 7 "NO z-buffer read ... sun-behind-building can only come from the stage"**: the
   sun sprite is drawn with `ZTESTENABLE = 1` into the glare source while the scene Z is intact, so
   the hardware depth test occludes it (no readback). `LF+0x8C/0x8D` are *hidden* flags set to 1 then
   cleared when the sun projects inside the respective band; the draw skips only when both are 1.
9. **doc 10 section 6(b) / section 12 "E+0x4C non-null at runtime"**: E+0x4C is NULL in stg0604 and
   stg0602 dumps; the atomic-shimmer draw returns immediately (`0x80438C64`), 0 hits.
10. **doc 10 section 8 DrawScreenColor "capture-tint mode (multiplicative/filter variant)"**: three
    exact modes (multiply via capture + ONE/ZERO, alpha overlay, invert via INVDESTCOLOR/ZERO) with
    byte layout in 4.12.
11. **doc 10 section 1 "two full-screen camera-texture rasters"**: they are shared - `G+0x08 = S+0x04 =
    C+0x10 = E+0x34`, `G+0x0C = E+0x38`, and SaveScreen copy1 targets the parent of E+0x34 - one
    640x480 scratch texture (MEM1 0x80C1EE60) is overwritten by up to four passes per frame.
12. **doc 10 section 4.2 "7 copies per active frame"**: correct for the ladder proper; the PostGlare
    source capture adds one more and PreGlare another, 9 per frame on Final Haunt. Tap draws are
    **unnormalised additive sums** (each tap ONE/ONE at full weight), and captures are modulated by
    trunc(E5 x 0.95) / E6 (raw in the cascade arm) - neither is stated in doc 10.
13. **doc 10 section 6(a) "152-vertex mesh"**: the strip length is authored (`S+0xB4`); Lava Shelter
    uses a 16x16 grid = 544 vertices.
14. **doc 00 section 4 / doc 14 section 1 "fn_801EB624 one pump per frame"**: a watch on
    `0x801EB624` fired once in a 44e9 run while all render execs fired per frame; the per-frame caller
    of the task executor needs re-checking (open question 1).

---

## 12. Open questions

1. Which entry pumps the task tree per frame (`0x801EB624` watch fired once)?
2. Layout rectangles 1-4 behind `fn_8027211C` / `fn_80272FF0` (split screen) - not decoded.
3. PAL 50 Hz rmode (efbHeight/xfbHeight, YScale, field mode) - only the 60 Hz preselect was run.
4. Grid weights produced by `fn_804374F0` (shimmer border behaviour) - not decoded.
5. Value of the fixed 2D view window `[0x805F1200]` (BSS, not dumped) and which HUD elements are 3D.
6. Writer of `E+0x4C`; whether any stage enables bucket-6 refraction.
7. Blur/SaveScreen/ScreenColor/lens flare were gated in the headless idle route; an in-stage trigger
   without pad input (e.g. a stage whose GlobalParam element enables them at start) would allow
   counting their copies in this lane's format.
8. Semantics of `C+0x02` and `B+0x08/0x0C` (written, not read by the draw code).

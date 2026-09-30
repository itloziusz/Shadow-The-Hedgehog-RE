# 04 — Effect, im-prim, particle, decal, lens-flare and sprite draw paths

Shadow the Hedgehog, GameCube PAL (GUPP8P). Lane: **effects** — every draw path that does *not*
go through the world/atomic native pipelines. Companion data: `04_effect_draw_paths.csv`
(one row per path). Scratch tools, raw runs and snapshots: `RENDERER_RE/_work/effects/`.

Confidence tags: **CONFIRMED** = read in code AND observed at runtime (hook counts or a live
memory snapshot); **CODE** = read in code (disassembly / generated C++) only; **INFERRED** =
reasoned from surrounding evidence, not read instruction by instruction.

Generated-code citations are `recomp_NNN.cpp:line` of the **current** corpus
(`GekkoForge/build-native/generated`, header comment line `// 0xADDR`). The line numbers in
`RECOMP_VISUAL_MAP/db/*.csv` and in the older RECOMP_VISUAL_MAP documents are stale (the corpus was
regenerated after 2026-09-01) — use the ones in this document.

---

## 0. Method and measurement window

* Static: whole-DOL capstone census of (a) every `bl` into the GX SDK range, (b) every `bl` to RW
  im-prim/PTank/clump entries, (c) every indirect RwDevice call `[[0x805F265C]+off]` with the
  constant state id/value in r3/r4, (d) every load of the GX FIFO address 0xCC008000, (e) every
  render-level attachment (`bl 0x800461A0` → `lwz rX, OFF(r3)`) with its bound draw functor
  descriptor `{0, 0xFFFFFFFF, fn}`. Tools: `_work/effects/scan.py`, `levels.py`, `dol.py`,
  outputs `scan_gx_calls.csv`, `scan_device_calls.csv`, `scan_watch_calls.csv`.
* Runtime: four headless runs of a private exe copy, `--boot-stage stg0604` (Final Haunt),
  budgets 45e9 / 50e9 / 55e9 / 55e9, 70–98 `--watch` hooks, `--save-state` at the end of three
  of them. The runs are deterministic (B55 and D55 have identical counts for every shared hook).
  **Per-frame numbers below are deltas over the in-stage window 50e9→55e9 = 618 presented frames**
  (cross-checked against 45e9→55e9 = 1,234 frames: identical rates). Frame = one call of
  `RwGameCubeRasterShowRaster` 0x80495CBC.
* Live structure reads: MEM1 was extracted from the savestates (`_work/effects/mem.py`,
  `snap.py`) at 45e9, 50e9 and 55e9 and the effect manager's draw list was walked.
* Hook caveat found during this work: the runtime itself installs HLE probes at **0x80434824**
  (EffectParticlePJS::Render) and **0x80449458** (RpPTankAtomicLock)
  (`runtime/src/gekko/system.cpp` "Effect particle draw (rotation-range probe)"); a `--watch` on
  either address is silently replaced and reports 0. Particle draws were therefore counted at
  0x80449010 / 0x80449A74 / 0x8044C260, which are 1:1 with a drawn particle element.

---

## 1. Executive summary

1. **Particles are RenderWare RpPTank, not a custom stream system.** The "5 double-buffered
   vertex streams" of RECOMP_VISUAL_MAP doc 08 are the RpPTank plugin (plugin id 0x12F =
   `MAKECHUNKID(rwVENDORID_CRITERIONTK, 0x2F)` = rwID_PTANKPLUGIN). `0x805F24F0` is
   `_rpPTankAtomicDataOffset`, `0x805F24F4` the engine-globals offset, `fn_80448690` is
   `RpPTankPluginAttach`, `fn_80449458/fn_804495E8` are `RpPTankAtomicLock/Unlock`,
   `fn_804490C4` is `RpPTankAtomicCreate`, and `fn_80449D54` is the GameCube PTank pipeline
   render callback. The doc-20 negative "no RW PTank" is refuted. (CONFIRMED)
2. Billboard expansion is **CPU-side inside RW**: ten per-format emitters
   (0x8044A25C … 0x8044C6A4) expand each particle into 4 vertices written **straight into the
   GX FIFO** with `psq_st` after one `GXBegin(GX_QUADS, GX_VTXFMT0, 4·n)`. The camera right/up
   axes come from the current RwCamera LTM (computed in 0x80449A74), not from globals
   0x805FC0F0.. (those are SDA2 constants). (CONFIRMED)
3. **On Final Haunt every visible effect is a particle.** All 16 live effect elements are
   `EffectParticlePJS`, all **re-bucketed into bucket 5 (Glare)**, all use emitter 0x8044C260
   (2D-rotate + size + per-particle color + TL/BR UV), 532–541 live particles → **16 draws,
   16 GX_QUADS primitives, ~2,150 vertices per frame**. Two textures: `ef_s0502_001` (32×32,
   ONE/ONE additive) and `cn0000dust02` (128×128, SRCALPHA/INVSRCALPHA). (CONFIRMED, snapshot)
4. Final Haunt per frame: 7 bucket walks, 16 PTank draws, **0 RwIm3D calls**, 89
   `Im2DRenderPrimitive` calls (33 of them CSD 2D, the rest post-processing), 9 `RpClumpRender`
   (1 is the drop shadow), 1 KageTask shadow draw, 2 lens-flare draw calls that emit nothing
   (flare disabled on this stage), 113 `GXBegin` total = 89 Im2D + 16 PTank + 8 camera Z-fill
   quads. (CONFIRMED)
5. The other element renderers use RW immediate mode: Particle3D → `RwIm3D` 4-vertex
   TRISTRIP per particle (CPU-built oriented quads) or one clump per particle; Ray → `RwIm3D`
   camera-facing beams; Locus → `RwIm3D` ribbons (2 verts/sample); Shimmer element → EFB copy +
   `Im2D` distortion grid; Object → `RpClumpRender`. None of them is active on Final Haunt. (CODE)
6. The drop shadow (KageTask) is one flattened clump (`EFFSHADOWEVIL/HERO.DFF`) re-rendered per
   queued matrix with `RpClumpRender`, attached to **RenderLevel_Trans** (not "the current
   level"). One submission and one draw per frame on Final Haunt. No projection, no stencil. (CONFIRMED)
7. Lens flare = `Im2D` TRISTRIP quads (4 verts/element + an optional full-screen additive flash
   quad), drawn twice per frame (two passes) from GlareWorld; on Final Haunt `LF+0x04 == 0` so
   it early-outs. (CONFIRMED gate, CODE draw)
8. Game-side (outside the effect manager) immediate draws: Eff::Belt and Eff::LocusMultiVtx
   (Im3D strips, RenderLevel_Add), ElecSearchLight cone (Im3D strip, **RenderLevel_Glare**),
   ElecFan glow clump (**RenderLevel_Glare**), enemy laser line (Im3D LINELIST), LockonMark
   reticle (Im2D, RenderLevel_Sprite3D), HP gauges / 3D-anchored CSD scenes (Sprite3D). All go
   through RW (Im2D/Im3D/clump) — none writes GX directly. (CODE; HP gauge/CSD CONFIRMED)
9. **Direct GX from game code outside the SDK and the RW driver** is limited to: the
   self-contained **DiscErrorScreen** (0x8032D4CC family: its own projection, light, TEV, blend,
   vertex format and raw FIFO quads, and its own `GXCopyDisp`), **two `GXSetBlendMode(GX_BM_SUBTRACT)`
   sites in the atomic-shimmer pass (0x804381C4, 0x80438570)**, boot `GXSetMisc` (0x80051028),
   VI copy Y-scale (0x80306888), `GXAbortFrame` in the reset handler (0x803746B0) and a dead
   TexObj query (0x80393634). Only the atomic-shimmer sites are in-frame rendering leaks; they
   also desynchronise RW's cached blend state. (CONFIRMED by census; 0 runtime hits on Final Haunt)
10. The only **raw FIFO vertex writers** outside the GX SDK are inside the RW GameCube driver
    (PTank emitters, Im2D line/tri/prim/indexed, Im3D SubmitNoLight node 0x804A0B68, Z-fill quad
    0x80499E58, Rw2D 0x80499394) plus DiscErrorScreen's `0x8032E038`. A native renderer that cuts
    at GX *calls* would lose vertex data; it must cut at the RW API/driver level.
11. RenderLevel_Glare is **not** empty (doc 14 §9 negative refuted): ElecSearchLight
    (attach 0x80155DD8) and ElecFan (0x80266834, functor 0x802662A4) and one more functor
    (0x80265AE0) attach render-node functors to it.
12. Level render-state decoder `fn_80046D80` fully decoded (§4.4); doc 14's "0x40 = vertexAlpha=1"
    is really `ZTESTENABLE=1` (state 6).

---

## 2. Cut-line map for this lane

```
GAME / EFFECT LOGIC (stays guest)                         RW API (cut here)                    RW GC DRIVER (re-implement)            GX / FIFO
Effect_SpawnAtPos → EffectManager (update, 0x804262E4)
EffectDraw* tasks (7) → fn_804261C0 bucket walk
   ├ EffectParticlePJS::Render 0x80434824 ─────────────► RpPTankAtomicLock/Unlock (data)        0x80449010 → 0x80449A74 → 0x80456F04
   │                                                      RpAtomic renderCallBack 0x80449010 ─►   → pipeline cb 0x80449D54 → emitter ─► GXBegin(QUADS) + raw psq_st FIFO
   ├ Particle3D 0x80433DA8 / Ray 0x8042D870 / Locus 0x8043B618 ► RwIm3DTransform/RenderPrimitive ► Im3D pipeline node 0x804A0B68 ─────► GXBegin + raw FIFO
   ├ Shimmer 0x8043531C ────────────────────────────────► RwCameraTextureCopy + Im2D (device +0x30) ► 0x80496208 / 0x80491E80 ────────► GXCopyTex / GXBegin + raw FIFO
   └ Object 0x80433AA0 ─────────────────────────────────► RpClumpRender 0x80457C14 ─────────────► world atomic pipelines (geometry lane)
GlobalLensFlare 0x80431E38 ─────────────────────────────► Im2D (device +0x30)
KageTask draw functor 0x80357578 ───────────────────────► RpClumpRender
Eff::Belt / LocusMultiVtx / SearchLight / laser ─────────► RwIm3D*
LockonMark / CSD at Sprite3D ───────────────────────────► Im2D (device +0x30) / CSD CPlatformRW
AtomicShimmer 0x804381C4 / 0x80438570 ──────────────────► RpClumpRender + Im2D  AND  ─────────────────────────────────────────────────► GXSetBlendMode(SUBTRACT)  [LEAK]
DiscErrorScreen 0x8032D4CC ─────────────────────────────────────────────────────────────────────────────────────────────────────────► full raw GX + FIFO + GXCopyDisp [LEAK]
All render states: RwRenderStateSet/Get (device +0x20/+0x24 = 0x80498954 / 0x804984C4)
```

| Path | Guest entry (above cut) | RW API used | Bypasses RW? | Final Haunt / frame |
|---|---|---|---|---|
| EP-01 particles | 0x80434824 | RpPTank + RpAtomic render cb | no | 16 draws, 16 QUADS prims, ~2,150 verts |
| EP-02 Particle3D quads | 0x80433DA8 (+frag 0x80433F40) | RwIm3D TRISTRIP | no | 0 |
| EP-03 Particle3D models | 0x80433DA8 | RpClumpRender | no | 0 |
| EP-04 Ray beams | 0x8042D870 (+frag 0x8042D8DC) | RwIm3D TRISTRIP | no | 0 |
| EP-05 Locus ribbons | 0x8043B618 | RwIm3D TRISTRIP | no | 0 |
| EP-06 Shimmer element | 0x8043531C (+frag 0x80435408) | camera texture copy + Im2D TRISTRIP | no | 0 |
| EP-07 Object element | 0x80433AA0 | RpClumpRender | no | 0 |
| LF-01 global lens flare | 0x80431E38 (+frag 0x80431E94) | Im2D TRISTRIP | no | 2 calls, 0 prims (disabled) |
| KG-01 drop shadow | 0x80357578 | RpClumpRender | no | 1 draw, 1 clump |
| GS-01..10 game-side | see §13 | Im3D / Im2D / clump / CSD | no | HP gauge 4, 3D-anchored CSD 1 (the other 8 `RpClumpRender`/frame are world-object functors, geometry lane) |
| LK-01 atomic shimmer | 0x804381C4, 0x80438570 | clump + Im2D **+ raw GXSetBlendMode** | **partly** | 0 |
| LK-02 DiscErrorScreen | 0x8032D4CC | none | **yes, entirely** | 0 |
| LK-03..06 boot/VI/reset/dead | 0x80051028, 0x80306888, 0x803746B0, 0x80393634 | none | yes (non-frame) | 0 |

---

## 3. Final Haunt runtime census

### 3.1 Per-frame call counts (in-stage, 618-frame window, deterministic)

| Address | Function | per frame | Notes |
|---|---|---|---|
| 0x80495CBC | RwGameCubeRasterShowRaster (present) | 1.000 | frame clock |
| 0x804262E4 | EffectManager::Update | 1.000 | |
| 0x80427E2C | EffectParticle core tick | 16.000 | = live particle elements |
| 0x80434B6C | EffectParticlePJS tick shim (frame sync) | 16.000 | |
| 0x804261C0 | EffectBucket_DrawList | 7.000 | one per bucket task |
| 0x8002774C…0x8002761C | 7 bucket wrappers | 1.000 each | |
| 0x80449010 | PTank atomic render callback | 16.000 | 1 per drawn particle element |
| 0x80449A74 | PTank GC render callback | 16.000 | |
| 0x80449D00 / 0x80449D54 | PTank pipeline reinstance / render | 16.000 / 16.000 | |
| 0x8044C260 | emitter "2D-rotate + size" | 16.000 | only emitter used |
| 0x804490C4 | RpPTankAtomicCreate | 0 | 16 atomics created before 45e9, then pooled |
| 0x80433DA8, 0x8042D870, 0x8043B618, 0x8043531C, 0x80433AA0 | P3D / Ray / Locus / Shimmer / Object render | 0 | no such elements exist |
| 0x8049B87C / 0x8049BB48 / 0x8049B9AC | RwIm3DTransform / RenderPrimitive / RenderIndexedPrimitive | 0 / 0 / 0 | all 49,912 Im3D calls of the run happen before the 45e9 point (boot/title/loading), all from 0x80471CA4; none in-stage |
| 0x804A0B68 | Im3D SubmitNoLight node | 0 | |
| 0x80491E80 | Im2DRenderPrimitive | 89.000 | 31 from CSD `CPlatformRW::DrawPrimitive2D` 0x80364024, 2 from 0x80363F10, ≈56 post-processing (INFERRED split) |
| 0x8049245C / 0x80491B08 / 0x80491D0C | Im2D indexed / triangle / line | 0 | |
| 0x80396934 | GXBegin (all callers) | 113.000 | = 89 Im2D + 16 PTank + 8 Z-fill |
| 0x80499E58 | RwGX_ZFillQuad (camera clear) | 8.000 | |
| 0x80399D44 | GXSetBlendMode (all callers) | 162.3 | |
| 0x80498954 | RW GC fpRenderStateSet | 615.3 | |
| 0x80457C14 | RpClumpRender | 9.000 | 1 = KageTask |
| 0x80357578 | KageTask draw functor | 1.000 | |
| 0x80357A78 | KageTask exec | 1.000 | |
| 0x80357260 / 0x803573A4 | Kage submit upright / oriented | 1.000 / 0 | player only |
| 0x80431E38 | GlobalLensFlare draw | 2.000 | gated off (`LF+0x04 == 0`) |
| 0x80432360 | GlobalLensFlare update | 1.000 | |
| 0x800BF634 | GiHPGaugeEnemy draw functor (Sprite3D) | 4.000 | CSD project |
| 0x8033BB10 | CSD scene functor (Sprite3D) | 1.000 | CSD project |
| 0x80496208 | RwGameCubeCameraTextureCopy | 9.000 | post lane |
| 0x8042F258 / 0x804302EC | glare ladder / blur pass pair | 1 / 2 | post lane |
| 0x80430D64, 0x80430904, 0x8042E254, 0x8042E838, 0x80438C90, 0x804369CC | PreGlare, PostGlare, DrawBlur, SaveScreen, ScreenColor, FullScreenShimmer | 1 each | post lane |
| 0x800620D4, 0x802DE4BC, 0x802DF140, 0x8015390C, 0x80039948, 0x802662A4, 0x80358A54 | LockonMark, Eff::Belt, LocusMultiVtx, SearchLight, laser, ElecFan, FeverRing | 0 | not present in this window |
| 0x8032D7CC, 0x8032E038, 0x8032E10C, 0x804381C4, 0x80438570 | DiscError / atomic-shimmer leaks | 0 | |

Raw data: `_work/effects/perframe_C50_D55.txt`, `runs/*/stdout.txt`.

### 3.2 Live effect elements (savestate snapshots at 45e9 / 50e9 / 55e9 — identical shape)

`EffectManagerPJS` singleton `[0x805EF2D8] = 0x8057798C`; hide byte `mgr+0x24 = 0`; draw list
`mgr+0x14` holds 16 elements; `_rpPTankAtomicDataOffset [0x805F24F0] = 0xB8`.

| elems | class (vtable) | bucket +0x30 | saved bucket +0x148 | attr+0x50 blend mode | elem+0x108 | PTank blend src/dst | cap / live | texture (RwTexture+0x10) |
|---|---|---|---|---|---|---|---|---|
| 8 | EffectParticlePJS 0x8053D9BC | 5 Glare | 3 Add | 1 | 0x00000A91 (local space b11) | 2/2 ONE/ONE | 50 / 40–50 | ef_s0502_001, 32×32, filter/addr 0x1102 |
| 8 | EffectParticlePJS 0x8053D9BC | 5 Glare | 2 Trans | 0 | 0x00040055 (b2 alpha variant) | 5/6 SRCALPHA/INVSRCALPHA | 20 / 17–20 | cn0000dust02, 128×128, filter/addr 0x1102 |

Visible particles 534 / 532 / 541 → 4 vertices each. Every PTank: dataFlags `0x200000A7`
(STRUCTURE|VTX2TEXCOORDS|2DROTATE|SIZE|COLOR|POSITION), 5 clusters, record stride 44,
platform flags `0x00110100`, emitter `0x8044C260`, `vertexAlphaBlend = 1`, pipeline
`atomic+0x6C = 0x807865C0`, GC native vertex-format bytes
`04 04 04 04 04 04 04 04 04 04 05 01 00 00 00 00` (POS F32, TEX F32, CLR0 RGBA8, frac 0).
Cross-check of one additive element: particle node local position (4.61, 65.01, −1.95) appears
in the PTank position cluster as world (−112.15, 31.47, −331.67) — the local-space transform of
§5.4 step 3 is live.

---

## 4. The effect draw scheduler

### 4.1 Draw tasks → bucket wrappers → walker (CONFIRMED)

| Task exec (vt+0x0C) | Generated | Wrapper | Bucket | RenderLevel (RenderTaskSystem offset) |
|---|---|---|---|---|
| EffectDrawOpaq 0x801D2B14 | recomp_006.cpp:65960 | 0x8002774C (recomp_062.cpp:17227) | 0 | Opeq (+0x08) |
| EffectDrawPunch 0x801D2B90 | recomp_008.cpp:64364 | 0x80027728 (recomp_061.cpp:5824) | 1 | Punch (+0x10) |
| EffectDrawTrans 0x801D2C0C | recomp_010.cpp:74811 | 0x80027704 (recomp_060.cpp:7391) | 2 | Trans (+0x18) |
| EffectDrawAdd 0x801D2C88 | recomp_012.cpp:69248 | 0x800276E0 (recomp_059.cpp:3577) | 3 | Add (+0x20) |
| EffectDrawSub 0x801D2D04 | recomp_014.cpp:78741 | 0x800276BC (recomp_058.cpp:3095) | 4 | Add (+0x20), after Add |
| EffectDrawGlare 0x801D2D80 | recomp_016.cpp:80451 | 0x80027640 (recomp_056.cpp:3262) | 5 | GlareWorld (+0x24) |
| EffectDrawShimmer 0x801D2DFC | recomp_018.cpp:72104 | 0x8002761C (recomp_055.cpp:3565) | 6 | PostGlare (+0x2C) |

Each exec is `r3 = EffectManagerPJS_get() (0x800A35D8); bl wrapper`. Wrappers 0,1,2,3,4,6 are
`li r4, N; bl 0x804261C0`. The **Glare wrapper 0x80027640** additionally does
`sys = EffectSystem_get(); usa = __dynamic_cast(sys → EffectSystemUSA); cam = usa->vt[+0x24]()`
(= `fn_8000DCC0()[0]`, the main RwCamera), `RwCameraBeginUpdate(cam)` (0x8048652C), walk bucket
5, `RwCameraEndUpdate(cam)` (0x80486504). Only bucket 5 re-begins the camera.

### 4.2 Walker `fn_804261C0(mgr, bucket)` — recomp_026.cpp:150553 (CONFIRMED)

```
it = mgr->drawHead            ; mgr+0x14 (element ptr) / mgr+0x18 (refcount of the intrusive smart ptr)
if (mgr->hideAll /*byte mgr+0x24*/ == 0):
    for e in list (next = e+0x3C):
        if e->bucket (+0x30) == bucket: e->vt[+0x10](e)             ; Render
else:
    for e in list:
        if e->bucket == bucket:
            if (e->flags18 & 0x08) == 0:                               ; not already hidden
                e->flags18 |= 0x08; e->vt[+0x10](e); e->flags18 &= ~0x08
            else: e->vt[+0x10](e)
```
Every Render virtual returns immediately when `(flags18 & 1) == 0` (dead) or `flags18 & 8`
(hidden). The hide-all mode therefore calls Render only so that classes that own GPU-side
resources (Particle3D models, Object clumps) can release them (their "hidden" branch). No
sorting; draw order inside a bucket = draw-list order.

Re-implementation note: keep this walk in the guest. There is no per-bucket state; the native
renderer only needs the level state that is current when a bucket runs (§4.4) plus the per-element
states the Render virtuals set.

### 4.3 Element fields read by the renderers (CONFIRMED unless noted)

| Offset | Meaning (renderer view) |
|---|---|
| +0x00 | vptr (PJS vtables 0x8053D9BC Particle, 0x8053DAB0 P3D, 0x8053D8D4 Ray, 0x8053D860 Locus, 0x8053D728 Shimmer, 0x8053DA3C Object; Render at +0x10) |
| +0x18 | flags: b0 alive, b3 hidden-for-draw |
| +0x1C | resource/manager handle (key of the PTank pool) |
| +0x30 | bucket 0..6 |
| +0x3C / +0x40 | draw-list next ptr / refcount |
| +0x44 | attribute record ptr: +0x50 PTank blend mode (0/1/2), +0x58 texture-name string ptr |
| +0x54 | world RwMatrix (used for local-space particles, b11) |
| +0x84..+0xB0 | element 3×4 matrix rows, copied into the PTank frame every tick (0x80434B6C) |
| +0xFC | live particle/instance list head (next at node+0x68 for particles/P3D, +0x6C for rays, +0x3C for locus nodes) |
| +0x104 | request flags; +0x148 saved bucket (Particle), +0x138 (Ray) — glare re-bucketing |
| +0x108 | mode flags: b1 fog OFF, b2 alpha-blend variant, b4 texture by name, b11 local-space positions |
| +0x10C / +0x110 | capacity (= PTank maxPCount) / live count (= actPCount) |
| +0x13C | Particle: RpPTank atomic; P3D: instance array (152-byte records); Ray: Im3D buffer (144 bytes per instance) |
| +0x140 | Particle: RwFrame of the PTank; P3D: model template (≠0 → model mode) |
| +0x144 | Particle: skip-draw flag (≠0 → data is uploaded but no render call); P3D: texture |

### 4.4 RenderLevel state that a bucket inherits (CONFIRMED descriptors, CODE decoder)

Descriptor table 0x804D24EC, 23 × 0x14 `{namePtr, stateEnable, stateFlags, camMode, attachGroup}`
(dumped from the live image):

| idx | level | enable | flags | cam | grp |
|---|---|---|---|---|---|
| 0 | PreRender | 0 | 0 | 0 | 0 |
| 1 | OpeqWorld | 0 | 0 | 0 | 1 |
| 2 | Opeq | 1 | 0x20 | 0 | 1 |
| 3 | PunchWorld | 0 | 0 | 0 | 1 |
| 4 | Punch | 1 | 0x800 | 0 | 1 |
| 5 | TransWorld | 0 | 0 | 0 | 1 |
| 6 | Trans | 1 | 0x40 | 0 | 1 |
| 7 | AddWorld | 0 | 0 | 0 | 1 |
| 8 | Add | 1 | 0x80 | 0 | 1 |
| 9 | GlareWorld | 0 | 0 | 0 | 1 |
| 10 | Glare | 1 | 0x80 | 0 | 1 |
| 11 | PostGlare | 0 | 0 | 0 | 1 |
| 12 | Sprite3D | 1 | 0x20 | 0 | 1 |
| 13 | Last | 0 | 0 | 0 | 1 |
| 14 | Sprite | 1 | 0x418 | 1 | 2 |
| 15 | OnFade | 1 | 0x418 | 0 | 2 |
| 16 | Gindows | 1 | 0x418 | 0 | 2 |
| 17 | SpriteLast | 0 | 0 | 2 | 2 |
| 18 | PostRender | 0 | 0 | 0 | 3 |
| 19–22 | SpriteBack/Middle/Front, PostEffect | 0 | 0 | 0 | −1 |

`RenderTaskSystem` (0x80574384) level pointer offsets are `4 × idx` (+0x18 Trans, +0x28 Glare,
+0x30 Sprite3D, +0x38 Sprite …). A level with `enable` gets a SetRenderState payload (exec
0x801D04D4, recomp_027.cpp:75031): `RpWorldAddCamera`, `RwCameraBeginUpdate(mainCam)`,
`fn_80046D80(ctx, flags, lightCtx)`; the next level carries a ResetRenderState payload (exec
0x801D0418, recomp_025.cpp:62747) that only does `RwCameraEndUpdate` + remove camera — **states are
not restored**, the next state-enabled level reprograms them.

`fn_80046D80` (recomp_022.cpp:7288) — exact decode (RW state ids: 6 ZTEST, 8 ZWRITE, 9 FILTER,
10 SRCBLEND, 11 DESTBLEND, 14 FOG, 20 CULL; blend 1 ZERO 2 ONE 5 SRCALPHA 6 INVSRCALPHA
10 INVDESTCOLOR; `AlphaFn(c0,r0,op,c1,r1)` = cached `GXSetAlphaCompare` 0x80498F74;
`ZC(x)` = 0x804987C8: x=1 → alpha test forced ALWAYS + `GXSetZCompLoc(GX_TRUE)`, x=0 → cached
compare + `GXSetZCompLoc(GX_FALSE)`):

| Condition (first match) | SRC | DST | ZTEST | ZWRITE | Alpha test | ZC |
|---|---|---|---|---|---|---|
| flags & 0x40 (Trans) | 5 | 6 | 1 | 0 | ALWAYS(7),0,AND,ALWAYS,0 | 1 |
| flags & 0x80 (Add, Glare) | 5 | 2 | 1 | 0 | ALWAYS | 1 |
| flags & 0x100 (unused) | 10 | 1 | – | – | ALWAYS | 1 |
| flags & 0x800 (Punch) | 5 | 6 | 1 | 1 | GREATER(4),200,AND,ALWAYS,r1 | 0 |
| flags & 0x400 (Sprite/OnFade/Gindows) | 5 | 6 | 0 | 0 | ALWAYS | 1 |
| otherwise (Opeq, Sprite3D) | 5 | 6 | 1 | 1 | GREATER,1,AND,ALWAYS,r1 | 0 |

then: `flags & 0x08` → FILTER=2 (LINEAR) else FILTER=0; `flags & 0x10` → CULL=1 (NONE) else
CULL=2 (BACK); `flags & 0x04` → FOG=`ctx[+0x1809]` else FOG=0; `flags & 0x01` → light setup
(`ctx+8 |= 0x30`, 0x80047E24). Blending itself is only active when the draw enables vertex alpha
or the texture has alpha (RW GC driver semantics — state lane).

Effective state at each bucket:
* buckets 0 (Opeq) and Sprite3D functors: opaque recipe, alpha > 1 test, Z write on.
* bucket 1: punch recipe. bucket 2: Trans recipe. buckets 3, 4: Add recipe.
* **bucket 5 (GlareWorld, enable=0)**: inherits the Add recipe as modified by PreGlare's own
  draws that run first in GlareWorld (post lane) and by the DrawGlobalLensFlare task; inside
  `RwCameraBeginUpdate(mainCam)`. INFERRED: Z test on, Z write off, cull back, fog off.
* bucket 6 (PostGlare, enable=0): inherits whatever PostGlare/DrawBlur left (post lane).

Re-implementation note: element renderers override SRC/DST/VERTEXALPHA/FOG/CULL/FILTER/
TEXTURERASTER themselves (per path below); Z test/write comes from the level. A native renderer
must keep an RW render-state shadow and apply it per draw — it cannot bake state per bucket.

---

## 5. EP-01 — EffectParticlePJS → RpPTank billboards (the Final Haunt effect path)

### 5.1 Element ↔ PTank binding — ctor tail 0x80434D30 (recomp_051.cpp:143137) (CONFIRMED)

1. `pt = fn_80433514(elem+0x1C, elem+0x10C)` — take a pooled PTank of that capacity; else
   `pt = RpPTankAtomicCreate(elem+0x10C, 0xA7, 0)` (0x804490C4). `elem+0x13C = pt`; on failure clear
   alive bit.
2. `elem+0x140 = RwFrameCreate()` (0x80487D50); `RpAtomicSetFrame(pt, frame)` (0x80459D0C).
3. `ext = *(pt + [0x805F24F0])`: `ext->instFlags |= 0x00800000` (ACTNUMCHG), `ext->actPCount = 0`,
   `ext->vertexAlphaBlend (+0xAC) = 1`.
4. Blend from `attr = elem ctor arg r4` (stored at elem+0x44), `attr+0x50` and elem+0x108 b2:

   | attr+0x50 | elem+0x108 b2 = 0 | b2 = 1 |
   |---|---|---|
   | 0 | ONE / ZERO (2/1) | SRCALPHA / INVSRCALPHA (5/6) |
   | 1 | ONE / ONE (2/2) | SRCALPHA / ONE (5/2) |
   | 2 | DESTCOLOR / ZERO (9/1) (multiply) | same |
   | other | unchanged (render state captured at create) | unchanged |

   and `instFlags |= 0x10000000` (ALPHABLENDING). Live check: mode 1/b2=0 → 2/2, mode 0/b2=1 → 5/6.
5. If elem+0x108 b4 and `attr+0x58` (name ptr) ≠ 0: `tex = RwTexDictionaryFindNamedTexture(
   dyncast(resource)->txd, name)` (0x8048E04C) and `RpMaterialSetTexture(geometry->matList[0], tex)`
   (0x8045C2C0).
6. Lock/unlock all five clusters once (sanity).

Release: dtor 0x80434C2C returns the PTank to the pool (`fn_804334F0`), clears count, detaches
the frame. The pool means 16 atomics were created once (hook count 16) and reused.

### 5.2 Per-frame fill — Render 0x80434824 (recomp_046.cpp:151275, fragment 0x80434AD8 recomp_048.cpp:159630) (CONFIRMED)

```
if !(flags18 & 1) || (flags18 & 8) || live(+0x110) == 0: return
lock(pt, POSITION 0x1, WRITE 0x40000000) → pos {data, stride}
lock COLOR 0x2, SIZE 0x4, 2DROTATE 0x20, VTX2TEXCOORDS 0x80          ; RpPTankAtomicLock 0x80449458
for node in live list (head +0xFC, next node+0x68):
    P = (flags108 & 0x800) ? RwV3dTransformPoint(elem+0x54, node+0x04) : node+0x04
    pos[i]   = P (3 f32)
    color[i] = bytes ((node+0x54)>>22, (node+0x58)>>22, (node+0x5C)>>22, (node+0x60)>>22)  ; R,G,B,A
    size[i]  = (node+0x28, node+0x2C)                                ; full width, full height
    a = node+0x18; while (a > π || a < −π) a += (a >= 0 ? −2π : +2π); node+0x18 = a   ; π=[0x805FBDB0], 2.0=[0x8051C798]
    rot[i]   = a
    uv[i]    = (node+0x34, node+0x38, node+0x40, node+0x44)          ; TL u,v then BR u,v
RpPTankAtomicUnlock(pt)                                              ; 0x804495E8: instFlags |= lockFlags
ext->instFlags |= 0x00800000; ext->actPCount = live
if elem+0x144 != 0: return                                           ; data uploaded, no draw
fogSaved = RwRenderStateGet(FOG)
RwRenderStateSet(FOG, (flags108 & 0x2) ? 0 : 1)
pt->renderCallBack(pt)   (atomic+0x48 = 0x80449010)
RwRenderStateSet(FOG, fogSaved)
```
The rotation loop is the documented runtime hang hazard (`system.cpp` probe) — it never terminates
for |a| ≥ 2^27.

Particle node (`Effect::System::EffectParticleExecUnit`) fields used: +0x04 position, +0x18
rotation (Z angle, radians), +0x28/+0x2C size, **+0x34/+0x38 top-left UV, +0x40/+0x44
bottom-right UV** (doc 08 called these "UV offset / UV scale"), +0x54..+0x60 color as u32
`c<<22`, +0x68 next.

### 5.3 RpPTank on GameCube — data structures (CONFIRMED)

Plugin registration `RpPTankPluginAttach` **0x80448690** (recomp_005.cpp:147182):
`RwEngineRegisterPlugin(4, 0x12F, ctor 0x8044960C, dtor 0x80449660)` → 0x805F24F4;
`RpAtomicRegisterPlugin(4, 0x12F, ctor 0x804485E8, dtor 0x804485F8, copy 0)` → **0x805F24F0**.
Engine ctor 0x8044960C (recomp_020.cpp:146509) creates the PTank render pipeline:
`fn_804643AC(0x12F, 0, 0, 0, reinstanceCB 0x80449D00, renderCB 0x80449D54)` → **0x805F24F8**.

`RpPTankAtomicCreate(max, dataFlags, platFlags)` **0x804490C4** (recomp_014.cpp:169520):
`flags' = fn_80448F04(dataFlags)` (adds POSITION if neither POSITION nor MATRIX, adds STRUCTURE
0x20000000 if neither ARRAY nor STRUCTURE) → `RpPTankAtomicCreateCustom(max, flags', plat, &cbs
0x8056E7B8)` **0x80449118**: `RpAtomicCreate`, `ext = RwMalloc(0x138, 0x3012F)`, default callbacks,
data allocation (array form 0x80448708 / structure form 0x80448B48; per-cluster sizes from table
0x8051CDC0 = {12, 64, 12, 8, 4, 16, 4, 16, 32}), then `create` cb, then constants.

GC `RpPTankAtomicExtPrv`, 0x138 bytes (offsets verified against the lock/unlock/render code and
the live image):

| Off | Field | Live value (Final Haunt) |
|---|---|---|
| +0x00 | maxPCount | 50 / 20 |
| +0x04 | actPCount | live count |
| +0x08 | isAStructure (INFERRED) | 0 |
| +0x0C | rawdata | |
| +0x10 | defaultRenderCB (old atomic render cb) | 0x80456F04 |
| +0x14 | ptankCallBacks.alloc | 0 |
| +0x18 | .create | 0x804498A0 |
| +0x1C | .instance | 0x80449A6C (`return TRUE` — no instancing step on GC) |
| +0x20 | .render | 0x80449A74 |
| +0x24 | insSetupCB (unused on GC) | 0 |
| +0x28 | **GC emitter function** (slot of insPosCB) | 0x8044C260 |
| +0x3C | lockFlags | |
| +0x40 | instFlags | |
| +0x44 | **GC emitter format flags** (§5.5) | 0x00110100 |
| +0x48 | publicData.data | |
| +0x4C + 8·i | clusters[i] {data, stride}, i = POS, MATRIX, NORMAL, SIZE, COLOR, VTXCOLOR, 2DROTATE, VTX2TEX, VTX4TEX | pos/size/color/rot/uv, stride 44 |
| +0x94 | userData — **ptr to camera axes {right, up'} (6 f32) on the render cb's stack** | |
| +0x98 / +0x9C / +0xA0 | format.numClusters / stride / dataFlags | 5 / 44 / 0x200000A7 |
| +0xA4 / +0xA8 / +0xAC | srcBlend / dstBlend / vertexAlphaBlend | 2,2,1 or 5,6,1 |
| +0xB0 / +0xB8 / +0xC0 | cCenter / cSize / cRotate | 0,0 / 1,1 / 0 |
| +0xC4 | cColor (RwRGBA) — material color when no per-particle color | 000000FF |
| +0xC8 / +0xD8 / +0xF8 | cVtxColor[4] / cUV[4] / cMatrix (RwMatrix) | |

Structure-form record (STRUCTURE flag) = clusters concatenated in cluster order; for 0xA7:
`+0 pos(3 f32) +12 size(2 f32) +20 color(RGBA8) +24 rot(f32) +28 uvTL(2 f32) +36 uvBR(2 f32)`,
stride 44 (live).

`RpPTankAtomicLock(atomic, out{data,stride}, clusterFlag, lockFlags)` **0x80449458**
(recomp_018.cpp:161098): returns 0 and zeroes `out` if `dataFlags & clusterFlag == 0`; picks
the cluster; if STRUCTURE (`+0x9C != 0`) stride = format stride; with WRITE (0x40000000)
`lockFlags |= lockFlags|clusterFlag`. `Unlock` **0x804495E8**: `instFlags |= lockFlags; lockFlags = 0`.

Create callback **0x804498A0** (recomp_023.cpp:149144): builds an `RpGeometry` with 0 vertices and
flags `POSITIONS(2) | [NORMALS|LIGHT 0x30 if NORMAL] | [PRELIT 0x8 if any colour] |
[TEXTURED 0x4 if any UV]`, sets the GC native vertex formats (`fn_80463FCC/…FD8/…FF4/…FE4`), one
material, a 28-byte mesh header, `RpAtomicSetGeometry`, calls the emitter selector 0x804496A8, and
sets `atomic+0x6C (pipeline) = [0x805F24F8]`. Live geometry flags 0xE.

### 5.4 Render call chain (CONFIRMED by hook ratios 1:1:1:1:1)

1. **0x80449010** atomic render callback (recomp_011.cpp:157098): if `actPCount > 0`:
   `instance(atomic, &publicData, actPCount?, instFlags)` (0x80449A6C, no-op) then
   `render(atomic, &publicData, actPCount)`; `instFlags = 0`.
2. **0x80449A74** GC render callback (recomp_025.cpp:171301):
   * If `dataFlags & 0x8000` (CNSMATRIX): `right = cMatrix.right`, `up = cMatrix.up`.
   * else `ltm = RwFrameGetLTM(RwEngineInstance->curCamera->frame)` (0x804882E8); `right = ltm.right`,
     `up = ltm.up`; if `dataFlags & 0x20000` (CNS2DROTATE): rotate both by `cRotate`
     (`sin 0x803B2AFC`, `cos 0x803B2594`): `right' = right·c − up·s`, `up' = right·s + up·c` (CODE).
   * `up *= −1.0` (`[0x805FC0E8]`). `userData (+0x94) = &{right, up}` (stack).
   * If `vertexAlphaBlend`: save SRC/DST; `Set(SRC, srcBlend)`, `Set(DST, dstBlend)`,
     `Set(VERTEXALPHAENABLE, 1)`; else `Set(VERTEXALPHAENABLE, 0)`.
   * `defaultRenderCB(atomic)` = `AtomicDefaultRenderCallBack` 0x80456F04 → atomic pipeline
     `[0x805F24F8]`; restore SRC/DST.
3. Pipeline → **0x80449D00** reinstance cb (recomp_028.cpp:166983): if geometry has normals
   `fn_80464478` (light setup) else zero the light fields of the per-draw state struct `r4`
   (+0x1C/+0x20/+0x24).
4. Pipeline → **0x80449D54** render cb (recomp_029.cpp:151918), args `(atomic r3, drawState r4)`,
   `fmt = geometry→[0x805F25C8]` (native format bytes):

   ```
   GXClearVtxDesc
   GXSetVtxDesc(GX_VA_POS, GX_DIRECT);  GXSetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, fmt[0], fmt[12])
   if geoFlags & 0x10: GXSetVtxDesc(GX_VA_NRM, DIRECT); VtxAttrFmt(NRM, GX_NRM_XYZ, fmt[1], 0)
   if geoFlags & 0x08: GXSetVtxDesc(GX_VA_CLR0, DIRECT); VtxAttrFmt(CLR0, GX_CLR_RGBA, fmt[10], 0)
   if geoFlags & 0x84: GXSetVtxDesc(GX_VA_TEX0, DIRECT); VtxAttrFmt(TEX0, GX_TEX_ST, fmt[2], fmt[14])
   fn_804960FC(NULL, geoFlags & 0x10)      ; GXLoadPosMtxImm(view, PNMTX0) [+NrmMtx]; GXSetCurrentMtx(0)
                                           ;   → positions are WORLD space
   channel setup (drawState+0x24 = numLights):
     lights>0: matsrc/ambsrc chosen from plat bits 0x80000 (constant colour → MatColor = cColor)
               and 0x70000 (vertex colour); AmbColor = drawState ambient×255+0.5 or 0x805FC0F4
     lights=0 (effects): colour present (plat & 0x70000) → matsrc = GX_SRC_VTX, ambsrc = GX_SRC_REG
               constant colour (plat & 0x80000) → GXSetChanMatColor(GX_COLOR0A0, cColor)
   GXSetNumChans(1)
   GXSetChanCtrl(GX_COLOR0, enable, ambsrc, matsrc, lightMask, GX_DF_CLAMP, GX_AF_SPEC)
   GXSetChanCtrl(GX_ALPHA0, enable, ambsrc, matsrc, 0, GX_DF_NONE, GX_AF_NONE)
   GXSetChanCtrl(GX_COLOR1A1, 0, REG, REG, 0, GX_DF_NONE, GX_AF_NONE)
   GXSetNumTevStages(1)
   textured: GXSetNumTexGens(1); GXSetTexCoordGen2(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, GX_FALSE, GX_PTIDENTITY)
             GXSetTevOrder(0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0); GXSetTevOp(0, GX_MODULATE)
             RwGC texture bind 0x8049AC88(material texture, GX_TEXMAP0)
             0x804987C8(!(raster GC flag bit0))   ; alpha-test/Z-comp-loc toggle by raster alpha kind
   untextured: NumTexGens 0; TevOrder(0, NULL, NULL, COLOR0A0); TevOp(0, GX_PASSCLR); 0x804987C8(1)
   GXBegin(GX_QUADS 0x80, GX_VTXFMT0, 4·actPCount)
   GQR6 ← dequant table[fmt[0]] | fmt[12]<<8 (both halves);  GQR7 ← table[fmt[1]] …   ; tables 0x805FC0F8/0x805FC100
   emitter(actPCount, platFlags, &publicData)   ; ext+0x28, writes vertices to 0xCC008000
   ```
   Live effect format: POS F32, CLR0 RGBA8, TEX0 F32, no normals → GQR6/7 = 0 (float passthrough).

### 5.5 Emitter selection — 0x804496A8 (recomp_022.cpp:147665) (CODE; 0x8044C260 CONFIRMED live)

| dataFlags | USECENTER (0x01000000) = 0 | USECENTER = 1 | plat bit |
|---|---|---|---|
| MATRIX 0x8 | 0x8044A25C | 0x8044A620 | 0x1 / 0x2 |
| CNSMATRIX 0x8000 | 0x8044A9E4 | 0x8044ADAC | 0x4 / 0x8 |
| 2DROTATE 0x20 + SIZE 0x4 | **0x8044C260** | 0x8044C6A4 | 0x100 / 0x200 |
| 2DROTATE only | 0x8044B98C | 0x8044BDF8 | 0x40 / 0x80 |
| SIZE only | 0x8044B1B8 | 0x8044B590 | 0x10 / 0x20 |
| neither | 0x8044A9E4 | 0x8044ADAC | 0x4 / 0x8 |

Plat flag additions: UV — VTX2TEX 0x80 → 0x100000, VTX4TEX 0x100 → 0x200000, CNSVTX2TEX 0x80000 →
0x400000, CNSVTX4TEX 0x100000 → 0x800000. Colour — COLOR 0x2 → 0x10000, VTXCOLOR 0x40 → 0x20000,
CNSVTXCOLOR 0x40000 → 0x40000, none → 0x80000 (constant colour). NORMAL 0x10 → 0x1000000.

### 5.6 Exact vertex expansion of emitter 0x8044C260 (CONFIRMED by numeric evaluation of the generated C++)

Inputs per particle: centre `P`, size `(sx, sy)`, angle `θ` (already wrapped to [−π, π]),
colour `C` (u32 RGBA8), UV `(u0, v0)` = TL and `(u1, v1)` = BR. Per draw: `R` = camera LTM right,
`U` = −(camera LTM up) (both from §5.4 step 2; RW's camera right points to screen-left).

```
P(x)  = (4/π²) · x · (π − |x|)                          ; parabolic sine; constants table 0x8051CE08:
                                                         ;   {0, π, 0, −π/2, 1, 1, 4/π², 1}
s = P(θ)
c = P(w),  w = (θ − π/2 ≥ 0) ? θ − 3π/2 : θ + π/2      ; c ≈ cos θ, exact at 0, ±π/2, ±π
hx = 0.5·sx,  hy = 0.5·sy                                ; 0.5 = [0x805FC110]
W  = hx · (c·R − s·U)
H  = hy · (s·R + c·U)
v0 = P − W − H   uv (u1, v0)
v1 = P + W − H   uv (u0, v0)
v2 = P + W + H   uv (u0, v1)
v3 = P − W + H   uv (u1, v1)
per vertex, in this byte order: pos x,y (GQR6), pos z (GQR6), [normal (GQR7)], colour u32 (stw), uv (f32 pair)
```
Numeric check (R=(1,0,0), U=(0,1,0), size (2,4)): θ=0 → v0..v3 = (−1,−2) (1,−2) (1,2) (−1,2);
θ=π/2 → (−2,1) (−2,−1) (2,−1) (2,1); θ=π/6 → (−2.00,−1.22) (−0.22,−2.33) … i.e. c=0.889, s=0.556
(the parabola), not 0.866/0.5. Evaluator: `_work/effects/emit_eval.py`.

On screen (INFERRED from the view-matrix sign flip in 0x804960FC): at θ=0 the quad is upright,
v0 = top-right, v1 = top-left, v2 = bottom-left, v3 = bottom-right, UV TL maps to the top-left
corner; positive θ rotates clockwise on screen.

### 5.7 Re-implementation note (EP-01)

* **Cut point**: the RpPTank render callback `0x80449A74` (or the atomic render callback
  `0x80449010`) — it receives the atomic with fully-populated, locked-then-unlocked cluster data,
  blend modes, texture, `actPCount` and the current camera. Everything below (default atomic render,
  pipeline 0x80449D54, emitters, GQR/FIFO writes) exists only to feed GX and can be dropped.
* Must reproduce: world-space positions under the current camera view/projection; per-particle
  RGBA8 × texture (GX_MODULATE); size semantics (full extent, half used per side); the corner
  order and UV assignment above; SRC/DST from the PTank, vertex alpha on; FOG per element
  (`elem+0x108` b1) around the draw; Z test/write from the level (§4.4); **draw order = draw-list
  order, particle order = live-list order, no sorting**; per-element texture filter/addressing
  from the RwTexture (`filterAddressing`, live 0x1102 = LINEAR, WRAP/WRAP).
* Byte-for-byte: the parabolic sin/cos (up to 5.6 % axis error vs. exact trig) if the output
  must match the console; the `c<<22` colour fixed point is guest-side and already resolved
  when the cluster is written. A GPU instanced-quad shader that takes (P, size, θ, colour,
  uvTL, uvBR) per instance and (R, U) per draw reproduces the path exactly.
* Can be dropped: GC native format bytes, GQR programming, `GX_QUADS` (use two triangles), the
  PTank "instance" no-op, the per-emitter specialisation (the other nine emitters are needed only
  for the flag combinations of §5.5 — the effect engine creates only 0xA7).
* Keep in guest: element update, pooling (`fn_80433514/fn_804334F0`), lock/unlock (pure data).

---

## 6. EP-02 / EP-03 — EffectParticle3DPJS (CODE; 0 instances on Final Haunt)

Render `0x80433DA8` (recomp_041.cpp:143817) is **vtable 0x8053DAB0 +0x10** (verified dump) — it
resolves doc 20 §1a: the "texture GC" loop is its hidden/model-release branch; the quad branch
continues in fragment `0x80433F40` (recomp_042.cpp:157693). Update `0x804341D0`
(recomp_043.cpp:189168) builds the geometry after the shared particle core.

Instance array `elem+0x13C`, **152-byte records**: `+0x00..+0x8F` four `RwIm3DVertex` (36 bytes
each), `+0x90` RpClump* (model mode), `+0x94` u8 "registered/visible" flag.

**Quad mode** (`elem+0x140 == 0`), CPU build per live node (update 0x80434364..):
```
M = identity (template 0x805E17E0); RwMatrixTranslate(M, node+0x04)
RwMatrixRotate Z by node+0x18 (0x8040C85C), Y by node+0x14 (0x8040C898), X by node+0x10 (0x8040C8D4)
RwMatrixScale(M, node+0x28)
if flags108 b11: M = M × elem+0x54
for k in 0..3: v[k].pos = M · elem+(0x14C + 12k)          ; 4 authored corner positions in the element
               v[k].rgba = node colour >> 22
               v[k].u = (k < 2) ? node+0x40 (u1) : node+0x34 (u0)
               v[k].v = (k even) ? node+0x44 (v1) : node+0x38 (v0)
```
Render fragment 0x80433F40: save CULL, VERTEXALPHA, FOG, TEXTURERASTER, FILTER; set CULL=NONE,
FILTER=LINEAR, VERTEXALPHA=1, FOG=(b1 ? 0 : 1), TEXTURERASTER = `(elem+0x144 texture)->raster` or
NULL; per particle `RwIm3DTransform(rec, 4, NULL, 0x19 = VERTEXUV|VERTEXXYZ|VERTEXRGBA)` +
`RwIm3DRenderPrimitive(rwPRIMTYPETRISTRIP)`; one `RwIm3DEnd`; restore. Blend = level state.

**Model mode** (`elem+0x140 != 0` = template clump): update clones the template per particle
(`RpClumpClone` 0x804580E0 → rec+0x90) and positions it; render calls `RpClumpRender(rec+0x90)`
(0x80457C14) or, if `elem+0x148 != 0`, registers it once with the world (0x8046334C) instead;
hidden branch unregisters (0x804635BC/0x8046343C).

Re-implementation: quads are pre-transformed world-space triangles — intercept at RwIm3D (§10);
models go to the world pipeline (geometry lane). Primitive volume: 1 TRISTRIP (2 triangles, 4
vertices) per particle, one draw per particle (batching possible, order must be kept).

---

## 7. EP-04 — EffectRayPJS beams (CODE; 0 instances on Final Haunt)

Render `0x8042D870` (recomp_059.cpp:132900) + fragment `0x8042D8DC` (recomp_060.cpp:143920).
Instance list head `elem+0xFC`, next `node+0x6C` (`Effect::System::EffectRayExecUnit`); vertex
buffer `elem+0x134`, **144 bytes (4 × 36) per instance**.

Per instance: `eye = camera frame LTM pos` (EffectSystem GetCamera); `A = node+0x04`;
`d = node+0x10`; `len = |d|`; if `len > node+0x30` (max length) `B = A + d·(max/len)` else `B = A + d`;
if b11 transform A and B by `elem+0x54`; `axis = B − A`;
`side = normalize(cross(axis, B − eye))` (falls back to `A − eye` when degenerate, threshold
`[0x8051C300]`); `side *= node+0x34 (width) · [0x8051C304]`; colours from `node+0x38..+0x44`
and a second set used for the far end (`>>22` fixed point); vertices `A±side, B±side` as a
4-vertex strip (CODE; exact vertex order not decoded).

State: save CULL/VERTEXALPHA/FOG/TEXTURERASTER; set CULL=NONE, VERTEXALPHA=1,
**TEXTURERASTER=NULL (untextured)**, FOG per b1; `RwIm3DTransform(buf+144i, 4, NULL, 0x18 =
XYZ|RGBA)` + `RenderPrimitive(TRISTRIP)` per instance; `RwIm3DEnd`; restore. Blend = level state.

---

## 8. EP-05 — EffectLocusPJS ribbons / trails (CODE; 0 instances on Final Haunt)

Render `0x8043B618` (recomp_045.cpp:164351). Gates: alive, not hidden, `+0x114 ≠ 0`,
`+0x104 ≠ 0`; if `+0x10C & 0x4000` rebuild vertices first (`fn_8043AB68`). Vertex buffer
`elem+0x170`, **0x48 bytes per sample = 2 × RwIm3DVertex** (left/right edge).

State: CULL=NONE, VERTEXALPHA=1, TEXTURERASTER=NULL, FOG=(`+0x10C` b1 ? 0 : 1).
* `+0x110 ≤ +0x120`: one strip per locus node (list `+0xFC`, next `+0x3C`):
  `RwIm3DTransform(buf + i·(+0x120)·0x48, 2·(+0x120), NULL, 0x18)` + TRISTRIP.
* else: `+0x120 − 1` strips of `2·(+0x114 + 1)` vertices at `buf + i·(+0x110)·0x48`.
Then `RwIm3DEnd`, restore. Untextured, vertex-coloured, blend = level.

---

## 9. EP-06 — EffectShimmerPJS heat haze element (CODE; 0 instances on Final Haunt)

Render `0x8043531C` (recomp_057.cpp:147204) + fragment `0x80435408` (recomp_058.cpp:144511).
Gates: alive/not hidden, `+0x1BC ≠ 0`, `+0x1B8 ≠ 0` (vertex buffer). Camera = EffectSystem
GetCamera. Projects the element into screen space (`0x804834A8`, raster dims), builds a
`(+0x108+1) × (+0x10C)` distortion grid with sin/cos offsets (`0x8000FFE8`), 24-byte Im2D
vertices: x, y, z(=f25), RGBA = (255,255,255,alpha r29), u, v.

Then: `RwGameCubeCameraTextureCopy(captureRaster [[r27]+0], 0)` (0x80496208 → GXCopyTex of the
current EFB into the capture texture), `RwCameraBeginUpdate(cam)`, `TEXTURERASTER=capture`,
`SRC=SRCALPHA`, `DST=INVSRCALPHA`, `VERTEXALPHA=1`, `ZTEST=1`, `ZWRITE=0`, `FOG=0`,
`FILTER=NEAREST(1)`, `Im2DRenderPrimitive(TRISTRIP, +0x1B8, +0x1C8)` (device +0x30),
`RwCameraEndUpdate`. If the "no capture" byte is set it clears the alive bit instead.

Note: the RW3 per-element *atomic* shimmer (`EffectShimmerRW3::Draw` 0x80028BA4 →
`0x80438C64` → `0x80438168`, doc 10 §6b) is a different class/vtable; see LK-01 (§13.2).

Re-implementation: a screen-copy-then-distort pass — needs a native "copy current colour target
region to texture" at exactly this point of the frame, then a textured triangle strip in screen
space with nearest sampling.

---

## 10. RW immediate mode on GameCube (what EP-02..06, LF-01, GS-* call)

### 10.1 RwDevice table (CONFIRMED static dump 0x8056F508, copied into RwEngineInstance+0x10)

| RwEngine+ | RwDevice field | GC function |
|---|---|---|
| +0x20 | fpRenderStateSet | 0x80498954 (31-way jump table 0x8056FAD4) |
| +0x24 | fpRenderStateGet | 0x804984C4 |
| +0x28 | fpIm2DRenderLine | 0x80491D0C (recomp_042.cpp:170815) |
| +0x2C | fpIm2DRenderTriangle | 0x80491B08 (recomp_041.cpp:155540) |
| +0x30 | fpIm2DRenderPrimitive | **0x80491E80** (recomp_043.cpp:203655) |
| +0x34 | fpIm2DRenderIndexedPrimitive | 0x8049245C (recomp_044.cpp:177691) |
| +0x38..+0x44 | fpIm3DRender* | NULL statically (set when the Im3D module opens) |
| +0x108.. | RwMalloc/Free/Realloc/Calloc | |

Primitive map (table 0x8056F120, RwPrimitiveType → GX): 1 LINELIST→0xA8 GX_LINES,
2 POLYLINE→0xB0 GX_LINESTRIP, 3 TRILIST→0x90 GX_TRIANGLES, 4 TRISTRIP→0x98 GX_TRIANGLESTRIP,
5 TRIFAN→0xA0 GX_TRIANGLEFAN, 6 POINTLIST→0xB8 GX_POINTS.

### 10.2 Im2D (CODE, CONFIRMED live call counts)

`RwIm2DVertex` on GC = **24 bytes**: `+0 x f32, +4 y f32, +8 z f32 (screen Z), +0xC R,G,B,A u8,
+0x10 u f32, +0x14 v f32`.
Setup `0x8049148C` (recomp_039.cpp:142939), run by every Im2D draw:
`GXClearVtxDesc; POS DIRECT XYZ F32; CLR0 DIRECT RGBA8; NumTevStages 1; NumChans 1;
ChanCtrl(COLOR0A0, off, REG, VTX, 0, DF_NONE, AF_NONE); ChanCtrl(COLOR1A1, off, …)`;
if a texture raster is set (`[[0x805F270C]]`): `TEX0 DIRECT ST F32; TevOp MODULATE; 1 texgen
identity; TevOrder(0, TEXCOORD0, TEXMAP0, COLOR0A0); bind (0x80498854)`, else `PASSCLR`; viewport
from the current camera raster (`GXSetViewport`/`GXSetViewportJitter`) and an orthographic
projection built in `0x80491774` (recomp_040.cpp:168106). Then `GXBegin(map[prim], VTXFMT0, n)` and
raw FIFO stores in the order x, y, z, R, G, B, A, [u, v].

### 10.3 Im3D (CODE)

`RwIm3DTransform` 0x8049B87C (recomp_017.cpp:150375), `RwIm3DRenderPrimitive` 0x8049BB48
(recomp_020.cpp:150875), `RwIm3DRenderIndexedPrimitive` 0x8049B9AC (recomp_019.cpp:161211),
`RwIm3DEnd` 0x8049B95C (recomp_018.cpp:171310); module globals offset `[0x805F272C]`, one render
pipeline per primitive type at globals+0x04.. . `RwIm3DVertex` on GC = **36 bytes**: `+0 pos,
+0x0C normal (unused by effects), +0x18 R,G,B,A, +0x1C u, +0x20 v` (confirmed by the P3D writer).
Flags: 1 VERTEXUV, 2 ALLOPAQUE, 4 NOCLIP, 8 VERTEXXYZ, 16 VERTEXRGBA.
GC submit node `"SubmitNoLight.csl"` body **0x804A0B68** (recomp_028.cpp:189464; node definition
0x8056FD60): `0x804960FC(transformMatrix or NULL)` (GX pos matrix = view × object), POS XYZ F32,
CLR0 RGBA8, [TEX0 ST F32 + MODULATE + bind] else PASSCLR, lighting off; `GXBegin` + raw FIFO.

### 10.4 Re-implementation note (Im2D/Im3D)

Intercept at the RW API (`RwIm3DTransform/Render*/End`, device `fpIm2DRender*`) with the RW
render-state shadow (TEXTURERASTER, SRC/DST, VERTEXALPHA, ZTEST, ZWRITE, FOG, CULL, FILTER,
ADDRESS). Vertex formats are fixed (above) — a single dynamic vertex buffer layout per API
suffices. Drop: the GC node/FIFO emission and GQR. Keep: Im2D screen-space convention (camera
raster pixels, orthographic, z = screen Z) and the prim-type map (LINELIST/POLYLINE are used).

---

## 11. LF-01 — Global lens flare draw (CODE; gate CONFIRMED)

Object `LF = [EffectManager+0x50]` (144 bytes). Draw **0x80431E38(LF, pass)** (recomp_050.cpp:143477)
+ fragment 0x80431E94 (recomp_051.cpp:143132), called twice per frame: `DrawGlobalLensFlare`
(pass 1) and `…Another` (pass 0), both from GlareWorld.

```
cam = EffectSystem GetCamera
if LF+0x04 (enable) == 0: return                       ; Final Haunt: 0 → nothing drawn
if LF+0x8C && LF+0x8D: return
save SRC, DST, VERTEXALPHA, ZTEST, ZWRITE, FOG, FILTER
RwCameraBeginUpdate(cam)
Set SRC=SRCALPHA, DST=ONE, VERTEXALPHA=1, ZWRITE=0, FOG=0, FILTER=NEAREST
if LF+0x8C == 0 and pass != elemDesc[0].pass:          ; element 0 (sun glow), depth tested
    ZTEST=1; TEXTURERASTER = texArr[0]->raster; Im2D TRISTRIP(LF+0x28 verts[0..3], 4)
if LF+0x8D == 0:
    ZTEST=0
    for i in 1 .. LF+0x24−1:
        if elemDesc[i].pass (byte +0x20 of 0x24-byte record at LF+0x18) == pass:
            TEXTURERASTER = texArr[i] (LF+0x1C)->raster; Im2D TRISTRIP(LF+0x28 + 0x60·i, 4)
RwCameraEndUpdate(cam); RwCameraBeginUpdate(cam)
if LF+0x8D == 0 and pass == 0:                          ; full-screen flash quad
    SRC=ONE, DST=ONE, TEXTURERASTER=NULL; Im2D TRISTRIP(LF+0x2C, 4)
restore the 7 states; RwCameraEndUpdate(cam)
```
Vertices are produced by the update 0x80432360 (recomp_052.cpp:152187, doc 10 §7): sun direction
LF+0x08 → screen; per element 4 Im2D vertices along the sun-to-centre axis. No occlusion query
exists (doc 10 negative stands). Re-implementation: plain screen-space textured additive quads
at the GlareWorld point, element 0 with Z test against the scene depth, the rest without.

---

## 12. KG-01 — KageTask drop shadows (CONFIRMED)

Corrections to doc 04 are marked ▲.

* Singleton `[0x805F1EEC]` (live 0x809E43C8), clump `task+0x28` (live 0x809A02C0 — the
  `EFFSHADOWEVIL.DFF`/`EFFSHADOWHERO.DFF` flat disc from player-effect bank id 0xC), layer byte
  `[0x805F1EE8] = 0` (live).
* ▲ Exec 0x80357A78 (recomp_005.cpp:112884) attaches its render node to
  **`[RenderTaskSystem + 0x18]` = RenderLevel_Trans** and binds functor descriptor 0x80560800
  `{0, −1, 0x80357578}` via the boost-style bind (`0x80357FDC`, `0x80062DF8`); invocation goes through
  the PTMF thunk 0x803579F4 → `__ptmf_scall` 0x803A1AAC.
* Submissions (immediate mode, list drained every exec): `Kage_SubmitUpright` 0x80357260
  (recomp_046.cpp:121802), `Kage_SubmitOriented` 0x803573A4 (recomp_047.cpp:122267), wrapper
  0x80357230 (recomp_045.cpp:124210). Matrix recipe as doc 04 §4: lift 0.02·worldScale along up,
  align to the ground normal when `dot < 0.9995`, **scale (size, 0.0, size)** (oriented: `(size,
  0, size·aspect)` with a caller basis).
* Draw functor **0x80357578** (recomp_050.cpp:120937): for each 0x30-byte record:
  `Matrix3x4_ToRwMatrix` (0x80059318) → copy 0x40 bytes into `clump->frame (+4) + 0x10`
  (modelling matrix) → `RwFrameUpdateObjects` (0x8048827C) → `RpClumpRender` (0x80457C14).
  Sets no render state.
* Final Haunt: 1 upright submission and 1 functor call per frame → 1 `RpClumpRender`.

Effective GX recipe = the DFF material through the world atomic pipeline under the **Trans level
state**: SRCALPHA/INVSRCALPHA, Z test on, Z write off, cull back, fog off, alpha test ALWAYS
(§4.4). The flattened matrix has a zero Y row, so model normals collapse — the material must be
unlit/prelit for the result to be stable.

Re-implementation: no projector/decal work exists; draw the shadow disc as a normal model with
the per-record matrix at the Trans point. The zero-Y scale must be kept byte-for-byte (the disc
lies 0.02 units above the contact point; any Y thickness would z-fight or poke through slopes).
Depth offset/bias is not used by the console.

---

## 13. Game-side draws outside the effect manager

### 13.1 Render-level functor/task attachment census (CONFIRMED static; `levels.py`)

75 `RenderTaskSystem_Get` sites resolved. Rows relevant to this lane (non-world, non-HUD):

| Attach site | Level | Draw fn (descriptor) | Owner / what | API | Final Haunt / frame |
|---|---|---|---|---|---|
| 0x80357AE8 | Trans | 0x80357578 (0x80560800) | KageTask shadow | RpClumpRender | 1 |
| 0x80359230 | Opeq | 0x80358A54 (0x80560828) | ▲ EggLastMechFeverRing (doc 04 called it "EffectKage_LightSwitch") — renders clump `this+0x70` with render-context mode 5 after a StageLight record switch | RpClumpRender | 0 |
| 0x802DE398, 0x802DE43C | Add (task) | Eff::Belt draw 0x802DE4BC | belt strip | RwIm3D TRISTRIP 22 v | 0 |
| 0x802DF0C0 | Add (task) | Eff::LocusMultiVtx draw 0x802DF140 | multi-vertex trail | RwIm3D TRISTRIP n v | 0 |
| 0x80155DD8 | **Glare** | 0x8015390C (0x8052B908) | ElecSearchLight light cone (LIGHT.BIN) | RwIm3D TRISTRIP 26 v | 0 |
| 0x80266834 | **Glare** | 0x802662A4 (0x8054D2EC) | ElecFan / DE0403DENSIRYUU.DFF glow | RpClumpRender (CULL NONE around it, vertex colour via 0x80037BAC) | 0 |
| 0x80265AE0 | **Glare** | (bound functor, not decoded) | same module | – | 0 |
| 0x80062D24 | Sprite3D | 0x800620D4 (0x8051FEA0) | LockonMark homing reticle | Im2D TRISTRIP 4 v ×2 | 0 |
| 0x800C002C | Sprite3D | 0x800BF634 (0x80523758) | GiHPGaugeEnemy HP bars | Chao::CSD project | 4 |
| 0x8033C3E4 | Sprite3D | 0x8033BB10 (0x8055E6C8) | CSD scene anchored in 3D | Chao::CSD project | 1 |
| 0x802B5108 | Sprite3D | 0x802B4A8C (0x80558748) | model functor | RpClumpRender | 0 |
| 0x8032A1D0 | Trans | 0x80329CA4 (0x8055DF88) | TPjsEventActionFade | – | – |

Other attachments (Opeq object models such as Coaster/MorphDoor, Sprite/SpriteBack/SpriteFront/
OnFade/Gindows HUD and CSD) belong to the geometry and 2D lanes; full list in the script output.

### 13.2 Recipes (CODE)

| Draw | Saved / set render states | Primitive |
|---|---|---|
| Eff::Belt 0x802DE4BC | SRC=SRCALPHA, DST=ONE, CULL=NONE, TEXTURERASTER=NULL, VERTEXALPHA=1, FOG=0; restores SRC/DST/CULL/VA/FOG | vertices built on the CPU from two transformed edge points per step (0x8040C6CC), then one `RwIm3DTransform(buf, 22, NULL, 0x18)` + TRISTRIP |
| Eff::LocusMultiVtx 0x802DF140 | same as Belt | `RwIm3DTransform(buf, n, NULL, 0x18)` + TRISTRIP |
| ElecSearchLight 0x8015390C | ZWRITE=0, FOG=0, CULL=NONE, SRC=SRCALPHA, DST=ONE, VERTEXALPHA=1, TEXTURERASTER=NULL; restores CULL/ZWRITE/FOG | `RwIm3DTransform(buf, 26, matrix?, 0x18)` + TRISTRIP |
| Enemy laser line 0x80039948 (called from 0x802E2858, enemy module) | same as SearchLight | `RwIm3DTransform(v, 2, …)` + `RwIm3DRenderPrimitive(rwPRIMTYPELINELIST)` |
| LockonMark 0x800620D4 | saves ZTEST/ZWRITE/FOG/SRC/DST; sets ZTEST=0, ZWRITE=0, FOG=0, SRC=SRCALPHA, DST=ONE, VERTEXALPHA=1, FILTER=NEAREST, TEXTURERASTER=tex A / tex B (TXD names at r2−0x76A8/−0x76A0); restores the five saved states (not VERTEXALPHA/FILTER) | projects the target with the camera, `Im2DRenderPrimitive(TRISTRIP, v, 4)` twice |
| AtomicShimmer 0x80438168 / frag 0x804381C4 (LK-01) | `RpClumpRender(clump)` inside `RwCameraBeginUpdate(cam r31)`; then, if the element camera `obj+4` and `obj+0x68/+0x70` are set: begin `obj+4` camera, CULL=FRONT, blend = (`obj+0x78` ? SRC=ZERO, DST=ONE : **direct `GXSetBlendMode(GX_BM_SUBTRACT, GX_BL_ONE, GX_BL_ONE, GX_LO_SET)`**), end; `RpClumpRender(clump)` again under `r31`; then the capture 0x804382F8 | clump ×2 |
| AtomicShimmer 0x80438570 (LK-01) | if `r29 || obj+0x78`: SRC=ONE, DST=ONE else **direct `GXSetBlendMode(SUBTRACT, ONE, ONE, SET)`**; then restores SRC/DST/VERTEXALPHA/ZTEST/ZWRITE/FOG from obj+0x90.. | `Im2DRenderPrimitive(TRISTRIP, obj+8, 4)` full-screen |

Re-implementation: all but LK-01 are ordinary RW immediate/clump clients — no special handling
beyond §10. The *effect-level* ones (Belt, LocusMultiVtx) live in RenderLevel_Add under the Add
state; SearchLight/ElecFan live in RenderLevel_Glare (additive 0x80 state, inside the glare
section).

---

## 14. GX emission that bypasses RenderWare — full classified list

### 14.1 Direct GX SDK calls from outside 0x80393754–0x8039AE9C and outside the RW driver 0x80445000–0x804AA000 (CONFIRMED: whole-text `bl` scan, 987 GX call sites total)

| Function (true start) | Generated | GX calls | Class | When | Leak? |
|---|---|---|---|---|---|
| 0x80051028 | recomp_007.cpp:9874 | GXSetMisc | boot video init | boot | non-frame |
| 0x80306888 | recomp_051.cpp:107031 | GXGetYScaleFactor, GXSetDispCopyYScale | VI / display copy config (movie/video module) | mode change | non-frame (present config) |
| 0x8032D4CC..0x8032E200 (0x8032D7CC, 0x8032DBB4, 0x8032DC20, 0x8032DCA4, 0x8032DD28, 0x8032DDAC, 0x8032DE30, 0x8032E038, 0x8032E10C) | recomp_030.cpp:120670 … recomp_042.cpp:126217 | 32 distinct incl. GXSetProjection, GXLoadPosMtxImm, GXSetZMode, ChanCtrl, InitLight*, LoadLightObjImm, TevOp/Order, GXSetBlendMode, VtxDesc/AttrFmt, CullMode, Viewport, CopyClear, ColorUpdate, **GXCopyDisp**, **GXDrawDone**, InvalidateTexAll/VtxCache, InitTexObj/LOD, LoadTexObj, LoadTexMtxImm, TexCoordGen2, GXBegin | **DiscErrorScreen** — self-contained renderer | disc/system error only | **yes — entire frame** |
| 0x803746B0 | recomp_011.cpp:130487 | GXAbortFrame | reset handler | reset | non-frame |
| 0x80393634 | recomp_051.cpp:126721 | GXGetTexObjFmt/MipMap | orphan (0 callers) | never | dead |
| 0x80438168 frag **0x804381C4** | recomp_020.cpp:145492 | GXSetBlendMode | AtomicShimmer offscreen clump pass | per atomic-shimmer element | **yes (in-frame)** |
| 0x804382F8 frag **0x80438570** | recomp_024.cpp:143656 | GXSetBlendMode | AtomicShimmer composite quad | per atomic-shimmer element | **yes (in-frame)** |

Inside the effect library range everything else goes through RwDevice. The GX calls inside the RW
driver range (0x80445000–0x804AA000: world pipelines, PTank 0x80449D54, Im2D/Im3D, raster, camera,
render-state set) are *below* the cut line by definition.

### 14.2 Raw FIFO writers (0xCC008000) outside the GX SDK (CONFIRMED scan)

| Writer | Owner | Data |
|---|---|---|
| 0x8044A25C, 0x8044A620, 0x8044A9E4, 0x8044ADAC, 0x8044B1B8, 0x8044B590, 0x8044B98C, 0x8044BDF8, 0x8044C260, 0x8044C6A4 | RpPTank GC emitters (via `[r2+0x1998]`) | billboard vertices (§5.6) |
| 0x80491B08, 0x80491D0C, 0x80491E80, 0x8049245C | RW GC Im2D | Im2D vertices (§10.2) |
| 0x804A0B68 | RW GC Im3D SubmitNoLight node | Im3D vertices |
| 0x80499E58 frag 0x80499F10 | RwGX_ZFillQuad (camera clear) | Z-only quad |
| 0x80499394 | RW 2D quads (s16 pos + 2 u32) | (0 hits in-stage) |
| **0x8032E038** | **DiscErrorScreen glyph quad** | s16 x,y,z + RGBA8 + s16 u,v |

### 14.3 LK-02 — DiscErrorScreen specification (CODE; notes_20 §1 control flow)

Controller `0x8032D4CC` (recomp_030.cpp:120670) polls DVD state, picks an error class, dispatch
`0x8032DB70` → handler slot in table 0x8055E2A0 (+0x10.., 13 slots, localized strings at +0x00..).
Scene setup `0x8032D7CC` (recomp_032.cpp:121009):
```
GXSetProjection(ortho, GX_ORTHOGRAPHIC); GXLoadPosMtxImm(identity, PNMTX0); GXSetCurrentMtx(0)
GXSetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE)
GXSetNumChans(1); GXSetChanCtrl(GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT0, GX_DF_NONE, GX_AF_NONE)
GXSetChanAmbColor(COLOR0A0, …); one light: InitLightColor/Pos/Dir/Spot/DistAttn, GXLoadLightObjImm(…, GX_LIGHT0)
GXSetNumTevStages(1); GXSetTevOp(0, GX_MODULATE); GXSetTevOrder(0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0)
GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR)
GXClearVtxDesc; POS/CLR0/TEX0 DIRECT; VAT0: POS XYZ S16 frac 0; CLR0 RGBA8; TEX0 ST S16 frac 0
GXSetCullMode(GX_CULL_NONE); GXSetViewport(full); GXInvalidateVtxCache; GXInvalidateTexAll
GXSetCopyClear(colour, z); GXSetColorUpdate(GX_TRUE); GXCopyDisp(xfb, GX_TRUE); GXDrawDone
```
Font texture `0x8032E10C` (recomp_042.cpp:126217): `GXInitTexObj(img, w, h, fmt, CLAMP, CLAMP, no mip)`,
`GXInitTexObjLOD(LINEAR, LINEAR, …)`, `GXLoadTexObj(…, GX_TEXMAP0)`, texture matrix (texel→normalised)
`GXLoadTexMtxImm(m, GX_TEXMTX0 0x1E, GX_MTX2x4)`, `GXSetNumTexGens(1)`,
`GXSetTexCoordGen2(TEXCOORD0, MTX2x4, TEX0, GX_TEXMTX0, GX_FALSE, GX_PTIDENTITY)`.
Glyph quad `0x8032E038(x, y, u, v, rgba)` (recomp_041.cpp:119444): cell `W,H = [[0x805F1C44]+0x10/+0x12]`;
`GXBegin(GX_QUADS, VTXFMT0, 4)`; vertices `(x,y,0)(u,v)`, `(x+W,y,0)(u+W,v)`, `(x+W,y+H,0)(u+W,v+H)`,
`(x,y+H,0)(u,v+H)`, colour `rgba` on all, raw FIFO. Each handler draws its text then
`GXDrawDone` + `GXCopyDisp` itself.

Re-implementation: the native renderer must either (a) implement a minimal raw-GX path for this
screen (the only complete raw-GX client in the game) including its own present, or (b) replace
the screen by host UI when the controller 0x8032D4CC runs. It never runs during normal play
(0 hits) and never touches RW state.

---

## 15. Cut-line implications (consolidated)

1. **Intercept points for this lane** (all at or just below the RW API, all receive complete
   primitives):
   * RpPTank GC render callback `0x80449A74` (per particle system: clusters, blend, texture,
     camera) — replaces 0x80456F04 → 0x80449D00/0x80449D54 → emitters.
   * RwIm3D: `0x8049B87C` Transform, `0x8049BB48` RenderPrimitive, `0x8049B9AC`
     RenderIndexedPrimitive, `0x8049B95C` End (or the Im3D render pipeline node 0x804A0B68).
   * RwDevice Im2D entries `0x80491E80` / `0x8049245C` / `0x80491B08` / `0x80491D0C` and the
     render-state pair `0x80498954` / `0x804984C4`.
   * `RwGameCubeCameraTextureCopy` `0x80496208` (shimmer element, atomic shimmer) and camera
     begin/end `0x8048652C` / `0x80486504` (the glare bucket and lens flare re-begin the camera).
   * `RpClumpRender` `0x80457C14` (Kage, Object, P3D models, ElecFan) → world atomic pipelines.
2. **Reproduce**: the RW render-state shadow semantics per draw; level states of §4.4; the
   billboard expansion of §5.6 (or its exact-trig equivalent if pixel identity is not required);
   UV corner assignments of §5.6 and §6; world-space positions with the current camera; the glare
   re-bucketing (particles drawn inside the GlareWorld section, between PreGlare and PostGlare,
   is what makes them bloom); draw-list/live-list order; per-element fog override.
3. **Drop**: GC native vertex formats and GQR quantisation, FIFO emission, `GX_QUADS`, the
   PTank instance no-op, per-emitter specialisation, the alpha-test/Z-comp-loc toggles that exist
   only because GX needs `GXSetZCompLoc` for alpha test (reproduce the alpha-test outcome, not the
   mechanism), texture-bind caches.
4. **Leaks to handle explicitly**: the two `GXSetBlendMode(SUBTRACT)` sites (LK-01) — the host
   must translate that call into a native subtractive blend and must not trust RW's cached
   SRC/DST afterwards (the atomic-shimmer code restores SRC/DST through RW right after); the
   DiscErrorScreen (LK-02) full raw-GX client; boot/VI/reset GX calls are configuration only.
5. **Volume budget on Final Haunt**: effects = 16 draw calls, ~2,150 vertices (~1,075 triangles
   if converted) per frame; Im3D = 0; lens flare = 0; shadow = 1 clump. The immediate-mode total
   including HUD and post is 113 primitives per frame.

---

## 16. Corrections to existing documentation

| Doc / claim | Correction | Evidence |
|---|---|---|
| RECOMP_VISUAL_MAP 08 §8, 00 §3, 17: "5 double-buffered vertex streams; g_StreamFrameIndex 0x805F24F0 flipped by fn_80448690; streams fn_80449458/fn_804495E8" | They are **RpPTank**: 0x805F24F0 = `_rpPTankAtomicDataOffset` (live 0xB8), fn_80448690 = `RpPTankPluginAttach` (plugin 0x12F), fn_80449458/fn_804495E8 = `RpPTankAtomicLock/Unlock`, mask 0xA7 = rpPTANKDFLAG POSITION|COLOR|SIZE|2DROTATE|VTX2TEXCOORDS, fn_804490C4 = `RpPTankAtomicCreate` | §5.3, CONFIRMED |
| 08 §8 / 20 §4: "fn_80449D54 billboards on camera basis vectors 0x805FC0F0..0x805FC0FC (writer untraced)" | 0x805FC0F0..0x805FC118 are SDA2 constants (white/black RGBA, GQR type tables, 0.5, 255.0, −1.0, FIFO address). Billboarding happens in the ten emitters using the camera LTM right/up computed in 0x80449A74 and passed via ext+0x94 | §5.4–5.6 |
| 20 §5: "No RenderWare particle plugin (PTank/PrtStd)" | **Refuted** — RpPTank is linked and is the particle renderer | §5 |
| 20 §1a: 0x8044960C ambiguous (particle pipe vs skin pipe) | It is the PTank **engine-plugin constructor** creating the PTank GC pipeline (id 0x12F, cbs 0x80449D00/0x80449D54) into 0x805F24F8 | §5.3 |
| 20 §1a: 0x80433DA8 ambiguous (texture GC vs P3D render) | It **is** EffectParticle3DPJS::Render (vtable 0x8053DAB0+0x10); the release loop is its hidden branch; quads in fragment 0x80433F40 | §6 |
| 08 §4: particle node +0x34 "UV offset", +0x40 "UV scale" | +0x34/+0x38 = top-left UV, +0x40/+0x44 = bottom-right UV (PTank VTX2TEXCOORDS) | §5.2, snapshot |
| 08 §9: "attr +0x50 billboard mode 0..4" | In the particle ctor `attr+0x50` selects the PTank blend pair (0/1/2 table of §5.1); live values 0 and 1 match blends 5/6 and 2/2 | §5.1 |
| 14 §9.1 / 00 open question: "RenderLevel_Glare has no attached tasks" | Refuted: ElecSearchLight (0x80155DD8 → 0x8015390C), ElecFan (0x80266834 → 0x802662A4) and 0x80265AE0 attach bound draw functors to it (functors, not tasks) | §13.1 |
| 14 §4 table row 0x40: "vertexAlpha=1, zwrite=0" | State 6 is ZTESTENABLE: Trans = SRCALPHA/INVSRCALPHA, **ZTEST=1**, ZWRITE=0; full decode of 0x20/0x80/0x100/0x400/0x800/0x08/0x10/0x04/0x01 in §4.4 | §4.4 |
| 14 §4 "Add vs Sub … flag byte at obj+120 switches to GXSetBlendMode(SUBTRACT) at 0x804381C4/0x80438570" | Those sites belong to the **atomic-shimmer** pass, not to EffectDrawSub; bucket 4 has no subtractive mechanism of its own | §13.2 |
| 04 §2: "Attach render node into the CURRENT render level" | Attaches to **RenderLevel_Trans** (`[RenderTaskSystem+0x18]`) | §12 |
| 04 §8.6 / corrections §4: 0x80358A54 "EffectKage_LightSwitch, not a draw" | It **is** a draw functor (descriptor 0x80560828, attached to RenderLevel_Opeq at 0x80359230) of `EggLastMechFeverRing`; the light-record switch is part of that draw | §13.1 |
| 10 §7 lens-flare field map (textures LF+0x14/0x18, element table LF+0x24) | Draw side reads: LF+0x18 element descriptors (0x24 stride, pass byte +0x20), LF+0x1C texture pointer array, LF+0x24 element count, LF+0x28 vertex array (0x60 per element), LF+0x2C flash quad; gate LF+0x04 then `!(LF+0x8C && LF+0x8D)` | §11 |
| 00 §3 layer model: "Effects draw through RW Im2D / im-prim submitters and the shared vertex-stream pipeline callback 0x80449D54" | Particles: RpPTank; Particle3D/Ray/Locus: RwIm3D; Shimmer element: EFB copy + Im2D; Object: RpClumpRender | §2 |
| RECOMP_VISUAL_MAP generated line numbers (all docs, db/functions.csv) | Stale since the corpus was regenerated; use the lines cited here | §0 |

---

## 17. Open questions

1. Exact inherited GX state at the GlareWorld bucket (PreGlare and DrawGlobalLensFlare run first
   in that level and may change Z/cull state) — needs a per-draw GX state dump at the first PTank
   `GXBegin` of bucket 5 (a GX-state capture tool exists only for 2D primitives).
2. Winding/cull behaviour of PTank quads under `CULL=BACK` from the Add state — visible in game,
   so they pass; the on-screen winding in §5.6 is INFERRED from the view-matrix sign convention.
3. `elem+0x144` (skip-draw after fill) owner/semantics; `RpPTankExtPrv+0x08` meaning on GC.
4. Ray vertex order and the second colour set (`node+0x48..`) — not decoded to the vertex level;
   no Ray elements on Final Haunt to observe.
5. The functor at RenderLevel_Glare attach 0x80265AE0 and the `Eff::Belt` owning class (strings
   only name "Eff::Belt").
6. Identity of the only in-run Im3D client `0x80471CA4` (49,912 indexed TRILIST calls, all before
   the stage starts; RW 2D-toolkit-like module 0x8047xxxx — 2D lane).
7. The per-frame split of the ~56 non-CSD Im2D calls between glare ladder, blur, screen colour
   and full-screen shimmer (post lane).
8. Stages other than Final Haunt: Ray/Locus/P3D/Shimmer/Object elements, lens flare, searchlight,
   belts, lock-on reticle were not exercised in this window — their per-frame volume is unmeasured.

---

## 18. Reproduction

```
W=<historical-renderer-work>/effects
bash $W/run.sh D55 55000000000            # fresh save-data from the seed, all watches in watchlist.txt
bash $W/run.sh C50 50000000000 --save-state --save-state-file $W/runs/C50/end.gekkostate
py $W/snap.py $W/runs/C50/end.gekkostate  # live effect draw list, PTank ext, textures
py $W/emit_eval.py                        # numeric corner evaluation of emitter 0x8044C260
py $W/scan.py; py $W/levels.py            # GX / device / watch-target / render-level census
py $W/dol.py 0x80449D54                   # annotated linear disassembly (capstone) of any address
```
Do not `--watch` 0x80434824 or 0x80449458 (runtime probes replace the hook).

# Motion blur — exact RenderWare/GX state and geometry

## Contract and evidence

This document describes the original local **Draw 8042E254**, **Save 8042E838**, and their installed GameCube backend, not a replacement renderer. Target SHA-256: `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`. Addresses/offsets are hexadecimal; RW IDs and argument values in tables are decimal unless prefixed `0x`. Ranges are end-exclusive.

Primary evidence is the [121 assembly exports](motion_blur_functions.md), [80 raw data records](motion_blur_original_addresses.md), [04 bottom-up closure](evidence/04_closure.md), [05 copy/lifecycle closure](evidence/05_closure.md), [06 state analysis](evidence/06_render_state.md), [07 geometry/matrix analysis](evidence/07_matrix_camera.md), and [backend instruction notes](evidence/backend_source_notes.md). Earlier missing-table lists are not current unknowns. Tables below decode **actual local words**, not names borrowed from another binary's SDK map. No GPU trace is available.

## 1. Installed API boundary and pass requests

Engine is `ptr32[805F265C]`. Device **8056F50C**, copied by system operation4 to engine+10, installs **+20=80498954 SetState**, **+24=804984C4 GetState**, **+30=80491E80 Im2D primitive**. Standard operation **11**, not 9, installs 27 records from 8051D400 into engine+48/count29; camera begin/end are +4C/+70 and subraster callback is **+78**. See [callgraph](motion_blur_callgraph.md).

Draw first gates enable, alpha, provider/camera and B+78. It then makes these Get requests **before BeginUpdate**, computes vertices, and calls Begin at **8042E60C**. Only a successful Begin executes the state-setting/primitive/restoration block.

| RW ID / meaning | Getter case | Setter case | Draw request; original Get / Set / Restore call sites | Save request |
|---|---|---|---|---|
| 1 raster binding | 804985A8 | 80498C18 | Hroot at **8042E634**; **no snapshot/restore** | Sroot at 8042E8F0, only if first Begin succeeds |
| 10 source blend | 804985E8 | 80498D38 | 5; **E2E8 / E654 / E758** | 2 at E910 |
| 11 destination blend | 80498600 | 80498DC0 | 6; **E308 / E674 / E778** | 1 at E930 |
| 12 unsupported | **804986B0** | **80498F58** | request1; **E328 / E694 / E798**, all ineffective | No request |
| 6 depth test policy | 804985D0 | 80498CA4 | 0; **E348 / E6B4 / E7B8** | Inherited |
| 8 depth write | 804985BC | 80498C38 | 0; **E368 / E6D4 / E7D8** | Inherited |
| 14 fog enable | 804984E4 | 80498990 | 0; **E388 / E6F4 / E7F8** | Inherited |
| 9 texture filter | 80498590 | 80498BFC | 1; **E3A8 / E714 / E818** | Inherited |

Abbreviated E-sites above have prefix **8042**. Dispatch tables are **8056FA58** (get) and **8056FAD4** (set), indexed directly by ID, not ID-1. RW9 is **filter**, not cull/shade. Getter12 returns0 without writing or dereferencing output; its raw word **8056FA88=804986B0**. Setter word **8056FB04=80498F58** returns initial r6=0 without state/GX work. Thus Draw issues **six implemented restores plus the ignored ID12 request**, not seven effective state changes. It still loads the unwritten saved word at **8042E780**; the original stack residue is not a saved vertex-alpha flag.

Supported getter outputs are full cached words except filter's low8; return statuses are ignored by the core. SetRaster changes T+0 and assigns dirty flags 0x01000000 when binding differs. Filter substitutes low8 of T+50, preserves upper24 and performs no immediate GX call or validation. Neither request sets U/V wrap.

### Blend factors and validation

Raw **8056FA28** contains `{0,0,1,2,3,4,5,6,7,2,3}` indexed by RW value:

| RW value | GX value | Local factor meaning / usage |
|---|---:|---|
| 0 | 0 | Table word only; not an accepted unequal blend request |
| 1 | 0 | ZERO; Save destination |
| 2 | 1 | ONE; Save source |
| 3 | 2 | SRCCOLOR in destination-factor interpretation |
| 4 | 3 | INVSRCCOLOR in destination-factor interpretation |
| 5 | 4 | SRCALPHA; Draw source |
| 6 | 5 | INVSRCALPHA; Draw destination |
| 7 | 6 | DSTALPHA |
| 8 | 7 | INVDSTALPHA |
| 9 | 2 | DSTCOLOR in source-factor interpretation |
| 10 | 3 | INVDSTCOLOR in source-factor interpretation |

Unequal source requests accept **1,2,5..10**; unequal destination requests accept **1..8**. Equality returns1 **before** validation. The selected arms call `80399D44 GXSetBlendMode(1,srcGX,dstGX,0)` at **80498DA0/80498E10** and update cache+34/+38. Cache hits can suppress hardware calls. GX modifies cached context+1D0 (source bits8..10, destination bits5..7, blend enable, logic/subtract fields) and emits BP/FIFO. It does **not** simultaneously enable all color/alpha writes or reset dither.

### Depth, fog and alpha tests

- RW6 enabled requests use **GXSetZMode(1,3,cachedWriteLow8)** (LEQUAL); disabling uses **(1,7,cachedWriteLow8)** (ALWAYS). RW8 requests use **(1,cachedCompare,1/0)**. Under the normal coherent cache, Draw's combined disabled-test/write policy is **(1,7,0)**, not hardware-enable0. Changes are cache-mediated, not unconditional calls.
- RW14 disable calls **GXSetFog at 80399A0C** with `type=0`, **f1=5, f2=10, f3=0.05, f4=10**, and cached RGBA. Literals: **805FC44C=40A00000**, **805FC450=41200000**, **805FC454=3D4CCCCD**. Restore-to-enabled may refresh planes from current camera+80/+84/+88, honoring end override; type lookup **8056FA18={0,2,4,5}** is known. This is not an exact snapshot of every prior fog plane.
- Flush **80498854** realizes texture then reloads current T and tests **the bound raster's own plugin alpha bit**. It computes `beforeTexture=(alphaFlags&1)^1`. If changed, 1 installs `(7,0,0,7,0)` = ALWAYS/AND/ALWAYS; 0 reinstalls **inherited cached alpha comparisons/references**, then calls ZCompLoc **80399E24**. Blur binds alpha-bearing roots, so a universally permissive alpha test is **not** established. This behavior is unrelated to unsupported RW12.

## 2. Draw geometry and exact numeric domains

B is the blur object. Let V=C.raster and P=V.root, `(x,y)=s16(V+1C/+1E)`, `(w,h)=s32(V+C/+10)`, `(pw,ph)=u32(P+C/+10)`, and `s=f32(B+4)`. **S/U** below denote the original signed/unsigned GPR-to-single conversion sequences, including their double biases. Every displayed arithmetic operation retains its PPC single rounding; `wrap32` occurs before signed conversion.

```text
u0 = FDIVS(FADDS(0.5,S(x)),                 U(pw))
u1 = FDIVS(FADDS(0.5,S(wrap32(x+w))),       U(pw))
v0 = FDIVS(FADDS(0.5,S(y)),                 U(ph))
v1 = FDIVS(FADDS(0.5,S(wrap32(y+h))),       U(ph))
d  = FSUBS(s,1)
dx = FMULS(0.5,FMULS(d,S(w)))
dy = FMULS(0.5,FMULS(d,S(h)))
TL = (-dx,             -dy,             u0,v0)
BL = (-dx,             FADDS(S(h),dy),  u0,v1)
TR = (FADDS(S(w),dx),   -dy,             u1,v0)
BR = (FADDS(S(w),dx),   FADDS(S(h),dy),  u1,v1)
```

The original range is **8042E3C0..8042E604**. Literals **8051C388=0.5**, **8051C38C=1**, signed bias **8051C390=4330000080000000**, unsigned bias **8051C398=4330000000000000**. UV denominators come from the **current camera root, not the sampled history root**. The addition is **+0.5**, not a negative half-texel correction. XY is overwritten absolutely every eligible Draw, not expanded cumulatively. No FMA occurs in this geometry block; algebraic reassociation or host casts can change bits/signed zero.

RGB is FF/FF/FF; alpha is B+1. Z was initialized by four separate engine+18 near-Z loads at construction; normal device near-Z word is +0, while far-Z is **3F7FFFFF**, not exactly1. The Draw array has no active center/velocity input at B+8/+C. Scale1 is unexpanded, scale>1 enlarges about the view's center, scale<1 contracts; arbitrary scale and zero dimensions are not sanitized.

Capture geometry is constructed once from camera **root** dimensions W/H: full XY `(0,0),(0,H),(W,0),(W,H)`; half XY substitutes signed `W/2,H/2` truncated toward zero. Both retain full-range biased UVs `0.5/U(W)` to `(0.5+S(W))/U(W)` and analogous V, white/opaque vertex colors and construction Z. Scratch dimensions instead came from the construction camera **view**. Compatibility after a camera/view/size change is not guaranteed.

## 3. Exact Im2D requests and FIFO format

`80491EA4 -> 8049148C` prepares shared Im2D. The blur uses its nonnull/textured path; these argument tuples are numeric original requests, not a modern shader specification.

| Original site | Local GX endpoint | Argument tuple |
|---|---|---|
| 8049149C | 80395758 ClearVtxDesc | No arguments |
| 804914A8 / 804914CC / 80491550 | 8039530C SetVtxDesc | `(9,1)`, `(11,1)`, `(13,1)` = direct POS/CLR0/TEX0 |
| 804914C0 | 80395790 SetVtxAttrFmt | `(0,9,1,4,0)` = VAT0, XYZ F32 |
| 804914E4 | Same | `(0,11,1,5,0)` = RGBA8 |
| 80491568 | Same | `(0,13,1,4,0)` = ST F32 |
| 804914EC / 804914F4 | 803999E4 / 80397C80 | NumTevStages1 / NumChans1 |
| 80491514 / 80491534 | 80397CBC SetChanCtrl | `(4,0,0,1,0,0,2)` / `(5,0,0,0,0,0,2)` |
| 80491574 | 803992EC SetTevOp | `(0,0)` stage0 modulate |
| 8049157C | 80395FC8 SetNumTexGens | 1 |
| 80491598 | 80395D48 SetTexCoordGen2 | `(0,1,4,0x3C,0,0x7D)`; coord0, 2x4, TEX0, identity/postidentity, no normalize |
| 804915AC | 80399848 SetTevOrder | `(0,0,0,4)`; stage0, coord0, map0, COLOR0A0 |
| 80491EC0 | 80396934 GXBegin | **`(0x98,0,4)`** |

Raw primitive table **8056F120**, index4 word **8056F130=0x98**, establishes a triangle strip. The pair loop emits **four** vertices in TL/BL/TR/BR order, each **xyz F32, RGBA bytes, uv F32**, 24 bytes, total96 vertex bytes to **CC008000**. Original memory loads include Z/Y/X and V/U order; FIFO writes are X/Y/Z and U/V. There is no `GXEnd` call. The loop is a vertex emitter, not a multi-tap blur kernel. Core submissions are **8042E738** (B+14), **8042E954** (B+7C), **8042E99C** (B+DC), all primitive4/count4.

### TEV program and effective alpha

Selected raw words at **80568B00 / 80568B28** are **C008F8AF / C108F2F0**. Color inputs `(A,B,C,D)=(15,8,10,15)` and alpha `(7,4,5,7)` select `(zero,texture,raster,zero)`, ADD, zero bias, scale1, clamp, previous-register destination. **GXSetTevOp preserves alpha word's low four swap-selector bits**; no identity swap-table/selector reset is issued here.

Under identity swaps and ordinary direct-stage state, before tests/coverage/quantization:

```text
Csrc = Ctex * Cvertex = Ctex for white vertices
Asrc = Atex * (B.alpha / 255)
Cout = Csrc*Asrc + Cdst*(1-Asrc)       // Draw's GX4/5 blend
```

This is a conditional explanatory equation, not a bit-exact EFB simulator. Captured alpha participates; **alpha128 does not unconditionally mean 128/255 history weight**. RGBA8/alpha-presence1 does not force texture alpha1. Indirect state, swap tables, alpha/depth tests, scissor, culling, write masks, EFB format and fixed-point rounding can affect the result. Save's ONE/ZERO blend requests replacement RGB, but do not guarantee accepted fragments or lossless restoration.

## 4. Texture format, sampler and realization

Nonempty 0x505 roots resolve to **GX6 RGBA8, depth32, alpha flag1, no palette/no mipmaps**, maxLOD0. Payload is **64*ceil(W/4)*ceil(H/4)** for positive nonoverflowing dimensions; allocation requests payload+31, retains unaligned owner+18 and aligned pixels+1C. `dcbi` is not an image clear. See [globals](motion_blur_globals.md).

| RW filter index in T+50 low8 | GX min / mag from 8056FC90 | SDK interpretation |
|---:|---|---|
| 0 | 0 / 0 | Raw entry; not a validated policy |
| 1 | 0 / 0 | Nearest / nearest (**Draw requests this**) |
| 2 | 1 / 1 | Linear / linear |
| 3 | 2 / 0 | Nearest-mip-nearest / nearest |
| 4 | 3 / 1 | Linear-mip-nearest / linear |
| 5 | 4 / 0 | Nearest-mip-linear / nearest |
| 6 | 5 / 1 | Linear-mip-linear / linear |

GX minimum-filter encoding bytes **805EED30** are `{0,4,1,5,2,6}`. U/V nibble table **8056FCC8** is `{0,1,2,0,0}`: GX clamp/repeat/mirror/clamp/clamp for indices0..4. These tables do not validate out-of-range inherited indices. Save does **not** choose filter or wrap; Draw restores the prior low-byte filter before Save's later realization. Open's initial filter2/U1/V1 values are not a capture-time guarantee. In particular, do not call the reduction intrinsically bilinear or impose clamp-to-edge.

`8049AC88` follows T.raster -> root plugin and stamps its token. On dirty nonpaletted realization, **8049B2F8 -> 80397E34 GXInitTexObj** receives root pixels/format, bound raster low16 dimensions, translated inherited wraps and mipmap0. **8049B348 -> 803980C8 GXInitTexObjLOD** supplies table min/mag, minLOD0, maxLOD0, bias0, clamp1, edgeLOD1, anisotropy0. A sampler-only change preserves bias/clamp/edge/aniso via the real SDK getters; an unchanged descriptor skips Init but still loads. SDK LOD constants include bias lower -4, upper trigger4/replacement3.99 (`407F5C29`), bias scale32, LOD bounds0..10, scale16 and bias decoding1/32. No extra levels are invented.

`8049B524 -> 8039843C` selects a cache region through GX-context+4C8, then calls `803982C0`; the alternative preloaded path is at **8049B538**. The loader emits texture-image **BP94** for unit0 with `(pixels>>5)&0x1FFFFF`. This is the same allocation written by capture, not a second upload/copy or temporal buffer. Cached-sampler flags and region selection remain shared mutable state.

## 5. Two copies, optional reduction and sampled restoration

Save clears B+78 first. With its gates passing, its sequence is:

1. **8042E8C0 -> 80496208(Sroot,0)**: full scratch-root source rectangle.
2. First Begin at **E8C8**; if successful, bind Sroot, set RW source2/destination1, submit half quad at **E954**, End at E95C.
3. **E968 -> 80496208(Hview,0)**: history-view source rectangle, Hroot destination layout.
4. Second Begin at **E970**; if successful, submit full quad at **E99C**, End at E9A4. **No texture rebind or blend reset here.**
5. **E9AC: valid=1**, even if either/both optional Begin calls failed.

The copy selector is **0 twice**. The half-size quad, not GX half-scale copy, performs reduction. Actual clear is **low8(u32[805F2688])** loaded at **804963AC/B4**, independent of selector. The first copy occurs before either optional Begin; a first-Begin failure does not prevent copy2 or the second Begin. If only the second Begin succeeds, its texture/blend can be unrelated inherited state.

### CopyEFB exact argument and offset path

Let Q be the passed raster, R=Q.root, X=R+pluginOffset, `x=s16(Q+1C)`, `y=s16(Q+1E)`, and W=R.width. All shifts/products/adds below use native wrapping 32-bit words; ASR is arithmetic right shift of the wrapped result.

| Root depth / raw dispatch entry | Byte offset added to X+1C |
|---|---|
| 4: `8056F544 -> 804962E4` | `ASR32(8*x + align8(W)*y,1)` |
| 8: `8056F554 -> 8049630C` | `4*x + align8(W)*y` |
| 16: `8056F574 -> 80496330` | `2*(4*x + align4(W)*y)` |
| **32: `8056F5B4 -> 80496358`** | **`4*(4*x + align4(W)*y)`**; blur RGBA8 path |
| Other: `80496380` | Error helpers 8047EC78/8047EBD4, code8000000C; return without copy/filter restoration/invalidation |

Selector0 uses low16 Q origin/width/height for `80396C50 GXSetTexCopySrc` at **8049629C**. Nonzero selector doubles each source word before low16 conversion at **80496274**; blur never requests it. **804962B8 -> 80396D00 GXSetTexCopyDst** receives low16 R dimensions, X.format and low8 selector. This does not arbitrarily rescale into root dimensions.

GX source caches **BP49/4A**, 10-bit coordinates and width/height-minus-one. RGBA8 destination stride in **BP4D** is `(2*ceil(max(u16(W),1)/4))&0x3FF` in 32-byte units; selector bit0 enters copy-control bit9. **804963B8 -> 803975AC GXCopyTex** emits BP49/4A/4D/4B/52; BP4B is `(destination>>5)&0x1FFFFF`, clear controls bit11, bit14 is forced0 for texture copy. This is **EFB-to-texture RAM**, not XFB history.

Copy first calls **80397228(0,0,0,0)** at **80496248**. Its disabled sample words are **01666666/02666666/03666666/04666666**; disabled vfilter taps are **[0,0,21,22,21,0,0]**, BP **53595000/54000015**. It is not a fabricated zero-tap copy. After copy, **804963BC -> 80396568 GXPixModeSync** writes pixel-mode BP state, not a CPU completion wait. **804963D4** restores `(mode.byte19,mode+1A,1,mode+32)` filtering — mode-derived, not saved entry state. Default modes contain taps `[8,8,10,12,10,8,8]`, but custom modes exist. Finally X+2C selects region invalidation **8039868C** or all invalidation **803987B8**; the all path emits **66001000/66001100**.

## 6. Projection, viewport and exact conditional restoration

Im2D reads **mode width u16(+4), height u16(+8), not +6**. It requests viewport `(0,0,Wmode,Hmode,0,1)`. If mode+18 is nonzero it calls **80380004 VIGetNextField**, XORs1, and passes that field to **8039A444**; SDK jitter subtracts0.5 from Y only for field0. Otherwise it calls **8039A49C**. There is no extra manually added jitter.

Persistent projection **8056F13C** initially has `[1,1,-1,1,1,-1,-1]`; only its width/height coefficients change, producing:

```text
[type=1, 2/Wmode, -1, -2/Hmode, 1, -1, -1]
position matrix 8056F158 = diag(1,1,-1) with tx=S(view.x)+0.5, ty=S(view.y)+0.5
```

**804916EC -> 8039A1E4** snapshots the incoming projection into **805E4218**; **804916F8 -> 8039A158** installs the vector; **80491754/75C -> 8039A22C/8039A2CC** load/select matrix0. This is a current screen-space projection, not `C_MTXOrtho` with guessed arguments, nor a previous-camera matrix. Camera-derived **805E428C** is also not temporal history.

Restore **80491774**, called at **80492438**, always reinstalls 805E4218 at **80491AF0**. If camera raster equals its root, that is **all** this restore does. For views, let `x,y,w,h` be their signed-origin/raw extent words, `m=mode.width`, `b=u32[805F26F0]`; comparisons below interpret wrapped expressions as signed32:

| Condition | Viewport / scissor requests |
|---|---|
| `u32[805F2698]==0` | Viewport `(S(x),S(y),S(w),S(h),0,1)` with ordinary or field-jitter branch; scissor `(x,y,w,h)` at 804918BC. |
| `u32[805F2698]!=0` | Normal viewport doubles **integer Y and height before conversion**; field-jitter branch does **not** double them. Scissor follows the split branches below. |
| Split selector `u32[805EEFD8]!=0`, `2*(y+h)<=b+2` | Scissor `(x,2*y,m,2*h)` at 80491A10. |
| Same selector, otherwise `2*y>b+2` | Scissor `(0,0,m,b+2)` at 80491A34. |
| Same selector, remaining case | Scissor `(x,2*y,m,b+2)` at 80491A48; **height is b+2, not b+2-2*y**. All three set scissor-box offset `(0,0)`. |
| Split selector0, `2*y>=b-2` | Scissor `(x,2*y,m,2*h)` at 80491A88. |
| Selector0, otherwise `2*(y+h)<b+2` | Scissor `(0,b-2,m,b+2)` at 80491AB8. |
| Selector0, remaining case | Scissor `(x,b-2,m,2*h)` at 80491AD4. All selector0 cases then set box offset `(0,reload(b)-2)` at 80491AE4. |

This is reconstructed conditional camera viewport/scissor, **not a complete saved GPU-state restoration**. Original reloads and signed/wrapping comparisons are significant; simplifying these branches into a conventional clipping rectangle changes arguments.

## 7. State inheritance and verification boundary

| Category | Established by these paths | Not established |
|---|---|---|
| Geometry/TEV | Direct XYZ/RGBA/ST, one strip, channel/texgen/TEV counts, stage0 program/order | Universal direct/indirect reset, identity swaps/tables, automatic coordinate-scale defaults |
| Sampler | Draw filter1; real table/LOD paths; binding H or S | Save linear filter, clamp wrap, arbitrary sampler validity |
| Tests/masks | Draw cached depth/fog requests; alpha-bit-dependent cached test realization | Cull-none, always-pass alpha, color/alpha writes enabled, benign dither/EFB format |
| Restoration | Six implemented RW restore requests, same-call projection and conditional viewport/scissor | Original raster binding, matrix0/current-matrix, descriptors/channels/TEV, all cached fog planes or all GX state |
| Capture | Two texture-copy submissions, real filter/sync/invalidation | Pixel-clear=false invariant, CPU-readable completion, bit-identical EFB preservation |

The **23 passing byte-grounded synthetic tests** include 90 geometry, 1,960 copy-offset, 144 ramp, 260 alpha, 640 progress and 240 interpolation fixtures plus dispatch/GX/TEV negative controls. They do not execute C++ or the GPU. Strict PPC syntax and default/O2 LLVM IR pass; installed LLVM cannot generate PPC objects. Integrated scope assertions are explicitly non-original, not fallback renderer behavior. Pixel/visibility/timing claims remain **UNVERIFIED** without the live state and trace identified in [unknowns](motion_blur_unknowns.md).

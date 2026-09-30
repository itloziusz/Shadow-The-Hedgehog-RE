# Original `main.dol` motion blur

## Result and scope

**This is a retained-screen-image feedback effect with a centered scale, not velocity-buffer, depth-reprojection, or per-object motion blur.** The normal registered pipeline composites the last saved image at **`0x8042E254`**, then replaces that history later through **`0x8042E838`**. One Draw submits one textured four-vertex strip; there is no intra-call radial-tap loop.

Target: **5,773,024 bytes**, SHA-256 **`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`**. All addresses below are original local virtual addresses, not relocated reference-map symbols. Ranges are end-exclusive; sizes/offsets prefixed `0x` are hexadecimal. The DOL, `motion_blur_analysis/`, and `GPT_SOL_analysis/` remain unchanged.

The current evidence contains **121 original function ranges / 80 data records**, verified against **10,075 instruction words / 3,908 file-backed data bytes**. The integrated readable source is **2,393 CPP lines / 456 header lines**, covering all 19 core/task entries and explicitly scoped backend/input definitions. **The original subsystem is statically analyzed; a standalone rebuilt/linked/executed replacement is not complete.** Source presence, selected-path scope and verification are distinguished in questions 12–13.

### Analysis documents

| Document | Content |
|---|---|
| [README](README.md) | Thirteen answered subsystem questions, cross-check matrix and artifact provenance |
| [Call graph](motion_blur_callgraph.md) | Recurring loop, task order, direct sites and installed indirect edges |
| [Functions](motion_blur_functions.md) | All 121 assembly-linked ranges; complete core metadata and source/API scope |
| [Globals](motion_blur_globals.md) | Owned layouts, image buffers, live inputs and lifetimes |
| [GX state](motion_blur_gx_state.md) | Numeric RW/GX tables, geometry, copy, texture, matrix and state inheritance |
| [Data flow](motion_blur_dataflow.md) | Immediate inputs through capture/composition to next eligible history |
| [Temporal state](motion_blur_temporal_state.md) | Normal, disabled, failed, skipped and multicamera cases |
| [Original addresses](motion_blur_original_addresses.md) | Complete 121-function / 80-data-record VA/end/size/file-offset atlas |
| [Unknowns](motion_blur_unknowns.md) | Genuine conditional/live/SDK/ABI/runtime limits, not closed table hypotheses |

Primary bytes: [address manifest](evidence/address_manifest.json), [data manifest](evidence/data_manifest.json), [range specification](evidence/function_manifest.json), [original assembly](evidence/original/). Source: [implementation](reconstructed/motion_blur.cpp), [header](reconstructed/motion_blur.h). [03](evidence/03_closure.md), [04](evidence/04_closure.md), [05](evidence/05_closure.md) and [12](evidence/12_closure.md) provide the final top-down, bottom-up, lifecycle and input closure.

## 1. Where is the original implementation, and how was it identified?

The effect-specific contiguous core is **`[0x8042E254,0x8042F258)`**, `0x1004` bytes. Its draw/save functions are respectively `0x5E4` and `0x18C` bytes. The rest comprises construction, parameter transfer, manager bridges, and destruction, not additional blur kernels.

```text
DrawBlur task:   0x801D27FC -> getter 0x800A35D8 -> 0x8042F16C -> 0x8042E254
SaveScreen task: 0x801D2764 -> getter 0x800A35D8 -> 0x8042F140 -> 0x8042E838
                                                manager+0x40 -> blur state
```

This identification combines constructor-installed names (`0x804D0F18 = DrawBlur`, `0x804D0F24 = SaveScreen`), vtables (`0x8053DC84 -> 0x801D27FC`, `0x8053DC68 -> 0x801D2764`), actual branches, allocations, and rendering semantics. **`0x801D280C` is a call instruction, not a function entry.** The map has different offsets in different regions; its misleading aliases are not adopted. [Evidence: 01](evidence/01_binary_mapper.md), [02](evidence/02_powerpc.md), data manifest.

## 2. What algorithm and buffers does it use? Which parts are shared?

Let `M` be the manager, `B = M+0x40`'s pointee, `H` the retained half-size image, and `S` the manager-owned full-size scratch image:

```text
current EFB + sample(previous H, centered scale) -> composited EFB    Draw 0x8042E254
later EFB -> S -> half-size EFB quad -> same H -> full-size S redraw Save 0x8042E838
```

`B+0x74` points to a raster **view**, whose first word points to the owning history root. This view/root pair is **one backing image**, not ping-pong. `M+0x34` points to the separate scratch view/root pair. Scratch is shared with a neighboring post-effect and overwritten at each qualifying save; it is not a second blur history.

The game-specific core chooses parameters, geometry, history, and call order. RenderWare camera/raster/state/Im2D functions and Nintendo GX copy/texture/FIFO functions are **shared dependencies**, not motion-blur algorithms. The index scopes their required paths instead of presenting a whole-game/whole-SDK decompilation. No modern renderer or Sol draft replacement has been substituted.

## 3. When does it run? What happens with multiple cameras?

The recurring event loop **`0x800510C0`** calls `0x80051978` with event18; dispatcher `0x80048EB8`, through word **`0x8051E628 -> 0x80049078`**, reaches frame driver **`0x801E2210`**. Its ordinary call **`0x801E23A4 -> 0x801EB680 -> 0x8004ECAC`** traverses the common root; group5 is actually named **Render**. [03_closure](evidence/03_closure.md) closes this path, including initialization/transition alternatives.

`0x801E4B0C` constructs and tail-registers Render's phase roots in order **Start (+0x5C), Div (+0x60), Static (+0x64), End (+0x68)**. `0x8004F014` appends tasks; `0x8004ECAC` executes a task, recursively visits its children, then follows siblings. The profile path calls the same Exec slot. Low flags can skip/delete; the pause path can suppress an Exec yet recurse, and these render/blur tasks set the relevant **0x100** bypass.

- Draw constructor `0x801D2058` attaches under render-level manager **+0x2C**, index **11**. Record `0x804D25C8` names **PostGlare** (`0x804D23D8`); selector **`u32[0x804D25D8] = 1`** places it under **RenderAttach_Div (+0x60)**. It is not GlareWorld, which is index 9.
- Save constructor `0x801D20D8` attaches under **+0x58**, the specially created **early PostEffect** child of **RenderAttach_Static (+0x64)**. Ownership by Draw does not make Save its scheduling child.
- Thus the normal draw precedes save, including intervening Div rendering. Save precedes the later DrawScreenColor and Static sprite/fade/UI levels; it does **not** necessarily capture the final presented image.
- Div's actual vtable slot **`0x8053EA7C -> 0x801E4EF0`** selects views. Modes **1/2** execute children internally for view **1/3**, then select view **2/4**; the outer walker visits children again. **Draw can execute twice before Save**, using the same old H. Mode 0 has no extra internal traversal.
- Static's slot **`0x8053EA60 -> 0x801E4E40`** resets selectors to view 0 before its children. There is no per-camera history array or camera-ID validity key.

This is a static registered-order proof, not one traversal per VI field/frame. Task skip/delete flags, live mode `0x8057E7C0`, and manual/runtime traversal changes matter; initialization/transitions can also traverse extra times. [03_closure](evidence/03_closure.md), [08](evidence/08_temporal.md), [14](evidence/14_adversarial.md), and the latest raw phase vtables close the earlier main-loop/selector/slot gaps.

## 4. Exactly how is history captured and the framebuffer restored?

`0x8042E838` first writes **`B+0x78 = 0`**, then checks enable, alpha, provider/camera availability. On the qualifying path:

| Original call/store | Operation |
|---|---|
| `0x8042E8C0 -> 0x80496208(Sroot,0)` | Copy full current EFB to scratch root. |
| `0x8042E8C8`, then `0x8042E954` | If BeginUpdate succeeds, bind Sroot, choose RW ONE/ZERO (`2/1`), draw the **half-size** quad at `B+0x7C`, end update. |
| `0x8042E968 -> 0x80496208(Hview,0)` | Copy the temporary reduced EFB rectangle into the same history root. |
| `0x8042E970`, then `0x8042E99C` | If BeginUpdate succeeds, draw the **full-size** quad at `B+0xDC`, end update. No new texture bind occurs here. |
| `0x8042E9AC` | Set **valid = 1**, even if either or both optional BeginUpdate calls failed. |

Both copy-helper arguments are **selector 0, not clear=false**. `0x80496208` independently loads **`low8(u32[0x805F2688])`** for **GXCopyTex `0x803975AC`**. Selector 0 uses the view's stored source rectangle and no hardware half-scale request; the quad provides the size reduction.

The copy helper uses root dimensions/format and root-plugin pixel base plus a tiled offset. For depth 32, the byte offset is `4 * (4*x + align4(rootWidth)*y)`, using original 32-bit arithmetic and signed 16-bit origins. It disables AA/vfilter via `0x80397228(0,0,0,0)`, copies, calls **GXPixModeSync `0x80396568`**, restores the mode's copy filter, and invalidates the relevant texture region/all. The SDK's disabled-vfilter branch itself packs `[0,0,21,22,21,0,0]`; do not replace it with an invented pixel-copy filter.

Both nonempty `0x505` raster roots follow the nonpaletted **GX format 6 (RGBA8), depth 32, alpha flag 1, no mipmaps** path through `0x80497CA8 -> 0x80497458`. Scratch's initially empty view is created with flags **5**, then inherits its parent's RGBA8 format on subraster attachment; it has no separate pixels. For positive W/H without overflow, `0x80496478` gives payload **`align32(4*align4(W)*align4(H))`**, and `0x80497F48` requests payload+31 bytes; plugin+0x18 owns the allocation and +0x1C holds its 32-byte-aligned pixel address. `0x80372508` invalidates cache, **not image contents**.

These are GPU-tiled runtime textures, not DOL texture assets or XFB history. The exact copy-to-sampling chain reaches the same pixels through `0x8049AC88 -> 0x80397E34 -> 0x8039843C/0x803982C0`; [05_closure](evidence/05_closure.md) follows the emitted BP address. Restoration is a sampled redraw, **not guaranteed bit-identical EFB preservation**, and the valid byte is not a fence or success result.

## 5. How is the saved image composited? What is the effective weight?

Draw `0x8042E254` checks enable and nonzero alpha, resolves/casts the provider, obtains a camera, then checks valid. It snapshots RW IDs **10,11,12,6,8,14,9**, computes geometry, and begins the camera. On successful begin it requests:

| RW ID / value | Actual local backend meaning |
|---|---|
| `1 = Hroot` | Bind history raster through `0x80498954 -> 0x8049819C`; no saved raster binding. |
| `10 = 5`, `11 = 6` | Blend lookup `0x8056FA28` maps to **GX 4/5 = SRCALPHA/INVSRCALPHA**. Save's RW 2/1 maps to GX ONE/ZERO 1/0. |
| `12 = 1` | **Unsupported**. Getter table `0x8056FA88 -> 0x804986B0` returns 0 without writing output; setter `0x8056FB04 -> 0x80498F58` has no state/GX effect. |
| `6 = 0`, `8 = 0` | Cache-mediated depth test/write disable; combined GX request is **`GXSetZMode(1,7,0)`** (enabled comparison ALWAYS, writes off), not literal test-enable 0. |
| `14 = 0` | Fog disabled. |
| `9 = 1` | **Texture filter**, not culling or shading. Verified table maps it to GX nearest min/mag `(0,0)`. |

Submission at **`0x8042E738`** is `(primitive=4, vertices=B+0x14, count=4)` through `0x80491E80`; table **`0x8056F130 = 0x98`** selects GX triangle strip, VAT 0. Direct FIFO records are xyz F32, RGBA8, uv F32, stride `0x18`.

Im2D `0x8049148C` establishes one textured TEV stage, texture unit 0, raster color and alpha from the vertices. `GXSetTevOp(0,0)` at `0x803992EC` selects **color `0xC008F8AF`, alpha `0xC108F2F0`** from `0x80568B00/+0x28`, preserving the old alpha word's low four swap-selector bits. With identity swaps and otherwise ordinary direct TEV operation:

```text
Csrc = Ctex * white;  Asrc = Atex * (alphaByte / 255)
Cout = Asrc * Csrc + (1 - Asrc) * Cdst
```

This is an idealized equation conditional on tests/coverage/write enables; GX fixed-point/EFB quantization is additional. **128 does not unconditionally mean 128/255 history weight**: captured texture alpha participates. RGBA8 does not force Atex=1.

Draw issues **six implemented restore requests plus ignored ID 12**, not seven effective state changes, and restores **neither texture ID 1 nor all GX state**. The ID-12 saved word is untouched stack residue, later loaded for an ineffective restore; source uses explicit PPC loads rather than inventing a value. Save does not perform this requested-state snapshot/restore at all.

## 6. What are the exact parameters and persistent state?

`B` is **0x13C bytes**; its parameter prefix is **0x10 bytes**:

| Offset | Meaning |
|---|---|
| `+0`, `+1` | Separate raw enable and alpha bytes. Noncanonical nonzero enable values are not normalized by copies. |
| `+2..3` | Untouched padding; not transferred by parameter helpers. |
| `+4` | f32 centered geometry scale. |
| `+8`, `+0xC` | Copied raw words, interpolated as floats by a producer, but **no draw/save GPU consumer**. No justified center/velocity meaning. |
| `+0x10` | Borrowed manager pointer. |
| `+0x14..0x73` | Four composite vertices, each `0x18` bytes. |
| `+0x74`, `+0x78` | Owned history-view pointer and separate valid byte; `+0x79..0x7B` untouched. |
| `+0x7C..0xDB`, `+0xDC..0x13B` | Four half-size capture vertices and four full-size restore vertices. |

`0x8042E9C4`, `0x8042E9F0`, `0x8042EA1C` transfer only fields **0/1/4/8/C**, not a whole-record memcpy. `0x8042F084` takes output storage in r3, manager in r4. Setters `0x8042F0E8/F114` take manager in r3, source in r4; only F114's underlying helper tolerates null source. None resets history validity.

## 7. Where do enable, scale, and strength come from?

These are immediate game/controller inputs, not values calculated from motion inside the image core:

| Producer | Exact blur-relevant behavior |
|---|---|
| `0x800A3528`, call `0x800A356C` | Submit enable=0, alpha=128, fixed scale. |
| `0x800A3714`, call `0x800A375C` | Submit enable=1, alpha=128, fixed scale. |
| `0x800A47B0`, call `0x800A481C` | Enable from scalar `object+0x38`; alpha = low byte of PPC `fctiwz(fmuls(64,scalar))`; fixed scale. |
| `0x800A4830` | Update that scalar by incoming delta **/0.5**, subtract/add selected by `+0x34`, with ordered clamps at 0 and 1; call `0x800A47B0`, request task deletion on terminal nonpositive scalar. |
| `0x80204714..0x80204760`, `0x802055DC..0x80205628`, `0x80206314..0x80206360` | Get/copy settings, replace only enable with 0, set back. These are slices, not three complete reconstructed functions. |
| `0x80439A8C`, blur block `0x80439EBC..0x80439FE0` | Resource-controller interpolation described below. Finalizer `0x8043A4B8` writes **target**, not saved starting parameters. |

Raw input constants: **`0x805F3BD0 = 0x3F866666`**, the binary32 representation of approximately **1.05**; `0x805F3BD4 = 0.5`, `0x805F3BDC = 0`, `0x805F3BE0 = 64`, `0x805F3BCC = 1`. A full normal ramp therefore reaches alpha **64**, distinct from the fixed path's **128**. Conversion truncates toward zero and stores low eight bits, **not saturation**. Unordered/nonfinite inputs require original PPC compare/conversion semantics; core setters do not clamp scale or alpha.

Ramp construction **`0x800A4AB4`** starts direction+0x34/scalar+0x38 at **zero**, not current blur alpha; its raw Exec slot **`0x80522A84 -> 0x800A4830`** is closed. `0x800A34B0` requests decrease on an existing ramp. For the resource controller, constructor **`0x8043A5A4..0x8043AB68`** obtains resource pointer/duration/source flags from descriptor+0x34/+0x30/+0x2C; `resource+0 == 1` selects blur. Its setup block **`0x8043A900..0x8043A97C`** snapshots start at controller+0x14C and forms target+0x15C: enable from **controller+0xFC bit 16**, alpha from resource+8, scale from +0xC, opaque words from +0x10/+0x14. Progress lives at controller+0x100.

When ordered `t < f32[0x8051CB58]` (**bits `0x3F7FF972`, approximately 0.9999**), enable is start OR target; each float uses **`fmadds(target,t,fmuls(start,fsubs(1,t)))`**, with complement unit **`0x8051CB5C = 1.0`**, not the threshold. Differing alpha bytes follow the same mixing then `fctiwz`/byte truncation. Otherwise target is used directly. `0x8042B63C` supplies progress from elapsed/duration with threshold **`0x8051BFA4 = 0x38D1B717`** (approximately 0.0001) and upper/fallback 1. Active/inhibit/time-advance flags affect it; it is not a speed estimator.

`0x8042B63C` only advances age with execution flag bit1; bit0/bit2 gate activity/inhibition. Overshooting duration+epsilon clears active and **skips that invocation's setter**, rather than forcing target. Finalization writes target only when it actually executes. [12_closure](evidence/12_closure.md) closes all 12 immediate getter/setter references, the fixed/decrease dispatch predicates, and containing ranges of the three disable slices; all three call `0x8042659C` before reading settings. No undocumented boost/speed/cutscene name is inferred from those numeric predicates.

Several fixed/ramp/default paths leave `+8/+C` uninitialized. Resource values, actual delta units/frequency, timer-flag writers and competing-writer order remain caller/live contracts, not missing literal tables. Earlier [09](evidence/09_dataflow.md)/[10](evidence/10_constants.md) constant gaps are superseded by the latest data manifest and role12.

## 8. What screen geometry, UV bias, and direction does it use?

At `0x8042E3C0..0x8042E604`, let `(x,y)` be signed 16-bit current-view origins, `(w,h)` signed current-view dimension words, `(pw,ph)` **unsigned current-camera-parent** dimensions, and `s=f32[B+4]`. S/U below mean the original signed/unsigned conversion sequences; each arithmetic operator retains its PPC single-rounding point:

```text
u0 = (0.5 + S(x)) / U(pw);       u1 = (0.5 + S(wrap32(x+w))) / U(pw)
v0 = (0.5 + S(y)) / U(ph);       v1 = (0.5 + S(wrap32(y+h))) / U(ph)
d  = s - 1;  dx = 0.5 * (d*S(w));  dy = 0.5 * (d*S(h))
TL = (-dx,-dy,u0,v0);            BL = (-dx,S(h)+dy,u0,v1)
TR = (S(w)+dx,-dy,u1,v0);        BR = (S(w)+dx,S(h)+dy,u1,v1)
```

Order is **TL, BL, TR, BR**, not perimeter-order quads. XY is overwritten **absolutely on every call**, never cumulatively expanded. Constants at `0x8051C388/38C` are exactly 0.5/1; signed/unsigned double biases at `0x8051C390/398` are `0x4330000080000000` / `0x4330000000000000`. Preserve integer add-before-convert, signed zero, and separate `fsubs/fmuls/fadds/fdivs`; algebraic reassociation changes bits.

Scale >1 enlarges about the current view's geometric center; scale 1 gives no expansion, scale <1 contracts. There is no directional vector, tunable active center, sample-count parameter, or core speed/camera-delta input. UV denominators are **not** loaded from the sampled history root. Save quads retain construction-time full-range biased UVs; half positions use signed divide-by-two truncated toward zero.

## 9. Which camera and matrices are used? Is any matrix previous-frame data?

Each construct/draw/save resolves the camera independently: **`0x8042B620` reads `0x805E25C8` -> `0x803A1AFC` RTTI cast -> provider vtable+0x24**. The installed provider vtable is **`0x8053E93C`**, whose slot **`0x8053E960 = 0x80028730`**. Method `0x80028730..0x80028754` calls `0x8000DCC0` and returns **word0 of the resulting camera manager**, not the manager itself. The manager is normally **`0x8056FF1C`**, cached at **`0x805EF068`**.

The PJS provider's RTTI **`0x805E9D98 -> 0x8053E920 -> 0x805E9D88`** supplies a name-matched RW3 base at adjustment0; duplicate descriptors are not an unproved cast. **The normal provider/cast/source chain is closed** by [04_closure §5](evidence/04_closure.md); arbitrary later replacement/lifetime and actual selected camera remain separate runtime/caller contracts.

`0x8048652C/0x80486504` call camera+0x18/+0x1C. Default callbacks `0x804863A4/0x80486340`, optionally wrapped by world-plugin callbacks `0x80461B6C/0x80461BB8`, reach installed camera backend `0x804956F4/0x80495CAC`. **Device `0x8056F50C` and standard records `0x8051D400` are separate tables**. Engine open **`0x80487254`** uses system operation4 to copy the device to engine+0x10, then **operation11 (0xB), not 9**, at **`0x804873A4..0x804873BC`** supplies engine+0x48/count29 to installer **`0x804954F8`**. Its 27 records establish camera indices1/10 and raster indices4/5/12. This installation edge is closed in [04](evidence/04_closure.md)/[05](evidence/05_closure.md), not inferred solely from table contents.

Im2D `0x8049148C` uses mode width `u16(mode+4)` and height **`u16(mode+8)`**, not an assumed resolution, to produce projection vector at **`0x8056F13C`**:

```text
[1, 2/Wmode, -1, -2/Hmode, 1, -1, -1]
position matrix 0x8056F158: diagonal (1,1,-1), x/y translation (view origin + 0.5)
```

The projection backup **`0x805E4218`** is saved/restored around the **same Im2D call** (`0x804916EC`, `0x80491AF0`); `0x805E428C` is current-camera-derived state. Neither is a previous-frame motion matrix. The 2D path calls GX projection-vector/matrix APIs, not an invented `C_MTXOrtho` tuple. [07](evidence/07_matrix_camera.md) plus current raw constants.

## 10. How is history updated, invalidated, or reset?

| Situation | Actual `0x8042E254/0x8042E838` behavior |
|---|---|
| Successful normal construction | Enable=0, alpha=128, scale=1, valid=0; allocated history is not seeded/cleared by the constructor. |
| First enabled traversal after invalidation | Draw skips old-history overlay; later Save captures and sets valid. |
| Steady enabled traversal | Draw samples the last qualifying Save; Save replaces that same H. Earlier blended contributions can feed into the next capture. |
| Disable or alpha=0 | Draw skips. A **reached Save** clears valid then exits without erasing old pixels. Setters alone do not invalidate. |
| Disable/re-enable between Saves | Old valid H may be reused immediately; no mandatory warm-up was triggered. |
| Save task skipped | It cannot clear valid; H may be older than one traversal/presentation. Draw never consumes/clears valid. |
| Camera cut/switch/resize | No automatic camera-key reset, matrix reset, or buffer resizing in the core. |
| Destroy/recreate | Old H and B are freed; a successful new construction begins invalid. |

An explanatory recurrence is `Q[n]=Composite(current[n],scaled H[n-1])`, then `H[n]=Downsample(interveningRendering(Q[n]))`. It is **not necessarily the previous unblurred scene or previous displayed frame**. Intervening overwrites and inherited GPU state can remove/change feedback.

## 11. Who owns allocations, and what state/failure contracts must be preserved?

The singleton cache is **`0x805EF2D8`**, normal static manager **`0x8057798C`**; scratch/blur pointer fields are **`0x805779C0/0x805779CC`**. Setup `0x801D1A68 -> 0x80433638` creates shared scratch before `0x801D1A80 -> 0x8042F20C` allocates B. History size uses camera **root** dimensions /2; scratch setup uses the camera **view** dimensions. Their compatibility is a normal setup contract, not universal equality for arbitrary subviews.

`0x8042F198` destroys a distinct history parent, then the view, frees B, and nulls manager+0x40. Stage calls it at `0x801D19B8` before scratch teardown `0x801D19D8 -> 0x804335C8`. Draw's task destructor `0x801D2788` marks its Save task for deferred deletion; it does not itself release H. Backend **`0x80498058`** conditionally emits a token (`0x80396428`), polls completion (`0x8049B7DC`), unbinds a matching current raster (`0x80498138`) and frees the allocation base. [05_closure §6](evidence/05_closure.md) closes this machinery, but no runtime wait duration/safety outcome was measured.

Do not add absent guards/initializers while calling the result exact: failed provider/camera lookup in `0x8042EA50` returns a **partially initialized nonnull B before +0x74/+0x78 initialization**; Create still installs it. Raster-allocation/subraster results are not all checked. Save directly dereferences scratch and its second optional draw inherits the first block's binding/blend only if that block ran.

**Inherited state is explicit, not replaced by defaults.** Save does not choose texture filter/wrap, depth test/write, fog, cull, color/alpha writes, or a universal alpha test. Draw chooses nearest and restores the prior filter; capture therefore need not be bilinear. Alpha-bearing texture flush `0x80498854` uses cached alpha compare and Z-compare-location behavior. TEV swap selectors/tables, indirect/direct-stage state and automatic coordinate-scale state are not comprehensively reset. Initialization defaults do not establish live values at either pass.

## 12. How exact is the machine correspondence, and what was actually verified?

The manifests retain original range hashes, instruction words, VA/file offsets, direct branch sites and raw data. In `.text1`, **file offset = VA - `0x80006740`**; e.g. Draw `0x8042E254 -> 0x00427B14`, Save `0x8042E838 -> 0x004280F8`. Startup `0x8000332C..0x8000333C` establishes **r2=`0x805FA780`, r13=`0x805EC500`**. The encompassing BSS interval overlaps initialized SDA sections; only the actual startup zero ranges are zero-backed. The atlas lists **61 file-backed and 19 zero-backed data records**; no live heap/pointer value is inferred from a startup zero.

| Check | Actual result / boundary |
|---|---|
| `python -B analysis/motion_blur/tools/dol_evidence.py verify` | **PASS:** 121 original ranges, **10,075 instruction words**, **3,908 file-backed data bytes**, 80 data records. Whole-DOL SHA-256, per-range hashes, offsets, every exported word and direct branch decoding agree. This authenticates bytes, not all possible indirect calls or source equivalence. |
| `python -B -W error analysis/motion_blur/tools/test_motion_blur_semantics.py` | **23 tests PASS:** 90 geometry, 1,960 copy-offset, 144 ramp-step, 260 finite alpha-conversion, 640 progress and 240 interpolation fixtures; state-dispatch, GX argument, TEV swap-bit, literal and negative controls. These are **byte-grounded synthetic checks, not C++ execution, game execution or GPU output**. |
| Integrated PPC syntax/layout | **PASS**, Clang21.1.8, freestanding PowerPC C++17, no fast-math/contraction, strict warnings; current CPP2,393/header456 lines. |
| Default and `-O2` PPC-target LLVM IR | **PASS** per [verification ledger](evidence/verification_ledger.json) and [audit resolution](evidence/source_audit_resolution.md). Final retained artifact: **[motion_blur_verified.ppc.ll](evidence/motion_blur_verified.ppc.ll)**. IR is frontend output, not machine code. |
| Constructor first-full-vertex audit | **Resolved:** original `0x8042EC24..0x8042ECB4` and optimized IR match **24/24 object-load/store and FP events**. Height division precedes first blue/alpha stores; no active old FP-interleaving finding remains. Original registers/SP/exception resumption are not reproduced by that check. |
| PPC machine object | **BLOCKED:** actual object attempt failed because installed LLVM has no PowerPC machine-code backend; `--print-targets` confirms it. No object/assembly-expansion/register-allocation proof. |
| Linking, original binary rebuild, game/FIFO/pixels | **UNVERIFIED / not performed.** Real SDK/ABI address bindings are external; no captured game/runtime/GPU trace or rebuilt runnable image is available. |

Reproducible strict syntax invocation from `sys/`:

```text
"C:/Program Files/LLVM/bin/clang++.exe" --target=powerpc-unknown-eabi -std=c++17
  -ffreestanding -fno-exceptions -fno-rtti -fno-fast-math -ffp-contract=off
  -Wall -Wextra -Wpedantic -Werror -fsyntax-only analysis/motion_blur/reconstructed/motion_blur.cpp
```

PPC load/store/arithmetic helpers retain observed operations where host C++ could change rounding or evaluate indeterminate typed values. They are not proof of identical original compiler stack residues, NaN/trap state, CPU registers or mixed GPR/FPR ABI. Nonfinite/out-of-range FCTIWZ and precise exceptions are not fabricated by the synthetic interpreter. [Semantic verification methods](evidence/semantic_verification_extra.md) state the modeled domains and negative controls.

## 13. Is the requested reconstruction/rebuild complete? What is delivered now?

**All nine analysis documents and the integrated core/backend/input source are present. A standalone rebuilt, linked or executed replacement is not complete.** The scope distinction is substantive: original byte coverage, complete source bodies, selected source slices, SDK declarations and runtime validation are different results.

| Layer | Current readable-source/evidence coverage |
|---|---|
| Core `0x8042E254..0x8042F258` | All **13 original functions**, plus all **six task entries**, complete body mappings. Core/task block **CPP1–808**. |
| Backend | **13 address-bearing definitions = 4 complete + 9 selected**, integrated at **CPP810–1814**. Complete: CopyEFB, SetRaster, Im2D conditional restore, BackendSubRaster. Selected: eight-ID get/set, RGBA8 flush, textured prepare/strip4, nonpaletted no-mip realization, format/create and level0-size. |
| Input/controller | **13 address-bearing definitions = 2 complete + 11 selected**, integrated at **CPP1816–2393**. Complete ApplyRamp and UpdateProgress; selected fixed prefixes, three ramp update blocks, kind1 construct/apply/finalize and three disable prefixes. |
| Shared boundaries | Camera/raster wrappers, scratch/manager/task lifecycle, RTTI, allocations and GX SDK retain real original addresses/contracts. Declarations do not implement or bind them. |
| Original evidence | **121 complete exported function ranges / 80 data records**, including full containing functions where source covers only selected blocks. Instruction count is not source-completion count. |
| Binary/runtime | No PPC object, linked extraction, original compiler reproduction, game execution or GPU validation. |

**Do not sum definition counts as distinct original functions:** the three ramp source slices are all within `0x800A4830..0x800A492C`. Source `input::Continuation` values describe omitted arms/tails; they are not original r3 returns or executable PC-resume/stack-register machinery. Interleaved non-blur side effects still belong to the containing function. Backend `require_extracted_path` / `__builtin_trap` at **CPP1100–1103** is an explicitly **non-original domain assertion**, not an original error path or invented fallback. No selected path is installed as a generic callback. Only CopyEFB supplies a definition under the core's existing `original::` declaration.

### Artifact identity and provenance

| Artifact | Current identity / role |
|---|---|
| Original `main.dol` | 5,773,024 bytes; SHA-256 **fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af**, unchanged |
| [CPP](reconstructed/motion_blur.cpp) | **2,393 lines**, SHA-256 **3d2cb9dca2a2b4f220adfb5c16163d9f8e61229b44d89f701a20564261b8ad1b** |
| [Header](reconstructed/motion_blur.h) | **456 lines**, SHA-256 **c1aefedcd422b5f16a14c4f461462c81725a6092fb17144b1966df8001328534** |
| [Verification ledger](evidence/verification_ledger.json) | Current manifest, semantic tests, source/IR, object blocker and unavailable runtime results |
| [Source integration](evidence/source_integration.md) | Historical 2,372-line source coverage report. Its old CPP lines498 onward shift **+21** in the current source; original addresses remain authoritative. |
| [Final source audit](evidence/final_source_audit.md), [resolution](evidence/source_audit_resolution.md) | Historical counterexample and its closed first-vertex finding; scope-assertion, continuation and fault-ABI limitations remain explicit |
| [Backend fragment](evidence/backend_reconstructed.inc), [notes](evidence/backend_source_notes.md) | Preserved 1,005-line integration input and instruction ledger; already included in CPP, not a second include requirement |
| [Final IR](evidence/motion_blur_verified.ppc.ll) | Verified PowerPC-target frontend artifact; older [core IR checkpoint](evidence/reconstructed_core.ll) is not the current integrated verification artifact |
| [Synthetic suite](tools/test_motion_blur_semantics.py), [method report](evidence/semantic_verification_extra.md) | 23 byte-grounded tests, documented floating domains and negative controls, not C++ or hardware execution |
| [Sol cross-check](evidence/sol_crosscheck.md) | Independent interpretation of the first31 exports; useful agreements and instruction-proven draft errors, not independent DOL acquisition or a runtime trace |

### Cross-check matrix: major claims and independent roles

The role columns identify independently examined aspects of the claim, not fifteen independent acquisitions of the same binary. Current raw exports/verification supply common byte provenance. Where one role initially lacked a table, the closure/raw record is the decisive evidence, not its earlier hypothesis.

| Major claim / original anchors | Independent role evidence | Cross-check result and boundary |
|---|---|---|
| Local identity/VA mapping; `801D27FC -> 8042F16C -> 8042E254`, Save pair | [01 mapper](evidence/01_binary_mapper.md), [02 PPC](evidence/02_powerpc.md), [15 integration](evidence/15_integration.md), [04 bottom-up](evidence/04_closure.md) | Names/constructors/actual branches and GPU convergence agree; map offsets are not universal. |
| Draw before Save; record11 selector804D25D8=1; common-root traversal | [03 top-down](evidence/03_closure.md), [08 temporal](evidence/08_temporal.md), [14 adversarial](evidence/14_adversarial.md) | Append/recursion and subtree proof agree; role03 supplies the complete recurring event route and raw phase-slot closure. Not a VI cadence measurement. |
| Div can visit twice before Static | [03](evidence/03_closure.md), [08](evidence/08_temporal.md), [14](evidence/14_adversarial.md) | Independent body reads agree; current vtable words resolve the installed Exec link. One shared old H, not two camera histories. |
| Installed state/camera/raster paths; op11/count29 | [04](evidence/04_closure.md), [05](evidence/05_closure.md) | Independent engine-open/installer reads converge; SubRaster standard12 is engine+78, not94. |
| Provider vslot24/cast/source | [09](evidence/09_dataflow.md), [04](evidence/04_closure.md) | Role09 identifies construction/slot use; role04 raw RTTI/name/slot and manager-field0 reads close its old gap. Actual selected C remains live. |
| Single H allocation, S separate; no ping-pong; view/root dimension distinction | [09](evidence/09_dataflow.md), [05](evidence/05_closure.md), [04](evidence/04_closure.md) | Independent pointer/lifecycle reads agree; RGBA8 tiles/owner-versus-aligned pointer are raw-backed. |
| Two copies/two optional quads; clear independent; valid is not success | [02](evidence/02_powerpc.md), [05](evidence/05_closure.md), [08](evidence/08_temporal.md), [14](evidence/14_adversarial.md) | Actual call/store order and failure branches agree; no synchronous completion/lossless restoration claim. |
| RW9 filter, RW12 unsupported, GX4/5 and inherited alpha/tests | [06](evidence/06_render_state.md), [04](evidence/04_closure.md), [10](evidence/10_constants.md) | Raw dispatch/blend/TEV/filter tables supersede unread-data hypotheses; six implemented restore requests, texture alpha participates. |
| Biased UVs, absolute TL/BL/TR/BR, current signed/unsigned domains | [02](evidence/02_powerpc.md), [07](evidence/07_matrix_camera.md), [10](evidence/10_constants.md), [11](evidence/11_decompiler.md) | Independent instruction formulas plus raw constants and90 geometry fixtures; no velocity or cumulative expansion. |
| Same-call projection, mode height+8, no previous-camera matrix | [07](evidence/07_matrix_camera.md), [06](evidence/06_render_state.md), [04](evidence/04_closure.md) | Current projection/matrix and backend calls agree; exact conditional restore is not all-state restoration. |
| Field-copy ABI, direct producers, ramp/FMA/target finalizer | [09](evidence/09_dataflow.md), [11](evidence/11_decompiler.md), [12](evidence/12_closure.md) | Getter/setter references and arithmetic agree; role12 and synthetic controls correct role11's old unordered upper-clamp interpretation. No gameplay-label inference. |
| Indeterminate auxiliary/stack storage, partial construction | [02](evidence/02_powerpc.md), [11](evidence/11_decompiler.md), [14](evidence/14_adversarial.md) | No memset, fake defaults or success guarantees; the later specific source event-order issue is resolved separately, not generalized to fault equivalence. |
| Runtime/standalone rebuild unavailable | [13 runtime](evidence/13_runtime.md), [14](evidence/14_adversarial.md), [15](evidence/15_integration.md), current ledger | Static predictions are not observations; current source/IR verification does not manufacture object/link/GPU validation. |

All **15 numbered roles have saved reports** (01–15 in the links above). Roles01/02 independently obtained the DOL hash but had some decode commands denied; roles07/08/09/10/11/13/15 used bounded cached listings and/or parent-verified raw data to differing extents. Roles03/04/05/12 performed hash-checked closure queries at their recorded historical snapshot. **Not every role independently hashed the DOL or reviewed the final CPP.** The current ledger and fresh verifier govern the 121/80 totals, not old 31/42/81-range checkpoints. Earlier reports and Sol remain provenance records, unchanged; their closed unknowns are not reintroduced into the final analysis.

The residual contracts are [SDK/ABI binding, live state/arbitration, indeterminate/fault behavior and absent runtime evidence](motion_blur_unknowns.md), not a missing original blur equation.

# Motion blur — input, image and feedback data flow

## Scope and notation

The original effect is a **scaled retained-screen-image overlay**, not velocity reconstruction or per-object/depth reprojection. Draw **8042E254** consumes the last retained image; later Save **8042E838** overwrites that same image. This document traces both control and pixel dependencies through the required original routines. Target SHA-256: `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`. Addresses/offsets are hexadecimal, ranges end-exclusive.

Evidence: [09 dataflow](evidence/09_dataflow.md), [12 input closure](evidence/12_closure.md), [03 scheduling closure](evidence/03_closure.md), [04 bottom-up closure](evidence/04_closure.md), [05 image lifecycle](evidence/05_closure.md), and the current [manifests/atlas](motion_blur_original_addresses.md). The old provider, RTTI, callback-installation and literal gaps are closed; actual runtime state remains distinct from file-backed bytes.

## 1. End-to-end graph

```text
Immediate fixed control              Ramp task                 Resource controller
 enable/disable branch         direction +34, strength +38    descriptor/resource, age/flags
 (0/1,128,1.05,?,?)            delta / 0.5, alpha low8          start/target, t, fused mix
            \                         |                         /
             \---- 8042F114 ----------+---- 8042F0E8 ------------/
                           B parameter prefix 0/1/4/8/C
                                  ^
  three numeric disable blocks -> getter/copy/enable=0/setter
            (after real 8042659C side effect; three manager lookups)

current camera/provider -----+                       B+78 valid latch
current view/root ---------->| Draw 8042E254 <---------+
 B+0 enable, B+1 alpha -------|    |
 B+4 scale ----------------->|    +--> current XY/UV + white/alpha vertices
 B+8/+C: no GPU consumer      |                |
                             |    sample retained Hroot pixels
                             +------> one scaled translucent strip on current EFB
                                            |
                           intervening registered rendering
                                            |
                             later Save 8042E838 (clears valid first)
                                            |
 current EFB -> scratch Sroot pixels -> optional half-size S quad onto EFB
                                            |
                        EFB -> same Hroot pixels through Hview
                                            |
                       optional full-size S quad restores EFB
                                            |
                       valid=1 at reached tail (not GPU success)
                                            |
                     NEXT ELIGIBLE Draw can sample that H
```

There is **one history allocation**, no ping-pong, no previous-camera matrix, and no history consumed/reset by Draw. The arrow from Save to “next eligible” is a software dependency, not a claim that exactly one VI frame elapsed or that the GPU completed successfully.

## 2. Concrete pointer provenance

| Flow | Original pointer chain / producer |
|---|---|
| Effect manager | `800A35D8 -> [805EF2D8]`, normally static **M=8057798C** |
| Blur object | `B=ptr32[M+40]`, allocation0x13C via `8042F20C -> 803A1380` |
| Scratch | `B+10 -> M+34 -> scratch view -> Sroot`; created by `80433638` before B |
| History | `B+74 -> Hview -> Hroot`; created/attached by `8042EA50` |
| Camera source | `8042B620 -> [805E25C8] -> 803A1AFC -> provider+24` |
| Installed provider | Vptr8053E93C; **[8053E960]=80028730**, calls **8000DCC0**, returns **camera-manager field0** |
| Camera manager | Cached at **805EF068**, normal body **8056FF1C**; field0 is C, not the manager itself |
| Current view/root | `V=ptr32[C+60]`, `P=ptr32[V+0]` |
| Pixel base | `X=root+u32[805F2700]`, owner allocation at X+18, aligned pixels at X+1C |
| Sampling | `T=ptr32[805F270C]`, bind T+0 to Hroot or Sroot; texture plugin at `T+u32[805F2718]` |

PJS RTTI **805E9D98 -> 8053E920** includes RW3 descriptor **805E9D88** at zero adjustment; its name matches core target **8056BCE4**, name8051C318. Duplicate descriptor addresses do not prevent the name-based cast. Constructor, Draw and Save each query the then-current provider/camera independently; no persistent camera key couples them.

Camera+18/+1C callbacks route through **8048652C/80486504**, default **804863A4/80486340**, optionally world wrappers **80461B6C/80461BB8**, to installed engine+4C/+70 = **804956F4/80495CAC**. Device installation operation4 and standard installation **operation11**, 27 records at8051D400/count29, close these indirect edges. They are not invented “get active camera” callbacks.

## 3. Parameter ABI and transport

P has extent0x10: **u8 enable@0, u8 alpha@1, f32 scale@4, raw words@8/C**; +2/+3 are padding. Helpers **8042E9C4, 8042E9F0, 8042EA1C** transfer only those five fields, in their actual load/store orders. They do not copy 16 bytes wholesale, normalize raw enable, clamp alpha/scale, allocate, or change valid/history.

- **8042F084(out=r3,M=r4)** reads B if present. Absent B: enable=alpha=0, scale=1, auxiliary output words copied from unwritten stack storage.
- **8042F0E8(M=r3,P=r4)** ignores absent B, but does not guard source once B exists.
- **8042F114(M=r3,P=r4)** additionally tolerates null source through EA1C. Neither wrapper guards null M.
- Supplied constructor parameters go through EA1C at **8042F048**. Successful null-parameter completion sets enable0/alpha128/scale1, leaving auxiliaries/padding unwritten.

B+0/B+1 gate both passes; B+1 becomes each Draw vertex alpha; B+4 controls centered XY. B+8/+C are transported and interpolated upstream but **have no rendering consumer**. Fixed/ramp stack records and missing-B fallback do not prove zeros or meaningful coordinates there.

## 4. Immediate producers and exact predicates

All **12 direct getter/setter references** are independently enumerated by role12 (eight setters, four getters). Source slices must not be mistaken for new original functions.

| Containing range / site | Parameter transformation |
|---|---|
| `800A3528..800A35D8`, setter **800A356C -> F114** | Unconditional entry prefix `(enable0,alpha128,scale bits3F866666,?,?)`; later unrelated work is not a condition on this write. |
| `800A3714..800A37A4`, setter **800A375C -> F114** | Same, enable1. |
| `800A47B0..800A4830`, setter **800A481C -> F114** | Load strength x from task+38; enable=`!(LT(x,0) or EQ(x,0))`; alpha=low8(FCTIWZ(FMULS(64,x))); scale1.05. |
| `802046BC..80204968`, block **80204714..80204760** | Getter80204720 -> F084, setter8020475C -> F0E8; change only returned enable to0. |
| `8020554C..80205E3C`, block **802055DC..80205628** | Getter802055E8, setter80205624; same transformation. |
| `802062AC..80206534`, block **80206314..80206360** | Getter80206320, setter8020635C; same transformation. |
| `8043A5A4..8043AB68`, block **8043A900..8043A97C** | Getter8043A908 -> F084, saved prefix E+14C; target E+15C from flags/resource. |
| `80439A8C..8043A4B8`, block **80439EBC..80439FE0** | Setter80439FD8 -> F0E8, interpolated or target prefix. |
| `8043A4B8..8043A5A4`, site **8043A544 -> F0E8** | Submit E+15C **target**, not saved E+14C. |

Here abbreviated F084/F0E8/F114 have prefix8042. The three disable blocks each have a preceding **8042659C** call (included by selected source from **8020470C / 802055D4 / 8020630C**) and **three separate 800A35D8 lookups**: before that helper, getter, setter. Preservation means **the values returned after the side effect**, not necessarily enclosing-function-entry values or even an assumed unchanged manager alias.

### Numeric caller conditions, without gameplay labels

| Route | Immediate predicate actually traced in 12_closure |
|---|---|
| Fixed enable | `8007BC8C -> 800A2E3C -> 800A3714`. For N: `(N+20 &8)!=0 OR 800ABF30(N+24)==5`, then signed N+1C==1, then `(N+20 &4)!=0`. Wrapper receives `ptr32[ptr32[N+4]+23C]` and loads its +28 controller. |
| Fixed disable versus new ramp | At **8007C140**, N+20 bit3 clear selects **8007C15C -> 800A2E18 -> 800A3528**; set selects **800A2DF0 -> 800A34C8** ramp creation. |
| Decrease request | **800A2DCC -> 800A34B0** sets current ramp+34=1 if it exists. In **800AF60C..800AF6A4**, actor=ptr32[self+28], flag0x15 set AND `(converted signed counter from 8007993C()+70C <=0 OR actor flag0x22 clear)`. |
| Message decrease | **800AF924..800AF998**: u16(message+4)==0x106 and actor flag0x15 set; requests decrease at800AF988 and ORs2 into message+7. |
| Disable A / C | Incoming self+4==0 and `80044E38()==1`. |
| Disable B | self+1C==0, same selector result1, then wrapping `self+4 -= u32[8058E76C]` becomes signed <=0. Table80544E74 entry0=80205584 identifies its dispatch. |

Actor flag0x15 is actor+A8 mask00200000; flag0x22 is actor+AC mask4. The counter threshold805F3D18 is zero and signed conversion bias805F3D20 is4330000080000000. `80044E38` returns a shared object's +14 word. No boost, speed or cutscene design meaning follows from these predicates.

## 5. Ramp arithmetic and interleaved side effects

`800A34C8` marks an old ramp decreasing, allocates0x40 and calls **800A4AB4**. The new task stores **direction+34=0, strength+38=0**, not current B alpha. Vptr80522A78 has Exec **[80522A84]=800A4830**. The actor reference is copied separately; flag0x15 is set during construction.

For incoming delta in f1, original **800A4830** computes `step=FDIVS(delta,0.5)`:

- Direction nonzero: `x=FSUBS(x,step)`; clamp to0 **only if LT(x,0)**.
- Direction zero: `x=FADDS(x,step)`; clamp to1 **only if GT(x,1)**.
- **UNordered skips both clamps**, absent trapping. This corrects the older role11 upper-clamp interpretation.
- After unrelated writes **800A48A4..800A48C4**, the actual call **800A48C4 -> 800A47B0** submits strength-derived parameters.
- After secondary work **800A48C8..800A48F8**, including **8007993C / 8016F4B0**, the task **reloads** strength. Ordered LT or EQ to0 sets task+4 bit0 for deletion. It is not valid to retire from a cached pre-callback value.

Constants **805F3BCC=1**, **805F3BD0=3F866666** (1.0499999523162842), **805F3BD4=0.5**, **805F3BDC=0**, **805F3BE0=64** are verified. Adjacent -0.5 at805F3BD8 is not this step's divisor. For finite x in[0,1], truncating conversion gives alpha **0..64**, distinct from preset128. `0<x<1/64` can enable yet produce alpha0, suppressing both passes. There is **no saturation** or host-language definition of nonfinite/out-of-range FCTIWZ. UN gives enable1 from `!(LT||EQ)` if comparison returns normally; conversion/trap behavior remains the original PPC contract.

## 6. Resource/controller flow

Outer descriptor **type8** at D+20 selects factory table word **8056BE28=804338D4**, allocation0x1F4 and constructor **8043A5A4**. This is separate from **resource kind1** at resource+0, which selects the blur arm.

Base construction supplies manager cast source E+1C, descriptor E+44, age E+C=0 and execution flags E+18=1. The active prefix transfers **D+2C -> E+FC source flags**, **D+30 -> E+104 duration**, **D+34 pointee -> E+108 resource**. Construction does not set the timer-advance flag (bit 1, mask 2). On active/successful-cast/kind1 setup, A908 snapshots B to E+14C, then forms target E+15C:

```text
target.enabled = (u32[E+FC] & 0x00010000) != 0
target.alpha   = u8[resource+8]
target.scale   = f32[resource+C]
target.aux08   = f32 storage[resource+10]
target.aux0C   = f32 storage[resource+14]
```

Resource+4 is not the source of enable. Resource/flags are reloaded after the getter; no extra alias/lifetime guarantee is inferred.

Progress **8042B63C** runs first in Apply at **80439AA0**. Let `F=u32[E+18]`, EPS bits **38D1B717** (~0.0001), unit1 at8051BFA8:

```text
if !(F & 1) or (F & 4): return
if F & 2: E.age = FADDS(E.age, incoming_delta)
if GT(E.age, FADDS(EPS,E.duration)):
    E.flags &= ~1; return                    // old weight retained
E.weight = GT(E.duration,EPS) ? FDIVS(E.age,E.duration) : 1
if GT(E.weight,1): E.weight = 1              // no lower clamp
```

Active/inhibit are rechecked by Apply after this helper. **Strict GT expiry skips this invocation's setter**, not a forced final target. With advance clear, age freezes but progress can still recompute. Unordered comparisons do not satisfy GT; NaN duration selects fallback1, and NaN quotient may survive the upper clamp. Delta units and timer-flag writers are not established.

Kind1 Apply first copies target into a local record. It interpolates **only if LT(t,snap)** where **8051CB58=3F7FF972** (~0.9999), not1. At/above snap or UN, target is submitted unchanged. In the interpolating branch:

```text
enabled = (saved.enabled != 0 OR target.enabled != 0) ? 1 : 0
c       = FSUBS(1,t)                         // exact1 at8051CB5C
float   = FMADDS(target,t,FMULS(saved,c))     // scale and both auxiliary floats
alpha   = endpoint byte if equal;
          otherwise low8(FCTIWZ(the same rounded-product/fused mix of unsigned bytes))
```

Unsigned alpha conversion uses **binary64 bias4330000000000000 at8051CB60**. Endpoint alpha is not zeroed just because its enable byte is0. The fused expression is not equivalent to `saved+(target-saved)*t`; no extra weight/byte saturation is permitted. Finalization **8043A4B8** writes **target** on nonnull self/successful cast/kind1, without testing active/inhibit/t. That write happens only if finalization actually runs, not automatically at timer expiry.

## 7. From control to EFB, capture and next eligible history

Draw uses current-view signed dimensions/origins, unsigned current-root UV denominators and scale to overwrite absolute TL/BL/TR/BR geometry. It does not read history pixels on the CPU. On successful Begin it binds Hroot and requests RW5/6 -> GX4/5 blending, disabled depth-write/test policy, fog off and filter1=nearest. Texture realization uses **the same X+1C pixels previously copied**, TEV source alpha=`Atex*vertexAlpha` under identity swaps. [GX state](motion_blur_gx_state.md) provides the exact equations, numeric tables and inherited-state conditions.

Later Save clears B+78, checks enable/alpha/provider/camera, and copies EFB to **Sroot**. Its optional half-size scratch strip changes EFB; then copy2 writes **Hview's root**, and an optional full-size scratch strip redraws EFB. Both selectors are0; clear comes independently from805F2688. Format is RGBA8/GX6/depth32/no mipmaps. Copy uses signed view origins and tiled offset `4*(4*x+align4(rootW)*y)` at depth32; sampling root base follows the same allocation. Pixel-mode synchronization and texture-cache invalidation do not make valid a completion fence.

An explanatory recurrence indexed by **qualifying Save opportunities**, not displayed frames, is:

```text
Q[k] = Composite(EFB_before_Draw[k], H[k-1], current geometry/alpha/GX state)
R[k] = intervening rendering applied to Q[k]
H[k] = capture of the temporary reduced EFB derived from R[k]
```

For failures or suppression, this mathematical shorthand must defer to the actual control flow: reached-disabled Save invalidates without changing pixels; skipped Save changes neither; optional Begin failure can still end with valid1 and a non-intended H; later rendering can overwrite feedback. In Div modes1/2 two Draws can read **the same old H before Save**. Thus H need not be the previous unblurred scene, previous camera-specific frame or previous presented framebuffer.

Setters never invalidate; Draw never consumes valid; camera cuts/resizes trigger no core reset/reallocation. Re-enable without a reached disabled Save can reuse stale valid H immediately. Successful destroy/recreate starts with new invalid storage; no pixel clear is proved. [Temporal state](motion_blur_temporal_state.md) enumerates these paths.

## 8. Source correspondence and limits

The integrated source has **19 complete core/task entries**, **13 backend address-bearing definitions (4 complete/9 selected)** and **13 input definitions (2 complete/11 selected)**. Ramp's three selected blocks belong to **one** original update function. Continuation PCs are source annotations, not executable register/stack resumption; omitted mixed-effect work still belongs between those blocks. Only the CopyEFB adapter gains a definition under the core's original namespace; no selected backend is installed as a generic engine callback.

The current 121-range/80-record evidence and strict syntax/default/O2 PPC LLVM IR checks are verified. SDK/ABI bindings, full machine-code generation, original stack residues, fault/exception ABI, live resource/producer arbitration, concrete cameras and GPU output remain external or **UNVERIFIED**. Synthetic tests are byte-grounded formula checks, not C++ or game execution. No fallback, timer unit, gameplay label, opaque-alpha assumption or fabricated history reset completes those contracts.

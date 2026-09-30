# Motion blur — temporal state and execution cases

## What is temporal here?

The original subsystem retains **one image H**, rooted through B+74, and a separate software byte **B+78**. It does not retain motion vectors, previous-camera matrices, a per-camera history map, alternating read/write textures, a timestamp or an image-age counter. Draw **8042E254** overlays H; later Save **8042E838** overwrites the same H after intervening rendering. A successful copy is not measured by the byte.

Target SHA-256: `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`. Addresses/offsets are hexadecimal, ranges end-exclusive. Static evidence: [08 temporal analysis](evidence/08_temporal.md), [03 task closure](evidence/03_closure.md), [05 lifecycle closure](evidence/05_closure.md), [12 inputs](evidence/12_closure.md), [14 adversarial analysis](evidence/14_adversarial.md), [original Draw](evidence/original/8042E254_MotionBlur_Draw.asm), [original Save](evidence/original/8042E838_MotionBlur_SaveScreen.asm). Current manifests authenticate 121 ranges/80 data records; historical unread-slot requests are superseded. **No runtime/VI/FIFO trace is available.**

## 1. Minimal machine state and transitions

Let `e=u8[B+0]`, `a=u8[B+1]`, `s=f32[B+4]`, `v=u8[B+78]`. H is the pixel allocation reached through B+74; S is shared scratch through B+10 -> M+34. “Camera succeeds” means the actual provider/cast/virtual lookup returns a usable camera, not an assumed global default.

| Operation | State transition / side effect |
|---|---|
| Create `8042F20C` | Allocates0x13C, conditionally calls constructor, then installs the allocation pointer at M+40. No old-state destruction is implicit. |
| Constructor prefix | e=0, a=0, s=1, owner=M; camera lookup may return early with history/valid/vertices untouched. |
| Constructor on completed null-parameter path | e=0, a=128, s=1, v=0; H allocated/attached but not seeded; padding/auxiliaries need not be initialized. |
| Parameter helper/setter | Changes fields0/1/4/8/C only. **v and H unchanged**. |
| Draw entry | Require e!=0, a!=0, successful camera lookup, then v!=0. Failure exits without invalidating H. |
| Draw after those gates | Snapshot requested states; recompute geometry/color/UV before Begin. Begin failure exits without submitting or restoring those requests. v remains unchanged. |
| Draw after successful Begin | Bind Hroot, set selected state, submit one strip, issue six implemented restores plus ignored12, End. No H write or valid consumption. |
| Reached Save entry | **v=0 first**, before e/a/provider/camera gates. |
| Save qualifying tail | Two copy submissions and two optional camera/draw blocks; **v=1 at8042E9AC**, irrespective of their optional Begin results. |
| Destroy `8042F198` | Root then view, B freed, M+40 cleared; subsequent bridge calls with absent B skip the cores. |

The latch means **“the last reached Save got through its early gates and reached its tail”**, under unmodified single-threaded calls returning normally. It does not mean image contents were initialized, reduction/restoration ran, asynchronous GPU work completed, or that the image is from exactly the previous display frame.

## 2. Registered normal chronology

The recurring event path is **800510C0 -> 80051978(event0x12) -> 80048EB8 case80049078 -> 801E2210 -> 801EB680 -> 8004ECAC(common root)**. Group5 is named Render. Actual tail append and execute-then-child-recursion order, not naming alone, establish:

```text
RenderAttach_Start (R+5C)
RenderAttach_Div   (R+60)
  ... PostGlare, index 11 / R+2C
      selector [804D25D8]=1 places this level under Div
      neighboring PostGlare task
      DrawBlur -> 801D27FC -> 8042F16C -> 8042E254
  ... intervening Div children
RenderAttach_Static (R+64), selector reset to0
  first child PostEffect (R+58)
    SaveScreen -> 801D2764 -> 8042F140 -> 8042E838
    later DrawScreenColor
  later Static sprite/fade/UI levels
RenderAttach_End (R+68)
```

Draw's level is **PostGlare (11), not GlareWorld (9)**. Save is scheduled under early Static/PostEffect even though Draw's task owns its pointer. Save is therefore not guaranteed to capture final UI/presentation output. The relevant construction, table words, Exec slots and loop edges are all closed in [callgraph](motion_blur_callgraph.md).

For a single-view ordinary traversal, with both core paths eligible and operations completing normally:

| Opportunity | Draw | Later Save | H available afterward |
|---|---|---|---|
| First enabled after v=0 | Skip overlay; invalid latch | Capture current/intervening EFB, reduce, copy, restore; v=1 | Newly captured H |
| Subsequent enabled | Sample prior H using current geometry/parameters | Capture EFB which may already include the overlay | Feedback can accumulate across captures |
| Several Draws with no Save | Each samples the same H | No update | H ages without an age counter |
| Several Saves with no Draw | No consumption | Same allocation repeatedly overwritten on qualifying paths | Latest qualifying stored image, not a queue |

An illustrative recurrence is `Q[k]=Composite(scene[k],scaled H[k-1])`, then `H[k]=CaptureReduced(interveningRendering(Q[k]))`. The index denotes opportunities, not VI frames. Scissor/overwrites/tests can change or remove feedback; no infinite trail or exact visual persistence is asserted.

## 3. Enable/alpha/valid matrix

| Situation | Draw result | Reached Save result | Subsequent consequence |
|---|---|---|---|
| e=0, any a/v | Skip | v=0, early exit; H pixels not erased | A later re-enable needs a qualifying Save only if this invalidation was reached. |
| e!=0, a=0 | Skip | Same invalidation/exit | Enable alone is insufficient. |
| e!=0, a!=0, v=0 | Camera lookup then invalid-history exit | If camera succeeds, attempts capture and sets v=1 | First eligible overlay can occur later. |
| e!=0, a!=0, v!=0 | Attempt overlay | Replaces H on qualifying path | v is reusable, not one-shot. |
| Change alpha or scale | Immediately affects next eligible Draw | No setter-side invalidation; next Save uses fixed capture quads | Old H is not automatically discarded because parameters changed. |
| Disable then re-enable between Saves | Old valid H may draw immediately | No disabled Save occurred | No mandatory warm-up/reset. |
| Disabled Save task skipped | Draw still tests current e/a | No core entry, so **no v=0 store** | H/v can persist while disabled. |
| Enable-only snapshot disable | e=0; other returned prefix fields retained | Invalidates only when Save later runs | Distinct from fixed disable's alpha128/scale1.05 overwrite. |
| Positive ramp x with alpha0 | e may be1, but Draw skips | Reached Save invalidates | Finite `0<x<1/64` is a concrete case. |
| Manager has no B | Bridge does not enter Draw | Bridge does not enter Save | There is no B valid byte to reset; missing-B parameter getter uses separate fallback semantics. |

No core camera-cut, teleport, actor-state, resize or level-transition reset is hidden in setters. Destroy/recreate or an actually reached invalidating Save can explain a reset; arbitrary game design labels cannot.

## 4. Save's two Begin results

After the early gates pass, **both copies are outside the optional Begin-success blocks**. Call order: copyS **8042E8C0**, Begin1 **E8C8**, optional bind/blend/half strip **E8F0/E910/E930/E954**, End1 **E95C**, copyH **E968**, Begin2 **E970**, optional full strip **E99C**, End2 **E9A4**, valid1 **E9AC**.

| Begin1 | Begin2 | CPU control-flow result | What is not guaranteed |
|---|---|---|---|
| Success | Success | Both copies, half-size and full-size quads; scratch binding/replacement factors established in block1 | Actual pixel coverage, accepted tests, clear effects, filter, quantization, GPU completion or bit-identical restoration |
| Failure | Success | CopyS; skip bind/blend/reduction; copyH still runs; second quad uses **inherited texture/blend** | H is not proved downsampled; full quad need not restore S |
| Success | Failure | CopyS; scratch reduction; copyH; no full redraw | EFB may remain altered by the temporary reduction/copy-clear |
| Failure | Failure | Both copies still attempted; neither quad/End | Neither intended reduction nor restoration; nevertheless v=1 |

These are conditional CPU paths, not observed failures. End/primitive return values and copy success are not used to determine v. The CopyEFB invalid-depth arm can report an error and return without copying; Save has no result test to prevent its final valid store. On normal allocated0x505 roots depth32 is established; invalid/mutated roots are outside that normal invariant, not an invented retry case.

Each copy uses selector0, **not clear=false**. The live low byte at **805F2688** controls actual GXCopyTex clear. Source S/H rectangles, inherited sampler, alpha tests, write masks, scissor, cull and EFB format can affect images independently of Begin's return. Copy-filter restoration is mode-derived; there is no complete state snapshot around Save. [GX state](motion_blur_gx_state.md) specifies the exact values and branches.

## 5. Lookup, allocation and exceptional cases

| Case | Established original behavior / temporal implication |
|---|---|
| Draw provider/cast/camera absent | Returns before history use. Existing v/H are retained. A later camera can therefore see old history. |
| Save provider/cast/camera absent | v has already been cleared; no copies/quads. Pixels remain but are gated from later Draw until a qualifying Save tail. |
| Draw Begin failure | Geometry/alpha/UV writes and state Get requests have already occurred; no selected Set/primitive/End block. H/v remain reusable. Begin may itself have external side effects. |
| Partial B construction | Prefix e0/a0/s1/owner exists, but +74/+78/vertices can be allocator residue. Create still installs B. Later enable/destruction can encounter invalid storage unless caller guarantees prevent it. No crash-free invariant is proven. |
| B allocation failure | Create stores null at M+40; no constructor. Existing pointer replacement is not automatically a destructor or recovery operation. |
| Raster allocation/attachment failure | Constructor does not fully check or transactionally undo the raster chain. Software valid is not a construction-success proof. |
| Zero/negative/changed dimensions | Original conversions/halving/low16 copies/32-bit arithmetic execute without sanitization; storage/capture compatibility and exceptional FP behavior are not generally validated. |
| External mutation/re-entrancy | Original code repeatedly resolves globals/pointers; a saved projection global, shared T/S and one v/H are not proven re-entrant. No thread-safety/per-camera partition is inferred. |
| Nonfinite scalar/scale/timer or enabled FP traps | Ordered/UN branches are decoded, but actual conversion result, precise exception/resumption and partial-store visibility require PPC state/ABI. No host cast or clamped fallback supplies them. |

The old constructor source grouping problem is **resolved at object/FP event-order level**: original **8042EC24..8042ECB4** and optimized IR agree on **24/24 events**, including height division before first full vertex blue/alpha stores. It is **not an active mismatch**, nor does that check establish register/stack or fault-for-fault runtime equivalence. See [audit resolution](evidence/source_audit_resolution.md).

## 6. Multiple cameras, view changes and cadence

Div's installed Exec **[8053EA7C]=801E4EF0** reads mode **8057E7C0**. Static's installed Exec **[8053EA60]=801E4E40** is separately resolved.

| Mode / situation | Exact temporal consequence |
|---|---|
| Div0 | No internal child traversal; the generic walker visits its children once. |
| Div1 | Internal traversal at **801E4FA0** for selector1; then selector2; generic walker traverses children again. |
| Div2 | Internal traversal at **801E501C** for selector3; then selector4; generic second traversal. |
| Static afterward | Resets selector/view index to0, calls view setup, then ordinary child traversal reaches Save. |
| Div1/2 with both Draws eligible | **Two overlays can use the same old H before a later Save**. They are not two per-camera captures. |
| Camera changes between Draw and Save | Each re-queries the provider. Save can use a different camera; there is no camera-ID match test. |
| View/root size changes after construction | Draw rebuilds current XY/UV using current root unsigned denominators; S/H sizes and Save quads retain construction dimensions. No automatic resize/reseed. |
| Other live Div mode | Falls through without the known0/1/2 selection setups; no guaranteed camera configuration follows. |

Scratch takes construction **view** dimensions; history takes construction **root/2**, signed truncation toward zero. One current subview can sample a region of the common H using biased UVs, but that does not create independent camera histories or independent validity ages.

The recurring loop's ordinary scheduler edge is statically closed, but **one traversal is not guaranteed to equal one VI field/frame**. First-use driver alternatives can add/skip a traversal; transitions can make three extra scheduler calls or self-loop; Div can recurse twice. Viewport field jitter queries VI's next field but does not make B+78 a presentation marker. Mode and delta are live inputs, not a measured cadence.

## 7. Scheduler suppression, pause and deletion

The base constructor **8004F014** tail-appends. Walker **8004ECAC** normally performs task Exec, child recursion, next sibling. Low task-flag nibble suppresses normal traversal; deletion bit0 has its own pause/bit0x20 path and virtual destructor call. A nonzero pause byte **805EF27C** can suppress an Exec while still visiting children when low flags are clear. The render/blur tasks carry **0x100**, the pause exception. Therefore “paused means no Draw/Save” is not a universal fact.

Profile mode **805EF274** invokes **8004EDF0** with the same Exec slot; it changes timing bookkeeping, not the data-flow order. Depth **805EF270** is recursion accounting, not a frame counter. Runtime edits to flags/tree/callbacks can skip just one phase: skipping Save preserves old valid H; skipping Draw does not prevent Save from replacing H.

Draw task destruction **801D2788** ORs bit0 into its retained Save task's flags, then runs base/optional deletion; Save is retired through its actual scheduler list. This does not immediately free H. Stage lifecycle calls **801D19B8 -> 8042F198** before **801D19D8 -> 804335C8** to destroy history then scratch. Owning-root backend token wait/unbind/free is known, but no runtime safety/wait-duration result is claimed.

## 8. Parameter-controller time is separate from image time

Ramp task **800A4830** uses incoming delta/0.5 and scalar+38, not measured camera speed. It starts at0, strengthens to finite alpha64, or decreases when +34 is nonzero; preset paths use alpha128/scale bits3F866666. UN skips both one-sided clamps. Submission is between other side effects; retirement tests a **post-callback reload**. Neither scalar nor retirement flag tells how many H captures occurred.

Resource controller **8042B63C** uses E+18 masks active1/advance2/inhibit4, age E+C, duration E+104 and weight E+100. Inactive/inhibited calls do not advance/recompute. Active with advance clear can recompute t from frozen age. Expiry is **strict age > FADDS(duration,EPS)**, EPS bits38D1B717; it clears active and preserves old t. Apply then **skips the setter**, not forces target. Otherwise duration>EPS divides age/duration, else t=1; only t>1 is clamped.

Kind1 interpolation uses snap bits **3F7FF972**, complement **1**, `FMULS(saved,1-t)` then **FMADDS(target,t,product)**. Below snap enable is endpoint OR; at/above snap or UN it is target's raw prefix. Finalization writes target even if inactive/inhibited, **only when the finalizer executes**. There is no automatic link from expiry to finalizer, no guaranteed time unit, and no proven arbitration priority over other writers. All setter paths leave v/H unchanged; reached Save supplies the invalidation policy.

## 9. Established versus unmeasured time

Established: registered Draw-before-Save order; one reusable history allocation; exact valid stores/gates; conditional copy/quad order; two-view recursion; parameter arithmetic and no setter reset; same-call, not previous-frame, projection backup. These claims have independent instruction interpretations in roles03/04/05/08/12/14 and current byte provenance.

**UNVERIFIED:** correspondence to a specific VI/presented frame, concrete image age, current resources/cameras/delta/clear value, allocation/Begin success, precise FPSCR/exception behavior, GPU completion, pixel fidelity and visible trail shape. [Role13](evidence/13_runtime.md) contains static predictions and a capture protocol, **not observations**. The [verification ledger](evidence/verification_ledger.json) records 23 synthetic tests, strict PPC syntax and default/O2 IR success; it records no object/link/game/GPU validation. Those boundaries are part of the analysis, not missing reset or fallback code.

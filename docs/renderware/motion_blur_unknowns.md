# Motion blur — genuine unknowns and verification boundaries

## Current evidence boundary

The normal original blur path is statically identified and its required table/ownership/control edges are closed. **An exact standalone rebuilt/linked/executed replacement is not established.** This distinction must not be erased by calling every dependency an SDK function or every source slice a complete entry.

Target `main.dol`: **5,773,024 bytes**, SHA-256 **`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`**, unchanged. Current [manifests](motion_blur_original_addresses.md): **121 original function ranges / 80 data records**, with **10,075 instruction words / 3,908 file-backed data bytes verified**. All addresses below are original local VAs; ranges are end-exclusive.

Current source is **2,393 CPP lines / 456 header lines**, with core/task, backend and input integration present. The [verification ledger](evidence/verification_ledger.json) records **23 synthetic tests PASS**, strict PPC syntax PASS, default/O2 PPC-target LLVM IR PASS, an installed-toolchain machine-object blocker, no linking/execution and unavailable runtime correlation. [Final IR](evidence/motion_blur_verified.ppc.ll) is frontend output, not a PPC object. Source/executable coverage is classified in [functions](motion_blur_functions.md).

## 1. Closed static facts, not remaining requests

| Former ambiguity | Current resolution / evidence |
|---|---|
| Task/core identity and recurring route | Draw `801D27FC -> 8042F16C -> 8042E254`; Save `801D2764 -> 8042F140 -> 8042E838`. Recurring `800510C0 -> 80051978(event0x12) -> 80048EB8 case80049078 -> 801E2210 -> 801EB680 -> 8004ECAC(common root)` reaches group5 Render. [03](evidence/03_closure.md), [04](evidence/04_closure.md). Case/call-site PCs are not extra functions. |
| Phase selector / multiple-view slots | **804D25D8=1**, Draw at level11 PostGlare/R+2C under Div/R+60; Save early PostEffect/R+58 under Static/R+64. **8053EA7C=801E4EF0**, **8053EA60=801E4E40**. Modes1/2 can draw twice before Save. Not an unknown vtable or VI cadence guarantee. |
| Device/standard installation | Device8056F50C copied by operation4 to engine+10; standard operation **11**, not9, installs27 records8051D400 into engine+48/count29. Standard index12 is **engine+78**, not94. [04](evidence/04_closure.md), [05](evidence/05_closure.md). |
| Provider/cast/camera source | Provider8053E93C+24 -> **80028730 -> 8000DCC0 -> camera-manager field0**, normal body8056FF1C/cache805EF068. RTTI805E9D98/base8053E920 includes805E9D88 with name matching target8056BCE4. Normal cast and slot identity are closed, not arbitrary live camera choice. [04 §5](evidence/04_closure.md). |
| Image format, allocation and sampling | One Hroot allocation behind B+74, plus shared S at M+34. Nonempty0x505 roots are GX6/RGBA8/depth32/alpha1/no mips. Payload64*ceil(W/4)*ceil(H/4), request+31; owner+18 versus aligned pixels+1C. Copy and realization reach the same pixels. No ping-pong or pixel clear is proven. [05](evidence/05_closure.md). |
| State, filter, TEV and matrices | RW9=filter; RW12 get returns0/no output write, set returns0/no effect. RW5/6 -> GX4/5; six implemented restores plus ignored12. TEV words C008F8AF/C108F2F0 preserve swap bits; alpha includes Atex. Projection/matrix literals and mode height+8 are read. 805E4218 is same-call save/restore, not previous-frame data. [GX state](motion_blur_gx_state.md). |
| Inputs/constants/containing boundaries | Fixed scale bits3F866666/alpha128; ramp delta/0.5, low8(FCTIWZ(64*x)); UN skips both clamps. Progress EPS38D1B717, snap3F7FF972, complement1, FMULS/FMADDs; target-writing finalizer. All12 immediate get/set references, resource kind1 versus outer type8, three preceding8042659C side effects and actor predicates are closed. [12](evidence/12_closure.md). |
| Lifetime machinery | Root destruction includes token handling/poll, conditional unbind and allocator-base free; history is destroyed before shared scratch. This is known machinery, not a measured safety/wait-duration result. [05 §6](evidence/05_closure.md). |
| Constructor source event ordering | **F1 is resolved**, not an active old finding: original8042EC24..8042ECB4 versus optimized IR **24/24 object/FP events** agree. Height division precedes blue/alpha stores. [Resolution](evidence/source_audit_resolution.md) supersedes that finding in the preserved [audit](evidence/final_source_audit.md), but not its fault-ABI caveat. |

Earlier role06/07/08/09/10/11/13 unread-table/constant/slot lists are historical acquisition limits, not current unknowns. Roles03/04/05/12 and the current manifests supply the decisive closure. Their earlier 81-range totals are snapshot provenance, not current coverage.

## 2. Live values and conditional contracts

These are not unread DOL tables; obtaining more static constants alone cannot supply them.

| Contract / live input | Established local behavior | Still unestablished |
|---|---|---|
| **Traversal versus presentation** | Registered Draw-before-Save; ordinary recurring event route; Div double traversal and driver initialization/transition alternatives | A particular callback's VI field/XFB presentation, exact capture age, frequency or a one-callback-per-frame invariant. No trace exists. |
| **Selected cameras and dimensions** | Each constructor/Draw/Save resolves provider anew. S uses construction camera view dimensions; H uses construction root dimensions signed-/2. Draw UV denominators use current root unsigned dimensions. | Actual C/V/root/mode values; cross-camera compatibility, dynamic resize lifecycle or later callback/slot mutation. No camera-ID reset in the core. |
| **Copy clear** | 804963AC/B4 passes **low8(u32[805F2688])** independently of both selector0 arguments. Startup slot is zero-backed. | Its live value and alias-complete writer exclusion. Load-only D-form/address scans do not rule out indirect/indexed/aliased writes. No clear=false invariant. |
| **EFB contents and alpha** | Retained roots are RGBA8; ordinary TEV source alpha is Atex*vertexA, with preserved swap selectors. | Captured alpha, actual EFB format/coverage/quantization and visible effective weight. Alpha128 is not automatically128/255. |
| **Save sampler** | Draw requests nearest and restores prior filter; Save chooses no sampler/wrap. Raw lookup and no-mip paths are decoded. | Filter/wrap/LOD at capture; bilinear reduction, clamp edges or a lossless restore. Open defaults are not entry snapshots. |
| **Inherited GPU state** | Selected depth/fog requests, alpha-bit-dependent test realization, viewport/scissor/projection branches are known. | Cull, color/alpha writes, alpha comparisons, Z location, indirect/direct stages, swaps/tables, automatic coordinate scale and other entry state. No invented fullscreen defaults. |
| **Allocation/Begin success and completion** | Original failure paths and valid stores are known; copy sync/cache invalidation and teardown token wait exist. | Successful live allocation/attachment/Begin, completed GPU work, wait duration, initialized padding/texels, or safety across arbitrary aliases. |
| **Service/resource lifetime and re-entrancy** | Normal provider install and image ownership traced; original reload points remain significant. | Arbitrary service replacement, E+108 resource retirement, callback side effects, re-entrant use of S/T/projection backup or task-tree mutation. |

Actual GPU masks/tests/samplers are part of the original contract, not permission to impose opaque alpha, clamp/linear, cull-none, always-write or a CPU completion fence in its name.

## 3. Input timing and competing writers

**Execution flags E+18** (active1/advance2/inhibit4) are distinct from **source flags E+FC**, whose bit16 provides target enable. Base construction starts age0/active1, not necessarily time-advance. UpdateProgress advances age only with mask2, tests strictly `age > FADDS(duration,EPS)`, and clears active without updating weight on expiry. Apply rechecks flags and skips its setter after that expiry. The finalizer writes target only when it runs; it is not automatic at expiry.

Concrete delta units/frequency, complete writer/scheduling lifecycle of the timer/inhibit bits, concrete resource instances and **arbitration/order among the fixed/ramp/resource/disable producers** remain caller/live contracts. The core has no priority blend or writer registry: last executed field transfer takes effect. A numeric immediate predicate does not establish an undocumented boost/speed/cutscene role.

Ramp update contains real unrelated writes/callbacks between advance, submit and retirement. Selected source preserves boundaries rather than treating that work as a no-op. The three disable prefixes call **8042659C before reading settings**, and do **three manager lookups**. Their guarantee is enable0 plus the other values returned **after that side effect**, not preservation of an assumed function-entry manager/state.

B+8/+C have no Draw/Save GPU consumer. They are transported as words and interpolated as floats by the resource path; their intended historical names remain unresolved. That naming limit does not hide another active center/velocity equation.

## 4. Known irregularities versus indeterminate machine state

| Known original behavior | Exact consequence / machine limit |
|---|---|
| State12 getter does not write original SP+1C; Draw later loads it at8042E780 | Raw residue is passed to an ineffective restore. No saved vertex-alpha value may be invented. Source models an indeterminate load, not original compiler stack location/bits. |
| Missing-B parameter getter reads unwritten local words into output+8/+C | Only enable0/alpha0/scale1 are defined defaults. Auxiliary zero, prior value or coordinate meaning is not established. |
| Constructor/default/fixed/ramp paths leave auxiliary/padding bytes unwritten | Raw-word/LFS semantics preserve the unresolved provenance; ordinary typed C++ initialization would silently change it. |
| Constructor can return before B+74/+78/geometry initialization while Create installs B | Partial-object behavior is known. A universal successful-construction invariant or fault-free teardown is not proved; no actual crash is claimed. |
| Save clears v before gates, sets1 after optional Begin blocks regardless of their results | v is not a success flag/fence. Second-only successful draw can use unrelated inherited texture/blend. |
| Draw never consumes v; setters do not reset it; skipped Save cannot clear it | Re-enable can reuse old valid H; no automatic camera-cut/reset/resize behavior exists in this family. |
| `80372508` uses `dcbi`, not memset | Raster/descriptor metadata initialization does not prove black/transparent initialized image contents. |

The unresolved **fault/ABI equivalence** domain includes actual FPSCR/MSR, signaling/quiet NaNs and payload handling, out-of-range/nonfinite FCTIWZ, original SP/GPR/FPR state, invalid pointers, overlapping argument storage, asynchronous alias mutation, exception handler/resumption and compiler lowering. The first-vertex event-order correction closes one concrete source mismatch, **not every possible memory-fault ordering or complete machine-state equivalence**. No new guards, saturating conversions or sanitized defaults are attributed to the original to hide these domains.

## 5. Source scope and external bindings

| Layer | Present | Remaining boundary |
|---|---|---|
| Core/task | **19 complete original body mappings**, CPP1–808 | Original allocator/manager/task/camera/raster/RTTI adapters still need genuine ABI/address bindings. Complete source is not byte-identical compiler output. |
| Backend | **13 address-bearing definitions: 4 complete / 9 selected**, CPP810–1814 | Complete CopyEFB, SetRaster, conditional restore and BackendSubRaster; selected get/set/flush/prepare/strip/realization/format/create/size are not a generic renderer. |
| Input | **13 definitions: 2 complete / 11 selected**, CPP1816–2393 | Complete ApplyRamp and UpdateProgress. Three ramp slices share one original update container; omitted fixed tails, mixed-kind arms, shared construction/destruction and intervening callbacks are not executed by the annotations. |
| SDK/engine | Real addresses, raw assembly, normal table installation and argument intent | Declarations do not bind linker symbols or implement SDK/plugin/freelist/allocator behavior. Mixed GPR/FPR, paired-single/Gekko, varargs/error and callback ABI are not validated by parsing. |

Backend **`require_extracted_path` / `__builtin_trap` at CPP1100–1103** is an **explicitly non-original analysis domain assertion**, not a recovered error path, fake success or generic callback. Null-texture fallback, untextured/general primitive paths, other states/formats, palettes and mipmaps are deliberately outside those source definitions. No selected path is installed into an engine/vtable slot. Only CopyEFB supplies an integrated definition for its pre-existing core `original::` declaration.

Input **Continuation** records are source-level control-flow annotations. A returned original PC is **not original r3**, not success, and **not executable binary resumption with live registers/stack**. Presence of eleven selected definitions does not establish eleven additional complete original functions. This boundary is explicit in header prototypes/comments as well as implementation.

## 6. Verification results and their limits

1. **Byte correspondence — PASS.** Whole target hash, per-range hashes, exact RAM/file offsets, complete exported instruction words, direct branch decode and file-backed data agree for the current121/80 snapshot. The 19 zero-backed records have no live DOL value. Supplemental closure windows remain distinguished from the121 exports; a manifest hash cannot authenticate an unrelated legacy window by association.
2. **Synthetic semantics — 23 PASS.** 90 geometry,1,960 copy offsets,144 ramp steps,260 finite alpha conversions,640 progress and240 interpolation fixtures, plus tables/GX/TEV/negative controls. [Methods](evidence/semantic_verification_extra.md) specify rounding/input domains. These are **not compiled C++ execution, callback integration, FPSCR/trap simulation, game execution or GPU output**.
3. **Strict frontend/layout — PASS.** Clang21.1.8, `--target=powerpc-unknown-eabi`, C++17/freestanding/no exceptions/RTTI/no fast-math/FP contraction off/strict warnings. Default and O2 PowerPC-target IR emission pass. The current first-vertex 24-event order passes optimized-IR comparison against fresh original instructions.
4. **PPC machine code — BLOCKED / UNVERIFIED.** Installed LLVM accepts the frontend/ABI but `--print-targets` has no PPC machine backend; actual object generation failed. No final inline-assembly expansion, register allocation, relocation, linked symbols or machine-disassembly comparison is available. No devkitPPC compiler was found in the reported PATH/standard C: locations; no installation is implied.
5. **Link/runtime/GPU — UNVERIFIED.** No linked/runnable extraction, original compiler/linker reproduction, captured game/FIFO/debugger/input/frame trace, GPU completion or pixel comparison. [Role13](evidence/13_runtime.md) is a static prediction/capture protocol, not observed data. The existence of an analysis ELF or LLVM IR is not runtime evidence.

Source hashes are **CPP `3d2cb9dca2a2b4f220adfb5c16163d9f8e61229b44d89f701a20564261b8ad1b`**, **header `c1aefedcd422b5f16a14c4f461462c81725a6092fb17144b1966df8001328534`**. [Source integration](evidence/source_integration.md) records the historical2,372-line revision; current old-line498 onward is shifted+21, as documented in the resolution. Its old count is not current coverage. The current final artifact is [motion_blur_verified.ppc.ll](evidence/motion_blur_verified.ppc.ll), not an older IR checkpoint.

## 7. Provenance and availability

All15 numbered specialist roles have evidence reports, linked in the [README cross-check matrix](README.md). Their independent instruction interpretations are valuable but are not fifteen independent DOL acquisitions or runtime experiments. Some early commands were denied and bounded cached listings supplied evidence instead; not all roles independently hashed the target or reviewed the final source. The current hash-checked manifests and ledger supersede stale counts without retroactively changing those reports' methods.

`GPT_SOL_analysis/` independently interprets the first31 exports. [Sol cross-check](evidence/sol_crosscheck.md) records agreements and instruction-proven differences; its draft is not a replacement implementation or a runtime witness. Original DOL, prior/Sol files, evidence reports, manifests, source, tests and tools retain their separate provenance.

A claim of displayed-frame age, effective blend weight, lossless restore or exact failure behavior would require actual correlated state at scheduler **801E23A4**, Draw/Save **8042E254/8042E838**, copies **8042E8C0/E968**, primitive sites **8042E738/E954/E99C** and **803975AC**, including camera/mode, B prefix/valid, root pixels/format,805F2688, inherited GX state and presentation timing. **Those observations are unavailable**, not silently assumed from passing static tests.

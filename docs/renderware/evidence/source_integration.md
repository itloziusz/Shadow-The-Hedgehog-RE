# Readable motion-blur source integration

## Scope, ownership, and evidence

Only these deliverables were written by this integration role:

- `analysis/motion_blur/reconstructed/motion_blur.h` — now 456 lines.
- `analysis/motion_blur/reconstructed/motion_blur.cpp` — now 2,372 lines.
- `analysis/motion_blur/evidence/source_integration.md` — this new ledger.

Original `main.dol`, prior reconstructions/reports, `GPT_SOL_analysis/`, evidence fragments, raw exports, manifests, tests, configuration, and game files were not edited by this role. No installation, game patch, SDK rewrite, or runtime/emulator run was performed. The semantic-test file already contains another role's extension; this role only read and ran it.

Notation below: **C** = `analysis/motion_blur/reconstructed/motion_blur.cpp`; **H** = corresponding `.h`; **O** = `analysis/motion_blur/evidence/original/`; **D[a,b)** = a hash-checked bounded `dol_evidence.py disasm` read, not a new export. All original ranges are half-open local VAs. **`12_closure.md` is authoritative for the input conditions**, particularly its correction of `11_decompiler.md:494`: unordered comparison skips **both** one-sided ramp clamps.

Fresh `dol_evidence.py verify` passed: SHA-256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`, **81 ranges / 7,249 instruction words / 3,626 data bytes** in the supplied 81-function/55-data snapshot. This verifies the original evidence, not execution of this C++.

## Integration structure and preserved core

1. **C:1-787** retains all 13 core entries in `[8042E254,8042F258)` and six task entries. No core executable statement or existing PPC helper was changed. The top scope note (C:3-15) now describes integration, including unsupported backend state12. The DrawBlurTask allocation provenance note (H:151-154, C:684-693) was explicitly updated after fresh D[801D1AD0,801D1AFC): `li r3,0x30` at 1AE0, allocator BL 1AE4, constructor BL 1AF0. Original cached provenance remains recorded.
2. **C:789-1793** appends all **1,005 lines** of `evidence/backend_reconstructed.inc` after the closed core namespace, preserving its comments and definitions. It reopens `motion_blur`; it is not nested inside the original namespace. Its historical two-include example must not be used to include the fragment a second time after integration (clarification C:1795-1802).
3. **C:1804-2372** adds namespaced PPC comparison/conversion/FMA notation and readable input definitions. **H:311-450** exposes backend/input prototypes, input layouts/assertions, a selected-block continuation type, and the one new real-address effect-side-effect declaration.

The only appended backend definition under an existing core `original::` declaration is **`original::RwRaster_CopyEFB_80496208` (C:999)**. Core engine getter/setter/primitive calls still use the original live device table. No backend selected path, input slice, or scope assertion is installed into that table or a task/effect vtable. Source availability is not live callback installation.

## Backend coverage imported unchanged

The fragment supplies **13 address-bearing definitions: four complete wrappers/bodies and nine explicitly restricted paths**. `backend_source_notes.md:47-107` retains the detailed original instruction, data, call-site, and DOL-offset ledger; the line mapping into C is **fragment line + 788**.

| Definition (C line) | Original containing range | Actual scope |
|---|---|---|
| `original::RwRaster_CopyEFB_80496208` — 999 | `[80496208,80496410)` | Complete; four depth-offset arms, actual error path, independent copy-clear word, filtering and invalidation |
| `backend::RwTexture_SetRaster_8049819C` — 1119 | `[8049819C,804981B8)` | Complete pointer store and dirty-flags assignment |
| `RwRenderState_Get_BlurCases_804984C4` — 1129 | `[804984C4,804986B8)` | IDs 1,6,8,9,10,11,12,14 only; state12 writes nothing and returns 0 |
| `RwRenderState_Set_BlurCases_80498954` — 1155 | `[80498954,80498F74)` | Same IDs, their complete input-word behavior; not a generic setter |
| `RwTexture_FlushState_RGBA8_Path_80498854` — 1271 | `[80498854,80498910)` | Flush control logic with explicitly scoped realization callee |
| `Im2D_Prepare_Textured_RGBA8_Path_8049148C` — 1337 | `[8049148C,80491774)` | Common/textured setup, both viewport choices, projection/matrix setup; not untextured |
| `Im2D_RestoreProjectionViewport_80491774` — 1440 | `[80491774,80491B08)` | Complete root/view and split-scissor logic, not universal GX restoration |
| `Im2D_RenderStrip4_RGBA8_Path_80491E80` — 1520 | `[80491E80,8049245C)` | RW4/count4/textured only; exact four FIFO vertices and restore call |
| `RwTexture_Realize_RGBA8_NoMip_Path_8049AC88` — 1583 | `[8049AC88,8049B568)` | Non-null, nonpaletted RGBA8/no-mip/map0; dirty, sampler-change, unchanged and both load paths |
| `RwRaster_ResolveFormat_BlurCreatePaths_80497458` — 1665 | `[80497458,80497A70)` | `0x505` and observed empty `(0,0,depth0,flags5)` scratch-view paths only |
| `RwRaster_Level0Size_RGBA8_Path_80496478` — 1689 | `[80496478,804965A8)` | RGBA8 level0 only; bounded evidence, not in current function manifest |
| `RwRaster_BackendCreate_BlurPaths_80497CA8` — 1709 | `[80497CA8,80498058)` | Selected roots, empty result, real pixel allocator and both allocation-error reports |
| `RwRaster_BackendSubRaster_804981B8` — 1767 | `[804981B8,804981E8)` | Complete metadata callback; outer parent attachment remains external |

`backend::require_extracted_path` (C:1075-1082) is the preserved **non-original analysis domain assertion**, not an original failure return and not proof of a full backend. Null-texture fallback, untextured/other primitives, other state IDs, palettes, mipmaps, generic raster/freelist/registry/destruction behavior and SDK implementations remain explicit limits. Additional bounded evidence identified by `backend_source_notes.md:33-45` was **not** added to manifests here, as required by ownership.

## Added input definitions and exact entry/continuation contracts

There are **13 new address-bearing input definitions: two complete original helpers and eleven selected slices**. None of the eleven slices claims an original standalone entry. Their `[[nodiscard]] input::Continuation` (H:400-405) is **source control-flow annotation**, not original r3, success, a stub, or a binary-resumption implementation. An omitted arm/tail still has to execute in the actual containing function; returning its PC does not execute it. Actual original live-in registers are named in the signatures/comments. Only the common update consumes incoming f1 in Apply; ApplyRamp loads its strength from T+38 instead.

| Definition (C line) | Containing original range | Implemented portion / remaining continuation |
|---|---|---|
| `MotionBlur_DisableFixedPrefix_Selected_800A3528` — 1876 | `[800A3528,800A35D8)` | Unconditional entry prefix through setter BL356C; stops at **3570**, before non-blur work |
| `MotionBlur_EnableFixedPrefix_Selected_800A3714` — 1904 | `[800A3714,800A37A4)` | Unconditional entry prefix through setter BL375C; stops at **3760** |
| `MotionBlur_ApplyRamp_800A47B0` — 1929 | `[800A47B0,800A4830)` | **Complete** original helper; no invented incoming strength argument |
| `MotionBlur_RampAdvance_Selected_800A4830` — 1973 | `[800A4830,800A492C)` | T+34 branch and T+38 updates/clamps `[4844,48A4)`; stops at **48A4** |
| `MotionBlur_RampSubmit_Selected_800A48C4` — 2004 | Same update container | Actual BL48C4 with live r3=T; stops at **48C8** |
| `MotionBlur_RampRetirement_Selected_800A48F8` — 2013 | Same update container | `[48F8,4918)`, using **post-callback** T+38; stops at epilogue **4918** |
| `EffectParameter_UpdateProgress_8042B63C` — 2032 | `[8042B63C,8042B6CC)` | **Complete** shared helper with actual flags, f1, age, duration and weight transfers |
| `EffectGlobalParam_ConstructKind1_Selected_8043A740` — 2103 | **`[8043A5A4,8043AB68)`**, not A900 entry | Guard/cast/dispatch `[A740,A7A4)` plus full `[A900,A97C)` saved/target construction; stops at AB50/AB54 or exact omitted-kind arm |
| `EffectGlobalParam_ApplyKind1_Selected_80439A8C` — 2160 | `[80439A8C,8043A4B8)` | Common progress first, guard/cast/dispatch `[9A8C,9B10)`, full blur `[9EBC,9FE0)`; stops at A4A4 or exact omitted-kind arm |
| `EffectGlobalParam_FinalizeKind1_Selected_8043A4B8` — 2251 | `[8043A4B8,8043A5A4)` | Null/vptr/cast/dispatch and `[A540,A54C)` **target** write; stops before shared destructor/free at A560, or exact other continuation |
| `MotionBlur_DisablePreservingPrefixA_Selected_8020470C` — 2306 | `[802046BC,80204968)` | `[470C,4760)`, including preceding 8042659C and blur block **`[4714,4760)`** |
| `MotionBlur_DisablePreservingPrefixB_Selected_802055D4` — 2331 | `[8020554C,80205E3C)` | `[55D4,5628)`, including preceding 8042659C and blur block **`[55DC,5628)`** |
| `MotionBlur_DisablePreservingPrefixC_Selected_8020630C` — 2354 | `[802062AC,80206534)` | `[630C,6360)`, including preceding 8042659C and blur block **`[6314,6360)`** |

### Why the ramp pieces are separate

The actual order is advance/clamp -> **unrelated global writes `[48A4,48C4)`** -> ApplyRamp BL48C4 -> **secondary side-effect work `[48C8,48F8)`**, including 8007993C and 8016F4B0 -> reload T+38 and retirement comparison. Those excluded effects are not replaced by made-up entry-point adapters or no-ops. There is deliberately no new single routine that skips them and prematurely checks retirement. The continuation boundaries preserve this ordering obligation.

The immediate numeric actor conditions and source of direction are heavily traced at **C:1952-1972** and `12_closure.md:52-63`: T+34 is initialized to 0 and T+38 to 0 in the new task, not read from current blur alpha. Actor flag **0x15 = actor+A8 mask 00200000**, **0x22 = actor+AC mask 4**. The 800AF60C path requires flag0x15 and `(signed counter converted to f32 <= 0 OR flag0x22 clear)`; the 0x106 message path requires flag0x15 and ORs 2 into message+7. Actor dispatch/constructor bodies are **documented prerequisites**, not extra unrelated actor reconstructions.

### Three disable-block branch scopes

The predicates remain properties of the real containing dispatch, not tests hoisted/repeated after side effects:

- A: incoming self+4 == 0 and `80044E38()==1`, with earlier calls completed (`C:2293-2305`).
- B: self+1C == 0, `80044E38()==1`, then the original **wrapping subtraction** `self+4 -= u32[8058E76C]`, reload, and **signed <= 0** test. Table80544E74 entry0 is the relevant dispatch arm (`C:2320-2330`). The subtraction/earlier effects are outside the selected prefix, not omitted implementations claimed as done.
- C: incoming self+4 == 0 and `80044E38()==1`, with its distinct earlier calls completed (`C:2344-2353`).

Each definition performs **three separate 800A35D8 manager lookups**: before 8042659C, before the parameter getter, and before the setter. **8042659C executes before the getter** through the real-address boundary declared at H:447-450. Its effects are not collapsed into a prefix-only guarantee. The disabled prefix retains alpha/scale/aux **as returned after that callback**, with separate getter-output and setter-input stack records. Only enabled is changed to zero. No assertion is made that the manager lookup returns the same pointer every time.

## Numerical and layout fidelity

- **H:334-397** asserts T+34/+38; resource kind+0/alpha+8/scale+C/aux+10/+14; E+0 vptr, +C age, +18 execution flags, +1C cast source, +FC source flags, +100 weight, +104 duration, +108 resource, +14C saved, +15C target. Observed extents are not whole object/allocation sizes. No C++ RTTI or fake cast-success implementation is used.
- **C:1811-1843** adds `ppc::compare_ordered` using `fcmpo` and `mfcr` in one volatile asm, returning LT/GT/EQ/UN from the high CR0 nibble. `fctiwz_low_word` uses actual `fctiwz; stfd` and a low-word `lwz`. `multiply_add_single` uses actual `fmadds`. These are non-original notation helpers, not new DOL functions. No undefined host floating-to-integer casts or implicit host FMA contraction is used.
- Fixed presets are `(enabled=0/1, alpha=128, scale bits 3F866666)`. Initial temporary `(0,0,1)` stores are retained before replacement. Aux bytes remain unwritten, not zero. Scale is nominal **1.05**, not an inferred amount.
- Ramp step is **FDIVS(incoming f1,0.5)**, followed by its own FADDS/FSUBS and store/reload. Decrease clamps **LT to 0 only**; increase clamps **GT to 1 only**. UN skips both. Enabled is **`!(LT || EQ)`**, including UN -> 1 absent trapping. Alpha is **low8(FCTIWZ(FMULS(64,x)))**, not saturation. Retirement sets T+4 bit0 only on LT or EQ from a fresh comparison.
- Common progress: inactive bit0 or inhibit bit2 returns without changes; bit1 alone advances age. Expiry is strictly **GT(age,FADDS(EPS,duration))**, with EPS bits **38D1B717**; it clears only bit0 and returns **without changing weight**. Otherwise duration GT EPS divides age/duration, else weight=1. Only GT weight>1 clamps; there is no lower clamp. NaN duration takes fallback1, and an unordered quotient may survive.
- Constructor kind1 uses successful RTTI of **E+1C**, not a supplied already-cast substitute. It snapshots via getter into E+14C, then reloads source flags/resource after the getter. **E+FC bit16** supplies target enabled, resource+8 supplies the alpha byte, +C scale, +10/+14 opaque float words. Resource+4 does not select enabled. The earlier shared/mixed construction and outer factory **type8** are not confused with resource **kind1**.
- Apply invokes common progress **first**, then reloads flags and skips the setter after expiry. It copies target to the local prefix before testing **LT(t,snap)**, where snap bits **3F7FF972** differ from the complement's exact1. At or above snap, or UN, the copied target is submitted unchanged. Below snap, enabled is normalized OR of endpoint enabled bytes. Unequal alpha uses exact unsigned-byte 2^52 conversion, **FMULS(saved,FSUBS(1,t))**, then **FMADDS(target,t,product)**, FCTIWZ/low byte; equal alpha bypasses conversion. Scale and aux use the same rounded-product/fused form. No endpoint alpha is replaced with zero merely because its enabled byte is zero.
- Finalization writes **E+15C target**, not E+14C saved, after nonnull self/successful manager cast/kind1 selection. It does **not** test active, inhibit or weight. Shared teardown and conditional deletion are still outside the selected prefix; expiry alone does not prove finalization happened.
- Core `RawWord32` and PPC word/float loads preserve the *fact* of indeterminate stack/allocation bytes without evaluating an uninitialized typed C++ float or inventing zero. They do **not** reproduce the original compiler's stack addresses, residues, NaN payload handling or trap state. Bit-exact replay requires actual machine state.
- No input setter resets B+74/B+78 or pixels. SaveScreen still clears B+78 before its gates. No history reset, capture ordering or producer arbitration is invented.

## Verification actually run

Compiler reports **Clang 21.1.8** at `C:\Program Files\LLVM\bin\clang++.exe`. `--print-targets` lists no PowerPC machine target.

**Strict final C++ syntax: PASS (exit 0)** from workspace `<historical-game-root>/sys`:

```bash
"/c/Program Files/LLVM/bin/clang++.exe" --target=powerpc-unknown-eabi \
  -std=c++17 -ffreestanding -fno-exceptions -fno-rtti \
  -fno-fast-math -ffp-contract=off -Wall -Wextra -Werror \
  -fsyntax-only analysis/motion_blur/reconstructed/motion_blur.cpp
```

**PowerPC LLVM-IR frontend emission: PASS (exit 0)** with the same flags, replacing `-fsyntax-only` by **`-S -emit-llvm -o -`**. A stdin-only Python `subprocess.run(..., capture_output=True, text=True)` driver held stdout in memory and printed the result, target lines, and relevant inline-asm sites. No `.ll`, object, executable, or verification output file was written. It produced **6,317 IR lines**, target triple `powerpc-unknown-unknown-eabi`, datalayout `E-m:e-p:32:32-Fn32-i64:64-n32`. The IR retains the exact `fcmpo/mfcr` (with cr0 clobber), `fctiwz/stfd`, and `fmadds` asm sequences. This is **not** machine assembly/constraint expansion, binary ABI, object/link or runtime validation.

**Original five semantic tests: PASS**, run explicitly from `analysis/motion_blur/tools`:

```bash
python -B -m unittest -v test_motion_blur_semantics.MotionBlurSemanticsTests
```

**Entire already-extended semantic suite: 23 tests PASS** (five original + eighteen added by the independent test role):

```bash
python -B analysis/motion_blur/tools/test_motion_blur_semantics.py
```

This includes 90 geometry fixtures, 1,960 copy-offset cases, 260 finite ramp-alpha fixtures, 640 finite progress fixtures, 240 interpolation fixtures, fused-rounding negative controls, ordered/unordered branches, state12 no-write behavior and GX/data decoding. These tests interpret bounded original instructions and compare readable formulas; **they do not execute the integrated C++, real callbacks, or GPU**. The test interpreter deliberately rejects nonfinite/out-of-range FCTIWZ and does not model full FPSCR/ABI.

**Evidence integrity: PASS**:

```bash
python -B analysis/motion_blur/tools/dol_evidence.py verify
```

All Python commands used `-B`. No verification command required a directory/file write.

### Additional read-only bounded commands

All from the workspace using `python -B analysis/motion_blur/tools/dol_evidence.py`:

- `disasm 0x8043A5A4 0x8043A7A4` — real constructor entry, prior fields and kind guard/cast.
- `disasm 0x8043A900 0x8043A97C` — every snapshot and resource-to-target transfer.
- `disasm 0x802046BC 0x80204768`, `disasm 0x8020554C 0x80205630`, `disasm 0x802062AC 0x80206368` — real dispatch conditions, earlier work, callback and repeated getter/setter contracts.
- `disasm 0x8042659C 0x8042662C` — actual incoming r3 and manager-entry walk/call to 80426440; only boundary identification, not a new full implementation.
- `disasm 0x801D1AD0 0x801D1AFC` — closes the inherited DrawBlurTask allocation provenance note.
- `data 0x805F3BCC 0x805F3BE4`, `data 0x8051CB58 0x8051CB68`, `data 0x8051BFA4 0x8051BFAC` — verifies every added literal against original bytes. 805F3BD8=-0.5 is adjacent but **not** used by the blur ramp; 8051CB60/64 together are the binary64 unsigned bias.

The specialized read/search/edit/write tools handled source and evidence files; no shell file concatenation, rewrite, export, `all`, or `analyze_dol.main()` was used.

## Remaining limits / parent handoff

- **No standalone renderer or game build:** real allocator/C++ RTTI/SDK/task/camera/address adapters remain unresolved bindings. The new 8042659C declaration requires genuine shared-effect behavior, not a stub. Mixed FPR/GPR adapter contracts and original calling convention still need validation before any linkage attempt.
- **No hidden fallback completion:** input continuations intentionally expose the excluded constructor prefix, fixed-producer tails, ramp side effects, other resource kinds, and shared destruction/free. Actual dispatch preconditions must be honored; the eleven slices are not callable replacements for whole containers.
- **No generic backend claim:** live callbacks, inherited samplers, TEV swap/alpha-write/cull/depth state, valid runtime pointers/plugin offsets and allocation success remain external. Default modulate alpha is texture-alpha times vertex-alpha subject to inherited state, not unconditional blurAlpha/255. No missing reset is manufactured to force visibility.
- **Static source only:** PPC machine backend is absent. Object generation, assembly validity after register allocation, linking, captured runtime traces, GPU completion, visual output and temporal history age are **not verified**. No such verification outputs were written.
- **Actual unknown inputs:** original stack residues, FPSCR/nonfinite conversion results, actor/descriptor instances, delta values/units, timer/inhibit writers, producer arbitration and lifecycle scheduling remain unknown, not newly defaulted.
- Current manifests remain untouched. The already-recorded backend evidence gaps (level-size helper and three small data spans in `backend_source_notes.md:33-45`) are bounded-read provenance, not fresh manifest entries. Any future export or broader SDK/runtime work needs separately authorized ownership; it was not folded into this integration.

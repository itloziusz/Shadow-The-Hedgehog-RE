# Constructor FP-order audit resolution

## Scope and revision

**F1 in `final_source_audit.md:23` is resolved at the source object-event/FP-arithmetic ordering level.** The earlier audit is preserved unchanged as a historical checkpoint. This note does not claim CPU-register, original-stack, exception-handler/resumption ABI, or whole-machine fault equivalence.

Only two files were written for this authorized follow-up:

- `analysis/motion_blur/reconstructed/motion_blur.cpp` (**C** below): expanded the first full-vertex transfer and updated its directly related trace comments. No other executable constructor statements or shared/backend/input bodies were changed.
- `analysis/motion_blur/evidence/source_audit_resolution.md`: this new report.

| CPP revision | Lines | SHA-256 |
|---|---:|---|
| Before this fix, actually re-read before editing | 2,372 | `9949f8b9809cc90f425dced1971fd03c3fdad88cf4ecf7f5e1313fad56853bac` |
| After fix and verification | **2,393** | **`3d2cb9dca2a2b4f220adfb5c16163d9f8e61229b44d89f701a20564261b8ad1b`** |

Changed source regions: **C:139–143** (helper's usage restriction), **C:481–518** (expanded/annotated first full vertex and remaining-copy trace). Constructor is now **C:428–579**. The exact replacement added 21 lines; old CPP lines 498 onward move by +21. For the parent's documentation updates, unchanged backend fragment now occupies **C:810–1814** (`fragment line + 809`), input integration **C:1816–2393**. No old report/manifests/header/test file was edited to adjust line references.

## Original counterexample and correction

The old source invoked `copy_vertex_storage(full[0], state->draw_vertices[0])` before all six height-UV arithmetic operations. Original `[8042EC24,8042ECB4)` instead interleaves object reads/writes with those operations.

Counterexample from the audit: valid camera/engine pointers, root **W=2/H=0**, finite near-Z, old allocation bytes `A5`, and precise enabled divide-by-zero exception delivery. Earlier width arithmetic is exact for W=2. Original **8042EC78** divides `0.5 / 0` before **8042EC84/8042EC94** write B+EA/B+EB (first full vertex's blue/alpha), so those bytes still contain `A5` at that boundary. The old grouped source had already copied `FF/FF` there.

Corrected **C:500** performs that division **before C:501–505** load/store blue and alpha. B+EA/B+EB therefore remain untouched if the division does not return. No zero-dimension guard, initialization, fake error, or changed arithmetic was introduced. The first U load also stays **before** the second division, and its destination store **after**, as required by **EC98/ECA0/ECA4**. Raw X/Y/Z/U/V transfers still use `ppc::load_single` / `ppc::store_single`, not evaluation of indeterminate C++ float objects.

This is a static counterexample/resolution, **not a captured hardware exception**. Live FPSCR/MSR settings, compiler machine lowering and exception ABI remain unverified.

## Exact first-vertex event correspondence

B is the original blur-object base. The table includes all **24 object-load/store and FP-arithmetic events** in the selected original range, excluding original stack-address construction, stack stores and constant/temporary LFD loads. Integer-to-FPR conversions continue using the existing PPC helpers, not host float casts.

| New C line | Original PC | Event now at this position |
|---:|---|---|
| 487 | 8042EC24 | LFS draw X, B+14 |
| 488 | 8042EC30 | STFS full X, B+DC |
| 489 | 8042EC34 | LFS draw Y, B+18 |
| 490 | 8042EC3C | STFS full Y, B+E0 |
| 491 | 8042EC44 | LFS draw Z, B+1C |
| 492 | 8042EC48 | Signed-height FSUBS |
| 493 | 8042EC50 | STFS full Z, B+E4 |
| 494 | 8042EC54 | FADDS bottom numerator |
| 495 | 8042EC58 | LBZ draw red, B+20 |
| 496 | 8042EC60 | STB full red, B+E8 |
| 497 | 8042EC68 | LBZ draw green, B+21 |
| 498 | 8042EC6C | First unsigned-height FSUBS |
| 499 | 8042EC74 | STB full green, B+E9 |
| 500 | 8042EC78 | FDIVS v0 |
| 501 | 8042EC7C | LBZ draw blue, B+22 |
| 502 | 8042EC84 | STB full blue, B+EA |
| 503 | 8042EC8C | LBZ draw alpha, B+23 |
| 504 | 8042EC90 | Second unsigned-height FSUBS |
| 505 | 8042EC94 | STB full alpha, B+EB |
| 506 | 8042EC98 | LFS draw U, B+24 |
| 507 | 8042ECA0 | FDIVS v1 |
| 508 | 8042ECA4 | STFS full U, B+EC |
| 509 | 8042ECA8 | LFS draw V, B+28 |
| 510 | 8042ECB0 | STFS full V, B+F0 |

**Optimized-IR cross-check: PASS, 24/24 in this exact order.** A stdin-only Python checker consumed Clang's `-O2 -fno-discard-value-names` IR, isolated the constructor, resolved its constant `%state` GEP offsets, and classified object LFS/STFS/LBZ/STB plus FSUBS/FADDS/FDIVS events through the first B+F0 store. It compared that list with events decoded from a fresh hash-checked `dol_evidence.py disasm 0x8042EC24 0x8042ECB4` subprocess. Equality and count=24 were asserted; it separately asserted that neither B+EA nor B+EB's store precedes the first divide. This is a frontend event-order check, not PPC execution or a new persistent test file.

## Other constructor interleavings re-checked

Fresh original instructions were re-read across the **whole `[8042EA50,8042F084)` constructor**. No further directly proved object-write/across-FP-arithmetic mismatch was found in the remaining segments:

| Original segment | Current C | Result |
|---|---|---|
| EA50–EC24 | 428–478 | Prefix/near-Z/color/history-valid writes still precede the width arithmetic. The six width FSUBS/FADDS/FDIVS operations remain ordered before the first full-vertex transfer. No guards or defaults moved. |
| ECB4–EDA8 | 512–518 | Remaining three draw-to-full copies have **no intervening FP arithmetic**; existing copy statements remain ordered and now carry individual original range comments. ED64's zero-literal LFS is not FP arithmetic. No claim is made to reproduce all constant/stack memory-access fault ordering. |
| EDAC–EE38 | 520–536 | Existing full-size XY/UV stores already bracket the signed conversions at **EDD0, EDF0, EE14, EE2C** correctly. Left untouched. |
| EE3C–EF80 | 538–547 | Signed integer halving remains toward zero. The four full-to-half copies contain no intervening FP arithmetic and remain unchanged. |
| EF84–F008 | 549–559 | Existing half-size XY stores already follow/interleave with their conversions at **EFA0, EFB8, EFEC, F004** correctly. Left untouched. |
| F00C–F084 | 561–579 | Real raster allocation/attachment calls, optional parameter copy and observed default stores are unchanged. No new local FP-arithmetic/write reordering introduced. |

The grouped helper is retained only for the uninterrupted vertex transfers. Its warning at **C:139–143** prevents reintroducing F1 by collapsing the first interleaved transfer. Reconstruction temporaries still do **not** reproduce original SP offsets or CPU registers.

## Commands and results

Working directory for all commands: `<historical-game-root>/sys`. All Python commands used `-B`; disassembly, tests and compiler-output checks wrote no files.

### Fresh original evidence

```text
python -B analysis/motion_blur/tools/dol_evidence.py disasm 0x8042EA50 0x8042EC24
python -B analysis/motion_blur/tools/dol_evidence.py disasm 0x8042EC24 0x8042EE5C
python -B analysis/motion_blur/tools/dol_evidence.py disasm 0x8042EE5C 0x8042F084
python -B analysis/motion_blur/tools/dol_evidence.py disasm 0x8042EC24 0x8042ECB4
python -B analysis/motion_blur/tools/dol_evidence.py verify
```

All succeeded. The fourth query was invoked by the IR event checker. Verification reported **PASS: 81 function ranges, 7,249 instruction words, 3,626 file-backed data bytes**, SHA-256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`. This records the manifest snapshot at this verification call; the parent owns any subsequent manifest/doc updates.

### Strict PowerPC syntax and LLVM IR

Actual syntax invocation (**PASS, exit 0**):

```text
"C:/Program Files/LLVM/bin/clang++.exe" --target=powerpc-unknown-eabi -std=c++17 -ffreestanding -fno-exceptions -fno-rtti -fno-fast-math -ffp-contract=off -Wall -Wextra -Wpedantic -Werror -fsyntax-only "analysis/motion_blur/reconstructed/motion_blur.cpp"
```

IR invocations used the same compiler/target/strict flags and CPP path, replacing `-fsyntax-only` with:

- **`-S -emit-llvm -o -`**: PASS, **6,419 IR lines / 324,769 characters**.
- **`-O2 -fno-discard-value-names -S -emit-llvm -o -`**: PASS, **4,312 IR lines / 277,880 characters**. Repeated for the independent 24-event assertion above; PASS.

Each was run under `set -o pipefail`, piped to `python -B -c` reading stdin. Drivers checked the PowerPC target triple, retained PPC asm strings and absence of **fptosi/fptoui/fptrunc**; the optimized inspection also printed the constructor window. Output was consumed in memory, not saved as `.ll` or object files. Inline-assembly memory barriers and the requested first-vertex ordering survive the checked optimized IR.

### All existing 23 tests

```text
python -B analysis/motion_blur/tools/test_motion_blur_semantics.py
```

**PASS, exit 0: 23 tests in 0.214s** — five original plus eighteen extended tests. The suite covers original geometry, copy offsets, state/GX data, ramp/progress/interpolation and finite/ordered/unordered negative controls. **These existing tests do not execute the CPP constructor or simulate precise FP exception delivery.** The new change's specific event order was additionally verified against optimized IR and fresh raw instructions as described above; no test file was added or modified.

### Integrity / scope checks

Before and after, ran SHA-256 `Get-FileHash` over the DOL, CPP, header, existing test file, original audit and backend fragment. These protected files remained byte-identical:

| Read-only artifact | Unchanged SHA-256 |
|---|---|
| `main.dol` (5,773,024 bytes) | `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af` |
| `reconstructed/motion_blur.h` | `c1aefedcd422b5f16a14c4f461462c81725a6092fb17144b1966df8001328534` |
| `tools/test_motion_blur_semantics.py` | `f2551537406a726045993d848f607a900515e710bfc88c596fa70fe7abe1c84b` |
| `evidence/final_source_audit.md` | `d04fb5c0bf5bde958f02e3345a233ee7d9fc13568ab016eb094733d40fbe3499` |
| `evidence/backend_reconstructed.inc` | `5ed0ebfe565a7119becf96bd418f630ce17f70f927c375cdead8b7e4055e6118` |

Specialized read/search/edit/write tools handled the two authorized deliverables. No export, DOL/config/header/test/manifest edit, SDK installation, binary patch, link or runtime run was performed by this role.

## Remaining limits — unchanged, not silently "fixed"

- **F2 remains:** backend `require_extracted_path` / `__builtin_trap` at **C:1100–1103** is a **non-original domain assertion**, not recovered behavior or a generic SDK callback. Excluded untextured/non-RGBA8/mipmap/palette/general-renderer cases are not invented or marked complete.
- **F3 remains:** input `Selected_*` continuations are source annotations, not executable original-register/stack continuations or full containing entries. No omitted mixed-effect/SDK behavior is replaced with fake success.
- The installed LLVM lacks a PowerPC machine backend. **No PPC object was produced; no object attempt was repeated here.** Syntax and LLVM-IR checks do not validate final instruction assembly/register allocation, original calling convention, hardware exceptions, GPU output or runtime behavior. Original stack residue, FPSCR/MSR, live pointers and external ABI bindings remain explicit requirements.

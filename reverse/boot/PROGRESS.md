# Native boot execution log — PAL GUPP8P

This log tracks the **connected** native boot prefix. Static maps and callable
research slices do not count as a new checkpoint until original-state and
end-to-end execution comparisons pass. Original fixture:
`main.dol` SHA-256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.

## 2026-09-30 — checkpoint 27: first HID2 read and rerun diagnosis

| Field | Result |
|---|---|
| Last fully validated connected boundary | Starting at `0x80003154`, the native probe with **caller-supplied** synthetic/HLE MSR `0x00002032` and HID2 readback `0xE0000000` reaches PC `0x80371724`, before executing that word. Default probe still stops at `0x80003400`; supplying only MSR stops before the HID2 accessor at `0x80370BA8`. |
| New machine-state transition | Four raw words `0x80371714..20` save LR, emit ordered BE32 stack writes at `0x8060C5F4` and `0x8060C5E8`, decrement r1, and call `0x80370BA8`. Its pinned two-word `mfspr hid2; blr` leaf returns r3 from an explicit input. Fresh synthetic/HLE captures at both boundaries confirm PC/LR/r0/r1/r3/r31/MSR and the stack bytes. The native write events are not yet applied to a downstream memory model. |
| Adversarial error fixed | The previous raw gate could omit `0x80003404` and insert another valid DOL word while preserving all counts. `tools/verify_binary_note.py` now requires exact aligned VA sets for each note, checks all direct branch annotations and CRT descriptor bytes, and `boot_pal_raw_coverage_negative` proves that substitution is rejected. An independent audit is in `research/ADVERSARIAL_BOOT_AUDIT.md`. |
| Explicit rerun result | The Release probe exited 0 with **no current execution or modeled-state error**. Boot-only CTest passed **12/12** and complete Release CTest **41/41**. `research/BOOT_RUN_DIAGNOSTIC_2026-09-30.md` records the exact outputs, reference comparison, limitations and next stop. No pixel/game boot claim follows. |
| First unimplemented instruction | `0x80371724`, DOL bytes `64 63 A0 00` (`oris r3,r3,0xA000`), precedes a call to the HID2 writer. Post-write HID2, HID0 ICFI, GQR/FPR/cache, retail IPL inputs, unmodeled CR/XER/CTR/FPR and downstream stack readers remain unvalidated. Continue from this exact word without forcing a reference value into the native implementation. |

## 2026-09-30 — checkpoint 26: binary-first FPR, cache and CRT records

| Field | Result |
|---|---|
| Last connected native checkpoint | Unchanged: ordinary probe stops at `0x80003400`; with caller-supplied synthetic MSR `0x00002032`, it stops at `0x80371714` before HID/GQR effects. Both paths reran from `0x80003154` after the evidence-gate changes. |
| New binary coverage | `research/BINARY_FPR_PREFIX.md`, `BINARY_CACHE_HANDLER.md` and `BINARY_CRT_PREFIX.md` record 74, 305 and 111 instruction words respectively; CRT also records 41 descriptor words. With the prior entry note, four byte gates check 568 rows, 90 direct branch targets and 32 SPR encodings against the SHA-pinned DOL. The notes retain register, CFG, memory and hardware dependencies. |
| First validation gap diagnosed | The initial byte checker accepted a note that omitted a direct-branch annotation. The missing `blt` at `0x8000544C`, `bdnz` at `0x803734AC`, and CR1 `bne` at `0x80370C90` were identified. The checker now requires a target for every I/B direct branch row, decodes those predicates, and checks CRT descriptor bytes. In-memory missing-branch and changed-descriptor mutations were rejected. |
| Multi-stage result | DOL SHA/mapping/bytes/BE words and the recorded branch/SPR/data fields pass the raw gate. Independent decoder/`q.py` comparisons and local ISA/state analyses are in the linked notes. **Connected native state, PS1, cache-command consumers and retail IPL parity have not been established for these new regions.** |
| Regression | MSVC Release build and **38/38 CTest passed**, including four PAL raw-note gates and all prior gameplay, asset, runtime and bounded boot cases. Direct default and caller-supplied-MSR probes reached their earlier exact stops. No pixel/game boot claim follows from these gates. |
| Next divergence | Resume at the first unimplemented instruction `0x80371714` with explicit pre-entry HID2/HID0/GQR/MSR/memory state. Validate paired/ICFI consequences and ordered writes against a same-run reference before any C++ lowering. The CRT path remains static until that hardware return is connected. |

## 2026-09-30 — checkpoint 25: binary-first correction and evidence gate

| Field | Result |
|---|---|
| Connected native boundary | Unchanged: normal probe stops at `0x80003400`; with caller-supplied synthetic MSR `0x00002032`, the five-word wrapper prefix stops at `0x80371714`. No HID/GQR/FPR/cache instruction has been claimed native-equivalent. |
| Raw authority | `research/BINARY_BOOT_PREFIX.md` records all 78 words of entry, register helper, hardware wrapper, paired setup and HID leaves with file offsets, four bytes, BE word and independently decoded fields. `tools/verify_binary_note.py` rechecks 78 words, 8 branches and 17 SPR operands against the exact DOL; a wrong expected count fails. |
| First newly diagnosed error | The old shorthand `HID0_after = HID0_before | 0x800` is false for an enabled instruction cache. Raw `0x803725F8/FC` writes the ICFI command; the Gekko manual says it self-clears on the next cycle when ICE is set. The synthetic reference writes `0x0011CC64` and later reads `0x0011C464`. Corrected the static hardware notes before any native HID0 model could absorb the error. |
| State/memory archaeology | `research/BOOT_MEMORY_TIMELINE.md` traces stack, FPR source, handler slot, low-memory BI2/FST words and CRT zeroing. The handler store before CRT is erased by the first zero range; a later OS call can reinstall it. The paired helper's saved LR and back-chain were observed at `0x80371724..0x80370CDC` in a fresh synthetic run. |
| Validation | Release build and **35/35 CTest passed**, including `boot_pal_raw_prefix`, prior gameplay/runtime/asset gates and connected HLE-input prefix; direct probe reran from `0x80003154`. This does not establish the later frame/pixel parity gate. An unfinished HID/GQR C++ draft was removed before validation. The raw record's bitfield prose had a swapped-SPR-half wording error, corrected and rechecked. |
| Next unresolved boundary | Binary-first per-word FPR/cache/CRT records are in progress. After those, establish HID0 ICFI/`sync` and PS1/cache consequences at the first consumer before extending connected C++. Retail IPL state and complete boot/game transition remain UNKNOWN. |

The proof ladder and region coverage are indexed in `ARCHAEOLOGY_INDEX.md`.
The startup-only Dolphin trace validates its own HLE path; it is not a retail
console/IPL or full hardware oracle. Keep raw bytes, ISA semantics and
hardware readback distinct from the native projection at every checkpoint.

## 2026-09-30 — checkpoint 24: assembly-first hardware entry

| Field | Result |
|---|---|
| Last fully validated checkpoint | `0x80003158` after all 36 register-helper instructions; the prior PC/GPR/LR state remains checked. |
| New connected checkpoint | `bl 0x80003158 -> 0x80003400` is translated, then the five pinned instructions `0x80003400..0x80003410` reach `0x80371714` when an observed MSR is supplied. The synthetic HLE run supplies `MSR=0x00002032`; native reaches `PC=0x80371714`, `LR=0x80003414`, `r0=0x2032`, `r31=0x8000315C`, matching its same-run PPC checkpoint. The normal probe without an MSR stops at `0x80003400`. |
| First fail-closed stop | `0x80371714`, before the HID2/HID0/GQR helper. Its initial HID2 and complete paired FPR state are not captured directly by the GDB interface. The selected synthetic path is now instruction-mapped and observed through hardware return, but the native path has not executed it. |
| Root cause and evidence | The previous stop at `0x80003158` was a deliberate missing-state boundary, not an observed crash. A previously archived startup-only synthetic disc was reverified against all five PAL `sys/` files and captured with a local Dolphin interpreter. Two fresh-user runs reached `0x8000315C` with identical exposed state. This is an HLE startup oracle, **not** retail IPL proof. `research/ASM_HARDWARE_CFG.md`, `ASM_HARDWARE_DATAFLOW.md`, `ASM_HARDWARE_VALIDATION.md` and `ASM_CRT_ENTRY.md` provide independently checked assembly, data-flow, branch and state evidence. |
| Adversarial findings | Corrected a text-section address typo in the CFG note. The HID2 branch needs rotate-left-3 (plain shift fails for observed `0xE0000000`); the guest explicitly clears the L2 invalidate request. The CRT walker overwrites the upper startup stack sentinel with saved LR and later zeroes the handler slot installed by hardware. These are documented before any corresponding C++ continuation. |
| Regression | Complete Release build; **34/34 CTest passed** with exact read-only PAL DOL, including new `boot_pal_synthetic_hle_prefix`, all previous gameplay/runtime/asset cases, mutation gates for each translated hardware-prefix word, and direct probe rerun from `0x80003154`. |
| Remaining unknowns | Retail IPL pre-entry state and FST placement; full PS1 and cache internals; native HID/GQR/paired-FPR/cache/handler effects and exact volatile register/CR/stack parity; CRT and OS connected execution; constructors and game/pixel transition. |

The next connected block is `0x80371714..0x80371764`. Implement its exact
ordered stack, HID2, HID0, GQR and register effects only after supplying an
explicit hardware-state interface; rerun from `0x80003154` and compare the
first reachable reference checkpoint. Do not infer retail constants from the
synthetic HLE run or skip the L2 polling dependency.

## 2026-09-30 — previous run

| Field | Result |
|---|---|
| Last fully validated checkpoint | Entry `0x80003154` and all 36 instructions of register helper `0x800032B0..0x8000333C`. Native PC/LR stop at `0x80003158`; r1=`0x8060C5F0`, r2=`0x805FA780`, r13=`0x805EC500`. |
| New connected checkpoint reached | None. |
| First stop / original continuation | Native stops **before** call `0x80003158 -> 0x80003400`. Original callee starts `mfmsr` at `0x80003400`, sets FP enable and calls `0x80371714`, `0x80370CDC`, `0x80372838`. |
| Root cause | No original same-run pre-entry MSR/HID0/HID2/L2CR and relevant memory/branch trace is available. The later FPR source `0x805F1F30..3F` is absent from DOL sections. Apploader state 6 clears the containing BSS interval on the ordinary completed DOL path, but later FST placement and external writes depend on IPL state, so final entry bytes are not yet established. |
| Native behavior and evidence | `shadow_boot_probe` runs the exact pinned entry/helper prefix and declines before hardware. `research/APPLOADER_PATH.md`, `HARDWARE_PATH.md` and `OS_STARTUP.md` map the original dependencies. A proposed separate FPR note was rejected by automatic approval review under the earlier gameplay-only path restriction; its evidence was reported in this task instead. |
| Execution evidence | Direct Release `shadow_boot_probe` output: `STOP pc=0x80003158 lr=0x80003158 r1=0x8060C5F0 r2=0x805FA780 r13=0x805EC500`. Full public MSVC Release CTest passed **33/33** against read-only game data, including the new `boot_fpscr_arithmetic` gate; the direct probe was rerun after the gate. |
| Remaining unknowns | Original IPL low memory `[0x80000028]`, final FST destination, hardware SPR values and branch outcomes, pre-entry FPR source after all loader writes, time base and OS device responses, constructor effects, application event outcomes and frame/pixel parity. |

The next attempt must start from the same entry, compare the original state at
`0x80003158` and inside `0x80003400`, then implement only the effects whose
inputs and ordered outputs are known. Run the full prefix and CTest after each
correction. Do not jump to the structurally mapped application event loop to
claim boot progress.

### Diagnosed reference-only bug

The archived `hardware_semantics.cpp` copied the low 32 bits of `f0` directly
to FPSCR for `mtfsf 0xFF,f0` at `0x80370DFC`. The IBM Gekko manual specifies
that FEX and VX are derived summary bits. The corrected helper clears source
bits 1/2 and derives them from the exception flags and enables. The executable
unit regression includes finite `f0=0x3FF0000040000000`: raw-copy yields
`0x40000000`, but the defined FPSCR result is zero. This corrects arithmetic
in a reference-only component and does **not** advance the connected boot PC.
The same helper now declines exceptional PS1 source encodings, including
denormals, rather than silently treating raw source bits as a proven
post-`psq_l` result. The 33/33 full gate and direct boot probe were rerun after
that change.

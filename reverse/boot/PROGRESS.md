# Native boot execution log — PAL GUPP8P

This log tracks the **connected** native boot prefix. Static maps and callable
research slices do not count as a new checkpoint until original-state and
end-to-end execution comparisons pass. Original fixture:
`main.dol` SHA-256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.

## 2026-09-30 — checkpoint 31: stack writes become addressable guest bytes

| Field | Result |
|---|---|
| Last validated connected boundary | Unchanged: with explicit synthetic/HLE MSR/HID2/HID0 inputs, the probe starts at `0x80003154` and stops **before** `sync` at `0x80371730`. Default and partial-input paths still stop earlier. |
| First unmodeled dependency addressed | The `0x80371718/1C` stores were previously only ordered write events. The later `lwz r0,12(r1)` at `0x80371758` must read actual guest bytes; using a remembered LR would be invalid if the saved word changed. `PairedSetupStackMemory` now applies the two BE32 stores to a validity-tracked 16-byte stack window and carries it through the HID2/HID0 boundaries. |
| Independent evidence and adversarial check | Raw words and the same-run synthetic stack window at `0x80371724` agree on `80 00 34 14` at `0x8060C5F4` and `80 60 C5 F0` at `0x8060C5E8`. Tests reject unwritten, unaligned and out-of-window reads/writes; a changed saved word reads back changed rather than a fixed return. `research/PAIRED_STACK_MEMORY.md` records scope and limits. An independent sync audit confirmed the next GQR tail is straight-line but the barrier still needs a completed ICFI/ordering contract. |
| Rerun | The native probe was run from `0x80003154` before the change and again after the memory application; it still stops at `0x80371730`. After the final alignment refinement, the complete MSVC Release build and root CTest passed **44/44**. The original DOL remained read-only. |
| Next proof obligation | Establish a stateful ICFI/`sync` completion and separately derived HID0 readback for a bounded ICE=1/ABE=0 path, then compare the eight ordered GQR writes and state at `0x80371758` to a same-run oracle. ICE=0, ABE=1, interrupts and retail inputs remain UNKNOWN and must decline. |

## 2026-09-30 — checkpoint 30: six-agent wall audit and privilege correction

| Field | Result |
|---|---|
| Last validated connected boundary | Unchanged: the native probe starts at `0x80003154` and, with explicitly supplied synthetic/HLE MSR, HID2 and HID0 values, stops **before** `sync` at `0x80371730`. Default and partial-input stops remain earlier. |
| Earliest divergence found and fixed | An adversarial run with user-mode MSR `0x00004000` previously passed `mfmsr` at `0x80003400`, fabricated a state at `0x80371714`, and emitted stack writes. Raw `0x7C0000A6` and the 750-family privilege rule show the first instruction should trap. `EnterPairedSetupCall()` now declines before it; `ReturnFromHid2Read()` separately declines a user-mode HID2 `mfspr`. Focused API cases and `boot_pal_user_mode_decline` enforce both earliest boundaries. |
| Six independent wall studies | `SYNC_GQR_CHAIN.md` checks all 14 tail words and nine SPRs; `ICFI_SYNC_BOUNDARY.md` shows why a logged barrier event is insufficient; `PAIRED_FPR_BREAKTHROUGH.md` adds a six-point synthetic PS0 capture but confirms stock GDB cannot expose PS1; `L2_STATUS_ORACLE.md` observes both L2IP polls clear in one synthetic run without inferring retail timing; `OS_ENTRY_NEXT_BOUNDARY.md` independently checks 63 words and traces the time-base/low-memory offset dependency; `NATIVE_PREFIX_ADVERSARIAL_29.md` records the two corrected privilege gaps. All are bounded evidence, not connected native equivalence. |
| Rerun | Full MSVC Release build and **44/44 CTest** passed after the fixes. The positive HLE-input probe still reaches the same `0x80371730` stop; the user-mode probe now exits nonzero with no later checkpoint or stack-write event. The game inputs stayed read-only. |
| Exact next proof obligation | `sync` must acknowledge ICFI completion and preserve instruction visibility/ordering before `0x80371734`. The native state has a write operand, not a completed cache/barrier effect or separately derived HID0 readback. Retail ICE/ABE, PS1, L2 timing and OS time-base/low-memory inputs remain UNKNOWN. Do not advance by incrementing PC past a log-only event. |

## 2026-09-30 — checkpoint 29: HID0 ICFI request and full rerun

| Field | Result |
|---|---|
| Last connected boundary | From `0x80003154`, with explicit synthetic/HLE MSR `0x00002032`, HID2 `0xE0000000` and HID0 `0x0011C464`, the native probe reaches PC/LR `0x80371730` and stops **before** `sync`. It reports an HID0 SPR1008 write request `0x0011CC64`; it does not equate this with the hardware-cleared readback `0x0011C464`. Earlier default and two-input stops remain supported. |
| Root-cause trace | The prior stop `0x8037172C` was a deliberate missing live-HID0 input. DOL `bl 0x803725F4` enters `mfspr HID0; ori 0x0800; mtspr HID0; blr`, producing the request operand and return LR. The synthetic interpreter capture confirms the exposed r3/PC/LR at `0x80371730` while separately showing the self-cleared HID0 register. The binary and Gekko manual distinguish command write from readback. |
| Adversarial validation | All five executed words and the next `sync` boundary are fingerprinted and individually mutated in the negative test. A distinct HID0 input proves the OR retains unrelated bits; user-mode SPR access and invalid entry PC decline. The full native prefix reran from entry; MSVC Release CTest passed **43/43**. |
| First remaining divergence/unknown | `0x80371730` is `sync`, and subsequent GQR writes plus FPR/paired, L1/L2 and CRT effects are not connected. In particular ICFI completion/cache-tag state, retail HID0/ICE input and actual HID2 readback remain unknown. Do not replace the request event with a persistent HID0 flag or skip the barrier. |

## 2026-09-30 — checkpoint 28: HID2 write request and six independent slices

| Field | Result |
|---|---|
| Last fully validated connected boundary | Starting at `0x80003154` with caller-supplied synthetic/HLE MSR `0x00002032` and HID2 read `0xE0000000`, the native probe now reaches PC/LR `0x8037172C` and stops **before** the HID0 call. The default stop remains `0x80003400`; MSR alone stops before the HID2 read at `0x80370BA8`. |
| New native transition | Raw `0x80371724/28` ORs `0xA0000000` into live r3 and calls `0x80370BB0/B4`, which issues `mtspr HID2,r3` and returns. The C++ boundary reports SPR 920 and its requested word; it does not assert later hardware readback. A fresh synthetic/HLE checkpoint at `0x8037172C` agrees on exposed PC/LR/GPR state. A distinct HID2 input proves unforced bits are preserved. All four instructions have mutation gates and user-mode SPR access declines. |
| Six separate archaeology results | `HID2_WRITE_BOUNDARY.md`, `HID0_ICFI_CONSUMERS.md`, `FPR_LANE_PROVENANCE.md`, `L2_CACHE_STATE.md`, `CRT_MEMORY_EXECUTION.md` and `CONSTRUCTOR_NEXT_TARGETS.md` cover different boot workstreams. HID0 ICFI readback, both L2 status polls, paired PS1 provenance, CRT fills and constructor table indices 12–15 are documented without implying connected native parity. |
| Genuine error and root cause | The CRT return note had incorrectly said r4=0 and r31=0. Exact updating stores leave `r4=0x805FC5E8`; the hardware wrapper saves outer LR `0x8000315C` in r31 and the walker restores it. Fresh HLE return state confirms both. The HLE GDB XER field is a stale SPR word and cannot refute the byte-derived final `addic.` carry. The corrected note and `boot_pal_crt_return_projection` test derive the two register values and reject a changed descriptor. |
| Full rerun | MSVC Release full-tree CTest **42/42** passed. The standalone gameplay script initially lacked a way to use separately stored read-only files; it now accepts `SHADOW_GAME_FILES_DIR`. With that path supplied it passed **27/27** gameplay CTest and stg0100 Dark 35/35 → stage index 6. The probe itself was rerun from entry and printed the exact new stop. |
| First unresolved instruction | `0x8037172C` is `bl 0x803725F4`, whose first instruction reads live HID0. Its power-on/retail value, ICE-dependent ICFI self-clear and later cache consumers need explicit provenance and state comparison before native continuation. Stack write events still require a memory application model before CRT. |

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

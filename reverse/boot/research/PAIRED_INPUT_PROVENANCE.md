# Inputs to the first paired-single boot call

Scope: PAL GUPP8P `main.dol`, from entry `0x80003154` through the paired-call
entry `0x80371714`, with the first consumers in
`0x80371714..0x80370E00`. **PROVEN** means checked against the original DOL
bytes and their instruction fields. **OBSERVED-HLE** means a pinned synthetic
startup-only Dolphin run; it does not establish retail IPL state. **UNKNOWN**
is an input or hardware consequence for which neither kind of evidence is
sufficient. No C++ behavior is inferred from a convenient reference value.

The read-only DOL SHA-256 is
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
Its text0 maps file `0x100` to `0x80003100`; text1 maps file `0x2600` to
`0x80008D40`. The independent byte check below used those mappings, then
cross-checked [the complete 78-word prefix](BINARY_BOOT_PREFIX.md) and
[74-word FPR routine](BINARY_FPR_PREFIX.md) with the read-only
`verify_binary_note.py` gate. Both passed (`78/8/17` and `74/1/1` raw
words/branches/SPRs respectively).

## Connected producer-to-consumer graph

| Input or state | Raw word anchor and exact producer | First relevant consumer | Status at `0x80371714` |
|---|---|---|---|
| PC/LR | File `0x154`: `4800015D` calls `0x800032B0`; file `0x158`: `480002A9` calls `0x80003400`; file `0x410`: `4836E305` calls `0x80371714`, setting LR=`0x80003414`. | `mflr r0` at `0x80371714`; saved by `stw r0,4(r1)` at `0x80371718`. | **PROVEN** connected direct-call chain; no inferred ABI return address. |
| Stack pointer | File `0x324/328`: `3C208060`, `6021C5F0` set r1=`0x8060C5F0` inside entry helper. The helper does not read or write RAM. | `0x80371718` stores LR at `0x8060C5F4`; `0x8037171C` sets r1=`0x8060C5E8` and stores old r1 there. `0x80371758` reads saved LR at new r1+12. | **PROVEN** address and ordered stores. Pre-existing bytes and retail writable-memory mapping are **UNKNOWN**. This stack is above DOL BSS end `0x805FC5EC`, so DOL BSS zeroing cannot initialize it. |
| MSR | File `0x400/404/408`: `7C0000A6`, `60002000`, `7C000124`: read full incoming MSR, OR `0x2000`, write full MSR. No instruction from `0x80003154` to this read produces the incoming word. | FP permission for this call and later `mfmsr` at `0x80370CDC`; `mtmsr` there repeats the FP OR. The L2 path later saves and restores the full word. | **PROVEN** transform `MSR_at_call = MSR_at_DOL_entry OR 0x2000` if `mtmsr` completes. Initial full MSR, privilege/exception context, and retail effects are **UNKNOWN**. |
| HID2 | Text1 file `0x36A468`: `7C78E2A6` reads SPR 920 at `0x80370BA8`; `0x80371724` word `6463A000` ORs `0xA0000000`; file `0x36A470`: `7C78E3A6` writes SPR 920. Swapped five-bit SPR decode gives 920, independently of a mnemonic. | Live HID2 read at `0x80370CE8`; `rlwinm.` at `0x80370CEC` computes `rotl32(HID2,3)&1`, then `beq` at `0x80370CF0` selects the paired path. HID2[LSQE/PSE] also gates `psq_l`. | **PROVEN** write request `HID2_in OR 0xA0000000`; all unforced bits, retail input/readback, and hardware enable timing are **UNKNOWN**. The requested mask forces numeric bits 31 and 29, not bit 30. |
| HID0 | Text1 file `0x36BEB4/BEB8/BEBC`: `7C70FAA6`, `60630800`, `7C70FBA6` read SPR 1008, OR `0x800`, write it. The `0x8037172C` direct call reaches this four-word helper. | The ICFI command affects instruction-cache validity; later `0x80372854..0x80372880` reads HID0 twice and selects ICE/DCE branches. | **PROVEN** issued command word `HID0_in OR 0x800`. ICFI is self-clearing when ICE is enabled per the Gekko manual, so later readback is **not** proven equal to that word. Retail input, cache consequence and readback timing are **UNKNOWN**. |
| GQR0..7 | `li r3,0` at `0x80371734` (`38600000`), then eight distinct `mtspr` words at `0x80371738..54` write SPR 912..919 in order. The first/last words are `7C70E3A6` / `7C77E3A6`. | GQR0 determines `psq_l` format at `0x80370CFC`; later context code has candidate readers of all eight. | Incoming GQRs are **UNKNOWN**, but this connected path overwrites all eight before the first proven DOL GQR consumer. The ordered write effects are **PROVEN ISA**; direct post-write SPR readback and asynchronous effects have not been captured. |
| r13 and first FPR source | File `0x334/338`: `3DA0805E`, `61ADC500` set r13=`0x805EC500`. `0x80370CF4..F8` construct r3=`0x805F1F38`. | `psq_l` word `E0030000` at `0x80370CFC` reads `0x805F1F38..3F` on the PSE edge, using GQR0. `lfd` word `C80D5A30` at `0x80370D7C` always reads `r13+0x5A30 = 0x805F1F30..37`. | **PROVEN** effective addresses and read order on the selected edge. Actual 16 bytes at retail handoff, PS1 result and exceptional FP effects are **UNKNOWN**. |

The branch at `0x80370CF0` is conditional on a *live* HID2 read. If the
preceding SPR write has its specified architectural effect, forcing bit 29
selects `0x80370CF4`; that is an ISA conditional conclusion, not a reason to
force the branch in native code. Incoming XER.SO remains an additional input
to CR0.SO at `0x80370CEC`. The FPR routine does not alter XER; no DOL
instruction in the connected prefix produces its initial value.

## Proven source region and remaining loader dependency

The DOL header gives BSS `[0x8056FE00,0x805FC5EC)`, data6
`[0x805E4500,0x805EF020)`, and data7 `[0x805F2780,0x805FC540)`.
Both FPR reads lie in the data6/data7 gap, absent from populated DOL
sections. The apploader's state-6 BSS clear at `0x81200D94..0x81200DA0`
is an ordinary-path producer of zero there **before** DOL section loading.
The later FST request at `0x81200FEC..0x8120101C` can overlap according
to IPL-dependent low-memory state and state-4 ordering. With ordinary
nonfatal placement and low-memory input `M=[0x80000028]`, its calculated
destination is `align_down_32(0x80000000 + min(M,0x01800000) - 0x18B19)`;
`M>=0x0060AA59` is sufficient to start the FST after `0x805F1F3F` on
that arithmetic path. This condition is **STRONG conditional arithmetic**,
not a measured retail `M` or final byte value. See
[pre-entry evidence](PREENTRY_STATE.md) and the
[memory timeline](BOOT_MEMORY_TIMELINE.md) for the loader branches and
write/read ordering. CRT later zeros `[0x805EF020,0x805F277C)`, but that
write occurs **after** both hardware FPR reads and cannot prove their input.

## What the existing synthetic capture actually observes

The ignored `build/boot-paired-setup-capture.json` is a startup-only Dolphin
HLE run, pinned to synthetic disc SHA-256
`a8a526ef7006a2d2a8d41f536ef30b208ff197c185c05e9b2276e6e93cb41a8e`
and Dolphin SHA-256
`db536025ee02190c20ccb9bfa94ee36ce28bb996f2e88574e46734b353bdc526`.
The disc embeds the byte-matching `sys/` regions but does not recreate an
original retail IPL path or ordinary game payload.

| Checkpoint | Observed state | What it can establish |
|---|---|---|
| `0x80003154` | MSR=`0x2032`, HID0=`0x0011C464`, `[0x80000028]=0x01800000`, FST base=`0x817E74E0`, source `0x805F1F30..3F` = 16 zero bytes. | **OBSERVED-HLE** initial values for this launch mode only; HID2/GQR not exposed by its direct RSP snapshot. |
| `0x80371714` | r1=`0x8060C5F0`, LR=`0x80003414`, r31=`0x8000315C`, MSR=`0x2032`, 32-byte window `[0x8060C5E0,0x8060C600)` zero. | **OBSERVED-HLE** agreement with the raw direct-call and register-producing prefix. Zero stack contents are not a DOL guarantee. |
| `0x80371724` | r3=`0xE0000000` after `mfspr HID2`; r1=`0x8060C5E8`; stack at E8=`0x8060C5F0`, F4=`0x80003414`. | **OBSERVED-HLE** HID2 *incoming read* through r3 and the first ordered stack stores. It is not a retail initial HID2 value. |
| `0x8037172C` / `0x80371764` | r3=`0xE0000000` after the HID2 setter; later HID0 readback=`0x0011C464`; GQR stores lie between. | **OBSERVED-HLE** command path and persistent HID0 readback. The snapshot cannot directly prove GQR contents, ICFI timing, or cache tags. |

The existing RSP tool records PC/GPR, MSR, HID0/HID1/L2CR, FPSCR, stack,
low memory and FPR source. It does **not** expose HID2, GQRs or the PS1
lanes as directly named registers. r3 immediately after the HID2 accessor
can reveal that particular read; neither the outgoing `mtspr` instruction
nor a PS0-only FPR snapshot can stand in for a post-write HID2/GQR/PS1
measurement.

## Differential capture plan and fail-closed gate

1. Pin the full disc and actual launch mechanism. Run a cold original IPL
   boot path on hardware with a debugger or a separately identified emulator
   plus original IPL, and a separate repeat of the synthetic HLE path. Record
   disc, IPL, debugger/emulator and build hashes. Break **before**
   `0x80003154` to retain the first input state; do not merge HLE and IPL
   observations into one expected-state table. Capture complete MSR/HID0/
   HID2/GQR0..7, CR/XER/LR/CTR/FPSCR, FPR PS0+PS1, low-memory producer
   fields, stack bytes and `0x805F1F30..3F`.
2. In each run take ordered stops at `0x80003400`, `0x80371714`,
   `0x80371724`, `0x8037172C`, `0x803725F8`, `0x803725FC`,
   `0x80371730`, `0x80371738`, `0x80371758`, `0x80370CE8`,
   `0x80370CEC`, `0x80370CF0`, `0x80370CFC`, `0x80370D00`,
   `0x80370D7C`, `0x80370D80`, and `0x80370E00`. For each transition
   compare the prior raw word, input registers, effective address, branch
   decision, SPR read/write value, stack bytes, both FPR lanes and FPSCR.
   `0x803725FC` is *before* the HID0 write; capture again after it to
   distinguish command word from self-cleared readback. Resolve the
   post-write HID2/GQR and PS1 values using a debugger interface that can
   actually expose them; if unavailable, mark those comparisons unvalidated.
3. Trace apploader writes to the two eight-byte source ranges, including
   BSS fill, FST placement/order and any later producer. Compare entry bytes
   with bytes just before each FPR load. Retain raw dumps outside version
   control and publish only hashes, selected values and discrepancy notes.
4. For falsification only, use isolated *state-perturbation* runs without
   changing the original DOL: vary HID2 unforced bits, HID0[ICE], incoming
   MSR[FP/EE], GQR prestate, and FPR source bit patterns (including paired
   denormals) where the debugger can set known initial state. Label those
   runs counterfactual; they are not retail parity evidence. Check that
   incoming GQRs are overwritten, ICFI readback follows the hardware rule,
   the HID2 rotate-mask branch follows the actual read, and both FPR lanes
   and FPSCR follow the original instructions.
5. Only after the complete connected native prefix reproduces the ordered
   branch, GPR/CR/XER/LR, MSR, SPR, stack, FPR/FPSCR and downstream state
   under the same measured inputs may full paired-setup parity be claimed.
   A mismatch at any earlier checkpoint is the next root-cause target.

**Current fail-closed result:** the native diagnostic has since recorded the
two ordered stack writes and returned from the HID2 read accessor with an
explicit HLE value, stopping before `0x80371724` executes. Retail
MSR/HID0/HID2, incoming GQRs, FPR source bytes and microarchitectural
consequences remain unresolved. The HLE capture supplies a measured
scenario, not a hard-coded native initialization recipe.

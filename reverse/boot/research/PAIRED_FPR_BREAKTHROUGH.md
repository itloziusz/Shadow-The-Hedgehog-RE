# PAL boot paired-FPR wall: measured boundary and exact missing channel

**Continuation:** checkpoint 36 implemented the separate read-only export and
measured both lanes/GQRs. See `HIDDEN_BOOT_STATE_36.md` for the new evidence and
bounded C++ projection. The connected stop still precedes sync; this earlier
note retains the original observation gap and experiment design for provenance.

**Scope:** SHA-256 pinned PAL `main.dol` `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`, wrapper `0x80371730..0x80371764`, and FPR routine `0x80370CDC..0x80370E00`. This note adds a fresh same-run HLE observation and an executable PS1 capture design. It does **not** extend the connected native boot checkpoint or establish physical Gekko parity.

## What the original bytes actually require

The next native word remains `0x80371730 = 7C0004AC`, `sync`. Raw DOL `0x80371734 = 38600000` makes r3 zero and `0x80371738 = 7C70E3A6` writes it to SPR912 (GQR0); `0x8037173C..54` do likewise for GQR1..7. The writes are unconditional along this wrapper's return path. At `0x80003414 = 4836D8C9`, the caller links into `0x80370CDC`, returning at `0x80003418`.

Inside that routine, `0x80370CE8 = 7C78E2A6` reads live HID2; `0x80370CEC = 54631FFF` computes `(HID2 >> 29) & 1` into r3 and CR0; `0x80370CF0 = 4182008C` skips to `0x80370D7C` only if that bit is zero. The preceding wrapper requests HID2 OR `0xA0000000` at `0x80371728`, so the paired edge is selected if that write succeeds and reads back. This is a condition on hardware state, not a claim about retail power-on HID2.

On the paired edge, `0x80370CF4/CF8` produce r3=`0x805F1F38`; `0x80370CFC = E0030000` performs `psq_l f0,0(r3),W=0,I=0`. With the written GQR0 zero, this consumes both big-endian words at `0x805F1F38` (PS0 source) and `0x805F1F3C` (PS1 source), subject to architectural FP/paired enable and memory validity. The 31 raw `ps_mr` words `0x80370D00..78` copy both lanes of f0 into f1..f31. `0x80370D7C = C80D5A30` then loads the distinct double source `0x805F1F30..37` into f0 using proven r13=`0x805EC500`; 31 `fmr` words `0x80370D80..F8` copy the double operand to PS0 of f1..f31 in the inspected Dolphin interpreter, preserving their PS1. `0x80370DFC = FDFE058E` writes FPSCR fields from f0 with derived FEX/VX; `0x80370E00` returns. Raw catalogues and manual citations are in `BINARY_BOOT_PREFIX.md` and `FPR_LANE_PROVENANCE.md`.

The source span `0x805F1F30..3F` is absent from populated DOL sections and lies in the ordinary apploader BSS clear. A later FST transfer can in principle overlap it depending on an unmeasured retail low-memory word; it cannot be promoted to a retail zero constant. On the captured synthetic run, `[0x80000028]=0x01800000` and FST base `0x817E74E0`, well beyond this span. The same-run memory read below measured all 16 bytes zero. These statements apply only to that HLE startup fixture.

## Fresh, ordered same-run observation

I ran `tools/agent_paired_ps0_capture.py` against the pinned interpreter Dolphin executable (SHA-256 `db536025ee02190c20ccb9bfa94ee36ce28bb996f2e88574e46734b353bdc526`) and startup-only ISO (SHA-256 `a8a526ef7006a2d2a8d41f536ef30b208ff197c185c05e9b2276e6e93cb41a8e`). Both are **OBSERVED-HLE**, not original retail IPL or hardware. Output is the ignored `build/paired_ps0_capture_20260930.json`; the script validates the disc SHA before launching. In order, breakpoints were at the following *pre-instruction* PCs:

| PC | Significance | r3 | CR | PS0 f0/f1/f31 | FPSCR | source `[805F1F30..3F]` |
|---|---|---:|---:|---|---:|---|
| `80370CFC` | before `psq_l`; paired edge selected | `805F1F38` | `40000000` | all `0000000000000000` | `00000000` | 16 zero bytes |
| `80370D00` | after `psq_l`, before first `ps_mr` | `805F1F38` | `40000000` | all zero | `00000000` | 16 zero bytes |
| `80370D7C` | after 31 `ps_mr`, before `lfd` | `805F1F38` | `40000000` | all zero | `00000000` | 16 zero bytes |
| `80370D80` | after `lfd`, before first `fmr` | `805F1F38` | `40000000` | all zero | `00000000` | 16 zero bytes |
| `80370DFC` | after 31 `fmr`, before `mtfsf` | `805F1F38` | `40000000` | all zero | `00000000` | 16 zero bytes |
| `80003418` | returned from FPR routine | `805F1F38` | `40000000` | all zero | `00000000` | 16 zero bytes |

All checkpoint MSRs were `0x00002032`. CR0 nibble `4` is GT, consistent with the HID2 bit test yielding one. The measured PS0 zeros and source zeros are useful state/branch checks, but they cannot distinguish PS1 zero from an unrelated retained PS1. In every snapshot, querying unused GDB register id 143 (`p8f`) returned `E01`; it is **not** a lane value. The stock stub's `ReadRegister()` in `si-oracle/source/Source/Core/Core/PowerPC/GDBStub.cpp` serializes `ps[id-32].PS0AsU64()` for ids 32..63, and `ReadRegisters()` sends only GPRs. It has no PS1 or GQR register id. A guest memory read cannot reveal internal PS1.

## Exact instrumentation needed to break the PS1 wall

Use a **separate, repository-contained instrumented Dolphin build** as a labelled HLE oracle; do not modify the DOL, retail inputs or the native C++ boot path. A minimal read-only GDB extension in the instrumented copy of `ReadRegister()` can reserve ids 143..174 (`p8f..pae`) for the 32 raw PS1 lanes:

```cpp
else if (id >= 143 && id < 175)
  wbe64hex(reply, ppc_state.ps[id - 143].PS1AsU64());
```

`PairedSingle::PS1AsU64()` exists in the pinned source's `PowerPC.h`; this returns its internal **64-bit** lane representation, not the 32-bit memory word. Reserve id 175 (`paf`) for `ppc_state.spr[SPR_GQR0]` and id 176 (`pb0`) for `ppc_state.spr[SPR_HID2]` if direct readback is needed. These identifiers must be checked against that build's existing map before use; this source revision ends its defined register ids at 142. The script can then capture all 32 PS0/PS1 raw words plus HID2, GQR0, FPSCR, CR, XER, MSR and source bytes in **one fresh run** at `0x80371734` (before zero write), `0x80371758` (after GQR writes), `0x80370CFC` (before paired load), `0x80370D00` (after), `0x80370D7C` (after paired moves), `0x80370D80` (after `lfd`), `0x80370D84` (after first `fmr`), `0x80370DFC` (after all `fmr`), and `0x80003418` (return). Capture `f0`, `f1`, and `f31` at every point; capture all 32 on at least the last two to detect an omitted sweep element. Include executable/disc SHA-256, checkpoint order and unchanged source bytes. Retain this as **OBSERVED-HLE**; physical Gekko PS1 parity remains a separate validation question.

For adversarial controlled runs, use distinct finite 32-bit source words for PS0 and PS1 and a distinct double at `0x805F1F30`; zeros alone cannot falsify whole-register overwrite. Check signed zeros, subnormal and NaN cases separately, because this Dolphin build's `Helper_Dequantize()` calls `ConvertToDouble()` and stores two host doubles in `SetBoth()`. Any exceptional input can require additional ISA-level validation before native lowering. Validate that `ps_mr` changes PS1 in all 31 targets, `lfd` changes f0 PS0 but not its PS1, and `fmr` changes each destination PS0 without silently erasing seeded PS1 in this oracle. Finally compare FPSCR after `mtfsf`, not merely before it.

## Reconstruction gate

The current native stop remains before `sync` at `0x80371730`. The fixed HLE observation proves the branch and PS0 path for one zero-input synthetic state; it supplies **no PS1 measurement** and no retail source bytes. The next native model needs a two-lane FPR type, explicit live HID2/GQR0 and memory inputs, and a fail-closed exceptional-value path. Do not set PS1 to zero because the observed PS0 and input bytes are zero; use the instrumented same-run lane capture and independent ISA checks before claiming paired-register parity.

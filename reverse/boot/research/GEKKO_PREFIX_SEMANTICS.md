# Gekko semantics at the connected boot hardware boundary

Scope: the second entry call, `0x80003158 -> 0x80003400`, especially the next
native stop at `0x80371714` and the paired-single/cache operations it calls.
The source is the **read-only original PAL GUPP8P `main.dol`**, SHA-256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
I decoded the listed words directly as big-endian 32-bit values from DOL text
sections; the first section maps file `0x100` to VA `0x80003100`, and the main
text section maps file `0x2600` to VA `0x80008D40`. I then checked mnemonic
and ISA effects against the [IBM Gekko User's Manual, version 1.2](https://doc.kodewerx.org/documents/gekko_user_manual.pdf).
`PROVEN` below means bytes plus ISA specification. `OBSERVED-HLE` means a
separate startup-only synthetic-disc Dolphin interpreter trace; it is **not**
an original retail IPL or silicon observation. There is no connected native
execution of this callee yet; the normal native probe stops before it.

## Raw words, bit fields, and architectural effects

The `raw` column is both the four original bytes and the big-endian instruction
word. For `mfspr`/`mtspr`, primary opcode is 31 and XO is 339/467; the ten
instruction SPR bits are decoded by reversing their two five-bit halves:
`SPR = ((word >> 11 & 31) << 5) | ((word >> 16) & 31)`.
Thus `0x31C -> 920` (HID2), `0x21F -> 1008` (HID0), `0x23C -> 913`
(GQR1), and `0x33F -> 1017` (L2CR). This is independent of mnemonic names.

| VA | DOL offset | Raw bytes / word | Field decode and exact effect |
|---|---:|---:|---|
| `80003400` | `000400` | `7C 00 00 A6` | `mfmsr r0`; r0 gets full incoming MSR. |
| `80003404` | `000404` | `60 00 20 00` | `ori r0,r0,0x2000`; sets MSR[FP] candidate, does not set CR. |
| `80003408` | `000408` | `7C 00 01 24` | `mtmsr r0`; writes full MSR, not only FP. Supervisor and execution synchronizing. |
| `80370BA8` | `36A468` | `7C 78 E2 A6` | `mfspr r3,920` (HID2). |
| `80371724` | `36AFE4` | `64 63 A0 00` | `oris r3,r3,0xA000`; r3 = old HID2 OR `0xA0000000`. |
| `80370BB0` | `36A470` | `7C 78 E3 A6` | `mtspr 920,r3`; requests HID2 write. |
| `803725F4` | `36BEB4` | `7C 70 FA A6` | `mfspr r3,1008` (HID0). |
| `803725F8` | `36BEB8` | `60 63 08 00` | `ori r3,r3,0x0800`; sets the HID0[ICFI] command bit. |
| `803725FC` | `36BEBC` | `7C 70 FB A6` | `mtspr 1008,r3`; **issues L1 instruction-cache flash invalidate** when ICE is enabled. HID0[ICFI] is self-clearing, so its write value is not necessarily the next read value. |
| `80371730` | `36AFF0` | `7C 00 04 AC` | `sync`; orders all preceding effects including the HID writes before following instructions. No GPR/CR/FPSCR result. |
| `80371734` | `36AFF4` | `38 60 00 00` | `li r3,0`; zero source for eight GQR writes. |
| `80371738..54` | `36AFF8..36B014` | `7C70E3A6`, `7C71E3A6`, `7C72E3A6`, `7C73E3A6`, `7C74E3A6`, `7C75E3A6`, `7C76E3A6`, `7C77E3A6` | Eight `mtspr` writes, respectively GQR0..GQR7 = 0 (SPR 912..919), ordered by program order. |
| `80370CE8` | `36A5A8` | `7C 78 E2 A6` | HID2 read again, this time by FPR seed. |
| `80370CEC` | `36A5AC` | `54 63 1F FF` | `rlwinm. r3,r3,3,31,31`: rotate-left-3, AND `0x00000001`, update CR0. The tested bit is HID2 numeric bit 29 / IBM bit 2, PSE. |
| `80370CF0` | `36A5B0` | `41 82 00 8C` | `beq 0x80370D7C` iff CR0.EQ. The preceding forced PSE bit makes this false on an architecturally successful HID2 write/read. |
| `80370CFC` | `36A5BC` | `E0 03 00 00` | `psq_l f0,0(r3),W=0,I=0`: primary opcode 56, rD=0, rA=3, W=0, GQR0. With r3=`0x805F1F38` and GQR0=0, read two 32-bit floating operands from `0x805F1F38..3F` into PS0/PS1. Requires MSR[FP], HID2[PSE], HID2[LSQE]. |
| `80370D7C` | `36A63C` | `C8 0D 5A 30` | `lfd f0,0x5A30(r13)`; r13=`0x805EC500` makes effective address `0x805F1F30`, reads eight bytes. |
| `80370DFC` | `36A6BC` | `FD FE 05 8E` | `mtfsf 0xFF,f0`; all selected FPSCR fields are written, with derived FEX/VX summary semantics; no ordinary RAM write. |
| `80372858` | `36C118` | `54 60 04 20` | `rlwinm r0,r3,0,16,16`: isolate HID0[ICE] (`0x8000`). **Rc=0**; the following `cmplwi` at `0x8037285C` sets CR0 for the branch. |
| `80372878` | `36C138` | `54 60 04 62` | `rlwinm r0,r3,0,17,17`: isolate HID0[DCE] (`0x4000`). **Rc=0**; `0x8037287C` sets CR0. |
| `80372898` | `36C158` | `54 60 00 00` | `rlwinm r0,r3,0,0,0`: isolate L2CR[L2E] (`0x80000000`). **Rc=0**; `0x8037289C` sets CR0. |
| `80372668` | `36BF28` | `64 63 00 20` | `oris r3,r3,0x20`; set L2CR[L2I] request (`0x00200000`). |
| `80372684` | `36BF44` | `40 82 FF F4` | `bne 0x80372678`: poll while CR0 says L2CR[L2IP] (`0x1`) is nonzero. |
| `8037268C` | `36BF4C` | `54 63 02 D2` | `rlwinm r3,r3,0,11,9`; wrapping mask clears numeric bit 21 / IBM bit 10 (L2I). |
| `803726C0` | `36BF80` | `40 82 FF E8` | `bne 0x803726A8`: second poll of live L2CR[L2IP]. |

`0x80370D00..0x80370D78` are 31 sequential `ps_mr f1..f31,f0` writes;
`0x80370D80..0x80370DF8` are 31 sequential `fmr f1..f31,f0` writes. This
note checks their ISA consequence, while the contiguous word catalogue and
register operands belong in the per-region binary database. These ranges
contain no branch or ordinary memory write. The wrapper's calls remain in
fixed order: HID/GQR setup, FPR/FPSCR seed, cache/handler setup.

## Hardware meaning and the correction to the earlier model

**PROVEN ISA:** The three forced HID2 bits in `0xA0000000` are IBM bit 0
(`LSQE`, numeric `0x80000000`) and bit 2 (`PSE`, numeric `0x20000000`);
bit 1 (`WPE`) is **not** forced. The [manual's HID2 table, pp. 2-13–14](https://doc.kodewerx.org/documents/gekko_user_manual.pdf)
states that the nonindexed `psq_l` needs both PSE and LSQE, while `ps_mr`
needs PSE. This prefix enables both before either instruction. `rlwinm.` at
`0x80370CEC` tests PSE, not LSQE. For the observed HID2 `0xE0000000`,
`rotl32(HID2,3) & 1 == 1`; a plain left shift gives zero and is incorrect.

**PROVEN ISA, material correction:** HID0 `0x00000800` is IBM bit 20,
**ICFI**, an *action* bit for whole L1 instruction-cache invalidation. The
[manual's HID0 table, pp. 2-11–12](https://doc.kodewerx.org/documents/gekko_user_manual.pdf)
says hardware clears ICFI in the next cycle when HID0[ICE] is enabled.
Therefore `HID0_in | 0x800` is the **written command word**, not generally
the persistent HID0 value. The same manual, p. 2-14, explicitly requires an
I-cache invalidate after `mtspr` newly enables HID2[LSQE/PSE/LCE] and before
using graphics extension instructions. The source sequence does precisely
`mtspr HID2; mfspr/ori/mtspr HID0[ICFI]; sync; GQR writes; psq_l`.
This explains a discrepancy in the earlier shorthand “HID0 becomes
`HID0_in | 0x800`”: that formula omits a hardware-generated state transition.
For the **synthetic HLE** input, HID0 before = `0x0011C464` (ICE=1, DCE=1,
ICFI=0), written command = `0x0011CC64`, later read = `0x0011C464`.
The observed return value is therefore consistent with ICFI self-clear;
the hidden cache tag effects themselves were not measured by GDB. If ICE
were 0, the manual conditions the self-clear/invalidation on cache enable;
the precise readback and subsequent path require an input-specific trace.

**PROVEN ISA:** `sync` is not a numerical no-op on Gekko. It waits for prior
memory/external effects to complete before later instructions begin; the
[manual, p. 12-239](https://doc.kodewerx.org/documents/gekko_user_manual.pdf)
states the ordering rule. `mtmsr` writes a whole MSR and is execution
synchronizing; newly enabling EE can admit a pending external/decrementer
exception before the next instruction (manual p. 12-139). The entry wrapper
ORs only FP, but later L2 code temporarily writes `MSR=0x30` and restores
the saved word. No ordinary C++ assignment can by itself claim equivalent
interrupt, cache, or ordering behavior.

**PROVEN ISA:** GQR0=0 selects 32-bit single-precision floating operands with
no quantization conversion ([GQR tables, pp. 2-24–25](https://doc.kodewerx.org/documents/gekko_user_manual.pdf)).
With `W=0`, `psq_l` reads both words. `ps_mr` copies both lanes. The manual
explicitly gives a `psq_l` followed by `lfd` as the restore recipe for a
paired FPR and cautions that denormal PS1 values may be lost on a preceding
`psq_st` ([pp. 2-29–30](https://doc.kodewerx.org/documents/gekko_user_manual.pdf)).
`fmr` following this `lfd` cannot safely be reduced to a host `double`
assignment for the complete paired state. The synthetic trace exposed PS0
only; PS1 remains unmeasured. The `mtfsf` source is f0 from the `lfd` bytes,
not the later `fmr` destination contents.

**PROVEN ISA:** The L2 sequence is a real state-machine interaction, not a
software wait on its own written bit. `0x00200000` requests global invalidate
(L2I); numeric bit 0 (`0x1`) is read-only L2IP, *invalidation in progress*.
The [manual, pp. 9-3–4](https://doc.kodewerx.org/documents/gekko_user_manual.pdf)
specifies disable L2E, `sync`, request L2I, poll L2IP, clear L2I and enable
L2E after completion. The synthetic interpreter trace observed L2IP=0 on
both first visits; that run did not measure retail invalidation latency or
cache tag contents. An unbounded native busy loop reading a static Boolean
would not reproduce the hardware protocol.

## Downstream readers and semantic-collapse boundary

A raw opcode scan of **populated DOL text sections**, independently decoding
the split SPR field, found later direct readers/writers of these registers:

| State from this prefix | Later original instructions that can observe/replace it | Status |
|---|---|---|
| HID2 | `0x80370CE8` immediately tests PSE; `0x8037102C` reads and `0x80371034` writes HID2 in later OS startup; context routine `0x8039F664` reads it; `0x803DB648` reads it. | First read is on the connected path; reachability of every later candidate still requires CFG proof. Cannot discard HID2 state yet. |
| HID0[ICE/DCE/ICFI] | `0x80372854..0x80372880` takes cache branches from live HID0 reads; `0x8039F5C4` context routine reads HID0. | Branch inputs are direct consumers; ICFI readback may differ from the command word. |
| GQR0..7 | GQR0 controls `0x80370CFC`; later `0x8039F644..0x8039F660` read all eight; many later `mtspr` writes occur in text. | Cannot replace all eight with an implicit constant across boot/runtime without reachability and consumer proof. |
| L2CR | `0x80372894` selects L2 path, `0x80372678..0x803726C0` polls live L2IP, and `0x8039F6F0` reads it in context code. | Do not infer poll termination from initial L2CR or Dolphin alone. |
| MSR | `0x80370CDC` reads after the wrapper write; cache setup and handler registration later read/restore it. | Full word and transient states matter until consumers and exception timing are settled. |

This text scan is a **candidate consumer set**, not proof that every listed
later instruction executes during a normal game boot. `mfspr` opcodes could
also occur in data embedded in text, and conditional paths remain to be
resolved. The immediate branch and load consumers are verified by the CFG
and synthetic run in `ASM_HARDWARE_VALIDATION.md`.

For a portable native x64 implementation, the proven *program semantics*
can eventually be expressed as `EnableRequiredFloatingPointState`,
`ConfigurePairedLoads`, `InitializePlatformCaches`, and
`RegisterExceptionHandler`, each taking explicit incoming platform/loader
state and producing documented state used by later game code. Their names
are only proposed semantic boundaries. No part of the ICFI command, L2
polling, paired FPR state, or interrupt timing may be erased merely because
the host machine has its own caches. Until the readers above and exception
paths are resolved, this region remains a fail-closed hardware dependency.

## Independent falsification and validation gates

1. **Byte decode:** Re-read each listed DOL offset, assert the four bytes and
   opcode/XO/SPR fields. Flip one source word at a time; a binary pin must
   decline the altered DOL. The bit transforms `rotl32(0xE0000000,3)&1=1`,
   mask(16,16)=`0x8000`, mask(17,17)=`0x4000`, and HID0 command
   `0x0011C464|0x800=0x0011CC64` were checked with executable assertions.
2. **ICFI counterexample:** Use a reference trace with ICE=1 and ICFI=0
   before `0x803725FC`; record HID0 immediately around the write and on the
   later read at `0x80372854`. A model that always persists ICFI should fail
   the synthetic observed `0x0011C464` later read. Repeat with ICE=0 if an
   authentic source can provide that state; do not infer the outcome.
3. **Paired state:** Record both PS0 and PS1 at `0x80370D00`, after `lfd`,
   and at `0x8000315C`, including nonzero and denormal source cases where
   possible. Check FPSCR before/after `mtfsf` and exception bits; a PS0-only
   GDB snapshot is insufficient.
4. **L2 state machine:** Record every L2CR read and write at both polls,
   including status-bit transitions and timing, on retail hardware or a
   clearly labeled independent hardware oracle. A zero from a synthetic
   interpreter is a valid conditional observation only.
5. **Native regression:** Re-run from `0x80003154`, compare each reachable
   GPR, CR/XER/LR/CTR, MSR, FPSCR, GQR/HID state, stack/handler writes and
   selected branch in order. Keep the connection stopped at `0x80371714`
   until these effects are represented by evidence-backed platform state.

Remaining **UNKNOWN**: retail IPL HID0/HID2/L2CR and cache contents;
completion timing of ICFI/L2 invalidation; exact PS1 at the reference
checkpoint; reachability/necessity of later context-save readers; and
which cache/interrupt consequences remain observable after translation to
native x64.

# FPR lane provenance at the PAL boot seed

**Continuation:** `HIDDEN_BOOT_STATE_36.md` now measures both HLE lanes with
distinct controlled inputs and validates a bounded unconnected C++ projection.
The physical-format/exceptional/retail questions below remain unresolved.

Scope: `0x80370CDC..0x80370E00` in the read-only PAL GUPP8P `main.dol` (SHA-256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`). **PROVEN** below means original bytes plus the [IBM Gekko User's Manual v1.2](https://doc.kodewerx.org/documents/gekko_user_manual.pdf). **OBSERVED-HLE** means the pinned synthetic-disc Dolphin interpreter run; it is neither retail IPL state nor physical Gekko measurement. **UNKNOWN** is a gate, not a default value.

## Independent binary decode

I read the DOL bytes directly, before using [BINARY_FPR_PREFIX.md](BINARY_FPR_PREFIX.md) as a comparison. Text section 1 maps `VA 0x80008D40` to file `0x002600`; `file(VA) = 0x2600 + VA - 0x80008D40`. This interval is 74 contiguous big-endian words at file `0x36A59C..0x36A6C3`. The complete catalogue in the comparison note agrees with the direct extraction. Critical independently decoded words:

| VA / file | Raw bytes / word | Fields and consequence |
|---|---|---|
| `80370CDC / 36A59C` | `7C 60 00 A6 / 7C6000A6` | `mfmsr r3`; reads full MSR. |
| `80370CE8 / 36A5A8` | `7C 78 E2 A6 / 7C78E2A6` | `mfspr r3,920` (HID2); split SPR field recombines to 920. |
| `80370CEC / 36A5AC` | `54 63 1F FF / 54631FFF` | primary 21, SH=3, MB=ME=31, Rc=1: `rlwinm. r3,r3,3,31,31`, result `(HID2 >> 29) & 1`; updates CR0 with XER.SO. |
| `80370CF0 / 36A5B0` | `41 82 00 8C / 4182008C` | BO=12, BI=2, displacement `+0x8C`, AA=LK=0: branch to `0x80370D7C` on CR0.EQ. |
| `80370CFC / 36A5BC` | `E0 03 00 00 / E0030000` | primary 56, fD=0, rA=3, W=0, I=0, signed 12-bit displacement 0: `psq_l f0,0(r3),0,0`. |
| `80370D00 / 36A5C0` | `10 20 00 90 / 10200090` | primary 4, XO=72, fD=1, fB=0, Rc=0: first `ps_mr`. |
| `80370D78 / 36A638` | `13 E0 00 90 / 13E00090` | same form with fD=31: final `ps_mr`. |
| `80370D7C / 36A63C` | `C8 0D 5A 30 / C80D5A30` | primary 50, fD=0, rA=13, displacement `0x5A30`: non-update `lfd`. |
| `80370D80 / 36A640` | `FC 20 00 90 / FC200090` | primary 63, XO=72, fD=1, fB=0, Rc=0: first `fmr`. |
| `80370DF8 / 36A6B8` | `FF E0 00 90 / FFE00090` | same form with fD=31: final `fmr`. |
| `80370DFC / 36A6BC` | `FD FE 05 8E / FDFE058E` | primary 63, FM=`0xFF`, fB=0, XO=711, Rc=0: `mtfsf 0xFF,f0`. |
| `80370E00 / 36A6C0` | `4E 80 00 20 / 4E800020` | `blr`; return through incoming LR. |

The two complete sweeps were independently checked against the file: for each `n=1..31`, `word(0x80370D00+4(n-1)) = 0x10000090 | (n << 21)` and `word(0x80370D80+4(n-1)) = 0xFC000090 | (n << 21)`. Every member therefore has source f0, destination fn, XO=72 and Rc=0. There is no hidden lane selection in these 62 words.

## Machine-state and lane transitions

The entry `mfmsr; ori 0x2000; mtmsr` requests MSR[FP] by writing the full MSR. Its live input and exception timing are external. `mfspr HID2; rlwinm.; beq` tests HID2 numeric bit 29, not a left shift of HID2; CR0 is updated from the 0/1 result and carries incoming XER.SO, while XER and other CR fields are not written. A zero readback skips the paired block. The earlier HID2 writer requests `HID2_in | 0xA0000000`; a successful live readback makes the paired branch execute, but retail readback remains unmeasured.

On the paired edge, `lis/addi` sets r3=`0x805F1F38`. GQR0 was explicitly written zero earlier in the connected assembly. With MSR[FP], HID2[PSE/LSQE] and a valid mapped source, `psq_l` reads two big-endian 32-bit words at `0x805F1F38` and `0x805F1F3C`. GQR0 load type 0 passes the single-precision bit patterns without integer dequantization; W=0 writes both PS0 and PS1 of f0. Each `ps_mr` copies **both** lanes of f0 into f1..f31. The manual gives these effects in `psq_l` p. 12-157 and `ps_mr` p. 12-179. None of these instructions has Rc=1, and the manual lists no FPSCR side effect for these moves/loads.

Both CFG edges then execute `lfd f0,0x5A30(r13)`. The entry helper's r13=`0x805EC500` makes the effective address `0x805F1F30`; the eight bytes there become a double-precision f0 operand. This is **not** the pair at `0x805F1F38`. The manual's paired-register save/restore recipe (`psq_l` followed by `lfd`, pp. 2-29–30) establishes that a prior paired PS1 may survive an `lfd` of the same register; it also says the internal relation between double and paired formats is unspecified. Do not infer all lanes from the eight loaded bytes.

The next 31 `fmr` instructions have Rc=0 and copy the f0 double operand to f1..f31. The manual p. 12-69 explicitly distinguishes a double source from a paired source; for a paired source it says destination PS1 is unchanged, whereas for a double source it says the operand is copied. The pinned local Dolphin interpreter source (`Interpreter_FloatingPoint.cpp`) implements `fmr` as a PS0 64-bit copy that leaves destination PS1 untouched; its `lfd` implementation (`Interpreter_LoadStore.cpp`) likewise sets PS0 only. That **Dolphin model** predicts that, on the paired edge, f1..f31 retain the earlier paired PS1 seed after their final `fmr` while receiving f0's new double in PS0. The same model predicts that, on the skipped edge, their PS1 derives from unknown incoming FPR state. These are testable hypotheses, **not measured PS1 parity**. The binary and manual do not license replacing the whole FPR with a host `double` assignment.

Finally `mtfsf 0xFF,f0` takes the low-order 32 bits of f0's freshly loaded double representation (`0x805F1F34..37`) for all eight FPSCR fields. Per manual p. 12-137, FPSCR FEX and VX are derived summaries rather than copied source bits; FX and OX are taken from source bits when field 0 is selected. Rc=0 leaves CR1 unchanged. It does not consume f1..f31. If the paired path ran, final r3 remains `0x805F1F38`; on the skipped edge, it remains zero from `rlwinm.`. `blr` returns to `0x80003418` if called by the documented wrapper. Exceptional MSR/FP/load conditions could divert control earlier and must not be silently treated as ordinary return.

## The exact observation gap

`capture_dolphin_rsp.py` snapshots PC, GPRs, MSR, CR, LR, CTR, XER, FPSCR and 16 source bytes at `0x805F1F30..3F`; it does **not** currently query FPRs. Even adding standard GDB `p20..p3f` reads would expose only PS0. The local Dolphin GDB stub's register handler for ids 32..63 explicitly serializes `ps[id-32].PS0AsU64()` and has no PS1 register id; its `g` packet sends GPRs only. A PS0 value of zero at return therefore cannot prove the other lane zero, prove an intermediate pair was initialized, or distinguish the paired and skipped paths. The synthetic HLE capture's source bytes are zero in that one run, while retail bytes and any FPR source writes before this call remain **UNKNOWN**. Neither the DOL's static section table nor a later CRT zero is a substitute for a same-time memory read.

A non-speculative measurement is to add a **read-only debugger export** in a repository-contained instrumented Dolphin build that serializes `PPCState::ps[0..31]` PS0 and PS1 **raw 64-bit representations**, alongside HID2, GQR0, FPSCR, CR/XER and the 16 source bytes. It must stop in one fresh same-run execution immediately before `0x80370CFC`, after that load at `0x80370D00`, before `lfd` at `0x80370D7C`, after `lfd` at `0x80370D80`, and at return `0x80003418`. Do not infer the second lane from standard GDB FPR packets or modify the original DOL. Pin the executable and synthetic input hashes, retain ordered checkpoint IDs, and label the result **OBSERVED-HLE**. For actual Gekko parity, the same raw-lane checkpoints need a hardware oracle or a second independent architecture-backed validation; Dolphin's implementation is not silicon proof.

Adversarial isolated cases should use separately labelled controlled memory inputs, never claim they are retail boot data: distinct finite PS0/PS1 values; +0 versus -0; smallest subnormal and signed subnormal; qNaN and sNaN payloads; and a double low word whose FPSCR FEX/VX source bits disagree with the summaries derived from exception/enables. Test both HID2 branch outcomes, XER.SO=1 at `rlwinm.`, and GQR0 nonzero as a negative control. Compare before and after each instruction, not merely final numeric doubles. Any exception or lane discrepancy blocks native collapse at that instruction.

## Reconstruction gate

The present native probe stops before this routine. The first lawful C++ model must carry two FPR lanes (including raw representation), FPSCR, CR0/XER.SO, live MSR/HID2/GQR0, and source-memory provenance. Validate the 74 words' lane and status effects at the checkpoints above, rerun the full connected prefix from `0x80003154`, and compare downstream consumers. Until then, this note is binary/ISA archaeology and a measurement plan, **not** a claim that the FPR routine has native parity.

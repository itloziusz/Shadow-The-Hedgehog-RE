# PAL GUPP8P FPR/FPSCR boot seed: binary-first record

**Fixture:** original read-only `sys/main.dol`, SHA-256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`. This file records all 74 four-byte words in `0x80370CDC..0x80370E00` (inclusive). The word catalogue was extracted and decoded before comparison with `q.py`. **PROVEN** here means original bytes plus instruction encoding/ISA documentation. **OBSERVED-HLE** below means a synthetic startup-only Dolphin trace, never retail IPL or physical GameCube proof.

## Mapping and bitfield rules

DOL text section 1 has file offset `0x002600`, VA `0x80008D40` and length `0x4A1F20`. Every word below maps by `file = 0x2600 + (VA - 0x80008D40)`. The interval begins at file `0x36A59C` and ends at `0x36A6C3`. Words are big-endian. Primary opcode is `word >> 26`; destination/source field is `(word >> 21) & 31`; rA is `(word >> 16) & 31`; rB is `(word >> 11) & 31`. The X-form XO is `(word >> 1) & 1023` and Rc is bit 0. For `mfspr`, the two five-bit SPR halves are exchanged: `SPR = ((word >> 11 & 31) << 5) | ((word >> 16) & 31)`. For `rlwinm.`, SH/MB/ME are word bits 16–20, 21–25, 26–30 in PPC numbering, i.e. shifts 11/6/1. The B-form displacement is the signed 16-bit field with its low two bits zero; target is PC+displacement when AA=0.

## Complete original word catalogue

| VA | File offset | Raw bytes / BE word | Independently decoded fields and instruction |
|---|---:|---|---|
| `80370CDC` | `36A59C` | `7C 60 00 A6` / `7C6000A6` | X op31 d3 XO83 → `mfmsr r3`; r3←MSR |
| `80370CE0` | `36A5A0` | `60 63 20 00` / `60632000` | D op24 s3 a3 imm2000 → `ori r3,r3,0x2000` |
| `80370CE4` | `36A5A4` | `7C 60 01 24` / `7C600124` | X op31 s3 XO146 → `mtmsr r3` |
| `80370CE8` | `36A5A8` | `7C 78 E2 A6` / `7C78E2A6` | X op31 d3 XO339 SPR920 → `mfspr r3,HID2` |
| `80370CEC` | `36A5AC` | `54 63 1F FF` / `54631FFF` | M op21 s3 a3 SH3 MB31 ME31 Rc1 → `rlwinm. r3,r3,3,31,31` |
| `80370CF0` | `36A5B0` | `41 82 00 8C` / `4182008C` | B op16 BO12 BI2 BD+0x8C AA0 LK0 → `beq 80370D7C` |
| `80370CF4` | `36A5B4` | `3C 60 80 5F` / `3C60805F` | D op15 d3 a0 imm805F → `lis r3,0x805F` |
| `80370CF8` | `36A5B8` | `38 63 1F 38` / `38631F38` | D op14 d3 a3 imm1F38 → `addi r3,r3,0x1F38` |
| `80370CFC` | `36A5BC` | `E0 03 00 00` / `E0030000` | DW op56 fD0 a3 W0 I0 d0 → `psq_l f0,0(r3),0,0` |
| `80370D00` | `36A5C0` | `10 20 00 90` / `10200090` | X op4 d1 b0 XO72 Rc0 → `ps_mr f1,f0` |
| `80370D04` | `36A5C4` | `10 40 00 90` / `10400090` | X op4 d2 b0 XO72 Rc0 → `ps_mr f2,f0` |
| `80370D08` | `36A5C8` | `10 60 00 90` / `10600090` | X op4 d3 b0 XO72 Rc0 → `ps_mr f3,f0` |
| `80370D0C` | `36A5CC` | `10 80 00 90` / `10800090` | X op4 d4 b0 XO72 Rc0 → `ps_mr f4,f0` |
| `80370D10` | `36A5D0` | `10 A0 00 90` / `10A00090` | X op4 d5 b0 XO72 Rc0 → `ps_mr f5,f0` |
| `80370D14` | `36A5D4` | `10 C0 00 90` / `10C00090` | X op4 d6 b0 XO72 Rc0 → `ps_mr f6,f0` |
| `80370D18` | `36A5D8` | `10 E0 00 90` / `10E00090` | X op4 d7 b0 XO72 Rc0 → `ps_mr f7,f0` |
| `80370D1C` | `36A5DC` | `11 00 00 90` / `11000090` | X op4 d8 b0 XO72 Rc0 → `ps_mr f8,f0` |
| `80370D20` | `36A5E0` | `11 20 00 90` / `11200090` | X op4 d9 b0 XO72 Rc0 → `ps_mr f9,f0` |
| `80370D24` | `36A5E4` | `11 40 00 90` / `11400090` | X op4 d10 b0 XO72 Rc0 → `ps_mr f10,f0` |
| `80370D28` | `36A5E8` | `11 60 00 90` / `11600090` | X op4 d11 b0 XO72 Rc0 → `ps_mr f11,f0` |
| `80370D2C` | `36A5EC` | `11 80 00 90` / `11800090` | X op4 d12 b0 XO72 Rc0 → `ps_mr f12,f0` |
| `80370D30` | `36A5F0` | `11 A0 00 90` / `11A00090` | X op4 d13 b0 XO72 Rc0 → `ps_mr f13,f0` |
| `80370D34` | `36A5F4` | `11 C0 00 90` / `11C00090` | X op4 d14 b0 XO72 Rc0 → `ps_mr f14,f0` |
| `80370D38` | `36A5F8` | `11 E0 00 90` / `11E00090` | X op4 d15 b0 XO72 Rc0 → `ps_mr f15,f0` |
| `80370D3C` | `36A5FC` | `12 00 00 90` / `12000090` | X op4 d16 b0 XO72 Rc0 → `ps_mr f16,f0` |
| `80370D40` | `36A600` | `12 20 00 90` / `12200090` | X op4 d17 b0 XO72 Rc0 → `ps_mr f17,f0` |
| `80370D44` | `36A604` | `12 40 00 90` / `12400090` | X op4 d18 b0 XO72 Rc0 → `ps_mr f18,f0` |
| `80370D48` | `36A608` | `12 60 00 90` / `12600090` | X op4 d19 b0 XO72 Rc0 → `ps_mr f19,f0` |
| `80370D4C` | `36A60C` | `12 80 00 90` / `12800090` | X op4 d20 b0 XO72 Rc0 → `ps_mr f20,f0` |
| `80370D50` | `36A610` | `12 A0 00 90` / `12A00090` | X op4 d21 b0 XO72 Rc0 → `ps_mr f21,f0` |
| `80370D54` | `36A614` | `12 C0 00 90` / `12C00090` | X op4 d22 b0 XO72 Rc0 → `ps_mr f22,f0` |
| `80370D58` | `36A618` | `12 E0 00 90` / `12E00090` | X op4 d23 b0 XO72 Rc0 → `ps_mr f23,f0` |
| `80370D5C` | `36A61C` | `13 00 00 90` / `13000090` | X op4 d24 b0 XO72 Rc0 → `ps_mr f24,f0` |
| `80370D60` | `36A620` | `13 20 00 90` / `13200090` | X op4 d25 b0 XO72 Rc0 → `ps_mr f25,f0` |
| `80370D64` | `36A624` | `13 40 00 90` / `13400090` | X op4 d26 b0 XO72 Rc0 → `ps_mr f26,f0` |
| `80370D68` | `36A628` | `13 60 00 90` / `13600090` | X op4 d27 b0 XO72 Rc0 → `ps_mr f27,f0` |
| `80370D6C` | `36A62C` | `13 80 00 90` / `13800090` | X op4 d28 b0 XO72 Rc0 → `ps_mr f28,f0` |
| `80370D70` | `36A630` | `13 A0 00 90` / `13A00090` | X op4 d29 b0 XO72 Rc0 → `ps_mr f29,f0` |
| `80370D74` | `36A634` | `13 C0 00 90` / `13C00090` | X op4 d30 b0 XO72 Rc0 → `ps_mr f30,f0` |
| `80370D78` | `36A638` | `13 E0 00 90` / `13E00090` | X op4 d31 b0 XO72 Rc0 → `ps_mr f31,f0` |
| `80370D7C` | `36A63C` | `C8 0D 5A 30` / `C80D5A30` | D op50 fD0 a13 imm5A30 → `lfd f0,0x5A30(r13)` |
| `80370D80` | `36A640` | `FC 20 00 90` / `FC200090` | X op63 d1 b0 XO72 Rc0 → `fmr f1,f0` |
| `80370D84` | `36A644` | `FC 40 00 90` / `FC400090` | X op63 d2 b0 XO72 Rc0 → `fmr f2,f0` |
| `80370D88` | `36A648` | `FC 60 00 90` / `FC600090` | X op63 d3 b0 XO72 Rc0 → `fmr f3,f0` |
| `80370D8C` | `36A64C` | `FC 80 00 90` / `FC800090` | X op63 d4 b0 XO72 Rc0 → `fmr f4,f0` |
| `80370D90` | `36A650` | `FC A0 00 90` / `FCA00090` | X op63 d5 b0 XO72 Rc0 → `fmr f5,f0` |
| `80370D94` | `36A654` | `FC C0 00 90` / `FCC00090` | X op63 d6 b0 XO72 Rc0 → `fmr f6,f0` |
| `80370D98` | `36A658` | `FC E0 00 90` / `FCE00090` | X op63 d7 b0 XO72 Rc0 → `fmr f7,f0` |
| `80370D9C` | `36A65C` | `FD 00 00 90` / `FD000090` | X op63 d8 b0 XO72 Rc0 → `fmr f8,f0` |
| `80370DA0` | `36A660` | `FD 20 00 90` / `FD200090` | X op63 d9 b0 XO72 Rc0 → `fmr f9,f0` |
| `80370DA4` | `36A664` | `FD 40 00 90` / `FD400090` | X op63 d10 b0 XO72 Rc0 → `fmr f10,f0` |
| `80370DA8` | `36A668` | `FD 60 00 90` / `FD600090` | X op63 d11 b0 XO72 Rc0 → `fmr f11,f0` |
| `80370DAC` | `36A66C` | `FD 80 00 90` / `FD800090` | X op63 d12 b0 XO72 Rc0 → `fmr f12,f0` |
| `80370DB0` | `36A670` | `FD A0 00 90` / `FDA00090` | X op63 d13 b0 XO72 Rc0 → `fmr f13,f0` |
| `80370DB4` | `36A674` | `FD C0 00 90` / `FDC00090` | X op63 d14 b0 XO72 Rc0 → `fmr f14,f0` |
| `80370DB8` | `36A678` | `FD E0 00 90` / `FDE00090` | X op63 d15 b0 XO72 Rc0 → `fmr f15,f0` |
| `80370DBC` | `36A67C` | `FE 00 00 90` / `FE000090` | X op63 d16 b0 XO72 Rc0 → `fmr f16,f0` |
| `80370DC0` | `36A680` | `FE 20 00 90` / `FE200090` | X op63 d17 b0 XO72 Rc0 → `fmr f17,f0` |
| `80370DC4` | `36A684` | `FE 40 00 90` / `FE400090` | X op63 d18 b0 XO72 Rc0 → `fmr f18,f0` |
| `80370DC8` | `36A688` | `FE 60 00 90` / `FE600090` | X op63 d19 b0 XO72 Rc0 → `fmr f19,f0` |
| `80370DCC` | `36A68C` | `FE 80 00 90` / `FE800090` | X op63 d20 b0 XO72 Rc0 → `fmr f20,f0` |
| `80370DD0` | `36A690` | `FE A0 00 90` / `FEA00090` | X op63 d21 b0 XO72 Rc0 → `fmr f21,f0` |
| `80370DD4` | `36A694` | `FE C0 00 90` / `FEC00090` | X op63 d22 b0 XO72 Rc0 → `fmr f22,f0` |
| `80370DD8` | `36A698` | `FE E0 00 90` / `FEE00090` | X op63 d23 b0 XO72 Rc0 → `fmr f23,f0` |
| `80370DDC` | `36A69C` | `FF 00 00 90` / `FF000090` | X op63 d24 b0 XO72 Rc0 → `fmr f24,f0` |
| `80370DE0` | `36A6A0` | `FF 20 00 90` / `FF200090` | X op63 d25 b0 XO72 Rc0 → `fmr f25,f0` |
| `80370DE4` | `36A6A4` | `FF 40 00 90` / `FF400090` | X op63 d26 b0 XO72 Rc0 → `fmr f26,f0` |
| `80370DE8` | `36A6A8` | `FF 60 00 90` / `FF600090` | X op63 d27 b0 XO72 Rc0 → `fmr f27,f0` |
| `80370DEC` | `36A6AC` | `FF 80 00 90` / `FF800090` | X op63 d28 b0 XO72 Rc0 → `fmr f28,f0` |
| `80370DF0` | `36A6B0` | `FF A0 00 90` / `FFA00090` | X op63 d29 b0 XO72 Rc0 → `fmr f29,f0` |
| `80370DF4` | `36A6B4` | `FF C0 00 90` / `FFC00090` | X op63 d30 b0 XO72 Rc0 → `fmr f30,f0` |
| `80370DF8` | `36A6B8` | `FF E0 00 90` / `FFE00090` | X op63 d31 b0 XO72 Rc0 → `fmr f31,f0` |
| `80370DFC` | `36A6BC` | `FD FE 05 8E` / `FDFE058E` | XFL op63 FMFF b0 XO711 Rc0 → `mtfsf 0xFF,f0` |
| `80370E00` | `36A6C0` | `4E 80 00 20` / `4E800020` | XL op19 BO20 BI0 XO16 LK0 → `blr` |

## Exact local control and data flow

Entry is a direct `bl` from `0x80003414`, so LR initially holds `0x80003418`. This leaf has no stack frame, indirect call, store to ordinary RAM, or constructor. Its only local conditional edge is `0x80370CF0`: if CR0.EQ, branch to `0x80370D7C`; otherwise execute the paired block `0x80370CF4..0x80370D78` then fall into `0x80370D7C`. The `blr` at `0x80370E00` returns through the incoming LR; no instruction here writes LR or CTR.

At `0x80370CDC..0x80370CE4`, r3 receives the full MSR, ORs `0x00002000`, and writes the **whole** MSR. This is a second FP-enable request after `0x80003400..08`; it does not prove the first request redundant because exception timing and later readers need separate evidence. `mtmsr` is execution-synchronizing and can change exception admission. No ordinary GPR or memory store represents every hardware consequence.

At `0x80370CE8`, r3 receives live HID2. `0x80370CEC` computes `r3 = rotl32(HID2,3) & 1 = (HID2 >> 29) & 1`, because IBM mask bit 31 is numeric bit 0. The Rc bit updates **CR0** to EQ for 0 or GT for 1; its SO bit reflects XER.SO. Other CR fields are preserved, and XER itself is not written. The B-form word `4182008C` has BO=12, BI=2, AA=0, LK=0, and target `0x80370D7C`, so it branches iff CR0.EQ. The preceding connected helper issued `HID2_out = HID2_in | 0xA0000000` at `0x80370BB0`. A successful architectural write/read forces the tested numeric bit 29 (`0x20000000`), selecting the paired block. This is a conditional claim about live SPR behavior, not an invented initial HID2 value. For the observed synthetic run, the read is `0xE0000000` and r3 becomes 1; a plain left shift would incorrectly yield 0.

On the paired edge, `lis r3,0x805F` followed by `addi r3,r3,0x1F38` yields `0x805F1F38`. Both instructions use r3, and `lis` uses the special rA=0 literal-zero rule. `psq_l` has W=0, I=0, signed 12-bit displacement 0, and uses GQR0. The preceding connected helper explicitly wrote GQR0=0. If MSR[FP], HID2[PSE] and HID2[LSQE] permit execution, it reads two consecutive 32-bit big-endian source words at `0x805F1F38..3F` and initializes f0 PS0/PS1 under GQR0's floating load format. `ps_mr` then copies **both** lanes of f0 into each destination f1..f31, exactly once and in ascending order. Each of the 31 words has opcode 4, XO 72, source f0, Rc=0 and destination encoded as its register number. This range has no CR/FPSCR update, address arithmetic, GPR write, or RAM store.

Both edges meet at `0x80370D7C`. `lfd f0,0x5A30(r13)` reads eight bytes at `r13 + 0x5A30`; the validated entry helper established r13=`0x805EC500`, so the selected address is `0x805F1F30..37`. It is a non-update D form: r13 is unchanged. Each of the next 31 words has opcode 63, XO 72, source f0, Rc=0 and destination f1..f31. They perform `fmr`, whose source has just been loaded as a double. Do not equate this with 31 host double assignments for full paired state: the Gekko instruction's handling of the PS1 part, especially after a paired seed, requires a two-lane observation or a separately validated architectural model. The manual explicitly distinguishes `fmr` behavior by whether the source is a double or paired single; for a paired source it preserves destination PS1. The previous PS1 contents and exact double-form destination representation therefore remain relevant.

`mtfsf 0xFF,f0` at `0x80370DFC` has primary 63, FM=255, source f0, XO=711 and Rc=0. It writes all eight FPSCR fields from the **low 32 bits** of f0's freshly loaded 64-bit double pattern, except that FEX and VX are derived summary bits rather than copied as source bits. Rc=0 means no CR1 update. The `blr` leaves final PC=`0x80003418` on this call. If the paired edge ran, final r3 remains `0x805F1F38`; if it was skipped, r3 remains zero from the test. The routine does not restore its input r3.

## State contract and unknown producers

| State | Producer or dependency | Locally proven consumer/effect | Status |
|---|---|---|---|
| MSR | Live incoming MSR; earlier wrapper ORs FP | Re-read, OR FP, full `mtmsr` write | **PROVEN words**, incoming value platform dependent |
| HID2 | Earlier `mfspr/OR/mtspr` helper and live readback | Numeric bit29 controls paired edge | **PROVEN conditional**, retail readback **UNKNOWN** |
| GQR0 | Earlier eight explicit zero writes | `psq_l` selects GQR0 load format | **PROVEN write sequence**, hardware readback/exception effects unmeasured |
| r13 | Entry helper at `0x80003334..38` | SDA effective address `0x805F1F30` | **PROVEN connected prefix** |
| Paired source | Loader/IPL/FST state at `0x805F1F38..3F` | Optional paired load and 31 two-lane copies | **UNKNOWN retail bytes** |
| Double/FPSCR source | Loader/IPL/FST state at `0x805F1F30..37` | `lfd`, 31 `fmr`, all-field `mtfsf` | **UNKNOWN retail bytes** |
| XER.SO | Incoming XER | CR0.SO on `rlwinm.` | **UNKNOWN retail input**, XER not modified |
| LR | Linked call at `0x80003414` | `blr` return `0x80003418` | **PROVEN** |

Both source ranges are outside populated DOL sections. The apploader's ordinary DOL-loading path clears the containing BSS interval, but a later IPL-dependent FST transfer can overlap it. Thus later CRT zeroing and static DOL contents do not establish retail bytes **at the earlier read**. The synthetic HLE startup-only run recorded 16 zero source bytes, PS0 for selected f0/f1/f31 as zero, and FPSCR zero; its GDB path did not expose PS1. Those observations select an HLE scenario only. Paired exceptional encodings, exact PS1 state and potential FP exception effects are not proven by PS0 samples.

## Portable reconstruction boundary

The corresponding portable C++17 contract can only be expressed with explicit state and validated floating/platform operations at present:

```cpp
// Contract sketch, not a connected native parity implementation.
void SeedFloatingState(BootState& s, PlatformEffects& platform, Memory& mem) {
    s.gpr[3] = platform.read_msr();
    s.gpr[3] |= 0x2000u;
    platform.write_msr(s.gpr[3]);
    const uint32_t hid2 = platform.read_hid2();
    s.gpr[3] = (hid2 >> 29) & 1u;
    s.cr0 = Cr0FromSignedResultAndXerSo(s.gpr[3], s.xer);
    if (s.gpr[3] != 0) {
        s.gpr[3] = 0x805F0000u + 0x1F38u;
        auto pair = platform.psq_load_gqr0(mem, s.gpr[3]); // exact two-lane effect
        s.fpr[0] = pair;
        for (unsigned i = 1; i != 32; ++i)
            platform.ps_mr(s.fpr[i], s.fpr[0]);
    }
    platform.lfd(s.fpr[0], mem, s.gpr[13] + 0x5A30u);
    for (unsigned i = 1; i != 32; ++i)
        platform.fmr(s.fpr[i], s.fpr[0]);
    platform.mtfsf_all(s.fpscr, s.fpr[0]); // derive FEX/VX, do not copy raw bits
}
```

The hooks are **explicit unresolved proof obligations**, not permission to skip, hard-code or declare the block native-complete. A native x64 collapse can remove hardware mechanisms only after downstream consumers and two-lane/FPSCR behavior are validated. It must compare the complete reconstructed prefix from `0x80003154`, including branch, r3, CR0, LR, MSR and all observable FPR state, against a same-run reference. The current connected native path stops before this routine.

## Validation and falsification

1. The extracted file SHA-256 matched the pinned PAL fixture. The contiguous word spans satisfy executable assertions: `word(0x80370D00 + 4*(n-1)) = 0x10000090 | (n << 21)` and `word(0x80370D80 + 4*(n-1)) = 0xFC000090 | (n << 21)` for all n=1..31. Independent bit extraction checked the branch (BO=12, BI=2, displacement +0x8C), `psq_l` (W=0, I=0), and `mtfsf` (FM=255, f0, XO=711, Rc=0).
2. **Only after that decode**, `q.py dis 0x80370CDC 74` agreed on all 74 mnemonics, operands, and the branch target. This agreement is a cross-check, not authority over the bytes.
3. The [IBM Gekko User's Manual, v1.2](https://doc.kodewerx.org/documents/gekko_user_manual.pdf), `rlwinm` (p. 12-198), `psq_l` (p. 12-157), `ps_mr` (p. 12-179), `lfd` (p. 12-94), `fmr` (p. 12-69), and `mtfsf` (p. 12-137), supports the architectural effects above. Any divergent disassembler, prior IR, host float behavior or synthetic oracle loses to the DOL bytes plus ISA.
4. Falsification targets: capture PS0 **and** PS1 at `0x80370D00`, `0x80370D7C`, `0x80370D80` and `0x80003418`; mutate only supplied source bytes/exception encodings under an instrumented same-run oracle; verify CR0.SO with XER.SO=1; verify that a model using `hid2 << 3` instead of rotate rejects `0xE0000000`; verify FPSCR derived FEX/VX with source bits that disagree. Do not promote the section to native parity until the connected run passes these gates.

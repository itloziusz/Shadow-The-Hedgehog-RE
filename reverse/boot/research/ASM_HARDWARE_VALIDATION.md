# Assembly-first validation: DOL hardware call (PAL GUPP8P)

Scope: the connected DOL call `0x80003158 -> 0x80003400` and its return to
`0x8000315C`. This is an independent instruction/branch/state audit of the
synthetic Dolphin startup-only run. It is **not** evidence of a retail IPL or
console handoff, complete game boot, or complete floating-point lane parity.

## Oracle identity and limits

- **PROVEN input identity:** the synthetic ISO SHA-256 is
  `a8a526ef7006a2d2a8d41f536ef30b208ff197c185c05e9b2276e6e93cb41a8e`.
  Its embedded `boot.bin`, `bi2.bin`, `apploader.img`, `main.dol`, and `fst.bin`
  compare byte-for-byte with the corresponding read-only PAL GUPP8P `sys/`
  files, at disc offsets `0`, `0x440`, `0x2440`, `0x20300`, and `0x5A1A00`.
  `main.dol` SHA-256 is
  `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
  The ISO has the five verified startup files; it does not contain the game's
  ordinary file payload.
- **OBSERVED synthetic oracle:** local Dolphin interpreter (`CPUCore=0`),
  executable SHA-256
  `db536025ee02190c20ccb9bfa94ee36ce28bb996f2e88574e46734b353bdc526`,
  debugger enabled, cold launch with a distinct user directory per run. RSP
  checkpoints used software breakpoints and then removed them before the next
  `continue`. Four captures, including two through return, are in the ignored
  repository `build/` directory. The two final captures agree on every field
  exposed by the capture helper. An older entry-only Dolphin 2606-368 capture
  independently agrees on the entry GPR/MSR/HID0/L2CR and low-memory values;
  its binary is unavailable for hash comparison and it is not a second
  full-path oracle.
- **PROVEN interface limit:** this Dolphin build's GDB stub reports FPR
  registers 32..63 as `PS0AsU64()` only (`GDBStub.cpp` register handler).
  It does not expose PS1 or HID2 as direct GDB registers. HID2 values below
  are observed indirectly in GPR3 immediately after `mfspr hid2`. Full FPR
  lane and all hardware/cache internal state remain unmeasured.
- **OBSERVED synthetic entry:** PC `0x80003154`, MSR `0x00002032`, HID0
  `0x0011C464`, L2CR `0`, FPSCR `0`, `[0x80000028]=0x01800000`,
  `[0x80000038]=0x817E74E0`, and the 16 FPR-source bytes
  `[0x805F1F30..0x805F1F3F]` are all zero. The earlier connected native
  helper's stop at `0x80003158` matches this run's r1 `0x8060C5F0`, r2
  `0x805FA780`, r13 `0x805EC500`, PC/LR `0x80003158`.

`PROVEN` below means the exact PAL DOL instructions and operands checked via
read-only `q.py dis`; `OBSERVED` means this **synthetic/HLE** execution. These
labels do not promote synthetic startup values to retail pre-entry constants.

## CFG and instruction/data-flow slices

| PPC range | Inputs, exact operations, stack/global effects | Successor and observed state |
|---|---|---|
| `0x80003400..0x80003420` | **PROVEN:** `mfmsr` -> r0, `ori 0x2000`, `mtmsr`; `mflr r31`; three ordered `bl` calls to `0x80371714`, `0x80370CDC`, `0x80372838`; `mtlr r31; blr`. No stack frame here. **OBSERVED:** input/output MSR `0x2032`; r31 becomes `0x8000315C` and is *not* restored to its pre-call zero value. | Calls in listed order, then `0x8000315C`. This non-ABI r31 effect matters to cross-boundary register parity. |
| `0x80371714..0x80371764` | **PROVEN:** saves LR at caller r1+4, creates 8-byte frame; `0x80370BA8` reads HID2; `oris r3,0xA000`; `0x80370BB0` writes HID2; `0x803725F4` reads HID0, ORs `0x0800`, writes HID0; `sync`; `li r3,0` and `mtspr gqr0..gqr7,r3`; restores frame/LR. **OBSERVED:** HID2 read and written value `0xE0000000` (r3 at `0x80371724/28`), HID0 stays `0x0011C464`. r1 `0x8060C5F0 -> 0x8060C5E8 -> 0x8060C5F0`. | `0x80370CDC`; all GQRs set to zero by eight explicit stores, although the stub cannot read GQR contents back. |
| `0x80370CDC..0x80370CF0` | **PROVEN:** read/set MSR FP bit; `mfspr hid2` into r3; `rlwinm. r3,r3,3,31,31` sets CR0; `beq 0x80370D7C`. **OBSERVED:** HID2 r3 `0xE0000000` at `0x80370CEC`; mask result r3 `1`, CR0 GT (`CR=0x40000000`) at `0x80370CF0`. | `beq` **not taken**; next PC `0x80370CF4`. |
| `0x80370CF4..0x80370D78` | **PROVEN:** r3 becomes `0x805F1F38`; `psq_l f0,0(r3),0,0` reads 8 bytes (GQR0), then 31 sequential `ps_mr f1..f31,f0` copies both paired lanes. **OBSERVED:** bytes read are zero; GDB PS0 for f0/f1/f31 remains zero at available checkpoints. No RAM writes. | `0x80370D7C`. PS1 copy behavior is instruction-backed but PS1 value is not directly observed by GDB. |
| `0x80370D7C..0x80370E00` | **PROVEN:** `lfd f0,0x5A30(r13)` reads 8 bytes at `0x805F1F30`; 31 `fmr f1..f31,f0` affect PS0, and `mtfsf 0xFF,f0` updates FPSCR. **OBSERVED:** r13 `0x805EC500`, source bytes zero, PS0 f0/f1/f31 zero, FPSCR zero before/after `mtfsf`. The Dolphin interpreter source uses `SetPS0` for `fmr` and both lanes for `ps_mr`; that supports, but does not independently measure, surviving PS1. | Return into `0x80372838`. CR0 remains GT from HID2 mask. |
| `0x80372838..0x803728A4` | **PROVEN:** saves LR and r31/r30 in 16-byte frame at r1 `0x8060C5E0`; HID0 read twice via `0x80370AEC`. At `0x80372860`, `bne 0x80372874` tests `r0=HID0 & 0x8000`; at `0x80372880`, `bne 0x80372894` tests `r0=HID0 & 0x4000`; `0x803728A0` tests L2CR top bit via `0x80370AFC`. **OBSERVED:** r0 `0x8000` and `0x4000` respectively, CR0 GT, so both L1 init calls are skipped. At L2 test r0=0/CR0 EQ, so L2 init is entered at `0x803728A4`. | Three resolved branch successors: `0x80372874`, `0x80372894`, `0x803728A4`. No assumption that these choices hold for a retail IPL. |
| `0x803728A4..0x803728F8` | **PROVEN:** saves original MSR in r30, writes MSR `0x30`, clears L2CR top bit, calls invalidate `0x80372640`, restores MSR, then sets L2CR enable bit and masks configuration; logger `0x80370C8C` only stores arguments and PS0 f1..f8 in its own 0x70-byte stack frame and returns. **OBSERVED:** MSR `0x2032` before, `0x30` at `0x803728BC..0x803728D4`, restored `0x2032` by `0x803728DC`; L2CR 0 before, `0x80000000` by `0x803728EC`. | `0x803728F8`. Cache contents/timing and hardware-generated status changes are outside GDB's measured register snapshot. |
| `0x80372640..0x803726D4` | **PROVEN:** 16-byte stack frame; writes L2CR invalidate request (`oris 0x20`), polls low status bit at `0x80372684`; `rlwinm` at `0x8037268C` masks out the request bit and the guest writes L2CR at `0x80372690`; polls again at `0x803726C0`, restores frame. **OBSERVED:** L2CR `0x00200000` at `0x80372678`; first `0x80372684` has r0=0/CR0 EQ and advances to `0x80372688`; L2CR is zero by `0x803726B4`; first `0x803726C0` has r0=0/CR0 EQ and advances to `0x803726C4`. The observed zero is explained by the guest `mtspr` path; no hardware auto-clear is needed to explain it. | Returns to `0x803728D4`, no poll back-edge on these observed visits. |
| `0x803728F8..0x80372928` and `0x80373378..0x8037358C` | **PROVEN:** `r3=1`, `r4=0x803726D8`, call handler installer. Installer 0x30-byte frame at r1 `0x8060C5B0`; `0x8037611C` saves/clears MSR interrupt enable, computes slot from `0x80586CB0 + ((1<<2)&mask(14..29)) = 0x80586CB4`; loads prior pointer; stores new pointer at `0x803733C0`; selector 1 takes `bne 0x80373564`, skipping selector-0x10 vector path; `0x80376144` restores interrupt state; returns old pointer in r3. **OBSERVED:** slot `0` just before store at `0x803733C0`, `0x803726D8` at `0x80373564` and hardware return. Old pointer r30/r3 is zero. MSR remains `0x2032`. Logger and stack restores follow. | Returns via `0x80372928 -> 0x8000341C -> 0x8000315C`. No indirect call occurs on this selected path; the handler pointer is stored for later possible use. |

The named subcalls above were independently inspected: `0x80370BA8` and
`0x80370AEC` are single-`mfspr` accessors; `0x80370BB0` and `0x80370B04`
are single-`mtspr` writers; `0x80370ADC/AE4` read/write MSR. The installer
skips `0x803733C8..0x80373560` because selector 1 is not 0x10. Its long
exception/vector and linked-object operations are **not** claimed executed.

## Same-run state comparison at the hardware boundary

| Field | Before call at `0x80003158` | On return at `0x8000315C` | Basis |
|---|---:|---:|---|
| r1 / r2 / r13 | `8060C5F0` / `805FA780` / `805EC500` | same | OBSERVED; stack frames fully unwind |
| r0 / r3 / r4 / r5 / r6 / r31 | `0` / `0` / `0` / `0` / `0` / `0` | `8000341C` / `8056157C` / `00002032` / `00002032` / `1` / `8000315C` | OBSERVED; do not infer ABI preservation |
| PC / LR / CTR / XER | `80003158` / `80003158` / `0` / `0` | `8000315C` / `8000315C` / `0` / `0` | OBSERVED |
| MSR / CR / FPSCR | `00002032` / `20000000` / `0` | same | OBSERVED; intermediate CR0 GT and MSR `0x30` above |
| HID0 / HID2 / L2CR | `0011C464` / `E0000000`* / `0` | `0011C464` / `E0000000`* / `80000000` | HID0/L2CR OBSERVED; HID2 inferred from both `mfspr -> r3` checkpoints (*) |
| handler slot `0x80586CB4` | `0` | `803726D8` | OBSERVED memory before/after the exact store |
| FPR PS0 f0/f1/f31 | `0` / `0` / `0` | same | OBSERVED only for these sampled registers; PS1 unavailable |

Observed return PC `0x8000315C` demonstrates this *original PPC section* can
hand state to the next DOL instruction under the synthetic oracle. It does not
validate the native C++ continuation or the later CRT/game transition.

## Portable representation boundary

An equivalent native routine must receive the **measured loader/hardware
state as input**, not bake the observed values into code. The instruction-level
shape is:

```cpp
// Schematic C++17 contract; Platform operations require their own validation.
void hardware_start(GuestState& s, Platform& p) {
  s.msr |= 0x2000;
  s.r31 = s.lr;                         // Original call does not restore r31.
  p.write_hid2(p.read_hid2() | 0xA0000000u);
  p.write_hid0(p.read_hid0() | 0x00000800u);
  for (unsigned i = 0; i != 8; ++i) p.write_gqr(i, 0);
  s.msr |= 0x2000;
  const uint32_t hid2 = p.read_hid2();
  const uint32_t hid2_rotl3 = (hid2 << 3) | (hid2 >> 29);
  if ((hid2_rotl3 & 1u) != 0) {        // rlwinm. rotates, never plain-shifts.
    s.fpr[0] = p.psq_load_pair(s.memory, 0x805F1F38u, p.read_gqr(0));
    for (unsigned i = 1; i != 32; ++i) s.fpr[i] = s.fpr[0];
  }
  p.lfd_ps0(s.fpr[0], s.memory, s.r13 + 0x5A30u);
  for (unsigned i = 1; i != 32; ++i) p.fmr_ps0(s.fpr[i], s.fpr[0]);
  p.mtfsf_all(s.fpscr, s.fpr[0]);
  if ((p.read_hid0() & 0x8000u) == 0) p.init_l1_instruction();
  if ((p.read_hid0() & 0x4000u) == 0) p.init_l1_data();
  if ((p.read_l2cr() & 0x80000000u) == 0) p.init_l2_from_ppc_sequence();
  p.install_exception_handler(1, 0x803726D8u, s.memory);
  // Exact r0/r3/r4/r5/r6, CR, LR and intermediate MSR effects are also
  // observable and must be reproduced in a connected native validation.
}
```

This is a **data-flow sketch, not a parity implementation**: `psq_load_pair`,
FPSCR conversion, cache/interrupt behavior, handler installation and exact
volatile-register effects must be expanded from their instructions and checked
against the same-run state. In particular the call must not be marked native
validated merely because it returns the right PC. PS1 and any hidden cache
state consumed later remain **UNKNOWN** to this RSP capture. No claim here
proves those operations redundant for native x64; later reads must be traced.
The observed HID2 `0xE0000000` is a mandatory rotate regression case:
`rotl32(HID2,3) & 1 == 1`, while `(HID2 << 3) & 1 == 0`; a native plain shift
would silently select the wrong paired-single branch.

## Next validation gate

1. Reconstruct the selected PPC operations and ordered side effects with no
   checkpoint-value constants. Compare all exposed boundary fields in the
   table and branch decisions, then rerun from `0x80003154` through
   `0x8000315C` and the next instruction.
2. Obtain a direct PS1 observation or a separate validated lane test before
   claiming complete FPR state parity; record GQR and cache state where later
   guest instructions observe them.
3. Keep the retail IPL pre-entry question separate. This startup-only HLE
   input proves the synthetic conditional path, not the retail one.

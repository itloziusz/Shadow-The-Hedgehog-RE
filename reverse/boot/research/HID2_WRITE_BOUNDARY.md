# HID2 write boundary from the PAL DOL

Scope: the former native stop at `0x80371724`, its HID2 write leaf, and the
first explicit downstream HID2 read and branch. The authority is the original
read-only PAL GUPP8P `sys/main.dol`, SHA-256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
`PROVEN` below refers to DOL bytes, encoding, and conditional ISA effects;
`OBSERVED-HLE` refers only to the pinned startup-only Dolphin interpreter run;
`UNKNOWN` remains unresolved for the retail IPL and native continuation.
The connected C++ probe now issues a labelled SPR write request for this leaf;
it does not claim to reproduce the HID2 hardware readback.

## Raw words and independently decoded fields

The DOL header maps file `0x2600` to text VA `0x80008D40` for the text1
addresses below, and file `0x100` to VA `0x80003100` for `0x80003414`.
The table was generated from each four-byte big-endian DOL word and its
bitfields before comparing the existing assembly notes. For X-form SPR
operands, `SPR=((word>>16)&31)|(((word>>11)&31)<<5)`; `0x31C` decodes as
SPR 920 (HID2), and `0x008` as SPR 8 (LR). Branch displacement is signed,
relative to the instruction address because AA=0. Arrows are decoded branch
targets, not inferred function boundaries.

| VA | File offset | Raw bytes / BE word | Encoding, decode, architectural effect |
|---|---:|---|---|
| `80371724` | `36AFE4` | `64 63 A0 00` / `6463A000` | D op25, rS3 rA3 UI=`A000`: `oris r3,r3,0xA000`; r3 ← r3 OR `0xA0000000`. |
| `80371728` | `36AFE8` | `4B FF F4 89` / `4BFFF489` | I op18, LI displacement `-0xB78`, AA0 LK1 → `bl 80370BB0`; LR ← `0x8037172C`. |
| `8037172C` | `36AFEC` | `48 00 0E C9` / `48000EC9` | I op18, LI displacement `+0xEC8`, AA0 LK1 → `bl 803725F4`; LR ← `0x80371730`. This is the next HID0/ICFI call. |
| `80370BA8` | `36A468` | `7C 78 E2 A6` / `7C78E2A6` | X op31 XO339 rD3 SPR920: `mfspr r3,HID2`; r3 ← live HID2. Producer of r3 at `0x80371724`. |
| `80370BAC` | `36A46C` | `4E 80 00 20` / `4E800020` | XL op19 XO16 BO20 BI0 LK0: `blr`; returns to `0x80371724` on this call. |
| `80370BB0` | `36A470` | `7C 78 E3 A6` / `7C78E3A6` | X op31 XO467 rS3 SPR920: `mtspr HID2,r3`; requests the full r3 word be written. |
| `80370BB4` | `36A474` | `4E 80 00 20` / `4E800020` | XL op19 XO16 BO20 BI0 LK0: `blr`; returns to `0x8037172C` on this call. |
| `80371758` | `36B018` | `80 01 00 0C` / `8001000C` | D op32 rD0 rA1 d12: `lwz r0,12(r1)`; reads the earlier saved LR. |
| `8037175C` | `36B01C` | `38 21 00 08` / `38210008` | D op14 rD1 rA1 imm8: `addi r1,r1,8`; restores the entry SP. |
| `80371760` | `36B020` | `7C 08 03 A6` / `7C0803A6` | X op31 XO467 rS0 SPR8: `mtlr r0`; restores caller LR. |
| `80371764` | `36B024` | `4E 80 00 20` / `4E800020` | XL op19 XO16 BO20 BI0 LK0: `blr`; returns to `0x80003414` on this call. |
| `80003414` | `000414` | `48 36 D8 C9` / `4836D8C9` | I op18, LI displacement `+0x36D8C8`, AA0 LK1 → `bl 80370CDC`; LR ← `0x80003418`. |
| `80370CDC` | `36A59C` | `7C 60 00 A6` / `7C6000A6` | X op31 XO83 rD3: `mfmsr r3`; r3 ← current full MSR. |
| `80370CE0` | `36A5A0` | `60 63 20 00` / `60632000` | D op24 rS3 rA3 UI=`2000`: `ori r3,r3,0x2000`. |
| `80370CE4` | `36A5A4` | `7C 60 01 24` / `7C600124` | X op31 XO146 rS3: `mtmsr r3`; writes the full MSR. |
| `80370CE8` | `36A5A8` | `7C 78 E2 A6` / `7C78E2A6` | X op31 XO339 rD3 SPR920: `mfspr r3,HID2`; **first explicit later HID2 read** on the connected path. |
| `80370CEC` | `36A5AC` | `54 63 1F FF` / `54631FFF` | M op21 rS3 rA3 SH3 MB31 ME31 Rc1: `rlwinm. r3,r3,3,31,31`; r3 ← `rotl32(HID2,3)&1`, update CR0 from the result and XER.SO. |
| `80370CF0` | `36A5B0` | `41 82 00 8C` / `4182008C` | B op16 BO12 BI2 BD `+0x8C`, AA0 LK0 → `beq 80370D7C` iff CR0.EQ; otherwise next `0x80370CF4`. |

These are 18 distinct code words, four checked direct branch targets and
four checked SPR operands. There is no ordinary RAM load/store in the
`0x80371724..28`/`0x80370BB0..B4` write slice. No instruction in that
slice writes CR, XER, CTR, FPSCR, r1, r2, r13, r31 or an FPR; the `bl`
changes LR, while the `blr` consumes it. The return and later branch depend
on the previously saved stack LR and on the intervening HID0/GQR path,
respectively; those intervening instructions are separately byte-inventoried
in [the paired-setup state note](PAIRED_SETUP_STATE.md).

## Write-to-read dependency and state boundary

Let `H2_in` be the live value produced by `mfspr` at `0x80370BA8`, not
an assumed retail power-on value. **PROVEN ISA:** at `0x80371724`,
`r3=H2_in | 0xA0000000`. The following `bl` sets LR to `0x8037172C`;
the leaf's `mtspr` issues that exact 32-bit write operand to HID2 and does
not alter r3. Its `blr` resumes at `0x8037172C`. No stack access occurs
in the leaf. The mask forces numeric bits 31 and 29; it does not force bit
30 or determine any unforced incoming bit. Per the [Gekko User's Manual,
HID2 register table](https://doc.kodewerx.org/documents/gekko_user_manual.pdf),
those forced bits are LSQE and PSE. Architectural retention, hardware
ordering and exception state are preconditions for using the requested word
as a later readback; the DOL alone does not measure them.

The first explicit later HID2 consumer is `mfspr` at `0x80370CE8`, reached
after the helper's HID0 invalidate command, `sync`, eight GQR writes,
restored stack/LR and the direct `bl` at `0x80003414`. Its result is tested
by `rlwinm.` at `0x80370CEC`: `rotl32(HID2_readback,3)&1` equals numeric
HID2 bit 29. CR0 is EQ if zero and GT if one, with SO copied from XER.SO;
other CR fields and XER remain unchanged. `beq` at `0x80370CF0` takes the
`0x80370D7C` edge only when that result is zero. **PROVEN conditional:**
if the earlier HID2 write is accepted and its forced PSE bit is visible
to this later read, the branch falls through to `0x80370CF4`; this is
derived from the live read, not a license to force a branch. That path
first attempts a paired load at `0x80370CFC`, whose actual FP/cache effects
remain a separate validation gate. The OR and write themselves do not set
CR0; the later record-form rotate does.

## Synthetic observation, limits, and adversarial checks

**OBSERVED-HLE only:** the ignored, pinned
`build/boot-paired-setup-capture.json` has synthetic disc SHA-256
`a8a526ef7006a2d2a8d41f536ef30b208ff197c185c05e9b2276e6e93cb41a8e`
and Dolphin executable SHA-256
`db536025ee02190c20ccb9bfa94ee36ce28bb996f2e88574e46734b353bdc526`.
It reports r3=`0xE0000000`, r1=`0x8060C5E8`, LR=`0x80371724`,
CR=`0x20000000`, XER=0 at `0x80371724`; r3 is still `0xE0000000`,
LR=`0x8037172C` at `0x8037172C`. A separate ignored hardware capture
reports r3=`0xE0000000` immediately after the later `mfspr` at
`0x80370CEC`, then r3=1, CR=`0x40000000` at `0x80370CF0`, and fallthrough
to `0x80370CF4`. The RSP interface does not expose HID2 as a named SPR;
these values come from r3 after the reads. The write instruction's outgoing
r3 at `0x8037172C` is **not** itself a HID2 readback measurement. Neither
capture proves retail IPL initial HID2 or physical cache/paired behavior.

- A plain left shift for `rlwinm.` is false for the measured
  `0xE0000000`: `(H2<<3)&1=0` while `rotl32(H2,3)&1=1`.
- Replacing the whole outgoing HID2 with `0xA0000000` loses unforced
  incoming bits. A native model must keep `H2_in` explicit.
- Treating `bl` as no LR write would return from the leaf to the wrong
  address. Treating `mtspr` as a RAM store or r3 mutation is also wrong.
- The `0x8037172C` HID0/ICFI call and subsequent `sync` cannot be skipped
  just because the first explicit HID2 read occurs after them.

**Current fail-closed native boundary:** the probe now derives
`H2_in|0xA0000000`, records the `mtspr HID2,r3` request at `0x80370BB0`,
and compares exposed state at `0x8037172C` for the supplied HLE scenario.
It subsequently records the HID0 ICFI request and stops at `0x80371730`
before `sync`. Full state parity cannot yet claim HID2 hardware readback,
ICFI cache effects, GQR or paired execution. The next HID2
consumer gate is the actual read at `0x80370CE8` and its CR0 branch.
**UNKNOWN:** retail `H2_in`, privilege/exception context, all unforced
HID2 bits, direct post-write SPR readback and timing on retail hardware,
and whether asynchronous effects alter the straight-line path. No native
hardware-readback equivalence or game transition is claimed here.

## Validation command

From the repository root, the read-only checker must report
`PASS raw words=18 branches=4 SPRs=4 data=0`:

```text
python reverse/boot/tools/verify_binary_note.py <read-only-PAL-main.dol> reverse/boot/research/HID2_WRITE_BOUNDARY.md --expected-words 18 --expected-branches 4 --expected-sprs 4 --required-code-range 80371724:8037172C --required-code-range 80370BA8:80370BB4 --required-code-range 80371758:80371764 --required-code-range 80003414:80003414 --required-code-range 80370CDC:80370CF0
```

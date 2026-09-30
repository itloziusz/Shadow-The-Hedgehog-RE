# PAL GUPP8P boot prefix: binary-first excavation

**Fixture:** read-only `sys/main.dol`, 5,773,024 bytes, SHA-256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
This note decodes bytes independently of `q.py`; the disassembler comparison is
at the end. It proves instruction identity and local architectural effects,
not the unknown initial HID/FPR state or a complete native boot.

## DOL address mapping and decoding rules

The big-endian DOL header supplies text section 0 at file offset `0x000100`,
VA `0x80003100`, size `0x2500`, and text section 1 at file offset `0x002600`,
VA `0x80008D40`, size `0x4A1F20`. Every byte below lies in one of those
sections. File offset is `section_file_offset + VA - section_VA`; the second
section accounts for the non-obvious `0x36xxxx` offsets.

In the tables, `D` is a primary-opcode immediate form, `X` is primary opcode
31 with a 10-bit XO, `XL` is opcode 19, and `I` is opcode 18. The primary
opcode is bits 0–5 (word `>>26`), `rD/rS` bits 6–10, `rA` bits 11–15,
`rB` bits 16–20, and XO bits 21–30. A 32-bit `mfspr/mtspr` word stores its
SPR number in **swapped five-bit halves**: with conventional numeric shifts,
`SPR = ((word >> 16) & 31) | (((word >> 11) & 31) << 5)`.
The first term is PPC bits 11–15 and the second is PPC bits 16–20; spelling
this in shift form prevents reversing the two numbering conventions. Thus
`0x7C78E2A6` addresses HID2 (`920`), not SPR
`796`, and `0x7C70FAA6` addresses HID0 (`1008`), not SPR `543`.
For an I-form branch, displacement is the sign-extended 26-bit field
`word & 0x03FFFFFC`; all branches below have `AA=0`, so target = PC +
displacement. `LK=1` writes PC+4 to LR.

## Entry and register-clear helper

| VA | File offset | Raw bytes / BE word | Field decode → instruction | Architectural effect |
|---|---:|---|---|---|
| `80003154` | `000154` | `48 00 01 5D` / `4800015D` | I op18, disp `+0x15C`, AA0 LK1 → `bl 800032B0` | LR=`80003158`; branch. |
| `80003158` | `000158` | `48 00 02 A9` / `480002A9` | I op18, disp `+0x2A8`, AA0 LK1 → `bl 80003400` | LR=`8000315C`; branch. |

All 29 zeroing words below are **op14 / D-form** `addi rD,rA,simm16` with
`rA=0`, immediate `0`. This encodes a literal zero rather than a read of r0.
The `rD` field is shown in each row. No CR/XER/FPR/memory state is changed by
these words. `r1`, `r2`, and `r13` are intentionally absent from this zero run.

| VA | File offset | Raw bytes / BE word | rD / decoded instruction |
|---|---:|---|---|
| `800032B0` | `0002B0` | `38 00 00 00` / `38000000` | 0 / `li r0,0` |
| `800032B4` | `0002B4` | `38 60 00 00` / `38600000` | 3 / `li r3,0` |
| `800032B8` | `0002B8` | `38 80 00 00` / `38800000` | 4 / `li r4,0` |
| `800032BC` | `0002BC` | `38 A0 00 00` / `38A00000` | 5 / `li r5,0` |
| `800032C0` | `0002C0` | `38 C0 00 00` / `38C00000` | 6 / `li r6,0` |
| `800032C4` | `0002C4` | `38 E0 00 00` / `38E00000` | 7 / `li r7,0` |
| `800032C8` | `0002C8` | `39 00 00 00` / `39000000` | 8 / `li r8,0` |
| `800032CC` | `0002CC` | `39 20 00 00` / `39200000` | 9 / `li r9,0` |
| `800032D0` | `0002D0` | `39 40 00 00` / `39400000` | 10 / `li r10,0` |
| `800032D4` | `0002D4` | `39 60 00 00` / `39600000` | 11 / `li r11,0` |
| `800032D8` | `0002D8` | `39 80 00 00` / `39800000` | 12 / `li r12,0` |
| `800032DC` | `0002DC` | `39 C0 00 00` / `39C00000` | 14 / `li r14,0` |
| `800032E0` | `0002E0` | `39 E0 00 00` / `39E00000` | 15 / `li r15,0` |
| `800032E4` | `0002E4` | `3A 00 00 00` / `3A000000` | 16 / `li r16,0` |
| `800032E8` | `0002E8` | `3A 20 00 00` / `3A200000` | 17 / `li r17,0` |
| `800032EC` | `0002EC` | `3A 40 00 00` / `3A400000` | 18 / `li r18,0` |
| `800032F0` | `0002F0` | `3A 60 00 00` / `3A600000` | 19 / `li r19,0` |
| `800032F4` | `0002F4` | `3A 80 00 00` / `3A800000` | 20 / `li r20,0` |
| `800032F8` | `0002F8` | `3A A0 00 00` / `3AA00000` | 21 / `li r21,0` |
| `800032FC` | `0002FC` | `3A C0 00 00` / `3AC00000` | 22 / `li r22,0` |
| `80003300` | `000300` | `3A E0 00 00` / `3AE00000` | 23 / `li r23,0` |
| `80003304` | `000304` | `3B 00 00 00` / `3B000000` | 24 / `li r24,0` |
| `80003308` | `000308` | `3B 20 00 00` / `3B200000` | 25 / `li r25,0` |
| `8000330C` | `00030C` | `3B 40 00 00` / `3B400000` | 26 / `li r26,0` |
| `80003310` | `000310` | `3B 60 00 00` / `3B600000` | 27 / `li r27,0` |
| `80003314` | `000314` | `3B 80 00 00` / `3B800000` | 28 / `li r28,0` |
| `80003318` | `000318` | `3B A0 00 00` / `3BA00000` | 29 / `li r29,0` |
| `8000331C` | `00031C` | `3B C0 00 00` / `3BC00000` | 30 / `li r30,0` |
| `80003320` | `000320` | `3B E0 00 00` / `3BE00000` | 31 / `li r31,0` |

| VA | File offset | Raw bytes / BE word | Field decode → instruction | Architectural effect |
|---|---:|---|---|---|
| `80003324` | `000324` | `3C 20 80 60` / `3C208060` | D op15, d1 a0 imm`8060` → `lis r1,8060` | r1=`0x80600000`. |
| `80003328` | `000328` | `60 21 C5 F0` / `6021C5F0` | D op24, s1 a1 imm`C5F0` → `ori r1,r1,C5F0` | r1=`0x8060C5F0`. |
| `8000332C` | `00032C` | `3C 40 80 5F` / `3C40805F` | D op15, d2 a0 imm`805F` → `lis r2,805F` | r2=`0x805F0000`. |
| `80003330` | `000330` | `60 42 A7 80` / `6042A780` | D op24, s2 a2 imm`A780` → `ori r2,r2,A780` | r2=`0x805FA780`. |
| `80003334` | `000334` | `3D A0 80 5E` / `3DA0805E` | D op15, d13 a0 imm`805E` → `lis r13,805E` | r13=`0x805E0000`. |
| `80003338` | `000338` | `61 AD C5 00` / `61ADC500` | D op24, s13 a13 imm`C500` → `ori r13,r13,C500` | r13=`0x805EC500`. |
| `8000333C` | `00033C` | `4E 80 00 20` / `4E800020` | XL op19 XO16, BO20 BI0 LK0 → `blr` | PC=LR (`80003158` after connected entry). |

The helper has 36 instructions, no stack or memory access, and no direct
conditional branch. Its initial r0/r3–r12/r14–r31 values do not matter for
these outputs. Its link register **does** matter to the final `blr`. r1/r2/r13
are concrete constants established only by the six final D-form instructions.

## Wrapper and paired setup

`X` rows give the decisive XO and, for SPR instructions, the decoded SPR.
The `rD/rS` field is given as the register after the comma or arrow.

| VA | File offset | Raw bytes / BE word | Bitfields → decoded instruction | Local effect / successor |
|---|---:|---|---|---|
| `80003400` | `000400` | `7C 00 00 A6` / `7C0000A6` | X op31 XO83 d0 → `mfmsr r0` | r0←MSR. |
| `80003404` | `000404` | `60 00 20 00` / `60002000` | D op24 s0 a0 imm`2000` → `ori r0,r0,2000` | r0←r0 OR `0x2000`. |
| `80003408` | `000408` | `7C 00 01 24` / `7C000124` | X op31 XO146 s0 → `mtmsr r0` | MSR←r0; precise external consequences depend on prior MSR. |
| `8000340C` | `00040C` | `7F E8 02 A6` / `7FE802A6` | X op31 XO339 d31 SPR8 → `mflr r31` | r31←outer LR (`8000315C`). |
| `80003410` | `000410` | `48 36 E3 05` / `4836E305` | I op18 disp `+0x36E304` LK1 → `bl 80371714` | LR=`80003414`; branch. |
| `80003414` | `000414` | `48 36 D8 C9` / `4836D8C9` | I op18 disp `+0x36D8C8` LK1 → `bl 80370CDC` | LR=`80003418`; branch. Callee outside this excavation. |
| `80003418` | `000418` | `48 36 F4 21` / `4836F421` | I op18 disp `+0x36F420` LK1 → `bl 80372838` | LR=`8000341C`; branch. Callee outside this excavation. |
| `8000341C` | `00041C` | `7F E8 03 A6` / `7FE803A6` | X op31 XO467 s31 SPR8 → `mtlr r31` | LR←r31. |
| `80003420` | `000420` | `4E 80 00 20` / `4E800020` | XL op19 XO16 BO20 BI0 LK0 → `blr` | PC←outer return `8000315C` if r31 preserved by callees. |
| `80371714` | `36AFD4` | `7C 08 02 A6` / `7C0802A6` | X op31 XO339 d0 SPR8 → `mflr r0` | r0←LR (`80003414` on this call). |
| `80371718` | `36AFD8` | `90 01 00 04` / `90010004` | D op36 s0 a1 imm`0004` → `stw r0,4(r1)` | BE 32-bit saved LR at old SP+4. |
| `8037171C` | `36AFDC` | `94 21 FF F8` / `9421FFF8` | D op37 s1 a1 imm`FFF8` → `stwu r1,-8(r1)` | new SP=old SP−8; BE 32-bit old SP stored at new SP+0. |
| `80371720` | `36AFE0` | `4B FF F4 89` / `4BFFF489` | I op18 disp `−0xB78` LK1 → `bl 80370BA8` | LR=`80371724`; branch to HID2 read. |
| `80371724` | `36AFE4` | `64 63 A0 00` / `6463A000` | D op25 s3 a3 imm`A000` → `oris r3,r3,A000` | r3←r3 OR `0xA0000000`. |
| `80371728` | `36AFE8` | `4B FF F4 89` / `4BFFF489` | I op18 disp `−0xB78` LK1 → `bl 80370BB0` | LR=`8037172C`; branch to HID2 write. |
| `8037172C` | `36AFEC` | `48 00 0E C9` / `48000EC9` | I op18 disp `+0xEC8` LK1 → `bl 803725F4` | LR=`80371730`; branch to HID0 bit-set. |
| `80371730` | `36AFF0` | `7C 00 04 AC` / `7C0004AC` | X op31 XO598 → `sync` | Ordering barrier; hardware consequences need separate proof. |
| `80371734` | `36AFF4` | `38 60 00 00` / `38600000` | D op14 d3 a0 imm0 → `li r3,0` | r3=0. |
| `80371738` | `36AFF8` | `7C 70 E3 A6` / `7C70E3A6` | X op31 XO467 s3 SPR912 → `mtspr gqr0,r3` | GQR0=0. |
| `8037173C` | `36AFFC` | `7C 71 E3 A6` / `7C71E3A6` | X op31 XO467 s3 SPR913 → `mtspr gqr1,r3` | GQR1=0. |
| `80371740` | `36B000` | `7C 72 E3 A6` / `7C72E3A6` | X op31 XO467 s3 SPR914 → `mtspr gqr2,r3` | GQR2=0. |
| `80371744` | `36B004` | `7C 73 E3 A6` / `7C73E3A6` | X op31 XO467 s3 SPR915 → `mtspr gqr3,r3` | GQR3=0. |
| `80371748` | `36B008` | `7C 74 E3 A6` / `7C74E3A6` | X op31 XO467 s3 SPR916 → `mtspr gqr4,r3` | GQR4=0. |
| `8037174C` | `36B00C` | `7C 75 E3 A6` / `7C75E3A6` | X op31 XO467 s3 SPR917 → `mtspr gqr5,r3` | GQR5=0. |
| `80371750` | `36B010` | `7C 76 E3 A6` / `7C76E3A6` | X op31 XO467 s3 SPR918 → `mtspr gqr6,r3` | GQR6=0. |
| `80371754` | `36B014` | `7C 77 E3 A6` / `7C77E3A6` | X op31 XO467 s3 SPR919 → `mtspr gqr7,r3` | GQR7=0. |
| `80371758` | `36B018` | `80 01 00 0C` / `8001000C` | D op32 d0 a1 imm`000C` → `lwz r0,12(r1)` | r0←saved caller LR from old SP+4. |
| `8037175C` | `36B01C` | `38 21 00 08` / `38210008` | D op14 d1 a1 imm`0008` → `addi r1,r1,8` | SP restored to old SP. |
| `80371760` | `36B020` | `7C 08 03 A6` / `7C0803A6` | X op31 XO467 s0 SPR8 → `mtlr r0` | LR←saved caller LR. |
| `80371764` | `36B024` | `4E 80 00 20` / `4E800020` | XL op19 XO16 BO20 BI0 LK0 → `blr` | PC=`80003414` on connected call. |

The `stwu` stack write is distinct from the prior `stw`: it stores the old
SP at `oldSP−8`, while the saved LR remains at `oldSP+4` (`newSP+12`). No
conditional branch appears in this local block. The three nested calls are
not opaque: their word-level effects follow. `sync` is not evidence that
all possible cache or device side effects have been reconstructed.

## Nested HID accessors and HID0 setter

| VA | File offset | Raw bytes / BE word | Bitfields → decoded instruction | State effect |
|---|---:|---|---|---|
| `80370BA8` | `36A468` | `7C 78 E2 A6` / `7C78E2A6` | X op31 XO339 d3 SPR920 → `mfspr r3,hid2` | r3←HID2. |
| `80370BAC` | `36A46C` | `4E 80 00 20` / `4E800020` | XL op19 XO16 BO20 BI0 → `blr` | Return to `80371724`. |
| `80370BB0` | `36A470` | `7C 78 E3 A6` / `7C78E3A6` | X op31 XO467 s3 SPR920 → `mtspr hid2,r3` | HID2←old HID2 OR `0xA0000000`. |
| `80370BB4` | `36A474` | `4E 80 00 20` / `4E800020` | XL op19 XO16 BO20 BI0 → `blr` | Return to `8037172C`. |
| `80370BB8` | `36A478` | `7C 79 E3 A6` / `7C79E3A6` | X op31 XO467 s3 SPR921 → `mtspr hid1,r3` | Adjacent accessor; **not reached** by this call chain. |
| `80370BBC` | `36A47C` | `4E 80 00 20` / `4E800020` | XL op19 XO16 BO20 BI0 → `blr` | Adjacent return; not reached here. |
| `803725F4` | `36BEB4` | `7C 70 FA A6` / `7C70FAA6` | X op31 XO339 d3 SPR1008 → `mfspr r3,hid0` | r3←HID0. |
| `803725F8` | `36BEB8` | `60 63 08 00` / `60630800` | D op24 s3 a3 imm`0800` → `ori r3,r3,0800` | r3←old HID0 OR `0x00000800`. |
| `803725FC` | `36BEBC` | `7C 70 FB A6` / `7C70FBA6` | X op31 XO467 s3 SPR1008 → `mtspr hid0,r3` | Writes the computed word to HID0. Bit `0x0800` is an ICFI command; hardware may clear it before later readback. |
| `80372600` | `36BEC0` | `4E 80 00 20` / `4E800020` | XL op19 XO16 BO20 BI0 → `blr` | Return to `80371730`. |

`0x80372604` begins another function (`isync`); it is **outside** the
`0x803725F4..0x80372600` call target. The HID0/HID2 setters' *write words*
are proven by bytes. Gekko HID0 bit `0x0800` is an instruction-cache flash
invalidate command (ICFI) when instruction caching is enabled; it is
self-clearing when HID0[ICE] is enabled, so the written
`HID0_in | 0x0800` must **not** be used as the subsequent HID0 readback.
The [IBM Gekko User's Manual](https://doc.kodewerx.org/documents/gekko_user_manual.pdf),
Table 2-4 (ICFI, bit 20) and §3.4.1.4, specifies the next-cycle clearing
condition. The project hardware CFG/data-flow notes separately observe the
synthetic run's readback. The actual initial register values and all cache and
paired hardware consequences remain independent input dependencies.

## CFG, data flow, and evidence status

Connected local CFG: `80003154 → 800032B0..333C → 80003158 →
80003400..3410 → 80371714..1764`, with called leaves
`80370BA8`, `80370BB0`, `803725F4`. The hardware wrapper later calls
`80370CDC` and `80372838`, then returns to `8000315C`; those callees require
their own binary-first records. There is no indirect call or data-dependent
branch in the excavated instructions. `blr` depends on LR, so the saved-return
data flow is part of the proof, not an ABI assumption.

**Proven local required inputs:** the LR established by the linked calls;
initial MSR at `80003400`; initial HID2 at `80370BA8`; initial HID0 at
`803725F4`; writable stack at r1 before the `80371718` store. The paired
setup outputs MSR with bit `0x2000` set, issues a HID2 write with mask
`0xA0000000` set and a HID0 ICFI write with bit `0x800` set, zeros
GQR0–GQR7, and restores original SP/LR. Post-write HID0 state is a hardware
readback question; it is not the issued command word.
Exactly how those hardware-state changes affect later `psq_l`, FPR/FPSCR,
cache and memory requires deeper Gekko evidence and same-run observation.

**Independent decode comparison:** after the byte and bitfield pass, `q.py dis`
for `80003154`, `800032B0`, `80003400`, `80371714`, `80370BA8`,
`80370BB0`, and `803725F4` agreed on the instruction mnemonics and direct
targets. No binary/disassembler discrepancy was found in these ranges. The
q.py function range `80371714–80371768` is end-exclusive; its last word is
`80371764`. No decompiled C++, prior IR, or expected checkpoint value was
used to derive the instruction words.

**Validation stage:** byte-to-word and bitfield decode passes, local register
and ordered store effects pass. Native state equivalence for the HID/GQR
section and downstream consumer acceptance are **not established** by this
note. Confidence: **PROVEN** instruction identity/targets and local ISA
effects; **UNKNOWN** initial HID values, external hardware effects, and
retail IPL entry state.

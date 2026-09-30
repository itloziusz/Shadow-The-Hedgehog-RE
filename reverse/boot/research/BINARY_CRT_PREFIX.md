# PAL GUPP8P CRT prefix: binary-first record

**Fixture:** read-only PAL `main.dol`, SHA-256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
The DOL header maps `0x80003100..0x800055FF` to file offset
`0x000100..0x0025FF` (text 0). For every code and descriptor row below,
`file offset = VA - 0x80003100 + 0x100`. Each four-byte raw sequence is a
big-endian instruction word. The raw-word table was extracted from the DOL;
PowerPC bitfields and a second decoder were checked independently against
`q.py` only afterward. **PROVEN** here denotes immutable bytes and local ISA
effects, not a retail IPL or complete native state comparison.

## Control and state boundary

The entry call at `0x80003158` must first return from hardware setup to
`0x8000315C`. The earlier register helper supplies `r1=0x8060C5F0`,
`r2=0x805FA780`, and `r13=0x805EC500`, conditional on that callee preserving
them. An observed synthetic Dolphin/HLE launch has reached the return, but
the connected native implementation currently stops in the hardware callee.
This record therefore describes the **conditional next CRT path**; it does
not promote it to a connected native checkpoint.

### Entry call setup: one word per instruction

| VA | File offset | Raw bytes / BE word | Encoding fields | Independent decode |
|---|---:|---|---|---|
| `8000315C` | `00015C` | `38 00 FF FF` / `3800FFFF` | D op14 RS/D0 RA0 imm FFFF | `li r0, -1` |
| `80003160` | `000160` | `94 21 FF F8` / `9421FFF8` | D op37 RS/D1 RA1 imm FFF8 | `stwu r1, -8(r1)` |
| `80003164` | `000164` | `90 01 00 04` / `90010004` | D op36 RS/D0 RA1 imm 0004 | `stw r0, 4(r1)` |
| `80003168` | `000168` | `90 01 00 00` / `90010000` | D op36 RS/D0 RA1 imm 0000 | `stw r0, 0(r1)` |
| `8000316C` | `00016C` | `48 00 01 D5` / `480001D5` | I op18 disp +0x1d4 AA0 LK1 target 80003340 → `bl 80003340` | `bl 0x80003340` |

`stwu` stores the old `r1` at the decremented stack address and then changes
`r1`. Thus `[0x8060C5E8]` first receives `0x8060C5F0`; `0x80003168`
overwrites that same word with `0xFFFFFFFF`. `[0x8060C5EC]` is initially
`0xFFFFFFFF`, then the callee at `0x80003344` overwrites it with return LR
`0x80003170`. Both overwrite orders are observable guest-memory effects.

### Copy/zero walker: one word per instruction

| VA | File offset | Raw bytes / BE word | Encoding fields | Independent decode |
|---|---:|---|---|---|
| `80003340` | `000340` | `7C 08 02 A6` / `7C0802A6` | X op31 RS/D0 RA8 RB0 XO339 Rc0 | `mflr r0` |
| `80003344` | `000344` | `90 01 00 04` / `90010004` | D op36 RS/D0 RA1 imm 0004 | `stw r0, 4(r1)` |
| `80003348` | `000348` | `94 21 FF E8` / `9421FFE8` | D op37 RS/D1 RA1 imm FFE8 | `stwu r1, -0x18(r1)` |
| `8000334C` | `00034C` | `93 E1 00 14` / `93E10014` | D op36 RS/D31 RA1 imm 0014 | `stw r31, 0x14(r1)` |
| `80003350` | `000350` | `93 C1 00 10` / `93C10010` | D op36 RS/D30 RA1 imm 0010 | `stw r30, 0x10(r1)` |
| `80003354` | `000354` | `93 A1 00 0C` / `93A1000C` | D op36 RS/D29 RA1 imm 000C | `stw r29, 0xc(r1)` |
| `80003358` | `000358` | `3C 60 80 00` / `3C608000` | D op15 RS/D3 RA0 imm 8000 | `lis r3, -0x8000` |
| `8000335C` | `00035C` | `38 03 55 44` / `38035544` | D op14 RS/D0 RA3 imm 5544 | `addi r0, r3, 0x5544` |
| `80003360` | `000360` | `7C 1D 03 78` / `7C1D0378` | X op31 RS/D0 RA29 RB0 XO444 Rc0 | `mr r29, r0` |
| `80003364` | `000364` | `48 00 00 04` / `48000004` | I op18 disp +0x4 AA0 LK0 target 80003368 → `b 80003368` | `b 0x80003368` |
| `80003368` | `000368` | `48 00 00 04` / `48000004` | I op18 disp +0x4 AA0 LK0 target 8000336C → `b 8000336C` | `b 0x8000336c` |
| `8000336C` | `00036C` | `83 DD 00 08` / `83DD0008` | D op32 RS/D30 RA29 imm 0008 | `lwz r30, 8(r29)` |
| `80003370` | `000370` | `28 1E 00 00` / `281E0000` | D op10 RS/D0 RA30 imm 0000 | `cmplwi r30, 0` |
| `80003374` | `000374` | `41 82 00 38` / `41820038` | B op16 BO12 BI2 BD +0x38 AA0 LK0 target 800033AC → `beq 800033AC` | `beq 0x800033ac` |
| `80003378` | `000378` | `80 9D 00 00` / `809D0000` | D op32 RS/D4 RA29 imm 0000 | `lwz r4, 0(r29)` |
| `8000337C` | `00037C` | `83 FD 00 04` / `83FD0004` | D op32 RS/D31 RA29 imm 0004 | `lwz r31, 4(r29)` |
| `80003380` | `000380` | `41 82 00 24` / `41820024` | B op16 BO12 BI2 BD +0x24 AA0 LK0 target 800033A4 → `beq 800033A4` | `beq 0x800033a4` |
| `80003384` | `000384` | `7C 1F 20 40` / `7C1F2040` | X op31 RS/D0 RA31 RB4 XO32 Rc0 | `cmplw r31, r4` |
| `80003388` | `000388` | `41 82 00 1C` / `4182001C` | B op16 BO12 BI2 BD +0x1c AA0 LK0 target 800033A4 → `beq 800033A4` | `beq 0x800033a4` |
| `8000338C` | `00038C` | `7F E3 FB 78` / `7FE3FB78` | X op31 RS/D31 RA3 RB31 XO444 Rc0 | `mr r3, r31` |
| `80003390` | `000390` | `7F C5 F3 78` / `7FC5F378` | X op31 RS/D30 RA5 RB30 XO444 Rc0 | `mr r5, r30` |
| `80003394` | `000394` | `48 00 21 61` / `48002161` | I op18 disp +0x2160 AA0 LK1 target 800054F4 → `bl 800054F4` | `bl 0x800054f4` |
| `80003398` | `000398` | `7F E3 FB 78` / `7FE3FB78` | X op31 RS/D31 RA3 RB31 XO444 Rc0 | `mr r3, r31` |
| `8000339C` | `00039C` | `7F C4 F3 78` / `7FC4F378` | X op31 RS/D30 RA4 RB30 XO444 Rc0 | `mr r4, r30` |
| `800033A0` | `0003A0` | `48 00 00 85` / `48000085` | I op18 disp +0x84 AA0 LK1 target 80003424 → `bl 80003424` | `bl 0x80003424` |
| `800033A4` | `0003A4` | `3B BD 00 0C` / `3BBD000C` | D op14 RS/D29 RA29 imm 000C | `addi r29, r29, 0xc` |
| `800033A8` | `0003A8` | `4B FF FF C4` / `4BFFFFC4` | I op18 disp -0x3c AA0 LK0 target 8000336C → `b 8000336C` | `b 0x8000336c` |
| `800033AC` | `0003AC` | `3C 60 80 00` / `3C608000` | D op15 RS/D3 RA0 imm 8000 | `lis r3, -0x8000` |
| `800033B0` | `0003B0` | `38 03 55 C8` / `380355C8` | D op14 RS/D0 RA3 imm 55C8 | `addi r0, r3, 0x55c8` |
| `800033B4` | `0003B4` | `7C 1D 03 78` / `7C1D0378` | X op31 RS/D0 RA29 RB0 XO444 Rc0 | `mr r29, r0` |
| `800033B8` | `0003B8` | `48 00 00 04` / `48000004` | I op18 disp +0x4 AA0 LK0 target 800033BC → `b 800033BC` | `b 0x800033bc` |
| `800033BC` | `0003BC` | `48 00 00 04` / `48000004` | I op18 disp +0x4 AA0 LK0 target 800033C0 → `b 800033C0` | `b 0x800033c0` |
| `800033C0` | `0003C0` | `80 BD 00 04` / `80BD0004` | D op32 RS/D5 RA29 imm 0004 | `lwz r5, 4(r29)` |
| `800033C4` | `0003C4` | `28 05 00 00` / `28050000` | D op10 RS/D0 RA5 imm 0000 | `cmplwi r5, 0` |
| `800033C8` | `0003C8` | `41 82 00 1C` / `4182001C` | B op16 BO12 BI2 BD +0x1c AA0 LK0 target 800033E4 → `beq 800033E4` | `beq 0x800033e4` |
| `800033CC` | `0003CC` | `80 7D 00 00` / `807D0000` | D op32 RS/D3 RA29 imm 0000 | `lwz r3, 0(r29)` |
| `800033D0` | `0003D0` | `41 82 00 0C` / `4182000C` | B op16 BO12 BI2 BD +0xc AA0 LK0 target 800033DC → `beq 800033DC` | `beq 0x800033dc` |
| `800033D4` | `0003D4` | `38 80 00 00` / `38800000` | D op14 RS/D4 RA0 imm 0000 | `li r4, 0` |
| `800033D8` | `0003D8` | `48 00 20 35` / `48002035` | I op18 disp +0x2034 AA0 LK1 target 8000540C → `bl 8000540C` | `bl 0x8000540c` |
| `800033DC` | `0003DC` | `3B BD 00 08` / `3BBD0008` | D op14 RS/D29 RA29 imm 0008 | `addi r29, r29, 8` |
| `800033E0` | `0003E0` | `4B FF FF E0` / `4BFFFFE0` | I op18 disp -0x20 AA0 LK0 target 800033C0 → `b 800033C0` | `b 0x800033c0` |
| `800033E4` | `0003E4` | `80 01 00 1C` / `8001001C` | D op32 RS/D0 RA1 imm 001C | `lwz r0, 0x1c(r1)` |
| `800033E8` | `0003E8` | `83 E1 00 14` / `83E10014` | D op32 RS/D31 RA1 imm 0014 | `lwz r31, 0x14(r1)` |
| `800033EC` | `0003EC` | `83 C1 00 10` / `83C10010` | D op32 RS/D30 RA1 imm 0010 | `lwz r30, 0x10(r1)` |
| `800033F0` | `0003F0` | `83 A1 00 0C` / `83A1000C` | D op32 RS/D29 RA1 imm 000C | `lwz r29, 0xc(r1)` |
| `800033F4` | `0003F4` | `38 21 00 18` / `38210018` | D op14 RS/D1 RA1 imm 0018 | `addi r1, r1, 0x18` |
| `800033F8` | `0003F8` | `7C 08 03 A6` / `7C0803A6` | X op31 RS/D0 RA8 RB0 XO467 Rc0 | `mtlr r0` |
| `800033FC` | `0003FC` | `4E 80 00 20` / `4E800020` | XL op19 BO20 BI0 XO16 LK0 | `blr` |

The `0x80003370` unsigned zero-size comparison is the only copy-table
terminator test. Loads at `0x80003378/7C` do not change CR0, so the repeated
`beq` at `0x80003380` reuses the size comparison and is false for each
nonzero entry. The later `cmplw` compares full 32-bit destination and source
addresses; equal addresses skip both `memmove` and `0x80003424` cache work.
The skipped direct `bl` at `0x80003394` encodes target `0x800054F4`
(`memmove`), and the skipped direct `bl` at `0x800033A0` encodes target
`0x80003424`. Their LR and cache/memory effects are conditional and are not
claimed as observed on this image path.
The cursor advances by exactly 12 bytes. The zero walker likewise tests
`[cursor+4]` only; `0x800033D0` repeats its size comparison after a CR-neutral
destination load. Its cursor advances by eight bytes. All walker calls are
direct. No SDA/SDA2, FPR, FPSCR, paired-single, vtable, or indirect call is
used by this walker. The integer comparisons update CR0; `r2/r13` carry
through as bases for later code.

The walker stack frame begins at `0x8060C5D0`, with back-chain word at
`+0x00`, saved `r29/r30/r31` at `+0x0C/+0x10/+0x14`, and caller-saved LR
at `+0x1C = 0x8060C5EC`. The epilogue reads that word, restores the three
nonvolatile registers, returns `r1` to `0x8060C5E8`, and branches to
`0x80003170`. It does **not** restore the `-1` at `0x8060C5EC`.
The wrapper's three calls each write `0x800033DC` to its saved LR slot
`0x8060C5D4` and save the final copy descriptor's destination
`r31=0x805F2780` at `0x8060C5CC`; these stack writes remain after frame
pop. The prior walker saves at `0x8060C5DC/E0/E4` remain until overwritten
by a later stack user.

## Immutable descriptor bytes and reachability

The copy table starts at `0x80005544`. Each row is twelve bytes
`{source,destination,size}`; the zero table starts at `0x800055C8` and each
row is eight bytes `{destination,size}`. The table words are data, **not
instructions**.

| VA | File offset | Raw bytes | BE word | Descriptor role |
|---|---:|---|---:|---|
| `80005544` | `002544` | `80 00 31 00` | `80003100` | copy[0].source |
| `80005548` | `002548` | `80 00 31 00` | `80003100` | copy[0].destination |
| `8000554C` | `00254C` | `00 00 24 E8` | `000024E8` | copy[0].size |
| `80005550` | `002550` | `80 00 56 00` | `80005600` | copy[1].source |
| `80005554` | `002554` | `80 00 56 00` | `80005600` | copy[1].destination |
| `80005558` | `002558` | `00 00 1F 08` | `00001F08` | copy[1].size |
| `8000555C` | `00255C` | `80 00 75 20` | `80007520` | copy[2].source |
| `80005560` | `002560` | `80 00 75 20` | `80007520` | copy[2].destination |
| `80005564` | `002564` | `00 00 18 14` | `00001814` | copy[2].size |
| `80005568` | `002568` | `80 00 8D 40` | `80008D40` | copy[3].source |
| `8000556C` | `00256C` | `80 00 8D 40` | `80008D40` | copy[3].destination |
| `80005570` | `002570` | `00 4A 1F 08` | `004A1F08` | copy[3].size |
| `80005574` | `002574` | `80 4A AC 60` | `804AAC60` | copy[4].source |
| `80005578` | `002578` | `80 4A AC 60` | `804AAC60` | copy[4].destination |
| `8000557C` | `00257C` | `00 00 04 6C` | `0000046C` | copy[4].size |
| `80005580` | `002580` | `80 4A B0 E0` | `804AB0E0` | copy[5].source |
| `80005584` | `002584` | `80 4A B0 E0` | `804AB0E0` | copy[5].destination |
| `80005588` | `002588` | `00 00 00 0C` | `0000000C` | copy[5].size |
| `8000558C` | `00258C` | `80 4A B1 00` | `804AB100` | copy[6].source |
| `80005590` | `002590` | `80 4A B1 00` | `804AB100` | copy[6].destination |
| `80005594` | `002594` | `00 07 24 18` | `00072418` | copy[6].size |
| `80005598` | `002598` | `80 51 D5 20` | `8051D520` | copy[7].source |
| `8000559C` | `00259C` | `80 51 D5 20` | `8051D520` | copy[7].destination |
| `800055A0` | `0025A0` | `00 05 28 C8` | `000528C8` | copy[7].size |
| `800055A4` | `0025A4` | `80 5E 45 00` | `805E4500` | copy[8].source |
| `800055A8` | `0025A8` | `80 5E 45 00` | `805E4500` | copy[8].destination |
| `800055AC` | `0025AC` | `00 00 AB 20` | `0000AB20` | copy[8].size |
| `800055B0` | `0025B0` | `80 5F 27 80` | `805F2780` | copy[9].source |
| `800055B4` | `0025B4` | `80 5F 27 80` | `805F2780` | copy[9].destination |
| `800055B8` | `0025B8` | `00 00 9D B8` | `00009DB8` | copy[9].size |
| `800055BC` | `0025BC` | `00 00 00 00` | `00000000` | copy[10].source |
| `800055C0` | `0025C0` | `00 00 00 00` | `00000000` | copy[10].destination |
| `800055C4` | `0025C4` | `00 00 00 00` | `00000000` | copy[10].size |
| `800055C8` | `0025C8` | `80 56 FE 00` | `8056FE00` | zero[0].destination |
| `800055CC` | `0025CC` | `00 07 47 00` | `00074700` | zero[0].size |
| `800055D0` | `0025D0` | `80 5E F0 20` | `805EF020` | zero[1].destination |
| `800055D4` | `0025D4` | `00 00 37 5C` | `0000375C` | zero[1].size |
| `800055D8` | `0025D8` | `80 5F C5 40` | `805FC540` | zero[2].destination |
| `800055DC` | `0025DC` | `00 00 00 AC` | `000000AC` | zero[2].size |
| `800055E0` | `0025E0` | `00 00 00 00` | `00000000` | zero[3].destination |
| `800055E4` | `0025E4` | `00 00 00 00` | `00000000` | zero[3].size |

The ten copy entries all have nonzero size and equal source and destination.
Therefore their **PAL image path** makes no `memmove` or cache-maintenance
call. The size-zero triple at `0x800055BC` stops iteration; its two address
words are not read as a condition. A modified image with unequal addresses
must take both calls, whose deeper effects remain separately unresolved.
The three nonzero zero entries have the following exclusive ends:

| Destination | Size | End | Full 32-byte groups | Extra 4-byte stores |
|---:|---:|---:|---:|---:|
| `8056FE00` | `00074700` | `805E4500` | `0x3A38` | 0 |
| `805EF020` | `0000375C` | `805F277C` | `0x01BA` | 7 |
| `805FC540` | `000000AC` | `805FC5EC` | `0x0005` | 3 |

Only the size at `0x800055E4` terminates the zero loop. In particular the
four bytes `0x805F277C..0x805F277F` are outside these three zero intervals.
The first clear overlaps the previously installed hardware handler at
`0x80586CB4`; the second clears `0x805F1F30..0x805F1F3F` **after** hardware
has loaded from them. These are ordered consequences of actual ranges, not
permission to skip the earlier handler/FPR operations.

## Reached `memset` call and fill leaf

The walker calls `0x8000540C` with `(r3,r4,r5)=(destination,0,size)`.
Its wrapper creates a 16-byte frame below walker `r1=0x8060C5D0`, so its
`r1=0x8060C5C0`; it stores return LR at `0x8060C5D4` and incoming `r31`
at `0x8060C5CC`. It saves original destination in `r31`, calls the leaf,
restores LR/r31/frame, and returns that destination in `r3`.

| VA | File offset | Raw bytes / BE word | Encoding fields | Independent decode |
|---|---:|---|---|---|
| `8000540C` | `00240C` | `94 21 FF F0` / `9421FFF0` | D op37 RS/D1 RA1 imm FFF0 | `stwu r1, -0x10(r1)` |
| `80005410` | `002410` | `7C 08 02 A6` / `7C0802A6` | X op31 RS/D0 RA8 RB0 XO339 Rc0 | `mflr r0` |
| `80005414` | `002414` | `90 01 00 14` / `90010014` | D op36 RS/D0 RA1 imm 0014 | `stw r0, 0x14(r1)` |
| `80005418` | `002418` | `93 E1 00 0C` / `93E1000C` | D op36 RS/D31 RA1 imm 000C | `stw r31, 0xc(r1)` |
| `8000541C` | `00241C` | `7C 7F 1B 78` / `7C7F1B78` | X op31 RS/D3 RA31 RB3 XO444 Rc0 | `mr r31, r3` |
| `80005420` | `002420` | `48 00 00 1D` / `4800001D` | I op18 disp +0x1c AA0 LK1 target 8000543C → `bl 8000543C` | `bl 0x8000543c` |
| `80005424` | `002424` | `80 01 00 14` / `80010014` | D op32 RS/D0 RA1 imm 0014 | `lwz r0, 0x14(r1)` |
| `80005428` | `002428` | `7F E3 FB 78` / `7FE3FB78` | X op31 RS/D31 RA3 RB31 XO444 Rc0 | `mr r3, r31` |
| `8000542C` | `00242C` | `83 E1 00 0C` / `83E1000C` | D op32 RS/D31 RA1 imm 000C | `lwz r31, 0xc(r1)` |
| `80005430` | `002430` | `7C 08 03 A6` / `7C0803A6` | X op31 RS/D0 RA8 RB0 XO467 Rc0 | `mtlr r0` |
| `80005434` | `002434` | `38 21 00 10` / `38210010` | D op14 RS/D1 RA1 imm 0010 | `addi r1, r1, 0x10` |
| `80005438` | `002438` | `4E 80 00 20` / `4E800020` | XL op19 BO20 BI0 XO16 LK0 | `blr` |
| `8000543C` | `00243C` | `28 05 00 20` / `28050020` | D op10 RS/D0 RA5 imm 0020 | `cmplwi r5, 0x20` |
| `80005440` | `002440` | `54 84 06 3E` / `5484063E` | M op21 RS4 RA4 SH0 MB24 ME31 Rc0 | `clrlwi r4, r4, 0x18` |
| `80005444` | `002444` | `38 C3 FF FF` / `38C3FFFF` | D op14 RS/D6 RA3 imm FFFF | `addi r6, r3, -1` |
| `80005448` | `002448` | `7C 87 23 78` / `7C872378` | X op31 RS/D4 RA7 RB4 XO444 Rc0 | `mr r7, r4` |
| `8000544C` | `00244C` | `41 80 00 90` / `41800090` | B op16 BO12 BI0 BD +0x90 AA0 LK0 target 800054DC → `blt 800054DC` | `blt 0x800054dc` |
| `80005450` | `002450` | `7C C0 30 F8` / `7CC030F8` | X op31 RS/D6 RA0 RB6 XO124 Rc0 | `nor r0, r6, r6` |
| `80005454` | `002454` | `54 03 07 BF` / `540307BF` | M op21 RS0 RA3 SH0 MB30 ME31 Rc1 | `clrlwi. r3, r0, 0x1e` |
| `80005458` | `002458` | `41 82 00 14` / `41820014` | B op16 BO12 BI2 BD +0x14 AA0 LK0 target 8000546C → `beq 8000546C` | `beq 0x8000546c` |
| `8000545C` | `00245C` | `7C A3 28 50` / `7CA32850` | X op31 RS/D5 RA3 RB5 XO40 Rc0 | `subf r5, r3, r5` |
| `80005460` | `002460` | `34 63 FF FF` / `3463FFFF` | D op13 RS/D3 RA3 imm FFFF | `addic. r3, r3, -1` |
| `80005464` | `002464` | `9C E6 00 01` / `9CE60001` | D op39 RS/D7 RA6 imm 0001 | `stbu r7, 1(r6)` |
| `80005468` | `002468` | `40 82 FF F8` / `4082FFF8` | B op16 BO4 BI2 BD -0x8 AA0 LK0 target 80005460 → `bne 80005460` | `bne 0x80005460` |
| `8000546C` | `00246C` | `28 07 00 00` / `28070000` | D op10 RS/D0 RA7 imm 0000 | `cmplwi r7, 0` |
| `80005470` | `002470` | `41 82 00 1C` / `4182001C` | B op16 BO12 BI2 BD +0x1c AA0 LK0 target 8000548C → `beq 8000548C` | `beq 0x8000548c` |
| `80005474` | `002474` | `54 E3 C0 0E` / `54E3C00E` | M op21 RS7 RA3 SH24 MB0 ME7 Rc0 | `slwi r3, r7, 0x18` |
| `80005478` | `002478` | `54 E0 80 1E` / `54E0801E` | M op21 RS7 RA0 SH16 MB0 ME15 Rc0 | `slwi r0, r7, 0x10` |
| `8000547C` | `00247C` | `54 E4 40 2E` / `54E4402E` | M op21 RS7 RA4 SH8 MB0 ME23 Rc0 | `slwi r4, r7, 8` |
| `80005480` | `002480` | `7C 60 03 78` / `7C600378` | X op31 RS/D3 RA0 RB0 XO444 Rc0 | `or r0, r3, r0` |
| `80005484` | `002484` | `7C 80 03 78` / `7C800378` | X op31 RS/D4 RA0 RB0 XO444 Rc0 | `or r0, r4, r0` |
| `80005488` | `002488` | `7C E7 03 78` / `7CE70378` | X op31 RS/D7 RA7 RB0 XO444 Rc0 | `or r7, r7, r0` |
| `8000548C` | `00248C` | `54 A3 D9 7F` / `54A3D97F` | M op21 RS5 RA3 SH27 MB5 ME31 Rc1 | `rlwinm. r3, r5, 0x1b, 5, 0x1f` |
| `80005490` | `002490` | `38 86 FF FD` / `3886FFFD` | D op14 RS/D4 RA6 imm FFFD | `addi r4, r6, -3` |
| `80005494` | `002494` | `41 82 00 2C` / `4182002C` | B op16 BO12 BI2 BD +0x2c AA0 LK0 target 800054C0 → `beq 800054C0` | `beq 0x800054c0` |
| `80005498` | `002498` | `90 E4 00 04` / `90E40004` | D op36 RS/D7 RA4 imm 0004 | `stw r7, 4(r4)` |
| `8000549C` | `00249C` | `34 63 FF FF` / `3463FFFF` | D op13 RS/D3 RA3 imm FFFF | `addic. r3, r3, -1` |
| `800054A0` | `0024A0` | `90 E4 00 08` / `90E40008` | D op36 RS/D7 RA4 imm 0008 | `stw r7, 8(r4)` |
| `800054A4` | `0024A4` | `90 E4 00 0C` / `90E4000C` | D op36 RS/D7 RA4 imm 000C | `stw r7, 0xc(r4)` |
| `800054A8` | `0024A8` | `90 E4 00 10` / `90E40010` | D op36 RS/D7 RA4 imm 0010 | `stw r7, 0x10(r4)` |
| `800054AC` | `0024AC` | `90 E4 00 14` / `90E40014` | D op36 RS/D7 RA4 imm 0014 | `stw r7, 0x14(r4)` |
| `800054B0` | `0024B0` | `90 E4 00 18` / `90E40018` | D op36 RS/D7 RA4 imm 0018 | `stw r7, 0x18(r4)` |
| `800054B4` | `0024B4` | `90 E4 00 1C` / `90E4001C` | D op36 RS/D7 RA4 imm 001C | `stw r7, 0x1c(r4)` |
| `800054B8` | `0024B8` | `94 E4 00 20` / `94E40020` | D op37 RS/D7 RA4 imm 0020 | `stwu r7, 0x20(r4)` |
| `800054BC` | `0024BC` | `40 82 FF DC` / `4082FFDC` | B op16 BO4 BI2 BD -0x24 AA0 LK0 target 80005498 → `bne 80005498` | `bne 0x80005498` |
| `800054C0` | `0024C0` | `54 A3 F7 7F` / `54A3F77F` | M op21 RS5 RA3 SH30 MB29 ME31 Rc1 | `rlwinm. r3, r5, 0x1e, 0x1d, 0x1f` |
| `800054C4` | `0024C4` | `41 82 00 10` / `41820010` | B op16 BO12 BI2 BD +0x10 AA0 LK0 target 800054D4 → `beq 800054D4` | `beq 0x800054d4` |
| `800054C8` | `0024C8` | `34 63 FF FF` / `3463FFFF` | D op13 RS/D3 RA3 imm FFFF | `addic. r3, r3, -1` |
| `800054CC` | `0024CC` | `94 E4 00 04` / `94E40004` | D op37 RS/D7 RA4 imm 0004 | `stwu r7, 4(r4)` |
| `800054D0` | `0024D0` | `40 82 FF F8` / `4082FFF8` | B op16 BO4 BI2 BD -0x8 AA0 LK0 target 800054C8 → `bne 800054C8` | `bne 0x800054c8` |
| `800054D4` | `0024D4` | `38 C4 00 03` / `38C40003` | D op14 RS/D6 RA4 imm 0003 | `addi r6, r4, 3` |
| `800054D8` | `0024D8` | `54 A5 07 BE` / `54A507BE` | M op21 RS5 RA5 SH0 MB30 ME31 Rc0 | `clrlwi r5, r5, 0x1e` |
| `800054DC` | `0024DC` | `28 05 00 00` / `28050000` | D op10 RS/D0 RA5 imm 0000 | `cmplwi r5, 0` |
| `800054E0` | `0024E0` | `4D 82 00 20` / `4D820020` | XL op19 BO12 BI2 XO16 LK0 | `beqlr` |
| `800054E4` | `0024E4` | `34 A5 FF FF` / `34A5FFFF` | D op13 RS/D5 RA5 imm FFFF | `addic. r5, r5, -1` |
| `800054E8` | `0024E8` | `9C E6 00 01` / `9CE60001` | D op39 RS/D7 RA6 imm 0001 | `stbu r7, 1(r6)` |
| `800054EC` | `0024EC` | `40 82 FF F8` / `4082FFF8` | B op16 BO4 BI2 BD -0x8 AA0 LK0 target 800054E4 → `bne 800054E4` | `bne 0x800054e4` |
| `800054F0` | `0024F0` | `4E 80 00 20` / `4E800020` | XL op19 BO20 BI0 XO16 LK0 | `blr` |

For all three actual entries, size is at least 32, destination is four-byte
aligned, and byte value is zero. At `0x80005444`, `r6=dest-1`; at
`0x80005450`, `r0=~(dest-1)`. The low two bits are zero for these
destinations, so `0x80005458` skips alignment byte stores. The zero-byte
comparison at `0x8000546C` skips the nonzero-byte replication. At
`0x8000548C`, `r3=size>>5`, and `r4=dest-4`; each eight-store group writes
`dest+32i..dest+32i+31` as big-endian zero words and advances `r4` by 32.
At `0x800054C0`, `r3=(size>>2)&7`; each extra word store advances `r4` by
four. Then `r6=dest+size-1`, `r5=size&3=0`; `cmplwi` sets CR0.EQ and
`beqlr` returns to the wrapper. No leading or trailing byte store executes
for these descriptors. The full `0x8000543C..0x800054F0` code includes
alignment, nonzero-fill, short-size and tail paths, but those edges are
**not reached** for the checked table values. The fill changes integer
registers, CR0 and XER carry via `addic.`; it does not access SDA/SDA2,
FPR/FPSCR, paired singles, MMIO or indirect targets.

Assuming the hardware callee returned with the earlier helper's cleared
general registers intact, the walker return at `0x80003170` has
`r0=LR=0x80003170`, `r1=0x8060C5E8`, `r3=0x805FC540` (last `memset`
return), `r4=r5=r7=0`, `r6=0x805FC5EB` (last destination plus size minus
one), and restored `r29=r30=r31=0`. The zero-table terminating comparison
leaves CR0 equal; the final `addic.` countdown in each reached fill makes
XER.CA=1. XER.SO is not overwritten here, so CR0.SO reflects its incoming
value. CTR is not written in this path. These are instruction deductions,
**not yet same-run/native boundary comparisons**.

## Data flow, CFG, portable projection, and limits

```text
8000315C..316C -> 80003340..3374
  size!=0 -> 3378..3388 -> source==dest -> 33A4 -> 336C (10 entries)
  size==0 -> 33AC..33C8
  size!=0 -> 33CC..33D8 -> 8000540C -> 8000543C -> wrapper -> 33DC (3 entries)
  size==0 -> 33E4..33FC -> 80003170
```

The actual copy path's `memmove`/cache edge is excluded by verified bytes,
not by a generic CRT assumption. For the checked image, a portable semantic
projection is a checked big-endian descriptor walker that performs the three
ordered zero intervals and preserves the stack/LR consequences needed by
later code. A different descriptor requires `memmove` plus proven cache
effects. The exact raw instruction effects are retained above until later
consumers prove which stack/CR/XER effects can safely collapse in native C++.
Current native implementation has **not** crossed the hardware prerequisite;
this projection is **not implemented or runtime-validated**.

**Validation:** source SHA-256 and every displayed code/data word can be
rechecked by `reverse/boot/tools/verify_binary_note.py` for instruction rows;
it passed with **111** expected instruction rows, all **25** I/B-form branch
targets, zero SPR rows, and **41** descriptor word rows. The descriptor
words were also independently reread for their count,
terminators, equal copy addresses, zero-range ends, and fill alignment
checked directly from the binary. The two independent decoders agreed on
all 111 instruction meanings, branch targets, and call sites. Capstone PPC32
big-endian and `q.py` spell two identical
encodings with different aliases: at `0x80005450`, Capstone `nor r0,r6,r6`
equals `q.py` `not r0,r6`; at `0x8000548C`, Capstone
`rlwinm. r3,r5,27,5,31` equals `q.py` `srwi. r3,r5,5`. Neither is an
instruction disagreement. The bitfield extraction in the table is separate
from both disassemblers: D/X/XL/M operands come from the documented word
positions; I-form displacement is sign-extended from 26 bits and B-form
displacement from 16 bits, both added to the current VA because `AA=0`.
Descriptor arithmetic was
recomputed separately from source bytes. **PROVEN:** bytes, local CFG,
addresses, table path, three zero intervals, local stack/CR effects.
**UNKNOWN:** full incoming post-hardware state, same-run memory snapshot
after CRT, any modified descriptor path, downstream acceptance, and retail
IPL equivalence.

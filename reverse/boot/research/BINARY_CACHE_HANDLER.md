# PAL GUPP8P cache and handler path, from DOL bytes

**Scope.** The third call of the entry hardware wrapper, `0x80003418 ->
0x80372838`, including the L2 helper entered on the captured synthetic/HLE
path, the handler installer, the directly called HID0/L2CR/MSR accessors, and
the optional L1 helpers. The authority is the read-only PAL `sys/main.dol`
(SHA-256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`).
The startup-only Dolphin/HLE capture is a conditional **execution oracle**;
its state is not asserted to be the retail IPL handoff. No native cache or
handler section has yet passed connected state parity, and none is translated
here.

All listed VAs lie in DOL text section 1: file offset `0x002600`, load VA
`0x80008D40`, size `0x4A1F20`. Therefore a row's file offset is
`0x2600 + VA - 0x80008D40`. I read each four-byte span directly from the
SHA-pinned binary as a big-endian word. The tables below independently decode
opcode fields and PPC branch displacements, then show the Capstone PPC decode.
Only afterward were the rows and targets compared with read-only `q.py dis`.
In field descriptions `rd/rs` is bits 6–10, `ra` bits 11–15, and `rb` bits
16–20 in PPC numbering; `XO` is the ten-bit X-form extension. For SPR forms,
the two five-bit fields are interchanged to form the SPR number.

## Connected control-flow graph and state dependencies

At entry `r1=S=0x8060C5F0` on the **observed synthetic run** and LR is
`0x8000341C`. The main routine writes LR at `S+4`, then `stwu` writes the old
SP at `S-16`; saves r31/r30 at `S-4`/`S-8`. It sets r31 to the string base
`0x80561380` and restores r31/r30 at return. The LR save and stack backchain
are independent memory writes. No SDA/SDA2 addressing occurs in the selected
cache and selector-1 installer path. The unselected selector-16 branch reads
`r13+0x26F0`; it must not be generalized away.

```text
C0 80372838..60: save frame; read HID0; test HID0 & 0x00008000
  nonzero -> C1 80372874
  zero    -> 80372864 call optional I-cache helper; logger; C1
C1 80372874..80: read HID0 anew; test HID0 & 0x00004000
  nonzero -> C2 80372894
  zero    -> 80372884 call optional D-cache helper; logger; C2
C2 80372894..A0: read L2CR; test L2CR & 0x80000000
  nonzero -> C3 803728F8
  zero    -> 803728A4..F4: save MSR in r30; write MSR=0x30;
             clear L2CR enable bit; call L2 helper; restore MSR;
             set L2CR enable bit and clear its invalidate bit; logger; C3
C3 803728F8..72928: r4=0x803726D8, r3=1; install handler;
                     logger; restore stack/LR; return 8000341C
L2 80372640..D4: clear L2 enable, set invalidate request;
                   poll status bit 0 until zero; clear request bit;
                   poll status bit 0 until zero, logging on repeat;
                   restore frame; return 803728D4
H  80373378..C4: save frame/args; disable interrupt bit 0x8000;
                   load old slot; store pointer to slot; test selector=0x10
  selector != 0x10 -> 80373564..8C: restore interrupt bit/frame;
                      return old handler to 80372908
  selector == 0x10 -> 803733C8..73560: separate vector/list path;
                      joins 80373564 (NOT reached by selector 1)
```

The main path's direct calls are the single-read/write HID0, L2CR, and MSR
wrappers in the last table. Their `blr` returns are controlled by the `bl` at
the call site. No indirect branch, virtual call, or constructor executes on
the observed selector-1 path. `0x803726D8` is an address **stored as data**;
its later invocation is outside this scope. Logger `0x80370C8C` creates a
`0x70`-byte temporary frame, then saves r3..r10. The caller's `crxor 6,6,6`
sets CR bit 6 to zero, so its `bne cr1` skips the optional FPR stores on this
path. The logger has no out-of-frame write or output device call; its stack
writes and CR change still count as state effects.

### Exact branch inputs and write/read chains

* **PROVEN:** `rlwinm r0,r3,0,16,16` extracts HID0 bit `0x8000`;
  `cmplwi` sets CR0; `bne` at `0x80372860` selects `0x80372874` if nonzero.
  The next independent HID0 read extracts bit `0x4000` and `bne` at
  `0x80372880` selects `0x80372894` if nonzero. The third independent read
  extracts L2CR bit `0x80000000` and `bne` at `0x803728A0` selects
  `0x803728F8` if nonzero. The first two zero branches invoke the five-word
  helpers tabulated below. No earlier HID0 ICFI command proves these bits.
* **OBSERVED, synthetic/HLE only:** HID0 readback is `0x0011C464`: both
  `0x8000` and `0x4000` tests are nonzero, so both L1 helper calls are skipped.
  L2CR reads zero at C2, so the L2 branch enters `0x803728A4`.
* **PROVEN:** the L2 branch saves the *current* MSR in r30, then writes `0x30`
  to MSR; after the nested call it writes saved r30 back. It writes L2CR with
  bit `0x80000000` cleared before nested invalidation. The L2 helper clears
  that bit again, ORs `0x00200000`, writes it, then reads status bit 0 until
  zero. At `0x8037268C`, the wraparound `rlwinm` mask (MB=11, ME=9) clears
  exactly PPC bit 10 (`0x00200000`) from the just-read L2CR and writes the
  resulting value. Its second status-bit-0 poll branches back to the logger
  at `0x803726A8` while nonzero. Poll termination is a hardware input, not a
  valid fixed native loop count.
* **OBSERVED, synthetic/HLE only:** first and second poll tests see status bit
  zero on their first visits. L2CR is `0x00200000` after the guest request
  write and zero after the guest's explicit clear. This explains the second
  zero without assuming hardware auto-clear. Returning to C2, the guest writes
  `0x80000000` (enable, invalidate bit cleared). Cache contents, timing,
  exception state and any hidden hardware side effects are **UNKNOWN**.
* **PROVEN:** selector 1 and handler value `0x803726D8` are immediate-derived
  in C3. In H, `rlwinm r5,r29,2,14,29` yields 4 for selector 1. The DOL
  constructs base `0x80586CB0` via `lis/addi`, so the `lwz` old-handler and
  `stw` new-handler address is `0x80586CB4`. The installer calls
  `0x8037611C`, which clears MSR interrupt bit `0x8000` and returns its
  previous one-bit value; `0x80376144` restores that bit according to this
  value. Its `cmplwi` compares zero-extended selector low 16 bits to `0x10`.
  Selector 1 takes the `bne` to `0x80373564` and does not execute the long
  vector/list path. **OBSERVED:** slot old value zero and new value
  `0x803726D8`; MSR remains `0x2032` at hardware return.

The selector-16 body is decoded below because it is a direct alternative edge,
but its pointer-producing `lwz 0xDC(r5)` from low memory, linked-list pointer
at `r6+0x2FC`, SDA word `r13+0x26F0`, and calls to `0x80370B60/0x80370B80`
are not resolved for this unselected mode. No semantic or native parity claim
is made for that body. If a future caller supplies selector 16, semantic
translation must stop at its first unknown address and trace those producers.

## Portable semantic projection and validation boundary

The selected path can eventually become a native platform contract with
explicit HID0/L2CR/MSR reads, cache operation consequences, status polling,
and a handler registry store, with r0/r3/r4/r5/r6/r30/r31/CR/LR and ordered
stack effects retained wherever downstream PPC-translated code observes them.
The present C++ equivalent is **UNKNOWN** because cache and handler consumers,
asynchronous effects and the complete native post-state have not been
validated. It would be false parity to replace either poll with a fixed zero
or set the handler slot solely to pass a checkpoint.

The five table rows shown as Capstone `crclr 6` are the `crxor 6,6,6`
encoding shown by `q.py`. Both express the same instruction bits and exact
CR-bit-6 result; this is an alias, not a binary disagreement. All 305 table
VAs appeared in `q.py`, and these were the only five mnemonic spellings that
differed.

Validation ladder: DOL hash and each word checked; primary fields, XO/SPR and
branch target decoded independently; Capstone and `q.py dis` agree on the
listed instructions and CFG; selected HID0/L2CR branches, MSR interval and
handler slot were observed in two cold-start synthetic/HLE runs. Full cache
state, retail IPL input, exceptional interrupt timing, later handler use, and
connected native state/consumer parity remain **UNKNOWN**. The direct native
probe must still rerun from `0x80003154` after any future translation.
The repository's `verify_binary_note.py` validates 305 raw rows, all 56
direct branch targets, and 14 SPR encodings. This includes the CR1 `bne`
at `0x80370C90` (BO4/BI6, target `0x80370CB4`) and the `bdnz` at
`0x803734AC` (count-register loop, target `0x80373424`). Their BD fields
and targets were also decoded independently and checked against Capstone
and `q.py`.

## Per-word binary records

In the tables, `op` is the six-bit primary opcode. `I disp` is a signed
26-bit byte displacement with AA/LK, and `B disp` is signed 16-bit BD with
BO/BI/AA/LK; target = this VA + displacement when AA=0. `D` rows expose
register and immediate fields; `X` exposes XO and registers; `M` exposes
rotate/mask fields. The decoded instruction is independent Capstone output.
The fields and raw word are sufficient to repeat the decode without trusting
either disassembler.

### Main cache routine, all words

| VA | File offset | Raw bytes / BE word | Encoding fields | Independent decode |
|---|---:|---|---|---|
| `80372838` | `36C0F8` | `7C 08 02 A6` / `7C0802A6` | X op31 XO339 d/s0 ra8 rb0 Rc0 SPR8 | `mflr r0` |
| `8037283C` | `36C0FC` | `90 01 00 04` / `90010004` | D op36 d/s0 ra1 imm0004 | `stw r0, 4(r1)` |
| `80372840` | `36C100` | `94 21 FF F0` / `9421FFF0` | D op37 d/s1 ra1 immFFF0 | `stwu r1, -0x10(r1)` |
| `80372844` | `36C104` | `93 E1 00 0C` / `93E1000C` | D op36 d/s31 ra1 imm000C | `stw r31, 0xc(r1)` |
| `80372848` | `36C108` | `93 C1 00 08` / `93C10008` | D op36 d/s30 ra1 imm0008 | `stw r30, 8(r1)` |
| `8037284C` | `36C10C` | `3C 60 80 56` / `3C608056` | D op15 d/s3 ra0 imm8056 | `lis r3, -0x7faa` |
| `80372850` | `36C110` | `3B E3 13 80` / `3BE31380` | D op14 d/s31 ra3 imm1380 | `addi r31, r3, 0x1380` |
| `80372854` | `36C114` | `4B FF E2 99` / `4BFFE299` | I op18 disp -0x1d68 AA0 LK1 -> 80370AEC → `bl 80370AEC` | `bl 0x80370aec` |
| `80372858` | `36C118` | `54 60 04 20` / `54600420` | M op21 rs3 ra0 sh0 mb16 me16 Rc0 | `rlwinm r0, r3, 0, 0x10, 0x10` |
| `8037285C` | `36C11C` | `28 00 00 00` / `28000000` | D op10 crf0 ra0 imm0000 | `cmplwi r0, 0` |
| `80372860` | `36C120` | `40 82 00 14` / `40820014` | B op16 BO4 BI2 disp +0x14 AA0 LK0 -> 80372874 → `bne 80372874` | `bne 0x80372874` |
| `80372864` | `36C124` | `4B FF FD A1` / `4BFFFDA1` | I op18 disp -0x260 AA0 LK1 -> 80372604 → `bl 80372604` | `bl 0x80372604` |
| `80372868` | `36C128` | `38 7F 01 AC` / `387F01AC` | D op14 d/s3 ra31 imm01AC | `addi r3, r31, 0x1ac` |
| `8037286C` | `36C12C` | `4C C6 31 82` / `4CC63182` | XL op19 XO193 BT/BO6 BA/BI6 BB6 LK0 | `crclr cr1eq` |
| `80372870` | `36C130` | `4B FF E4 1D` / `4BFFE41D` | I op18 disp -0x1be4 AA0 LK1 -> 80370C8C → `bl 80370C8C` | `bl 0x80370c8c` |
| `80372874` | `36C134` | `4B FF E2 79` / `4BFFE279` | I op18 disp -0x1d88 AA0 LK1 -> 80370AEC → `bl 80370AEC` | `bl 0x80370aec` |
| `80372878` | `36C138` | `54 60 04 62` / `54600462` | M op21 rs3 ra0 sh0 mb17 me17 Rc0 | `rlwinm r0, r3, 0, 0x11, 0x11` |
| `8037287C` | `36C13C` | `28 00 00 00` / `28000000` | D op10 crf0 ra0 imm0000 | `cmplwi r0, 0` |
| `80372880` | `36C140` | `40 82 00 14` / `40820014` | B op16 BO4 BI2 disp +0x14 AA0 LK0 -> 80372894 → `bne 80372894` | `bne 0x80372894` |
| `80372884` | `36C144` | `4B FF FC 71` / `4BFFFC71` | I op18 disp -0x390 AA0 LK1 -> 803724F4 → `bl 803724F4` | `bl 0x803724f4` |
| `80372888` | `36C148` | `38 7F 01 C8` / `387F01C8` | D op14 d/s3 ra31 imm01C8 | `addi r3, r31, 0x1c8` |
| `8037288C` | `36C14C` | `4C C6 31 82` / `4CC63182` | XL op19 XO193 BT/BO6 BA/BI6 BB6 LK0 | `crclr cr1eq` |
| `80372890` | `36C150` | `4B FF E3 FD` / `4BFFE3FD` | I op18 disp -0x1c04 AA0 LK1 -> 80370C8C → `bl 80370C8C` | `bl 0x80370c8c` |
| `80372894` | `36C154` | `4B FF E2 69` / `4BFFE269` | I op18 disp -0x1d98 AA0 LK1 -> 80370AFC → `bl 80370AFC` | `bl 0x80370afc` |
| `80372898` | `36C158` | `54 60 00 00` / `54600000` | M op21 rs3 ra0 sh0 mb0 me0 Rc0 | `rlwinm r0, r3, 0, 0, 0` |
| `8037289C` | `36C15C` | `28 00 00 00` / `28000000` | D op10 crf0 ra0 imm0000 | `cmplwi r0, 0` |
| `803728A0` | `36C160` | `40 82 00 58` / `40820058` | B op16 BO4 BI2 disp +0x58 AA0 LK0 -> 803728F8 → `bne 803728F8` | `bne 0x803728f8` |
| `803728A4` | `36C164` | `4B FF E2 39` / `4BFFE239` | I op18 disp -0x1dc8 AA0 LK1 -> 80370ADC → `bl 80370ADC` | `bl 0x80370adc` |
| `803728A8` | `36C168` | `7C 7E 1B 78` / `7C7E1B78` | X op31 XO444 d/s3 ra30 rb3 Rc0 | `mr r30, r3` |
| `803728AC` | `36C16C` | `7C 00 04 AC` / `7C0004AC` | X op31 XO598 d/s0 ra0 rb0 Rc0 | `sync` |
| `803728B0` | `36C170` | `38 60 00 30` / `38600030` | D op14 d/s3 ra0 imm0030 | `li r3, 0x30` |
| `803728B4` | `36C174` | `4B FF E2 31` / `4BFFE231` | I op18 disp -0x1dd0 AA0 LK1 -> 80370AE4 → `bl 80370AE4` | `bl 0x80370ae4` |
| `803728B8` | `36C178` | `7C 00 04 AC` / `7C0004AC` | X op31 XO598 d/s0 ra0 rb0 Rc0 | `sync` |
| `803728BC` | `36C17C` | `7C 00 04 AC` / `7C0004AC` | X op31 XO598 d/s0 ra0 rb0 Rc0 | `sync` |
| `803728C0` | `36C180` | `4B FF E2 3D` / `4BFFE23D` | I op18 disp -0x1dc4 AA0 LK1 -> 80370AFC → `bl 80370AFC` | `bl 0x80370afc` |
| `803728C4` | `36C184` | `54 63 00 7E` / `5463007E` | M op21 rs3 ra3 sh0 mb1 me31 Rc0 | `clrlwi r3, r3, 1` |
| `803728C8` | `36C188` | `4B FF E2 3D` / `4BFFE23D` | I op18 disp -0x1dc4 AA0 LK1 -> 80370B04 → `bl 80370B04` | `bl 0x80370b04` |
| `803728CC` | `36C18C` | `7C 00 04 AC` / `7C0004AC` | X op31 XO598 d/s0 ra0 rb0 Rc0 | `sync` |
| `803728D0` | `36C190` | `4B FF FD 71` / `4BFFFD71` | I op18 disp -0x290 AA0 LK1 -> 80372640 → `bl 80372640` | `bl 0x80372640` |
| `803728D4` | `36C194` | `7F C3 F3 78` / `7FC3F378` | X op31 XO444 d/s30 ra3 rb30 Rc0 | `mr r3, r30` |
| `803728D8` | `36C198` | `4B FF E2 0D` / `4BFFE20D` | I op18 disp -0x1df4 AA0 LK1 -> 80370AE4 → `bl 80370AE4` | `bl 0x80370ae4` |
| `803728DC` | `36C19C` | `4B FF E2 21` / `4BFFE221` | I op18 disp -0x1de0 AA0 LK1 -> 80370AFC → `bl 80370AFC` | `bl 0x80370afc` |
| `803728E0` | `36C1A0` | `64 60 80 00` / `64608000` | D op25 d/s3 ra0 imm8000 | `oris r0, r3, 0x8000` |
| `803728E4` | `36C1A4` | `54 03 02 D2` / `540302D2` | M op21 rs0 ra3 sh0 mb11 me9 Rc0 | `rlwinm r3, r0, 0, 0xb, 9` |
| `803728E8` | `36C1A8` | `4B FF E2 1D` / `4BFFE21D` | I op18 disp -0x1de4 AA0 LK1 -> 80370B04 → `bl 80370B04` | `bl 0x80370b04` |
| `803728EC` | `36C1AC` | `38 7F 01 E4` / `387F01E4` | D op14 d/s3 ra31 imm01E4 | `addi r3, r31, 0x1e4` |
| `803728F0` | `36C1B0` | `4C C6 31 82` / `4CC63182` | XL op19 XO193 BT/BO6 BA/BI6 BB6 LK0 | `crclr cr1eq` |
| `803728F4` | `36C1B4` | `4B FF E3 99` / `4BFFE399` | I op18 disp -0x1c68 AA0 LK1 -> 80370C8C → `bl 80370C8C` | `bl 0x80370c8c` |
| `803728F8` | `36C1B8` | `3C 60 80 37` / `3C608037` | D op15 d/s3 ra0 imm8037 | `lis r3, -0x7fc9` |
| `803728FC` | `36C1BC` | `38 83 26 D8` / `388326D8` | D op14 d/s4 ra3 imm26D8 | `addi r4, r3, 0x26d8` |
| `80372900` | `36C1C0` | `38 60 00 01` / `38600001` | D op14 d/s3 ra0 imm0001 | `li r3, 1` |
| `80372904` | `36C1C4` | `48 00 0A 75` / `48000A75` | I op18 disp +0xa74 AA0 LK1 -> 80373378 → `bl 80373378` | `bl 0x80373378` |
| `80372908` | `36C1C8` | `38 7F 01 FC` / `387F01FC` | D op14 d/s3 ra31 imm01FC | `addi r3, r31, 0x1fc` |
| `8037290C` | `36C1CC` | `4C C6 31 82` / `4CC63182` | XL op19 XO193 BT/BO6 BA/BI6 BB6 LK0 | `crclr cr1eq` |
| `80372910` | `36C1D0` | `4B FF E3 7D` / `4BFFE37D` | I op18 disp -0x1c84 AA0 LK1 -> 80370C8C → `bl 80370C8C` | `bl 0x80370c8c` |
| `80372914` | `36C1D4` | `80 01 00 14` / `80010014` | D op32 d/s0 ra1 imm0014 | `lwz r0, 0x14(r1)` |
| `80372918` | `36C1D8` | `83 E1 00 0C` / `83E1000C` | D op32 d/s31 ra1 imm000C | `lwz r31, 0xc(r1)` |
| `8037291C` | `36C1DC` | `83 C1 00 08` / `83C10008` | D op32 d/s30 ra1 imm0008 | `lwz r30, 8(r1)` |
| `80372920` | `36C1E0` | `38 21 00 10` / `38210010` | D op14 d/s1 ra1 imm0010 | `addi r1, r1, 0x10` |
| `80372924` | `36C1E4` | `7C 08 03 A6` / `7C0803A6` | X op31 XO467 d/s0 ra8 rb0 Rc0 SPR8 | `mtlr r0` |
| `80372928` | `36C1E8` | `4E 80 00 20` / `4E800020` | XL op19 XO16 BT/BO20 BA/BI0 BB0 LK0 | `blr` |

### L2 invalidate helper, all words

| VA | File offset | Raw bytes / BE word | Encoding fields | Independent decode |
|---|---:|---|---|---|
| `80372640` | `36BF00` | `7C 08 02 A6` / `7C0802A6` | X op31 XO339 d/s0 ra8 rb0 Rc0 SPR8 | `mflr r0` |
| `80372644` | `36BF04` | `90 01 00 04` / `90010004` | D op36 d/s0 ra1 imm0004 | `stw r0, 4(r1)` |
| `80372648` | `36BF08` | `94 21 FF F0` / `9421FFF0` | D op37 d/s1 ra1 immFFF0 | `stwu r1, -0x10(r1)` |
| `8037264C` | `36BF0C` | `93 E1 00 0C` / `93E1000C` | D op36 d/s31 ra1 imm000C | `stw r31, 0xc(r1)` |
| `80372650` | `36BF10` | `7C 00 04 AC` / `7C0004AC` | X op31 XO598 d/s0 ra0 rb0 Rc0 | `sync` |
| `80372654` | `36BF14` | `4B FF E4 A9` / `4BFFE4A9` | I op18 disp -0x1b58 AA0 LK1 -> 80370AFC → `bl 80370AFC` | `bl 0x80370afc` |
| `80372658` | `36BF18` | `54 63 00 7E` / `5463007E` | M op21 rs3 ra3 sh0 mb1 me31 Rc0 | `clrlwi r3, r3, 1` |
| `8037265C` | `36BF1C` | `4B FF E4 A9` / `4BFFE4A9` | I op18 disp -0x1b58 AA0 LK1 -> 80370B04 → `bl 80370B04` | `bl 0x80370b04` |
| `80372660` | `36BF20` | `7C 00 04 AC` / `7C0004AC` | X op31 XO598 d/s0 ra0 rb0 Rc0 | `sync` |
| `80372664` | `36BF24` | `4B FF E4 99` / `4BFFE499` | I op18 disp -0x1b68 AA0 LK1 -> 80370AFC → `bl 80370AFC` | `bl 0x80370afc` |
| `80372668` | `36BF28` | `64 63 00 20` / `64630020` | D op25 d/s3 ra3 imm0020 | `oris r3, r3, 0x20` |
| `8037266C` | `36BF2C` | `4B FF E4 99` / `4BFFE499` | I op18 disp -0x1b68 AA0 LK1 -> 80370B04 → `bl 80370B04` | `bl 0x80370b04` |
| `80372670` | `36BF30` | `48 00 00 04` / `48000004` | I op18 disp +0x4 AA0 LK0 -> 80372674 → `b 80372674` | `b 0x80372674` |
| `80372674` | `36BF34` | `48 00 00 04` / `48000004` | I op18 disp +0x4 AA0 LK0 -> 80372678 → `b 80372678` | `b 0x80372678` |
| `80372678` | `36BF38` | `4B FF E4 85` / `4BFFE485` | I op18 disp -0x1b7c AA0 LK1 -> 80370AFC → `bl 80370AFC` | `bl 0x80370afc` |
| `8037267C` | `36BF3C` | `54 60 07 FE` / `546007FE` | M op21 rs3 ra0 sh0 mb31 me31 Rc0 | `clrlwi r0, r3, 0x1f` |
| `80372680` | `36BF40` | `28 00 00 00` / `28000000` | D op10 crf0 ra0 imm0000 | `cmplwi r0, 0` |
| `80372684` | `36BF44` | `40 82 FF F4` / `4082FFF4` | B op16 BO4 BI2 disp -0xc AA0 LK0 -> 80372678 → `bne 80372678` | `bne 0x80372678` |
| `80372688` | `36BF48` | `4B FF E4 75` / `4BFFE475` | I op18 disp -0x1b8c AA0 LK1 -> 80370AFC → `bl 80370AFC` | `bl 0x80370afc` |
| `8037268C` | `36BF4C` | `54 63 02 D2` / `546302D2` | M op21 rs3 ra3 sh0 mb11 me9 Rc0 | `rlwinm r3, r3, 0, 0xb, 9` |
| `80372690` | `36BF50` | `4B FF E4 75` / `4BFFE475` | I op18 disp -0x1b8c AA0 LK1 -> 80370B04 → `bl 80370B04` | `bl 0x80370b04` |
| `80372694` | `36BF54` | `48 00 00 04` / `48000004` | I op18 disp +0x4 AA0 LK0 -> 80372698 → `b 80372698` | `b 0x80372698` |
| `80372698` | `36BF58` | `3C 60 80 56` / `3C608056` | D op15 d/s3 ra0 imm8056 | `lis r3, -0x7faa` |
| `8037269C` | `36BF5C` | `3B E3 13 80` / `3BE31380` | D op14 d/s31 ra3 imm1380 | `addi r31, r3, 0x1380` |
| `803726A0` | `36BF60` | `48 00 00 04` / `48000004` | I op18 disp +0x4 AA0 LK0 -> 803726A4 → `b 803726A4` | `b 0x803726a4` |
| `803726A4` | `36BF64` | `48 00 00 10` / `48000010` | I op18 disp +0x10 AA0 LK0 -> 803726B4 → `b 803726B4` | `b 0x803726b4` |
| `803726A8` | `36BF68` | `7F E3 FB 78` / `7FE3FB78` | X op31 XO444 d/s31 ra3 rb31 Rc0 | `mr r3, r31` |
| `803726AC` | `36BF6C` | `4C C6 31 82` / `4CC63182` | XL op19 XO193 BT/BO6 BA/BI6 BB6 LK0 | `crclr cr1eq` |
| `803726B0` | `36BF70` | `4B FF E5 DD` / `4BFFE5DD` | I op18 disp -0x1a24 AA0 LK1 -> 80370C8C → `bl 80370C8C` | `bl 0x80370c8c` |
| `803726B4` | `36BF74` | `4B FF E4 49` / `4BFFE449` | I op18 disp -0x1bb8 AA0 LK1 -> 80370AFC → `bl 80370AFC` | `bl 0x80370afc` |
| `803726B8` | `36BF78` | `54 60 07 FE` / `546007FE` | M op21 rs3 ra0 sh0 mb31 me31 Rc0 | `clrlwi r0, r3, 0x1f` |
| `803726BC` | `36BF7C` | `28 00 00 00` / `28000000` | D op10 crf0 ra0 imm0000 | `cmplwi r0, 0` |
| `803726C0` | `36BF80` | `40 82 FF E8` / `4082FFE8` | B op16 BO4 BI2 disp -0x18 AA0 LK0 -> 803726A8 → `bne 803726A8` | `bne 0x803726a8` |
| `803726C4` | `36BF84` | `80 01 00 14` / `80010014` | D op32 d/s0 ra1 imm0014 | `lwz r0, 0x14(r1)` |
| `803726C8` | `36BF88` | `83 E1 00 0C` / `83E1000C` | D op32 d/s31 ra1 imm000C | `lwz r31, 0xc(r1)` |
| `803726CC` | `36BF8C` | `38 21 00 10` / `38210010` | D op14 d/s1 ra1 imm0010 | `addi r1, r1, 0x10` |
| `803726D0` | `36BF90` | `7C 08 03 A6` / `7C0803A6` | X op31 XO467 d/s0 ra8 rb0 Rc0 SPR8 | `mtlr r0` |
| `803726D4` | `36BF94` | `4E 80 00 20` / `4E800020` | XL op19 XO16 BT/BO20 BA/BI0 BB0 LK0 | `blr` |

### Handler installer, including the unselected selector-16 body

| VA | File offset | Raw bytes / BE word | Encoding fields | Independent decode |
|---|---:|---|---|---|
| `80373378` | `36CC38` | `7C 08 02 A6` / `7C0802A6` | X op31 XO339 d/s0 ra8 rb0 Rc0 SPR8 | `mflr r0` |
| `8037337C` | `36CC3C` | `90 01 00 04` / `90010004` | D op36 d/s0 ra1 imm0004 | `stw r0, 4(r1)` |
| `80373380` | `36CC40` | `94 21 FF D0` / `9421FFD0` | D op37 d/s1 ra1 immFFD0 | `stwu r1, -0x30(r1)` |
| `80373384` | `36CC44` | `93 E1 00 2C` / `93E1002C` | D op36 d/s31 ra1 imm002C | `stw r31, 0x2c(r1)` |
| `80373388` | `36CC48` | `93 C1 00 28` / `93C10028` | D op36 d/s30 ra1 imm0028 | `stw r30, 0x28(r1)` |
| `8037338C` | `36CC4C` | `93 A1 00 24` / `93A10024` | D op36 d/s29 ra1 imm0024 | `stw r29, 0x24(r1)` |
| `80373390` | `36CC50` | `3B A3 00 00` / `3BA30000` | D op14 d/s29 ra3 imm0000 | `addi r29, r3, 0` |
| `80373394` | `36CC54` | `93 81 00 20` / `93810020` | D op36 d/s28 ra1 imm0020 | `stw r28, 0x20(r1)` |
| `80373398` | `36CC58` | `3B 84 00 00` / `3B840000` | D op14 d/s28 ra4 imm0000 | `addi r28, r4, 0` |
| `8037339C` | `36CC5C` | `48 00 2D 81` / `48002D81` | I op18 disp +0x2d80 AA0 LK1 -> 8037611C → `bl 8037611C` | `bl 0x8037611c` |
| `803733A0` | `36CC60` | `3C 80 80 58` / `3C808058` | D op15 d/s4 ra0 imm8058 | `lis r4, -0x7fa8` |
| `803733A4` | `36CC64` | `57 A5 13 BA` / `57A513BA` | M op21 rs29 ra5 sh2 mb14 me29 Rc0 | `rlwinm r5, r29, 2, 0xe, 0x1d` |
| `803733A8` | `36CC68` | `38 04 6C B0` / `38046CB0` | D op14 d/s0 ra4 imm6CB0 | `addi r0, r4, 0x6cb0` |
| `803733AC` | `36CC6C` | `57 A6 04 3E` / `57A6043E` | M op21 rs29 ra6 sh0 mb16 me31 Rc0 | `clrlwi r6, r29, 0x10` |
| `803733B0` | `36CC70` | `7C 80 2A 14` / `7C802A14` | X op31 XO266 d/s4 ra0 rb5 Rc0 | `add r4, r0, r5` |
| `803733B4` | `36CC74` | `83 C4 00 00` / `83C40000` | D op32 d/s30 ra4 imm0000 | `lwz r30, 0(r4)` |
| `803733B8` | `36CC78` | `28 06 00 10` / `28060010` | D op10 crf0 ra6 imm0010 | `cmplwi r6, 0x10` |
| `803733BC` | `36CC7C` | `7C 7D 1B 78` / `7C7D1B78` | X op31 XO444 d/s3 ra29 rb3 Rc0 | `mr r29, r3` |
| `803733C0` | `36CC80` | `93 84 00 00` / `93840000` | D op36 d/s28 ra4 imm0000 | `stw r28, 0(r4)` |
| `803733C4` | `36CC84` | `40 82 01 A0` / `408201A0` | B op16 BO4 BI2 disp +0x1a0 AA0 LK0 -> 80373564 → `bne 80373564` | `bne 0x80373564` |
| `803733C8` | `36CC88` | `4B FF D7 15` / `4BFFD715` | I op18 disp -0x28ec AA0 LK1 -> 80370ADC → `bl 80370ADC` | `bl 0x80370adc` |
| `803733CC` | `36CC8C` | `3B E3 00 00` / `3BE30000` | D op14 d/s31 ra3 imm0000 | `addi r31, r3, 0` |
| `803733D0` | `36CC90` | `63 E3 20 00` / `63E32000` | D op24 d/s31 ra3 imm2000 | `ori r3, r31, 0x2000` |
| `803733D4` | `36CC94` | `4B FF D7 11` / `4BFFD711` | I op18 disp -0x28f0 AA0 LK1 -> 80370AE4 → `bl 80370AE4` | `bl 0x80370ae4` |
| `803733D8` | `36CC98` | `4B FF D7 89` / `4BFFD789` | I op18 disp -0x2878 AA0 LK1 -> 80370B60 → `bl 80370B60` | `bl 0x80370b60` |
| `803733DC` | `36CC9C` | `28 1C 00 00` / `281C0000` | D op10 crf0 ra28 imm0000 | `cmplwi r28, 0` |
| `803733E0` | `36CCA0` | `41 82 01 18` / `41820118` | B op16 BO12 BI2 disp +0x118 AA0 LK0 -> 803734F8 → `beq 803734F8` | `beq 0x803734f8` |
| `803733E4` | `36CCA4` | `3C A0 80 00` / `3CA08000` | D op15 d/s5 ra0 imm8000 | `lis r5, -0x8000` |
| `803733E8` | `36CCA8` | `3C 80 60 06` / `3C806006` | D op15 d/s4 ra0 imm6006 | `lis r4, 0x6006` |
| `803733EC` | `36CCAC` | `80 C5 00 DC` / `80C500DC` | D op32 d/s6 ra5 imm00DC | `lwz r6, 0xdc(r5)` |
| `803733F0` | `36CCB0` | `38 84 F8 FF` / `3884F8FF` | D op14 d/s4 ra4 immF8FF | `addi r4, r4, -0x701` |
| `803733F4` | `36CCB4` | `48 00 00 E8` / `480000E8` | I op18 disp +0xe8 AA0 LK0 -> 803734DC → `b 803734DC` | `b 0x803734dc` |
| `803733F8` | `36CCB8` | `80 06 01 9C` / `8006019C` | D op32 d/s0 ra6 imm019C | `lwz r0, 0x19c(r6)` |
| `803733FC` | `36CCBC` | `60 00 09 00` / `60000900` | D op24 d/s0 ra0 imm0900 | `ori r0, r0, 0x900` |
| `80373400` | `36CCC0` | `90 06 01 9C` / `9006019C` | D op36 d/s0 ra6 imm019C | `stw r0, 0x19c(r6)` |
| `80373404` | `36CCC4` | `A0 A6 01 A2` / `A0A601A2` | D op40 d/s5 ra6 imm01A2 | `lhz r5, 0x1a2(r6)` |
| `80373408` | `36CCC8` | `54 A0 07 FF` / `54A007FF` | M op21 rs5 ra0 sh0 mb31 me31 Rc1 | `clrlwi. r0, r5, 0x1f` |
| `8037340C` | `36CCCC` | `40 82 00 AC` / `408200AC` | B op16 BO4 BI2 disp +0xac AA0 LK0 -> 803734B8 → `bne 803734B8` | `bne 0x803734b8` |
| `80373410` | `36CCD0` | `60 A5 00 01` / `60A50001` | D op24 d/s5 ra5 imm0001 | `ori r5, r5, 1` |
| `80373414` | `36CCD4` | `38 00 00 04` / `38000004` | D op14 d/s0 ra0 imm0004 | `li r0, 4` |
| `80373418` | `36CCD8` | `B0 A6 01 A2` / `B0A601A2` | D op44 d/s5 ra6 imm01A2 | `sth r5, 0x1a2(r6)` |
| `8037341C` | `36CCDC` | `7C 09 03 A6` / `7C0903A6` | X op31 XO467 d/s0 ra9 rb0 Rc0 SPR9 | `mtctr r0` |
| `80373420` | `36CCE0` | `38 A6 00 00` / `38A60000` | D op14 d/s5 ra6 imm0000 | `addi r5, r6, 0` |
| `80373424` | `36CCE4` | `38 00 FF FF` / `3800FFFF` | D op14 d/s0 ra0 immFFFF | `li r0, -1` |
| `80373428` | `36CCE8` | `90 05 00 94` / `90050094` | D op36 d/s0 ra5 imm0094 | `stw r0, 0x94(r5)` |
| `8037342C` | `36CCEC` | `90 05 00 90` / `90050090` | D op36 d/s0 ra5 imm0090 | `stw r0, 0x90(r5)` |
| `80373430` | `36CCF0` | `90 05 01 CC` / `900501CC` | D op36 d/s0 ra5 imm01CC | `stw r0, 0x1cc(r5)` |
| `80373434` | `36CCF4` | `90 05 01 C8` / `900501C8` | D op36 d/s0 ra5 imm01C8 | `stw r0, 0x1c8(r5)` |
| `80373438` | `36CCF8` | `90 05 00 9C` / `9005009C` | D op36 d/s0 ra5 imm009C | `stw r0, 0x9c(r5)` |
| `8037343C` | `36CCFC` | `90 05 00 98` / `90050098` | D op36 d/s0 ra5 imm0098 | `stw r0, 0x98(r5)` |
| `80373440` | `36CD00` | `90 05 01 D4` / `900501D4` | D op36 d/s0 ra5 imm01D4 | `stw r0, 0x1d4(r5)` |
| `80373444` | `36CD04` | `90 05 01 D0` / `900501D0` | D op36 d/s0 ra5 imm01D0 | `stw r0, 0x1d0(r5)` |
| `80373448` | `36CD08` | `90 05 00 A4` / `900500A4` | D op36 d/s0 ra5 imm00A4 | `stw r0, 0xa4(r5)` |
| `8037344C` | `36CD0C` | `90 05 00 A0` / `900500A0` | D op36 d/s0 ra5 imm00A0 | `stw r0, 0xa0(r5)` |
| `80373450` | `36CD10` | `90 05 01 DC` / `900501DC` | D op36 d/s0 ra5 imm01DC | `stw r0, 0x1dc(r5)` |
| `80373454` | `36CD14` | `90 05 01 D8` / `900501D8` | D op36 d/s0 ra5 imm01D8 | `stw r0, 0x1d8(r5)` |
| `80373458` | `36CD18` | `90 05 00 AC` / `900500AC` | D op36 d/s0 ra5 imm00AC | `stw r0, 0xac(r5)` |
| `8037345C` | `36CD1C` | `90 05 00 A8` / `900500A8` | D op36 d/s0 ra5 imm00A8 | `stw r0, 0xa8(r5)` |
| `80373460` | `36CD20` | `90 05 01 E4` / `900501E4` | D op36 d/s0 ra5 imm01E4 | `stw r0, 0x1e4(r5)` |
| `80373464` | `36CD24` | `90 05 01 E0` / `900501E0` | D op36 d/s0 ra5 imm01E0 | `stw r0, 0x1e0(r5)` |
| `80373468` | `36CD28` | `90 05 00 B4` / `900500B4` | D op36 d/s0 ra5 imm00B4 | `stw r0, 0xb4(r5)` |
| `8037346C` | `36CD2C` | `90 05 00 B0` / `900500B0` | D op36 d/s0 ra5 imm00B0 | `stw r0, 0xb0(r5)` |
| `80373470` | `36CD30` | `90 05 01 EC` / `900501EC` | D op36 d/s0 ra5 imm01EC | `stw r0, 0x1ec(r5)` |
| `80373474` | `36CD34` | `90 05 01 E8` / `900501E8` | D op36 d/s0 ra5 imm01E8 | `stw r0, 0x1e8(r5)` |
| `80373478` | `36CD38` | `90 05 00 BC` / `900500BC` | D op36 d/s0 ra5 imm00BC | `stw r0, 0xbc(r5)` |
| `8037347C` | `36CD3C` | `90 05 00 B8` / `900500B8` | D op36 d/s0 ra5 imm00B8 | `stw r0, 0xb8(r5)` |
| `80373480` | `36CD40` | `90 05 01 F4` / `900501F4` | D op36 d/s0 ra5 imm01F4 | `stw r0, 0x1f4(r5)` |
| `80373484` | `36CD44` | `90 05 01 F0` / `900501F0` | D op36 d/s0 ra5 imm01F0 | `stw r0, 0x1f0(r5)` |
| `80373488` | `36CD48` | `90 05 00 C4` / `900500C4` | D op36 d/s0 ra5 imm00C4 | `stw r0, 0xc4(r5)` |
| `8037348C` | `36CD4C` | `90 05 00 C0` / `900500C0` | D op36 d/s0 ra5 imm00C0 | `stw r0, 0xc0(r5)` |
| `80373490` | `36CD50` | `90 05 01 FC` / `900501FC` | D op36 d/s0 ra5 imm01FC | `stw r0, 0x1fc(r5)` |
| `80373494` | `36CD54` | `90 05 01 F8` / `900501F8` | D op36 d/s0 ra5 imm01F8 | `stw r0, 0x1f8(r5)` |
| `80373498` | `36CD58` | `90 05 00 CC` / `900500CC` | D op36 d/s0 ra5 imm00CC | `stw r0, 0xcc(r5)` |
| `8037349C` | `36CD5C` | `90 05 00 C8` / `900500C8` | D op36 d/s0 ra5 imm00C8 | `stw r0, 0xc8(r5)` |
| `803734A0` | `36CD60` | `90 05 02 04` / `90050204` | D op36 d/s0 ra5 imm0204 | `stw r0, 0x204(r5)` |
| `803734A4` | `36CD64` | `90 05 02 00` / `90050200` | D op36 d/s0 ra5 imm0200 | `stw r0, 0x200(r5)` |
| `803734A8` | `36CD68` | `38 A5 00 40` / `38A50040` | D op14 d/s5 ra5 imm0040 | `addi r5, r5, 0x40` |
| `803734AC` | `36CD6C` | `42 00 FF 78` / `4200FF78` | B op16 BO16 BI0 disp -0x88 AA0 LK0 -> 80373424 → `bdnz 80373424` | `bdnz 0x80373424` |
| `803734B0` | `36CD70` | `38 00 00 04` / `38000004` | D op14 d/s0 ra0 imm0004 | `li r0, 4` |
| `803734B4` | `36CD74` | `90 06 01 94` / `90060194` | D op36 d/s0 ra6 imm0194 | `stw r0, 0x194(r6)` |
| `803734B8` | `36CD78` | `80 0D 26 F0` / `800D26F0` | D op32 d/s0 ra13 imm26F0 | `lwz r0, 0x26f0(r13)` |
| `803734BC` | `36CD7C` | `80 A6 01 94` / `80A60194` | D op32 d/s5 ra6 imm0194 | `lwz r5, 0x194(r6)` |
| `803734C0` | `36CD80` | `54 00 06 38` / `54000638` | M op21 rs0 ra0 sh0 mb24 me28 Rc0 | `rlwinm r0, r0, 0, 0x18, 0x1c` |
| `803734C4` | `36CD84` | `7C A0 03 78` / `7CA00378` | X op31 XO444 d/s5 ra0 rb0 Rc0 | `or r0, r5, r0` |
| `803734C8` | `36CD88` | `90 06 01 94` / `90060194` | D op36 d/s0 ra6 imm0194 | `stw r0, 0x194(r6)` |
| `803734CC` | `36CD8C` | `80 06 01 94` / `80060194` | D op32 d/s0 ra6 imm0194 | `lwz r0, 0x194(r6)` |
| `803734D0` | `36CD90` | `7C 00 20 38` / `7C002038` | X op31 XO28 d/s0 ra0 rb4 Rc0 | `and r0, r0, r4` |
| `803734D4` | `36CD94` | `90 06 01 94` / `90060194` | D op36 d/s0 ra6 imm0194 | `stw r0, 0x194(r6)` |
| `803734D8` | `36CD98` | `80 C6 02 FC` / `80C602FC` | D op32 d/s6 ra6 imm02FC | `lwz r6, 0x2fc(r6)` |
| `803734DC` | `36CD9C` | `28 06 00 00` / `28060000` | D op10 crf0 ra6 imm0000 | `cmplwi r6, 0` |
| `803734E0` | `36CDA0` | `40 82 FF 18` / `4082FF18` | B op16 BO4 BI2 disp -0xe8 AA0 LK0 -> 803733F8 → `bne 803733F8` | `bne 0x803733f8` |
| `803734E4` | `36CDA4` | `80 0D 26 F0` / `800D26F0` | D op32 d/s0 ra13 imm26F0 | `lwz r0, 0x26f0(r13)` |
| `803734E8` | `36CDA8` | `63 FF 09 00` / `63FF0900` | D op24 d/s31 ra31 imm0900 | `ori r31, r31, 0x900` |
| `803734EC` | `36CDAC` | `54 00 06 38` / `54000638` | M op21 rs0 ra0 sh0 mb24 me28 Rc0 | `rlwinm r0, r0, 0, 0x18, 0x1c` |
| `803734F0` | `36CDB0` | `7C 63 03 78` / `7C630378` | X op31 XO444 d/s3 ra3 rb0 Rc0 | `or r3, r3, r0` |
| `803734F4` | `36CDB4` | `48 00 00 58` / `48000058` | I op18 disp +0x58 AA0 LK0 -> 8037354C → `b 8037354C` | `b 0x8037354c` |
| `803734F8` | `36CDB8` | `3C A0 80 00` / `3CA08000` | D op15 d/s5 ra0 imm8000 | `lis r5, -0x8000` |
| `803734FC` | `36CDBC` | `3C 80 60 06` / `3C806006` | D op15 d/s4 ra0 imm6006 | `lis r4, 0x6006` |
| `80373500` | `36CDC0` | `80 C5 00 DC` / `80C500DC` | D op32 d/s6 ra5 imm00DC | `lwz r6, 0xdc(r5)` |
| `80373504` | `36CDC4` | `38 84 F8 FF` / `3884F8FF` | D op14 d/s4 ra4 immF8FF | `addi r4, r4, -0x701` |
| `80373508` | `36CDC8` | `38 A0 F6 FF` / `38A0F6FF` | D op14 d/s5 ra0 immF6FF | `li r5, -0x901` |
| `8037350C` | `36CDCC` | `48 00 00 2C` / `4800002C` | I op18 disp +0x2c AA0 LK0 -> 80373538 → `b 80373538` | `b 0x80373538` |
| `80373510` | `36CDD0` | `80 06 01 9C` / `8006019C` | D op32 d/s0 ra6 imm019C | `lwz r0, 0x19c(r6)` |
| `80373514` | `36CDD4` | `7C 00 28 38` / `7C002838` | X op31 XO28 d/s0 ra0 rb5 Rc0 | `and r0, r0, r5` |
| `80373518` | `36CDD8` | `90 06 01 9C` / `9006019C` | D op36 d/s0 ra6 imm019C | `stw r0, 0x19c(r6)` |
| `8037351C` | `36CDDC` | `80 06 01 94` / `80060194` | D op32 d/s0 ra6 imm0194 | `lwz r0, 0x194(r6)` |
| `80373520` | `36CDE0` | `54 00 07 6E` / `5400076E` | M op21 rs0 ra0 sh0 mb29 me23 Rc0 | `rlwinm r0, r0, 0, 0x1d, 0x17` |
| `80373524` | `36CDE4` | `90 06 01 94` / `90060194` | D op36 d/s0 ra6 imm0194 | `stw r0, 0x194(r6)` |
| `80373528` | `36CDE8` | `80 06 01 94` / `80060194` | D op32 d/s0 ra6 imm0194 | `lwz r0, 0x194(r6)` |
| `8037352C` | `36CDEC` | `7C 00 20 38` / `7C002038` | X op31 XO28 d/s0 ra0 rb4 Rc0 | `and r0, r0, r4` |
| `80373530` | `36CDF0` | `90 06 01 94` / `90060194` | D op36 d/s0 ra6 imm0194 | `stw r0, 0x194(r6)` |
| `80373534` | `36CDF4` | `80 C6 02 FC` / `80C602FC` | D op32 d/s6 ra6 imm02FC | `lwz r6, 0x2fc(r6)` |
| `80373538` | `36CDF8` | `28 06 00 00` / `28060000` | D op10 crf0 ra6 imm0000 | `cmplwi r6, 0` |
| `8037353C` | `36CDFC` | `40 82 FF D4` / `4082FFD4` | B op16 BO4 BI2 disp -0x2c AA0 LK0 -> 80373510 → `bne 80373510` | `bne 0x80373510` |
| `80373540` | `36CE00` | `38 00 F6 FF` / `3800F6FF` | D op14 d/s0 ra0 immF6FF | `li r0, -0x901` |
| `80373544` | `36CE04` | `54 63 07 6E` / `5463076E` | M op21 rs3 ra3 sh0 mb29 me23 Rc0 | `rlwinm r3, r3, 0, 0x1d, 0x17` |
| `80373548` | `36CE08` | `7F FF 00 38` / `7FFF0038` | X op31 XO28 d/s31 ra31 rb0 Rc0 | `and r31, r31, r0` |
| `8037354C` | `36CE0C` | `3C 80 60 06` / `3C806006` | D op15 d/s4 ra0 imm6006 | `lis r4, 0x6006` |
| `80373550` | `36CE10` | `38 04 F8 FF` / `3804F8FF` | D op14 d/s0 ra4 immF8FF | `addi r0, r4, -0x701` |
| `80373554` | `36CE14` | `7C 63 00 38` / `7C630038` | X op31 XO28 d/s3 ra3 rb0 Rc0 | `and r3, r3, r0` |
| `80373558` | `36CE18` | `4B FF D6 29` / `4BFFD629` | I op18 disp -0x29d8 AA0 LK1 -> 80370B80 → `bl 80370B80` | `bl 0x80370b80` |
| `8037355C` | `36CE1C` | `7F E3 FB 78` / `7FE3FB78` | X op31 XO444 d/s31 ra3 rb31 Rc0 | `mr r3, r31` |
| `80373560` | `36CE20` | `4B FF D5 85` / `4BFFD585` | I op18 disp -0x2a7c AA0 LK1 -> 80370AE4 → `bl 80370AE4` | `bl 0x80370ae4` |
| `80373564` | `36CE24` | `7F A3 EB 78` / `7FA3EB78` | X op31 XO444 d/s29 ra3 rb29 Rc0 | `mr r3, r29` |
| `80373568` | `36CE28` | `48 00 2B DD` / `48002BDD` | I op18 disp +0x2bdc AA0 LK1 -> 80376144 → `bl 80376144` | `bl 0x80376144` |
| `8037356C` | `36CE2C` | `7F C3 F3 78` / `7FC3F378` | X op31 XO444 d/s30 ra3 rb30 Rc0 | `mr r3, r30` |
| `80373570` | `36CE30` | `80 01 00 34` / `80010034` | D op32 d/s0 ra1 imm0034 | `lwz r0, 0x34(r1)` |
| `80373574` | `36CE34` | `83 E1 00 2C` / `83E1002C` | D op32 d/s31 ra1 imm002C | `lwz r31, 0x2c(r1)` |
| `80373578` | `36CE38` | `83 C1 00 28` / `83C10028` | D op32 d/s30 ra1 imm0028 | `lwz r30, 0x28(r1)` |
| `8037357C` | `36CE3C` | `83 A1 00 24` / `83A10024` | D op32 d/s29 ra1 imm0024 | `lwz r29, 0x24(r1)` |
| `80373580` | `36CE40` | `83 81 00 20` / `83810020` | D op32 d/s28 ra1 imm0020 | `lwz r28, 0x20(r1)` |
| `80373584` | `36CE44` | `38 21 00 30` / `38210030` | D op14 d/s1 ra1 imm0030 | `addi r1, r1, 0x30` |
| `80373588` | `36CE48` | `7C 08 03 A6` / `7C0803A6` | X op31 XO467 d/s0 ra8 rb0 Rc0 SPR8 | `mtlr r0` |
| `8037358C` | `36CE4C` | `4E 80 00 20` / `4E800020` | XL op19 XO16 BT/BO20 BA/BI0 BB0 LK0 | `blr` |

### Direct HID0/L2CR/MSR accessors and optional L1 helpers

| VA | File offset | Raw bytes / BE word | Encoding fields | Independent decode |
|---|---:|---|---|---|
| `80370AEC` | `36A3AC` | `7C 70 FA A6` / `7C70FAA6` | X op31 XO339 d/s3 ra16 rb31 Rc0 SPR1008 | `mfspr r3, 0x3f0` |
| `80370AF0` | `36A3B0` | `4E 80 00 20` / `4E800020` | XL op19 XO16 BT/BO20 BA/BI0 BB0 LK0 | `blr` |
| `80370AFC` | `36A3BC` | `7C 79 FA A6` / `7C79FAA6` | X op31 XO339 d/s3 ra25 rb31 Rc0 SPR1017 | `mfspr r3, 0x3f9` |
| `80370B00` | `36A3C0` | `4E 80 00 20` / `4E800020` | XL op19 XO16 BT/BO20 BA/BI0 BB0 LK0 | `blr` |
| `80370B04` | `36A3C4` | `7C 79 FB A6` / `7C79FBA6` | X op31 XO467 d/s3 ra25 rb31 Rc0 SPR1017 | `mtspr 0x3f9, r3` |
| `80370B08` | `36A3C8` | `4E 80 00 20` / `4E800020` | XL op19 XO16 BT/BO20 BA/BI0 BB0 LK0 | `blr` |
| `80370ADC` | `36A39C` | `7C 60 00 A6` / `7C6000A6` | X op31 XO83 d/s3 ra0 rb0 Rc0 | `mfmsr r3` |
| `80370AE0` | `36A3A0` | `4E 80 00 20` / `4E800020` | XL op19 XO16 BT/BO20 BA/BI0 BB0 LK0 | `blr` |
| `80370AE4` | `36A3A4` | `7C 60 01 24` / `7C600124` | X op31 XO146 d/s3 ra0 rb0 Rc0 | `mtmsr r3` |
| `80370AE8` | `36A3A8` | `4E 80 00 20` / `4E800020` | XL op19 XO16 BT/BO20 BA/BI0 BB0 LK0 | `blr` |
| `80372604` | `36BEC4` | `4C 00 01 2C` / `4C00012C` | XL op19 XO150 BT/BO0 BA/BI0 BB0 LK0 | `isync` |
| `80372608` | `36BEC8` | `7C 70 FA A6` / `7C70FAA6` | X op31 XO339 d/s3 ra16 rb31 Rc0 SPR1008 | `mfspr r3, 0x3f0` |
| `8037260C` | `36BECC` | `60 63 80 00` / `60638000` | D op24 d/s3 ra3 imm8000 | `ori r3, r3, 0x8000` |
| `80372610` | `36BED0` | `7C 70 FB A6` / `7C70FBA6` | X op31 XO467 d/s3 ra16 rb31 Rc0 SPR1008 | `mtspr 0x3f0, r3` |
| `80372614` | `36BED4` | `4E 80 00 20` / `4E800020` | XL op19 XO16 BT/BO20 BA/BI0 BB0 LK0 | `blr` |
| `803724F4` | `36BDB4` | `7C 00 04 AC` / `7C0004AC` | X op31 XO598 d/s0 ra0 rb0 Rc0 | `sync` |
| `803724F8` | `36BDB8` | `7C 70 FA A6` / `7C70FAA6` | X op31 XO339 d/s3 ra16 rb31 Rc0 SPR1008 | `mfspr r3, 0x3f0` |
| `803724FC` | `36BDBC` | `60 63 40 00` / `60634000` | D op24 d/s3 ra3 imm4000 | `ori r3, r3, 0x4000` |
| `80372500` | `36BDC0` | `7C 70 FB A6` / `7C70FBA6` | X op31 XO467 d/s3 ra16 rb31 Rc0 SPR1008 | `mtspr 0x3f0, r3` |
| `80372504` | `36BDC4` | `4E 80 00 20` / `4E800020` | XL op19 XO16 BT/BO20 BA/BI0 BB0 LK0 | `blr` |
| `8037611C` | `36F9DC` | `7C 60 00 A6` / `7C6000A6` | X op31 XO83 d/s3 ra0 rb0 Rc0 | `mfmsr r3` |
| `80376120` | `36F9E0` | `54 64 04 5E` / `5464045E` | M op21 rs3 ra4 sh0 mb17 me15 Rc0 | `rlwinm r4, r3, 0, 0x11, 0xf` |
| `80376124` | `36F9E4` | `7C 80 01 24` / `7C800124` | X op31 XO146 d/s4 ra0 rb0 Rc0 | `mtmsr r4` |
| `80376128` | `36F9E8` | `54 63 8F FE` / `54638FFE` | M op21 rs3 ra3 sh17 mb31 me31 Rc0 | `rlwinm r3, r3, 0x11, 0x1f, 0x1f` |
| `8037612C` | `36F9EC` | `4E 80 00 20` / `4E800020` | XL op19 XO16 BT/BO20 BA/BI0 BB0 LK0 | `blr` |
| `80376144` | `36FA04` | `2C 03 00 00` / `2C030000` | D op11 crf0 ra3 imm0000 | `cmpwi r3, 0` |
| `80376148` | `36FA08` | `7C 80 00 A6` / `7C8000A6` | X op31 XO83 d/s4 ra0 rb0 Rc0 | `mfmsr r4` |
| `8037614C` | `36FA0C` | `41 82 00 0C` / `4182000C` | B op16 BO12 BI2 disp +0xc AA0 LK0 -> 80376158 → `beq 80376158` | `beq 0x80376158` |
| `80376150` | `36FA10` | `60 85 80 00` / `60858000` | D op24 d/s4 ra5 imm8000 | `ori r5, r4, 0x8000` |
| `80376154` | `36FA14` | `48 00 00 08` / `48000008` | I op18 disp +0x8 AA0 LK0 -> 8037615C → `b 8037615C` | `b 0x8037615c` |
| `80376158` | `36FA18` | `54 85 04 5E` / `5485045E` | M op21 rs4 ra5 sh0 mb17 me15 Rc0 | `rlwinm r5, r4, 0, 0x11, 0xf` |
| `8037615C` | `36FA1C` | `7C A0 01 24` / `7CA00124` | X op31 XO146 d/s5 ra0 rb0 Rc0 | `mtmsr r5` |
| `80376160` | `36FA20` | `54 83 8F FE` / `54838FFE` | M op21 rs4 ra3 sh17 mb31 me31 Rc0 | `rlwinm r3, r4, 0x11, 0x1f, 0x1f` |
| `80376164` | `36FA24` | `4E 80 00 20` / `4E800020` | XL op19 XO16 BT/BO20 BA/BI0 BB0 LK0 | `blr` |

### Logger and selector-16-only FPSCR helpers

| VA | File offset | Raw bytes / BE word | Encoding fields | Independent decode |
|---|---:|---|---|---|
| `80370C8C` | `36A54C` | `94 21 FF 90` / `9421FF90` | D op37 d/s1 ra1 immFF90 | `stwu r1, -0x70(r1)` |
| `80370C90` | `36A550` | `40 86 00 24` / `40860024` | B op16 BO4 BI6 disp +0x24 AA0 LK0 -> 80370CB4 → `bne cr1, 80370CB4` | `bne cr1, 0x80370cb4` |
| `80370C94` | `36A554` | `D8 21 00 28` / `D8210028` | D op54 d/s1 ra1 imm0028 | `stfd f1, 0x28(r1)` |
| `80370C98` | `36A558` | `D8 41 00 30` / `D8410030` | D op54 d/s2 ra1 imm0030 | `stfd f2, 0x30(r1)` |
| `80370C9C` | `36A55C` | `D8 61 00 38` / `D8610038` | D op54 d/s3 ra1 imm0038 | `stfd f3, 0x38(r1)` |
| `80370CA0` | `36A560` | `D8 81 00 40` / `D8810040` | D op54 d/s4 ra1 imm0040 | `stfd f4, 0x40(r1)` |
| `80370CA4` | `36A564` | `D8 A1 00 48` / `D8A10048` | D op54 d/s5 ra1 imm0048 | `stfd f5, 0x48(r1)` |
| `80370CA8` | `36A568` | `D8 C1 00 50` / `D8C10050` | D op54 d/s6 ra1 imm0050 | `stfd f6, 0x50(r1)` |
| `80370CAC` | `36A56C` | `D8 E1 00 58` / `D8E10058` | D op54 d/s7 ra1 imm0058 | `stfd f7, 0x58(r1)` |
| `80370CB0` | `36A570` | `D9 01 00 60` / `D9010060` | D op54 d/s8 ra1 imm0060 | `stfd f8, 0x60(r1)` |
| `80370CB4` | `36A574` | `90 61 00 08` / `90610008` | D op36 d/s3 ra1 imm0008 | `stw r3, 8(r1)` |
| `80370CB8` | `36A578` | `90 81 00 0C` / `9081000C` | D op36 d/s4 ra1 imm000C | `stw r4, 0xc(r1)` |
| `80370CBC` | `36A57C` | `90 A1 00 10` / `90A10010` | D op36 d/s5 ra1 imm0010 | `stw r5, 0x10(r1)` |
| `80370CC0` | `36A580` | `90 C1 00 14` / `90C10014` | D op36 d/s6 ra1 imm0014 | `stw r6, 0x14(r1)` |
| `80370CC4` | `36A584` | `90 E1 00 18` / `90E10018` | D op36 d/s7 ra1 imm0018 | `stw r7, 0x18(r1)` |
| `80370CC8` | `36A588` | `91 01 00 1C` / `9101001C` | D op36 d/s8 ra1 imm001C | `stw r8, 0x1c(r1)` |
| `80370CCC` | `36A58C` | `91 21 00 20` / `91210020` | D op36 d/s9 ra1 imm0020 | `stw r9, 0x20(r1)` |
| `80370CD0` | `36A590` | `91 41 00 24` / `91410024` | D op36 d/s10 ra1 imm0024 | `stw r10, 0x24(r1)` |
| `80370CD4` | `36A594` | `38 21 00 70` / `38210070` | D op14 d/s1 ra1 imm0070 | `addi r1, r1, 0x70` |
| `80370CD8` | `36A598` | `4E 80 00 20` / `4E800020` | XL op19 XO16 BT/BO20 BA/BI0 BB0 LK0 | `blr` |
| `80370B60` | `36A420` | `94 21 FF E8` / `9421FFE8` | D op37 d/s1 ra1 immFFE8 | `stwu r1, -0x18(r1)` |
| `80370B64` | `36A424` | `DB E1 00 10` / `DBE10010` | D op54 d/s31 ra1 imm0010 | `stfd f31, 0x10(r1)` |
| `80370B68` | `36A428` | `FF E0 04 8E` / `FFE0048E` | A/X op63 d31 a0 b0 XO583 Rc0 | `mffs f31` |
| `80370B6C` | `36A42C` | `DB E1 00 08` / `DBE10008` | D op54 d/s31 ra1 imm0008 | `stfd f31, 8(r1)` |
| `80370B70` | `36A430` | `80 61 00 0C` / `8061000C` | D op32 d/s3 ra1 imm000C | `lwz r3, 0xc(r1)` |
| `80370B74` | `36A434` | `CB E1 00 10` / `CBE10010` | D op50 d/s31 ra1 imm0010 | `lfd f31, 0x10(r1)` |
| `80370B78` | `36A438` | `38 21 00 18` / `38210018` | D op14 d/s1 ra1 imm0018 | `addi r1, r1, 0x18` |
| `80370B7C` | `36A43C` | `4E 80 00 20` / `4E800020` | XL op19 XO16 BT/BO20 BA/BI0 BB0 LK0 | `blr` |
| `80370B80` | `36A440` | `94 21 FF E0` / `9421FFE0` | D op37 d/s1 ra1 immFFE0 | `stwu r1, -0x20(r1)` |
| `80370B84` | `36A444` | `DB E1 00 18` / `DBE10018` | D op54 d/s31 ra1 imm0018 | `stfd f31, 0x18(r1)` |
| `80370B88` | `36A448` | `38 80 00 00` / `38800000` | D op14 d/s4 ra0 imm0000 | `li r4, 0` |
| `80370B8C` | `36A44C` | `90 81 00 10` / `90810010` | D op36 d/s4 ra1 imm0010 | `stw r4, 0x10(r1)` |
| `80370B90` | `36A450` | `90 61 00 14` / `90610014` | D op36 d/s3 ra1 imm0014 | `stw r3, 0x14(r1)` |
| `80370B94` | `36A454` | `CB E1 00 10` / `CBE10010` | D op50 d/s31 ra1 imm0010 | `lfd f31, 0x10(r1)` |
| `80370B98` | `36A458` | `FD FE FD 8E` / `FDFEFD8E` | X op63 XO711 FM255 FRB31 Rc0 | `mtfsf 0xff, f31` |
| `80370B9C` | `36A45C` | `CB E1 00 18` / `CBE10018` | D op50 d/s31 ra1 imm0018 | `lfd f31, 0x18(r1)` |
| `80370BA0` | `36A460` | `38 21 00 20` / `38210020` | D op14 d/s1 ra1 imm0020 | `addi r1, r1, 0x20` |
| `80370BA4` | `36A464` | `4E 80 00 20` / `4E800020` | XL op19 XO16 BT/BO20 BA/BI0 BB0 LK0 | `blr` |

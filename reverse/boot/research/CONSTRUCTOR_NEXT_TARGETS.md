# PAL constructor table: next four targets from raw DOL bytes

**Boundary.** This is a static binary and instruction audit of constructor
table indices **12–15 only**. It does not assert that the constructor walker
ran in a retail boot, that its writable table retained its initial image, or
that native C++ has reproduced these effects. The prior audit covers indices
0–11 in [ASM_CONSTRUCTORS.md](ASM_CONSTRUCTORS.md); indices 16–281 remain
**UNKNOWN here**. The target binary is PAL GUPP8P `sys/main.dol`, SHA-256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.

## Binary provenance and indirect target set

**PROVEN static image.** The DOL header maps data2 virtual
`0x804AAC60..0x804AB0DF` to file `0x4A7C60..0x4A80DF`. A fresh big-endian
read of table words 0–282 (including the first zero terminator) has SHA-256
`655d42743b66361082a98665dfc0fbad894a689407e977e288652153dba492b3`.
The walker at `0x80379700` reads one word through `r31`, compares it with
zero at `0x80379704`, and uses `mtlr r12; blrl` at `0x803796F4/F8` for each
nonzero entry. The following targets are therefore the **static** target set
for the next four iterations, conditional on normal returns and no earlier
mutation of the writable table:

| Index | Table VA / file offset | Raw BE bytes | Decoded target | Body range / file offset |
|---:|---|---|---|---|
| 12 | `0x804AAC90` / `0x4A7C90` | `80 05 55 64` | `0x80055564` | `0x80055564..0x800555AB` / `0x04EE24..0x04EE6B` |
| 13 | `0x804AAC94` / `0x4A7C94` | `80 05 6B B0` | `0x80056BB0` | `0x80056BB0..0x80056C63` / `0x050470..0x050523` |
| 14 | `0x804AAC98` / `0x4A7C98` | `80 05 8A 98` | `0x80058A98` | `0x80058A98..0x80058B2B` / `0x052358..0x0523EB` |
| 15 | `0x804AAC9C` / `0x4A7C9C` | `80 05 A3 2C` | `0x8005A32C` | `0x8005A32C..0x8005A34F` / `0x053BEC..0x053C0F` |

Text1 maps `0x80008D40..0x804AAC5F` to file `0x002600..0x4A451F`, so
`file offset = VA - 0x80008D40 + 0x2600` for each body. Each listed body
ends in `4E800020` (`blr`). Independent Capstone PPC32 big-endian decoding
of the DOL slices agreed with `q.py dis` for every one of the **109** words
(18 + 45 + 37 + 9). Each body's only control transfer is its final `blr`;
none contains an internal branch, call, CTR dispatch, or vptr store. This
conclusion applies to these bounded bodies only; the
walker itself calls through the table.

## Raw instruction words

Every row starts at the stated VA; words proceed by four bytes. These are
the original big-endian words, not recompiled output. The body SHA values
allow an independent full-slice check.

| Body start | SHA-256 of raw body bytes | Address: consecutive instruction words |
|---|---|---|
| `0x80055564` | `0818470acffeaba0e7b80be70b3c600909d654ffb44473e713e7bdc454c6dca1` | `80055564: 3C608052 C0A286B0 3863EB48 C08286B4 C04286BC C00286A8 C06286B8 C02286C0`<br>`80055584: D0A30004 D0830018 D063002C D0430040 D0230054 D0830068 D043007C D0030090`<br>`800555A4: D00300A4 4E800020` |
| `0x80056BB0` | `b257239e6413a124e831eb52c55ac0a23900e17cdf2da4dc76ee06b9be12e3bb` | `80056BB0: 9421FFE0 3C608052 3883EC30 C0E286E8 BF810010 3B840024 C0A286F0 3BA40038`<br>`80056BD0: C0C286EC 39640088 3944009C 392400B0 C08286F4 3BC4004C C06286F8 3BE40060`<br>`80056BF0: D0BC0010 390400C4 39840074 38640128 D0BD0010 38E400D8 C04286FC 38C400EC`<br>`80056C10: 38A40100 C0228700 D09E0010 38840114 C0028704 D07F0010 BB810010 D0ED2DF4`<br>`80056C30: D0CD2DF8 D06C0010 D0AB0010 D0AA0010 D0A90010 D0880010 D0470010 D0460010`<br>`80056C50: D0250010 D0040010 D0630010 38210020 4E800020` |
| `0x80058A98` | `862aac052f11f95cbf72195ec9e5d05abbf1b5272678bda0362f1908174f0fb5` | `80058A98: 9421FFF0 3C608052 3863EE10 C0828714 93E1000C 3BE30024 C0628718 39830038`<br>`80058AB8: C0428708 3963004C 39430060 39230074 39030088 38C300B0 38A300C4 C022871C`<br>`80058AD8: 38E3009C 388300D8 C0028720 386300EC D08D2E0C D08D2E14 D06D2E18 D05F0010`<br>`80058AF8: 83E1000C D04C0010 D04B0010 D04A0010 D0490010 D0480010 D0270010 D0460010`<br>`80058B18: D0450010 D0040010 D0030010 38210010 4E800020` |
| `0x8005A32C` | `21873659e68e3e9a2bbdb581a96cd04f0f8550d47d2e69e0a07aec917800c891` | `8005A32C: C0428740 3C608052 C0228744 3863EFD0 C0028734 D04D2E34 D02D2E38 D0030010`<br>`8005A34C: 4E800020` |

All four bodies use `r2` for the float source words. The preceding register
helper established `r2=0x805FA780` and `r13=0x805EC500`; the effective
addresses below are conditional on those values surviving the intervening
boot path. The original DOL has the following normal finite binary32 source
words in data7 (VA `0x805F2780`, file `0x577920`):

| Target | Source VA → raw word (decimal value) |
|---|---|
| 12 | `2E28→00000000 (0)`, `2E30→41F00000 (30)`, `2E34→42480000 (50)`, `2E38→42C80000 (100)`, `2E3C→42200000 (40)`, `2E40→428C0000 (70)` |
| 13 | `2E68→43FA0000 (500)`, `2E6C→43960000 (300)`, `2E70→437A0000 (250)`, `2E74→43C80000 (400)`, `2E78→44160000 (600)`, `2E7C→44FA0000 (2000)`, `2E80→42C80000 (100)`, `2E84→43480000 (200)` |
| 14 | `2E88→43960000 (300)`, `2E94→43160000 (150)`, `2E98→43FA0000 (500)`, `2E9C→44FA0000 (2000)`, `2EA0→44BB8000 (1500)` |
| 15 | `2EB4→43C80000 (400)`, `2EC0→42C80000 (100)`, `2EC4→41A00000 (20)` |

The abbreviated source addresses in this table have prefix `0x805F`.
**PROVEN** DOL source words do not by themselves prove their contents at
constructor entry; any intervening write to data7 is **UNKNOWN** without a
same-run memory comparison. `lfs`/`stfs` carry each loaded value to a
destination without arithmetic. This does not claim full FPR/FPSCR parity.

## Instruction-level state and ordered memory effects

**Index 12, `0x80055564..0x800555A8` (18 words).** `lis` at `0x55564`
and `addi` at `0x5556C` make `r3=0x8051EB48`. The six `lfs` instructions
at `0x55568/70/74/78/7C/80` read respectively `r2-0x7950`, `-0x794C`,
`-0x7944`, `-0x7958`, `-0x7948`, `-0x7940` into f5/f4/f2/f0/f3/f1.
Stores execute in the following exact order:

| PPC store | Destination | Source FPR / DOL source VA |
|---|---|---|
| `0x80055584` | `0x8051EB4C` | f5 / `0x805F2E30` |
| `0x80055588` | `0x8051EB60` | f4 / `0x805F2E34` |
| `0x8005558C` | `0x8051EB74` | f3 / `0x805F2E38` |
| `0x80055590` | `0x8051EB88` | f2 / `0x805F2E3C` |
| `0x80055594` | `0x8051EB9C` | f1 / `0x805F2E40` |
| `0x80055598` | `0x8051EBB0` | f4 / `0x805F2E34` |
| `0x8005559C` | `0x8051EBC4` | f2 / `0x805F2E3C` |
| `0x800555A0` | `0x8051EBD8` | f0 / `0x805F2E28` |
| `0x800555A4` | `0x8051EBEC` | f0 / `0x805F2E28` |

`blr` at `0x800555A8` returns through LR. No stack, CR, XER, CTR or SPR
instruction occurs here. The symbol `CannonBombTable_StaticInit` is a
recovered label, **STRONG** semantic context, not proof of an original class.

**Index 13, `0x80056BB0..0x80056C60` (45 words).** `stwu` at `0x56BB0`
sets `r1=old_r1-0x20` and stores the old SP at the new frame base;
`stmw r28,0x10(r1)` at `0x56BC0` saves r28–r31 to new `r1+0x10..1C`.
`lis/addi` at `0x56BB4/B8` makes `r4=0x8051EC30`. Address-forming
`addi` instructions at `0x56BC4/BCC/BD4/BD8/BDC/BE4/BEC/BF4/BF8/BFC/C04/C0C/C10/C1C`
derive the destination bases; `0x56C28` restores r28–r31 before the final
stores, which use other registers. The eight `lfs` at
`0x56BBC/BC8/BD0/BE0/BE8/C08/C14/C20` load f7/f5/f6/f4/f3/f2/f1/f0
from source VAs `0x805F2E68/70/6C/74/78/7C/80/84`. Every `stfs` is
accounted for in instruction order:

| PPC store(s) | Destination(s), same order | Source |
|---|---|---|
| `0x56BF0`, `0x56C00` | `0x8051EC64`, `0x8051EC78` | f5 / `0x805F2E70` |
| `0x56C18`, `0x56C24` | `0x8051EC8C`, `0x8051ECA0` | f4 / `0x805F2E74`; f3 / `0x805F2E78` |
| `0x56C2C`, `0x56C30` | `0x805EF2F4`, `0x805EF2F8` | f7 / `0x805F2E68`; f6 / `0x805F2E6C` |
| `0x56C34`, `0x56C38`, `0x56C3C`, `0x56C40` | `0x8051ECB4`, `0x8051ECC8`, `0x8051ECDC`, `0x8051ECF0` | f3, f5, f5, f5 |
| `0x56C44`, `0x56C48`, `0x56C4C`, `0x56C50` | `0x8051ED04`, `0x8051ED18`, `0x8051ED2C`, `0x8051ED40` | f4, f2, f2, f1 |
| `0x56C54`, `0x56C58` | `0x8051ED54`, `0x8051ED68` | f0, f3 |

`addi r1,r1,0x20` at `0x56C5C` restores SP; `blr` at `0x56C60`
returns with LR unchanged. No CR, XER, CTR or SPR instruction. The
`BulletGunTable_StaticInit` label is recovered semantic context only.

**Index 14, `0x80058A98..0x80058B28` (37 words).** `stwu` creates a
16-byte frame at `0x58A98`; `stw` saves r31 at new `r1+0xC` at
`0x58AA8`, and `lwz` restores it at `0x58AF8`. `lis/addi` at
`0x58A9C/AA0` makes `r3=0x8051EE10`; the `addi` sequence at
`0x58AAC/AB4/ABC/AC0/AC4/AC8/ACC/AD0/AD8/ADC/AE4` derives the
destination bases. Five `lfs` at `0x58AA4/AB0/AB8/AD4/AE0` read source
VAs `0x805F2E94/98/88/9C/A0` into f4/f3/f2/f1/f0.

| PPC store(s) | Destination(s), same order | Source |
|---|---|---|
| `0x58AE8`, `0x58AEC`, `0x58AF0` | `0x805EF30C`, `0x805EF314`, `0x805EF318` | f4, f4, f3 |
| `0x58AF4` | `0x8051EE44` | f2 |
| `0x58AFC`, `0x58B00`, `0x58B04`, `0x58B08`, `0x58B0C` | `0x8051EE58`, `0x8051EE6C`, `0x8051EE80`, `0x8051EE94`, `0x8051EEA8` | f2 each |
| `0x58B10` | `0x8051EEBC` | f1 |
| `0x58B14`, `0x58B18` | `0x8051EED0`, `0x8051EEE4` | f2 each |
| `0x58B1C`, `0x58B20` | `0x8051EEF8`, `0x8051EF0C` | f0 each |

`addi` at `0x58B24` restores SP, then `blr` returns at `0x58B28`.
No CR, XER, CTR or SPR instruction occurs. `BulletParams_StaticInit_Homing`
is a recovered semantic label.

**Index 15, `0x8005A32C..0x8005A34C` (9 words).** `lfs` at
`0x5A32C/334/33C` loads f2/f1/f0 from `r2-0x78C0/-0x78BC/-0x78CC`
(`0x805F2EC0/C4/B4`). `lis/addi` at `0x5A330/338` makes
`r3=0x8051EFD0`. `stfs` at `0x5A340/344/348` writes
`0x805EF334←f2`, `0x805EF338←f1`, `0x8051EFE0←f0` in that
order, each four bytes big-endian. `blr` at `0x5A34C` returns. No stack,
CR, XER, CTR or SPR instruction occurs. `RingTable_StaticInit` is a
recovered semantic label.

Across all four bodies, no store writes an object pointer, vptr, or
constructor-table slot. No allocation or nested call is present. **PROVEN**
raw local effects and addresses therefore narrow these four callbacks to
parameter writes, but final values remain conditional on actual `r2/r13`,
source memory, FPR handling, and normal completion. There is no basis here
for inventing object layouts or claiming a validated native equivalent.

The register exit contract also matters for the walker. Index 12 leaves
`r3=0x8051EB48` and loads the scalar side of f0–f5 from the listed sources. Index 13
restores r28–r31 and r1, leaves
`r3=0x8051ED58`, `r4=0x8051ED44`, and loads scalar f0–f7. Index 14 restores
r31 and r1, leaves `r3=0x8051EEFC`, `r4=0x8051EEE8`, and loads scalar f0–f4.
Index 15 leaves `r3=0x8051EFD0` and loads scalar f0–f2. The exact paired
PS1 state after those scalar loads is **UNKNOWN** here. All four preserve LR
and r2/r13, and have no explicit CR/XER/CTR/MSR/FPSCR register access
instruction; possible floating status effects still need runtime validation.
Other address-forming GPR outputs are visible in the decoded word lists and
destination tables; later callback entry GPR/FPR values depend on the
walker's and earlier callbacks' effects, not on an assumed clean ABI state.

## Validation boundary and next unknown

1. **Raw/decode:** DOL SHA, header mapping, table hash, four pointer words,
   109 instruction words, body SHA values, and independent Capstone decode
   were checked against the read-only executable. The PPC `lis/addi` and
   signed displacement arithmetic explains every effective address above.
2. **Data flow:** each non-stack store has a traced FPR source; no arithmetic
   or hidden call appears in these four slices. Store widths and order are
   explicit. Stack saves/restores for indices 13 and 14 are included.
3. **Unvalidated execution:** no same-run constructor entry/exit trace was
   captured. Runtime source-memory integrity, exact FPR/FPSCR state, and the
   walker table image when it reaches index 12 remain **UNKNOWN**. These
   bodies are not promoted to native C++ or constructor parity.
4. **Next frontier:** table index 16 is initially `0x8005C75C` at
   `0x804AACA0` (file `0x4A7CA0`). Its body and all later entries were
   deliberately not traced in this bounded audit.

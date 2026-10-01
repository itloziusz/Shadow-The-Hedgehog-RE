# `sync` / GQR boot tail from the PAL DOL

**Continuation:** checkpoint 37 `NATIVE_SYNC_COMPLETION_37.md` connects this
tail after the bounded immutable native completion and reaches `80372894`.
The original request-only profile and physical/retail paths remain separate.

Scope: the fourteen reached words at `0x80371730..0x80371764` after the
HID0 ICFI request. **PROVEN-BINARY** below means a direct read of the
read-only PAL GUPP8P DOL (SHA-256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`)
plus independently decoded instruction fields. **PROVEN-ISA** means the
[IBM Gekko User's Manual v1.2](https://doc.kodewerx.org/documents/gekko_user_manual.pdf),
particularly `sync` p. 12-239 and GQRs pp. 2-23–24. **OBSERVED-HLE** is a
synthetic startup-only Dolphin interpreter capture, not retail IPL or silicon
proof. The connected native C++ prefix stops *before* `0x80371730`.

## Raw inventory and CFG

The DOL header maps text section 1 from file `0x002600` to
VA `0x80008D40`; every row uses `file = 0x2600 + VA - 0x80008D40`.
The big-endian words were independently extracted from the DOL before
comparison with `BINARY_BOOT_PREFIX.md` and `PAIRED_SETUP_STATE.md`.
For `mtspr`, `XO=(word>>1)&1023`, source GPR is `(word>>21)&31`, and the
encoded SPR halves recombine as
`((word>>16)&31) | (((word>>11)&31)<<5)`.
The apparent `rA` field in this X-form encoding is part of the split SPR,
not a GPR source.

| VA | File offset | Raw bytes / BE word | Independent decode and direct effect |
|---|---:|---|---|
| `80371730` | `36AFF0` | `7C 00 04 AC` / `7C0004AC` | op31, XO598, all reserved fields zero: `sync`; completion/ordering point, no architectural register result. |
| `80371734` | `36AFF4` | `38 60 00 00` / `38600000` | op14, rD3, rA0, signed imm0: `li r3,0`. |
| `80371738` | `36AFF8` | `7C 70 E3 A6` / `7C70E3A6` | op31 XO467, rS3, SPR912: `mtspr GQR0,r3`; write zero. |
| `8037173C` | `36AFFC` | `7C 71 E3 A6` / `7C71E3A6` | op31 XO467, rS3, SPR913: `mtspr GQR1,r3`; write zero. |
| `80371740` | `36B000` | `7C 72 E3 A6` / `7C72E3A6` | op31 XO467, rS3, SPR914: `mtspr GQR2,r3`; write zero. |
| `80371744` | `36B004` | `7C 73 E3 A6` / `7C73E3A6` | op31 XO467, rS3, SPR915: `mtspr GQR3,r3`; write zero. |
| `80371748` | `36B008` | `7C 74 E3 A6` / `7C74E3A6` | op31 XO467, rS3, SPR916: `mtspr GQR4,r3`; write zero. |
| `8037174C` | `36B00C` | `7C 75 E3 A6` / `7C75E3A6` | op31 XO467, rS3, SPR917: `mtspr GQR5,r3`; write zero. |
| `80371750` | `36B010` | `7C 76 E3 A6` / `7C76E3A6` | op31 XO467, rS3, SPR918: `mtspr GQR6,r3`; write zero. |
| `80371754` | `36B014` | `7C 77 E3 A6` / `7C77E3A6` | op31 XO467, rS3, SPR919: `mtspr GQR7,r3`; write zero. |
| `80371758` | `36B018` | `80 01 00 0C` / `8001000C` | op32, rD0, rA1, signed imm12: `lwz r0,12(r1)`; BE32 read of the earlier saved LR. |
| `8037175C` | `36B01C` | `38 21 00 08` / `38210008` | op14, rD1, rA1, signed imm8: `addi r1,r1,8`. |
| `80371760` | `36B020` | `7C 08 03 A6` / `7C0803A6` | op31 XO467, rS0, SPR8: `mtlr r0`. |
| `80371764` | `36B024` | `4E 80 00 20` / `4E800020` | op19 XO16 BO20 BI0 LK0: unconditional `blr` to the restored LR. |

There is no local conditional branch, CTR dispatch, FPR operation, data
store, SDA/SDA2 access, or call in this tail. The only control-flow successor
on ordinary execution is the LR value loaded from stack. It is **not** valid
to replace `lwz` with the previously saved LR constant: a write or exception
between save and load could alter those four bytes. The preceding direct
call at `0x8037172C` sets LR=`0x80371730`; the enclosing call at
`0x80003410` supplied LR=`0x80003414` before the tail's prologue saved it.

## Machine-state transition

Let entry SP be `S`, saved return word at `S+4` be `M`, and incoming GQRs
and r3 be arbitrary. The earlier `stwu` made r1=`S-8`; on this path
`S=0x8060C5F0`, r1=`0x8060C5E8`, and the saved LR bytes at
`0x8060C5F4..F7` were written as BE32(`0x80003414`). The tail has the
following ordered state transformation, conditional on successful privileged
SPR access, mapped stack memory and no interrupt/exception diversion:

1. `sync` completes the preceding instruction effects before allowing the
   following instructions to initiate. The manual specifies an ordering
   function and no altered registers. It does **not** expose cache tags or
   prove retail ICFI completion timing from this trace alone.
2. r3 becomes zero. Eight separate privileged writes set GQR0 through GQR7
   to zero in program order. Their incoming contents are irrelevant to this
   post-write architectural value, but can still matter to earlier code or
   exceptional paths. The writes are not a RAM `memset`.
3. r0 becomes `load_be32(S+4)` (`M`); r1 returns to `S`; LR becomes `M`;
   `blr` transfers PC to `M`. On the uninterrupted documented path,
   `M=0x80003414`. The stack bytes remain written after SP restoration.

CR, XER, CTR, MSR, FPSCR, FPRs and r2/r4..r31 have no direct write in these
fourteen words. `sync` has no GPR/CR/FPSCR result. The `mtspr` instructions
do not consume r0 or a second GPR despite the split SPR fields occupying
bits that resemble register fields. Actual exceptional behavior and hardware
acceptance are not inferred from the DOL alone.

## First downstream consumers

`0x80003414` (text section 0 file `0x414`, word `4836D8C9`) calls
`0x80370CDC`, making LR=`0x80003418`. In that callee the first
GQR-dependent reached instruction is `0x80370CFC` (file `0x36A5BC`,
word `E0030000`, `psq_l f0,0(r3),W=0,I=0`) if HID2[PSE] selects the
paired path at `0x80370CF0` and FP/LSQE permit it. Its I field selects
GQR0, whose zero load-type/scale makes the two source 32-bit words
single-precision operands. Source address `0x805F1F38` follows from
`0x80370CF4..F8`; actual retail bytes and both resulting FPR lanes
remain **UNKNOWN**. The skipped PSE edge reaches `0x80370D7C` without
this `psq_l`. The `ps_mr`/`lfd`/`fmr` lane caveats are detailed in
`FPR_LANE_PROVENANCE.md`; this tail alone does not validate them.

A later context routine contains eight consecutive `mfspr` words at
`0x8039F644..0x8039F660` (file `0x398F04..0x398F20`) reading SPR
912..919 into r20..r27. This is a **static candidate** readback of all
GQRs; normal boot reachability and intervening writes require a separate
CFG proof. It does not establish that the initial eight zeroes persist to
that point.

## Reference observation and adversarial limits

The existing same-run synthetic capture
`build/agent-boot-hardware-capture.json` exposes:

| PC before instruction | r0 | r1 | r3 | LR | CR | XER | HID0 readback |
|---|---:|---:|---:|---:|---:|---:|---:|
| `80371730` | `80003414` | `8060C5E8` | `0011CC64` | `80371730` | `20000000` | `00000000` | `0011C464` |
| `80371758` | `80003414` | `8060C5E8` | `00000000` | `80371730` | `20000000` | `00000000` | `0011C464` |
| `80370CDC` | `80003414` | `8060C5F0` | `00000000` | `80003418` | `20000000` | `00000000` | `0011C464` |

The last PC implies the `blr` returned to `0x80003414` and that call
advanced to `0x80370CDC`. The exposed r0/r1/r3/LR agree with the symbolic
trace. The debugger did **not** expose GQR0..7, cache tags, full paired FPR
lanes, or physical instruction-cache visibility. The HID0 command operand
in r3 differs from its readback because ICFI can self-clear; neither number
is a retail IPL measurement. The capture supports reachable control and
GPR state, **not** GQR readback parity.

Adversarial checks that a native lowering must survive:

- Treat each GQR write as an individual ordered effect; the eight distinct
  SPR fields refute a guessed single flag or persistent host constant.
- Vary incoming r3 and all eight incoming GQRs: on the uninterrupted path
  r3 and post-write GQRs must still be zero, while CR/XER/CTR/FPSCR remain
  unchanged by this tail.
- Mutate saved BE32(`S+4`) before `lwz`: the return target must follow the
  memory word or fail closed if it is invalid. Do not hard-code
  `0x80003414` into the `blr` translation.
- Keep the HID0 request, `sync` boundary and GQR writes separate. Passing
  a GPR checkpoint cannot show that ICFI or GQR hardware effects occurred.

## Lawful portable boundary

A portable semantic projection can issue a platform ordering/completion
operation, record eight zero GQR writes, load the saved return address from
guest stack memory in big-endian order, restore SP/LR, and return. This is a
*proposed* C++ boundary, not implemented parity. It must use the existing
explicit platform state and stack-write memory model; a no-op `sync` or
hard-coded LR would discard required dependencies. Then validate the exact
native state at `0x80371758` and `0x80370CDC` from a full rerun starting
`0x80003154`, test alternate incoming GQR/r3 and mutated stack inputs, and
pass all earlier CTest gates. Before claiming the following FPR section,
obtain same-run GQR0 readback or independently establish its architectural
write effect together with PS0/PS1 and source-memory state. Retail IPL
inputs and ICFI/cache effects remain **UNKNOWN**.

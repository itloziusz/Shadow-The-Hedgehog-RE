# Paired-setup machine-state transition from PAL DOL bytes

Scope: the connected call at `0x80371714..0x80371764` and **only** its
three direct leaves at `0x80370BA8..0x80370BB4` and
`0x803725F4..0x80372600`. Source: read-only PAL GUPP8P `main.dol`,
SHA-256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
I independently read the DOL section header, located each virtual address in
the section mapped from file `0x2600` to VA `0x80008D40`, read each four-byte
word in big-endian order, and decoded its fields before comparing existing
assembly notes. `PROVEN` means bytes plus ISA effects, `OBSERVED-HLE` means the
separate synthetic startup-only Dolphin interpreter capture, and `UNKNOWN`
means no qualifying measurement or original IPL evidence. This is a state
proof, **not** a native implementation or retail-hardware validation.

## Byte-level inventory

The branch target is computed from sign-extended LI displacement with AA=0;
each of the three calls has LK=1. For X-form SPR instructions, decode the
split field as `SPR=((word>>16)&31)|(((word>>11)&31)<<5)`; this yields LR=8,
GQR0..7=912..919, HID2=920, and HID0=1008. `XO` is bits 1..10, `Rc` bit 0.
The `stw`/`stwu` immediates are signed 16-bit values; all displayed store
widths are 32 bits. The arrows below indicate encoded direct branch targets,
not a guessed callee.

| VA | File offset | Raw bytes / BE word | Decode and exact local effect |
|---|---:|---|---|
| `80371714` | `36AFD4` | `7C 08 02 A6` / `7C0802A6` | op31 XO339 SPR8: `mflr r0`; r0=entry LR. |
| `80371718` | `36AFD8` | `90 01 00 04` / `90010004` | op36: `stw r0,4(r1)`; BE32 store at entry SP+4. |
| `8037171C` | `36AFDC` | `94 21 FF F8` / `9421FFF8` | op37: `stwu r1,-8(r1)`; write old SP at old SP−8, then r1=old SP−8. |
| `80371720` | `36AFE0` | `4B FF F4 89` / `4BFFF489` | op18 LI=`0xFFFD22`, AA0 LK1: `bl` → `bl 80370BA8`; LR=`0x80371724`. |
| `80371724` | `36AFE4` | `64 63 A0 00` / `6463A000` | op25: `oris r3,r3,0xA000`; r3=HID2 readback OR `0xA0000000`. |
| `80371728` | `36AFE8` | `4B FF F4 89` / `4BFFF489` | op18 LI=`0xFFFD22`, AA0 LK1: `bl` → `bl 80370BB0`; LR=`0x8037172C`. |
| `8037172C` | `36AFEC` | `48 00 0E C9` / `48000EC9` | op18 LI=`0x0003B2`, AA0 LK1: `bl` → `bl 803725F4`; LR=`0x80371730`. |
| `80371730` | `36AFF0` | `7C 00 04 AC` / `7C0004AC` | op31 XO598 Rc0: `sync`; ordering/completion barrier, no integer result. |
| `80371734` | `36AFF4` | `38 60 00 00` / `38600000` | op14 rD3 rA0 signed imm0: `li r3,0`. |
| `80371738` | `36AFF8` | `7C 70 E3 A6` / `7C70E3A6` | op31 XO467 SPR912: `mtspr gqr0,r3`; GQR0=0. |
| `8037173C` | `36AFFC` | `7C 71 E3 A6` / `7C71E3A6` | op31 XO467 SPR913: `mtspr gqr1,r3`; GQR1=0. |
| `80371740` | `36B000` | `7C 72 E3 A6` / `7C72E3A6` | op31 XO467 SPR914: `mtspr gqr2,r3`; GQR2=0. |
| `80371744` | `36B004` | `7C 73 E3 A6` / `7C73E3A6` | op31 XO467 SPR915: `mtspr gqr3,r3`; GQR3=0. |
| `80371748` | `36B008` | `7C 74 E3 A6` / `7C74E3A6` | op31 XO467 SPR916: `mtspr gqr4,r3`; GQR4=0. |
| `8037174C` | `36B00C` | `7C 75 E3 A6` / `7C75E3A6` | op31 XO467 SPR917: `mtspr gqr5,r3`; GQR5=0. |
| `80371750` | `36B010` | `7C 76 E3 A6` / `7C76E3A6` | op31 XO467 SPR918: `mtspr gqr6,r3`; GQR6=0. |
| `80371754` | `36B014` | `7C 77 E3 A6` / `7C77E3A6` | op31 XO467 SPR919: `mtspr gqr7,r3`; GQR7=0. |
| `80371758` | `36B018` | `80 01 00 0C` / `8001000C` | op32: `lwz r0,12(r1)`; BE32 load from old SP+4. |
| `8037175C` | `36B01C` | `38 21 00 08` / `38210008` | op14: `addi r1,r1,8`; restore old SP. |
| `80371760` | `36B020` | `7C 08 03 A6` / `7C0803A6` | op31 XO467 SPR8: `mtlr r0`; restore entry LR. |
| `80371764` | `36B024` | `4E 80 00 20` / `4E800020` | op19 XO16 BO20 BI0 LK0: `blr`; PC=restored LR. |
| `80370BA8` | `36A468` | `7C 78 E2 A6` / `7C78E2A6` | op31 XO339 SPR920: `mfspr r3,hid2`; r3=live HID2 readback. |
| `80370BAC` | `36A46C` | `4E 80 00 20` / `4E800020` | op19 XO16 BO20 BI0 LK0: `blr`; PC=`0x80371724`. |
| `80370BB0` | `36A470` | `7C 78 E3 A6` / `7C78E3A6` | op31 XO467 SPR920: `mtspr hid2,r3`; write computed HID2 word. |
| `80370BB4` | `36A474` | `4E 80 00 20` / `4E800020` | op19 XO16 BO20 BI0 LK0: `blr`; PC=`0x8037172C`. |
| `803725F4` | `36BEB4` | `7C 70 FA A6` / `7C70FAA6` | op31 XO339 SPR1008: `mfspr r3,hid0`; r3=live HID0 readback. |
| `803725F8` | `36BEB8` | `60 63 08 00` / `60630800` | op24: `ori r3,r3,0x0800`; r3=HID0 readback OR `0x00000800`. |
| `803725FC` | `36BEBC` | `7C 70 FB A6` / `7C70FBA6` | op31 XO467 SPR1008: `mtspr hid0,r3`; issue ICFI command word. |
| `80372600` | `36BEC0` | `4E 80 00 20` / `4E800020` | op19 XO16 BO20 BI0 LK0: `blr`; PC=`0x80371730`. |

The three `bl` targets and every SPR number were calculated from the words
above, then cross-checked against `BINARY_BOOT_PREFIX.md`. There is no local
conditional branch, `bctr`, pointer-loaded call, loop, FPR instruction, or
ordinary data read beyond the saved LR stack load. The adjoining HID1 setter
at `0x80370BB8` and `isync` routine at `0x80372604` are **not** called here.

## Ordered state derivation

Let `S` be entry r1, `L` be entry LR, `H2` the value actually returned by
the HID2 read, and `H0` the value actually returned by the HID0 read. These
are inputs, not values inferred from a function name. The connected caller
has `S=0x8060C5F0` from its earlier constant-setting helper and
`L=0x80003414` from `bl 0x80371714`. In the broader boot, their provenance
must remain attached to that caller, not assumed as an ABI convention.

| Order | PC / operation | Resulting state or memory effect | Downstream consumer |
|---:|---|---|---|
| 1 | `0x80371714..1C` | r0=L; `[S+4,S+7]`=BE32(L); `[S−8,S−5]`=BE32(S); r1=S−8. The second store does **not** overlap the first. | `lwz` at `0x80371758`; backchain is stack-visible. |
| 2 | `0x80371720`, HID2 leaf, `0x80371724`, write leaf | LR changes to `0x80371724`, r3=H2, returns with LR unchanged; r3=H2 OR `0xA0000000`; LR changes to `0x8037172C`; that word is written to HID2. | `mfspr hid2` at `0x80370CE8`, then `rlwinm.`/`beq` in the next wrapper callee. |
| 3 | `0x8037172C`, HID0 leaf | LR changes to `0x80371730`; r3=H0 OR `0x00000800`; `mtspr hid0` issues that command word. | Cache-state consequence; later live HID0 reads at `0x80372854` and `0x80372874`. |
| 4 | `0x80371730..54` | `sync`; r3=0; eight ordered SPR writes make GQR0..7 zero. | GQR0 controls `psq_l` at `0x80370CFC`; other GQRs have later candidate readers. |
| 5 | `0x80371758..64` | r0=BE32(`[S+4,S+7]`)=L; r1=S; LR=L; PC=L. Stack bytes written in order 1 remain overwritten. | Wrapper resumes at `0x80003414`. |

For the connected `S`, the two written ranges are `0x8060C5F4..F7`
(saved LR) and `0x8060C5E8..EB` (backchain). A writable, correctly mapped
stack is a precondition. If a nested call or asynchronous handler changes
either saved-LR bytes, the final LR need not equal `L`; the ordinary
straight-line proof assumes no such intervening mutation. The leaf bodies
contain no RAM access and no explicit exception handler call.

All listed arithmetic and OR instructions have `Rc=0` or D-form semantics
without a record bit. `mflr`, `mtlr`, `mfspr`, `mtspr`, `sync`, loads/stores,
and unconditioned `blr` do not update CR or XER in this straight-line path.
CTR, FPRs, paired lanes, FPSCR, r2, r4..r31, and MSR receive no direct write
inside this region. r0 ends as L; r3 ends as zero. The input GPRs r0/r3
are otherwise irrelevant to those outputs. The GQR writes are eight
distinct ordered effects, not a single inferred reset. `sync` is an
architectural ordering operation, not an ordinary GPR or RAM assignment.

## Architectural and hardware boundary

`PROVEN` from the Gekko manual and encoding: HID2 mask `0xA0000000` forces
LSQE and PSE, the paired/quantized instruction enable prerequisites. It
does not prove all other HID2 bits or the initial HID2 readback. A later
`rlwinm. r3,r3,3,31,31` at `0x80370CEC` tests PSE by **rotate**, so a
successful HID2 write/read makes its following `beq` false. This statement
is conditional on the architectural SPR write being accepted and retained.

HID0 bit `0x00000800` is **ICFI**, a cache invalidation action, rather than
a persistent software flag. The [IBM Gekko User's Manual, Table 2-4 and
§3.4.1.4](https://doc.kodewerx.org/documents/gekko_user_manual.pdf)
specifies self-clear after an enabled instruction-cache invalidate. Thus
`H0|0x800` is the **issued write value**, not a proved subsequent HID0
readback. Cache tags, completion timing, instruction visibility, and any
interrupt interaction are outside the DOL words and require hardware or
reference-state evidence. The `sync` proves an ordering point but does not
turn those unmeasured internals into known values. GQR writes produce zero
contents under the architectural SPR semantics; this capture did not read
them back directly.

`OBSERVED-HLE`: the synthetic startup-only Dolphin interpreter supplied
MSR=`0x2032`; the first HID2 `mfspr` yielded r3=`0xE0000000`, and the OR
still yielded `0xE0000000`. HID0 before was `0x0011C464`, giving an issued
ICFI word `0x0011CC64`; a later live HID0 read returned `0x0011C464`.
The synthetic r1 transition was `0x8060C5F0 -> 0x8060C5E8 ->
0x8060C5F0`. These observations agree with the instruction-derived path
but are **not** retail IPL initial values or a measurement of cache tags.
The GDB interface did not expose GQRs or HID2 directly; HID2 was inferred
from r3 immediately after `mfspr`. Its precise hardware readback under
other incoming states is `UNKNOWN`.

## Validation and fail-closed frontier

- **Raw decode:** 29 listed instruction words map to the pinned DOL bytes;
  independently decoded three relative `bl` targets and 14 SPR operands.
  `verify_binary_note.py` rechecks the complete table against DOL and
  declines an altered hash or changed bytes. This is the byte and CFG gate.
- **State:** the symbolic ordered trace explains the two stack stores,
  three LR changes, two HID write operands, one barrier, eight GQR writes,
  and final r0/r1/r3/LR. It makes no assumption about initial HID words.
- **Adversarial checks:** treating `stwu` as only an r1 decrement loses its
  backchain write; retaining ICFI in later HID0 would contradict the enabled
  cache case; replacing the PSE rotate with a shift fails the observed
  `0xE0000000` input. No conditional branch can be selected inside this
  local routine, but the next callee's PSE branch depends on the live read.
- **Native/downstream:** a later bounded native probe passed the first four
  words' register/ordered-write comparison and the two-word HID2 read
  accessor with a caller-supplied HLE value. It stops before `0x80371724`;
  no HID2 write, GQR/cache readback or full paired-setup parity has passed.
  See `BOOT_RUN_DIAGNOSTIC_2026-09-30.md` for that separate execution gate.

Remaining `UNKNOWN`: retail IPL HID2/HID0/GQR initial states; precise
ICFI completion and readback under other ICE states; asynchronous exception
possibilities; direct GQR readback in the oracle; and whether all later
candidate readers execute on the intended game path. These unknowns do
not alter the decoded local words, but they block declaring full hardware
or native parity.

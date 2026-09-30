# PAL GUPP8P CRT memory execution audit

Scope: the DOL path `0x8000315C..0x8000316C`, its table walker
`0x80003340..0x800033FC`, reached fill wrapper and leaf
`0x8000540C..0x800054F0`, and descriptor words `0x80005544..0x800055E4`.
This is an ordered machine-state ledger for the **conditional** path after the
hardware call returns. It is not a connected native implementation. The
current native probe stops before `0x80371724`, far earlier than this CRT
boundary. `PROVEN` below means a consequence of the SHA-pinned DOL bytes and
stated inputs; `OBSERVED-HLE` means a startup-only synthetic Dolphin run;
`UNKNOWN` remains an input or effect without that proof. Neither label
establishes a retail IPL handoff.

The read-only PAL `main.dol` SHA-256 is
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
Text0 maps VA `0x80003100` to file offset `0x000100`. I independently read
the header, descriptor words and selected instructions as big-endian 32-bit
words. The raw-record checker passed all 111 required instruction rows, 25
direct branches and 41 descriptor words:

```text
python reverse/boot/tools/verify_binary_note.py <read-only-main.dol> \
  reverse/boot/research/BINARY_CRT_PREFIX.md \
  --expected-words 111 --expected-branches 25 --expected-sprs 0 \
  --expected-data-words 41 \
  --required-code-range 8000315C:8000316C \
  --required-code-range 80003340:800033FC \
  --required-code-range 8000540C:800054F0 \
  --required-data-range 80005544:800055E4
PASS raw words=111 branches=25 SPRs=0 data=41
```

The table in `BINARY_CRT_PREFIX.md` records every raw instruction byte.
Selected anchors are `0x80003160=0x9421FFF8` (`stwu r1,-8(r1)`),
`0x8000336C=0x83DD0008` (copy length load),
`0x80003384=0x7C1F2040` (destination/source compare),
`0x800033C0=0x80BD0004` (zero length load),
`0x800054B8=0x94E40020` (last updating store in an eight-word group), and
`0x800054CC=0x94E40004` (updating remaining-word store). These word values
were reread directly from the DOL, not adopted from the disassembler.

## Entry boundary and exact control flow

The entry register helper supplies `r1=0x8060C5F0`, `r2=0x805FA780`, and
`r13=0x805EC500`; passage through the intervening hardware call is a
separate prerequisite. **OBSERVED-HLE:** the fresh synthetic reference
capture `build/crt_return_capture_20260930.json` has PC `0x8000315C`,
`r1=0x8060C5F0`, `r31=0x8000315C`, LR `0x8000315C`, and CR
`0x20000000`. These are scenario observations, not DOL constants or native
state validations. The `r31` value has a byte-level producer: the hardware
wrapper's `0x8000340C` `mflr r31` captures outer LR `0x8000315C`, and
`0x8000341C` later returns through that saved value.

```text
8000315C..316C -> 80003340..3374
    copy size != 0 -> read src/dst -> src == dst -> 33A4 -> 336C  [10]
    copy size == 0 -> 33AC..33C8
    zero size != 0 -> 33CC..33D8 -> 8000540C -> 8000543C
                   -> 80005424..38 -> 33DC -> 33C0       [3]
    zero size == 0 -> 33E4..33FC -> 80003170
```

**PROVEN:** each copy descriptor is `{source,destination,size}`. The size
alone terminates its loop. The second `beq` at `0x80003380` reuses the
nonzero size comparison because the intervening loads leave CR0 alone.
The `cmplw` at `0x80003384` finds equal addresses for all ten nonzero PAL
descriptors, so their `memmove` and `0x80003424` cache edges are not taken
for these bytes. The zero loop likewise terminates on size alone and its
second `beq` at `0x800033D0` reuses the size comparison. It makes exactly
three direct calls to `0x8000540C`; there is no indirect call, FPR access,
SDA access or MMIO in this reached CRT region. An intervening runtime
writer to the descriptor table is **UNKNOWN**; the conclusion about the
selected path assumes the table still contains its loaded DOL words.

## Descriptor and destination proof

The three raw zero descriptors, as `(destination,size)`, are
`(0x8056FE00,0x74700)`, `(0x805EF020,0x375C)`, and
`(0x805FC540,0xAC)`, followed by `(0,0)`. On their reached path, the fill
byte in `r4` is zero and all addresses and lengths are multiples of four.
The leaf skips leading-byte, nonzero-byte replication and trailing-byte
stores. `0x8000548C` computes `size >> 5` groups, with eight consecutive
big-endian zero-word stores per group; `0x800054C0` computes the remaining
`(size >> 2) & 7` words. The exact fill effects are:

| Call order | Zero interval, exclusive end | 32-byte groups | Remaining words | Last word address | `r6` after fill |
|---|---|---:|---:|---:|---:|
| 1 | `[0x8056FE00,0x805E4500)` | `0x3A38` | 0 | `0x805E44FC` | `0x805E44FF` |
| 2 | `[0x805EF020,0x805F277C)` | `0x01BA` | 7 | `0x805F2778` | `0x805F277B` |
| 3 | `[0x805FC540,0x805FC5EC)` | 5 | 3 | `0x805FC5E8` | `0x805FC5EB` |

**PROVEN:** `0x80586CB4`, the earlier hardware handler slot on its selected
path, lies in call 1 and is cleared. The FPR seed source
`0x805F1F30..0x805F1F3F` lies in call 2 and is cleared **after** the
hardware FPR loads. The word interval `0x805F277C..0x805F277F` is not
covered by these zero calls; its pre-CRT value cannot be inferred from the
table. Call 3 stops at `0x805FC5EC`, before the CRT stack at
`0x8060C5C0..0x8060C5F4`.

The copy table size is **not** the DOL loader section size. For example,
copy[9] at VA `0x805F2780` has size `0x9DB8`, whereas the matching DOL
data7 header entry has size `0x9DC0`. The latter ends at `0x805FC540`,
exactly where zero call 3 begins. Its last eight bytes
`0x805FC538..0x805FC53F` are a loaded section tail, not an unloaded gap
deduced from the identity-copy descriptor. The checked PAL copy descriptor
still causes no write because its source equals its destination.

## Ordered stack and register effects

Let `S=0x8060C5F0`. Old stack RAM is **UNKNOWN** before these stores.
All addresses are guest effective addresses; words are stored big-endian.

| Executed instruction | Ordered effect |
|---|---|
| `0x8000315C..68` | `r0=0xFFFFFFFF`; `stwu` first writes old `S` at `[S-8]=[0x8060C5E8]`, then changes `r1=S-8`; stores `-1` at `[S-4]=[0x8060C5EC]`, then overwrites `[S-8]` with `-1`. |
| `0x8000316C`, `0x80003340..48` | `bl` sets LR `0x80003170`; `mflr` puts it in `r0`; `stw` overwrites `[S-4]` with `0x80003170`; `stwu` writes old `S-8` at `[S-0x20]=[0x8060C5D0]` and changes `r1` to `S-0x20`. |
| `0x8000334C..54` | Save incoming `r31/r30/r29` at `0x8060C5E4/E0/DC` respectively. In the synthetic capture, the saved `r31` is `0x8000315C`, not zero. |
| Each `0x800033D8 -> 0x8000540C` | `bl` sets LR `0x800033DC`; wrapper `stwu` writes `0x8060C5D0` at `0x8060C5C0`; saves LR at `0x8060C5D4`, and walker `r31` at `0x8060C5CC`. The three calls overwrite these same spill slots in order. |
| `0x80005424..38` | Wrapper reloads its saved LR and `r31`, restores `r1=0x8060C5D0`, and returns the original destination in `r3`. Its other volatile integer registers, including `r4`, are not restored. |
| `0x800033E4..FC` | Reloads `r0=0x80003170` from `0x8060C5EC`, restores incoming `r31/r30/r29`, sets `r1=0x8060C5E8`, sets LR `0x80003170`, and returns. The word at `0x8060C5E8` remains `0xFFFFFFFF`; the word at `0x8060C5EC` remains the saved return address. |

**PROVEN from leaf data flow:** for the final fill, `r4` starts as
`destination-4`, advances by 32 in each of five `stwu` group tails, then
by four in each of three remainder `stwu` instructions. It therefore ends
at **`0x805FC5E8`**, the last written word address. `0x800054D4` sets
`r6=r4+3=0x805FC5EB`. The wrapper and walker do not restore `r4`.
`r3=0x805FC540` is restored by the wrapper. The terminating zero-size
comparison leaves CR0.EQ set, with CR0.SO inherited from architectural
XER.SO. The **last XER.CA writer is `0x800054C8`**, the third execution of
`addic. r3,r3,-1` in the last fill's remaining-word loop. Its input is
`r3=1`, the sign-extended immediate is `0xFFFFFFFF`, and the 33-bit sum is
`0x100000000`: the 32-bit result is zero and CA is one. The following
`stwu`, branch, `addi`, `rlwinm`, `cmplwi`, return and walker epilogue do
not change CA.
XER.SO and XER.OV are carried from the incoming state. CTR is not written
on this checked path. `r2/r13` are carried through, and `r29/r30/r31` are
restored to their incoming values.

**OBSERVED-HLE:** the same synthetic capture at PC `0x80003170` reports
`r1=0x8060C5E8`, `r3=0x805FC540`, `r4=0x805FC5E8`,
`r5=0`, `r6=0x805FC5EB`, `r7=0`, `r31=0x8000315C`,
LR `0x80003170`, and CR `0x20000000`. Its stack window includes
`[0x8060C5E8]=0xFFFFFFFF` and `[0x8060C5EC]=0x80003170`.
These observations independently refute the old note's claims that
`r4=0` and restored `r31=0` at this boundary.

### The reported XER value is not an architectural observation

The capture's RSP field reports `XER=0x00000000`, apparently conflicting
with the counted `addic.` transitions. This is a debugger transport defect,
not evidence that `1 + (-1)` fails to carry. The local Dolphin GDB stub's
register-69 response reads its cached `spr[SPR_XER]`, while the interpreter
`addic.` updates the separate `xer_ca` field. Its architectural `GetXER()`
assembles `xer_ca` and the other XER bits; the cached SPR word is refreshed
by an explicit guest `mfspr XER`. Therefore the RSP XER field can remain
stale at `0x80003170`. The DOL does not issue `mfxer` inside this CRT region.
**UNKNOWN:** an independent architectural XER capture at this boundary.
The required check is a guest-visible `mfxer` or a debugger query using the
architectural `GetXER()`, not RSP register 69. This Dolphin-source finding
only diagnoses the reference interface; DOL bytes and ISA semantics remain
the authority for the guest behavior.

## First connected prerequisites and validation plan

1. **Hardware return:** execute the native chain from `0x80003154` through
   the exact first stop and prove its PC, LR, r1/r2/r13, r31, CR/XER and
   prior handler/FPR/stack effects at `0x8000315C`. The current native
   probe has not returned from hardware; a synthetic HLE snapshot cannot
   silently fill those missing effects.
2. **Descriptor state:** establish that live guest memory at
   `0x80005544..0x800055E7` still equals the SHA-pinned DOL table at the
   walker entry. Reject invalid descriptor pointers and modified data;
   resolve the `memmove`/cache branch separately if any source differs.
3. **Ordered CRT run:** implement only the proven reached path with checked
   guest addresses and BE writes. Record transient stack overwrites and
   all three fill intervals in order. Run the connected native prefix from
   `0x80003154`, comparing checkpoints at `0x8000315C`, each zero-call
   entry/return, and `0x80003170`; use boundary samples and hashes for the
   large zero ranges plus the handler and FPR source bytes.
4. **Architectural state:** compare r0/r1/r3/r4/r5/r6/r7/r29/r30/r31,
   LR/CTR/CR, stack words, and XER via a trustworthy channel. Include a
   regression that fails if r4 is reset to zero, r31 is lost, the upper
   sentinel is left `-1`, the second fill touches `0x805F277C`, or the
   first fill fails to erase the earlier handler slot.
5. **Next consumer:** only after CRT parity, trace `0x80003170` onward.
   Its low-memory BI2 pointer and optional debug/relocation paths remain
   separate unknowns. Do not infer runtime or game transition parity from
   these static CRT facts.

Portable C++ lowering is deliberately deferred until the hardware return
and live descriptor memory are proven in the connected run. The semantic
candidate is the ordered descriptor-driven zero operations and observed
stack/register consequences; no omitted hardware or unobserved debug path
is promoted to a native implementation here.

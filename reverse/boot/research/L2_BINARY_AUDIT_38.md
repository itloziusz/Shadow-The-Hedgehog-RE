# Independent binary audit of the checkpoint-38 L2 section

Date: 2026-10-01. This is a binary/CFG/data-flow audit, not a new execution
checkpoint. The coordinator owns advancement and the regression gates.
The checkpoint-37 sync implementation was neither edited nor reinterpreted.

**Authority:** read-only PAL GUPP8P `main.dol`, SHA-256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
Its header maps text section 1 from file `002600` to VA `80008D40`;
`offset = VA - 80008D40 + 002600`. The audit reread that header and the
original bytes before comparing the existing notes or native implementation.

## Scope and checks

**PROVEN, binary:** 95 words were reread in these inclusive ranges:

| Region | VA range | Words | File range |
|---|---|---:|---|
| Caller through the next unexecuted call | `80372894..80372904` | 29 | `36C154..36C1C4` |
| Nested helper | `80372640..803726D4` | 38 | `36BF00..36BF94` |
| MSR accessors | `80370ADC..80370AE8` | 4 | `36A39C..36A3A8` |
| SPR1017 accessors | `80370AFC..80370B08` | 4 | `36A3BC..36A3C8` |
| Leaf stack writer called as logger | `80370C8C..80370CD8` | 20 | `36A54C..36A598` |

The repository decoder was used as a comparison after the bytes were read.
Opcode, SH/MB/ME, SPR, BO/BI, signed displacement and CR-XOR fields were
checked independently from the words. Seven masks were tested against all
32 individual input bits: 224 bit cases passed. The existing complete
`BINARY_CACHE_HANDLER.md` raw gate also passed: 305 words, 56 direct branch
targets and 14 SPR operands. Those checks write no files and prove neither
physical cache completion nor native execution parity.

## Exact consumed fields and provenance

**PROVEN, binary:** the accessor word `7C79FAA6` at `80370AFC` is opcode31,
XO339, destination r3 and split-field SPR1017. Its following word is
`4E800020` (`blr`). The write accessor `7C79FBA6` at `80370B04` is XO467,
source r3, SPR1017, also followed by `blr`. Each read is fresh. No cache read
in this section is an ordinary load from RAM or SDA/SDA2.

| VA / raw BE word | Independently recovered transformation |
|---|---|
| `80372898 / 54600000` | SH=0, MB=0, ME=0: `r0 = r3 & 80000000`. |
| `803728C4 / 5463007E` | SH=0, MB=1, ME=31: `r3 &= 7FFFFFFF`. |
| `80372658 / 5463007E` | Same enable-bit clear, from a different fresh read. |
| `80372668 / 64630020` | `oris r3,r3,0020`: OR numeric `00200000`. |
| `8037267C / 546007FE` | SH=0, MB=31, ME=31: `r0 = r3 & 1`. |
| `8037268C / 546302D2` | Wraparound MB=11, ME=9: `r3 &= FFDFFFFF`. |
| `803726B8 / 546007FE` | A second independent `r0 = r3 & 1`. |
| `803728E0 / 64608000` | `r0 = r3 | 80000000`. |
| `803728E4 / 540302D2` | Wraparound MB=11, ME=9: `r3 = r0 & FFDFFFFF`. |

**PROVEN, binary:** only numeric bits `80000000`, `00200000` and `1`
receive a mask/OR interpretation in this section. The other bits survive
the software expressions, sourced from each specific readback. The binary
does not establish whether hardware accepts every retained bit on a write.
For example, `00200001 & FFDFFFFF == 1`: clearing the request bit cannot
make a pending-status bit disappear by arithmetic.

**PROVEN, binary:** conditional branches are BO4, BI2, AA=LK=0 at:

| Branch | Signed displacement | Taken edge | Not-taken edge | Input |
|---|---:|---|---|---|
| `803728A0 / 40820058` | +88 | `803728F8` | `803728A4` | CR0.EQ from `L2CR & 80000000`. |
| `80372684 / 4082FFF4` | -12 | `80372678` | `80372688` | CR0.EQ from first `L2CR & 1`. |
| `803726C0 / 4082FFE8` | -24 | `803726A8` | `803726C4` | CR0.EQ from second `L2CR & 1`. |

These branches test CR, do not decrement or consume CTR, and have no
software timeout. Each preceding `28000000` is an unsigned 32-bit compare
against zero, replacing CR0 and copying live XER.SO into CR0.SO. Their
result is EQ for zero and GT for nonzero; XER itself is unchanged.

## Control and data-flow sequence

**PROVEN, binary:** the enabled branch performs one L2 read and none of the
disabled-branch writes, temporary MSR change, helper or caller logger.
It still joins `803728F8` and prepares the next call.

**PROVEN, binary:** the disabled path has eight fresh reads and five write
operands when neither poll takes a back edge:

| Order | Site | Read/write dependency |
|---:|---|---|
| R1 | `80372894` | Initial read consumed by enable test. |
| R2 / W1 | `803728C0 / C8` | Fresh read, then clear E. |
| R3 / W2 | `80372654 / 5C` | Fresh helper read, then clear E again. |
| R4 / W3 | `80372664 / 6C` | Fresh read, then OR I. |
| R5 | `80372678` | First status read; each retry adds another read. |
| R6 / W4 | `80372688 / 90` | Fresh read after first zero, then clear I. |
| R7 | `803726B4` | Second status read; each retry adds a logger call and another read. |
| R8 / W5 | `803728DC / E8` | Fresh caller read after MSR restoration; OR E and clear I. |

Before R2 the caller reads **current** MSR at `803728A4`, saves it in r30,
executes `sync` at AC, writes literal `30` via `803728B4`, and executes
`sync` at B8 and BC. There is another `sync` after W1 at CC. The helper
executes `sync` at `80372650` and after W2 at `80372660`. The caller
restores saved r30 via `803728D4/D8` before R8. No literal reference-run
MSR value is needed by this data flow.

**PROVEN, binary:** the direct edges `80372670→74→78`, `80372694→98`,
`803726A0→A4→B4` do not change LR or data state. The helper's second-poll
setup overwrites r3 with `80560000`, computes r31=`80561380`, then the
fresh R7 overwrites r3 again. A successful helper return loads the saved LR
and r31 from actual stack words at `803726C4/C8`, restores SP at CC, writes
LR at D0 and returns through its low-two-bit-cleared address at D4.
There is no unresolved indirect call in this bounded interval; return
targets still require the proven saved memory, not ABI faith.

## Stack and leaf-writer effects

Let P be the caller SP at the frontier. The connected prefix produces
P=`8060C5E0`. **PROVEN, binary:** helper stores are, in instruction order:

1. `80372644`: `[P+4] = live LR`, here `803728D4` at `8060C5E4`.
2. `80372648`: `[P-10] = P`, here `8060C5D0 = 8060C5E0`; SP becomes P-10.
3. `8037264C`: `[P-4] = live r31`, here `8060C5DC = 80561380`.

The caller's leaf writer at `803728F4` allocates 70 bytes:
`[P-70]=P` at `8060C570`, then r3..r10 at `8060C578..8060C594`.
It restores SP with `addi`, not by reading its backchain. Its r3 value is
`r31+1E4 = 80561564`. The other saved arguments are their live values;
their validity must not depend on presumed zeros.

**PROVEN, binary:** `4CC63182` at `803728F0` has BT=BA=BB=6 and XO193:
CR bit6 becomes zero, numeric mask `02000000`. In the leaf writer,
`40860024` at `80370C90` is BO4/BI6, taken to `80370CB4` when this EQ bit
is false. Consequently all eight `stfd` instructions are skipped on this
call. The leaf writer has no string read, formatting, device write, nested
call, CR update or LR save; it only writes its stack and returns. XER,
CTR, FPR lanes and FPSCR are unchanged by the selected L2/logger path.

**Important alias falsifier:** skipped FPR stores do **not** mean their
slots are wholly unknown. The skipped f8 store at `80370CB0` would target
`[8060C5D0..8060C5D7]`; the earlier helper has already initialized its first
four bytes with the backchain. Those bytes remain known and unchanged.
Unknown bytes must remain unknown, while earlier known bytes keep their
validity. `NativeL2Prefix.cpp` preserves this alias correctly. The draft
completion note was corrected to distinguish these cases.

**PROVEN, binary:** a second-poll retry invokes the same leaf writer while
the helper SP is P-10. Its backchain is therefore at P-80=`8060C560`, and
r3..r10 at `8060C568..584`. It is not the caller's `8060C570` frame.
Native admission excludes this busy path; a future implementation must
expand its owned storage and preserve these addresses before admitting it.

## Distinct join states; implementation cross-check

**PROVEN, binary:** at the proposed stop **before** `80372904`, both paths
have r3=1, r4=`803726D8`, SP=P, original r31, and restored/original MSR.
They need not have otherwise equal states:

| State | E initially set | E initially clear; both polls finish |
|---|---|---|
| r0 | `80000000`, the extracted E bit | Final R8 OR `80000000`, before clearing I into r3. |
| r30 | Unchanged caller value | Saved current MSR. |
| LR | `80372898`, from initial read call | `803728F8`, from caller leaf-writer call. |
| CR0 | GT plus live SO | EQ plus live SO from the second zero poll. |
| CR bit6 | Original value | Cleared by caller `crxor`. |
| New memory writes | None | Three helper words and nine caller leaf-writer words. |
| L2 writes | None | Five operands, each from the listed fresh read. |

**STRONG, implementation audit:** `NativeL2Prefix.cpp` was compared with
the original words and the above independently derived effects. Its 95
fingerprints match; masks, MSR provenance, read/write ordering, distinct
branch states, stack stores/loads and leaf-writer effects have no identified
binary contradiction. It declines unexplained pending/request/reserved
input state and stops before the handler call. This static cross-check
does not itself prove the owned completion contract or replace the
coordinator's differential native execution and complete regressions.

## Downstream consumers and remaining boundaries

**PROVEN, complete text-word scan:** the two DOL text sections contain four
SPR1017 access instructions: read `80370AFC`, write `80370B04`, read
`8039F6F0 / 7FF9FAA6`, write `8039F7E4 / 7FF9FBA6`. All thirteen direct
branches to the two accessors are the thirteen R/W sites listed above.
The direct callers of `80372838` are `80003418 / 4836F421` and
`80371014 / 48001825`. This is aligned instruction-scan evidence;
possible dynamically manufactured code is not established or admitted.

**PROVEN, binary:** the earliest downstream consumer of helper control
state is R8 at `803728DC`, before the final write. The later direct call
at `80371014` can consume final E/config state by repeating this routine.
**UNKNOWN:** connected native reachability/state at that later call or at
the `8039F6F0/F7E4` access pair. Their incoming control word cannot be
assumed unchanged merely from this scan.

**PROVEN, binary:** `803728F8..80372900` constructs selector r3=1 and
pointer r4=`803726D8`; `48000A75` at `80372904` calls `80373378`.
That installer constructs base `80586CB0`, shifts/masks selector1 to 4,
then first reads the **live** old-handler word at `80586CB4` through
`803733B4 / 83C40000`. It writes the supplied pointer at `803733C0`.
The pointer is data at this point; this slice does not call `803726D8`.
**UNKNOWN:** the live slot's initial producer/ownership and later callback
invocation semantics; static DOL/BSS bytes are not a valid default.

**UNKNOWN, outside binary proof:** hardware write acceptance, physical
L2IP/timing/tag/dirty-data evolution, retail IPL inputs and exceptional
interleavings. No full-cache emulation follows from these instruction
effects. A native owner may collapse the mechanism only after proving its
ordinary memory, ordering, configuration and status consequences for its
explicit scope. A trace with IP=1 must take the exact back edge or fail
closed, never clear the bit to reach the next checkpoint.

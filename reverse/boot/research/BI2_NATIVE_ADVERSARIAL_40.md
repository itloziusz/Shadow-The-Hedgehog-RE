# Independent native BI2 / OS / clock-prefix adversarial audit

2026-10-01. PAL GUPP8P original DOL SHA-256:
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
Original inputs are read-only. This audit does not edit native source,
advance a checkpoint or waive the full regression gates.

`PROVEN` denotes exact bytes/local architectural effects; `STRONG` denotes
a bounded portable consequence; `UNKNOWN` includes unmeasured timing,
retail ownership or unexecuted downstream effects.

## Raw and independent portable checks

`tools/agent_bi2_native_audit.py <original-main.dol>` independently rereads
**105** instructions: entry188..264, debug4 leaf, metadata leaf, OS first
guard, clock prologue, full six-word clock sampler and the prior EE leaf.
It derives branch displacements directly from machine words, compares their
targets with the project decoder and fails on any undecoded word. Every
one of the native implementation's **95** additional literal gates also
matches the original binary.

Its isolated portable tests demonstrate modulo32 **data-word** relocation,
wide checked **address/count** extents, same-owner offset/debug-field alias,
one-byte debug4 state and current metadata/guard views. Thirteen unsafe
mapping/array cases decline. These tests are not a complete boot execution
or a native differential claim.

## Pointer and extent safety

**PROVEN source audit:** `NativeBi2Prefix.cpp` calls the unchanged connected
CRT runner, and proceeds only at80003188. The measured low-memory pointer
is an explicit optional input; missing state stops before its first load.
Known zero stops before the separate unknown arena word at800031A4.
Nonzero pointer requires aligned ordinaryMEM1 bounds, complete8192-byte
owned contents and disjoint loaded/CRT/stack/low-memory storage. Missing
blob stops at80003194; partial blob and unexplained addresses decline.

The implementation excludes copy-descriptor extents, which are shorter
than some full DOL section extents. I independently checked the combined
exclusion union, including exact CRT clears and low memory. For a fixed
8192-byte aligned BI2 mapping, **every possible overlap with every full
DOL section, including its section tail, is excluded**. Thus no current
section-tail ownership hole was found. This conclusion depends on the
fixed full-blob extent and the exact PAL interval union; it does not prove
the same exclusion strategy adequate for arbitrary shorter owners.

**PROVEN:** offset/count reads are checked against the current blob owner;
`B+O` uses a wider overflow check before a pointer consumer. The entire
`P+4+4*N` extent is checked in64bits before the relocation loop. HugeN,
unalignedO, beyond-ownerO and the finalcount-at1FFC withnonzeroarray all
decline before an unknown store. Relocated **values** addB modulo32; they
are opaque words, not automatically valid native pointers or call targets.
The operation has no Rc/OE/carry effect and retains the earlier CRT CA.

## Fresh reads and alias preservation

The native operation reads the separate closed low-memoryF4 owner twice,
at80003188 and80003200. It does not recycle the first register value. The
two values necessarily agree inside this admitted private interval because
its debug4 and relocation stores cannot aliasF4; debug2/3 stop before their
context-transfer call. This is an ownership proof, not permission to omit
the second load on a future broader callback path.

The relocator loads and writes in order in one mutableBI2 vector, soO8
correctly treats the offset word as its own count and overwrites the later
debug word while retaining the already loadedr7. No independent cached
debug/offset field view supplies a later read. Counts1/3/7 and a zero count
retain the correct distinctr6/r14/r15/CTR states. Arena publication uses
aligned **first** array word, not the end. An invalid relocated word may
remain opaque; a future dereference requires its own producer/target proof.

An early manual comment suggested the zero-pointer fallback left
r6=800030E8. Direct byte inspection disproved it:31B0..B8 materialize and
load **r7** (`3CE08000`, `38E730E8`, `80E70000`); r6 stayszero. The
coordinator received this correction immediately. The current native path
stops before the fallback read, so no code defect arose from that comment.

## Current SDA and stack owners

Debug4 setsr0=1 and stores exactly one byte at805F1FF0; its adjacent
CRT-produced bytes remain unchanged. Metadata stores805F1F18=80000040
and805F1F1C=1, and the later guard read/store at805F1F40, all use the
same current CRT-produced byte owner. `run.prefix.zero_ranges` is an
earlier **historical checkpoint**, not an independently live allzero view.
The public current globals and checkpoint SDA words agree with real reads
from the updated owner. The metadata physical-pointer value00370C60 is
derived by `addis` modulo32; it is stored as data at80000048.

OS prologue overwrites the savedLR at8060C5EC with80003268 and saves
its incomingr31/r30/r29 atE4/E0/DC. Its actual guard load consumes the
CRT-cleared word, not a constant substituted for that read. Any unexplained
nonzero guard declines; the signedzero compare has the same result on the
provedzero input while preservingCR fields andliveXER.SO.

Clock prologue savesLR80370EA8 at8060C5D4, writes the frame backchain
8060C5D0 at8060C5B0, and spillsOSr31=80586C40/r30=805610F8/r29 at
8060C5CC/C8/C4. These are real alias overwrites of earlier fill/handler
stack bytes. The shared stack updates the paired view before every new
checkpoint. The EE leaf reads currentMSR, clears bit8000, returns priorEE
inr3, then80379664 retains it inr31. No clock sample has occurred yet.

No concrete native implementation discrepancy was found in these reviewed
ownership, pointer, register, word/byte-store or alias paths. Native input
unit tests cover bothL2 profiles, debug0/1/4/5/FFFFFFFF, zero/nonzero arrays,
O8 alias, debug2/3 stops, missing/partial input and95word mutations. Their
execution still needs the independent reference and full gates below.

## Exact unresolved timing frontier and learned fingerprint

**PROVEN next state:**80379668 setsLR8037966C and reaches80379628.
The next raw word is7C6D42E6, `mftbu r3`. The complete sampler is:

```text
80379628 7C6D42E6  readliveTBupper→r3
8037962C 7C8C42E6  readliveTBlower→r4
80379630 7CAD42E6  readliveTBupper→r5
80379634 7C032800  signedcompare r3,r5→CR0 +XER.SO
80379638 4082FFF0  retry28 if CR0.EQclear; CTRunchanged
8037963C 4E800020  return throughLR
```

**UNKNOWN:** the high/low/high producer values, retry count, timer epoch,
advancement/units and preentry offset words800030D8/DC. These must not be
replaced by an expected snapshot, fixed clock, zero offsets, a singleread
or a forced no-rollover branch. After actual sampling,80379678/7C use
`addc/adde`; their carry chain will need new proof. The stop is **before**
the first read at80379628, not after a manufactured time sample.

The tool scans all executable words and found eleven timebase-read words.
It recognizes two complete high→low→high→compare→retry→return structural
families:80379628 and804035D4. The latter uses scratchr0 and reversed
comparison order; the detector preserves actual registers/clobbers and
requires distinct sample destinations, the equality CR dependency and the
exact backward retry target. Four adversarial mutations reject aliasing,
wrong compare operand, wrongCR bit and wrong target. Both remain
`STRUCTURAL_MATCH`; neither resolves the live time producer. A separate
80376EBC/C0 consumer measures TBlow delta until unsigned>=1124, showing
why future unit/advancement evidence will matter.

## Remaining validation requirements

### Independent compiled execution check

I executed the compiled nativeBI2 runner from the coordinator's explicit
entry fixtures for `bi2-zero-40`, `bi2-four-40`, `bi2-array-40` and
`bi2-empty-40`. An independent comparison did not import its differential
validator. It associated each memory/global observation with the immediately
preceding native state and preserved repeated array-PC occurrences. It
derived stack validity separately:21 known words afterCRT, plus the two
newclock spills atC4/C8 when80379660 is reached.

| Capture | New checkpoints | Mandatory raw/global fields | Known stack bytes | Full BI2 bytes |
|---|---:|---:|---:|---:|
| zero | 26 | 3,146 | 2,536 | 212,992 |
| debug4 | 29 | 3,509 | 2,824 | 237,568 |
| array3 | 40 | 4,840 | 3,880 | 327,680 |
| offset/nonzeropointer/countzero | 28 | 3,388 | 2,728 | 229,376 |
| Total | **123** | **14,883** | **11,968** | **1,007,616** |

All compare exactly and stopbefore80379628. Mandatory field counts are
113 architectural fields plusL2CR, handler, lowmemory44, BI2pointer and
fourSDA words per point. Producedoptional arena/lowmemory48 values were
also compared wheneverknown but are excluded from those fixed fieldcounts.
The comparison confirms both the read-only and mutated fullBI2 owner,
onebyte debug4, countzero versus countthree, actualspill overwrite timing
and retained FPR/SPR/CR/XER states. This is still conditional syntheticHLE
chain evidence, notretail timing or wholeOS completion. A controlledO8
reference profile would additionally exercise the same-owner alias case
already covered by native unit and rawsemantic checks.

Capture the complete original liveBI2 before entry/first load; label and
readback every controlled preentry mutation. Compare both pointer reads,
all branch checkpoints, in-place relocation bytes/CTR transitions, exact
metadata and byte stores, guard source/current owner and stack aliases.
Then run the full connected prefix, all previous sync/L2/CRT gates and
complete repository/gameplay regressions. Static classifications and unit
tests cannot advance the frontier. Retail timing/ownership, debugcontext,
zero-pointer arena input and laterOS/device consumers remainUNKNOWN.

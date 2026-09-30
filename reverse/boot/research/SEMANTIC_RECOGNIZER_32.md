# Machine-code recognizer batch 1 — bounded findings

PAL DOL SHA-256:
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
The tool reads raw bytes and stores raw words, DOL file offsets, decoded PPC,
normalization, CFG, symbolic effects and provenance in its local SQLite DB.
The generated detailed JSON report is under ignored `build/`.

## Connected frontier: `0x80371730`

**PROVEN-BINARY:** `0x80371730..0x80371767` is the 14-word tail listed in
`SYNC_GQR_CHAIN.md`: `sync`, `li r3,0`, eight consecutive `mtspr` writes to
SPR912–919, `lwz r0,12(r1)`, `addi r1,r1,8`, `mtlr r0`, `blr`. The recognizer
independently checked the bytes and symbolically propagated zero to all eight
SPR write operands. The restored LR is an expression based on a live stack
load, not a constant copied from the reference run. The next return target
cannot be established from static DOL data alone.

**PROVEN-NATIVE-PREFIX:** With explicitly supplied synthetic MSR `00002032`,
HID2 `E0000000`, and HID0 `0011C464`, the Release probe reran from DOL entry
`0x80003154` and stopped before `sync` at `0x80371730`. It reported LR
`0x80371730`, r0 `0x80003414`, r1 `0x8060C5E8`, r3 `0x0011CC64`, and
ordered stack writes `0x8060C5F4=0x80003414` and
`0x8060C5E8=0x8060C5F0`. The recognizer attached this probe output only
when its stop PC equaled the frontier.

**UNKNOWN:** ICFI completion and ordering at `sync`, retail HID0/ICE/ABE
state, actual GQR readback and subsequent paired-single behavior. The
`sync_gqr_zero_chain` detector means the raw instruction sequence matches;
it does not make the hardware consequences or connected native tail valid.
The persistent frontier therefore stays at `0x80371730`. A minimal oracle
experiment should capture HID0/HID2/MSR and stack bytes before/after `sync`,
GQR readback, then the first GQR-dependent consumer at `0x80370CFC`.

## Unknown constructor-table cluster

The initial table at `0x804AAC60` has static code pointers. Scanning indices
16–47 produced 32 bounded target candidates. A complete-linkage cluster of
15 (`ctor_16`–`ctor_26`, `ctor_28`, `ctor_32`–`ctor_34`) had minimum pairwise
structural score 0.8036 under the disclosed weighting. Its common coarse
motif includes SDA load, two address materializations, SDA2 float loads,
an update-form store, another integer store, two float stores, and return.
This is a **STRUCTURAL_MATCH**, not an established common semantic family.

For example, table word `0x804AACA0` points statically to `0x8005C75C`.
The `ctor_16` first-return window is `0x8005C75C..0x8005C783`:

| VA | Raw word | Decoded operation | Abstract effect |
|---|---|---|---|
| `8005C75C` | `800D86D0` | `lwz r0,-0x7930(r13)` | reads live SDA word |
| `8005C760` | `3C608052` | `lis r3,0x8052` | address high half |
| `8005C764` | `3C808052` | `lis r4,0x8052` | second address high half |
| `8005C768` | `C0228778` | `lfs f1,-0x7888(r2)` | reads live SDA2 float |
| `8005C76C` | `9403F554` | `stwu r0,-0xAAC(r3)` | stores at `0x8051F554`; updates r3 |
| `8005C770` | `C002877C` | `lfs f0,-0x7884(r2)` | reads live SDA2 float |
| `8005C774` | `9004F530` | `stw r0,-0xAD0(r4)` | stores at `0x8051F530` |
| `8005C778` | `D0230010` | `stfs f1,0x10(r3)` | stores at `0x8051F564` |
| `8005C77C` | `D0030014` | `stfs f0,0x14(r3)` | stores at `0x8051F568` |
| `8005C780` | `4E800020` | `blr` | returns to unknown live LR |

The addresses follow from the raw `lis` and `stwu` update semantics. The
values read through SDA/SDA2 stay symbolic because runtime RAM can differ
from DOL initial contents. The table is writable; the original live target,
ordered memory delta, object ownership, and connection to boot are **UNKNOWN**.
The next experiment is a breakpoint before table dispatch, a live pointer
read, then entry/return register and four-store memory-delta capture.

## Self-learning and falsification

The first batch stored 39 regions: 2 previously validated bounded seeds,
4 statically strongly supported constructor slices, and 33 unknowns
(32 table targets plus the frontier). It generated 13 reusable coarse
patterns from the four constructor seeds and 35 **structural** hits among
unknown targets. Five unknown clusters contained more than one candidate.
These counts are discovery metrics, not boot-completion percentages.

Adversarial tests mutate a GQR index, interrupt the consecutive write run,
insert unknown/unsupported semantics, change branch structure, check that
untraced calls poison post-call state, and reject stale paired-lane knowledge
after a scalar load. Learned patterns are rebuilt
from current evidence so demoted seeds do not leave stale patterns. None of
the new candidate hypotheses passed behavioral or chain validation, so no
new classification was promoted and the boot PC did not advance.

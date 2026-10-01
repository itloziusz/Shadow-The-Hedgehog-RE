# Adversarial audit of the bounded native L2 contract

**2026-10-01. Audit only; no checkpoint advancement or new physical-cache
parity claim.** This review covers the proposed continuation from
`0x80372894` to the unexecuted handler call at `0x80372904`, including
`0x80372640..D4` and the selected logger path. It leaves checkpoint 37's
sync implementation unchanged. PAL DOL SHA-256:
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.

## Binary and architectural facts

**PROVEN-BINARY:** the complete instruction catalogue is
[`BINARY_CACHE_HANDLER.md`](BINARY_CACHE_HANDLER.md). An independent direct
DOL read during this audit checked seven anchor words and independently
formed four rotate masks:

| Address | File offset | BE word | Independently checked consequence |
|---|---|---|---|
| `80372898` | `36C158` | `54600000` | isolate `80000000` |
| `80372668` | `36BF28` | `64630020` | OR request `00200000` |
| `8037267C` | `36BF3C` | `546007FE` | isolate status `00000001` |
| `8037268C` | `36BF4C` | `546302D2` | mask `FFDFFFFF` |
| `803728E4` | `36C1A4` | `540302D2` | same mask, different source register |
| `80370AFC` | `36A3BC` | `7C79FAA6` | read SPR 1017 |
| `80370B04` | `36A3C4` | `7C79FBA6` | write SPR 1017 |

**PROVEN-MANUAL:** numeric bits are E=`80000000`, CE=`40000000`,
DO=`00400000`, I=`00200000`, WT=`00080000`, TS=`00040000`, and read-only
IP=`1`. DO changes instruction-cache participation; CE controls double-bit
error handling. WT must be configured before enable. I requires E clear.
Global invalidation clears tags/status/LRU; it is not an instruction to
flush all modified cache data. The prescribed sequence finishes stores,
disables L2, orders pending L2 operations, requests invalidation, waits for
IP clear, clears I, then enables. Initialization also recommends disabling
interrupts and DPM. These facts come from the
[IBM Gekko manual](https://doc.kodewerx.org/documents/gekko_user_manual.pdf),
Table 2-19 (printed 2-25/26), §9.1.1 and §9.1.3–4 (9-2/4).

**PROVEN-BINARY, `80372894..A0`:** E is the only directly tested control
bit. E set skips all MSR/L2 writes and the L2 helper. E clear selects the
helper path. **PROVEN-BINARY, `80372678..84` and `803726B4..C0`:** each
poll consumes only IP from a separate read. The software clear at `80372690`
clears I, not IP. CE/DO/WT and other readback bits participate in write
operands because masks retain them; they are not branch predicates here.

## Minimum observable native effects

**STRONG, confined to this new native owner and these addresses:** no cache
tag/data simulation is needed when all load-visible bytes are authoritative
owned memory, compiled bodies are immutable, stores materialize immediately,
and no external observer, pending device queue, inherited cache-only data,
or fault state exists. These are backend properties, not facts inferred from
an arbitrary physical L2CR word.

The required retained effects are:

1. Derive the first branch from the actual entry E bit at `803728A0`.
2. On the disabled branch, save the current produced MSR in r30, use the
   binary's literal `30` during the L2 interval, and restore r30 at
   `803728D8`. Do not copy a normal-case expected MSR.
3. Preserve ordered, input-dependent L2 writes at `803728C8`, `8037265C`,
   `8037266C`, `80372690`, and `803728E8`, preserving passive controls.
4. Keep software I request state distinct from read-only IP work state.
   Complete the owned invalidation consequence, then obtain IP from that
   producer. I remains asserted until the actual guest clear.
5. Perform both fresh poll reads with their exact masks, CR0 comparison
   effects including live XER.SO, and successors. An unexplained busy state
   is not an authorization to take the zero branch.
6. Preserve helper frame bytes and load saved LR/r31 from them on return.
   Preserve the selected final logger's nine ordinary stores, its restored
   SP, and cleared CR bit 6. Its FPR-store branch is not selected after the
   actual caller clear (`803728F0`).
7. Retain final input-dependent L2CR for the fresh read at `803728DC` and
   later consumers. Construct selector 1 and pointer `803726D8` from the
   three common successor instructions, then stop before `80372904`.

No timer read, external callback, instruction store, DMA submission,
reservation, or indirect call occurs in this selected interval. Repeated
first-poll iterations change temporary r3/r0/CR0/LR but converge to the same
exit state when completion occurs. The bounded replacement may complete its
owned effect synchronously; it does not claim console loop-count, latency,
or intermediate busy-snapshot parity. The second poll remains independently
represented. In the private owner, clearing an already completed I request
does not create a new request; a new unexplained IP state must decline.

## Audit of the implementation

**STRONG / code-reviewed, not a substitute for coordinator regression:**
[`NativeL2Prefix.cpp`](../src/NativeL2Prefix.cpp) calls the unchanged
checkpoint-37 runner from entry. `OwnedL2` rejects incoming I, IP, TS and
reserved bits, and retains E/CE/DO/WT under mask `C0480000`.

`Value()` derives IP from private requested/completed epochs rather than
echoing a write operand. `Write()` checks passive-control preservation and
forbids E+I. Its request transition verifies committed owned words, records
the request, performs the bounded consequence and closes the completion
epoch. No caller completion flag, supplied poll sequence, or reference
checkpoint value is accepted. Both poll sites still use fresh reads and
their exact branch projection. The selected saved LR/r31 loads consume
validity-tracked bytes; unknown bytes cannot satisfy them.

The epoch mechanism alone is not evidence that physical cache operations
completed. Its justification is the absence of independent cache payload
and pending work in this private backend. Under that explicit scope, the
actual native byte stores and immutable bodies satisfy the available
downstream observations. I found no concrete code contradiction in this
scope. No narrower E/CE/DO/WT mask is required for this finite native owner.
The mask must not be advertised as accepting every physical machine state
with those four bits.

## Falsifiers and unresolved physical state

**UNKNOWN / not accepted by this contract:**

- E=0 does not prove that a real machine has no inherited dirty L2 sector.
  A counterexample is cache-only modified data whose tag is erased by this
  helper; a native copy of stale backing RAM could diverge later. Sync is
  not a proof that every such sector was flushed.
- E=1+WT does not establish that WT was configured before a previous enable.
  This native owner imports no such historical cache generation. CE=1 also
  does not prove absence of a physical ECC fault; owned byte storage has no
  inherited ECC state. Those physical histories remain unresolved.
- HID0 `0011C064` includes DPM while this L2 slice never writes HID0. The
  initialization recommendation above therefore prevents claiming the DOL
  sequence establishes every prescribed physical initialization condition.
  It does not contradict checkpoint 37's private sync completion or require
  altering that validated path.
- A physical IP transition, timing interrupt, machine check, or new external
  writer between the polls would invalidate the closed-owner proof. Incoming
  I/IP cannot be made acceptable by masking them away.
- The later reader at `80371014` and context-save read at `8039F6F0` remain
  outside connected native reach. Stable final controls here do not prove
  their future incoming state or eliminate future hardware boundaries.

**PROVEN-SOURCE / HLE limitation:** the pinned
`Interpreter_SystemRegisters.cpp` (SHA-256
`698f153ad3a9e457c7d51f9a84358b86140c29b879f35dc39fe70412f23f8556`)
assigns the full mtspr operand to its SPR slot before an empty L2CR switch
case. It does not implement the physical invalidation state machine or the
read-only nature of IP. Thus HLE perturbations prove operand/branch data
flow, not cache completion or physical status timing.

## Validation obligations

This audit checked the binary anchors, bit masks, primary manual, pinned HLE
source and new implementation independently. It ran no new reference capture
and does not claim new execution parity or full regression results.

Before advancing, the coordinator must require full entry replays for both
E branches with nonzero passive controls and varied inherited CPU/FPR state;
byte-level stack comparisons including the logger; fail-closed I/IP/TS and
reserved-state cases; independent busy first/second branch projections;
instruction mutation rejection; and the complete repository and gameplay
gates. In particular, busy HLE experiments must not be relabeled as admitted
native completions merely because the HLE can write its read-only IP slot.

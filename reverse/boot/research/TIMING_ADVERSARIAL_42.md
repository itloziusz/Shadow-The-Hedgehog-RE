# Independent timing and event ownership audit after research41

2026-10-01. Binary authority: original PAL `main.dol`, SHA256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
This audit writes no central/native source. It does not admit an elapsed clock
or advance connected checkpoint40, which remains **before80379628**.

`PROVEN` below means exact raw bytes or the pinned private reference's source
contract. It does not mean physical Gekko timing. Research41's four successful
clock projections remain conditional on an observed upstream cycle producer.

## What elapsed ownership must establish

An epoch is a legitimate initial input when it belongs to an identified
boot/environment owner at the actual entry boundary. A frontier cycle count is
an output of the prefix, so accepting it as a native clock input leaves the
prefix's elapsed production unproved. Renaming it an origin, work token, tick
budget, phase or external time service does not establish that production.

An acceptable connected implementation would have to:

1. Own the initial clock epoch and epoch cycle origin; retain the phase lost by
   division by12. Do not infer either origin from a single TB snapshot. Keep
   the architectural cached TU/TL pair separate from the live producer.
2. Derive elapsed work from the already reconstructed semantic operations,
   selected branches and actual loop counts. Costs and retirement boundaries
   must be independently justified from the original route and the admitted
   timing model. Native code must not fetch/dispatch arbitrary PPC operations,
   ingest a captured opcode/cycle stream at runtime, or accept expected reads.
3. Own the scheduler's initial global timer, slice length, downcount, sane
   state, overclock factors, FIFO id and event queue. A restricted contract can
   admit factor1 and reject other modes; it cannot silently round them.
4. Own every event/device state that can affect the interval. Each event needs
   its due time, FIFO order, callback identity, userdata and callback effects.
   Thread ingress, configuration changes, HLE hooks, savestate/time rebases,
   unrecognised callbacks or exceptions require a decline at their first
   consumption unless their producers are reconstructed.
5. Preserve operation effects, block retirement, due-event dispatch, callback
   scheduling, interrupt checks and memory visibility in the proved order.
   A large native fill can use a mathematical work count only if callbacks
   still see the correct progressively written prefix of the destination.

The raw PC words may remain fingerprints and proof annotations. They must not
become a generic runtime PPC execution engine. Trace elapsed values remain
validation outputs, never native inputs.

For a source-specific interpreter contract this design may produce a portable
native elapsed owner. Physical equivalence additionally needs a justified
Gekko counter/retirement and cache/bus/interrupt timing contract; private
interpreter table cycles alone cannot supply that proof.

## Debugger observations do not preserve uninterrupted elapsed work

**PROVEN private source**, `Interpreter.cpp:257..286`: the debug runloop creates
a local `cycles=0`, executes operations until `m_end_block`, then subtracts
the accumulated costs from downcount. `CheckAndHandleBreakPoints()` returns
from inside that block, before the subtraction. Costs of operations already
executed in the interrupted block are discarded with the local variable.

`HW/CPU.cpp:137..145` forces one SingleStep on resume from a breakpoint.
`Interpreter.cpp:204..218` calls Advance, executes the operation, ignores its
returned table cost, then sets slice length1/downcount0. That operation follows
a different schedule; it does not restore the lost block cost. Waiting while
paused itself does not advance CoreTiming.

Consequently a trace with many otherwise read-only checkpoints can change
elapsed production. The observed research41 entry-to-frontier difference
154066 cannot be promoted to an uninterrupted semantic work constant.
Even matching final TB/GPR results is weak because division by12 hides cycle
residues and an event may observe the difference before that final sample.

An installed `80003154` address breakpoint/watchpoint would likewise force an
entry SingleStep on resume. Initial GDB control is not necessarily an installed
address breakpoint. The passive42 trace resolves this distinction: its first
entry branch retires1 by downcount2A0B->2A0A while global timer28D70D and
slice2A0B remain unchanged at800032B0. That is ordinary block retirement;
a SingleStep reset/Advance would produce a different scheduler tuple.

## Continuous block cost is not an instruction-count clock

**PROVEN private source**, `PPCTables.cpp`: normal integer/load/store operations
in the admitted fill path cost1; `mtspr` costs2, including `mtlr`; `sync`,
`mtfsf` and `mulli` cost3; `lmw/stmw` cost11. Both runloops sum returned costs.
An imprecise comment in the debug runloop is not the executable contract.

Table `FL_ENDBLOCK` flags are not sufficient to derive interpreter blocks.
Although `mtspr` has that metadata flag, the ordinary interpreter LR/CTR/HID/GQR
write effects do not set `m_end_block`. Its actual branch/mtmsr/HLE effect
functions determine retirement boundaries. Using the metadata flag here would
move clock/event observations even if the total cost remained correct.

`Interpreter_Branch.cpp` ends a block at direct/conditional/LR/CTR branches,
including an untaken conditional branch. `mtmsr` ends a block after checking
exceptions. `mftb` does not end a block. Thus unmodified continuous
`80379628/2C/30` reads share the same downcount in an uninterrupted sampler
block and, under a fixed producer epoch, return the same source TB value.
The controlled SingleStep rollover experiments prove sampler retry effects
under their declared schedule, not a continuous same-block rollover.

Changing retirement placement can change a clock read or event lateness even
when the final total cost agrees. Native semantic work accounting must retain
the boundaries at which the reference commits that cost.

### Independently derived finite loop costs

For the admitted aligned, all-zero CRT fills with size>=32 and size%4=0,
the exact raw leaf `8000543C..800054F0` has:

```text
leaf cost = 19 + 10*(size >> 5) + 3*((size >> 2) & 7)
```

The fixed19 operations cover the selected preparation, remainder check and
zero-byte-tail return. Each32-byte group executes8 stores, one addic. and one
conditional branch. Each residual word executes addic., stwu and branch.
The12-operation wrapper `8000540C..80005438` costs **13**, since its mtlr costs2.

| Actual descriptor | Size | Groups | Residual words | Leaf cost | With wrapper |
|---|---:|---:|---:|---:|---:|
|8056FE00|476928|14904|0|149059|149072|
|805EF020|14172|442|7|4460|4473|
|805FC540|172|5|3|78|91|
|Total||||153597|153636|

BI2 relocation `80003230..80003240` costs5 per actual item: address increment,
load, add, store, CTR branch. Native bounds and alias guards must establish
the actual count; a supplied timing count cannot replace the semantic count.

These exact cost formulas cover a large part of work but are not yet a whole
prefix timing proof. Unknown hooks, event interleavings, path variants and
interrupt consequences remain relevant. The initial manual wrapper estimate
was corrected from12 to13 after inspecting the exact `mtspr` table entry.

## HLE hooks are another scheduling owner

**PROVEN private source**, `Interpreter.cpp:110..139`: `HandleFunctionHooking`
calls `TryReplaceFunction` before reading the opcode. A Replace hook executes
the hook and returns early; its cost is read from the **previous**
`m_prev_inst`, not the current DOL operation. The source TODO identifies this
oddity. A Start hook sets `m_end_block=true` in HLEFunction and still executes
the current operation. Both paths can change block structure.

`HLE.cpp:183..233` looks up an active address map. Nonfixed hooks apply only
at the corresponding symbol's start; fixed hooks do not need a symbol.
`IsEnabled` admits Debug hooks in Interpreter mode even without the debugging
configuration. The constant hook catalogue includes OSReport/printf Start
hooks and OSPanic Replace, while the active map depends on symbol loading and
patch installation. DOL words alone cannot establish an absent active hook.
In particular, the stack-only original logger at80370C8C still needs a
same-run no-hook or exact known-hook observation at each visit.

An observer must record the actual hook decision and any early-return cost.
It should capture the already fetched word/opinfo, rather than call
Read_Opcode again: another instruction fetch can change cache state.

## Event timing is more than pending CPU exceptions

**PROVEN private source**, `CoreTiming.cpp:376..418`: Advance checks
configuration, moves thread events, commits elapsed cycles, sets sane=true,
dispatches every due event, then sets sane=false and chooses the next slice.
Callbacks can schedule additional due events that dispatch in that same loop.
The final CheckExternalExceptions occurs after callbacks.

The heap order is `(signed due time, unsigned fifo_order)`, not event name or
current insertion container order (`CoreTiming.h:62..78`). Equal-time callback
ordering can affect memory even with the same final timer value. Callback
lateness is actual global timer minus due time; a block can overshoot a slice.

`ScheduleEvent` on the CPU thread uses GetTicks and can shorten the current
slice via ForceExceptionCheck. Off-thread publication instead stores a
relative timeout; MoveEvents adds the **pre-advance committed global_timer**
and assigns the FIFO id. Replacing that base with current GetTicks changes
the resulting due time. A publication timestamp or later exception snapshot
cannot establish the original thread ingress order.

Downcount/cycle conversion uses float32 multiplication and truncating int
conversion. At overclock1 the ForceExceptionCheck slice rewrite preserves
GetTicks. At factor1.5, downcount100, slice100 and requested delay1, the actual
rounding changes the projected elapsed value from34 to35. A generic
preserve-time rewrite therefore needs the exact source conversion or a
rejecting mode restriction.

Pending architectural exception flags0 can coexist with a queued event or
a callback that mutates memory without raising an interrupt. With EE0,
callbacks still execute, and external exception delivery is deferred, not
cancelled (`PowerPC.cpp:591..642`). Enabling EE via the clock restore can
change PC/NPC/SRR0/SRR1 immediately if a pending flag appears. Returning the
ordinary clock tail requires proof of the applicable event and exception
conditions, not an unconditional assumption that the enable is harmless.

## Minimum passive trace and falsification gates

A sufficient observation stream for the source contract needs:

- Pinned executable, copied observer translation unit, linked library, original
  DOL and configuration; label every pre-entry control and resume schedule.
- Passive initial clock and scheduler tuple, active hook decisions/map, initial
  event/device state, queued thread ingress and FIFO ownership. An all-events
  initialization-to-frontier journal may reconstruct the queue; a filtered
  window cannot assume the missing initial queue empty.
- For every operation: ordinal, mode, PC, fetched raw word, actual opinfo cost,
  previous word, hook type/index, before timing tuple/MSR/pending flags, after
  PC/NPC/end_block/returned cost and relevant architectural/memory changes.
- Retirement before/after downcount and cost, every Advance phase, schedule,
  force, cancel, dispatch/callback-return and configuration/rebase/ingress.
  Preserve complete blocks or a proved lossless representation.
- Pure initial/final checkpoints and actual TB destinations/cache. A getter
  must not execute another guest read or call a device-producing API.

Promotion must fail if any of these focused experiments disagrees:

1. Passive entry versus entry-breakpoint/resume and dense-breakpoint profiles;
   explain all lost/forced-step work rather than fit their final delta.
2. Every admitted cache/L2/MSR/BI2 path and at least relocation counts0/1/3/7;
   actual loop work must change by the independently derived increments.
3. Epoch phase residues0..11, signed-high transition and full64 wrap; the
   sampler must follow its genuine selected mode and retry branch.
4. Slice deadline exactly at, before and inside a cost-bearing block; compare
   lateness, callback memory visibility, FIFO order and nested due callbacks.
5. Event/interrupt perturbations before disable, during EE0, and at restore;
   include a callback with memory effects but unchanged exception flags.
6. Start/Replace/no-hook paths, factor1 and a rejected changed factor, queued
   foreign-thread ingress, source rebase and malformed/unknown event owner.
7. Independent checkpoint parity for all owned state, followed by full
   repository and gameplay regression. Arithmetic parity alone is insufficient.

## Independent passive42 stream validation

The first passive zero capture was rejected as full-prefix evidence: its
buffered instruction stream ended with a partial hook8037612C record. The
observer needed a mandatory flush at the final boundary before process
termination. Its initial event window also ended at ppc80379630, leaving the
third read/effect and tail outside that stream. The corrected42c observer
flushes exit80373AC0->80373AC4 and marks timing-prefix-end. Inherited metadata
that incorrectly said SingleStep/80379628 was corrected to continuous/80373AC4.
The auditor rejects the earlier full trace and the stale schedule metadata.

Final42d captures are under `build/passive-timing-{zero,paused,enabled}-42d.*`.
The zero and1-second-paused profiles share exactly the same entire instruction
stream bytes; the enabled profile supplies the alternate L2 stateC0480000.

|Profile|Rows|Original operations|Total table cost|Elapsed to stop|Pending cost|First-clock operations/cost|First-clock retired blocks|
|---|---:|---:|---:|---:|---:|---:|---:|
|zero|462747|154249|154290|154286|4|154206/154246|15546|
|paused, same L2|462747|154249|154290|154286|4|154206/154246|15546|
|enabled L2|462453|154151|154174|154170|4|154108/154130|15502|

The4 pending cycles are the original next-frame operations
80373AB4/AB8/ABC/AC0. They have architectural effects but no terminating branch
before the endpoint breakpoint. Their block cost has not retired to downcount;
the debug return discards that local cost. The endpoint elapsed value therefore
equals total table cost minus4. Treating total154290 as the endpoint GetTicks
delta would be wrong even in this fully checked source profile.

The source tick tuple at first-clock is28D70D->2B3193 for zero/paused, or
28D70D->2B311F for enabled. The first-clock interval is a complete retirement
boundary with no pending block cost. In the zero route, extra40 cycles beyond
the154206 operations are24 mtspr extra cycles, seven sync extra2 cycles, and
one mtfsf extra2 cycles. This independently accounts for the table-cost sum;
the earlier dense-step154066 delta differs by180 and is not reused.

For all3 profiles the stream validator checks every complete hook/enter/exit
triplet, all fetched DOL words, independently parsed table costs, PC/NPC
continuity, actual branch successors, interpreter block endings, source timing
tuples, and exact next-block elapsed changes. All hook decisions are absent,
all pending exception fields0, and all OC inverses1 in these measured routes.
All3 CRT leaf PC routes/counts match the descriptor-derived formulas exactly.

The independent capture check also verifies the corrected continuous schedule,
sole endpoint, acknowledged entry controls, no clock/BI2/midchain forcing,
unchanged paused pure getter tuples, entry/final source-cycle association,
unchanged epoch/units/RTC-offset, actual sampled cachedTB and the final
unsigned64 offset-added timestamp globals. It does not use the expected TB
value to produce the clock and does not call a host clock.

Instruction stream hashes:

- zero and paused:
  `030d18eb37836fc2b76cdf1cc1c92a8565b6a3be2ef5c9ae59806baf68f68a8b`
- enabled:
  `8959e4d60567d92cdbe6125db5d797292c3e7b8893a07ad464e7847a91714043`

The42d instruction streams are byte-identical to42c. Expanded event callback
records have the following final identities, separately audited by the source
event-owner tooling:

|Profile|Event TSV SHA256|
|---|---|
|zero|`a026cd7fbddf1b24a444b35bffcbf170ecbc2130d7eea1558d7e6d81f5eedd8f`|
|paused|`b5a8a0c8cfde10f1100b7fd0d8b0bc37dd1ec3674d16eb9db7b10ccf52ae3bcb`|
|enabled|`631b2f3ffa66f1dbfccba117a8f22e53fa8f54a4419533903e6475c2bec0a1f3`|

Identical operation work does not erase queued or device effects. These event
identities preserve that independent ownership requirement.

The final42d local event-arithmetic auditor passes479213/479235/478874 checks
and deliberate arithmetic mutations for zero/paused/enabled respectively.
Filtered sequence gaps and unknown callback/device state remain explicit; the
1437322 local checks cannot substitute for the independent queue/device owner.

These are source-validation outputs. Runtime replaying this stream or supplying
its154246/154130 frontier elapsed values would still violate native ownership.

## Read-only audit tool and current conclusion

`tools/agent_timing_adversarial_42.py` pins10 local source files and the original
DOL, gates69 raw words, independently parses the relevant source cost tables,
and checks the finite19/10/3 zero-fill and5/item relocation formulas. It
demonstrates22 counterexamples to proposed shortcuts, including all11
nonzero phase residues, debugger cost loss, nonintegral overclock truncation,
FIFO ordering, pending-exception insufficiency and eager-fill visibility.
It can check local clock-read/retire/step-reset arithmetic in the original
event TSV format, reject arithmetic mutations and report filtered gaps.
It never admits a connected clock from those trace values.

`--instruction-trace` performs the stream checks above. Optional repeated
`--capture` arguments pair42d provenance/source outputs with those streams.
Both `--dol` and `--source-root` require explicit read-only paths; there are no
machine-specific CLI defaults.
The3 complete runs reject4910/4910/4290 local raw-word, table-cost, block,
successor and source-timing mutations, **14110 total**. The deliberate
`--first-clock-only` option labels a complete bounded interval and cannot be
paired as full endpoint evidence. No coordinator validator is imported.

### Mandatory gate status for this independent audit

|Gate|Status|
|---|---|
|Reference capture|Complete3 passive source profiles; malformed first capture rejected; source scope and endpoint explicit|
|Semantic reconstruction|Exact cost/block arithmetic and large loop formulas independently reconciled; connected native scheduler/device owner remains unresolved|
|Perturbation/falsification|22 finite shortcut counterexamples;14110 trace mutations rejected; paused stream identical and alternate L2 cost explained|
|Checkpoint parity|Initial/final clock source tuple, sampled cachedTB and timestamp globals match; this audit does not claim full native architectural/memory parity|
|Full repository + gameplay regression|Coordinator-owned gate; this audit does not waive it or infer a pass from source arithmetic|

**Current dependency is genuine:** original uninterrupted prefix table work,
its retirement and absent active hooks are now observed and independently
reconciled for these two source paths. A connected native semantic work owner,
complete event/device state/consequences and physical Gekko elapsed equivalence
were the next gates at the original trace-only handoff. The finite native work
producer is now implemented and executed successfully; see
`TIMING_NATIVE_WORK_42.md`. Its source-derived scalar sums do not consume the
observed frontier count. Complete clock/event-chain admission and physical
equivalence remain separate gates, and source cost agreement does not promote
research41's measured-cycle input into a live native provider. This independent
audit retains the connected frontier before80379628.

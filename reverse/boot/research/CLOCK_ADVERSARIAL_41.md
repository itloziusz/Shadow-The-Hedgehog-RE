# Independent clock / immediate OS successor audit

2026-10-01. Original PAL GUPP8P DOL SHA256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
The original input remains read only. This note and the accompanying
`tools/agent_clock_adversarial_semantics.py` were prepared independently of
the coordinator's clock implementation and validator. No connected progress,
reference parity or hardware equivalence follows from these isolated checks.

**PROVEN:** 71 exact original instructions and their relevant bitfields;
the bounded arithmetic, stack and branch consequences below. **UNKNOWN:**
retail timing, asynchronous behavior, and any production state that has not
been derived and compared against a same-run reference capture.

## Independent raw and arithmetic gate

The tool parses the DOL header independently and requires its complete
SHA256. It rereads the six sampler words, complete 25-word clock helper,
five-word EE disable, nine-word EE restore, five following caller words,
complete 16-word source-pointer helper and five-word low-TB delay. It checks
TBR encodings, destinations, addc/adde XO/Rc/OE, unsigned source-pointer
comparison and direct CFG from raw fields, then cross-checks the decoder.

The gate passes 9,604 isolated arithmetic cases over seven boundary values
per input word and four XER seeds. Wide 64-bit arithmetic independently
checks the word carry chain, modulo64 result and preservation of every XER
bit except CA. Separate probes check signed failed sampler comparisons,
lower/full64 rollover, production-cycle division, current-MSR restoration,
stack aliases and pointer predicates. These probes contain no PPC execution
engine and call no host clock. Their synthetic production inputs are clearly
separate from a connected original reference run.

```powershell
python -B reverse/boot/tools/agent_clock_adversarial_semantics.py <PAL-main.dol>
```

## Exact continuation from the checkpoint40 entry

Checkpoint40 stops before `80379628`, after the existing connected clock
prologue and EE clear. Required entry includes SP=`8060C5B0`,
LR=`8037966C`, r31=the prior EE bit, cleared current MSR, the actual owned
stack, retained XER and CR fields, and a proven time-base producer state.
The repeated PCs below are instruction states, never one deduplicated row.

| PC | Raw word | Effect |
|---|---|---|
|80379628|7C6D42E6|Read current TBU into r3; TBR269.|
|8037962C|7C8C42E6|Read current TBL into r4; TBR268.|
|80379630|7CAD42E6|Read current TBU into r5; TBR269.|
|80379634|7C032800|Signed32 compare r3,r5 into CR0, copying XER.SO.|
|80379638|4082FFF0|Retry **all three reads** at28 when CR0.EQ=0; CTR unchanged.|
|8037963C|4E800020|Return through actual LR, aligned to four bytes.|
|8037966C|3CC08000|r6=`80000000`.|
|80379670|80A630DC|Read offset **low** word at`800030DC` into r5.|
|80379674|800630D8|Read offset **high** word at`800030D8` into r0.|
|80379678|7FA52014|addc r29,r5,r4; writes low sum and replaces CA.|
|8037967C|7FC01914|adde r30,r0,r3; includes low carry, replaces CA again.|
|80379680|7FE3FB78|r3=saved priorEE.|
|80379684|4BFFCAC1|Call EE restore at`80376144`, LR=`80379688`.|

After each half read, the inspected private oracle routes through
Interpreter `mftb -> mfspr`, writes the **complete newly produced64-bit
counter** into its TBU/TBL register mirrors, then selects the requested
destination. A checkpoint that captures those SPR mirrors must compare
both halves. This mirror behavior is a private-oracle implementation fact,
not an assertion that hardware reads modify its own counter.

The stable loop discards every unsuccessful low sample. Failed signed
comparisons differ from unsigned comparisons at the high-word sign boundary:
`7FFFFFFF` compared with `80000000` is GT; `FFFFFFFF` compared with0 is LT.
Both outcomes retry, but their intermediate CR states differ. Equality alone
does not prove the low word belongs to a stable high interval if the provider
permits a complete high-word cycle between the first and third reads; a
bounded production timeline must exclude that or explicitly model it.

## Addition, restoration and exit state

The offset is two architectural bit patterns. This slice has no sign test on
it: two's-complement signed and unsigned interpretations give the same
modulo64 result. It does **not** prove a calendar epoch or offset sign convention.
Do not use overflowing signed C++64 addition. Both arithmetic instructions
have OE=0 and Rc=0; they replace CA and preserve all other XER bits and CR.

```text
lower = uint64(T_lo) + O_lo
r29   = uint32(lower)
upper = uint64(T_hi) + O_hi + (lower >> 32)
r30   = uint32(upper)
CA    = upper >> 32
```

The restoration leaf executes a signed `cmpwi priorEE,0`, reads the current
MSR into r4, sets/clears only EE in r5, installs r5, then returns the EE bit
that was in r4. The caller overwrites that returned r3 with the high sum.
An original full-MSR snapshot is not a replacement for the current-MSR read.
The priorEE input is proved0/1 on this connected path. Consequently return
CR0 is EQ+SO for0, or GT+SO for1; it is **not** the stable sampler EQ result.
The restore leaf preserves arithmetic CA.

| PC | Ordered epilogue consequence |
|---|---|
|80379688|r4=r29, low sum.|
|8037968C|r3=r30, high sum.|
|80379690|r0=owned savedLR from`8060C5D4`, normally`80370EA8`.|
|80379694|r31=owned saved value from`8060C5CC`, normally`80586C40`.|
|80379698|r30=owned saved value from`8060C5C8`, normally`805610F8`.|
|8037969C|r29=owned saved incomingr29 from`8060C5C4`.|
|803796A0|SP=`8060C5D0`; old frame bytes remain valid history.|
|803796A4|LR=r0.|
|803796A8|Return through actual alignedLR.|
|80370EA8|Store low r4 at`805F1F54`.|
|80370EAC|Store high r3 at`805F1F50`.|

At the ordinary `80370EA8` exit, r0/LR=`80370EA8`, SP=`8060C5D0`,
r3/r4=wrapped sum, r5=installed restoredMSR, r6=`80000000`,
r31=`80586C40`, r30=`805610F8`, and r29=its actual saved input.
All other GPRs, CTR, FPRs, FPSCR and earlier SPR owners are retained except
the explicit producer mirrors if that private profile captures them.

Store order is low first, high second although the high address is lower.
An interrupt-enabled observer may see the new low word with the old high
word. A native64-bit assignment cannot silently substitute for this order.
Pending interrupts or FP exceptions at `mtmsr` require an explicit event
ordering proof; unexplained delivery must decline rather than skip callbacks.

## Continue to the next genuine low-memory dependency

Clock validation does not require stopping at its return. These next words
have a fully determined predecessor and must be reconstructed in sequence:

| PC | Raw word | Effect |
|---|---|---|
|80370EB0|4800526D|Call EE disable at`8037611C`, LR=`80370EB4`.|
|80370EB4|387F0050|r3=restoredr31+50=`80586C90`.|
|80370EB8|48002BFD|Call`80373AB4`, LR=`80370EBC`.|
|80373AB4|7C0802A6|r0=LR=`80370EBC`.|
|80373AB8|90010004|Save r0 at`8060C5D4`; overwrites the old clockLR slot.|
|80373ABC|9421FFF8|SP=`8060C5C8`; store backchain`8060C5D0` there.|
|80373AC0|3CA08000|r5=`80000000`.|
|80373AC4|808530F0|**Stop before unknown pointer read** into r4.|

The new backchain at`8060C5C8` aliases the clock savedr30 slot, after that
savedr30 has been restored. A stale separate clock-frame memory view would
fail this transition. Both updates must flow through the same current stack
owner and all checkpoint views.

At this stop: PC=`80373AC4`, LR/r0=`80370EBC`, SP=`8060C5C8`,
r3=`80586C90`, r4=the EE-cleared MSR from the second disable, r5=`80000000`,
r6=`80000000`, and the clock result has already been stored in current CRT
globals. The second disable leaves CR0 and XER unchanged and installs an
EE-cleared currentMSR. The word at`800030F0` requires its own producer.

If its producer later becomes known, `80373AC8 /7C052040` is
**unsigned** `cmplw r5,r4`; `80373ACC /41810010` branches to`80373ADC`
only when`80000000 > pointer`. That arm sets r0=0 and clears exactly the
first destination word at`80373AE0`; the other24 destination bytes stay as
owned prior state. Pointer>=`80000000` calls`800054F4` with r3=destination,
r4=source, r5=28. The comparison proves no upper memory bound or source
extent. Unknown target bytes, wrapped extents, or unexplained aliases remain
separate read/copy boundaries. No automatic28-byte zero block is justified.

## Required producer contract and falsification matrix

The inspected private `SystemTimers::GetFakeTimeBase` expression is:

```text
T = uint64(FakeTBStartValue +
           uint64(GetTicks() - FakeTBStartTicks) / 12)
```

Its GC CPU clock is486,000,000 ticks/second, giving40,500,000 TBticks/second.
`GetTicks` starts with unsigned globalTimer and, only when globalTimer is not
sane, adds `sliceLength - DowncountToCycles(downcount)`. Conversion uses the
current overclock factor. Initialcounter and low-memory offset have distinct
boot producers calling the emulatedRTC source; later TimeBaseSet operations
can rebase counter and core epoch. Their complete order must be captured.
These are local source facts; this audit does not promote them to retail proof.

A faithful C++ provider needs explicit measured **initial production state**,
proved frequency/divisor, unsigned epoch/counter arithmetic, exact admitted
CPU cycle charges and event/slice transitions, and ordered TBU/TBL reads.
It must derive samples from that state. Known reference samples, known
reference return values, per-read referenceCoreTicks, forcedretry counts,
a generalPPC interpreter or hidden mutable guestRAM cannot supply the native
result. A measured initial epoch is an input; an unexplained live tick is a
dependency. Reject any non-proved core timing mode, pending event or rebase.

| Required perturbation | What it falsifies |
|---|---|
|Cycle offsets0/11/12/23/24 from same epoch|Wrong divisor/truncation or host units.|
|Low word`FFFFFFFF` crossing between each pair of half reads|Skipped high recheck, reused failedlow, incorrect retry PC.|
|High sign crossing`7FFFFFFF -> 80000000`|Unsigned intermediatecmpw.|
|Full64 wrap`FFFFFFFFFFFFFFFF -> 0`|Saturating or signedoverflow counter.|
|Two or more failed triples then success|Single retry assumption, repeatedPC deduplication.|
|No successful triple in a bounded production profile|Forced lastsample return instead of exact unknown/end stop.|
|Offsetlower sums0,FFFFFFFF,100000000,1FFFFFFFE|Stale CA, highcarry omission, saturating low sum.|
|Offset`FFFFFFFFFFFFFFFF` withTB0/1|Invented offsetsign semantics or signed C++ UB.|
|Highsum below/at/above100000000 and initialCA0/1|Wrong finalCA or oldCA leakage.|
|XER.SO0/1, OV/lowerbits canaries, all CRfield canaries|Lost preservedbits or wrong restoredCR0.|
|priorEE0/1 and changed currentnonEE bits|Full-MSR restoration or wrong restore-returnvalue.|
|Checkpoint after EA8 before EAC|Atomic pair store or reversed storeorder.|
|Changed savedr29/r30/r31/LR and later alias overwrites|Hardcoded registers, cachedframe view or wrong epilogue offsets.|
|Missingoffsetlow/high, missingF0, unresolved events/rebase|Unexplained reads silently becoming zero/current time.|
|F0`7FFFFFFF`,80000000,FFFFFFFF and copy-owner alias|Signed pointertest, unproved extent or assumed copy direction.|

Every candidate must still complete reference capture, semantic reconstruction,
these falsifications, checkpoint parity of all current owners and ordered
effects, the unchanged earlier chain, and full repository/gameplay regression.
This independent note alone advances no checkpoint.

## External clock and interrupt limits of a replacement

The conversion from production CPUcycles toTBticks is noninjective: with
counterorigin0, `(startTicks,currentTicks)=(100,100)` and`(89,100)` both
produceTB0. At the next coretick101 they produce0 and1 respectively. A
snapshot ofTB alone loses the remainder modulo12, so it cannot seed a
faithful future clock. Dividing a host timestamp into microseconds loses
even more phase: at40.5TBticks/microsecond adjacent intervals contain40 or41
ticks depending on phase. Proven frequency alone does not recover that phase,
initial epoch, scheduled event costs, or original instruction observation times.

The local private source has three emulatedRTC branches. Movie and NetPlay
use their seed seconds plus `GetTicks()/GetTicksPerSecond()`. The local branch
uses currenthostlocalUnixseconds minus a stored RTCoffset; the return narrows
to32bits and subtracts the selected epoch modulo32. A customRTC computes
the offset at boot, then still consults later livehostseconds. Thus two RTC
calls separated by a hostsecond boundary need not agree. Recording one call
as an explicit externalinput is valid; assuming that future calls reuse it
is a different contract and requires proof.

RSP single stepping is a further observation mode dependency. The inspected
Interpreter `SingleStep` advances CoreTiming, executes one instruction, then
sets `slice_length=1` and`downcount=0`; its source explicitly says this path
ignores instructiontiming. The ordinary interpreter runloop retires its
instructioncycle count at block boundaries. Consequently an exact RSP-step
trace can validate a **step-mode research projection**, but does not identify
retail or ordinary free-run sampling times. Removing debugger pauses from
hostclock elapsedtime is also insufficient: the retirement/slice semantics
differ, and mode changes can change the samples while all instructions agree.

There is no currently proved invariant making the clock unobservable. The
helper exposes its absolute pair inGPRs and globals, the other low-TB
consumer tests elapsedticks, and pending interrupt delivery can change PC,
SRR0/SRR1, MSR and subsequent execution. Source `mtmsr` updates MSR, checks
FP exceptions, checks exceptions, and ends the interpreter block. With EE=0,
external interrupts, performance monitor exceptions and the decrementer
remain pending; enabling EE can deliver them. EE=0 does not suppress program,
storage, alignment or FP exception paths. Eventabsence must therefore be
measured/proved, not inferred from EE clear alone.

An externalclock service could eventually replace this dependency only with
a declared input contract proving its origin/phase, exact integer frequency,
rollover, per-read coherence and ordering relative to all admitted events.
If that service uses a host source, an exact transformation and complete
consumer-equivalence proof are still required. An API that merely returns
observed countervalues would restate the missing producer. Until a valid
production/event contract exists, the connected native frontier remains
checkpoint40 at`80379628`; the arithmetic and immediateOS successor above
can be validated as separate research consequences without claiming forward
connected execution.

## Independent review of the research C++ projection

`TimeBaseSemantics.cpp` and its header expressly preserve the unchanged
checkpoint40 native stop. I reviewed their ordered register/CR/XER/global
effects, actual LR returns and later stack overwrites against the raw DOL.
The tail matches the static consequences above. The input frontiercycles
remain observed upstream data, so this is a research consequence rather
than a derived native producer. Earlier unknown BI2/context states preserve
their stops before entering the projection.

The source audit found a real phase error in the initial `step` helper:
advancingcycles **before** applying the read effect substitutes the next
checkpoint `GetTicks` value for the value observed by the instruction. The
correct order is `effect(); ++cycles; save(next)`. This matters even when
most samples fall within a12-cycle plateau. The independent script now
contains a decisive production-input falsification:

```text
RTCseconds = 0
sourcecycles = 100
frontiercycles = 100 + 12*(2^32-1) + 11

read-before-advance TBU/TBL/TBU = 0 / 0 / 1   -> retry
advance-before-read TBU/TBL/TBU = 1 / 0 / 1   -> false success
```

At onecycle earlier (phase10), the correct sampledTBL is`FFFFFFFF`; the
wrong order samples0. These profiles derive all values from the initial
RTC/cycle inputs and do not supply expected TBsamples as producer inputs.
They should be carried into compiled C++ unit/differential coverage.

Two limitations must remain explicit. The `CycleTimeBaseEpoch` initialRTC
origin is `uint32(seconds)*40500000`; adding `uint64(elapsedcycles)/12`
cannot itself reach a full64TBcounterwrap under that restricted domain.
Unsigned sourcecycle subtraction can wrap, and the clock-offset sum can
wrap64bits, but those are different effects. Arbitrary TBrebases through
TimeBaseSet require their own producer contract or a decline. Also the
linear research projection does not execute callbacks/exceptions; reference
validation must prove their absence at entry and every admitted transition,
particularly EE restoration. A label saying "no callbacks" cannot turn an
unknown pendingevent into a proved absence.

The corrected C++ now has the read-before-retirement order, a separate
`CounterTimeBaseAt` arithmetic helper covering full64counter/sourcecycle
wrap, and an explicit optionalpendingexceptions input that rejects unknown
or nonzero state after the validcheckpoint40 entry. The research provider
still uses the declared initialRTC epoch, not an arbitrary rebase or live
native clock.

The independent finite effect ledger was then run on the new pureexception
getter captures `clock-exceptions-zero-41.json` and
`clock-exceptions-offset-41.json`, without importing the coordinator's
validator. Each passes43 ordered transitions and8,901 conservative
field/byte comparisons. These include all32GPRs, signedCR effects,
architecturalXER, MSR/LR/CTR, the complete observed144-byte stack, coherent
clockglobals, fullunchangedBI2 bytes and other captured architectural
owners, before-read productionphase, currentcountermirrors, and no pending
exception at everypoint. Each27 altered/malformedeffect mutation declines.
The old captures have no exceptionfield and deliberately cannot pass.

There is a separate newly observed limit: instructionfetch updates the
private oracle's icachevalid/PLRU arrays during this slice. Valid/PLRU cells
change after28,9680,96A0,3AB4 and3AC0; PLRU also changes after the restore
leaf's6144 and6160. These are recorded as **UNKNOWN separate fetch-cache
effects**, rather than forced unchanged or synthesized by the C++ clock
projection. NativeTraceIO does not publish a reconstructed fetchcache, so
the research comparison must not claim allcapturedbackendfields agree.
The purecache disabledflag remains checked. This does not justify advancing
the connected40 frontier.

```powershell
python -B reverse/boot/tools/agent_clock_adversarial_semantics.py <PAL-main.dol> `
  --capture build/clock-exceptions-zero-41.json `
  --capture build/clock-exceptions-offset-41.json
```

## Fresh independent compiled replay

I replayed the compiled `shadow_boot_clock_research.exe` independently from
four explicit BI2 entry fixtures and their measured initial producer inputs.
No coordinator validation module was imported. The saved outputs equal the
fresh executable outputs, and a separate parser preserves the complete
ordered checkpoint and store records.

| Reference profile | CP | Architectural fields | Known stack bytes | Clock fields | Ordered stores | Current global bytes |
|---|---:|---:|---:|---:|---:|---:|
|Exception-aware original offset|43|4,988|4,472|215|4|491,272|
|Exception-aware offsetFFFFFFFFFFFFFFFF|43|4,988|4,472|215|4|491,272|
|Controlled full64 counter wrap|48|5,568|4,992|240|4|491,272|
|Controlled high-word signed-boundary rollover|48|5,568|4,992|240|4|491,272|
|**Total**|**182**|**21,112**|**18,928**|**910**|**16**|**1,965,088**|

All compare exactly. The stack validity mask is derived independently from
the admitted earlier stores: words atC570, C578..C594, C5B0, C5C0..C5EC and
C5F4, with the paired view atC5E8..C5F7. Later writes alias already valid
words. Unknown bytes retain invalid masks and are not promoted by matching
incidental reference zero bytes. The complete three current CRT global
owners are reconstructed from their proved fills plus the actual metadata,
guard, debug flag and ordered clock stores; all491,272 bytes agree.

Each compiled output rejects131 changed register, producer, validity, owner
and store-order mutations. This includes every published numeric state
field in the first checkpoint, the counter phase/mirror and globals,
known stack bytes, validity masks, a global owner byte, store values and
readbacks, and reversing the low/high stores. Together with the54 separate
reference-ledger mutation rejections, these checks falsify stale states,
unexpected aliases and several plausible incorrect clock reconstructions.

The final independently executed four-profile binary SHA256 is
`a1963b2cb6f3b7dcff697b41f295930c7b1c7f13102ebed009de57a3b7f65ff3`.
The earlier two-profile build was
`18d6c247ab3d5d43612c6297ed7e82483efcbb668aa880d77d6ad0bddb26ed40`.
These are research results conditioned on measured upstream timing and
event absence. They do not close the source timing, retail latency,
interrupt scheduling or fetch-cache dependencies, and do not advance the
production frontier beyondcheckpoint40.

```powershell
python -B reverse/boot/tools/agent_clock_adversarial_semantics.py <PAL-main.dol> `
  --capture build/clock-exceptions-zero-41.json `
  --native-output build/clock-validation-41/clock-exceptions-zero-41.research.txt `
  --native-executable build/reverse/boot/Release/shadow_boot_clock_research.exe
```

Repeat with the corresponding `offset` capture/output pair. Fixtures are
read from the matching `.bi2-reference.entry.txt` files; all original and
generated inputs remain unmodified by this independent replay.

## Controlled source rebases and observed original retry branches

The final controlled profiles use a separate experiment oracle that permits
only exact eight-hex-digit writes atPC`80003154`: `Pfa` sets the counter
epoch low word while preserving high, `Pfb` sets high while preserving low,
and `Pfc=00000001` commits `epoch_cycles=GetTicks` and mirrors the complete
counter value. The original DOL instructions, sampler branch and midchain
registers are unchanged. The capture records a declared16-digit
`requested_epoch`, all three ordered control packets, their PC and complete
seven-getter readbacks, plus complete entry state before and after controls.

The independent provenance check reconstructs each partial write from the
original producer tuple. The low/high writes change only the specified
epoch bits; commit changes only the source-cycle origin and counter mirror.
The declared request must equal the combined control words and committed
entry counter. The manifest must identify the separate experiment oracle,
the exact entry-only allowedPC, packet width and three writer IDs. Every
captured point retains the committed source epoch/frequency. Unknown modes,
missing requests, incomplete/reordered controls, altered readbacks,
midchain/per-read writes or additional rebases decline. The preliminary
`clock-rollover-41.json` lacks the declared request and is deliberately not
admitted into the final proof.

Both final profiles naturally execute154,066 source cycles from entry to
the first sampler read:12,838 complete ticks plus phase10. Their first
unmodified attempt therefore samples the last low word before rollover,
then observes the next high word at the third read.

| Profile | Declared entry epoch | First TBU / TBL / second TBU | Failed signed CR0 | Successful pair |
|---|---|---|---|---|
|`clock-wrap-41`|`FFFFFFFFFFFFCDD9`|`FFFFFFFF / FFFFFFFF / 00000000`|LT=`80000000`|`00000000:00000000`|
|`clock-signed-rollover-41`|`7FFFFFFFFFFFCDD9`|`7FFFFFFF / FFFFFFFF / 80000000`|GT=`40000000`|`80000000:00000000`|

Both branches execute the actual retry at`80379638 -> 80379628`. The
second attempt samples all three halves again, returns the newly sampled
low0, and leaves EQ before the later EE comparison. Full64 wrap then adds
the controlled offset`FFFFFFFFFFFFFFFF`; the signed-boundary profile adds0.
The observed carry, restored CR, return pair, ordered globals and later
stack aliases agree with the compiled C++ consequence.

The final reference ledgers total182 transitions and37,674 conservative
field/byte comparisons. The ordinary profiles each reject27 mutations;
the controlled profiles each reject37, adding explicit source-provenance
falsifications. Thus128 reference changes and524 compiled-output changes
decline. The source-rebase acknowledgement is exactly
`research-rebased-epoch`; ordinary captures retain `research-only`.

A useful tooling correction emerged from these profiles: a low-word store
can write0 over an existing0. Reversing an intermediate `[0,0]` pair makes
no actual mutation and cannot test store order. The reference falsifier now
changes the high word prematurely; the compiled ledger independently
rejects reversing the ordered store records. Unknown zero-delta side
effects cannot be dismissed merely because a byte diff is empty.

The generic rebase arithmetic is an explicitly labelled research input
contract. It proves observed rollover and retry consequences under that
contract and remains separate from a native or retail clock producer.

## Downstream time-unit warning

The raw low-TB wait reads r5 once at`80376EBC`, then repeatedly reads r6 at
`80376EC0`, computes `(r6-r5) mod2^32` atC4, and compares **unsigned** with
raw immediate **0x1124 =4,388 ticks** atC8. The retry target isC0, notBC.
Under the inspected40.5MHz private profile that is about108.346 microseconds.
A decimal1,124 threshold, fresh starting sample per iteration, signeddelta,
or missing low-word wrap would all change behavior. This static example is
not a complete downstreamconsumer audit or a proof that its loop returns.

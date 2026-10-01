# Clock production research 42 — a derived C++ path, with the owner gate still closed

## Current execution boundary

**PROVEN for the existing native profile:** production checkpoint40 still
stops before **80379628**, `mftbu r3`. Original sync/L2/CRT/BI2 implementations
are unchanged. Research42 does not promote that frontier.

The new C++ experiment derives elapsed work from original inputs, calls the
unchanged native prefix, then executes the clock slice through **before
80373AC4**, the read of **800030F0**. It accepts no observed frontier cycles,
TB result, instruction-count trace, queue snapshot or host duration.

**UNKNOWN:** ownership of the native pre-entry event/device lifecycle, its
progressive effects and live ingress; equivalence between reference virtual
work and physical Gekko elapsed time. Those are required production gates.
Do not rename `research-produced-work` to a production backend.

Authority remains the original PAL DOL, SHA256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
The four independent notes are:

- [TIMING_SOURCE_AUDIT_42.md](TIMING_SOURCE_AUDIT_42.md): 37 source pins,
  scheduler lifecycle, complete queue replay and bounded callback effects.
- [TIMING_NATIVE_WORK_42.md](TIMING_NATIVE_WORK_42.md): 65 raw-gated semantic
  work units from80003154 through before80379628.
- [TIMING_ADVERSARIAL_42.md](TIMING_ADVERSARIAL_42.md): independent raw/source
  interpretation, whole-stream mutation rejection and counterexamples.
- [OS_TIMER_OWNERSHIP_AUDIT_42.md](OS_TIMER_OWNERSHIP_AUDIT_42.md): original
  apploader work, F0 provenance, ordered copy, alarm/DEC and callback chains.

Latest research42c adds `APPLOADER_ENTRY_RECEIPT_42.md`,
`DTK_OWNER_RECEIPT_42.md` and independent `OWNER_RECEIPT_ADVERSARIAL_42.md`.
Fresh source-bound DTK C++ reaches the undelivered GPU worker effect. Original
Entry/callee receipts now validate a finite frame implementation; neither
component certifies the connected native elapsed/event lifecycle.

## Producer reconstruction

| Producer | Proven bounded consequence | Native implementation / limit |
|---|---|---|
| Fresh source scheduler, CoreTiming.Init and first Advance | Initial slice20000/downcount0 commits20000; initialization is not phase0 after its first advance. | `InitialBootEventOwner` constructs fresh queue/state. Producer-bound DTK now stops before GPU AllowSleep; the original unbound mode stops at the log read. Declared-control completion is conditional; no complete lifecycle is connected to production. |
| Original apploader81200258/278/298/2B8 and copied return thunk812FFF80 | N=2656493 unit steps:19 call counts derived from original bytes, DOL descriptors, devkit10000006 range tests, BSS fill/flush and return thunk. | `ApploaderElapsedWork.cpp` derives N without tracing/interpreting opcodes. It counts semantic work; it does not reproduce every pre-entry store or device callback.31 negative gates pass. |
| Entry80003154 through80379628 | Enabled L2=154130 reference work units; disabled=154246. CRT contributes153636. Live admitted BI2 inputs select their actual path. | `BootElapsedWork.cpp` executes unchanged native prefix and totals65 sealed semantic units;524 raw/data gates,25 paths,557 compiled mutation declines. No observed total or runtime instruction dispatcher. |
| Three reads80379628/2C/30 | TB=RTC_seconds*40500000+unsigned64(cycles-epoch_cycles)/12, modulo64. Continuous getters share one retired phase. | `ProjectClockContinuousResearch` preserves exact sampled values, CR/SO, carry, offset, MSR and ordered stores. Full64/signed rollover research41 remains separately tested. |
| Return/EE/stores through80373AC4 |40 more work units retire after the first getter; four final prologue units have effects but remain unretired at the debugger stop. | This terminal experiment matches the reference breakpoint's discarded partial cost. Uninterrupted continuation must retain pending4; resuming a debugger breakpoint has a distinct forced-step contract. |

The source-specific relation is independently derived:

```text
entry cycles =20000 + original apploader work
first getter cycles =entry cycles + native prefix work
TB =original RTC epoch + floor(unsigned64(first getter cycles)/12)
```

The compiled mode requires `-` in place of a frontier count and epoch-cycle0.
It rejects BI2 debug/array paths without a continuous witness. It deliberately
keeps broader semantic work tests separate from admitted clock connections.
`FreshGcHleApploaderSourceInitialization()` selects audited premises; it is
not a certificate that a native runtime has created those premises.

## Continuous reference captures and differential execution

**PROVEN reference behavior:** copied passive instrumentation observes only
already-fetched instructions, actual HLE decisions/costs and actual exit
state. It performs no extra opcode fetch, state write or clock advance.
The original reference source/library remain read-only. There is one internal
stop, before80373AC4; entry80003154 is the initial GDB pause, not an installed
address breakpoint forcing a SingleStep.

The first capture's buffered, truncated tail was rejected. The observer now
flushes its closed endpoint; the capture helper declines absent, partial or
unbounded instruction/event tails. It records exact observer environment,
source/tool/executable identities and all evidence hashes.

The three complete42d runs are independently audited and then compared against
the compiled producer:

| Profile | Derived N | Derived prefix work | First getter cycles | Stop cycles |
|---|---:|---:|---:|---:|
| L2=0 |2656493|154246|2830739|2830779|
| L2=0, one-second host pause |2656493|154246|2830739|2830779|
| L2=C0480000 |2656493|154130|2830623|2830663|

Zero/paused instruction streams are byte identical. All three getters in
each continuous run see the same counter, unlike the stepped experiments.
The source rate is40.5MHz; its work counter is a reference scheduler choice,
not a measurement of native wall time or physical PPC pipeline latency.

Each run passes:

- 43 complete clock phase/cache/global checkpoints, against the next raw
  instruction's entry phase or the actual terminal snapshot;
- 39 available selected machine-state checkpoints and the full113-field
  terminal state, plus L2/handler/low44:1637 field comparisons;
- 104 previously owned stack/paired bytes at the terminal boundary;
- Four ordered word stores, against both raw semantic expectations and original
  write evidence;
- 348 corrupted-output rejections, covering every terminal field, every clock
  phase/cache/global observation, work totals and ordered stores.

Totals for three runs:117 selected machine checkpoints,129 complete clock
phase checkpoints,4911 field comparisons,312 owned terminal bytes,12 ordered
stores and1044 output declines.17 intermediate prefix observations per run
are inside existing collapsed semantic units and have no individual native
checkpoint. They are explicitly reported as unavailable granularity, not
counted as compared. Earlier dense prefix validation remains mandatory.

Current probe SHA256:
`b79b07f83687a39a3cdb532e90b0c7d5200a865e93ac69389db1e3da34d741e7`.
Current prefix SHA256:
`967edc828b2f94c7a7d2016f1fc0581181d7379b64a7b6dc883c20d8beb74110`.
The final staged source's trailing-blank cleanup rebuilt these artifacts;
all six native/prefix outputs are byte-identical to the earlier5cf06a/2514d5
runs. All three complete audits were rerun against the new executables in
`build/clock-production-published-42`. Historical capture/report identities
are retained separately rather than rewritten.
Passive oracle SHA256:
`84be4d74d3a566b92e46c416d5e9c4ff87f313612fb2f4e529852cb74b0cfe78`.
Private reports/captures live under `build/`; no binary, raw runtime dump or
machine profile is committed. The older research41 profiles and all their
rollover evidence remain unchanged.

The additional fresh42f **initial SI device0=NONE** falsifier changes actual
channel/error/status output while retaining the exact zero-profile instruction
stream, derived N/work, queue/FIFO, endpoint state and all39/43 checkpoints.
Its separate348 output corruptions decline. The original three-profile totals
above exclude this fourth experiment. SI/DVD ranges are added only for this
falsifier: it observes one SI poll-word CPU read at812002E0, tick2479817,
physical0C006430=01EC0000, with no channel/DVD access or watched control write.
Thus controller samples do not influence this bounded clock projection;
their ownership and later consumers remain separate gates. Source audit pins
the43 source files, source configurations and final capture/tool/binary
identities. No observed controller word becomes a constant.

## Queued events are real state

**PROVEN in the pinned fresh reference:** there are seven entry queue items.
Entry->first getter dispatches10 VI callbacks, one GDB poll and one audio DMA
callback. No additional callback enters the final clock slice. VI/audio retain
phase by subtracting lateness; GDB schedules from actual dispatch and therefore
has a six-cycle deadline difference between the L2 profiles.

The earliest missing native ownership is **before the apploader entry**:
the first Advance at G20000 dispatches DTK/FIFO0, GPUSleeper/FIFO1,
DSP/FIFO3, then VI/FIFO2 (due15444). DTK changes pending-block state;
VI0->1 clears the initial PI VI cause and invokes NewField/Movie paths.
The count-only apploader module does not reproduce those transitions.
Its first omitted guest-store consequence is also concrete:
8120025C stores backchain815EDCA8 at815EDCA0;81200260 saves LR0 at815EDCAC.
Counting those instructions proves their work, not memory ownership.

**STRONG conditional:** bounded callback predicates explain the absence of
additional guest exceptions in the42d interval. DSP DMA stays disabled, ROM
update has no guest effect, DTK playback is off, and VI half-lines173..182
cross no field/poll/interrupt comparator boundary. PI cause10000 remains
distinct from mask0 and pending CPU Exceptions0. The watch excludes CPU
accesses only in its four bound DSP/VI/PI/AI ranges; it is not an SI/DVD or
external-writer exclusion. Source audit gives every predicate and writer.

**UNKNOWN native:** no complete event/device owner yet carries initial VI/PI, DSP-ROM,
DVD pending blocks, GDB ingress and SI poll state through native entry and
progressive work. SI polls once at half-line15 before entry and samples
host/controller/config state. Its channel outputs may remain symbolic until
consumed, but its control/mask/EN3 state must actually be owned and its first
consumer must decline unexplained channel data. Copying observed seven-item
queue contents would not solve this producer dependency. The initial NONE
experiment now falsifies controller-output influence through80373AC4; it does
not create a native SI control/configuration object.

## Research42b: finite first-event owner and its exact stop

`InitialBootEvents.hpp/.cpp` implements a separate finite C++17 owner for
fresh initialization and the first Advance. It consumes declared configuration
and branch/effect bindings, never a reference queue, deadline or counter.
The MSVC runner is `shadow_boot_initial_events_tests`; `--dump` runs with
unbound live consumers and `--dump-bound-control` runs separately declared
controls. Neither path is called by the production boot runner.

**PROVEN bounded source operation:** first Advance commits G20000 and
dispatches `FinishExecutingCommand`, due0/FIFO0, with DTK userdata. Transfer
length0 raises no DI interrupt; pending blocks become0 and the zero-sample
request reaches `Mixer::PushStreamingSamples`. The unbound C++ owner stops
**before Mixer.cpp:253 reads `m_log_dtk_audio`**, retaining the active DTK
callback and prior effects. It schedules no DTK successor or later callback.
This live log flag and WAV-writer lifecycle are **UNKNOWN** in the existing
direct observations. A constructor's false default cannot certify live ingress.

**STRONG conditional completion:** with separately declared log-off, GPU
delivery, inactive Movie, no frame-step and absent achievement client/DLL
controls, DTK/GPU/DSP/VI execute in due/FIFO order. The next slice/downcount
is10888; the heap and FIFO10 are produced by callback scheduling, not copied
from a capture. Movie/PI/DSP private state and GPU effect delivery remain
conditional source-derived fields rather than claimed observed parity.
An unknown binding stops before consumption; a stopped capsule refuses
resumption with replacement values. A throwing GPU sink preserves the
earlier DTK effects and leaves GPU active/undelivered with no successor.

The strengthened `agent_initial_events_42.py` pins51 sources, the original
library, capture tool, copied Interpreter, oracle and disc. It runs the complete
37-pin lifecycle/instruction/MMIO/environment/source-location/closed-tail audit
before comparing any subset. All three42d references pass:

| Scope per profile | Ordered records | Exported observed fields |
|---|---:|---:|
| Unbound prefix through DTK dispatch |11|84|
| Declared-control complete first Advance |26|200|

The latter fields validate exported events and heap consequences, not unseen
device interiors. Each profile rejects52 native-output corruptions and14
malformed capture bindings. Root additionally tests consistently wrong oracle
hashes and non-object manifests; equality between supplied hashes is not
identity. The old subset-only report is historical, not the admission gate.
Root's full MSVC report is `build/initial-events-final-42.json`, executable SHA
`2a3fc25920c3ef5c5fe323f5cfbd87d4cbf1a9ec70ebf52be4cde7f6a3407d81`.

Independent validation in `INITIAL_EVENT_ADVERSARIAL_42.md` pins30 source
gates and checks60 rows/480 exported fields per reference. Literal padding
is included in that count, not480 independent device observations. It rejects
1620 trace corruptions/deletions, six malformed closures and535 compiled
export/journal mutations. Two different Movie control states produce the
same observed queue; this is a concrete counterexample to treating queue
parity as complete side-state equivalence.

`APPLOADER_ENTRY_OWNER_42.md` independently traces the original region2
producer to SP815EDCA8 and LR0. Its three raw words predict backchain and
LR stores at815EDCA0/CAC. Existing42d snapshots are after all19 callbacks,
so their reused stack cannot prove the first Entry stores' order or inputs.
The note specifies the minimal passive first-three receipt. The report/HLE
stub is installed after Entry returns and before Init, not an Entry precondition.

## Research42c: producer-bound DTK and original Entry frames

### Identification and original captures

**PROVEN in the finite original receipt:** the first-three Entry8 state rows
and nested18 rows observe the actual SP/LR/PC/NPC, mapping/cache and backing
bytes, with two plus eight ordered physical stores. Source.Clear produces
the initial stack bytes; the actual apploader copy produces the instruction
bytes. Zero-valued LR/stmw stores have separate write receipts, not just
unchanged bytes. The strengthened independent observer inverse, saved
compile/link recipes and265 inherited inputs are checked.237+628 receipt
mutations/omissions reject. Later stack reuse is no longer the only evidence.

**PROVEN finite DTK source ancestry:** copied passive constructor, configuration,
Start/Stop/writer/exit and read receipts bind the same Mixer lifetime. The
fresh off profile produces false from construction and absent config layers;
there is no changed Set or mutable DSP lease before consumption. A separate
initial-DumpAudio control executes the actual successful Start writer and
then consumes true even at sample count0. Both captures retain1291 exported
queue/device records and the original exact instruction stream. Independent
154+188 receipt mutations reject. General racing host ingress and WAV lifetime
are not covered by those observations.

### Executed semantic reconstruction

**PROVEN bounded source reconstruction:** `WithOwnedDtkLogging` takes lifecycle,
configuration origin and excluded-ingress premises, never a flag sample.
Native construction produces backend48000 and false. The logging-off path
retains G20000 and DTK successor1699488/FIFO6, pending6, then stops before
**BlockingLoop.h:233 undelivered-allow-sleep**. GPU remains active; no DSP/VI,
Movie, NewField, achievement or guest-store completion is fabricated. Unknown
config/alias/lifetime stops before Advance; unknown Start/Stop stops at the
actual consumer after prior zero-transfer/sample-request effects.

Enabled startup config first reads **System.IsAudioDumpStarted at
AudioCommon.cpp:78**. That owner is missing, so the C++ enabled path stops
there. The earlier suggested WAV-start boundary was too late: host time,
LocalTime, path creation and WAV Start lie before the flag's successful writer.
None is silently skipped. The new native executable SHA is
`312a33d24aa975b57ab9e589dace78c4ee077ad0ff15125f394146495ea43ca9`.
The original unbound/declared-control dumps remain exactly4232/9030 bytes.
All three complete earlier event-admission/parity audits rerun against it.

**PROVEN finite raw-gated frame effects:** `ApploaderEntryFramesResearch`
constructs private64-byte clear/code/register state from original apploader,
boot and BI2 inputs plus explicit conditional source prerequisites. It executes
11 words/10 stores through **before812003B8**, carrying live LR, saved registers
and arguments; altered live values change the actual spills. It has no runtime
opcode dispatcher, clock or scheduler. Its native connector rejects every
current first-event owner, including declared-only Complete. Selecting the
research source profile cannot grant admission.

The frame executable SHA is
`07075d9cbe4440e9d78e5da10c0227bb37a893304489f1905cc0840702c9096b`.
22 compiled enter/inner-exit snapshots and10 ordered stores agree with the
receipt/source ledger. Dynamic receipt fields and source-only retained reset
fields are reported separately; no missing timing field is counted as parity.
The source-defined Reset of paired lanes, segment/GQR/CTR, architectural CR,
XER/FPSCR and reservation is independently checked. The original receipt
does not separately observe every one of those reset fields.

### Falsification and regression

Native DTK tests pass41 declines; frame tests49. Strict typed author comparators
reject769 off/495 enabled DTK mutants and6066 frame mutants; independent
comparators reject992 DTK and3410 frame mutants. These include field changes,
numeric-bool substitutions, keys, list arity, missing rows/stores and wrong
mode/provenance. Explicit journal padding is checked but is not a separate
device observation. Python's `False == 0`/`True == 1` originally weakened the
independent output diff; exact recursive types and reusable-validator mutation
tests now close that genuine tooling bug.

After these owners, all three complete native entry-to-clock comparisons
rerun under `build/clock-production-owned-slices-42`:117 machine/129 clock
checkpoints,4911 fields,312 owned bytes,12 stores and1044 output declines.
Clock/prefix artifact hashes remain the b79b07/967edc identities above.
Final full MSVC Release **66/66**, standalone gameplay **27/27**, exact required
Dark mission ->stage index6. No frontier, first-frame or pixel promotion.

### Genuine remaining dependency

**PROVEN source consequence, UNKNOWN connected owner:** AllowSleep sets the
actual live BlockingLoop flag; TestAndClear permits its worker to move from
DONE toward SLEEP. A dummy local atomic leaves that worker busy. Wakeup later
follows an async closure push, so these states can change dispatch ordering in
the intervening interval. The worker pulls AsyncRequests before its paused
check; CP publication is another possible effect under live input changes.
This does not assert a particular observed interleaving, lost work, or that
fresh initial CP necessarily interrupts. Queue equality cannot prove a closed
worker lifetime or all ingress.

The minimum next receipt is the actual instance/lifecycle, existing store and
consume results, worker transitions, async push/pull ancestry and CP interrupt
publication. It must reuse existing values rather than add atomic reads or
force writes. Movie/frame-step/achievement lifetime then remains separate.
For the finite frame slice,812003B8's lis is already raw-known; the next unowned
range is callback publication at812003C4/D4/E4 into80003100/04/08. Stack-only
storage cannot admit that effect. Progressive memory/SI/event ownership and
physical work-to-time equivalence still keep production before80379628.

## F0, alarms and later OS chain

**PROVEN in the fresh GC source profile:** MemoryManager.Init/Clear produces
zero at800030F0 before boot. The tempting explicit store in the Wii setup is
not the GameCube producer. Admitted original apploader paths read F0 but do
not write it. Later DOL reboot/transfer writers80373EA4/3F6C are not the first
OS call's producer. Restored/retail/nonfresh ownership remains UNKNOWN.

**PROVEN binary semantics, not executed continuation:**80373AC8 compares P
unsigned with80000000. A lower P clears only destination word0 at80586C90;
a higher P invokes800054F4 for28 ordered bytes. Copy direction uses original
unsigned guest addresses, so canonicalizing an uncached alias before choosing
direction can diverge from the original writes. Alarm creation clears two
words only; signed64 deadline insertion is stable for equal deadlines,
periodic requeue precedes callback, and the DEC setter at80370B0C has exact
negative/low/saturation behavior. Callback/vtable ownership and live dispatch
remain gates. See the complete independent address ledger rather than
inventing defaults from these summaries.

## Bugs fixed and regression

The new diff initially searched all native checkpoints for a missing prefix
PC and borrowed a later call with that same PC. It falsely compared LR80370EB4
with803733A0. Region-bounded occurrence matching fixes the checker; a dedicated
regression prevents recurrence. Original assembly/native prefix was correct.

An independent reviewer also found the research CLI used `.back()` on an empty
BI2 checkpoint vector after an earlier unknown CRT handler. It now declines
cleanly, with a compiled regression. No fabricated checkpoint is emitted.

A fresh NONE capture initially failed before GDB readiness and was rejected.
Windows explicit binding then showed that an automatically assigned bind(0)
port could be excluded for Dolphin. The new helper probes explicit loopback
ports, skips excluded/busy candidates and declines when none is available.
Three regressions cover automatic/requested success and all invalid/blocked
cases without binding0; the produced-clock/tool suite now passes15/15. Original
base capture source remains byte-identical and older artifacts keep their
original tool identity. The final42f capture uses automatically probed port30000.

The recognizer also learns the nine raw words at80373AC0..80373AE0: global
load, unsigned high-address branch, count/call and partial word-clear arm.
Signed-compare, operand, branch, call and store decoys decline. Whole-text
rescan finds one such motif among1,216,776 words; its database status stays
UNKNOWN, with pointer extent and callee behavior explicitly unresolved.
The full raw/symbolic/proof recognizer suite passes37/37.

The event owner's first draft used exception mask10 rather than the source's
EXCEPTION_EXTERNAL_INT=4. Zero-state parity could not expose this bug; the
corrected helper now sets/clears bit4 while preserving bit10 and other flags.
The VI edge note also omitted odd_even_psb_diff, making even boundaries off
by one; corrected520/519/1045/1044 values have native assertions. These fixes
change no earlier validated sync/L2/CRT/BI2 path.

The original-input comparison now pins the whole PAL DOL, not only consumed
word ranges. A test changes an unconsumed last byte and proves identity
rejection. Final clock parity was rerun for all three references after this
gate; reports live under `build/clock-production-final-42`.

Research42b MSVC Release repository regression: **65/65**; standalone gameplay:
**27/27**, including exact
`RESULT: Dark mission cleared -> next stage index 6 (stg0200)`.
The standalone invocation first rejected a missing data-directory environment;
rerunning with the explicit read-only `SHADOW_GAME_FILES_DIR` passed.
Runtime, parity/decline and existing pixel requirements remain unchanged.
No first-game-frame or new pixel-parity result is claimed.

## Reproduce and continue

Run `validate_produced_clock.py --help` for explicit DOL, compiled probes,
capture, pinned source/library and private build-output arguments. It invokes
the independent source audit first, then runs both C++ paths, compares every
available state/effect and falsifies348 outputs. It never feeds a reference
frontier count into C++ and never promotes production.

Next close the **actual GPU worker/async/CP lifecycle** with producer-bound
passive receipts and an admitted exact effect owner. Then bind Movie/frame-step
and achievement lifetime without promoting declared controls to observed state.
Fresh DTK and original Entry/callee receipts are now obtained; reconstruct
progressive low-memory callback publication and the later RAM producers.
Separately prove SI control/masks and preserve unknown channel values until
their consumers; the standard-controller->NONE input-leak falsifier is now
complete for this bound. Then
replay the whole original prefix and carry pending work correctly beyond
80373AC4. F0 zero may be admitted only from an owned fresh-GC lifecycle;
unsupported pointer extents and timer/callback state must fail at consumption.

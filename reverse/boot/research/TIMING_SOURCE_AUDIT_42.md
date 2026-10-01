# Timing source audit after clock research 41

This audit traces the pinned HLE cycle scheduler, startup phase, queue order and
existing observer. It provides predicates for a reference capture; it does not
admit elapsed cycles observed at a frontier as a native input. Original oracle,
game and toolchain sources remain read only.

## Source authority

Paths are relative to `<matching-oracle-source>/`. The clock audit41 already
pins the producer, interpreter, boot, RTC, hardware initialization, physical
write observer and event observer. `agent_timing_source_42.py` checks those
fourteen pins plus the following twenty-three. Its `--source-root` and `--library`
arguments are explicit; public files contain no machine paths.

| Additional source | SHA256 |
|---|---|
|Source/Core/Core/CoreTiming.h|fdfab0fbb4bdc953a946f746012636b6005a96d875d82ad3263543cacaa7ea7b|
|Source/Core/Core/OracleEventAudit.h|781b5f3e344877582d2e7b4506d9fc0f7c12e1ada672fe75fc9c7b9f432a036d|
|Source/Core/Core/HW/VideoInterface.cpp|f1dc11d057c8a52e47316580713d27d9e8f6458e84726f45e7c648bdbfd92295|
|Source/Core/Core/HW/AudioInterface.cpp|bc698352d4276b63ba99629adf0987b8e8e252b5034ef76e4701717709d76b71|
|Source/Core/Core/HW/DSPHLE/DSPHLE.cpp|e1059fccb22935e2e71dd93090836e1358c0fd8dc9e087b3ebf4da7b2122cc8c|
|Source/Core/Core/HW/DSPLLE/DSPLLE.cpp|5520fe7ad302db9cba7962ac589601cebff881e4804fc75f672eca32a1402f8e|
|Source/Core/AudioCommon/Mixer.h|612025371230d433dcf62e027d1abd2da8be5c3533a8ed2947a0e3b6f9480bd5|
|Source/Core/Core/PowerPC/PPCTables.cpp|4814b6c63088c0227eadf2fefa4f32fe2109afde7319b88697b35a7b67dcf771|
|Source/Core/Core/PowerPC/Interpreter/Interpreter_Branch.cpp|ac92c3fc9f5d05b7bc6fea7a39bdb0682c13d80223fc8ca88131a55a6df29068|
|Source/Core/Core/ConfigManager.cpp|3418e4660cfd4baf1d8d570e6dd7bc18779499c7310ad45684da1914413b1912|
|Source/Core/Core/HW/DSP.cpp|110d30677b3bf63e0bd8043377a566d7c3625f312ed76c09824f08796b2e62dd|
|Source/Core/Core/HW/DSP.h|9e98d77bd6af4d462a39d75e0532102b3fc942d95e76b8400977f2ca53b2aecc|
|Source/Core/Core/HW/DVD/DVDInterface.cpp|b017c9ce65bd788be55c3310d2a0f4cb8687f2a3a7f0aa674131548e45997da1|
|Source/Core/Core/HW/ProcessorInterface.cpp|62310e34368b1d61fff9b9dee7848ab8f1e0125edef4015f8ab3cd3fbb7e6186|
|Source/Core/Core/HW/ProcessorInterface.h|3dc6a462d4de00429cffcec4b6c3464ced1cf24a27d02a6c27ffa2ab5fb73763|
|Source/Core/Core/HW/MMIO.h|7168d2bd96224d29d85e99e8972aa991ae5608136e82ce3eab25fb730649d94c|
|Source/Core/Core/PowerPC/GDBStub.cpp|c2e3ae4869d41eb212bf9fb08940958f17939a4a2f19d5354ec4e386d74ba35e|
|Source/Core/Core/Boot/Boot.cpp|2245b1ab41fa4ab092d26416016c05d52b5591c9035f2ed4b1e3ffe8d256daaa|
|Source/Core/Core/HW/DSPHLE/UCodes/ROM.cpp|16900823dc3d2b70f5413b030869e0507f3f68a3e6a8755101b94b3b00369814|
|Source/Core/AudioCommon/AudioCommon.cpp|de119dfd09a5f10309bbdd0b2de83f5666a8c2f413bf87143ab3322d7b527430|
|Source/Core/VideoCommon/Fifo.cpp|971d31caf503e18e9435202aa7f576a0ab281630a127e22a42063ab30795a15e|
|Source/Core/Core/HW/SI/SI.cpp|4dd9a3b22f3610d7ffe7704b6ee5b4142f29ad6e932c391edc094b22ac6abae7|
|Source/Core/Core/HW/StreamADPCM.h|f1159e7c6df1c4058e6854ad2dd32640ab4d970f34875283bf5afe90f7b395b0|

The linked producer remains
`<matching-oracle-build>/Source/Core/Core/Release/core.lib`, SHA256
`eb0e5041b253072d735ddaac5fba9079ed2ca4916f26312845c706bd132b2939`.
Source and library hashes establish identity. Actual emitted source locations
and equations must also match the executing reference to establish their
correspondence.

## Exact scheduler recurrence

Let G be signed global_timer, L be signed slice_length, D be signed downcount,
I be the stored float inverse overclock factor, and S be global_timer_sane.
`CoreTiming.cpp:58..65,247..256` implements:

```text
DC_cycles = int(float(D) * I)    // float multiplication, truncation toward zero
ticks = u64(G)                  // S=true
ticks = u64(G) + (L-DC_cycles)   // S=false
```

The private GC default proof must establish I=3F800000 and configured factor
and inverse=3F800000. Then DC_cycles=D exactly for the bounded small counts
here. Non-unit factors need exact float conversion, range and scheduling
proof; the new audit declines them.

`CoreTiming.cpp:376..424` Advance first checks configuration, imports foreign
events using the OLD global phase, then commits G += L-DC_cycles. It refreshes
the factors, sets L=20000 and S=true, and dispatches all events whose deadline
is at most G. A callback can enqueue another event already due; the same while
loop dispatches it. After all callbacks, S=false and L is the smaller of20000
and the next event's deadline minus G (20000 for an empty queue). D becomes
int(float(L)*factor). External exception handling follows these mutations.
The timer getter does not advance this recurrence.

`CoreTiming.cpp:343..356` ForceExceptionCheck clamps the requested relative
cycles to zero. If DC_cycles exceeds that request, it subtracts their
difference from L and replaces D with the converted request. At factor1 this
preserves G+L-D and therefore current tick phase. It changes when the next
Advance happens. Idle at `:613..629` sets D=0 after any required FIFO flush;
that consumes the rest of a slice, rather than importing host elapsed time.

## Startup phase has a real producer

`CoreTiming.h:197` initializes m_last_oc_factor=0.0f.
`CoreTiming.cpp:90..118` calls CyclesToDowncount(20000) BEFORE RefreshConfig
and before assigning that factor. For a fresh instance this yields D=0,
L=20000, G=0 and S=true. Init's stored inverse is still zero; its initial sane
GetTicks observation is zero without using the inverse. Init-ready follows
configuration and has inverse1 while D remains zero.

The first Advance therefore commits20000 cycles. This is a source consequence
that requires the init/init-ready/first advance-enter/advance-clock records;
it must not be chosen to fit a later getter. The 20,000 increment has phase8
modulo12. A reused scheduler object requires a separate lifecycle proof.

`HW/HW.cpp:34..53` runs CoreTiming.Init, clock PreInit, device initialization,
CPU.Init, then SystemTimers.Init. Clock PreInit selects GC486MHz and may adjust
pending event deadlines. The early queue is empty only if capture proves it.
CPU Reset clears GPRs/SPR caches and rebases TB; it does not reset the already
initialized CoreTiming slice/downcount in `PowerPC.cpp:145..204`.

`Boot_BS2Emu.cpp:64..75` executes apploader functions using SingleStep until
PC=0. Their entry/init/main/close call chain is at `:176..235`; repeated main
calls depend on original apploader-produced section requests and synchronous
DVD reads. `Interpreter.cpp:204..218` advances first, executes one instruction,
then sets L=1,D=0 while ignoring opinfo cost. After N uninterrupted boot steps,
the pending getter count is 20000+N: the first step commits20000, each later
step commits one, and the last reset exposes one pending cycle. This formula
does not establish N. Original apploader bytes, instruction paths and HLE
hook results must produce N independently.

The observer's compressed boot count can falsify a derived N. It cannot
supply N as a production input. GDB pause duration has no place in this
formula.

## Continuous execution and debugger loss

`Interpreter.cpp:257..302` sums each SingleStepInner opinfo cost in a local
block accumulator, then subtracts it from downcount at block end.
`PPCTables.cpp:43,55,81,375` gives bc, addi, stw and mftb one cycle; the full
table contains other costs. A native cycle producer needs every executed
opcode's admitted cost and every block ending condition, including hooks and
exceptions. `Interpreter_Branch.cpp:69` ends a block even for an untaken bc.
`Interpreter.cpp:344..347` also ends blocks for exceptions.

In the debug loop, a breakpoint hit at `Interpreter.cpp:279` immediately
returns before the retirement subtraction at `:284`. The current partial
block sum is discarded. Resuming at a breakpoint forces one SingleStep
(`HW/CPU.cpp:137..145`) before RunLoop. Thus internal checkpoint stops can
change elapsed cycles and phase. The observed retirement sum is correct for
that debugger schedule; transferring it to uninterrupted gameplay would need
equivalence proof.

All three TBU/TBL/TBU reads in one uninterrupted interpreter block observe
the same retired cycle state. A stepped schedule can cross a TB boundary
between reads. Capture labels must retain that distinction.

## Queue production and ordering

`CoreTiming.h:59..75` orders Event by signed deadline, then unsigned FIFO ID.
Name/userdata do not break ties. At `CoreTiming.cpp:270..318`, CPU scheduling
sets deadline=GetTicks()+signed relative cycles and takes the next FIFO ID.
When outside Advance, it can shorten the current slice through
ForceExceptionCheck before insertion. CPU scheduling order, including
callback rescheduling, therefore determines equal-deadline dispatch order.

Foreign scheduling publishes a relative deadline in a separate queue under
a writer lock. `CoreTiming.cpp:359..373` imports it in FIFO order, assigns a
new main-queue FIFO ID, and adds the OLD G at import. It does not use current
GetTicks. Publish/commit records can interleave with the CPU import because the
observer mutex is separate from the event queue's publication. All foreign
input and its import boundary require original provenance.

RemoveEvent records and removes every item of a selected type at
`:319..335`. RemoveAllEvents first imports foreign events at `:337..341`.
ClearPendingEvents clears only the main queue at `:264..267`.
Clock changes rescale each pending signed deadline about G at
`:595..610`, with C++ division truncating toward zero. These producers must
be replayed before deciding that a queue item is absent.

## Candidate periodic callbacks

Initialization at `SystemTimers.cpp:278..301` registers seven timer types,
but registering an event does not enqueue it. Only the following defaults are
scheduled in GC mode; other devices can add events.

|Type|Initial relative deadline|Next-deadline producer|
|---|---|---|
|GPUSleeper|0|CPU_hz/1000 minus cycles_late|
|DSPCallback|0|selected DSP emulator update rate minus cycles_late|
|VICallback|VI half-line period|current VI half-line period minus cycles_late|
|AudioDMACallback|audio DMA period|current audio DMA period minus cycles_late|
|PatchEngine|VI field period|field-period/pruning result or1000-cycle retry|
|DecCallback|not initially scheduled|nonnegative guest decrementer write times12|
|IPC_HLE_UpdateCallback|not scheduled in GC|Wii-only branch|

Callback implementations are `SystemTimers.cpp:71..161`.
HLE DSP uses CPU_hz/1000 at `DSPHLE.cpp:56..61`; LLE uses12600 at
`DSPLLE.cpp:282..285`. The selected DSP implementation must be established.
The audio period is u64(CPU_hz)*AID_divisor/(108000000*4/32) at
`SystemTimers.cpp:81..85`; GC AI32k yields divisor 3372 and period 121392. The constructor's3375
is overwritten by Init: Get48KHzSampleRateDivisor selects1124*2 for GC,
then Get32KHzSampleRateDivisor multiplies by3/2 (AudioInterface.cpp:348..355).
Audio selection/guest changes are at `AudioInterface.cpp:193..204,328..331`.

VI periods depend on disc region, preset registers and config factor.
`VideoInterface.cpp:112..192` presets HLW429 and field timing, with clock
selected from disc region. `:790..810` computes samples=2*CPU_hz/clock_frequency,
nominal half-line=samples*HLW and divides by configured VI factor. The clocks
are27MHz and54MHz (`:57..60`). Default half-lines are15444 or7722 CPU cycles;
the preset even field is 525 half-lines (`:498..513`). The actual current
period may differ after MMIO/config changes and needs its own source chain.
A first20k Advance can dispatch multiple overdue VI callbacks; cycles_late
preserves their scheduled phase when rescheduling.

This list is a candidate producer inventory. Only a complete queue replay
can state which callbacks enter entry->first-clock. Callback bodies can change
device state and pending interrupts even when they add no cycles themselves.
The timing observer does not establish those device effects.

## Observer schema and bounded capture

`OracleEventAudit.cpp:46..71,358..393` writes sixteen tab-separated columns:
decimal sequence, decimal thread ID, kind, name, eight16-digit hex u64 values,
source file, decimal source line, function, and stack offsets or '-'.
Signed values are exported as raw u64. Filtered sinks use a sequence-zero
event-filter header; later global numbers may have gaps from suppression.
Overlapping sinks must agree for every shared global number. LIMIT means
incomplete evidence and must decline.

Clock-state rows carry v0=G,v1=L,v2=D,v3=S,v4=next FIFO,v5=PC,v6=inverse float
bits; v7 is ticks, relative request, committed cycles or zero according to kind.
Queue enqueue/dispatch/cancel-item/adjust-item rows carry v0=deadline,
v1=FIFO,v2=userdata; v3 is relative request, lateness or zero.
Boundary rows use v0=PC,v1=G,v2=L,v3=D,v4=ticks,v5=MSR,v6=exceptions,v7=PIcause.

The observer at `OracleEventAudit.cpp:78..165` coalesces consecutive retirements
only while their downcounts chain exactly. It compresses only the five-row
step-enter/advance-enter/advance-clock/advance-exit/step-reset pattern with
unit-cycle, stable-factor and stable-FIFO predicates. Any interleaved event,
clock getter or foreign observation flushes it. Boot-steps exports initial G,
count, first/last exit slice, FIFO, first/last PCs and unit flag. Compression
retains timer mutations and aggregate block cost. It does not retain every
opcode or reconstruct instruction paths.

`OraclePhysicalWrites.cpp:112` calls the PPC observer from SingleStepInner's
checkpoint before opcode fetch. `OracleEventAudit.cpp:181..191` has a DOL entry
boundary. The first mftbu selected PPC record appears only when PC80379628 is
executed; merely stopping at its breakpoint does not emit that anchor.

Minimal capture configuration:

1. A startup sink from `init..ppc@80379628` retaining all scheduler, queue,
   config, compressed boot, retirement and boundary mutations. Empty KINDS
   keeps all kinds. A chosen KINDS filter must admit every lifecycle kind in
   the auditor's REQUIRED set. Include VI/RTC source kinds for their producers.
2. A second detailed sink from `boundary@80003154..ppc@80379628`, with an
   explicit adequate limit, to inspect the finite interval without full dumps.
3. Execute the first mftbu once to emit the stop anchor. Preserve binary/source
   pins, filter header, original input/config provenance and capture schedule.
4. Repeat with a controlled pause at entry. Scheduler recurrence and relative
   deadlines should be identical; RTC initializer provenance is recorded
   separately and may differ between boots.

The new helper rejects missing init/entry/first-clock anchors, truncated
streams, conflicting overlap, unexplained clock/FIFO changes, wrong deadlines,
nonminimal dispatches, wrong lateness, incomplete callbacks/cancellation,
unknown savestate producers and non-unit conversion factors. It derives
current phase from replayed mutations, then compares observer getters.
Nine altered synthetic lifecycle cases decline. Synthetic checker tests are
not reference parity or instruction-schedule proof.


## Complete continuous references 42d

Three fresh profiles use original disc bytes, initial HID0=0011C064,
no midchain state writes and a sole internal stop before 80373AC4.
All event windows are init..timing-prefix-end@80373ac4, with an explicit
final record emitted after actual exit80373AC0 and before the breakpoint.
Earlier42/42b files without this anchor do not close the full prefix.

The copied interpreter records original fetched words, returned costs and
exit state without an extra fetch. Original SHA256 is
1993e7a3976be7aa7d276f1e0b25e04bbdcbe1a4555fac837a4794f6bc18128d;
copied observer SHA256 is
355c7217ae52cb5a6b117b5b5d2a565078e9dd713833a23ad2366348f071b85f;
executable SHA256 is
84be4d74d3a566b92e46c416d5e9c4ff87f313612fb2f4e529852cb74b0cfe78.
The auditor now checks37 source pins, emitted source locations, copied
observer identity, capture hashes, queue recurrence and every fetched
opcode's cost through the closed stop.

| Quantity | zero L2 | zero L2,1s pause | L2=C0480000 |
|---|---:|---:|---:|
| Boot unit steps N |2656493|2656493|2656493|
| Entry ticks |2676493|2676493|2676493|
| Entry ticks mod12 |1|1|1|
| Completed opcodes before first mftbu |154206|154206|154108|
| Source cost/retirement before first mftbu |154246|154246|154130|
| First mftbu ticks |2830739|2830739|2830623|
| First mftbu ticks mod12 |11|11|3|
| Closed prefix completed opcodes |154249|154249|154151|
| Closed prefix source cost sum |154290|154290|154174|
| Final block unretired cost |4|4|4|
| Closed prefix retired cycles |154286|154286|154170|
| Stop ticks |2830779|2830779|2830663|

Zero/paused instruction files are byte identical, SHA256
030d18eb37836fc2b76cdf1cc1c92a8565b6a3be2ef5c9ae59806baf68f68a8b.
Enabled instruction SHA256 is
8959e4d60567d92cdbe6125db5d797292c3e7b8893a07ad464e7847a91714043.
Final42d event/MMIO hashes and capture bindings are listed in the
environment-bound section below.

The first DOL instruction runs in the normal continuous loop:
80003154:4800015D is a one-cycle branch ending its block and retiring normally.
There is no entry SingleStep/step-reset. Final boot state isG=28D70C,L=1,D=0;
the getter exposes28D70D. Initial Run Advance commits that pending cycle
toG=28D70D, then the entry boundary is emitted. Zero's409 literal resets
plus 2656084 compressed steps give N=2656493. Unrelated thread observations
change compression grouping, while total N is invariant. The independent
binary/apploader audit now derives N from the original devkit10000006 path,
including range checks80700000/81200000. This count validates that derivation
and cannot replace it.

The closed stop executes four final one-cycle instructions in a partial
block. Their effects are present, but their cost has not reached downcount.
The breakpoint returns before subtraction and discards the local cost.
Resuming from that boundary requires the separately identified forced-step
schedule; this is a debugger-specific state.

## Actual queue and bounded callback effects

All profiles have the same entry queue:

| Event | Signed deadline | FIFO | Userdata |
|---|---:|---:|---|
|VICallback|2687256|214|0|
|GDBStubUpdate|2776493|215|0|
|AudioDMACallback|2792016|213|0|
|GPUSleeper|2916000|195|0|
|DSPCallback|2916000|196|0|
|FinishExecutingCommand|3398976|138|0000000300000001|
|PatchEngine|8108100|5|0|

VI is first due,10763 cycles after entry. Entry->first-clock dispatches10 VI,
one GDB and one audio callback. No further callback enters first-clock->stop.
The complete all-kind streams contain no foreign publish/import. This is
an actual queue replay; Exceptions=0 is not evidence that events were absent.

At first clock, next VI is 2841696/FIFO227 and audio is 2913408/FIFO224.
GDB is 2876500/FIFO222 for zero/paused,2876494/FIFO222 for enabled.
GDB reschedules100000 from actual dispatch GetTicks, so its7-vs1 lateness
causes six cycles of drift. VI/audio subtract lateness and retain phase.
Other pending deadlines/FIFOs remain unchanged.

### VI and PI

VideoInterface.cpp:112..192 presets state, then Boot/Boot.cpp:516..517 presets
again from disc region. Both captures select clock0, although the first
NTSC argument is true: clock selection itself uses disc region.
Final raw HTiming0=476901AD, vertical=6, odd blank=000501F6,
even blank=000401F7. Half-line is 36*429=15444; each field has525 half-lines.

The 183 updates start at0 and advance to 183. The entry interval executes
preupdate states173..182. Source conditions at VideoInterface.cpp:944..1046
exclude field boundaries0/525, active edges520/519/1045/1044 and next SI
poll 999. Only half-line count and last-line-start change. Last-line-start
at stop is 2810808, from update181->182.

Interrupt presets are reg0 HCT430/VCT263 and reg1 HCT1/VCT1, masks on and
assertions off; reg2/3 are zero. The comparator uses postupdate count.
No count 1..183 matches reg0 count 525, reg1 count 0 or a zeroVCT.
The initial VI update clears PI's initial VI cause. Fresh CoreTiming's
throttle-disable flag is false (CoreTiming.h:217), before any throttle.
PI Init sets mask 0,causeRST_BUTTON|VI=10100
(ProcessorInterface.cpp:53..54); after the VI clear cause 10000 persists.
Both emitted boundaries confirm10000. Later host-dependent GetVISkip can
skip an update, but cannot create a comparator hit or recreate the cleared
cause inside this bound. UpdateException at ProcessorInterface.cpp:181..188
projects cause&mask into the external exception bit. Cause and pending
Exceptions are different state.

The first VI update also advances Movie counters and invokes NewField.
Those branches precede entry and have frame-step/achievement dependencies.
Their absence inside the later interval follows half-line predicates;
they are not globally omitted.

### Audio and DSP

DSP Reinit at DSP.cpp:121..149 sets m_audio_dma={} at143; Enable is the
control's high bit. All writers are that initializer, savestate DoState
at81..93, DSP-control reset at271, and AUDIO_DMA_CONTROL_LEN MMIO writer
at327..347. Reset only clears Enable; the MMIO writer can enable it and
schedule an AID interrupt, so that writer must be excluded explicitly.

With the initializer and excluded restore/control writes, Enable remains0.
DSP.cpp:426..455 sends16 external zero samples, changing no guest RAM,
DMA address/count or DSP interrupt state. The event still reschedules at
the GC divisor 3372/period 121392. Unknown Enable remains UNKNOWN:
Exceptions=0 cannot establish this branch.

Before entry, six HLE DSP callbacks call the initial ROM ucode's empty
Update (DSPHLE.cpp:29,50..53; ROM.cpp:36..38), provided no DSP mailbox/control
write changed that ucode. ROM's initializer queues8071FEED without an
interrupt request. Retain this mail/control state until a guest consumer.
Six GPU callbacks only request external AllowSleep
(VideoCommon/Fifo.cpp:399..402). Neither callback is due again in the bound.

### DTK, SI and debugger

Two preentry FinishExecutingCommand callbacks carry DTK3/TCINT1.
DVDInterface.cpp:1306..1351 uses transfer_size0 and DTK reply handling,
preserving DI address/length and avoiding the guest DI-interrupt reply path.
The scheduled callback receives empty data. ProcessDTKSamples at135..155
decodes zero blocks, leaving the temporary buffer silent and decoder intact.
AI Init selects48k streaming with PSTAT0
(AudioInterface.cpp:193..204,323..325). With no AI-control writes,
m_stream&&ai.IsPlaying() is false regardless of unknown DVD stream state.
DTK then sets pending_blocks6 and schedules1699488 minus lateness, from
486000000*6*28*2248/108000000. This produces pending3398976. DVD stream
state itself cannot be admitted from the narrow MMIO watch.

Boot VI polls SI once at scheduled247104/half-line15. It throttles,
updates input gating and calls SI.UpdateDevices, changing guest-readable
channel words, error/status bits and RDSTINT from controller inputs
(SI.cpp:554..606). Initial RDSTINT/TCINT masks are0, so this poll does not
request a PI SI interrupt under the unchanged-mask contract
(SI.cpp:102..119). Next poll is 15+2*492=999. Observed input words cannot
be native constants. Their state stays UNKNOWN unless the already admitted
controller pipeline owns this exact poll. Decline at its first unmet consumer.

GDB's callback invokes ProcessCommands(false). No available packet returns
without guest mutation (GDBStub.cpp:953..967); it still reschedules100000.
The capture client continues and waits for the final stop without another
packet. Preserve that command discipline as provenance. Generic GDB
callback harmlessness would be false because command writers exist.


## Environment-bound references 42d

The three final 42d profiles bind capture_complete=true, the exact event/MMIO
observer environment, and capture tool SHA256
d477a14b913c1b9d3b07be5a0412233121f75aa1d368a7a57b3940df388f9f81.
All three pass the updated 37-pin auditor, including complete instruction
tail and strict observer_environment equality. Their timing metrics, pending
queue and instruction hashes are the final 42d values in the table above. The 109 MMIO rows contain
zero watched reads/writes with environment_bound=true. This now excludes
CPU-origin DSP/VI/PI-mask/AI control writes for this exact finite startup
window. Non-CPU/restore provenance and native ownership remain distinct
requirements; the full event audit declines savestate producers.

| Profile | Event SHA256 | MMIO SHA256 |
|---|---|---|
|zero|a026cd7fbddf1b24a444b35bffcbf170ecbc2130d7eea1558d7e6d81f5eedd8f|5561fd6b4039cdd84e2bf47c87cb7f516df7d6033259b787e85efed9b07d13f3|
|paused|b5a8a0c8cfde10f1100b7fd0d8b0bc37dd1ec3674d16eb9db7b10ccf52ae3bcb|d0337ccda46d6c24c2e3666c9d31faece4e14d5ad5c7b5964a412a4223a3979f|
|enabled|631b2f3ffa66f1dbfccba117a8f22e53fa8f54a4419533903e6475c2bec0a1f3|da9cb69cdcd9fe113d16817dc09734ff17058043640400ee1d0fde8d6a1e5dcf|

Reports are build/agent-source-bound-zero-audit-42d.json,
build/agent-source-bound-paused-audit-42d.json and
build/agent-source-bound-enabled-audit-42d.json. They also pin the current
capture JSONs. A changed watch flag/range, incomplete capture or mismatched
capture tool hash declines before using negative MMIO evidence.

## MMIO ancestry evidence and limits

All final 42d MMIO files have109 contiguous rows and no watched read or write.
MMU.cpp:413 observes actual writes before handlers;247..250 observes actual
reads after them without added reads. OraclePhysicalWrites.cpp:988..995,
1007..1015 emits uncapped watch rows in phase0 when WATCH_PHASE0=1.
The capture explicitly supplies DOLPHIN_OSINIT_WATCH_PHASE0=1 and
DOLPHIN_OSINIT_WATCH as these inclusive-start/exclusive-end ranges:

    0c005000:0c005100;0c002000:0c002100;0c003000:0c003008;0c006c00:0c006c20

They cover DSP, VI, PI cause/mask and AI; SI/DVD are outside this watch.
The logger header omits the watch environment. Captured environment/source
binding is required to promote negative accesses to exclusion of a device
writer. The helper reports this condition explicitly.

Existing GDB memory reads are RAM-only (GDBStub.cpp:808..837), so they cannot
query private DSP/VI state through memory packets. Arbitrary MMIO reads can
have side effects. Additional queries must target source-proved DirectRead
fields or observers in their owner, and cannot replace producer inputs with
fresh clock outputs.

## Minimum consequence and genuine remaining gate

The source-specific reference consequence is closed: fresh initial 20k,
independently reconstructed boot N, continuous source costs/block retirement,
queue ordering and bounded callback predicates produce the recorded cycle
states. The previously audited TB formula then applies epoch plus truncated
cycles/12. Host pause duration contributes no cycles.

This does not prove physical Gekko elapsed production or a connected native
clock. Interpreter table cost is a virtual scheduler choice. Its unit boot
stepping and delayed block retirement differ from physical execution.
The table does not model cache/bus stalls, overlap, hardware exceptions or
physical TB enable/freeze. A physical 40.5MHz TB rate cannot prove how far
a native C++ operation advances its phase; host timing cannot supply that
missing equivalence.

For the source-specific model, native semantic work must own its work count,
original retirement boundaries, progressive memory visibility at each
callback, and initial epoch/phase. Raw opcode/cost traces and the source table
validate that ownership; they cannot become runtime dispatch tables or
frontier counter inputs. The coordinator's produced-work C++ candidate now connects the finite
semantic work and retirement ledger in research mode: all 43 clock phase/cache/
global checkpoints,39 selected machine checkpoints and116 full-end fields
match the final 42d references. This closes the earlier work-production gap
for those declared source paths. It still constructs no owned event/device
lifecycle, so this result does not discharge the remaining gate below.

This narrow interval permits precise projection of its three due callbacks
under initialized-state, MMIO and debugger-input contracts. It does not
permit generically omitting the event queue. Retain VI/PI state, DSP disabled
DMA/ROM mail, DVD pending blocks/deadlines and SI poll outputs. SI input,
unproved control and foreign ingress remain UNKNOWN until their consumers.
Source/prefix consistency alone advances no connected checkpoint and provides
no full repository or gameplay regression evidence.

## Exact remaining native owner and decisive falsifier

The seven pending entry items no longer contain an unexplained numeric
deadline: their enqueue ancestry is determined by fresh source initialization,
N and the declared GC/HLE/PAL/unit-factor/GDB-active inputs. A native event
owner must actually construct that state and replay its callback state,
rather than accepting the observed heap or a no-events acknowledgment.

The earliest external input in that lifecycle is SI.UpdateDevices during
the boot VI callback at half-line15/tick247104. It reads current device
configuration and controller state and writes SI channel/error/status state.
The analytical work factory presently selects SI poll492<<16 and
no-additional-paths as premises. It does not initialize an owned SI object,
run the poll or carry its outputs. The port3 enable bit EN3 remains zero
under the fresh initialization contract; SI.UpdateDevices itself never
writes poll bits or interrupt masks. Thus the source path does not consume
controller channel data before the current clock boundary, but that data
must stay UNKNOWN until its first actual consumer.

Already proved for this bound: initialized VI has no postentry field/SI/
comparator branch; disabled audio DMA sends only an external zero buffer;
inactive AI streaming makes DTK avoid a read regardless of unknown stream
state; the captured GDB command discipline leaves only rescheduling.
These consequences permit a bounded state projection under an owned
initialization contract. They do not permit dropping arbitrary callbacks.

The smallest initial-source perturbation is a fresh boot with
Dolphin.Core.SIDevice0=0 (NONE), preserving poll bits/masks and all other
clock/BI2/L2 source inputs, without midchain writes. MainSettings.cpp:171..178
selects device0's default controller and other ports NONE; SI_Device.cpp:
198..200 maps NONE to SI_DeviceNull. SI_DeviceNull.cpp:18..21 returns
ErrorNoResponse without fetching host pad data. Added phase0 MMIO watches
are exactly0c006400:0c006500 for SI and0c006000:0c006100 for DVD, appended
to the existing four ranges.

This experiment must show changed actual SI channel/status outputs while
N, queue/FIFO ordering, tick phase and produced-work CPU/RAM endpoints remain
equal. A difference rejects the current independence claim at its first
producer. Equal timing proves only the stated bounded projection; native
ownership still requires constructing SI control/configuration state and
retaining unknown channel values or implementing the finite NONE device.
The real controller profile needs its already identified input pipeline
when those values become consumed.

## Historical initial NONE-device falsifier result

The separately authored agent_si_device_falsifier_42.py executed a fresh
profile with only initial Dolphin.Core.SIDevice0=0 added to the original
capture command. It reused the original passive42c executable and pinned
capture logic. The original capture_timing_prefix.py SHA remains
d477a14b913c1b9d3b07be5a0412233121f75aa1d368a7a57b3940df388f9f81.
No original source, input, executable or midchain state was edited.
The helper's own metadata binds its exact source hash, the base capture hash,
the adapted function hash and the two explicit watch/configuration changes.
It verifies43 source pins: the37 timing pins plus MainSettings, SI device
factory/null/enum/control definitions and Core startup.

Artifacts are build/agent-si-none-capture-42.json and
build/agent-si-none-capture-42.comparison.json, with sibling raw logs.
Capture SHA256:
c70e7a1639da077ccf8dc9630d65338362bd2d5f9b4a34ce3962f9c726c9cb08.
Event SHA256:
6698120502ae5a42e176052de30ec08532bff168bc6a85f78e78f6a4f1739a54.
MMIO SHA256:
2b7f381c363ff3cfbc713f5d30856472203499dec41f56421ceb0fe635a0b4b2.
Instruction SHA256 is exactly the final 42d zero stream:
030d18eb37836fc2b76cdf1cc1c92a8565b6a3be2ef5c9ae59806baf68f68a8b.

The actual initialization record changes port0 configured/current type6
to0. At the SI callback, all four desired/current types are0 with no recent
unplug, excluding a device-change queue producer in this experiment.

| Actual SI consequence | final 42d controller0 | fresh NONE0 |
|---|---|---|
| Port0 response | Success1 | ErrorNoResponse2 |
| Port0 data before error update | hi00808080,lo80800000 | hi0,lo0 |
| Postpoll status |20080808|08080808|
| Postpoll communication CSR |10000000|00000000|
| Poll register before/after |01EC0000|01EC0000|

The data observer runs before SI.cpp applies ErrorNoResponse's status/error
effects; those pre-error words must not be mislabelled as final channel words.
The postpoll CSR difference is RDSTINT, while its mask remains0. These are
guest-readable changes and cannot be erased globally.

The additional SI/DVD phase0 watch records exactly one read and no writes.
It is a four-byte read of physical0C006430 at actual PC812002E0,
ticks2479817, returning01EC0000. There is no channel-data read or DVD access
in this complete bounded window. This directly falsifies the possibility
that the changed host-derived channel0 sample is consumed before this stop
under the stated source control state.

N=2656493, entry tick2676493, first-clock tick2830739, elapsed154246,
all seven pending items/deadlines/FIFOs and the10VI/1GDB/1audio dispatch
inventory equal the final 42d baseline. The full trace is byte identical.
The independently executed produced-work C++ candidate accepts no observed
frontier count and matches39 selected machine checkpoints,43 complete clock
phase/cache/global checkpoints,116 endpoint fields,104 known endpoint
memory bytes and four ordered stores. The report retains17 inner-unit
checkpoint-granularity gaps; it does not borrow later states for them.

### Historical 42e recapture under explicit executable pins

The previous complete experiment is build/agent-si-none-capture-42e.json and
build/agent-si-none-capture-42e.comparison.json, with sibling raw logs and
a fresh build/agent-si-none-profile-42e. It uses the same passive42c
executable SHA84be4d74d3a566b92e46c416d5e9c4ff87f313612fb2f4e529852cb74b0cfe78.
The final42d zero capture and build/agent-source-bound-zero-audit-42d.json
remain its source-bound baseline; the latter SHA is
f1aa08fe4a3fdca581ed4dc46ebcdd15571f6ac16670d7fab01df83df0bf0df9.

The public helper now requires --program-sha256,
--prefix-program-sha256 and --baseline-audit explicitly. It has no fixed
candidate executable hash or inferred baseline-audit filename. Its own text
reads/writes and subprocess decoding use UTF-8 explicitly. The exact helper
source SHA bound by this new capture is
35036d6c8dff625d758308dac00c9b4edcf852f52cc03a0ccfbfc229bbc97efd.
The original helper bytes are preserved in ignored
build/agent-si-device-falsifier-42-original.py with SHA
d7d5a9ea78a95de275b20a0a7e845547358d42f0caf561b6d73738fadab71945;
the historical original NONE capture and its comparison are retained.

| Fresh42e artifact | SHA256 |
|---|---|
| capture JSON | db425a8416bcd9e7acc9ac64ce7d59e71e3cd93e62bf79b3c82ba7a7f9fc4df1 |
| comparison JSON | bac3a509a7f6ced55d04a3774204fd82ae90e145e96c5facf0d6729ade1243c4 |
| event stream | 4624ab0021bc3829387ac53288f0e26c99922c6322998b9df79972809881c0e8 |
| instruction trace | 030d18eb37836fc2b76cdf1cc1c92a8565b6a3be2ef5c9ae59806baf68f68a8b |
| MMIO stream | b966877af9d1401c65f231de0f25bb5e15d350690e395b9f746c8cb434b1650e |
| clock research executable | 5cf06a044d4a9a47bce5df0c045b1d5af0ff341e39b9e0213c926efd9c444c55 |
| native prefix executable | 2514d5c62a2e22f512af522a4154d8689b500906926a7a5ac0ea0150106e42c0 |

The capture binds all six inclusive-start/exclusive-end watch ranges and
phase0=true to its own source hash, original capture SHA and the unchanged
adapted-function SHA4f1007d11840301f3e8a521fdcbcf48e2ee3ece26281110590243cd80dade4dd.
The base capture source still hashes
d477a14b913c1b9d3b07be5a0412233121f75aa1d368a7a57b3940df388f9f81.

Fresh42e again changes port0 Success1 to ErrorNoResponse2 and the same
postpoll status/CSR fields, while boot N2656493, entry2676493,
first-clock2830739, elapsed154246, final2830779, pending heap/FIFO and
callback inventory match final42d. Its complete instruction trace is byte
identical to the baseline. The110-row MMIO file contains one SI poll read
and zero watched writes, with no channel-data read or DVD access.

The current C++ comparison matches39 selected machine checkpoints,
43 complete clock phase/cache/global checkpoints,116 endpoint fields,
104 known endpoint memory bytes,30 elapsed samples and four ordered stores.
It reports17 inner-unit granularity gaps explicitly. It derives boot N and
prefix work without receiving an observed frontier count. Terminal work
is now explicit as SOURCE_PENDING_WORK 00000004: the final cost4 remains
pending at the continuous stop. produced.falsify_output rejects348 output
mutations, including suppression of that pending work. These are checker
falsifiers plus source-conditioned parity, not event/device ownership.

A preliminary fresh run timed out before GDB readiness and produced no
complete capture. Its profile/raw logs are retained under
build/agent-si-none-failed-readiness-42e and provide no parity evidence.
An explicit socket probe demonstrated Windows excluded-port rejection
(WinError10013). The helper now tests an explicit bind before launching,
including its automatic port selection, rather than assuming a bind(0)
port can subsequently be bound by Dolphin. The successful complete capture
records the explicitly checked loopback port39042. Port selection affects
observer transport; it supplies no elapsed time or clock phase.

### Final fresh 42f capture after the port-selection regression

The authoritative latest NONE falsifier is now
build/agent-si-none-capture-42f.json with sibling comparison/raw logs and
fresh build/agent-si-none-profile-42f. The42e capture above remains valid
under its archived build/agent-si-device-falsifier-42e.py source SHA
35036d6c8dff625d758308dac00c9b4edcf852f52cc03a0ccfbfc229bbc97efd.
No old artifact or capture metadata was rewritten.

The final helper factors transport selection as
select_explicit_port(requested=None, socket_factory=socket.socket).
An independent fake-socket regression proves that automatic selection
skips an excluded port and a busy port before selecting a usable explicit
port, that a requested usable port is probed once, and that invalid values
decline before socket creation. A blocked requested port and a fully blocked
automatic range both decline. No factory receives port0.
The complete fresh capture exercised automatic selection and records
explicitly checked port30000. Its helper SHA is
c08774890339faa029d103f99b3f2beeb2124f971a26e97bb00632e3b5d58b46.

| Final42f artifact | SHA256 |
|---|---|
| capture JSON | 83a1094e37974f4dd82ca5e85ce652dcb7d392d46c11e915f20d439edd532417 |
| comparison JSON | 74305e6604947751d25bde4531d8e5553548f3315eb085b28bafe20017678c60 |
| event stream | 852c1180625645c224c39ce822387f87627faa710c7a56bfcbf6d8940a0ebf52 |
| instruction trace | 030d18eb37836fc2b76cdf1cc1c92a8565b6a3be2ef5c9ae59806baf68f68a8b |
| MMIO stream | 5f196c4d209feb28acc4d5569c7fc7dd0bbd5b63a24a4ffb98d05377a20ebbda |

Final42f binds the same final42d baseline/source audit,43 source pins,
passive executable, original base capture and two source adaptations as42e.
Actual NONE SI output changes,110 MMIO rows/one poll read/zero watched
writes, seven-item pending heap/FIFO and10VI/1GDB/1audio inventory all
repeat. Boot N2656493, prefix work154246, first2830739 and final2830779
repeat; the instruction trace is byte identical to final42d zero.
Current executable pins remain5cf06a044d4a9a47bce5df0c045b1d5af0ff341e39b9e0213c926efd9c444c55
and2514d5c62a2e22f512af522a4154d8689b500906926a7a5ac0ea0150106e42c0.
Parity again covers39 machine checkpoints,43 complete clock checkpoints,
116 endpoint fields,104 known memory bytes,30 elapsed samples and four
stores, while preserving17 granularity gaps. SOURCE_PENDING_WORK remains
00000004 and all348 intentional native-output mutations decline.
The source-conditioned equivalence and ownership limit below are unchanged.

### Minimum remaining dependency after this falsifier

There is no remaining unexplained numeric enqueue deadline or controller
sample influence on the proved entry-to80373AC4 clock projection.
The remaining native prerequisite is construction and lifetime ownership
of the initialized event/device control state: fresh scheduler factors and
heap/FIFO; VI register/half-line/last-line/poll state; PI cause/mask; DSP
disabled DMA and ROM state; DTK pending-block/deadline state; immutable SI
poll/masks/device configuration; and the declared GDB-active command schedule.
The source-specific C++ work candidate has not constructed that object or
replayed those callbacks. The analytical initialization factory still
selects premises rather than establishing them by execution.

A restricted native owner may retain controller channel words as UNKNOWN
while the proved EN3/mask path does not consume them. It must decline at
their first subsequent consumer or invoke the admitted controller pipeline.
A finite NONE-device implementation can instead produce its no-response
state from the pinned source rule. Neither choice permits accepting captured
heap/state words, globally suppressing callbacks or claiming physical time
equivalence. The next work is an owned finite initializer and callback-state
projection, followed by renewed checkpoint and repository/gameplay gates.

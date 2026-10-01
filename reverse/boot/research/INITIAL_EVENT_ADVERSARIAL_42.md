# Independent first-event audit, research 42

## Admission status

This audit reads the matching private source directly and independently predicts
the initialization and first-Advance journal subset. It imports no coordinator
validator or owner implementation. The subset passes all three complete 42d
captures. It does **not** establish full callback state parity or admit a
production event or clock provider. The earlier checkpoint 40 boundary remains
unchanged.

The new `InitialBootEvents` C++ implementation is a finite research owner with
explicit unknown reads and declared control branches. Its first due-event order,
arithmetic, VI edges, DSP ROM behavior, and partial stop states agree with this
source audit. Calling its external GPU effect function establishes an invocation
under the caller's contract; it cannot independently prove what that function
delivers to a real worker.

The smallest presently unbound effect-producing read is
`Mixer.cpp:253 m_log_dtk_audio`, during the first DTK callback. A known false can
be derived for a fresh process with default audio-dump configuration and no dump
toggle ingress; an absent journal field cannot establish that predicate. After
that predicate is bound, the GPU worker effect and first VI Movie/NewField inputs
need their own ownership or explicit unresolved stops.

## Independent evidence

`agent_initial_event_adversarial_42.py` requires explicit `--source-root` and
`--event-log` paths. It pins 30 source files by SHA256, parses the 16-column TSV
schema, requires a complete unfiltered `init..timing-prefix-end@80373ac4` stream,
and predicts the first lifecycle from source equations. Its equations accept no
observed deadline, cycle total, queue state, or callback output as an input.

| 42d profile | Event SHA256 | Rows | Fields | Mutation rejects |
| --- | --- | ---: | ---: | ---: |
| Zero hardware | `a026cd7fbddf1b24a444b35bffcbf170ecbc2130d7eea1558d7e6d81f5eedd8f` | 60 | 480 | 540 |
| Paused hardware | `b5a8a0c8cfde10f1100b7fd0d8b0bc37dd1ec3674d16eb9db7b10ccf52ae3bcb` | 60 | 480 | 540 |
| Enabled hardware | `631b2f3ffa66f1dbfccba117a8f22e53fa8f54a4419533903e6475c2bec0a1f3` | 60 | 480 | 540 |

The 480 fields include literal padding and metadata, and are not 480 independent
device-state observations. The deliberate rejects consist of every one-bit
field mutation and each omitted lifecycle row: 1,620 rejects across the three
captures. Two additional Movie counterexamples per capture demonstrate that
equal queue/timer observations do not bind latent polled/recording inputs.
Deleting the evidence header or terminal row is also rejected for each profile.

With `--native-executable`, the tool independently executes the C++ unbound and
declared bound controls and compares all exported snapshot groups, pending heap,
active callback and ordered journal to its own source projection. The tested
executable SHA256 is
`2a3fc25920c3ef5c5fe323f5cfbd87d4cbf1a9ec70ebf52be4cde7f6a3407d81`.
The unbound control has 14 predicted rows and 170 deliberate mutation rejects;
the declared bound control has 38 rows and 365 rejects. These 535 checks validate
the research reconstruction and partial states. They do not turn declared
branch conditions into observed live inputs or establish original side-state
parity. The executable's source-authored test also checks conditional PI
projection using the actual external exception bit `0x4` and preserving bit `0x10`.

Example invocation from the repository root:

```powershell
python reverse/boot/tools/agent_initial_event_adversarial_42.py `
  --source-root <matching-private-source> `
  --event-log build/passive-timing-zero-42d.events.tsv `
  --event-log build/passive-timing-paused-42d.events.tsv `
  --event-log build/passive-timing-enabled-42d.events.tsv --self-test
```

The zero/paused/enabled first-event values agree. Their thread IDs, sequence
numbers, startup input observations and later reachable paths differ. This
audit compares semantic row values, not host thread IDs or symbolic stacks.

## Fresh initialization and queue equations

The admitted source projection is a newly constructed GC system with HLE DSP,
CPU frequency 486,000,000 source cycles per second, unit scheduler/VI factors,
PAL clock selection, no restored state, no extra queued events, unchanged
configuration, and no foreign scheduler or device writer ingress before the
first Advance. These are source conditions to prove, not fallback defaults for
an arbitrary process.

`HW.cpp:34..53` orders CoreTiming initialization, timer PreInit, AI, VI, SI, PI,
other devices, DSP, DVD, CPU, then SystemTimers initialization. DVD queues DTK
before SystemTimers queues its five initial callbacks. CPU initialization's
null decrementer cancellation does not remove an event. SystemTimers binds
cached TB/DEC origin ticks through two zero tick reads; this audit does not
reconstruct the RTC epoch production again.

`CoreTiming.cpp:90..118` computes initial downcount before RefreshConfig.
The fresh cached factor is zero, producing `(G,L,D,S)=(0,20000,0,true)`.
RefreshConfig establishes factor/inverse bits `3f800000`. The later
ForceExceptionCheck(50) leaves downcount zero because the existing zero is
already sooner. The first Advance therefore commits `20000-0=20000`.

AI initialization sets GC streaming divisor `1124*2=2248` and DMA divisor
`2248*3/2=3372`. VI has HLW429, three equalization half-lines per EQU6, odd
PRB502/PSB5 and even PRB503/PSB4, producing 525 half-lines per field.

| Initial event | Deadline equation | Deadline | FIFO | Userdata |
| --- | --- | ---: | ---: | --- |
| DTK FinishExecutingCommand | 0 | 0 | 0 | `0000000300000001` |
| GPUSleeper | 0 | 0 | 1 | 0 |
| VICallback | `(2*486000000/27000000)*429` | 15444 | 2 | 0 |
| DSPCallback | 0 | 0 | 3 | 0 |
| AudioDMACallback | `486000000*3372/(108000000*4/32)` | 121392 | 4 | 0 |
| PatchEngine | `15444*525` | 8108100 | 5 | 0 |

At G20000, the comparator is ascending `(deadline,FIFO)`, so due order is
DTK0/F0, GPU0/F1, DSP0/F3, VI15444/F2. Sorting only by FIFO incorrectly places
VI before DSP. A callback schedules relative to sane G20000, and its lateness
is subtracted exactly once. AudioDMA and PatchEngine are not dispatched here.

| Dispatch | Lateness | Next relative | Next absolute | New FIFO |
| --- | ---: | ---: | ---: | ---: |
| DTK | 20000 | 1679488 | 1699488 | 6 |
| GPU | 20000 | 466000 | 486000 | 7 |
| DSP | 20000 | 466000 | 486000 | 8 |
| VI | 4556 | 10888 | 30888 | 9 |

Next heap is VI30888/F9, Audio121392/F4, GPU486000/F7, DSP486000/F8,
DTK1699488/F6, Patch8108100/F5. The first exit is
`(G,L,D,S,nextFIFO)=(20000,10888,10888,false,10)`. PC81200258 belongs to the
separately owned apploader and is observed, not guessed by the new event owner.

## Complete callback effects and retained state

### DTK and mixer

`DVDInterface.cpp:1306..1351` unpacks DTK/TCINT. Transfer size is zero for this
reply type, so DIMAR and DILENGTH retain their prior values. It takes the DTK
case rather than the DI interrupt/IOS cases. No TSTART clear, DI IRQ or IOS
callback occurs. The initial DTK event has empty audio data.

`ResetDrive(false)` sets stream false and pending blocks zero. The TCINT path
constructs a zero PCM buffer and calls ProcessDTKSamples with zero blocks:
no ADPCM decoder, filter, payload or sample mutation occurs. It still calls
`Mixer::PushStreamingSamples(...,0)`. Output-rate validity is read but either
outcome executes zero PushSample iterations. It then reads `m_log_dtk_audio`.

If logging is false, the call returns without mixer sample queue changes or
WAV effects. If logging is true, it reads input divisor and volume and invokes
WaveFileWriter even with count zero. A closed file emits an error; skip-silence
returns; an open non-skip writer with a changed divisor closes the current WAV,
increments its index and creates a new WAV. Zero sample count alone therefore
cannot prove that all host effects are absent.

Because stream is false, `m_stream && ai.IsPlaying()` short-circuits; this branch
does not need an IsPlaying sample. Pending blocks become six at fresh AI48KHz.
Audio position/start/length/decoder state retain their prior values, and no
DVDThread read is launched. Period is
`486000000*6*28*2248/108000000 = 1699488`, then lateness20000 is subtracted.
The queued TCINT repeats as future state. Audio dumping defaults false in
`MainSettings.cpp:312`, and Mixer logging defaults false in `Mixer.h:194`.
Those declarations require a lifecycle/no-toggle argument for a live process.

### GPU worker

`SystemTimers.cpp:111..120` calls Fifo.GpuMaySleep before rescheduling.
`Fifo.cpp:399..402` delegates to BlockingLoop.AllowSleep, whose exact effect is
`m_may_sleep.Set()` (`BlockingLoop.h:233`). This is an external worker atomic
flag write. A worker in STATE_DONE may TestAndClear it and transition to sleep,
so reading true afterward is not the only equivalent outcome. A typed effect
sink can preserve delivery and order under an owned worker contract; a no-op
sink with an acknowledgment does not establish a live worker effect. The
callback does not itself execute a GPU command or read guest memory.

### DSP HLE ROM

DSP Reinit chooses HLE, clears audio/ARAM DMA state and sets halt. HLE Initialize
selects ROM, resets CPU/DSP fake mailboxes to zero and sets Halt/Init. ROM
Initialize enqueues `(8071FEED,false)` with no interrupt request. MailHandler
last_mail defaults zero; halt true prevents this mail from being consumed by
DSP mailbox reads. These are distinct state: retained pending queue, last
mail, fake mailbox words, and control registers.

`DSP_UpdateRate` is pure CPUHz/1000. DSPCallback supplies 466000 after lateness.
The HLE UpdateDSPSlice branch calls DSP_Update and does not modify DSP slice;
DSP_Update invokes ROM.Update, whose body is empty. No mail is added, removed,
made readable, or interrupt-marked. No DSP DMA/ARAM payload is consumed. ROM
upload task parameters and counters remain zero from construction. A scalar
`rom_mail` in a bounded snapshot cannot substitute for the full future mailbox
semantics once a downstream read becomes reachable.

### VI first half-line, Movie and NewField

VICallback supplies **scheduled** tick15444, not current timer20000. Preset
half-line0 is a field boundary. VI calls Movie.FrameUpdate before rendering,
then Core.Callback_NewField before polling.

Movie.FrameUpdate increments current_frame; if polled is false it increments
current_lag_count. Recording mode additionally copies those counts into total
frames/lag, then polled becomes false. Fresh inactive fields produce frame1,
lag1,total_frames0,total_lag0,polledfalse. PlayInput/BeginRecordingInput may be
called before Movie.Init; Movie.Init intentionally preserves an active movie.
Thus a fresh-system claim must also close preboot movie setup and later movie
ingress. The capture tool has no `-m` argument; MainWindow.cpp:289..298 contains
the explicit command-line movie setup branch.

Active-line edges are odd520/519 and even1045/1044, derived by
UpdateParameters from ACV0 and the preset blanking counts. Half-line0 matches
none, so no BeginField, EndField, XFB request or frame-render count occurs.
It also does not equal next SI poll15, so no throttle, input-gate update, SI
device update or host input poll occurs at this callback.

NewField reads the host-writable `s_frame_step`. If true it waits for the GPU
request queue, reads atomic stop_frame_step, and can clear frame_step, break
the CPU and notify state listeners. EmuThread resets frame_step false before
Movie/HW Init (`Core.cpp:532`), but host frame-step ingress can change it.
NewField always calls AchievementManager.DoFrame.

The actual private build has USE_RETRO_ACHIEVEMENTS ON and supports
RAINTEGRATION; the constexpr stub is not the executable path. DoFrame first
uses IsGameLoaded/rc_client_get_game_info and DLL-found. The rcheevos getter
returns null for a null client. Null client plus DLLfalse returns before
CPU-thread test, rc_client_do_frame, guest memory peeker, achievement state,
rich-presence/network callbacks, Discord and host steady_clock reads. If an
active game or integration DLL is present, these effects need ownership.
RA_ENABLED defaults false and client/DLL start inactive; absence of activation
and host ingress must be established, not inferred from pending exceptions.

After these callbacks VI rebases next SI poll to15, increments half-line to1,
and leaves last_line_start zero because the count is odd. Fresh CoreTiming's
VI-skip member is false and no prior Throttle has run. ResetThrottle itself
does **not** reset this member; it stores reference cycle and host time only.
The false skip member short-circuits graphics/determinism reads here.

Interrupt comparison uses postupdate half-line1. Reg0 is HCT430/VCT263,
reg1 HCT1/VCT1, reg2/3 zero; no comparator matches. UpdateInterrupts clears
PI VI cause. Fresh PI cause10100 becomes10000, mask0 remains zero, and
UpdateException clears only EXCEPTION_EXTERNAL_INT (`0x4`) while preserving
other pending bits. Fresh zero pending bits remain zero, so the final
CheckExternalExceptions has no handler effect. This derivation does not treat
MSR.EE or a literal log padding field as pending exception state.

## Journal observability limits

CoreTiming callback-return exports event time/FIFO/userdata and literal zero.
Its remaining columns are padding. Advance-enter/exit v7 is also literal zero;
neither records Exceptions. Explicit later `boundary` records export
PC/G/L/D/ticks/MSR/Exceptions/PIcause. Both boundaries at80003154 and80003170
observe cause10000 and Exceptions0. They support the retained later state but
do not directly capture the first callback's before/after PI transition.

The first Advance has no passive Movie counter, frame-step, achievement,
GPU atomic, DSP/mailbox, mixer log or DTK-pending state export. Current parity
covers six initial enqueues, four dispatches, four callback enqueues/returns,
three VI state rows and associated clock/configuration equations. Full callback
parity needs passive source-derived before/after state exports or independently
proved unreachable branches and retained-state provenance. Getter outputs
used for validation must never become live owner inputs.

## Required falsification before connection

1. Change the heap comparator to FIFO-first, remove lateness subtraction,
   subtract it twice, schedule against oldG0, or use tick20000 in VI.Update.
   The unchanged 42d rows must disagree at the first affected dispatch/store.
2. Change CPU clock, scheduler/VI factor, restored lifecycle, extra event or
   foreign queue ingress. The bounded capsule must decline before mutation.
3. Leave each live branch unbound in turn. The owner must stop at the exact
   read, retain prior owned changes, not emit callback-return, and not admit a
   captured downstream field as a substitute. A stopped capsule cannot resume
   with injected outputs.
4. Enable DTK audio logging with zero pending blocks and use an open/non-skip
   WAV writer whose divisor differs. Observe the file/header effect or decline
   before it. Empty PCM is not an admissible no-op proof.
5. Exercise a real GPU effect sink once, and test worker consumption racing
   the readback. Preserve atomic delivery rather than demanding final flagtrue.
6. Select Movie recording/playback at preboot, set polled via its genuine
   consumer, cross frame/lag uint64 wrap, or inject frame-step ingress. Observe
   Movie totals, queue wait, CPU break and state listener effects or decline.
7. Activate a real achievement game or integration client and ensure that
   memory peeking/host-clock/external effects cannot pass the inactive guard.
8. Change first VI comparator HCT/VCT/mask, set VI-skip through a real prior
   throttle path, or set PI mask/pending exception bits. These require a broader
   source profile or rejection; zero MSR.EE alone cannot justify omission.
9. Observe DSP pending tuple/last mail/control/mailboxes before and after the
   first Update. Change to LLE or another ucode and decline this ROM capsule.
10. Remove any first lifecycle row, truncate the sink, filter out kinds, or
    corrupt padding/state/control fields. The independent audit must reject.

## Mandatory stage ledger

| Stage | Status |
| --- | --- |
| Reference capture | Three complete 42d streams; first callback side-state observations incomplete |
| Semantic reconstruction | Independent finite queue/VI equations; source callback effects and unknown guards audited |
| Perturbation/falsification | 1,620 trace mutations rejected; latent Movie counterexamples; live branches require controlled source captures |
| Checkpoint parity | First-event observable subset passes; whole callback state parity not established |
| Repository/gameplay regression | Coordinator-owned; this audit makes no regression or promotion claim |

No unexplained side state is replaced by zero. A source-bound fresh inactive
control can extend a research projection, while an arbitrary live event owner
still fails closed at the first missing condition.

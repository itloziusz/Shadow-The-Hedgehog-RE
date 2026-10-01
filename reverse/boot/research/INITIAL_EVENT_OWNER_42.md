# Finite initial event owner after checkpoint 42

This records the research42b unbound/declared-control implementation and its
archived source/executable identities. Research42c adds the separate owned
DTK path described in `DTK_OWNER_RECEIPT_42.md`; the older mode outputs below
remain byte-identical. Read the latest `PROGRESS.md` first for current scope.

## Scope and result

InitialBootEvents.hpp/.cpp construct a typed native scheduler queue and the
bounded DVD/AI/DSP/VI/PI control state. The queue is produced by source
initialization order; captured queue items, deadlines or tick counters are
never passed to C++. The first Advance commits20000 source scheduler units.

The unbound reference capsule stops at the first unresolved consumer:
Mixer.cpp:253 reads live m_log_dtk_audio inside the first DTK callback.
It retains the dispatched item, zero transfer and zero-sample mixer request,
but does not set pending_blocks6, enqueue the DTK successor or run GPU/DSP/VI.
The complete first-Advance result is a separately declared control with
logging disabled, a delivered GPU effect sink, inactive fresh Movie state,
frame-step=false and absent achievement client/DLL. Those controls are not
inferred from a matching queue trace.

This is a finite research owner, not a connected boot frontier. No host clock,
PPC interpreter, observed work count, queue injection or midchain value is
used. Root owns CMake integration and repository/gameplay gates.

## Compilable API and machine output

The C++17 API has no BootFoundation dependency:

    auto config = shadow::boot::FreshInitialBootSourceConfig42();
    shadow::boot::InitialBootEventOwner owner(config); // live branches UNKNOWN
    auto stop = owner.FirstAdvance({true, true, true});
    const auto& state = owner.State();
    const auto& ordered_effects = owner.Journal();

The three Advance authorizations mean unchanged configuration, an empty
foreign scheduler queue, and no intervening device/guest writers. UNKNOWN or
false declines before any Advance mutation. They must be independently bound
by the calling runtime; choosing them is not an ownership receipt.

InitialBootBranchBindings retains UNKNOWN Movie lifecycle, DTK log flag,
host frame-step and achievement client/DLL state. A GPU AllowSleep effect
has a typed std::function<void()> sink. Delivery is separate from the later
worker flag value: the worker may consume its atomic flag. A missing sink
stops exactly at BlockingLoop.h:233. A throwing sink preserves that stop,
the active GPU callback, prior DTK effects and undelivered receipt, propagates
the exception, and refuses a second Advance. It does not invent a GPU
successor after a failed effect.

test_initial_boot_events supports:

- --dump: compact unbound snapshot/journal, stopping at Mixer.cpp:253.
- --dump-bound-control: explicit conditional control that completes the four
  due callbacks and sends AllowSleep to an owned test atomic.
- No arguments: semantic and rejection tests.

JSON schema initial-boot-events-42-v1 contains scope/control/stop, scheduler,
signed pending queue, active_callback, device/control snapshots, retained
UNKNOWN values as null, and ordered eight-word effect records. PC is not
invented: apploader PC belongs to the separate not-yet-connected guest owner.

## Source identity and production

All paths below are relative to <matching-oracle-source>. The audit helper
requires explicit --source-root and --library and verifies the37 existing
timing pins plus these14, totalling51. The core library remains SHA
eb0e5041b253072d735ddaac5fba9079ed2ca4916f26312845c706bd132b2939.

| Additional source | SHA256 |
|---|---|
| Source/Core/Core/Core.cpp | 8bf9e90fc56b5b4d7a1e42410ea2e2291f44b5d86fb4e982b3aa68912edf5944 |
| Source/Core/AudioCommon/Mixer.cpp | 24f91152ffe824019053f89acc77a974992fa9ed19e68b0c94194521987f90a3 |
| Source/Core/AudioCommon/WaveFile.cpp | bd9020f3769c3a4df30932cae3fa7efeb5dddd6f5251ce59221d59edba9d4961 |
| Source/Core/Common/BlockingLoop.h | 410f0fc12388580664481862d8cbc975539b7bfb56ecdef935d07c255adb3043 |
| Source/Core/Core/Movie.cpp | c6dc7711039e9c3c271412311bb5b342f31c39dd95300e35f970d15836b0ba39 |
| Source/Core/Core/Movie.h | 6f799ca407acc14f8d0ceab919aad63220ae7f9ce4c8e6a52f9afc52a9ce0fb7 |
| Source/Core/Core/AchievementManager.cpp | 687394d3f9601102b200f22048f52ac2bd6dd2eae976ac07eb3f83c6a435f9d2 |
| Source/Core/Core/AchievementManager.h | 689252000d9b1d9b11f1207e9f3bd3f5ba3ba21b13538fe054d52dcee44b37e5 |
| Source/Core/Core/Config/AchievementSettings.cpp | eaa04213954068ee4e4168259d4a005b0bd9d59d9c60f5b80e15ab6ca9533f79 |
| Externals/rcheevos/rcheevos/src/rc_client.c | ec6c68ffa4769b786d3ab3f45065d8872ad8b70afc8e15ac1436ee3910935029 |
| Source/Core/Core/PowerPC/Gekko.h | fad9f1cbb274f63955b44680627bfab542e1940b24b6f6625ee1653436a6020c |
| Source/Core/Core/HW/DSPHLE/MailHandler.cpp | 950b42a3ad61480cc54bf26163d7802f471305c8ce52f30a4099585696af789f |
| Source/Core/Core/HW/DSPHLE/MailHandler.h | 35a94fd4d1fc7dc7302f8208adeade9a79db01d78fa8b0c0e0fe53e0dee41cc5 |
| Source/Core/Core/HW/DSPHLE/DSPHLE.h | 4ad76991b0f777779e72bacf2d8050032b21bf2354a6a904503413992987e6e6 |

HW.cpp:34..53 initializes CoreTiming, clock PreInit, AI, VI, SI, PI,
DSP, DVD, CPU and SystemTimers in that order. CoreTiming.cpp:90..118 uses
fresh factor0 for its first downcount conversion before RefreshConfig:
G0,L20000,D0,sane=true; init inverse0 becomes unit3F800000 at init-ready.
Unit factors make the bounded float conversions exact. Unknown profile,
frequency/factor changes, restores and extra initial events decline.

AI Init at AudioInterface.cpp:193..204 selects GC AIS48KHz and AID32KHz,
playing=false, divisors1124*2=2248 and2248*3/2=3372. VI Init/PAL boot
preset constructs HLW429, clock0, EQU6/ACV0, PRB502/503 and PSB5/4.
Half-line period=(2*486000000/27000000)*429=15444; field length525.
PI Init at ProcessorInterface.cpp:51..64 sets cause10100, mask0.
DSP Reinit/HLE initialization clears DMA, installs ROM with queued8071FEED
and halted mail; ROM.cpp:36..38 has an empty Update. DVD ResetDrive at
DVDInterface.cpp:306..345 clears streaming/positions/pending blocks.

DVDInterface.cpp:287..290 schedules DTK/TCINT first. SystemTimers.cpp:
289..297 then schedules GPU, VI, DSP, Audio and Patch in that order:

| Native initial enqueue | Signed deadline | FIFO | Userdata |
|---|---:|---:|---|
| FinishExecutingCommand |0|0|0000000300000001|
| GPUSleeper |0|1|0|
| VICallback |15444|2|0|
| DSPCallback |0|3|0|
| AudioDMACallback |121392|4|0|
| PatchEngine |8108100|5|0|

No registered-only decrementer or GC-inapplicable Wii callback is enqueued.

## First Advance and conditional callback effects

CoreTiming.cpp:376..424 checks configuration/import before committing G.
With the declared empty ingress, it commits20000, sets sane=true and
dispatches in signed deadline/FIFO order. FIFO cannot move VI ahead of
DSP's earlier deadline0.

| Dispatch order | Due | FIFO | Lateness | Conditional successor/FIFO |
|---|---:|---:|---:|---|
| DTK/TCINT |0|0|20000|1699488/6|
| GPU |0|1|20000|486000/7|
| DSP |0|3|20000|486000/8|
| VI |15444|2|4556|30888/9|

DTK's ReplyType3 gives transfer_size0 at DVDInterface.cpp:1312..1322;
DIMAR/DILENGTH are preserved and no DI interrupt arm is entered. Empty
audio_data decodes zero blocks. Pending0 calls PushStreamingSamples(0).
Mixer.cpp:241's valid-output-rate condition has identical no-sample-loop
consequences for either result; it writes no mixer granule. The subsequent
m_log_dtk_audio read at253 is not removable. A true flag can enter
WaveFileWriter even at count0, including file stop/start on divisor changes.
A false bound control has no WAV write, sets pending_blocks6, and schedules
486000000*6*28*2248/108000000-lateness relative units. Its absolute next
deadline is1699488. The unbound baseline stops before these later effects.

GPU calls Fifo.cpp:399..402 -> BlockingLoop.h:233 atomic AllowSleep Set.
The typed sink delivers the request; no persistent worker flag is assumed.
DSP gets update budget486000-20000=466000; HLE ROM ignores that budget and
preserves DMA/mail/slice fields, then reschedules with lateness correction.

VI Update uses the original due tick15444, not currentG20000. The first
half-line0 is a field boundary and invokes Movie::FrameUpdate before any
render/NewField/poll mutation. Unbound Movie values remain null and stop at
Movie.cpp:175 before increment. The explicit fresh-inactive control owns
initial0 counters/polled=false and advances frame1/lag1; recording totals
remain0. Core.cpp:880's host-writable frame-step flag is a separate UNKNOWN
read. A true branch stops before waiting for the GPU/possibly breaking CPU.

NewField then invokes AchievementManager::DoFrame. The actual oracle was
built with USE_RETRO_ACHIEVEMENTS ON; using its header no-op is incorrect.
AchievementManager.h:259/284 initializes client=null and dll_found=false,
and RA_ENABLED defaults false. Those defaults alone do not bind later live
ingress. An absent bound client makes IsGameLoaded false because
rc_client.c:3493..3496 returns null; an absent bound DLL causes DoFrame at
AchievementManager.cpp:343 to return before rc_client_do_frame, CPU-thread
tests or host steady_clock. Unknown or present client/DLL stops there.

With those separately declared controls, no active edge or SI poll occurs.
VI rebases next_poll15, increments half-line0->1, leaves last_line_start0,
and no comparator asserts. UpdateInterrupts clears initial PI VI cause:
10100->10000; mask0 clears only external exception bit4. Final queue head
30888 gives L/D10888, G20000,sane=false,FIFO10. CheckExternalExceptions
returns with zero exceptions. Source fields are retained; no guest RAM
write is emitted by these finite callback branches.

## Substantive source corrections and falsifiers

The earlier source audit's even active edges1046/1045 were off by one.
UpdateParameters at VideoInterface.cpp:743..763 includes odd_even_psb_diff
5-4=1: odd_first520/last519, even_first=520+5+18+503-1=1045/last1044.
The earlier note is corrected and the C++ test asserts all four edges.
This correction changes no path in the previously bounded173..182 interval.

The initial new owner draft used external exception mask10; the zero-mask/
zero-exception baseline could not reveal that latent error. Gekko.h:926
defines EXCEPTION_EXTERNAL_INT=4. ProjectInitialPiException is now a pure
source-semantic helper used by VI dispatch. Its tests set and clear bit4
while preserving bit10 and every other pending bit. No new production path
or nonzero live exception is admitted by this helper.

The standalone clang++ C++17-O2/-Wall/-Wextra build passes22 config/ingress/
branch declines, signed negative deadlines, equal-time FIFO, pending heap,
ordered effect handoff, Movie/PI consequences and throwing GPU sink behavior.
Unknown bindings stop before subsequent effects; a stopped capsule refuses
resumption with replacement values.

## Original-reference comparison and actual evidence

agent_initial_events_42.py accepts a required native executable SHA, source
root/library, complete original capture JSONs and a repository-build output.
It invokes only --dump/--dump-bound-control with no captured runtime inputs.
It pins the51 sources/library and exact passive executable/copied Interpreter,
disc, original tool and observer environment. Before accepting subset parity,
it invokes the unchanged complete37-pin timing auditor with capture, events,
instructions and MMIO. All three references pass the complete lifecycle,
instruction, source-location, four-range phase0/environment and terminal
closure gates. It then reads original init through first advance-exit and
reconstructs the pending heap independently from enqueue/dispatch records.
The source auditor's traces are validation data, never C++ runtime input.

validate_capture_contract and capture_binding_self_test expose the new pure
provenance gate. Fourteen malformed bindings decline per reference: incomplete
capture, altered tool/oracle/original/copied identities, wrong environment,
unsupported HID0/L2/disc, midchain writes and unprovided epoch forcing.

The three final42d zero/paused/L2-enabled source captures each match:

| Comparison scope | Records | Exported observed fields | PC fields excluded |
|---|---:|---:|---:|
| Unbound prefix through DTK dispatch |11|84|4|
| Declared-bound complete first Advance |26|200|8|

Each profile rejects52 deliberately corrupted outputs: changed initial DTK
deadline0->1, queue FIFO/userdata/relative request, successor timing, swapped
equal-time dispatches, VT effect rows and exit timing. These checker
falsifiers do not create original-reference perturbation outcomes. The
existing actual pause and L2 controls are separate unchanged first-Advance
references; they do not bind Movie/RA/audio logging ingress.

The direct observations cover initialization/clock/queue/VI exported rows
and the reconstructed signed/FIFO heap. They do not expose DTK pending
blocks, mixer log state, DSP private fields, first-callback PI state,
Movie counters, frame-step or RA client/DLL fields. Callback-return padding
zero is not pending-exception evidence. The original later entry boundary
has PIcause10000, but cannot prove first-callback private state on its own.
These conditional/source-derived fields are listed separately in the JSON.

Schema initial-event-owner-audit-42-v1 provides source/candidate/tool pins,
reference capture hashes, unbound_prefix_parity,
declared_bound_control_parity, output_mutation_declines, native_outputs,
source_derived_conditional_fields, complete_source_audit,
malformed_binding_declines and unresolved dependencies, with
native_ownership_admitted=false.

Actual standalone artifact pins:

| Artifact | SHA256 |
|---|---|
| InitialBootEvents.hpp | b0c281f0171c028bf3288c86e0a1512ecd8fb42e5b2f2596219d547d34bffd6f |
| InitialBootEvents.cpp | e614b699245e7d1574028aa7a89c1dff71e4fa088e566ad5b65e5a24fafe01db |
| test_initial_boot_events.cpp | f4fe8e7920cb5f6a3dbdc8640ba03d6e74351f45273a994577bfb98304895c3c |
| agent_initial_events_42.py | dcc57a2550fe05901593b8306fa9a70fe25c04a7aa8af43a816ac714a04bac6d |
| build/agent_initial_boot_events_42.exe | 825d3a9e185210e3da85b23a06afc246639e9125bb13f82555a9d220518a7572 |
| build/agent-initial-events-complete-audit-42.json | f606c6eff99865e48d3e4d34fc4cc0942d66c5367d7758aac93725fc9d5eec02 |

Coordinator recheck compiled the same capsule with MSVC and ran all three
references through the complete admission gate. The executable SHA is
2a3fc25920c3ef5c5fe323f5cfbd87d4cbf1a9ec70ebf52be4cde7f6a3407d81.
The tool additionally rejects non-object capture/manifest/private bindings
cleanly; its new SHA is
ecdb4c01cde684f3969fa209a51b14bb6b93eb7da227829f270f2369c67cb03c.
`build/initial-events-final-42.json` preserves the new complete report.
The table above retains the author's original artifact identities.

The earlier build/agent-initial-events-audit-42.json is retained as a
historical subset experiment; the complete-gate report above is authoritative.
Its earlier26/200 and11/84 metrics remain honest, but that older report alone
did not provide the complete capture admission gate.

Original capture_timing_prefix.py remains SHA
d477a14b913c1b9d3b07be5a0412233121f75aa1d368a7a57b3940df388f9f81.
The earlier42d/42e/42f artifacts and helper identities are unchanged.

## Exact remaining dependency

The earliest current unbound consumer is now identified before the old Movie
gate: live DTK audio logging and its external WAV writer lifecycle. Fresh
false defaults and a queue match cannot replace an ingress-bound live flag.
The smallest next observation is a passive owner log at Mixer.cpp:253 plus
the startup/toggle producer chain, or an admitted external effect interface
that carries its exact writer branch. It must preserve count0 behavior.

After that binding, GPU AllowSleep requires delivery to the owned loop;
Movie startup/live counters/mode and frame-step ingress require their own
producer-bound observations. RA-enabled builds require client/DLL lifetime
binding, not a compile-time no-op. NewField cannot be generically skipped.
The capsule records exact partial progress and refuses each unprovided
consumer before applying later effects.

A connected native initializer still needs the original progressive guest
RAM/register owner and the later event/input slices. SI channel values stay
UNKNOWN until an admitted consumer; RTC epoch and physical Gekko elapsed
equivalence remain separate gates. Passing the declared control alone does
not promote any of these dependencies or stop the broader reconstruction.

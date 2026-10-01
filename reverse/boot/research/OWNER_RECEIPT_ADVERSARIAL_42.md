# Independent owner receipt audit 42

Scope: copied-source passive receipts. These receipts do not admit a native event owner, elapsed provider, production clock, or a connected frontier beyond checkpoint 40. The previous raw, work, clock, and event gates remain required.

## Apploader first three instructions: accepted bounded receipt

The independently reread capture is `build/agent-entry-receipt-original-42b.json`, SHA256 `a639cd3c33267d629753298fc1745dba03117bcd69ebd26e34ab0b1671a327c3`. Its executable is SHA256 `ef208d1a55fc635aa8ac1d47aff2b44e6b669df6d05c9ddeb67632d0cbdc2933`. This is the original apploader entry at `81200258`, not DOL entry `80003154`.

Independent tool `tools/agent_owner_receipt_adversarial_42.py` imports no authoring validator. Its completed combined audit verifies 22 original source identities, the unchanged asset, every saved build binding, the exact observer inverse, and the exact link transformation. The link retains repeated named system libraries in their original order. It substitutes one Interpreter object, retains the separately bound debugger object, and rereads all 265 existing file inputs. Named SDK libraries are recipe-bound names; their resolved SDK binaries are not independently inventoried by this audit.

The observer reads existing registers/timing globals and mapped RAM. Its enter hook uses the instruction already fetched by the original interpreter; it adds no opcode fetch, `GetTicks`, condition writer, or guest-memory access. This is source-observer noninterference under the observed input order. Added file/mutex work does not establish equivalence for arbitrary racing host activity.

The raw apploader image produces:

| PC | Word | State effect | Physical effect |
| --- | --- | --- | --- |
| `81200258` | `7c0802a6` | `mflr r0`, incoming LR `0` gives r0 `0` | none |
| `8120025c` | `9421fff8` | `stwu r1,-8(r1)`: `815edca8` becomes `815edca0` after the successful store | `015edca0` receives `815edca8` at G=`20001` |
| `81200260` | `9001000c` | `stw r0,12(r1)`, r0 remains `0` | `015edcac` receives `00000000` at G=`20002` |
| `81200264` | `48000139` | enter observation only | branch not consumed by this receipt |

The eight rows match all 224 fields. Before the first Advance, PC=`81200258`, NPC=`0`, G=`0`, slice=`20000`, downcount=`0`. After the first Advance, G=`20000`, slice/downcount=`10888`; each subsequent SingleStep commits one unit before entering the next instruction. The exit observation is before the outer SingleStep resets slice/downcount. MSR=`2032`, Exceptions=`0`, CR=`0`, effective cache=`0`, DBAT0=`80001fff/00000002`, and the 32-byte physical stack watch match each row. HID0.DCE alone does not establish effective accurate-cache behavior.

The 11 bounded early physical records include four allocated-RAM clear overlaps, syscall handler installation, the apploader copy lease/write, two exact opcode-cacheline read leases, and both stack stores. The zero-valued second stack store is demonstrated by a store receipt; unchanged bytes alone would be insufficient. Unknown early overlapping writers and unexplained pointer owners are rejected. This does not prove complete exclusion of every foreign writer over the entire boot interval.

Falsification: 224 individual row-field changes, 11 physical data changes, and omission of either store all fail: **237 rejects**. Later DOL entry retains the original work value `2676493`. Detailed output: `build/owner-receipt-adversarial-entry-42.json`.

Reproduce with explicit input paths:

```text
python reverse/boot/tools/agent_owner_receipt_adversarial_42.py --source-root <original source> --apploader <original sys/apploader.img> --entry-capture build/agent-entry-receipt-original-42b.json --self-test
```

## Nested apploader call/frame: accepted bounded receipt

Capture `build/agent-entry-nested-original-42.json` has SHA256 `8caee16bc973b29532fe392111b921f89e3ee806389f26aa25ee3eab52d5af0f`; executable SHA256 `016b4a460f2b5cb7dd92bb4529e805641732af9cb16b7b73709b9a60c500366b`. The independent inverse preserves the original observer and adds only the finite PC selection, six register receipts, and a wider mapped-RAM watch. It adds no clock getter or extra instruction fetch. All **18 rows / 612 fields** and **8 physical spills** match raw semantics; **628 field/store/omission mutations** fail.

The branch word `48000139` at `81200264` sets LR=`81200268` and targets `8120039c`. Its following mflr copies the new LR to r0. The callee stores LR at `815edca4`, establishes a 40-byte frame at `815edc78`, stores live r29..31 at `815edc94/98/9c`, and stores live r3/r4/r5 at `815edc80/84/88`. This receipt observes r29..31=`0`, r3/r4/r5=`80003100/04/08`; these are output observations, not acceptable constructor injections. A native semantic frame owner must use its live registers and source-derived initial/reset producers. Every stmw spill is a distinct ordered store, including zero-valued stores.

Before the call's SingleStep Advance, G=`20002`, slice=`1`, downcount=`0`, PC/NPC=`81200264`. Enter-call G=`20003`, slice/downcount=`10885`. Original SingleStep commits one unit per invocation even for stmw. The last receipt is enter `812003b8` at G=`20011`, slice/downcount=`10877`, before word `3c608120` (lis) is executed. Known mapping/cache/backing-RAM, no overlapping unknown writers or leases, and exception prerequisites remain necessary for connected ownership.

## DTK receipt: accepted copied-source flag ancestry

The original private `m_log_dtk_audio` begins false in `Mixer.h` and has exactly two source assignments, in StartLogDTKAudio and StopLogDTKAudio. PushStreamingSamples reads it even when `num_samples=0`. If true, zero samples do not eliminate WaveFile branch effects. DoState does not bind this flag.

The reviewed copied Mixer caches each existing branch condition once and records that same value. Constructor observation precedes config callback registration. Both flag writers and every success/failure/already-started/already-stopped branch have receipts. The copied Layer extension observes changed Set/delete values, layer/load snapshots, and mutable DSP section leases. It adds a snapshot read of DumpAudio; this is passive configuration observation, not evidence of a producer in the native owner.

The draft defects found and sent to its author were: a nonmatching `divisor` source needle, an incorrect DTK dispatch-name selector, raw cross-process comparison of live RTC values and Qt/host pointer fields, incomplete build-artifact rereading, an overwritten Mixer compile recipe, and missing exact push/read callback/thread nesting. Final 42c addresses these findings with separately archived recipes/logs and the exact helper snapshot.

The independently audited final executable is SHA256 `9da7238efe5ac44ae1f01088c4b95995d0232edb3ceebdd453d6141a9ec3e6b2`, helper `220c45b5526c03ee27612105e006ba967df867960b3b4494bd8883f92c409c6d`. Both source inverses, all 12 archived build artifacts, both exact compile recipes, compiler/linker, link map method ownership, exact two-TU link transform, and inherited file identities are checked. The copied Layer methods and OracleViAudit resolve to Layer.obj; the flag consumer and Start/Stop methods resolve to Mixer.obj.

| Receipt | Capture SHA256 | Event rows | Initial flag / count | Effective flag writes | Independent semantic declines |
| --- | --- | ---: | --- | ---: | ---: |
| `agent-dtk-owner-capture-42c.json` | `0e82d10f8f4cec10baac85b66ff3b6a9c92277cd4d7d2994a6b2c131b1f7ed32` | 505520 | false / 0 | 0 | 154 |
| `agent-dtk-owner-capture-42c-dump-control.json` | `7b74395dd485a9941356fa61fcb658c065991f6d5d3b336f80fb9aa16dbb20e3` | 505657 | true / 0 | 1 | 188 |

Each capture has exactly two FinishExecutingCommand DTK dispatches, userdata `0000000300000001`. Each nests exactly one same-thread push/read pair before its matching callback return. The sample counts are 0 then 168. Baseline has no Start/Stop calls or flag writes; its constructor produces false at rate48000. Positive startup configuration produces an actual changed Set, Start entry, successful true assignment, completed exit, then a true zero-sample consumer. These are source-control effects, not forced flag values.

Each **1291 queue/device/boundary records** match the unchanged baseline. Passive host records flush lossless boot/retirement compression at different points; raw compressed groups are not compared as if they had identical boundaries. Independent normalization checks **2656493 apploader SingleSteps**, **15557 retired instructions**, and **154286 retired cost units** in all three captures. The executed instruction trace is byte-identical (SHA256 `030d18eb37836fc2b76cdf1cc1c92a8565b6a3be2ef5c9ae59806baf68f68a8b`). Live RTC inputs obey the original subtraction equation; their host-time values are not cross-process constants. These observations do not become a native elapsed input.

Falsifiers reject missing ctor/read/writer/exit, invalid flag/rate/padding values, wrong instance/sample count, mismatched producer or consumer thread, config true/unknown/lease ingress, and unpaired callback/push histories. A different valid backend-rate input is not mislabeled as an impossible value. The output-rate validity remains a sampled input receipt: SetSampleRate is a separate member writer outside the private logging-flag proof. First zero samples bypass both rate outcomes; a general nonzero mixer producer would require its own ownership proof.

Host racing ingress, true-branch WaveFile ownership, and historical original live private-flag parity are not established by this fresh copied-source receipt. The logger mutex and file I/O can alter raced host interleavings. Complete stream ancestry and semantic parity establish the observed source execution, not arbitrary unobserved scheduling equivalence.

## Executed finite native DTK initialization contract

Reviewed corrected proposal `build/dtk-native-owner-42.patch`, SHA256 `c8e7568eec98782670acc1ee930174137824d59ed10721159f0dca5d22a472af`. The explicit owned source capsule supplies Mixer lifecycle, all relevant DumpAudio layers absent, and exclusion of changed config, mutable aliases, logging requests, and lifetime changes. It accepts no observed private flag. The constructor derives false from SoundStream's Mixer(48000) and Mixer.h's member initializer; absent configuration derives MainSettings.cpp312's false default. Unknown/present producer ingress remains a stop. Existing unbound and declared-control modes retain their previous contracts.

At the first DTK consumer, the owned false path delivers no WAV effect, owns pending_blocks=`6`, and schedules deadline=`1699488`, FIFO=`6`, userdata=`0000000300000001`. At G=`20000` it stops before the GPU atomic AllowSleep effect, with GPU as the active callback and no DSP/VI/Movie/NewField/achievement completion claimed. This remains a finite source capsule, not a connected host/UI event provider.

Enabled configuration must first evaluate System.IsAudioDumpStarted at AudioCommon.cpp78. Its value is not owned by this DTK capsule. The corrected proposal stops at that read and does not claim StartAudioDump delivery. Even if that read were later source-bound false, std::time at219, LocalTime, user path/GameID, path creation and WaveFile.Start precede successful flag assignment; LocalTime can return before either logging writer. The earlier proposal's Mixer.WAV-start stop was therefore too late.

The corrected proposal was applied by the coordinator and executed as MSVC test executable SHA256 `312a33d24aa975b57ab9e589dace78c4ee077ad0ff15125f394146495ea43ca9`. My independent ledger compares the complete typed JSON state and ordered journal, with no author validator import. Off mode matches **23 records / 306 leaf fields** and enabled mode **11 records / 187 leaf fields**. Off mode retains the active GPU callback and its undelivered sleep request. Enabled mode retains all six initial queue entries, G=`0`, constructor-owned false flag, no consumer or WAV delivery, and the precise AudioCommon.cpp78 stop. The executable also passed its **41 input/branch declines**.

The initial comparator used Python equality, which permits `False == 0` and `True == 1`, and its mutation loop checked inequality rather than invoking validation. That tooling defect was identified by the coordinator and repaired. The shared exact comparator now recursively requires scalar types, dict key sets, and list arity. Every value/type mutation calls the actual validator. There are **615 off + 377 enabled = 992 independent rejects**, covering 493 leaf fields, all 272 journal value slots including explicit zero padding, plus missing/extra-key and truncated-journal cases. Callback padding is not an observation of CPU Exceptions.

This execution establishes the explicit finite source capsule and its two stops. It does not certify current live host/UI ingress or a complete first Advance. Shared source identities at review are header `178016c5205faa8b866cfda5f6bcd2c078a9e0b812150dfd9cab12fa3ce3f2df`, source `f76ce6ede51bf3812fb5d8c25596adfdd612619cf5f3517b73fb64137e9ac0d2`, test `ba8c11689499c024b536096a93da3fe48a3533a2b3f47ce204cb1897dae350a5`.

## Executed conditional apploader frame owner

The coordinator applied proposal `0d97cb6fff51e48d0b2f0148d7112c4941e93d05cc19aeeea37ea61c8b5d955b` without semantic changes and compiled MSVC executable SHA256 `07075d9cbe4440e9d78e5da10c0227bb37a893304489f1905cc0840702c9096b`. Independent comparison of `build/apploader-entry-msvc-42.json` (SHA256 `9df98f07bcfe8bd068b48c6a3375602db3899d21a05ac7a4d5848a27d2bb96b0`) verifies **22 instruction enter/inner-exit states, ten ordered stores, and 1688 typed leaf fields**. The pinned executable was rerun and passed its **49 negative gates**. Local permissions refused creation of a fresh output file; the immutable coordinator output was reread rather than overwritten. The comparison tool records that distinction.

No captured register, stack, elapsed value, or timing total is passed to the C++ constructor. It owns private bounded backing arrays under explicit source/input conditions, gates complete original assets and exact words, and implements the eleven finite instruction effects. It spills live registers and stores before updating the stack register, retains unrelated state, and treats stmw's three stores as distinct ordered stores. Unsupported exception/endian/alignment/mapping/cache/lease paths decline before the bounded stage. Both incomplete and currently declared-only Complete event capsules are rejected by the native connection gate. This owner stops before `812003b8` and owns no clock, scheduler, TB/DEC, or worker device.

The copied receipts cover r0/r1/LR, r3/r4/r5, r29/r30/r31, CR/MSR/Exceptions/HID and mapping/cache/stack subsets. Complete GPR retention and XER/CTR/FPSCR, SR/GQR/paired singles, IBAT0 and reservation construction have a **source proof**, not a fabricated reference observation:

| Retained state | Exact source producer |
| --- | --- |
| GPR and SR arrays | PowerPC.cpp143..146 resets; PAL BS2 r1/r2/r13 at343/345/347; RunApploader r3/r4/r5 at179..181 |
| both paired lanes | Reset `PairedSingle{}`; PowerPC.h100..101 explicitly initialize both `u64` lanes to zero |
| CTR/GQR and untouched SPR subset | SPR fill at PowerPC.cpp146 |
| FPSCR / reservation | explicit FPSCR=`0`, reserve=`false`, reserve_address=`0` at173 and178 |
| architectural CR | reset internal sentinel `8000000000000001`; ConditionRegister.h63..77 yields no LT/SO, nonzero low32 and negative signed value, hence architectural zero |
| XER | SetXER({}) clears string count/compare, CA and SO/OV; GetXER combines those zero components |
| IBAT0 / DBAT0 / DBAT1 | Boot_BS2Emu.cpp125..130 source overrides `80001fff/2`, `80001fff/2`, `c0001fff/2a` |
| effective cache route | source config refresh, separate from HID0.DCE; the conditional profile must gate this route |

RoundingModeUpdated changes the host floating-point mode from FPSCR; it does not invent a guest FPSCR write. The native profile remains conditional on fresh source reset, original copy/clear, known mapping, no restore/foreign writes, and unchanged CPU/RAM through first Advance. Receipt observations alone do not establish these lifetime exclusions.

All 1688 individual leaf values and scalar types are mutated through the reusable validator. Missing/extra top keys, every store omission, and every instruction-row omission also fail: **3410 rejects**. This includes zero-valued spill omission. Exported JSON does not expose SR/GQR/paired/IBAT/reservation arrays; their proof is source construction/retention and must not be called raw receipt parity. Shared header/source/test hashes are `346f3a7f6b652b3a1c4d0004abe1c37eddc83ae923faf256bfb0a808f18512a1`, `851301dbb6a9c263f71410109088f42a537488908fe67f3d949c21575b364f8f`, `f2d53ba5120ed1e1d62126eab029e02a4f6587208018aaf34d018c318a29276f`.

## GPU atomic dependency: source counterexample and next receipt

At SystemTimers.cpp110..119, GPUSleepCallback calls Fifo::GpuMaySleep, which calls the actual `m_gpu_mainloop.AllowSleep` at Fifo.cpp399..401. BlockingLoop.h233 stores true into its member Flag. Flag.h uses an atomic bool with default sequentially consistent store and compare-exchange. The consequence is associated with that live object and shared modification order.

BlockingLoop's Prepare, Wait/WaitYield, and Fifo::EmulatorState are other sleep-flag producers. Its Run worker executes payload, then at state DONE calls the original TestAndClear at165. A true result permits the DONE→SLEEP transition; a false result allows another busy payload iteration. Prepare initializes the running-state atomic; a constructor default cannot substitute for that live transition. Core.cpp445..481 starts the GPU worker before boot in the dual-core route; source preparation and later worker lifetime must therefore be owned explicitly.

A local unrelated atomic is insufficient. Consider the original flag false and worker in DONE. The original timer store can be consumed as true and permit sleeping; replacing it with a store to a different native object leaves the original flag false and permits more payload executions. At minimum the actual worker state differs. AsyncRequests::QueueEvent pushes the closure before calling RunGpu/Wakeup; a busy worker can consume it in that interval, whereas a sleeping worker requires the later wakeup or timeout. Wakeup is not omitted from this counterexample. The difference is ordering and live worker state, not permanent loss of queued work. Fifo.cpp291 pulls asynchronous events **before** checking the paused flag. Pausing alone cannot suppress those effects. In the non-deterministic route, Fifo.cpp314 calls SetCPStatusFromGPU before checking FIFO data. CommandProcessor.cpp442..504 consumes live FIFO/watermark/control state and can publish a NON_CPU event via UpdateInterruptsFromVideoBackend428. Extra or differently ordered payload iterations can therefore affect callback ordering and, under live CP input changes, interrupt publication. This is a source-valid counterexample to unconditional equivalence, not a claim that the observed baseline executed that interleaving or that initialized CP state necessarily triggers an interrupt.

Queue/device parity at CPU checkpoints does not prove actual BlockingLoop identity, atomic state, worker consumption order, or absence of asynchronous ingress. The immutable source gates included by the independent tool are:

| Source | SHA256 |
| --- | --- |
| Common/BlockingLoop.h | `410f0fc12388580664481862d8cbc975539b7bfb56ecdef935d07c255adb3043` |
| Common/Flag.h | `4f1cb93ddb864189ba5d10e230c1e43986414d7131bd19fb8a789f0072889b00` |
| VideoCommon/Fifo.cpp | `971d31caf503e18e9435202aa7f576a0ab281630a127e22a42063ab30795a15e` |
| VideoCommon/AsyncRequests.cpp | `c564c643eafd485c6bf50f66bbb95a5ba20f7a98a097b5d16a353f132bb19f67` |
| VideoCommon/AsyncRequests.h | `484d89b0605a2242cd76e9bbec8fa2d64646e3eb3cafbe78799cf40a7a8fd7fa` |
| VideoCommon/CommandProcessor.cpp | `c4a3d2fbe508bfe203721917a666c34516e6b72d2c2504096c3e422a4f125623` |
| Core/HW/SystemTimers.cpp | `578abca48d1ab5273d44bdf337431ff66817352d6862e6c28c79890969962bf7` |
| Core/Core.cpp | `8bf9e90fc56b5b4d7a1e42410ea2e2291f44b5d86fb4e982b3aa68912edf5944` |

The smallest useful next passive receipt starts before video initialization and identifies actual BlockingLoop construction/Prepare/Run/destruction. Observe each existing AllowSleep store site once, existing TestAndClear result once, running-state load/decrement results already used by the worker, Wakeup/Stop/Wait transitions, and Fifo emulator-state/config branch inputs. Add exact AsyncRequests passthrough/queue/pull identity and dispatch ancestry, plus CP live condition reads/results and NON_CPU publication ancestry. Do not add atomic loads or repeat TestAndClear to produce the receipt. Capture through the first Advance endpoint; compare the unchanged event projection and exact work/instruction stream. Mutate instance identity, producer/consumer order, worker state and each ingress/publication omission. Multiple observed schedules still do not prove arbitrary racing host interleavings; native admission needs closed ingress and owned lifetime, not merely another matching trace.

## Mandatory validation status

1. Reference capture: Entry42b, nested42, baseline DTK42c and startup-enabled DTK42c accepted under their finite receipt scopes.
2. Semantic reconstruction: raw Entry/callee effects and private logging-flag ancestry independently match. Executed native ownership remains finite and source-conditioned.
3. Perturbation/falsification: **1207** receipt, **992** owned-DTK, and **3410** frame-output mutations rejected. No inference of completeness outside the watched/input scope.
4. Checkpoint parity: reference queue/device projection, normalized work and exact executed trace agree; the new compiled finite owner state/journal/store ledgers match independently.
5. Full repository/gameplay regression: coordinator reports **66/66 repository tests**, **27/27 standalone gameplay probes** and exact stg0100 RESULT parity. Checkpoint 40 production stop remains. These receipts and research owners do not satisfy promotion.

Smallest next native dependency remains the first Advance callback owner: closing a DTK private flag in a fresh source receipt would still leave GPU BlockingLoop's atomic AllowSleep delivery/worker ingress, Movie/NewField and achievements callback effects, then whole apploader stores/devices and state parity.

# Research 42: live DTK receipt and finite logging owner

## Result and boundary

A fresh passive reference now closes the logging-off constructor → configuration → first DTK read chain. A finite C++ research path constructs that state from source inputs, completes the first DTK callback, and stops before GPU `AllowSleep`. It accepts no captured logging flag. The original unbound and declared-control modes retain their exact output bytes.

This does not promote the connected native boot backend. The earliest remaining logging-off dependency is delivery of the GPU `BlockingLoop` atomic request and ownership of its worker. The later Movie/NewField/achievement consumers and equivalence of the source virtual cycle table to physical Gekko elapsed time remain unresolved. The live DTK flag in historical 42d/42f runs remains unobserved; fresh receipts do not retroactively fill it.

## Reference production

`tools/agent_dtk_owner_42.py` builds a separate executable by compiling only copied `Mixer.cpp` and `Common/Config/Layer.cpp`, then linking them ahead of unchanged original libraries and the previously copied GDB/Interpreter objects. Original sources, objects, libraries, executable, and earlier captures are read only.

The source transformations have an exact text inverse against pinned originals. Each existing Mixer condition is read once into a local value; the original branch uses that same value. Constructor logging records the member value and existing backend argument. Start/Stop logging records their original condition, every assignment, every success/failure/already-started/already-stopped exit, and object identity. Destructor identity closes the lifetime. No clock/config/sample/opcode reads are introduced in Mixer observation arguments. There is no added timing read, opcode fetch, guest write, flag forcing, or breakpoint.

The already linked event logger supports an empty start bound. The new all-kind window is `..timing-prefix-end@80373ac4`, limit 3,000,000, including construction before `CoreTiming::Init`. The original phase0 DSP/VI/PI/AI MMIO watches remain bound to capture metadata. The original debugger schedule and entry controls are unchanged.

The copied Layer observer extends existing changed-Set/delete/layer/load hooks to `Main/DSP/DumpAudio`; it observes a mutable DSP section lease and adds readonly DumpAudio layer snapshots. The source `resolve-result` hook also reports the real resolver result. Its existing noninline observer receives already supplied values from inline Set callers. A changed Set cannot bypass this hook in the pinned API. No-op Set attempts return before mutation and are explicitly outside request-attempt observation. A mutable lease, unknown value, incomplete call, pointer reuse, changed thread, or unexplained writer declines.

Fresh final captures:

| Artifact | SHA256 |
|---|---|
| `build/agent-dtk-owner-capture-42c.json` | `0e82d10f8f4cec10baac85b66ff3b6a9c92277cd4d7d2994a6b2c131b1f7ed32` |
| Logging-off events | `942e353f347bd73a08dbde57d47af1a5d16249980a9d117c9a046f73631b6e90` |
| `build/agent-dtk-owner-capture-42c-dump-control.json` | `7b74395dd485a9941356fa61fcb658c065991f6d5d3b336f80fb9aa16dbb20e3` |
| Initial DumpAudio=True events | `3a0db9c360b4cc7953cd3c769389a20678f3aeb90af51e1c8dafc077c452bdd8` |
| Both instruction streams | `030d18eb37836fc2b76cdf1cc1c92a8565b6a3be2ef5c9ae59806baf68f68a8b` |
| `build/agent-dtk-owner-capture-42c.receipt.json` | `e80433f338acef76b6663ac8ba41080065fcbbcc17e045e02b7edcb54b469311` |
| New passive executable | `9da7238efe5ac44ae1f01088c4b95995d0232edb3ceebdd453d6141a9ec3e6b2` |
| Frozen capture helper | `220c45b5526c03ee27612105e006ba967df867960b3b4494bd8883f92c409c6d` |

The frozen helper is archived at `build/passive-dtk-owner-oracle-42c/observer_helper.py`. The current public helper additionally implements native comparison; it does not relabel these earlier capture identities. Its native comparison mode imports and reruns the exact archived auditor before consuming the receipt.

Earlier attempt42 was retained after a raw grouping comparison declined. Attempt42b was retained with its archived helper; it passed semantic checks but lacked a separately retained Mixer compile recipe. Final42c retains and rechecks both compile recipes/logs, both objects, the exact link recipe, compiler/linker, original inputs, and map ownership of Mixer consumers/writers and Layer observers/lifecycle methods.

## Exact source chain

All following paths are relative to `<matching-oracle-source>`.

| Source | Producer or consumer |
|---|---|
| `Source/Core/AudioCommon/SoundStream.h:17` | Constructs `Mixer(48000)` |
| `Source/Core/AudioCommon/Mixer.h:194` | Private `m_log_dtk_audio` initializer is false |
| `Source/Core/AudioCommon/Mixer.cpp:36` | Constructor; observer before config callback registration |
| `Source/Core/AudioCommon/Mixer.cpp:52` | DoState serializes FIFOs, not the logging flag |
| `Source/Core/Core/Config/MainSettings.cpp:312` | DumpAudio absent-setting default is false |
| `Source/Core/Common/Config/Layer.h:148` | Changed Set writes then invokes the existing audit hook |
| `Source/Core/Common/Config/Layer.cpp:88` | DeleteKey; DeleteAllKeys/load/snapshots also covered |
| `Source/Core/Common/Config/Layer.cpp:113` | Mutable section is an alias escape; the new receipt gates DSP leases |
| `Source/Core/Common/Config/Config.cpp:246` | Actual resolver result |
| `Source/Core/AudioCommon/AudioCommon.cpp:78` | False DumpAudio short-circuits `IsAudioDumpStarted` and StartAudioDump |
| `Source/Core/AudioCommon/Mixer.cpp:239` | Streaming sample call; count0 performs no sample loop work |
| `Source/Core/AudioCommon/Mixer.cpp:253` | Separate live logging read after the count0 loop |
| `Source/Core/AudioCommon/Mixer.cpp:354` | Start condition; only successful writer at360 sets true |
| `Source/Core/AudioCommon/Mixer.cpp:378` | Stop condition; writer at380 clears false |

The private flag has only its initializer and these two assignments in the pinned source. Source search found no other writers; DoState and RefreshConfig do not own it. `GetLayerMap()` returns a const reference; the mutable-section interface is observed and no DSP lease appears in the admitted interval. Source ownership is finite and specific to these normal APIs, not arbitrary memory corruption or racing host behavior.

The logging-off capture records:

- Constructor seq13901 on CPU thread21544: object `00000262e2cf4040`, flag0, backend48000.
- CoreTiming Init seq13926; first DTK dispatch seq15676: deadline0, FIFO0, userdata `0000000300000001`, late20000.
- Push seq15677: same object/thread, sample count0, original output-rate predicate1.
- Original log condition seq15678: same object/thread, flag0, sample count0.
- Successor seq15681: deadline1699488, FIFO6, relative1679488, same userdata; callback return15682; GPU dispatch15683.
- Twenty pre-read config lifecycle/result records, all absent; layers0/1/2/3/6, no changed Set, no true value, no mutable DSP lease.
- Two nested DTK consumers through the closed endpoint: counts0 and168, both logging false. No Start/Stop entry or flag assignment occurs anywhere in that stream.

This is constructor plus complete effective-producer ancestry, not just a false frontier sample. The parser also requires exact dispatch/push/read/return nesting and same-thread calls, complete writer exits, and no unmatched sample request.

The logger mutex orders observations, not the emulator object. We do not claim equivalence for arbitrary raced host ingress. The native capsule explicitly admits a closed ingress contract; unknown changed configuration, logging requests, mutable aliases, or lifetime changes stop before their state can be consumed. No-op requests remain semantically inert but are not claimed absent as human actions.

## Perturbation and parity

The second fresh profile changes only initial `Dolphin.DSP.DumpAudio=True`. It observes one real Layer Set, the actual StartLogDTKAudio condition, successful true assignment and completed exit, then a first sample-count0 read of true. The flag was never directly forced. This falsifies a model that assumes false unconditionally or ignores logging because count0.

The true path can read live system state, host time and create directories/WAV files. These effects are retained as unowned dependencies; they are not part of the logging-off conclusion.

Both captures have exactly 1,291 matching source queue/device projection records against the original42d baseline. That projection includes init/config/init-ready/register, every queue publication/mutation/dispatch/return, boundaries, the endpoint and all VI semantic rows. Step/retirement compression grouping can change when observations flush runs; the complete timing auditor independently reconstructs those recurrences. Both full instruction streams are byte-identical to baseline, pre-entry work remains2,656,493 units, and source prefix work154,246 units. RTC-local/result equations and RTC seconds→40,500,000 TB units are checked separately.

Both current C++ clock research comparisons match39 machine checkpoints,43 clock phases,116 full-end fields,104 known memory bytes and4 ordered stores, with348 output-corruption declines each. These are source-conditioned research results; physical elapsed-work equivalence and broader device ownership are not inferred.

Author capture checks reject12 malformed ancestry cases and22 altered build bindings. Independent receipt review adds154 logging-off and188 enabled source/lifecycle/thread/config corruptions. Its separate normalization confirms the same produced work and records. Historical42d/42f capture/tool hashes were rechecked unchanged.

## Executed C++ owner

The coordinator applied `build/dtk-native-owner-42.patch` (`c8e7568eec98782670acc1ee930174137824d59ed10721159f0dca5d22a472af`) and compiled the separate initial-event research target. This library remains outside production boot ownership.

The API is:

```cpp
auto owner = InitialBootEventOwner::WithOwnedDtkLogging(
    FreshInitialBootSourceConfig42(), FreshInitialDtkLoggingSource42());
owner.FirstAdvance(authorization);
```

`InitialDtkLoggingSourceInputs` contains typed lifecycle, configuration-origin and ingress states, not a logging flag. The native constructor creates backend48000 and false from the source initializer. Absent configuration layers resolve through the source default, so PostInit cannot request dumping. The sole private logging value is produced inside C++; observer values are witnesses only.

Unknown or present config/alias/lifetime ingress stops before Advance. Unknown or present Start/Stop ingress stops at the actual log read, retaining the preceding zero-transfer and zero-sample request. Initial enabled configuration stops at `AudioCommon.cpp:78 unknown-audio-dump-started`: `IsAudioDumpStarted` is unowned, before StartAudioDump's later `std::time`, directory and WAV effects. Neither WAV success nor a true private member is invented.

The owned logging-off CLI `--dump-owned-dtk` executes23 journal records and stops at `BlockingLoop.h:233 undelivered-allow-sleep`. It retains G20000, slice20000, downcount0, sane1, FIFO7, active GPU callback, DVD pending6, DTK successor1699488/FIFO6, unchanged PI10100/mask0/exceptions0, no GPU delivery, DSP/VI/NewField effects or guest RAM writes. No observed frontier count or flag is a program input.

`--dump-owned-dtk-enabled` stops before the live system dump-state read, with G0/FIFO6, constructor false and no sample call. The old `--dump` and `--dump-bound-control` outputs remain byte-identical:4,232 and9,030 bytes respectively. The compiled C++ tests pass41 decline cases, including every unknown/present ingress, invalid enum, mixed declared/owned input and enabled path, with earlier declared-control semantics preserved.

Final author native comparison:

| Artifact | SHA256 |
|---|---|
| Current `agent_dtk_owner_42.py` | `08ae4342906b49821a863391277c5e4781395af1dcd0d3bba697ffcfcd6c6262` |
| Native initial-event executable | `312a33d24aa975b57ab9e589dace78c4ee077ad0ff15125f394146495ea43ca9` |
| `InitialBootEvents.hpp` | `178016c5205faa8b866cfda5f6bcd2c078a9e0b812150dfd9cab12fa3ce3f2df` |
| `InitialBootEvents.cpp` | `f76ce6ede51bf3812fb5d8c25596adfdd612619cf5f3517b73fb64137e9ac0d2` |
| `test_initial_boot_events.cpp` | `ba8c11689499c024b536096a93da3fe48a3533a2b3f47ce204cb1897dae350a5` |
| `build/agent-dtk-owned-native-comparison-42.json` | `a76cd0ccb8bb8a40dd16b184db79054d188d28c5a2070218006eec11805c7ea9` |
| Owned native output | `3bb6ea127cbac9763bf7c5fa996c4f5d81cabc5b0097bc44f49319143933257d` |

The comparator reruns the complete frozen44-pin logging-off and positive-control receipt audits before validation. It checks receipt replay fields and every exported owned field and journal record through strict recursive types and exact key/list sets, then compares the six genuine dispatch/push/read/enqueue/return/GPU records and constructor values. It rejects769 logging-off and495 enabled field/omission/extra-key/numeric-bool corruptions. Every mutant passes through the reusable strict validator; Python's equality of false and zero cannot admit an altered field. Captured values never enter native construction.

The independent validator imports no author validator and matches the complete typed state/journal:23 records and306 leaf fields for logging off,11 records and187 leaf fields for initial enabled configuration. Its reusable strict checker rejects615 off and377 enabled mutations,992 total, including numeric bools, missing/extra keys, zero padding and truncated journals. Its final report `build/owner-receipt-adversarial-final-42.json` is pinned to `f593e776ab39c03e7c269a58279ecdeeb3a02d3a56d52eca8f9245c130a0e6ec`; detailed scope and source review are recorded in `OWNER_RECEIPT_ADVERSARIAL_42.md`.

The coordinator's final full repository MSVC run passed66/66 suites in27.03 seconds. Standalone gameplay passed27/27 in14.08 seconds, including the required `RESULT Dark mission -> stage6`. The current clock, prefix and initial-event executable hashes stayed unchanged after that build. These regression gates preserve checkpoint40 production and do not promote this finite owner.

The native comparison can be reproduced from the repository root with explicit private source and raw DOL locations:

```powershell
python -u -B reverse/boot/tools/agent_dtk_owner_42.py --compare-owned-native `
  --program build/reverse/boot/Release/shadow_boot_initial_events_tests.exe `
  --program-sha256 312a33d24aa975b57ab9e589dace78c4ee077ad0ff15125f394146495ea43ca9 `
  --old-program build/agent-dtk-old-initial-tests-42.exe `
  --old-program-sha256 2a3fc25920c3ef5c5fe323f5cfbd87d4cbf1a9ec70ebf52be4cde7f6a3407d81 `
  --capture build/agent-dtk-owner-capture-42c.json `
  --receipt build/agent-dtk-owner-capture-42c.receipt.json `
  --receipt-sha256 e80433f338acef76b6663ac8ba41080065fcbbcc17e045e02b7edcb54b469311 `
  --source-root '<matching-oracle-source>' `
  --baseline build/passive-timing-zero-42d.json `
  --baseline-audit build/agent-source-bound-zero-audit-42d.json `
  --clock-program build/reverse/boot/Release/shadow_boot_clock_research.exe `
  --prefix-program build/reverse/boot/Release/shadow_boot_native_bi2.exe `
  --dol '<raw-main.dol>' `
  --out build/agent-dtk-owned-native-comparison-42.json
```

## Scope review and next experiment

Automatic approval review rejected this agent's shared native-source write because it still applied the earlier helper/doc/build-only restriction. This was a scope issue, not missing behavioral evidence. The agent did not retry or bypass that rejection; it produced the permitted unified proposal under build. The coordinator reviewed and applied it under the human's C++ reconstruction authorization.

The next finite owner must deliver GPU's `m_may_sleep.Set()` effect to the actual BlockingLoop and preserve interaction with its worker's `TestAndClear`, without assuming its final atomic value. `SystemTimers.cpp:110` → `Fifo.cpp:399` → `BlockingLoop.h:233` reaches a shared atomic, not a private queue-only field. A local unrelated atomic does not own that effect: the real worker can remain busy, pull asynchronous events and evaluate live CP state on a different iteration. CPU checkpoint parity therefore cannot substitute for worker identity and publication order.

A decisive passive experiment starts before video initialization, covers construction/Prepare/Run/destruction, every genuine AllowSleep producer and original TestAndClear result, Wakeup/Stop/Wait, AsyncRequests queue/pull ancestry and CP/NON_CPU publication ingress. It must use existing values without added atomic reads, repeated consumers, timing reads or forced state. A complete receipt plus an admitted closed ingress/lifetime contract is needed; observing another raced schedule alone is insufficient. Until that owned effect contract is established, this capsule stops there. Even after that, Movie frame counters, frame-step, achievements and physical work→time equivalence retain their independent gates.

### Selected original source pins

The receipt report contains all44 pins and the private manifest all build/input bindings. Core, AudioCommon and Common library hashes remain `eb0e5041b253072d735ddaac5fba9079ed2ca4916f26312845c706bd132b2939`, `6274ea941d8545b3367bbd35cf1cebda8a88cb4a6ed1555e61c18cef9aeae8b2` and `e230a2f6d17d6588dd1bae6ee72dc4b4331bb29ad79fbb531ea420a536341532`.

| Relative source | SHA256 |
|---|---|
| `Source/Core/AudioCommon/Mixer.cpp` | `24f91152ffe824019053f89acc77a974992fa9ed19e68b0c94194521987f90a3` |
| `Source/Core/AudioCommon/Mixer.h` | `612025371230d433dcf62e027d1abd2da8be5c3533a8ed2947a0e3b6f9480bd5` |
| `Source/Core/AudioCommon/AudioCommon.cpp` | `de119dfd09a5f10309bbdd0b2de83f5666a8c2f413bf87143ab3322d7b527430` |
| `Source/Core/AudioCommon/SoundStream.h` | `6d78d46f205f66a6804e59975df1d2344f4474021f59b1a85bf7716af3374a43` |
| `Source/Core/Common/Config/Layer.h` | `66fed0ac7d91e93e1f6168e837d1c8efa837df8d106993060eb6bc8cde9b2470` |
| `Source/Core/Common/Config/Layer.cpp` | `af5702de9c65d59902989b362da716acbc8bdcfb9a8c9fbf23ab67f389ad2c8f` |
| `Source/Core/Common/Config/Config.cpp` | `7cc14cab8048767c336c5219234b1527a434576cc9db09db13ccc716bfcacb05` |

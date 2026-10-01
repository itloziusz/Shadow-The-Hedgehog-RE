# Shadow the Hedgehog (PAL GUPP8P) — SYS boot reverse map

This directory holds PAL GUPP8P boot evidence and bounded native C++17 slices.
`shadow_boot_probe` translates the DOL entry branch at
`0x80003154` and the complete register helper at `0x800032B0..0x8000333C`,
then the direct call at `0x80003158`, stopping at `0x80003400`. With a supplied
observed MSR, it translates the first five wrapper words and the paired
callee's four-word stack prefix, stopping before the HID2 read at
`0x80370BA8`. An additional supplied HID2 word runs the two-word accessor
then issues the HID2 OR/write request and stops at `0x8037172C`. A third
explicit HID0 input runs the ICFI request leaf and stops at `0x80371730`,
before `sync`. It does not execute GQR, FPR or cache effects, or equate an
issued SPR request with a later hardware readback.
The paired-setup stack stores are applied as bounded, validity-tracked
big-endian guest bytes; the later saved-LR load remains behind the `sync` stop.
The separate `RunApplicationRecurringPhase` translates the static event and
exit sequence at `0x800511E0..0x80051218` with explicit unresolved hooks.
**PROVEN for these bounded slices; full hardware, OS, constructors and game
boot remain UNKNOWN or incomplete.** These fixture-backed components do not
make a playable game.

Latest research42 connects original apploader work and native prefix work
to the compiled continuous clock experiment. Three fresh profiles compare
129 clock phase checkpoints and117 available machine checkpoints, including
complete endpoint state, without feeding observed frontier cycles into C++.
Full Release **66/66**, standalone gameplay **27/27**, required route pass.
Production remains checkpoint40, **before80379628**: the native event/device
lifecycle is not completely owned. Fresh producer-bound DTK research now
executes to the undelivered GPU AllowSleep effect. The original unbound mode
still stops at Mixer.cpp:253. Original Entry/callee receipts support11 native
words/10 stores through before812003B8; this slice owns no clock/scheduler
and rejects current incomplete and declared-only Complete event owners.
Worker/async effects, callback private state and progressive apploader memory
stay gated. Read `research/CLOCK_PRODUCTION_RESEARCH_42.md`
first; it records the exact pre-entry blocker, next experiment and decline.

Checkpoint41 is **research**, not a connected frontier advance. The new
`shadow_boot_clock_research` executes source-derived TB arithmetic and exact
sampler/retry/offset/EE/return/store effects through before **80373AC4**. Four
full-entry profiles compare182 CP/21112 fields/18928 known stack bytes, including
original full64 and signed-high rollover retries. Final Release **60/60** and
gameplay **27/27** plus the required route pass. Production checkpoint40 stays
before **80379628** until native elapsed time and events are proven. Read
`research/CLOCK_RESEARCH_41.md`, its raw ledger and independent audits first.
No host clock, interpreter or per-read TB result was introduced.

Checkpoint 40 adds `shadow_boot_native_bi2`: complete measured BI2 ownership,
two fresh pointer reads, debug/relocation routes, coherent SDA metadata and
first OS/clock frames. It stops before **`80379628`**, the first live time-base
read. Six independent replays match191 new CP/22156 fields/1564672 BI2 bytes;
all earlier handler/CRT checkpoints rerun. Final Release **56/56**, gameplay
**27/27** and required route pass. Read `research/NATIVE_BI2_COMPLETION_40.md`
and checkpoint40 first. Live clock/offset producers, retail/hardware timing,
later OS/constructors and first-frame/pixel parity remain UNKNOWN.

Checkpoint39 adds `shadow_boot_native_crt`: connected handler registration,
actual stack/return effects and exact three-range CRT zeroing from entry.
Three complete replays match246 CP/28200 fields; the new tail compares1473816
cleared bytes with hostile pre-entry seeds and preserved boundary canaries.
Unknown old-slot words stop803733B4; admitted inputs reach **80003188**, before
live BI2 pointer800000F4. Full Release **53/53**, gameplay **27/27** and required
route. Read `NATIVE_CRT_COMPLETION_39.md` and checkpoint39 first; later BI2/OS,
physical/retail and first-frame/pixel gates remain unresolved.

Checkpoint 38 adds `shadow_boot_native_l2`, which calls the unchanged
checkpoint-37 entry runner then traces live L2CR through both E branches,
both IP polls, MSR restoration and the leaf logger. The bounded private
completion requires no full L2 cache and accepts only explained E/CE/DO/WT
native states; pending I/IP, TS and reserved bits decline. Three entry replays
match **10,602 raw fields + 2,228 known-memory byte comparisons** and 24 new
committed words. It stops before **`80372904`**, the handler-install call
with unresolved live slot `80586CB4`. Full Release CTest: **51/51**;
standalone gameplay: **27/27** plus the required stg0100 route.
Read `research/NATIVE_L2_COMPLETION_38.md`, its three independent audits and
`PROGRESS.md` first. No physical cache timing, dirty-cache history or retail
input is inferred. First-frame/runtime/pixel requirements remain outstanding.

Checkpoint 37 adds `shadow_boot_native_prefix`, an entry-owned immutable
native backend. It completes the local sync consequence, all GQR writes,
the FPR seed and enabled ICE/DCE checks, stopping before live L2CR at
**`0x80372894`**. Three controlled pre-entry profiles compare 17 checkpoints
each: **5,763 state fields + 444 known stack bytes**. All 166 checked words
have mutation gates. The old HLE HID0 includes an unimplemented DCFI command;
the new backend rejects that word. No retail input or physical bus timing
is inferred. Read `research/NATIVE_SYNC_COMPLETION_37.md` and `PROGRESS.md`
first. Complete Release CTest: **49/49**. Checkpoint 36's six standalone
FPR experiments and reproduced FPSCR/RSP fixes remain valid historical
evidence in `research/HIDDEN_BOOT_STATE_36.md`.

The original boot handoff below was written for a static recompiler. This
public repository keeps its authored evidence and portable C++ slice. Raw
`sys/` inputs, generated manifests and disassembly dumps named by the older
handoff are intentionally excluded; use a read-only, legally obtained PAL
`main.dol` to run the fixture tests.

## Source identity

- Game ID: **GUPP8P** (PAL)
- Title: **SHADOW THE HEDGEHOG**
- DOL entry: **0x80003154**
- Apploader revision/date: **2004/11/10**
- FST entries: **3615** (3480 files, 135 directories)
- Input archive SHA-256: `6a64e64da9573250a86e3eb63564964787d6175861b12369a764acab3047f622`

## What each file becomes in a native build

| Original | Original purpose | Native recompiler treatment |
|---|---|---|
| `boot.bin` | Disc header and offsets | Parse at build time; emit only required constants/metadata. Do not ship the loader header as executable logic. |
| `bi2.bin` | Boot information / memory and region configuration | Parse at build time; synthesize only guest-visible low-memory values that the game actually reads. |
| `apploader.img` | DVD-to-RAM DOL/FST loader | Candidate native loader replacement; preserve each game-observed handoff effect until pre-entry parity is demonstrated. |
| `main.dol` | Actual game executable | Primary PowerPC translation input. |
| `fst.bin` | Disc filesystem index | Parse at build time into an asset manifest or generated lookup table. |

The core rule is to preserve every observed guest effect. Console-specific
operations may be replaced only after their producers and consumers are traced.

## Files in this public directory

- `BOOT_PROCESS.md` — boot-chain dependency map and proposed native cuts.
- `APPLOADER_STATE_MACHINE.md` — loader callback/state-machine map.
- `DOL_STARTUP.md` — DOL/CRT initialization path and memory tables.
- `MINIMAL_BOOT_FOUNDATION.md` — historical first-entry boundary and evidence.
- `REFINED_BOOT_GRAPH.md` — original boot dependencies and proposed native cuts.
- `OPEN_QUESTIONS.md` — prioritized barriers to a real native game boot.
- `PROGRESS.md` — connected native prefix, first fail-closed stop and next
  original-state comparison required by the run/trace/fix workflow.
- `ARCHAEOLOGY_INDEX.md` — binary-first coverage, proof ladder and exact
  boundaries for each boot region.
- `recognizer/` — hash-pinned raw-word semantic recognizer with bounded CFG,
  symbolic state, structural detectors, SQLite evidence DB, unknown-region
  clustering, and fail-closed boot-frontier analysis. See its `README.md` and
  `research/SEMANTIC_RECOGNIZER_32.md` and `SEMANTIC_RECOGNIZER_33.md` for
  the scans and recovered multiword effects. `MOTION_TABLE_CONSTRUCTORS_34.md`
  records the eight NPC MotionImpl copy graphs and their validation limits.
- `research/CLOCK_RESEARCH_41.md`, `BINARY_CLOCK_PREFIX_41.md` and the three
  `CLOCK_*_AUDIT_41.md` notes — source/units/phase/rollover/consumer evidence,
  executed research parity and the unclosed native clock provider.
- `research/CLOCK_PRODUCTION_RESEARCH_42.md` and independent
  `TIMING_SOURCE_AUDIT_42.md`, `TIMING_NATIVE_WORK_42.md`,
  `TIMING_ADVERSARIAL_42.md`, `OS_TIMER_OWNERSHIP_AUDIT_42.md` — raw-derived
  elapsed work, pre-entry phase, queue/callback recurrence, F0 and timer evidence.
- `research/INITIAL_EVENT_OWNER_42.md`, `INITIAL_EVENT_ADVERSARIAL_42.md`,
  `APPLOADER_ENTRY_OWNER_42.md` — finite native first-Advance effects,
  independent counterexamples and the historical missing pre-entry receipts.
- `research/APPLOADER_ENTRY_RECEIPT_42.md`, `DTK_OWNER_RECEIPT_42.md`,
  `OWNER_RECEIPT_ADVERSARIAL_42.md` — passive original producers/consumers,
  executed native research, exact typed state diffs and GPU counterexample.
- `include/shadow/boot/InitialBootEvents.hpp`, `src/InitialBootEvents.cpp` —
  finite research owner with explicit live branch/effect bindings and an
  immutable partial stop; not connected to the production boot runner.
- `include/shadow/boot/ApploaderEntryFrames.hpp`, `src/ApploaderEntryFrames.cpp`
  — original-byte-gated11-word frame reconstruction, private64-byte storage,
  ordered live-register spills and an explicitly closed production connector.
- `include/shadow/boot/ApploaderElapsedWork.hpp`, `BootElapsedWork.hpp` and
  their `src/` implementations — finite semantic work producers, explicit
  conditional premises, raw input gates and no runtime opcode interpreter.
- `tools/build_timing_trace_oracle.py`, `capture_timing_prefix.py` — private
  copied passive observer and strictly closed continuous capture.
- `tools/validate_produced_clock.py` — all clock phases, occurrence-bounded
  machine state, ordered original stores and adversarial differential checks;
  does not admit a native event owner or promote a checkpoint.
- `tools/agent_initial_events_42.py`, `agent_initial_event_adversarial_42.py`,
  `agent_apploader_entry_42.py` — complete capture admission before event
  subset comparison, independent falsification, raw/source Entry proof gates.
- `tools/agent_entry_receipt_42.py`, `agent_dtk_owner_42.py`,
  `agent_owner_receipt_adversarial_42.py` — independently bound build/source
  receipts, native execution/comparison and typed mutation/omission rejection.
- `include/shadow/boot/TimeBaseSemantics.hpp`, `src/TimeBaseSemantics.cpp` —
  pure counter/carry/CR semantics and explicitly separate SingleStep projection.
- `tools/capture_clock_state.py`, `validate_clock_research.py` — producer/phase,
  ordered stores/current owners, control provenance and full-prefix regression.
- `research/` — independent pre-entry, HID2/HID0, FPR-lane, L2, CRT-memory,
  constructor and application-loop audits with exact address evidence, including assembly CFG/data-flow and
  a synthetic Dolphin/HLE checkpoint study. The latter is not retail IPL proof.
- `research/ICFI_SYNC_BOUNDARY.md`, `SYNC_GQR_CHAIN.md`,
  `POST_SYNC_GQR_PROJECTION_35.md`,
  `PAIRED_STACK_MEMORY.md`, `PAIRED_FPR_BREAKTHROUGH.md`, `L2_STATUS_ORACLE.md` and
  `OS_ENTRY_NEXT_BOUNDARY.md` — new bounded hardware, FPR and OS proof
  obligations beyond the connected stop. `NATIVE_PREFIX_ADVERSARIAL_29.md`
  records the fixed supervisor-privilege divergence.
- `tools/capture_dolphin_rsp.py` — hash-pinned external PPC checkpoint capture
  for validation only; the native executable does not depend on Dolphin.
- `tools/make_boot_oracle_disc.py`, `build_readonly_boot_oracle.py`,
  `capture_boot_machine_state.py`, `validate_fpr_capture.py` — reproduce the
  synthetic SYS-only fixture, build a separate read-only debugger export,
  capture hidden state and compare the bounded native FPR projection.
- `include/shadow/boot/NativeBootPrefix.hpp`, `src/NativeBootPrefix.cpp`,
  `tools/native_prefix_probe.cpp`, `tools/validate_native_prefix.py` — connected
  entry run, private completion contract, actual bytes and controls, explicit
  input CLI and same-run downstream comparison. `test_native_prefix.cpp`
  covers all 166 words, adverse inputs and later stack aliases.
- `include/shadow/boot/NativeBi2Prefix.hpp`, `src/NativeBi2Prefix.cpp`,
  `tools/native_bi2_probe.cpp`, `capture_bi2_state.py`, `validate_native_bi2.py`
  — connected typed BI2/OS prefix, controlled experiments and complete-byte
  state diffs. `test_native_bi2.cpp` rejects95 raw mutations; `test_bi2_tools.py`
  protects provenance, packed GPR slices and original branch hints.
- `include/shadow/boot/NativeCrtPrefix.hpp`, `src/NativeCrtPrefix.cpp`, `tools/native_crt_probe.cpp`,
  `capture_crt_state.py`, `validate_native_crt.py`, `boot_state_diff.py` —
  connected handler/CRT semantics, reusable opt-in capture and strict repeated-PC
  comparisons. `test_native_crt.cpp` rejects216 raw/data mutations; the state
  schema gate independently checks shape, validity, seeds and occurrence order.
- `tools/scan_boot_motifs.py` — scans original text with shared raw EE/fill/
  time-base detectors; persists UNKNOWN candidates without promoting them.
  `BINARY_BI2_OS_PREFIX_40.md` retains the registered exact95-word ledger.
- `include/shadow/boot/NativeL2Prefix.hpp`, `src/NativeL2Prefix.cpp`,
  `tools/native_l2_probe.cpp`, `tools/capture_l2_state.py`,
  `tools/validate_native_l2.py` — separate checkpoint38 continuation, controlled
  entry/poll experiments and strict full-chain comparison. `test_native_l2.cpp`
  checks sixteen admitted profiles and 95 new word mutations;
  `test_l2_capture.py` checks provenance, effect order and memory validity.
  `research/NATIVE_L2_COMPLETION_38.md` and the three `L2_*_AUDIT_38.md`
  files distinguish native completion from unresolved physical cache state.
- `include/shadow/boot/FprSeedProjection.hpp`, `src/FprSeedProjection.cpp` —
  explicit-input paired/scalar state projection with fail-closed unsupported
  paths; supplied from the connected entry runner only in its admitted profile.
- `include/shadow/boot/BootFoundation.hpp`, `src/` — C++17 section-backed
  register startup, wrapper/stack prefix with applied BE32 bytes, explicit-input HID2 read and
  issued HID2 and HID0 SPR write requests, plus a conditional, unconnected
  projection of the thirteen post-sync GQR/stack-return words,
  recurring event loop and command-line probe. A separate
  `ConstructorTableProjection.hpp`/`.cpp` applies only the proven global
  three-word copy graph for eight NPC MotionImpl static initializer bodies;
  it is not on the connected boot path.
- `tests/` — exact DOL SHA-256 gate, expected stop state and mutations of all
  36 helper instructions, entry words, descriptors, invalid reads and the
  recurring event/exit order. The HLE-input gate checks the connected prefix
  through the HID0 ICFI request at `0x80371730`; a separate executable
  unit gate checks Gekko `mtfsf` summary arithmetic. The CRT return projection
  gate checks two corrected register claims without claiming connected CRT execution.
- `experimental_native_boot/` — reference-only older authored experiments;
  its generated recompiler dependencies are not in this repository.

Build with the root CMake project. Supply
`-DSHADOW_BOOT_DOL_PATH=<path-to-PAL-main.dol>` to enable the content-backed
CTest cases. This path stays in the ignored build cache and is never committed.
The PAL fixture also enables `boot_pal_semantic_recognizer`, which checks the
raw frontier words and adversarial detector/data-flow mutations.

The ordinary probe stops at `0x80003400`. An independently measured MSR can
be supplied as `shadow_boot_probe <main.dol> --observed-msr 00002032` to stop
at the HID2 read. Add `--observed-hid2 E0000000` to run that accessor and stop
at `0x8037172C` after reporting the HID2 write request. Add
`--observed-hid0 0011C464` to report the ICFI request and stop at
`0x80371730` before `sync`. All supplied values came from a startup-only
synthetic HLE run and are not retail boot defaults.
The probe declines a user-mode MSR before privileged `mfmsr`; a dedicated
negative CLI gate prevents regression. This historical executable remains
request-only. The new executable uses an **explicit entry fixture**, produced
by `validate_native_prefix.py` from a controlled reference capture:

```powershell
build/reverse/boot/Release/shadow_boot_native_prefix.exe <PAL-main.dol> <explicit-entry.txt>
```

The fixture order is MSR/HID0/HID2/CR/XER/CTR/FPSCR (seven 32-bit hex words),
32 raw PS0 and 32 PS1 64-bit words, eight GQR words, source address and four
BE32 source words. There is no supplied intermediate PC/LR/GPR, expected
branch, or sync acknowledgment. Missing/extra fields decline. The supported
mode and reproduction instructions are in `NATIVE_SYNC_COMPLETION_37.md`.

The L2 continuation uses the same fixture followed by one explicit pre-entry
32-bit L2CR word. `validate_native_l2.py` produces it from a controlled capture:

```powershell
build/reverse/boot/Release/shadow_boot_native_l2.exe <PAL-main.dol> <explicit-entry-with-l2.txt>
```

Its supported profile and reproduction/decline requirements are in
`NATIVE_L2_COMPLETION_38.md`. It cannot accept a supplied poll result or
completion acknowledgment; the native owner produces those effects.

## Confidence convention

Current assembly notes use `PROVEN` for exact instructions or observed bytes,
`STRONG` for conclusions compelled by several checked facts, `LIKELY` for
supported interpretations, and `UNKNOWN` where an input or effect is missing.
`OBSERVED-HLE` labels the startup-only Dolphin run and never implies retail
IPL parity. Some older notes retain `CONFIRMED`/`STRUCTURAL`/`INFERRED` labels;
read those as historical scope, not as a stronger parity claim.

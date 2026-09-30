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
**PROVEN for these bounded slices; full hardware, CRT, constructors and game
boot remain UNKNOWN or incomplete.** These fixture-backed components do not
make a playable game.

Checkpoint 36 adds a read-only reference export for PS1/GQR/HID2/cache state
and an **unconnected** native projection of `0x80370CDC..0x80370E00`.
Six controlled HLE runs match 4,896 raw fields; all 74 words have mutation
gates. A genuine FPSCR reserved-bit bug was reproduced and fixed in both
implementations. See `research/HIDDEN_BOOT_STATE_36.md`. The connected stop
remains before sync: the reference interpreter ignores that barrier and
cannot prove physical ordering. The complete Release CTest gate is **48/48**.

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
- `include/shadow/boot/FprSeedProjection.hpp`, `src/FprSeedProjection.cpp` —
  explicit-input paired/scalar state projection with fail-closed unsupported
  paths; not connected through the unresolved barrier.
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
negative CLI gate prevents regression. Crossing `sync` still requires
validated cache-command completion and separate HID0 readback.

## Confidence convention

Current assembly notes use `PROVEN` for exact instructions or observed bytes,
`STRONG` for conclusions compelled by several checked facts, `LIKELY` for
supported interpretations, and `UNKNOWN` where an input or effect is missing.
`OBSERVED-HLE` labels the startup-only Dolphin run and never implies retail
IPL parity. Some older notes retain `CONFIRMED`/`STRUCTURAL`/`INFERRED` labels;
read those as historical scope, not as a stronger parity claim.

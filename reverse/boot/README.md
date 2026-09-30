# Shadow the Hedgehog (PAL GUPP8P) — SYS boot reverse map

This directory holds PAL GUPP8P boot evidence and bounded native C++17 slices.
`shadow_boot_probe` translates the DOL entry branch at
`0x80003154` and the complete register helper at `0x800032B0..0x8000333C`,
then the direct call at `0x80003158`, stopping at `0x80003400`. With a supplied
observed MSR, it also translates the first five hardware-helper instructions
and stops at `0x80371714`. It does not execute the HID/GQR/FPR/cache path.
The separate `RunApplicationRecurringPhase` translates the static event and
exit sequence at `0x800511E0..0x80051218` with explicit unresolved hooks.
**PROVEN for these bounded slices; full hardware, CRT, constructors and game
boot remain UNKNOWN or incomplete.** These fixture-backed components do not
make a playable game.

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
- `research/` — independent pre-entry, hardware, CRT and application-loop
  audits with exact address evidence, including assembly CFG/data-flow and
  a synthetic Dolphin/HLE checkpoint study. The latter is not retail IPL proof.
- `tools/capture_dolphin_rsp.py` — hash-pinned external PPC checkpoint capture
  for validation only; the native executable does not depend on Dolphin.
- `include/shadow/boot/BootFoundation.hpp`, `src/` — C++17 section-backed
  register-startup and five-instruction hardware-entry slices, recurring event
  loop and command-line probe.
- `tests/` — exact DOL SHA-256 gate, expected stop state and mutations of all
  36 helper instructions, entry words, descriptors, invalid reads and the
  recurring event/exit order. The HLE-input gate checks the connected prefix
  through `0x80371714`; a separate executable unit gate checks Gekko
  `mtfsf` summary arithmetic; it does not advance the connected boot prefix.
- `experimental_native_boot/` — reference-only older authored experiments;
  its generated recompiler dependencies are not in this repository.

Build with the root CMake project. Supply
`-DSHADOW_BOOT_DOL_PATH=<path-to-PAL-main.dol>` to enable eight content-backed
CTest cases. This path stays in the ignored build cache and is never committed.

The ordinary probe stops at `0x80003400`. An independently measured MSR can
be supplied as `shadow_boot_probe <main.dol> --observed-msr 00002032` to run
the proven five-instruction prefix. `00002032` came from the documented
startup-only synthetic HLE run and is not a retail boot default.

## Confidence convention

Current assembly notes use `PROVEN` for exact instructions or observed bytes,
`STRONG` for conclusions compelled by several checked facts, `LIKELY` for
supported interpretations, and `UNKNOWN` where an input or effect is missing.
`OBSERVED-HLE` labels the startup-only Dolphin run and never implies retail
IPL parity. Some older notes retain `CONFIRMED`/`STRUCTURAL`/`INFERRED` labels;
read those as historical scope, not as a stronger parity claim.

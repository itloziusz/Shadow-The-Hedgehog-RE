# Shadow the Hedgehog (PAL GUPP8P) — SYS boot reverse map v1

This directory holds the PAL GUPP8P boot evidence and the first buildable native
startup slice. `shadow_boot_probe` translates the DOL entry branch at
`0x80003154` and the complete register helper at `0x800032B0..0x8000333C`,
then stops at `0x80003158` before the hardware helper at `0x80003400`.
**PROVEN for this bounded slice; full CRT, constructors and game boot remain
UNKNOWN or incomplete.** It is a fixture-backed research executable, not a
playable game.

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
| `apploader.img` | DVD-to-RAM DOL/FST loader | **Eliminate from runtime.** Replace with the recompiler/linker layout pass. |
| `main.dol` | Actual game executable | Primary PowerPC translation input. |
| `fst.bin` | Disc filesystem index | Parse at build time into an asset manifest or generated lookup table. |

The core rule is: **preserve semantics, not the GameCube boot machinery.**

## Files in this public directory

- `BOOT_PROCESS.md` — complete boot-chain reconstruction and what can be deleted.
- `APPLOADER_STATE_MACHINE.md` — loader callback/state-machine map.
- `DOL_STARTUP.md` — DOL/CRT initialization path and memory tables.
- `MINIMAL_BOOT_FOUNDATION.md` — precise boundary and original evidence.
- `REFINED_BOOT_GRAPH.md` — original boot dependencies and proposed native cuts.
- `OPEN_QUESTIONS.md` — prioritized barriers to a real native game boot.
- `include/shadow/boot/BootFoundation.hpp`, `src/` — C++17 section-backed
  register-startup slice and command-line probe.
- `tests/` — exact DOL SHA-256 gate, expected stop state and mutations of all
  36 helper instructions, entry words, descriptors and invalid reads.
- `experimental_native_boot/` — reference-only older authored experiments;
  its generated recompiler dependencies are not in this repository.

Build with the root CMake project. Supply
`-DSHADOW_BOOT_DOL_PATH=<path-to-PAL-main.dol>` to enable the two content-backed
CTest cases. This path stays in the ignored build cache and is never committed.

## Confidence convention

`CONFIRMED` means derived directly from the supplied bytes/instructions. `STRUCTURAL` means the behavior is clear from the instruction pattern and standard GameCube boot contract. `INFERRED` is deliberately marked where a symbol name still needs map/symbol confirmation.

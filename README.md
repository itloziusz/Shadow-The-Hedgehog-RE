# Shadow the Hedgehog — reverse engineering and native reconstruction

This project recovers the systems of *Shadow the Hedgehog* (GameCube, PAL
**GUPP8P**) from the original `main.dol` and game data. It records PowerPC
addresses, RTTI, layouts, stage parameters, and observed control flow alongside
maintainable native C++ implementations. It is an incomplete reconstruction,
not a playable port or a replacement for a legally obtained copy of the game.

## Current state

- **Gameplay:** the task scheduler, SET loading and spawning, enemy framework,
  mission counts and stage routes have native C++17 implementations. GUN Soldier
  has a detailed trace and native patrol; several other enemies have bounded
  decision loops. Player behavior, physics, weapons, vehicles, and many enemy
  families remain partial. Unrecovered world services are explicit hooks.
- **Asset reading:** the GameCube ONE/FST/container, RenderWare DFF/TXD,
  animation, skinning and native conversion code is a separate C++20 module.
  The RenderWare parser and its GameCube-specific data model live under
  `renderware/`. Other RenderWare research and a PowerPC-only motion-blur
  reconstruction are kept there and in `docs/renderware/`. The older Dolphin
  live-inspection tool is isolated under `renderware/tools/live_lab/`.
- **Native runtime:** `runtime/` is an experimental C++20 resource and streaming
  layer with null, Direct3D 12 and optional Vulkan backends. Its modern design
  choices are distinct from confirmed game behavior.
- **Boot entry:** `reverse/boot/` now builds a C++17 library and diagnostic
  executable for the exact PAL DOL entry, register helper and direct hardware
  call (`0x80003154` → `0x800032B0` → `0x80003400`). With a caller-supplied
  measured MSR, the next stack prefix reaches the HID2 accessor at
  `0x80370BA8`. Supplying measured HID2 and HID0 words issues the two
  byte-derived SPR write requests and reaches `0x80371730`, before `sync`.
  A second entry-owned native runner now completes the bounded local barrier
  consequence, GQR/FPR seed and enabled ICE/DCE checks, stopping before the
  live L2CR call at **`0x80372894`**. Three explicit pre-entry experiments
  match 5,763 state fields and 444 known stack bytes. Its immutable-code
  backend rejects DCFI/ABE/disabled-cache and other unvalidated profiles;
  the old HLE HID0 word remains a request-only input, not a retail default.
  The binary-first archaeology index links raw bytes, decoded fields, CFG and
  memory/ISA evidence. The bounded recurring event/exit loop at
  `0x800511E0..0x80051218` remains separate. Full hardware, CRT, constructor
  effects and game boot remain unresolved.
  The 74-word FPR projection also retains its six earlier standalone HLE
  experiments; it is now connected under the bounded native backend.
- **Evidence and tools:** `gameplay/` contains the address-backed subsystem
  documents, curated symbols, SET catalog and Python query tools. `reverse/`
  keeps the conservative GX FIFO prototype, boot foundation and named streaming
  reconstruction. Claims in the gameplay documents use **PROVEN**, **STRONG**,
  **LIKELY**, or **UNKNOWN**; recovered function names describe semantics,
  while original RTTI names are identified as such.

The full public-tree MSVC Release gate passed **49/49 CTest suites** against
read-only local game data (27 gameplay, one asset, one runtime, twenty boot).
`reverse/boot/PROGRESS.md` records the last
connected boot checkpoint and first fail-closed stop.
Some suites require the read-only PAL fixture. The scripted stg0100 Dark
mission reached 35/35 and routed to stage index 6. The simulator deliberately
supplies labelled engine and player hooks, so a passing route is an integration
check, not full game parity.

## Layout

| Path | Purpose |
|---|---|
| `gameplay/src/`, `gameplay/enemy/`, `gameplay/notes/` | native gameplay, family evidence and investigation notes |
| `gameplay/tools/`, `gameplay/data/` | DOL/RTTI/SET query tools and compact derived tables |
| `assets/` | GameCube asset readers and portable native conversion |
| `renderware/platform/gamecube/`, `renderware/include/` | recovered RenderWare stream/data parser |
| `renderware/reverse/`, `renderware/tools/`, `docs/renderware/` | isolated PPC reconstruction, live inspector and RenderWare evidence |
| `runtime/` | experimental platform-independent resource runtime and optional PC backends |
| `reverse/gx/`, `reverse/boot/`, `reverse/streaming/` | conservative GX research, buildable bounded boot entry, recovered resource functions; the older boot experiment is reference only |
| `docs/assets/`, `docs/reverse-engineering/` | asset-format findings and research indexes |

The modules retain their tested internal layouts. RenderWare-specific code and
research have a single top-level home; the asset reader links its parser from
there. GameCube parsing and original behavior stay at the import/reverse
boundary. The long-term target is a maintainable, native 64-bit C++ game with
platform-specific code isolated from game systems.

## Build

```sh
cmake -S . -B build -DSHADOWPC_ENABLE_DX12=OFF
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The core code builds without game files. Tests that inspect real game content
are registered only when you place your own extracted PAL GUPP8P `files/` and
`sys/` next to `gameplay/`, or provide paths through the module CMake options.
Do not commit those inputs. On Windows, `gameplay/build_and_run.ps1` runs the
full gameplay regression and stg0100 simulator; set `SHADOW_GAME_FILES_DIR`
to a read-only extracted `files/` directory when it is outside this tree.
For the boot entry tests, set `SHADOW_BOOT_DOL_PATH` to the read-only PAL
`main.dol`; CTest verifies its exact SHA-256 before executing the probe and
mutation regression.
The default probe stops at `0x80003400`; the optional measured-MSR path stops
at `0x80370BA8`; supplying measured HID2 reaches `0x8037172C` with a HID2
request, and supplying HID0 also reaches `0x80371730` with an ICFI request.
Later hardware readbacks and cache effects are unresolved.
Synthetic Dolphin startup evidence is clearly separated
from unknown retail IPL state in `reverse/boot/ARCHAEOLOGY_INDEX.md`.

## Provenance and limits

Start with [`gameplay/HANDOFF.md`](gameplay/HANDOFF.md),
[`gameplay/README.md`](gameplay/README.md) and
[`gameplay/OPEN_QUESTIONS.md`](gameplay/OPEN_QUESTIONS.md). The first two also
contain historical working-copy paths; use paths relative to this repository
for new work. `gameplay/symbols_curated.csv` is the naming source of truth;
`GAMEPLAY_SYMBOLS.csv` is its exported index. Generated caches and raw DOL
disassembly are intentionally excluded. See
[`docs/reverse-engineering/PROVENANCE.md`](docs/reverse-engineering/PROVENANCE.md)
for the source-to-public-tree mapping.

The source is for research and interoperability. It contains no disc image,
original game assets, proprietary SDK, or extracted executable.

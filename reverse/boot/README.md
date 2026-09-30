# Shadow the Hedgehog (PAL GUPP8P) — SYS boot reverse map v1

This bundle is a reusable handoff for the static recompiler. It converts the supplied `sys` files into explicit, machine-readable boot metadata so future agents do not have to rediscover the loader.

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

## Files in this handoff

- `BOOT_PROCESS.md` — complete boot-chain reconstruction and what can be deleted.
- `APPLOADER_STATE_MACHINE.md` — loader callback/state-machine map.
- `DOL_STARTUP.md` — DOL/CRT initialization path and memory tables.
- `RECOMPILER_SINGLE_EXE_PLAN.md` — design for a single host executable and safe dead-code elimination.
- `SYS_LAYOUT.json` — machine-readable authoritative map for this exact input.
- `BOOT_GRAPH.json` — machine-readable boot/dependency graph for agents and compiler passes.
- `LOW_MEMORY_BOOT_FIELDS.json` — explicit low-memory compatibility state.
- `DOL_SECTIONS.csv` — all loaded DOL sections.
- `FST_MANIFEST.csv` — all 3615 FST entries with paths, disc offsets and sizes.
- `FST_EXTENSION_COUNTS.csv` — quick asset-type census.
- `analyze_sys.py` — standalone parser for future `sys` bundles.
- `REFERENCES.md` — external format/behavior cross-checks.
- `evidence/` — disassembly evidence used for the reconstruction.

## Confidence convention

`CONFIRMED` means derived directly from the supplied bytes/instructions. `STRUCTURAL` means the behavior is clear from the instruction pattern and standard GameCube boot contract. `INFERRED` is deliberately marked where a symbol name still needs map/symbol confirmation.

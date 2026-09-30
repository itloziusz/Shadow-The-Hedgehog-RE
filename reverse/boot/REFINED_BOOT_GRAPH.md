# Refined boot dependency graph — PAL GUPP8P

This graph separates the historical execution path from the state that a native executable may still need. `CONFIRMED` means checked against the supplied bytes or listed instructions; `STRONGLY SUPPORTED` means a direct structural reading with an unmeasured branch or input; `PROVISIONAL` means a proposed replacement or an unmeasured result; `UNKNOWN` means the evidence does not settle it. The companion `refined_boot_graph.json` uses the same node and edge IDs.

## Historical order and fan-out

```text
GameCube IPL / boot service (external to this package)
  ├─ reads boot.bin disc header ──> DOL/FST disc offsets, game identity
  ├─ establishes boot environment / loads bi2.bin
  └─ loads apploader.img ──> Entry 0x81200258
                                ├─ Init  0x81200278
                                ├─ Main  0x81200298 ──> DVD requests
                                │                        ├─ DOL header/sections
                                │                        ├─ BI2/boot handoff fields
                                │                        └─ FST placement/content (conditional path)
                                └─ Close 0x812002B8 ──> entry address
             DOL loaded + FST/low-memory handoff
                           ↓
                0x80003154 DOL entry
                  ├─ 0x800032B0 register/SDA/stack initialization
                  ├─ 0x80003400 low-level initialization calls
                  ├─ 0x80003340 CRT copy/zero-table processing
                  ├─ debug, OS, constructors, callbacks, game setup
                  └─ call target 0x800510C0 (application event-loop wrapper)
                           ↓
                    game initialization (extent unresolved)
                           ↓
                    game runtime / assets
```

The supplied disassembly proves apploader callback addresses and a 13-entry switch. It does **not** establish which optional loader branches execute for this disc. `boot.bin` and `bi2.bin` are inputs to the original boot path, not sequential executable stages. FST bytes are placed before DOL entry on the documented loader path, while *SDK DVD/FST initialization and path lookups* occur later in game/OS startup. The sequence in the user request is therefore a dependency outline, not a literal one-function-after-another call chain.

## State edges and proposed cuts

| Edge | Evidence and status | Native disposition / proof gate |
|---|---|
| Header → DOL/FST offsets | `sys/boot.bin` words `0x420=0x20300`, `0x424=0x5A1A00`, `0x428=0x18B19`; **CONFIRMED** | Build-time `BootManifest`; verify bounds and hash. |
| BI2 → memory/region facts | `sys/bi2.bin` nonzero words at `+0x04`, `+0x18`, `+0x1C`, `+0x20`; **CONFIRMED** as bytes | Preserve only proven game-observed effects. IPL placement and final low-memory values remain **UNKNOWN** without trace. |
| Apploader → section/DVD requests | `evidence/apploader_full_disasm.txt` around `0x81200820..0x812010C8`; **STRONGLY SUPPORTED** | Replace with build-time section materialization after independent pre-entry comparison. No production apploader translation. |
| Apploader → low-memory block | helper `0x81200774` stores `+0x20,+0x24,+0x30,+0x34,+0x38,+0x3C`; **CONFIRMED** conditional stores | Resulting values and read liveness require a trace. Do not equate stored intermediate values with final entry state. |
| Apploader → FST placement | state `0x81200FEC` branches on an internal flag; `0x8120108C` finalizes handoff; **STRONGLY SUPPORTED** | Compile path/entry metadata ahead of time only after preserving guest-visible ordering, entry numbers, sizes and any offset identity. |
| DOL header → initialized sections | 10 nonempty ranges in `DOL_SECTIONS.csv`, ending at file size `0x5816E0`; **CONFIRMED** | Build-time `SectionMap` and immutable data. Preserve guest-address identity where observed. |
| DOL header → BSS envelope | `0x8056FE00+0x8C7EC`, ending `0x805FC5EC`; **CONFIRMED** as header data | The envelope overlaps initialized `data6`/`data7`; it is **not** a post-load blanket zero operation. Determine true pre-entry writes before lowering. |
| CRT table → zeroed ranges | `0x800055C8` lists three ranges; `evidence/main_text0_disasm.txt` offset `0x240..0x2FC`; **CONFIRMED** | `MemoryInitManifest` reproduces timing and bytes; remaining four-byte gap before `data7` is not silently zeroed by CRT lowering. |
| CRT copy table → copies | Ten entries at `0x80005544` all have source=destination; loop skips equal addresses; **CONFIRMED** | No copy work for these entries in this image; retain table facts until any address-taking/read dependence is ruled out. |
| DOL entry → application wrapper | branch at text0 offset `0x1A8` targets `0x800510C0`; recurring `0x800511E4..0x800511FC` dispatches event `0x12`, calls `0x8032D444` and tests `0x80576DBC`; **PROVEN static role** | Keep all intervening startup paths and side effects until traced. The bounded recurring phase is native C++17; no direct jump from entry to it or live-frame claim. See `research/APPLICATION_LOOP.md`. |
| FST → game assets | 3,615 entries, 3,480 files, 135 directories; **CONFIRMED** | External assets through a native index/provider. Game-visible DVD ABI behavior remains until every caller is lifted or adapted. |

## Responsibility cut points

| Layer | Original responsibility | Proposed owner | Removal condition |
|---|---|---|---|
| IPL/boot service | Set hardware state, low memory, callbacks, initial video/timing | Native entry plus explicit startup-state recipe | Guest-observable state and event order measured. |
| Apploader | Fetch BI2, DOL and FST by DVD request; publish handoff | Build-time materializer | Full pre-entry image and register/low-memory equivalence to independent original path. |
| DOL loader | Copy text/data to guest addresses | Linker/generated data initialization | Address, alignment, endianness and alias behavior proved. |
| CRT | Registers, zero table, hardware/OS setup, constructors | Split between native initialization and retained translated game behavior | Per-effect proof, including callbacks and exception/interrupt paths. |
| FST parser/DVD facade | Name, entry number, offset, size, async completion | Generated asset index and native file I/O | Exhaustive FST mapping and observed API behavior parity. |
| Game runtime | Gameplay, render, audio, input, resource ownership | Native x86-64 code and platform systems | Continues at runtime; semantic changes require separate evidence. |

## Evidence locations and limitations

- Raw immutable inputs: `sys/{boot.bin,bi2.bin,apploader.img,main.dol,fst.bin}`; identities in `INPUT_HASHES.txt`.
- Static extracts: `SYS_LAYOUT.json`, `DOL_SECTIONS.csv`, `FST_MANIFEST.csv`, and three `evidence/*disasm.txt` files. `evidence/reparse_check.json` is a second parse produced by `analyze_sys.py`, not an independent runtime oracle.
- Current native source samples: `GekkoForge/runtime/src/gekko/system.cpp` (`load_dol`, `apply_boot_configuration`), `GekkoForge/runtime/src/shadow_main.cpp` (embedded raw SYS inputs), and `GekkoForge/CMakeLists.txt` (generated PPC shards/resources). These describe the **current implementation**, not necessarily the original boot result.
- No original-console or pinned Dolphin pre-entry memory snapshot, full low-memory read census, branch trace for the supplied apploader, constructor inventory, or complete indirect-target proof is contained in this package.

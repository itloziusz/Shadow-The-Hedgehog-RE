# Provenance of the initial public tree

Target: *Shadow the Hedgehog*, GameCube PAL **GUPP8P**. Addresses in the
gameplay and renderer notes refer to that original DOL unless a document says
otherwise. Function names in the curated symbol map are recovered semantic
names; RTTI labels are original binary strings. Confidence labels indicate the
strength of the stated evidence, not completeness of a native implementation.

| Public location | Source investigation | Status |
|---|---|---|
| `gameplay/` | gameplay DOL recovery, SET catalog and native C++17 simulator | bounded modules; full world runtime incomplete |
| `assets/` | GameCube asset loader and native conversion work | parser and conversion code, C++20 |
| `renderware/platform/gamecube/` | asset loader's RenderWare DFF/TXD parser | recovered format behavior; not the full RenderWare SDK |
| `renderware/reverse/motion_blur/` | renderer motion-blur investigation | static 32-bit PPC reconstruction; not linked to host build |
| `runtime/` | native PC resource/streaming experiment | modern target architecture, not claimed original |
| `reverse/gx/` | GX/PPC FIFO research prototype | conservative decoding rules, not original game code |
| `reverse/boot/` | authored startup investigation | isolated native boot foundation and evidence; not linked into the game runtime |
| `reverse/boot/experimental_native_boot/` | older authored startup experiment | reference-only C++; generated recompiler corpus excluded, so not a build target |
| `renderware/tools/live_lab/` | v0.8 live RenderWare inspector | research tool and address catalogs, not game runtime code |
| `reverse/streaming/` | named resource-streaming PPC-to-C++ work | partial functions with explicit ABI obligations |

The tree excludes `main.dol`, extracted `files/`, proprietary RenderWare SDKs,
raw 44 MB assembly dumps, analysis pickle caches, local logs, and generated
binaries. Compact symbol and SET tables remain because they carry address
provenance needed to reproduce or extend the research.

The historical gameplay handoffs were written for a local checkout and retain
some task-specific guidance. New work should use repository-relative paths and
the current root README. An unknown engine service must remain a labelled hook
until evidence and an independent regression test support its implementation.

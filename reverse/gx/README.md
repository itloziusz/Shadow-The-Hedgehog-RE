# GameCube GX / PPC -> C Research Pack

Purpose: a conservative foundation for recovering GameCube GX intent from Gekko PowerPC code and GX FIFO/display-list data.

This pack deliberately separates **observation** from **lifting**:

1. PPC recovery: disassemble/decode Gekko and identify writes to the GX write-gather pipe (`0xCC008000`) plus calls into known GX SDK functions.
2. FIFO IR: convert those writes into width-accurate symbolic FIFO events.
3. Packetization: decode CP/BP/XF/display-list/primitive packets while maintaining CP state.
4. Vertex recovery: use VCD + VAT to determine exact vertex payload layout.
5. Semantic lifting: emit `GX_*`/C calls only when the observed byte stream/state proves the call semantics. Otherwise preserve a raw event.

## Included prototypes

- `cpp/gx_fifo_decoder.cpp` — C++20 stateful GX FIFO/display-list decoder. Tracks CP VCD/VAT and decodes direct/indexed vertex streams into C-like GX calls where the encoding is unambiguous.
- `go/ppc_wgpipe_scan.go` — Go raw big-endian PPC scanner for conservative WGPIPE store recovery. It recognizes common address-construction idioms and reports exact writes to `0xCC008000`.
- `python/make_sample_fifo.py` — builds a known-good synthetic FIFO stream for cross-language tests.
- `tests/run_tests.sh` — compiles/runs C++ and Go probes and checks their output.
- `docs/SOURCES.md` — curated GX/Gekko/ABI/recompiler source map.
- `docs/ARCHITECTURE.md` — proposed production decompiler architecture.
- `docs/RECOVERY_RULES.md` — fail-closed rules for PPC -> GX lifting.
- `docs/GX_PACKET_CHEATSHEET.md` — command/VCD/VAT quick reference.
- `docs/PPC_ABI_AND_PATTERNS.md` — EABI/SDA/paired-single/WGPIPE recovery notes.
- `docs/TEXTURE_TEV_NOTES.md` — texture formats and TEV state-recovery rules.
- `data/gx_command_map.json` — machine-readable command/register seed data.

## Build and test

```bash
cd gx_ppc_to_c_research_pack
bash tests/run_tests.sh
```

The prototype uses only standard-library C++/Go/Python, so it has no network dependency.

## Key design rule

Never infer a high-level GX call merely because a store sequence resembles one. The lift is accepted only if the current GX state (especially VCD/VAT) makes the widths, ordering, and payload meaning consistent. Unknown state remains unknown.

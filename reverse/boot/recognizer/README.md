# Binary-first PAL boot recognizer

This scriptable tool reads the original PAL GUPP8P DOL as a **read-only** input
and checks its SHA-256 before analysis. It is an evidence index and hypothesis
generator for the connected boot path. It is not a PPC interpreter, emulator,
or source-code generator.

From the repository root, with Python 3.10+ and an existing Release build:

```powershell
python -B reverse/boot/recognizer/cli.py `
  --dol H:\ShadowTheHedgehogAssetLoaderRE\sys\main.dol `
  --probe-exe build\reverse\boot\Release\shadow_boot_probe.exe `
  --observed-msr 00002032 --observed-hid2 E0000000 `
  --observed-hid0 0011C464 batch
```

The three supplied words are synthetic HLE observations, not retail hardware
defaults. Omit `--probe-exe` and the observed values for a static-only pass.
`--db` and `--report` default to ignored files under `build/`. Run `batch`
again after evidence changes; the learned structural patterns and matches are
recomputed from current records. `inspect ctor_16` or
`inspect frontier_sync_gqr` emits a detailed JSON report. `scan`, `frontier`,
`rescan`, `bootstrap`, and `report-db` support narrower automated workflows.

Pipeline:

1. `machine.py` maps original DOL bytes, retains word and file offset, reuses
   the project's Gekko decoder, and emits precise and coarse normalized forms.
2. Its bounded CFG and symbolic transfer track GPRs, CR fields, LR/CTR,
   XER/MSR/FPSCR, FPR lanes, SDA and stack address expressions, calls, and
   ordered memory/SPR effects. Unsupported words or semantics poison abstract
   state. A call without a traced callee poisons its return state.
3. `fingerprints.py` records opcode and normalized n-grams, CFG shape,
   access/effect signatures, and a disclosed weighted structural score.
   Detector matches are hypotheses with contradictions and validation needs.
4. `store.py` persists regions, evidence, statuses, matches, learned patterns,
   and the **connected** native frontier in SQLite. Reusable n-grams require
   at least two strongly supported or validated examples. They never promote
   an unknown candidate automatically.
5. `cli.py` scans bounded constructor-table targets and the current connected
   stop, clusters unknown candidates by complete linkage, reruns the native
   probe when requested, and writes machine-readable JSON.

The constructor-table words and first-return extents are static heuristics.
The table is writable at runtime; a static pointer is not proof that its
target executes. A structural similarity score is **not** confidence or
runtime parity. `promote` requires explicit identification, falsification,
reference behavior, chain validation, addresses, and oracle provenance;
the submitted evidence still requires human review. No CLI command advances
the boot frontier automatically. `VALIDATED` seeds have the bounded meaning
stated in `seeds.json`; the full connected boot remains incomplete.

For an unresolved indirect transfer, barrier, or hardware consequence, the
report retains symbolic state and suggests the smallest reference capture it
needs. Full dynamic execution, Gekko cache behavior, paired-single PS1,
retail IPL state, and later constructors are not inferred from a match.

Regression:

```powershell
python -B reverse/boot/tests/test_recognizer.py H:\ShadowTheHedgehogAssetLoaderRE\sys\main.dol
ctest --test-dir build -C Release --output-on-failure
```

The first command includes raw PAL bytes plus synthetic mutation and
fail-closed tests. CMake registers it as `boot_pal_semantic_recognizer` when
the PAL DOL fixture is configured.

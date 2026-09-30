# Gameplay RE toolkit (Shadow the Hedgehog GC, `sys/main.dol`)

Pure Python 3 (no Ghidra needed). Run from `gameplay/tools/`.

## Pipeline (re-run in this order after changing a tool)

| Step | Command | Output |
|---|---|---|
| 1 | `python program.py` | `data/program.pkl` — 34.5k functions, call graph, jump tables, data→code pointers |
| 2 | `python dataflow.py` | `data/dataflow.pkl` — per-function symbolic dataflow records |
| 3 | `python rtti.py` | `data/classes.json` — MWCC RTTI classes, bases, vtables, ctor/dtor evidence |
| 4 | `python setobj_catalog.py` | `data/setobj_catalog.{json,txt}` — SET object types + param schema |
| (auto) | first `q.py xref/callers` | `data/xref_index.pkl` |

## Data extractors / generators (run after the pipeline)

| Command | Output | Purpose |
|---|---|---|
| `python classify.py` | `data/func_families.json`, `data/family_runs.txt` | subsystem family per function |
| `python setparse.py [stage kind]` | stdout | parse/validate SET layout files against the DOL schema; dump one stage |
| `python enemy_index.py` | `data/enemy_index.json` | enemy/boss types: create hook, sizeof, class, AI states, resources |
| `python command_catalog.py` | `data/commands.csv` | CharCommand type ids per command class |
| `python scoredata.py` | `data/score_table.csv` | `files/common/ScoreData.bin` (115 score/karma entries) |
| `python gen_catalog_cpp.py` | `src/SetObjCatalog.generated.cpp` | SET object catalog as C++ |
| `python gen_stage_table_cpp.py` | `src/StageTable.generated.cpp` | stage table 0x804C5AE8 as C++ |
| `python merge_symbols.py ../notes/X_symbols.csv` | `../symbols_curated.csv` | merge an investigation symbol fragment (curated wins, conflicts reported) |
| `python export_symbols.py` | `GAMEPLAY_SYMBOLS.csv` | curated (`../symbols_curated.csv`) + RTTI-derived names |
| `agent_mission_*.py`, `agent_enemy_*.py`, `agent_player_*.py` | stdout | helpers written by the investigations (see notes/) |

Curated names live in `gameplay/symbols_curated.csv` (source of truth, read by `symbols.py`); re-run
`export_symbols.py` after editing it.

## Modules

- `dol.py` — DOL loader (`get_dol()`, `.u32/.f32/.cstr/.region(addr)`). r2 = 0x805FA780, r13 = 0x805EC500.
- `ppc.py` — own Gekko decoder (Capstone 5 PS-mode mis-decodes `fcmpo`). `python ppc.py ADDR N` = raw disasm.
- `program.py` — function discovery (evidence per start: bl/extab/dataptr/codeptr/gap), `func_of(p, addr)`.
- `dataflow.py` — forward value propagation. Values: `('k',c)` const, `('a',n)` entry arg rN,
  `('l',base,disp,size)` loaded value, `('+',base,off)`, `('sp',off)`, `('ret',callee)`, `('fk',addr)` float const.
  Records: `acc` (pc, base, disp, size, is_store, is_fp, stored_value), `call` (pc, target, args r3..r10,f1..f4),
  `icall` (pc, ctr_value, args), `cptr` (pc, materialised address).
- `symbols.py` — names: `GAMEPLAY_SYMBOLS.csv` (curated, wins) > RTTI-derived (`Class::vfNN`, `Class::Class`,
  `Class::~Class`, `thunk-N_target`). **Class names are original (RTTI); method names are not.**
- `q.py` — query CLI:
  - `q.py dis ADDR|NAME [n]` annotated disassembly (globals, strings, floats, vtables, arg-relative fields)
  - `q.py callers X` / `q.py callees X` / `q.py xref ADDR`
  - `q.py class NAME|regex` — bases (with offsets), vtables, slots, vptr writers
  - `q.py fields X [reg]` — struct offsets accessed via entry register (default r3 = this)
  - `q.py off HEX [R|W]` — every function touching displacement HEX on a non-constant base
  - `q.py str REGEX` — strings + referencing functions
  - `q.py vcalls X` — indirect calls with symbolic vtable path

## Virtual call resolution (MWCC)

vptr points at `&vtable.typeinfo`; slot at `vptr + 8 + 4*i`. For multiply-inherited objects the ctor writes
`vtable_group + k` into several vptr fields — resolve `[[obj+X]+off]` as `u32(vptr_value_at_X + off)`.
Derived classes append new virtuals *after* the last secondary vtable of their base group, so a slot beyond
a secondary table's natural end can belong to the primary chain (takes complete `this`). Thunks are
`addi r3,r3,-N ; b target` and are named `thunk-N_target`.

## Conventions

Confidence: PROVEN (direct code/data evidence), STRONG (multiple consistent evidence), LIKELY (single
indirect evidence), UNKNOWN. Never promote LIKELY to PROVEN without new evidence.

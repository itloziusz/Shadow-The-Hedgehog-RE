# Recognizer batch 2 — repeated multiword table propagation

Input: PAL GUPP8P `main.dol`, SHA-256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
The scan covered the 266 static constructor-table indices 16–281. Together
with six prior evidence seeds and the connected frontier, the local database
contains 273 regions: 2 bounded `VALIDATED` seeds, 4 static
`STRONGLY_SUPPORTED` slices and 267 `UNKNOWN` candidates. These statuses
describe evidence for each region, not boot completion. The generated SQLite
database and full JSON report stay under ignored `build/`.

## Raw-byte finding

Eight initial constructor-table words at `0x804AAC60+4×index` point to
different code addresses. Their 58-word bodies are byte-distinct (eight raw
SHA-256 values) but have **one identical precise normalized instruction
sequence**. They are branchless bounded windows ending in `blr`, with `stmw`
and `lmw` around repeated loads and stores. The recognizer's complete-linkage
structural score is 1.0000 across all eight; that score is not semantic
confidence.

| Table index | Code range (half-open) | First concrete destination store |
|---:|---|---|
| 135 | `0x8020E1E4..0x8020E2CC` | `0x80545424` |
| 141 | `0x802128C8..0x802129B0` | `0x805459CC` |
| 143 | `0x80213D60..0x80213E48` | `0x80545BEC` |
| 145 | `0x80215058..0x80215140` | `0x80545E0C` |
| 147 | `0x802163F0..0x802164D8` | `0x8054602C` |
| 149 | `0x80217788..0x80217870` | `0x8054624C` |
| 151 | `0x80218A80..0x80218B68` | `0x8054646C` |
| 155 | `0x8021AFD8..0x8021B0C0` | `0x80546864` |

**PROVEN-BINARY:** in each body, three direct loads address
`0x80514CB8`, `0x80514CBC` and `0x80514CC0`. The DOL initializes these
12 source bytes to zero. The symbolic transfer derives 24 distinct concrete
destination addresses per body and records ordered register-sourced memory
effects. It does not resolve live memory aliasing. The first body is
illustrated by raw instructions:

| VA | Raw word | Decoded effect |
|---|---|---|
| `0x8020E1E4` | `9421FFD0` | `stwu r1,-0x30(r1)` |
| `0x8020E1F0` | `BF61001C` | `stmw r27,0x1C(r1)` (five ordered BE32 stack writes) |
| `0x8020E1F4` | `3B645400` | `addi r27,r4,0x5400` after `lis r4,0x8054`; base `0x80545400` |
| `0x8020E234` | `83EB0000` | `lwz r31,0(r11)` from `0x80514CB8` |
| `0x8020E278` | `93E10008` | `stw r31,8(r1)` |
| `0x8020E288` | `93FB006C` | `stw r31,0x6C(r27)` to `0x8054546C` |
| `0x8020E2B8` | `BB61001C` | `lmw r27,0x1C(r1)` (five ordered BE32 stack reads) |
| `0x8020E2C8` | `4E800020` | `blr` |

The initial DOL destination bytes contain pointers and sentinel words, so
the body is operating on an initialized table-like region. The data layout,
owner, and meaning of its fields remain **UNKNOWN**. In particular, the
source bytes can change in live RAM before these constructor indices execute.
The writable constructor table can also change. Neither the initial zeros
nor the static table pointers are sufficient to claim runtime zeroing or
actual execution of these bodies.

The broader `bulk_table_propagation` detector matched nine candidates. Its
ninth match, index 36 at `0x8006D164..0x8006D274`, has 68 instructions and
30 concrete destinations, rather than 58 and 24. It is a related structural
motif, **not** a member of the exact eight-body family.

## Root-cause correction and validation ladder

The first full scan exposed a recognizer defect: `stmw` and `lmw` were
decoded but treated as unmodeled, poisoning the subsequent address state.
`machine.py` now expands each into ordered four-byte effects for every
register from rD through r31. A second adversarial check found that sorting
effects lexically would put r10 before r9; the report now preserves original
within-instruction order. Tests include a 24-register synthetic `stmw`,
base-overlap decline for `lmw`, mutation of the first body's `stmw`, all eight
raw table words, eight distinct raw hashes, one normalized hash, the shared
source addresses, and 24 destination addresses per body.

1. **Identification:** structural family established from raw words,
   normalized sequence and explicit data-flow effects.
2. **Falsification:** different raw bodies and the distinct index-36 layout
   were checked; a mutated multiword instruction breaks the detector.
3. **Behavioral validation:** **UNKNOWN**; no same-run entry/return memory
   delta or live constructor-table capture exists.
4. **Boot-chain validation:** **UNKNOWN**; connected native execution still
   stops before `sync` at `0x80371730`, far before constructor dispatch.

The next minimal experiment is to break at the constructor-table walk,
capture the live table word for one of these indices and the 12 source bytes,
then record all destination fields immediately before and after that body.
Only after that can the family be promoted or represented in native C++.

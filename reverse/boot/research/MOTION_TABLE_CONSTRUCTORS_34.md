# Eight NPC MotionImpl table constructors — exact copy graph

Target: PAL GUPP8P `main.dol`, SHA-256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
This extends `SEMANTIC_RECOGNIZER_33.md` from structural clustering to a
bounded global-memory transformation. The static constructor walker has not
been reached by connected native execution. The table and memory values at
runtime remain conditional inputs.

## Binary identity and owner evidence

Each 58-word constructor body has a statically initialized pointer in the
282-entry table at `0x804AAC60+4×index`. Its target is byte-distinct but
precisely normalized-identical to the other seven. The base below is the
address used by the body before its `+0x0C` entry array. At `base+0xE4`,
the DOL has a vtable whose typeinfo name is the original RTTI class shown.
Five code words in the entry array point into the matching `MotionImpl`
code band, between the class's first recovered method and the constructor
body. The nearby destructor writes that same vtable address to its object
vptr; for Knuckles, `0x8020E180/184` writes `0x805454E4` to `this+0x18`.

| Index | Body start | Base | Vtable / original RTTI class |
|---:|---|---|---|
| 135 | `0x8020E1E4` | `0x80545400` | `0x805454E4` — `Player::Npc::Knuckles::MotionImpl` |
| 141 | `0x802128C8` | `0x805459A8` | `0x80545A8C` — `Player::Npc::Maria::MotionImpl` |
| 143 | `0x80213D60` | `0x80545BC8` | `0x80545CAC` — `Player::Npc::Tails::MotionImpl` |
| 145 | `0x80215058` | `0x80545DE8` | `0x80545ECC` — `Player::Npc::Omega::MotionImpl` |
| 147 | `0x802163F0` | `0x80546008` | `0x805460EC` — `Player::Npc::Espio::MotionImpl` |
| 149 | `0x80217788` | `0x80546228` | `0x8054630C` — `Player::Npc::Vector::MotionImpl` |
| 151 | `0x80218A80` | `0x80546448` | `0x8054652C` — `Player::Npc::Rouge::MotionImpl` |
| 155 | `0x8021AFD8` | `0x80546840` | `0x80546924` — `Player::Npc::Amy::MotionImpl` |

**PROVEN-BINARY:** the class strings, vtable/typeinfo pointer chains, five
initial code pointers per body, table words, instruction words and addresses
above are read directly from the DOL. **STRONG ownership inference:** the
13-entry data area immediately precedes the matching RTTI/vtable group and
the body writes only its own base. The exact source-level type is still
UNKNOWN. Five initial entries look like three-word callable descriptors
`{0, 0xFFFFFFFF, code_pointer}`; their invocation ABI has not been traced,
so calling them C++ member-function pointers would be premature.

## Complete global-memory effect

Starting at `base+0x0C`, there are 13 consecutive three-word entries, each
12 bytes. Let `E[i]` denote the **live** entry value before this body runs,
and let `Z` denote the live three words at `0x80514CB8..0x80514CC0`.
Every one of the eight bodies performs these complete entry copies:

| Destination | Source | Raw Knuckles destination words |
|---:|---:|---|
| `E[2]` | `E[0]` | `0x80545424..2C` |
| `E[3]` | `E[1]` | `0x80545430..38` |
| `E[5]` | `E[4]` | `0x80545448..50` |
| `E[6]` | `Z` | `0x80545454..5C` |
| `E[8]` | `Z` | `0x8054546C..74` |
| `E[9]` | `E[7]` | `0x80545478..80` |
| `E[11]` | `Z` | `0x80545490..98` |
| `E[12]` | `E[10]` | `0x8054549C..A4` |

All 24 destination words are distinct. All concrete source addresses are
distinct from destinations, and a complete ordered scan shows no global
load after an earlier global write to the same address. Thus the eight
entry assignments are equivalent to the original global writes for any
live input words, on normal return, without substituting DOL initial values.
The bodies also save and restore r27–r31 through their own stack frames and
write three temporary stack words. The native projection covers the global
entry copies only; it is not a full machine-state or exception projection.

The DOL initializes `Z` to twelve zero bytes and preinitializes `E[0]`,
`E[1]`, `E[4]`, `E[7]`, `E[10]` with `{0,0xFFFFFFFF,text_target}`. Those are
**static initial values**, not proof of values at constructor dispatch.
Earlier code may modify either region; the table itself is writable RAM.

## Validation and portable representation

`tests/test_recognizer.py` checks all eight raw table pointers, RTTI strings,
vtable address relation, five text-pointer bands, every one of the 24
destination←source word edges, absence of global read-after-write aliases,
and rejection of an instruction mutation. The native C++17
`ApplyMotionTableCopies()` uses 13 opaque three-word entries and a caller
supplied `Z`; its dedicated test varies all input entries and `Z`. It is a
**bounded semantic projection** and is deliberately not called by the
connected boot probe. No default zero or pointer is baked into it.

The recognizer's two static seeds from this family yielded six held-out
complete-family matches (110/110 common n-grams, equal normalized sequences
and effect counts). Self-matches are excluded. These are candidate family
matches, not new validated boot behavior; all six retain `UNKNOWN` status.

**Pass 1 — raw/structural:** passes for all eight bodies. **Pass 2 — data
flow:** exact global copy graph and disjoint addresses pass. **Pass 3 —
behavioral:** live before/after memory and invocation remain UNKNOWN.
**Pass 4 — chain:** constructor walker is not reached by native boot; the
first connected stop remains before `sync` at `0x80371730`.

Next capture: live constructor-table words, `Z`, the 13 entries and the
matching vtable pointer before and after one body. Separately trace a
consumer of the three-word entries to establish their call convention and
gameplay meaning.

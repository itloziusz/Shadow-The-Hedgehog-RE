# PAL GUPP8P boot: CRT to application call

This is a static audit of the exact `sys/main.dol` whose SHA-256 is
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
It extends the buildable foundation's stop at `0x80003158` as a dependency
map. It does **not** assert that these paths executed in an original boot or
that the archived `experimental_native_boot` code is a runnable replacement.

## Direct entry order

**PROVEN (DOL instructions):** `__start` at `0x80003154` calls the register
helper at `0x800032B0`, then `0x80003158` calls `0x80003400`. That helper
executes `mfmsr; ori 0x2000; mtmsr`, then calls `0x80371714`, `0x80370CDC`,
and `0x80372838` in that order before returning to `0x8000315C`. Entry
pushes two `-1` stack words and calls the CRT table routine at `0x80003340`
from `0x8000316C`. Only after its return does it begin the low-memory and
debug path at `0x80003170`. The next fixed calls in `__start` are
`0x80370BF0` at `0x80003260`, `0x80370E68` at `0x80003264`,
`0x803796AC` at `0x8000329C`, and `0x800510C0` at `0x800032A8`.
The last call is followed by a branch to `0x803A3B04` at `0x800032AC`.
The path to each later call depends on the earlier callees returning.

**PROVEN (instruction effects):** `0x80371714` reads and ORs HID2 with
`0xA0000000` through `0x80370BA8` / `0x80370BB0`, invokes HID0/cache
setup at `0x803725F4`, and clears GQR0..GQR7 at `0x80371738..0x80371754`.
`0x80370CDC` takes its paired-single path after the HID2 OR, loads
`0x805F1F38` at `0x80370CFC`, then loads a double from `0x805F1F30`
at `0x80370D7C` and copies it through FPR0..31. It writes FPSCR from FPR0
at `0x80370DFC`. `0x80372838` branches on cache state and installs a
machine-check handler via `0x80373378` at `0x80372904`.

**UNKNOWN (runtime effect):** the pre-entry contents of `0x805F1F30..3F`,
incoming MSR/HID/cache values, and resulting floating-point status have no
original-path snapshot in this repository. The two FPR source addresses are
outside every DOL-loaded section. They fall within the **later** CRT zero
range `0x805EF020..0x805F277C`, so post-CRT zero does not establish the
bytes observed earlier by `0x80370CDC`. The archived experiment accepts an
external oracle for these values; its comments are not an independent oracle.
No native continuation should synthesize zeros for the pre-CRT reads.

## CRT table contract

**PROVEN:** the loop at `0x8000336C..0x800033A8` reads triples from
`0x80005544`, calls `memmove`/cache maintenance only when the source and
destination differ (`0x80003384..0x800033A0`), and stops on zero size.
Independent big-endian parsing of text0 found ten entries, all with
`source == destination`, followed by `(0,0,0)` at `0x800055BC`.
Thus this exact table triggers no `memmove` call on the direct loop path.

**PROVEN:** `0x800033C0..0x800033E0` reads zero-range pairs from
`0x800055C8` and calls `memset(dst,0,size)` for each nonzero size.
The three ranges are:

| Start | Size | Exclusive end |
|---:|---:|---:|
| `0x8056FE00` | `0x74700` | `0x805E4500` |
| `0x805EF020` | `0x375C` | `0x805F277C` |
| `0x805FC540` | `0xAC` | `0x805FC5EC` |

The pair `(0,0)` at `0x800055E0` terminates the table. The total cleared
length is `0x77F08` bytes. `0x805F277C..0x805F2780` is outside these
zero ranges and before initialized data7; no blanket clear of that gap is
justified. The DOL header BSS envelope `0x8056FE00+0x8C7EC` overlaps
loaded data6/data7 and cannot replace the executed table semantics.

**STRONG (bounded native candidate):** a table-driven CRT projection can be
added after the `0x80003400` frontier with three checked zero intervals and
ten identity copy descriptors. It must retain effect timing: the FPR source
is consumed first, then zeroed. Unit tests should mutate all 13 descriptors,
terminators, and the copy/zero loop branch words, and should show that an
unknown pre-CRT FPR image is declined rather than defaulted. This is a
bounded component, not whole hardware or boot parity.

## Low-memory branch inputs after CRT

**PROVEN (reads and writes in `__start`):** `0x8000317C` writes zero to
`0x80000044`; `0x80003188` reads BI2 pointer `0x800000F4`. A nonzero
pointer supplies debug field `BI2+0x0C` at `0x80003194`. On the zero-pointer
path, `0x800031A4` reads `0x80000034`, and `0x800031B8` reads
`0x800030E8`. Values 2/3 route through `0x8039F8E0`
(`0x800031BC..0x800031F4`); value 4 sets byte `0x805F1FF0` by
`0x80003140`. At `0x80003200`, entry rereads `0x800000F4`; a nonzero
argument offset at `BI2+0x08` invokes the relocation loop
`0x8000320C..0x80003254` and writes `0x80000034`.

**PROVEN:** the OS path at `0x80370E68` reads `0x800000F4`, arena words
`0x80000030` and `0x80000034` (`0x80370F08`, `0x80370F74`,
`0x80370FD0`), then fans out into many OS, interrupt, cache, and device
initializers from `0x80370FEC..0x80371028`. Back at `__start`, the
halfword `0x800030E6` controls the branch through `0x80003100`
(`0x80003268..0x80003288`); the latter reads `0x800030E4` at
`0x80003110`. The byte at `0x805F1FF0` controls optional `0x8039F978`
at `0x8000328C..0x80003298`. These are observable branch inputs and
cannot be replaced by a single assumed retail path without an entry-state
trace. Archive code in `experimental_native_boot/src/runtime_route.cpp`
models one path and explicitly does not execute the OS body.

**UNKNOWN:** the final values of the listed low-memory words and the
selected conditional path for this disc at original entry. Apploader
disassembly identifies candidate writes (`0x81200774` and later states),
but this audit found no independent pre-entry snapshot tying the chosen
apploader path to all consumed words. The OS call at `0x80370E68` also
contains return-value, timing, device, and interrupt dependencies not
closed by the DOL image.

## Constructors and application boundary

**PROVEN:** `0x803796AC` calls walker `0x803796CC`, which begins at
`.ctors` `0x804AAC60`, reads a function pointer at `0x80379700`, calls
it indirectly at `0x803796F4..0x803796F8`, advances by four bytes and
continues until a null pointer. Independent parsing of the exact DOL found
282 nonzero, unique, four-byte-aligned entries; all fall inside its text
sections. The first is `0x803A2520`, the last `0x804216F4`, and the zero
terminator is at `0x804AB0C8`. This proves the table, not execution or
correct side effects of all 282 targets.

**PROVEN:** the direct target of `0x800032A8` is `0x800510C0`. Its body
begins by enabling callback state through `0x8037DF9C`, dispatches event
codes via `0x80051978`, calls `0x80486EC0` / `0x80486E34`, then loops at
`0x800511E4..0x800511FC` until word `0x80576DBC` changes. Event dispatch
at `0x80051AA8..0x80051AF8` initializes a structure bearing the string
`RenderWare Application` and calls `0x80051028`, which calls OS, platform,
and render initialization functions. **STRONG:** `0x800510C0` is the
application entry wrapper. The static event order is mapped further in
`APPLICATION_LOOP.md`. **UNKNOWN:** original event outcomes,
window/render readiness, and the first gameplay transition without an
original trace and recovered callees.

## Required original-path evidence for full boot parity

1. Capture the unmodified original at DOL entry `0x80003154`: registers,
   MSR/HID/FPSCR, bytes `0x805F1F30..3F`, low-memory words
   `0x80000030/+34/+44/+F4`, `0x800030E4/+E6/+E8`, BI2 pointer target,
   loaded section/BSS/FST ranges. Record game ID and input hashes.
2. Trace the selected branches and ordered externally visible writes from
   `0x80003400`, `0x80003340`, `0x80370E68`, and each of the 282 constructor
   calls, including callbacks and indirect targets. Compare boundaries
   before native promotion; do not infer full parity from static DOL data.
3. Continue the trace through `0x800510C0` to a stable first-frame/event
   boundary, then prove native platform, RenderWare, asset, and gameplay
   effects independently. A process that merely reaches `0x800510C0`
   is an application-entry milestone, not a bootable game.

## Independent checks performed

- Queried the original gameplay `q.py dis` cache for `0x80003154`,
  `0x80003400`, `0x80003340`, hardware callees, `0x80370E68`,
  `0x803796CC`, `0x800510C0`, and its event dispatch. No cache rebuild.
- Independently parsed big-endian copy, zero and `.ctors` tables from the
  read-only original `sys/main.dol` bytes; recomputed its SHA-256. No game
  input or toolchain file was changed.
- Cross-checked the resulting addresses with `DOL_STARTUP.md`,
  `REFINED_BOOT_GRAPH.md` and the reference-only archived source. No
  runtime parity test is claimed because the required original-path
  snapshot is absent.

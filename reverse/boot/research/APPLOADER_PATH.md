# PAL GUPP8P apploader path and DOL handoff

2026-09-30. The scope is the supplied `sys/{boot.bin,bi2.bin,apploader.img,
main.dol,fst.bin}` and the apploader's PPC at runtime `0x81200000+offset`.
The first apploader code byte is file offset `0x20`. **PROVEN** denotes raw
bytes/instructions and their deterministic consequences on a stated path;
**UNKNOWN** denotes an unobserved IPL/hardware input or executed branch.
See `PREENTRY_STATE.md` for the full input hashes and oracle capture plan.

## State dispatch and callback result

**PROVEN:** callback entry `0x81200258` writes Init/Main/Close pointers
`0x81200278/0x81200298/0x812002B8` (`0x812003B8..0x812003E4`). Main
dispatches 13 states through the table at `0x812018B4`; direct raw-file
reading gives targets `0,1→0x81200820`, `2→0x81200850`,
`3→0x812008C8`, `4→0x81200ADC`, `5→0x81200C00`,
`6→0x81200C30`, `7→0x81200DC8`, `8→0x81200EE0`,
`9→0x81200FEC`, `10→0x8120108C`, `11→0x8120109C`,
`12→0x812010C8`. Close calls `0x81201138`, which reads the entry word at
`0x81201940+0xE0` (`0x81201A20`; DOL header `+0xE0`). On the ordinary
`main.dol` path that word is `0x80003154`. An alternate DOL-header source
is possible and is not silently mapped to this entry.

## AplMain request sequence: fixed bytes versus conditional order

| State | Address-backed fact | Open condition |
|---|---|---|
| 0/1 | **PROVEN** `0x81200820..0x8120083C` requests `0x20` bytes at disc `0x420`, to loader `0x81201920`. It includes DOL disc offset `0x20300`, FST disc offset `0x5A1A00`, size/max `0x18B19` and header word `0x803E74E0`. | Which of 0/1 was first entered is **UNKNOWN**; both issue the same request. |
| 2 | **PROVEN** `0x81200898..0x812008B4` requests `0x20` bytes at disc `0x440` to loader `0x81201A80`. | No disc-data ambiguity. |
| 3 | **PROVEN** `0x812008C8..0x81200ACC` publishes low-memory fields, computes FST/BI2 placement, then requests full BI2 `0x2000` bytes from disc `0x440` to `[0x800000F4]`. | Its computed destination depends on pre-existing `[0x80000028]`; see equation below. |
| 4 | **PROVEN** `0x81200ADC..0x81200B70` chooses a DOL-header source/offset and computes whether FST is read before or after DOL. If DOL offset is below FST disc offset, flag `+0x194=1` and state 5 follows; otherwise state 4 issues an early FST read, then state 5. | Internal `+0x14C..+0x164` derives from a pointer stored at `[0x800030F0]` (`0x81201224..0x81201254`). That pointer and the selected branch are **UNKNOWN**. |
| 5/6 | **PROVEN** `0x81200C00..0x81200C2C` requests `0x100` DOL header bytes from chosen offset. State 6 uses the returned header (`0x81200C30..0x81200DB4`), then enters section loading. | On the ordinary offset `0x20300`, the header is the supplied `main.dol`; alternative source remains **UNKNOWN**. |
| 7/8 | **PROVEN** `0x81200DC8..0x81200FEC` iterates 7 text and 11 data slots, skipping empty descriptors and rounding DVD lengths to 32-byte lines. State 12 chunks requests over `0x20000`. | For the supplied DOL there are 2 text and 8 data sections; all ten sizes are already 32-byte multiples. |
| 9 | **PROVEN** `0x81200FEC..0x81201084` requests FST to loader `+0x30` with 32-byte-rounded `+0x28` length from disc `+0x24` if flag `+0x194=1`; otherwise it proceeds to finalization because FST was requested in state 4. | The order depends on state-4 source/offset; final placement still depends on `[0x80000028]`. |
| 10/11 | **PROVEN** state 10 calls handoff writer `0x81200774`; state 11 may query hardware for a halfword at `0x800030E4`, then Main returns zero. | Query only when loader `+0x14C==0`; returned halfword is **UNKNOWN** (`0x8120109C..0x812010C0`). |

The ordinary ordering is **conditional**: with no alternate DOL offset from
the IPL block, `0x20300 < 0x5A1A00` makes `+0x194=1`, so the requests are
boot-header probe, BI2 probe, full BI2, DOL header, ten DOL sections (possibly
chunked), then FST. The code proves this implication; no original callback
trace here proves the branch premise.

## Exact symbolic FST and low-memory result

**PROVEN:** state 3 reads initial word `M=[0x80000028]` at `0x812008DC`,
while the exact BI2 probe supplies `E=BI2[+0]=0` and
`S=BI2[+4]=0x01800000`. The apploader writes

```text
[0x800000E8] = E = 0
[0x800000EC] = align_down_32(0x80000000 + M - E)
[0x800000F0] = S              (S != 0 for this BI2)
FST_base      = align_down_32(0x80000000 + min(M, S) - 0x18B19)
[0x800000F4] = FST_base - 0x2000
```

The `min` expression is the nonfatal branch synthesis of
`0x81200990..0x81200A94`: for `S < M`, `0x81200A20..0x81200A38`
subtracts FST max from `0x80000000+S`; for `S >= M`,
`0x81200A40..0x81200A94` uses the aligned `0x800000EC` and clamps `S` to
`M` if needed. All arithmetic is 32-bit guest arithmetic. Internal
`0x81201900+0x30` has only those two post-header writers, and the handoff
helper later publishes it to both `[0x80000034]` and `[0x80000038]`
(`0x812007A0..0x812007AC`). It publishes `0x18B19` to `[0x8000003C]`
(`0x812007B0..0x812007B4`), plus fixed boot magic/version and ArenaLo zero
at `[0x80000020/+0x24/+0x30]` (`0x81200784..0x8120079C`).

**PROVEN non-uniqueness from these files:** two possible initial `M` values
produce two non-overlapping, structurally valid placements:

| Hypothetical `M` | Conditional FST base | Conditional BI2 base |
|---:|---:|---:|
| `0x01000000` | `0x80FE74E0` | `0x80FE54E0` |
| `0x01800000` | `0x817E74E0` | `0x817E54E0` |

For `M=0x01800000`, the FST read is `align_up_32(0x18B19)=0x18B20`
bytes and ends exactly at `0x81800000`. This is a useful candidate and an
internal consistency check, **not** an observed retail address. Neither
`boot.bin`, BI2, DOL nor apploader contains the initial value of low-memory
`0x80000028`, so a static-only claim of the final FST address is **UNKNOWN**.
The `boot.bin[0x430]=0x803E74E0` word cannot substitute for it: that range
overlaps DOL text1 `0x80008D40..0x804AAC60`.

## A second deterministic part of the handoff: BSS then sections

**PROVEN on a normal completed AplMain path with the supplied DOL header:**
from initialized state 0, the state writes form `0→2` (`0x81200838..3C`),
`2→3` (`0x812008B0..B4`), `3→4` (`0x81200AC4..C8`), either
state-4 arm `→5` (`0x81200B68..6C` or `0x81200B94..98`), `5→6`
(`0x81200C18..1C`), `6→7` (`0x81200DB0..B4`), `7→8`
(`0x81200DF8..E04`), `8→9` (`0x81200F10..1C`), `9→10`
(`0x81201018..1C` or `0x81201084..88`), `10→11`
(`0x81201094..98`), then return zero at `0x812010C0`. State 12
only continues a large request and returns to its saved state. Therefore
neither FST order arm skips state 6. An externally corrupted/forced state
is outside this proof.

State 6 unconditionally executes raw words `0x807F0118` at
`0x81200D94` (`lwz r3,+0x118(r31)`), `0x38800000` at `+0xD98`
(`li r4,0`), `0x80BF011C` at `+0xD9C` (`lwz r5,+0x11C(r31)`),
and `0x4BFFF261` at `+0xDA0` (`bl 0x81200000`, the apploader zero-fill).
The DOL header is at loader internal `+0x40`, so those are header `+0xD8`
(BSS address) and `+0xDC` (BSS size). `0x81200DA4..0x81200DAC`
flushes that range. Only afterward do state 7/8 request text/data sections.
The PAL header says BSS `0x8056FE00 + 0x0008C7EC`, ending
`0x805FC5EC`. Data6 and data7 are loaded into that envelope after its
clear. Thus the deterministic pre-entry zero gaps, assuming completed
ordinary loading with no later overlapping FST/other writer, are:

| Zero gap | Size | Why |
|---|---:|---|
| `0x8056FE00..0x805E4500` | `0x74700` | Before data6. |
| `0x805EF020..0x805F2780` | `0x3760` | Between data6 and data7. |
| `0x805FC540..0x805FC5EC` | `0xAC` | After data7. |

This explains a subtle difference from the later CRT zero table at
`0x800055C8`: its middle range is only `0x375C`, four bytes shorter than the
full pre-entry gap. The apploader's earlier BSS clear covers those four
bytes, but the native CRT implementation should still preserve its *own*
table lengths and ordering. A DOL-only loader that clears just the later CRT
ranges would miss this pre-entry fact. The BSS conclusion is **UNKNOWN** for
an unobserved alternate DOL header or an interrupted loader path.

### FPR source before hardware initialization

The first `0x80003400` hardware helper calls `0x80371714` then
`0x80370CDC` (`0x80003410/14`). At `0x80370CFC`, the HID2-selected
paired-single path loads `0x805F1F38..3F`; at `0x80370D7C`, the common path
loads `0x805F1F30..37` as a double. Both addresses lie in the middle BSS
zero gap above. **PROVEN DOL-side nonwrite:** entry `0x80003154` first calls
the register helper `0x800032B0..0x8000333C`, then `0x80003400`. The
preceding `0x80371714` helper writes only at the configured stack
`r1=0x8060C5F0` and to HID0/HID2/GQR special registers
(`0x80370BA8`, `0x80370BB0`, `0x803725F4`); it does not store to either
FPR source address. Thus no DOL instruction writes those bytes between
entry and the first FPR load.

**Remaining pre-entry uncertainty:** on the ordinary DOL-before-FST arm,
state 9's DVD request writes the FST *after* the state-6 clear. Its
IPL-dependent destination could overlap the FPR source. A sufficient
non-overlap condition for the high-memory arm is
`M=[0x80000028] >= 0x0060AA59`; then
`FST_base >= 0x805F1F40`, past both load ranges. With the conventional
conditional candidate `M=0x01800000`, FST is at `0x817E74E0` and no
overlap exists. **UNKNOWN** until a retail-path trace: actual `M`, the
FST-order arm, and any external IPL/DVD callback or DMA write after the
BSS clear. Therefore the FPR bits must remain **UNKNOWN**, not promoted
to a proven zero merely from the BSS clear.

## Remaining proof boundary

An exact final low-memory/FST image cannot be derived from the five static
files alone. At minimum the original path must supply `M=[0x80000028]`, the
pointer and 28-byte block selected by `[0x800030F0]`, and the state-11
hardware-query result if that branch executes. These must be captured with
the AplMain request/branch trace and the DOL-entry snapshot described in
`PREENTRY_STATE.md`. The symbolic calculation and BSS ordering narrow the
required oracle; neither is a replacement for it.

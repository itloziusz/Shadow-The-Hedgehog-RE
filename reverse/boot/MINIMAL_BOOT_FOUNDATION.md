# Minimal boot foundation — PAL GUPP8P

**Boundary:** native process → validated DOL section image → translated DOL entry `0x80003154` → register helper return at `0x80003158`. Stop **before** the next `bl 0x80003400` hardware/runtime path. This is a deliberately narrow executable foothold, not a claim that the full original pre-entry machine state or CRT is already reproduced.

## Exact inputs and historical handoff

The checked input set is `../sys/boot.bin` (1,088 bytes, SHA-256 `7d387ce9162342a93a3becac1d2df6db1cccf6a2439f3fc97b29b2f7047d8e88`), `bi2.bin` (8,192, `8bbadffc2a7f79eda5982aa2fe30831969c2702aa9206f621b930aae0b70af6b`), `apploader.img` (122,456, `8a1fe9487b029b2518a6633b1e0eb9ec0922f5f5eb5afdd7eba1267a85f096fe`), and `main.dol` (5,773,024, `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`). These identities come from `INPUT_HASHES.txt` and were checked against the raw files in the earlier `REFINED_64BIT_RECOMPILER_PLAN.md` audit. The proof program consumes only `main.dol` as a **test fixture**; the other three are manifest provenance, not runtime inputs. A production build would compile validated section bytes and metadata into native objects.

`boot.bin` identifies disc `GUPP8P` and gives DOL disc offset `0x20300`, FST disc offset `0x5A1A00`, FST size `0x18B19` and a header FST RAM-address word `0x803E74E0`. `bi2.bin` contains the 24 MiB simulated-memory word (`0x01800000`), country code 2, one disc and long-filename flag 1. These are confirmed **file bytes**, not automatically required native runtime globals. The original console IPL/boot service loads the apploader; its Entry/Init/Main/Close callbacks are at `0x81200258/0x81200278/0x81200298/0x812002B8`. Main requests BI2/header and DOL sections by disc offset and has a conditional FST path; Close returns the DOL entry. The supplied disassembly proves that structure, but no independent trace in this package proves the exact selected callback sequence or final low-memory image for this disc. Consequently the proof replaces the disc request machinery with the known section map and stops before it could depend on uncertain state.

## Required section mappings and memory state

The `main.dol` header has entry `0x80003154`. Its ten nonempty sections, independently re-read in the earlier audit, are the **known** pre-entry code/data bytes. The test program validates these descriptors against the raw big-endian header and resolves guest addresses as checked views into the fixture; it neither allocates a 24 MiB emulated machine nor assumes unmapped bytes are zero.

| Section | DOL file offset | Guest address | Size | Guest end |
|---|---:|---:|---:|---:|
| text0 | `0x00000100` | `0x80003100` | `0x00002500` | `0x80005600` |
| text1 | `0x00002600` | `0x80008D40` | `0x004A1F20` | `0x804AAC60` |
| data0 | `0x004A4520` | `0x80005600` | `0x00001F20` | `0x80007520` |
| data1 | `0x004A6440` | `0x80007520` | `0x00001820` | `0x80008D40` |
| data2 | `0x004A7C60` | `0x804AAC60` | `0x00000480` | `0x804AB0E0` |
| data3 | `0x004A80E0` | `0x804AB0E0` | `0x00000020` | `0x804AB100` |
| data4 | `0x004A8100` | `0x804AB100` | `0x00072420` | `0x8051D520` |
| data5 | `0x0051A520` | `0x8051D520` | `0x000528E0` | `0x8056FE00` |
| data6 | `0x0056CE00` | `0x805E4500` | `0x0000AB20` | `0x805EF020` |
| data7 | `0x00577920` | `0x805F2780` | `0x00009DC0` | `0x805FC540` |

The header also states BSS envelope `0x8056FE00 + 0x8C7EC`. It overlaps initialized data6/data7. The existing host `System::load_dol` clears before section placement; this is a **current-host recipe**, not independent evidence of original final bytes. The CRT later processes separate zero ranges `0x8056FE00+0x74700`, `0x805EF020+0x375C`, `0x805FC540+0xAC`. They are **not** applied in this milestone because the stop point precedes the CRT data initializer at `0x80003340`; applying them now would shift an original startup effect earlier and could erase or falsely define bytes. No code in the translated slice reads BSS, FST, BI2, MMIO or low memory. Unknown bytes remain unavailable in `BootImage::ReadWord`.

## Low-memory values and first startup dependencies

For the **executed slice**, required low-memory values: **none**. `0x80003154` is `bl 0x800032B0` (`0x4800015D`), which only sets LR to `0x80003158`; the helper at `0x800032B0..0x8000333C` clears GPRs and sets `r1=0x8060C5F0`, `r2=0x805FA780`, `r13=0x805EC500`, then returns. These values are the helper's **output**, not assumed IPL inputs. The next entry instruction at `0x80003158` is `bl 0x80003400` (`0x480002A9`). The proof stops there; it does not claim hardware, exception, OS or CRT-data readiness.

Known low-memory **candidate writes**, deliberately not installed by this proof: apploader helper `0x81200774` conditionally writes `0x80000020=0x0D15EA5E`, `0x80000024=1`, `0x80000030=0`, and `0x80000034/+0x38/+0x3C` from internal FST/arena values. Other observed loader sites affect `0x800000E8/+0xEC/+0xF0` and optional `0x800030E4`. The existing host additionally synthesizes disc header, BI2 at its chosen address, memory-size/clock words, VI state and exception stubs in `System::apply_boot_configuration`; those are compatibility hypotheses for later startup, **not** evidence that the first two instructions need them. In particular `0x800030E4` is read after this stop point and its final value/branch is unresolved. Do not fill any unknown low-memory field with a convenient zero.

## What becomes build metadata, what remains unknown

The disc identity and DOL/FST offsets from `boot.bin`, BI2 facts, apploader callback/code identity, DOL header/section descriptors and entry address can be validated once by the build and recorded in a `BootManifest`. The disc offsets, apploader DVD request loop/128 KiB chunking, callback dispatch and runtime DOL file parsing exist to boot from optical media; this *proof slice* needs none of them. This does **not** authorize deleting original boot facilities from an existing full runtime before a pre-entry comparison: final low-memory values, FST placement, stack/device state, optional apploader branch selection and CRT effects remain unresolved above the chosen stop point.

The only supported minimum conclusion is: to execute through the first entry branch and register helper, the translated code needs known text0 bytes, a guest-address-to-section lookup, PC/LR/GPR state and the helper's translated semantics. The manifest maps all ten sections now so later startup can request known bytes without adding a generic DOL loader. Anything outside a section fails a checked lookup. The proof does not enter `0x80003400`, `0x80003340`, constructors or the candidate game call at `0x800510C0`.

## Tiny architecture and exact acceptance test

The public implementation is the C++17 `shadow_boot_foundation` library in
`include/shadow/boot/BootFoundation.hpp` and `src/BootFoundation.cpp`, with
`src/boot_probe.cpp` as its fixture-backed command-line probe:

```text
BootManifest (ten fixed section descriptors, entry)
    → InitializeMinimalBootState(fixture DOL bytes)
      (validate header; provide checked section-backed word lookup)
    → EnterTranslatedGameStartup()
      (entry bl + directly translated register helper)
    → STOP at 0x80003158 before hardware init
```

No general PPC interpreter, guest bus, apploader implementation, FST parser,
renderer or game framework is introduced. The fixture file reader in the probe
is only an executable test harness; swapping it for generated section arrays
is a later packaging change. The library checks **all 36** register-helper
instruction words at runtime, plus the entry and next branch. CTest separately
pins the complete DOL SHA-256 on each fixture run. It is not an invitation to
skip the rest of CRT.

Acceptance on this exact fixture:

1. Verify the four SHA-256 values above against `../sys/{boot.bin,bi2.bin,apploader.img,main.dol}`. The DOL hash is the executable-code identity gate; the others pin manifest provenance.
2. Configure the root project with `-DSHADOW_BOOT_DOL_PATH=<path-to-PAL-main.dol>`, build it, and run `ctest -R boot_pal`. The probe must exit 0 and print exactly `STOP pc=0x80003158 lr=0x80003158 r1=0x8060C5F0 r2=0x805FA780 r13=0x805EC500`.
3. A truncated DOL, altered section/header descriptor, wrong entry opcode/helper fingerprint or an unmapped guest word must fail rather than silently manufacture state.
4. Inspect the stop PC: `0x80003400` and later CRT/game calls have **not** executed. The full original pre-entry parity gate remains open until an independent original-path snapshot exists; this foothold must not be reported as full boot equivalence.

The earlier standalone proof passed on 2026-09-24. The buildable module was
rechecked on 2026-09-30 against the exact PAL fixture. Its regression test
rejects truncation, header changes and mutation of any of the 36 helper
instructions. This is the first native entry transition only.

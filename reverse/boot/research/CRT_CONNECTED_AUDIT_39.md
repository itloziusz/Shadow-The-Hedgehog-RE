# Connected CRT candidate after checkpoint 38 — independent binary audit

Date: 2026-10-01. Target: PAL GUPP8P, original read-only `main.dol`, SHA-256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.

This audit independently rereads bytes behind the already published CRT
ledger. It identifies the exact continuation and the next live dependency.
It does **not** promote a connected checkpoint or claim original console
execution. `PROVEN` means raw bytes/local effects, `STRONG` a bounded native
ownership consequence, and `UNKNOWN` an unprovided live state/reference
comparison. The coordinator must run the connected implementation and all
validation gates before any frontier moves.

## Raw machine code

`tools/agent_crt_connected_audit.py` reads the pinned DOL directly, derives
branch fields independently, checks their target against the existing Gekko
decoder, rejects undecoded words, and prints optional complete raw-word rows.
It passed **194 words**, eleven copy descriptors including the terminator,
and four zero descriptors including the terminator. The existing independent
raw-note gate also passed **111 words, 25 branches, zero SPRs, 41 data words**
for the CRT walker/fill/table. No generated output or original game file was
modified. An initial helper call had decoder parameters reversed; independent
branch-target checking caught it at `80372910`, and the call was corrected
before any audit result was used.

| Inclusive range | Words | SHA-256 of original bytes |
|---|---:|---|
| `80372908..80372928` | 9 | `8c479eac8507c67e243533cfa39f12e5120393c9c14c6188a4ff31e55579a533` |
| `8000341C..80003420` | 2 | `0cb782b06f956825b27451eaecc61084c0f64efe09268a623d7e8ba591f833fc` |
| `8000315C..80003264` | 67 | `fb1522f39384e02a869a64c7a4bb6f20102fabf532964809b35e687e6ca4a4e1` |
| `80003340..800033FC` | 48 | `ce0fe931e88e0a2bf60b46e3051c7dfaa17419b08f71e2fddb81b302964b157d` |
| `8000540C..800054F0` | 58 | `7f304b1e092ba0627d83c7dd1350bf790bcc3ec0537ed10b2bacf9054da80c1e` |
| `80370BF0..80370C14` | 10 | `e869b5b2fa29c9b8fa49e1529b30148e92b3654c83d2edde57471a8830bb6ff9` |

Descriptor bytes `80005544..800055E7` have SHA-256
`e2aac952ab345b3c3f16bc88ae11721119652bbd2a5731e8b37d9368dcf4dff2`.
Full instruction records already exist in `BINARY_CACHE_HANDLER.md` and
`BINARY_CRT_PREFIX.md`; the helper reproduces them from bytes rather than
using those notes as instruction inputs. Text0 file offset is `VA-80003000`.

## CFG position and hardware return

**PROVEN:** selector-one registration returns through actual saved bytes to
`80372908`. Then:

```text
80372908 addi r3,r31,0x1FC           # diagnostic pointer 8056157C
8037290C crxor 6,6,6                # clear numeric CR bit02000000
80372910 bl80370C8C                 # same stack-only logger; LR80372914
80372914 lwz r0,0x14(r1)            # actual cache-frame saved LR
80372918 lwz r31,0x0C(r1)           # restore outer wrapper LR in r31
8037291C lwz r30,8(r1)              # restore pre-cache incoming r30
80372920 addi r1,r1,0x10
80372924 mtlr r0
80372928 blr                       # return8000341C, clear PC low two bits
8000341C mtlr r31
80003420 blr                       # return8000315C, clear PC low two bits
```

Neither return target can be invented from ABI. The cache frame's saved
LR word was written by `8037283C` at `8060C5F4`; the saved r31/r30 words
were written at `8060C5EC/E8`. Their actual bytes must supply the loads.
The outer wrapper `8000340C` produced r31 from the entry `bl` LR
`8000315C`; it remains in r31 after the two returns. No new global read or
indirect callback invocation occurs after registration on this selected
tail. `803726D8` is installed as data; its eventual invocation remains a
separate consumer question.

## Input provenance and ordered CRT memory

**PROVEN:** after the hardware wrapper returns, `8000315C..68` creates
the sentinel frame and `8000316C` calls the table walker. The table cursor
is formed solely by instruction immediates. Exact eleven triples are ten
nonzero `source==destination` entries and a size-zero terminator. Thus
`80003394` memmove and `800033A0` cache-maintenance calls are unselected.
The three actual clear calls consume only immediate-derived table addresses
and immutable descriptor bytes; no old destination byte is read by the
reached aligned zero-byte fill path.

**STRONG, private owner only:** in a closed immutable native backend that
has no external writer or text modification, descriptors remain the loaded
DOL bytes. Prior reconstructed stores target bounded stack or handler RAM,
not text0 or descriptor bytes. This permits using the pinned descriptor
image as the live descriptor producer. It does not prove retail memory was
immutable. A live capture must still compare all `0xA4` bytes at entry and
walker entry; a modified descriptor requires decline or separate recovery.

| Ordered writer | Address | Written value / producer |
|---|---|---|
| `80003160` stwu | `8060C5E8` | old `r1=8060C5F0`, before r1 update |
| `80003164` stw | `8060C5EC` | `FFFFFFFF` from preceding li |
| `80003168` stw | `8060C5E8` | `FFFFFFFF`, overwrites backchain |
| `80003344` stw | `8060C5EC` | actual `bl` LR `80003170`, overwrites sentinel |
| `80003348` stwu | `8060C5D0` | caller SP `8060C5E8` |
| `8000334C/50/54` stw | `8060C5E4/E0/DC` | incoming r31/r30/r29, respectively |
| each `8000540C` stwu | `8060C5C0` | walker SP `8060C5D0` |
| each `80005414` stw | `8060C5D4` | actual fill-call LR `800033DC` |
| each `80005418` stw | `8060C5CC` | walker r31 = last identity-copy dst `805F2780` |
| each fill leaf | range below | eight zero-word stores per 32-byte group, then remaining zero-word stores |

The CRT stack is already within the checkpoint-38 fixed stack window, but
must retain overwritten values across all existing byte views. A separate
window with stale old copies would fail alias parity. The walker and fill
epilogues load their saved values from the bytes actually committed.

| Clear order | Exact interval, exclusive end | Byte count | Groups / extra words | Last store / final r6 |
|---|---|---:|---|---|
| 1 | `[8056FE00,805E4500)` | `74700` | `3A38` / 0 | `805E44FC` / `805E44FF` |
| 2 | `[805EF020,805F277C)` | `375C` | `1BA` / 7 | `805F2778` / `805F277B` |
| 3 | `[805FC540,805FC5EC)` | `AC` | 5 / 3 | `805FC5E8` / `805FC5EB` |

**PROVEN:** total `77F08` bytes, disjoint from every DOL-loaded section.
Only those bytes acquire proven zero state. Old bytes may be unknown because
the path fully overwrites them without reading. This is a write consequence,
not an assumption about loader-cleared memory. Handler `80586CB4` is erased
by call 1 after registration. FPR source `805F1F30..3F` is erased by call 2
after its earlier loads; already seeded FPRs stay unchanged.

**PROVEN:** gap `805F277C..7F`, memory from `805FC5EC` onward and the CRT
stack are outside these zero effects. Initialized data6/data7 must remain
intact. Copy[9] describes `9DB8` bytes, while the DOL data7 section is
`9DC0` bytes; the loaded tail `805FC538..3F` cannot be confused with a gap.
The helper tests hostile nonzero initial bytes plus four-byte sentinels at
every zero interval boundary and rejects overlap with loaded sections.
Those are interval-collapse checks, not differential PPC execution.

## Exact architectural output before80003170

**PROVEN, checked PAL path and stated entry state:**

| Field | Required value / provenance |
|---|---|
| r0 | `80003170`, walker epilogue's actual saved-LR load |
| r1 | `8060C5E8`, walker frame pop |
| r3 | `805FC540`, fill wrapper preserves actual final destination |
| r4 | `805FC5E8`, updating stores leave last written word address |
| r5 | zero, zero-table terminator size load |
| r6 | `805FC5EB`, final fill `r4+3` |
| r7 | zero, zero fill byte |
| r29/r30/r31 | restored from actual walker save bytes; r31 originates at `8000340C` |
| CR0 | EQ plus incoming XER.SO; other CR fields preserved |
| XER.CA | one from last `800054C8` `1+FFFFFFFF` = `100000000` |
| XER.SO/OV | retained; no OE arithmetic in this path |
| CTR | unchanged; fills branch on CR, not CTR |
| LR | `80003170`, epilogue mtlr from committed bytes |
| FPR/PS1/FPSCR/GQR/HID/L2CR | unchanged through this integer-only CRT slice |
| r2/r13 | unchanged SDA2/SDA bases |

Do not copy an old debugger `spr[SPR_XER]` field: it is known stale for CA.
Use the existing extended oracle's `architectural_xer` (`GetXER`) export.
The later `800033C4` compare only updates CR0 and cannot erase XER.CA.

## Small next block and first genuinely unprovided read

**PROVEN:** six instructions after CRT may execute without any new input:

| VA | File offset | Raw bytes / BE word | Effect |
|---|---:|---|---|
| `80003170` | `000170` | `38 00 00 00` / `38000000` | r0=0 |
| `80003174` | `000174` | `3C C0 80 00` / `3CC08000` | r6=80000000 |
| `80003178` | `000178` | `38 C6 00 44` / `38C60044` | r6=80000044 |
| `8000317C` | `00017C` | `90 06 00 00` / `90060000` | BE32 storezero at80000044 |
| `80003180` | `000180` | `3C C0 80 00` / `3CC08000` | r6=80000000 |
| `80003184` | `000184` | `38 C6 00 F4` / `38C600F4` | r6=800000F4 |
| `80003188` | `000188` | `80 C6 00 00` / `80C60000` | consumes live BI2 pointer; stop if unknown |

**UNKNOWN:** `800000F4` is outside all DOL-loaded sections and all three
CRT zero ranges. No earlier admitted native store creates it. Therefore
zero is not a default. The binary next compares the actual word to zero
at `8000318C`; if nonzero, `80003194` dereferences `B+0xC`. If zero,
`800031A4` consumes another preentry word `80000034`, and a nonzero value
then reads full debug word `800030E8`. These need explicit entry provenance.

The archive's `APPLOADER_PATH.md` / `BOOT_MEMORY_TIMELINE.md` find a
producer at apploader `81200AA4`, but its BI2 placement depends on prior
low-memory `80000028`. Raw BI2 `+8/+C` are zero only conditional on those
bytes being loaded at the actual B and remaining untouched. A synthetic
HLE capture may establish a bounded profile; it does not establish a
retail loader handoff. Changing B must trigger range/ownership checks.

If explicit validated low-memory/BI2 inputs are supplied, the next integer
route can proceed toward `80003264 ->80370E68`: preserve both fresh BI2
loads at `80003188/80003200`; reject debug2/3 context transfer until proven;
recover any relocation pointer/count before writes. The small leaf
`80370BF0..C14` then writes `805F1F18=80000040`, `80000048=00370C60`,
`805F1F1C=1` before the large OS dependency. These are exact raw effects,
not a claim of connected OS parity.

## Portable consequence and mandatory validation

**STRONG candidate:** express descriptor identity checks, three ordered
native zero operations and semantic stack save/restore. Maintain exact
volatile/CR/XER outputs where checkpoint consumers use them; avoid a CPU
interpreter. There is no hardware cache operation on the immutable identity
copy path. A descriptor with unequal src/dst must fail closed until both
copy and cache consumers are separately proven.

1. Capture complete entry, descriptor bytes atentry/walker, hardware return,
   each clear entry/return and `80003170/80003188`; run from80003154.
2. Compare all architectural fields including composedXER and unchanged
   FPR lanes, actual saved/readback stack words, exact range SHA/boundaries,
   earlier handler and FPR-source overwrite timing. Validate descriptors
   against DOL independently from implementation outputs.
3. Perturb preentry oldhandler and finite FPRsource, and hostile old bytes
   inside each zero range. Show clearing applies from actual stores while
   initialized sections/gaps and adjacent sentinels stay intact.
4. Mutate every reached/stop instruction and every descriptor/terminator;
   require decline before unknown copy/cache/invalidrange propagation.
5. Preserve original checkpoint37/38 replay gates, independent audit,
   repository and standalone gameplay gates before advancing the frontier.

Remaining physical interrupts/retail startup, descriptor external writers,
first frame and pixel gates stay **UNKNOWN** outside the admitted private
owner. The bounded CRT candidate has no new cache/timing mechanism to hide.

## Adversarial implementation re-check

I separately reviewed the new `src/NativeCrtPrefix.cpp` after producing the
binary audit. All **216** words in its raw code/descriptor literal gates
match the original binary. This includes the selected handler/EE paths as
well as the CRT reached/untaken variants and descriptor terminators. No
native source was edited by this audit.

The selected CRT loops have no signedness mismatch: all actual decremented
group/remainder values are nonnegative and below signed overflow, so their
CR0 result equals the raw `addic.` signed result; every reached decrement
starts above zero, so its carry is one. The implementation preserves SO/OV
and other XER bits rather than replacing XER with the reference carry word.
All three destinations and counts are four-byte aligned, have no byte tail,
and are at least32; the unselected leading/nonzero-byte branches remain
rejected. Every allocated range byte receives an explicit fill, so vector
zero initialization is not used as an unproven incoming-memory producer.

The native stores/save-load stack addresses and order agree with the ledger.
The paired memory view is refreshed from the shared fixed stack before each
checkpoint. In particular the lower sentinel is overwritten after the old
hardware saved register was consumed, the upper sentinel becomes actual
return LR, and each fill wrapper reloads its own committed saves. The final
r4 is retained and CR0 comparisons cannot silently clear XER.CA. The six
post-CRT instructions end with r0=0/r6=800000F4 and do not read the BI2 word.

The final controlled references `build/crt-zero-39.json`,
`build/crt-word-39.json` and `build/crt-config-39.json` independently supply
**90/75/90**, respectively, or **255** checkpoint observations. All captured
instruction words equal the original DOL, all164 descriptor bytes agree at
every point, all six outside canaries remain their controlled preentry values,
and the three full fill dumps per run contain **491,272** zero bytes at the
exact expected ranges, or **1,473,816** byte comparisons over three runs.
They vary initial MSR2032/30, L2E/configuration, oldhandler0/DEADBEEF/FFFFFFFF
and finite FPR seed values. PS0/PS1/FPSCR/GQR/MSR/L2CR/CTR remain unchanged
through all34 purely integer CRT observations per run.

An early capture variant recorded successful poison writes without explicit
readbacks for all nine interior probe words. The capture now verifies each
write immediately and records allseed words at every checkpoint. The strict
independent `--capture` mode rejects older missing-timeline captures; allthree
final captures pass **4,080** seed write/read timeline checks. It verifies
the installed-handler overwrite and each separate clear, source memory's
second-fill clear, unchanged outside canaries and complete raw-state shapes.
This prevents metadata alone from being counted as an observed perturbation.

At `80003188` the independently derived r0/r1/r2/r3/r4/r5/r6/r7/r13/r31,
architectural XER.CA, cleared handler and low-memory44 values all agree with
the reference. These are reference/raw-source checks, not yet a complete
native differential result or frontier approval. Review additionally found
that optional/empty outside-canary arrays could erase boundary coverage in
the developing differential gate; enforce the canonical six addresses and
six complete four-byte values with a negative regression before approval.

I then independently executed `shadow_boot_native_crt.exe` from the complete
entry fixtures produced for allthree final captures. A separate comparison
did not import the implementation author's differential helper. It retained
occurrence order for the repeated copy/fill PCs and derived stack validity
from this audit's raw prologue/store ledger. All **51** new checkpoints per
run, or **153** total, match **17,595** fields (113 raw architectural fields,
L2CR and handler per point), **13,324** known-stack byte comparisons and all
**1,473,816** produced zero bytes. The stop is `80003188` in every run.
The 51-point count is20 handler/return points, eleven copy heads, one zero
setup, fifteen fill entry/return points and four final points; no repeated
PC was discarded. Outside the new tail, the coordinator's full gate retains
the earlier L2/prefix comparison requirements.

**No concrete native discrepancy found within the pinned selected path.**
Remaining approval gates are mutation/decline tests, strict shape/store-order
regressions and the mandatory complete repository/gameplay regressions.
Neither this independent comparison nor an HLE zero dump proves retail
memory ownership, interrupts or physical timing. The first missing native
input remains the live word at800000F4.

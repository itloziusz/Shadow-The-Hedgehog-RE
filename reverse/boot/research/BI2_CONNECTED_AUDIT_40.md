# Independent BI2 continuation audit from original bytes

Target: PAL GUPP8P. Original inputs are read only:

| Input | Size | SHA256 |
|---|---:|---|
| `main.dol` | 5773024 | `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af` |
| `bi2.bin` | 8192 | `8bbadffc2a7f79eda5982aa2fe30831969c2702aa9206f621b930aae0b70af6b` |
| `apploader.img` | 122456 | `8a1fe9487b029b2518a6633b1e0eb9ec0922f5f5eb5afdd7eba1267a85f096fe` |

**PROVEN** means raw bytes and conditional architectural semantics;
**STRONG** means the bounded semantic classification supported by those
effects; **UNKNOWN** includes live bytes not yet exported and physical/
retail producer state. Static analysis cannot promote connected parity.

## 1. Exact data bytes versus observed live pointer

The whole original BI2 contains only four nonzero BE32 words:

```text
offset0004 =01800000
offset0018 =00000002
offset001C =00000001
offset0020 =00000001
```

**PROVEN raw:** `BI2+8` and `BI2+C` are zero, as is `BI2+24`.
These are file bytes, not native defaults or proof of loaded memory.
`tools/agent_handler_bi2.py <original sys directory> [capture.json]`
hash-checks the entire input files, prints the exact words and the raw
apploader request producer, and compares a full live `bi2_blob` when
available. Incomplete live blobs fail rather than silently truncate.

**OBSERVED synthetic:** the current `build/crt-zero-39.json` has
`[800000F4]=817E54E0` at original entry and the first unexecuted BI2 read
`80003188`. It contains no BI2 target dump, so the live debug and relocation
words remain **UNKNOWN in that artifact**, even though the input file is
known. The capture also has FST base`817E74E0`; the difference is8192bytes.
This observation agrees with the conditional loader formula, but is not a
retail IPL or DVD request trace.

## 2. Pointer and blob producer

Apploader VA maps to file offset`20+VA-81200000`. Independent BE-word read
and Capstone decode give:

| VA | File offset | BE word | Exact effect |
|---|---:|---|---|
| `81200A98` | `00000AB8` | `807F0030` | r3=loader`[r31+30]`, computed FST placement. |
| `81200A9C` | `00000ABC` | `3803E000` | r0=r3-2000 modulo32bits. |
| `81200AA0` | `00000AC0` | `3C608000` | r3=80000000. |
| `81200AA4` | `00000AC4` | `900300F4` | BE32store`[800000F4]=r0`. |
| `81200AA8` | `00000AC8` | `3C608000` | r3=80000000. |
| `81200AAC` | `00000ACC` | `800300F4` | Fresh reload of just-published BI2 destination. |
| `81200AB0` | `00000AD0` | `901E0000` | Write destination to caller output`[r30]`. |
| `81200AB4` | `00000AD4` | `38002000` | r0=2000. |
| `81200AB8` | `00000AD8` | `901D0000` | Write request length to`[r29]`. |
| `81200ABC` | `00000ADC` | `38000440` | r0=440. |
| `81200AC0` | `00000AE0` | `901B0000` | Write disc offset to`[r27]`. |
| `81200AC4` | `00000AE4` | `38000004` | r0=4. |
| `81200AC8` | `00000AE8` | `901F0144` | Set loader state4. |
| `81200ACC` | `00000AEC` | `807E0000` | Read published request destination again. |

**PROVEN conditional producer:** the ordinary completed caller must load
2000bytes at disc440 into this published destination. The source is the
entire original BI2, not just its32byte probe. Earlier raw apploader data
flow (`APPLOADER_PATH.md`) computes
`B=align_down32(80000000+min(M,01800000)-18B19)-2000`, with live
`M=[80000028]`. For M=01800000 it gives817E54E0; for M=01000000 it gives
80FE54E0. Neither M is encoded as an unconditional input by the disc.

For the observed high-memory B, `[B,B+2000)` ends exactly at observed FST
base817E74E0. The DOL-loaded sections, stack windows and three exact CRT
zero ranges are disjoint from this blob. The admitted closed native prefix
has no intervening registry/stack/CRT writer to `800000F4` or this BI2 range.
**UNKNOWN:** a retail request completion or external asynchronous overwrite.
The native continuation should therefore receive a measured pointer plus
known loaded bytes, never a fabricated address or disk-zero assumption.

## 3. Raw instruction ranges and CFG

The independent DOL audit helper prints every BE word/file offset/field and
checks project-decoder direct targets against independently sign-extended
LI/BD fields plus Capstone decode. Relevant inclusive ranges are
`80003188..80003264` (56words, including the next OS call),
`80003140..80003148` (three-word debug4 leaf),
`80370BF0..80370C14` (ten-word metadata leaf).
The raw next-stage guard range`80370E68..80370EA4` is16words.
The clock prefix`80379648..80379668` is nine words; its first unexecuted
producer`80379628` is separately pinned. The reusable EE-disable leaf has
five words. The expanded audit helper rereads all100DOLwords and
independently verifies the19direct branch displacements. It also checks
the14raw apploader producer words above; none is inferred from C++.

```text
3188 readB@800000F4 -> compareB
  B!=0:3194 readD@B+C --------------------------┐
  B==0:319C build/readA@80000034                │
        A==0 ->31F8                           │
        A!=0 ->31B8 readD@800030E8 ------------┤
  31BC/C0 compareD2; ifequal ->31E8            │
  31C8/D0 compareD3; ifequal ->31E8            │
  31D4/D8 compareD4; ifunequal ->31F8          │
  D4:31DC/31E0 ->3140 byteflagwrite ->31F8      │
  D2/D3:31E8 constanttarget8039F8E0 ->STOP      │
31F8 build ->3200 freshreadB@800000F4          │
  B0 ->3258                                  │
  Bnonzero ->320C readO@B+8                   │
     O0 ->3258                               │
     Ononzero ->3218 P=B+O ->321C readN@P      │
         N0 ->3258                           │
         Nnonzero ->3228 first=P+4            │
            CTR=N; relocateNconsecutivewords │
            ->3250 writearenaalignedfirst    │
3258 setr14/r15zero (onlyskipbranches)         │
3260 metadata leaf ->3264 STOPbeforeOScall ----┘
```

All comparisons here are **unsigned word** comparisons. Branches at
`80003208` and `80003214` are BO13/BI2, with a static prediction hint;
their semantic condition remains EQ and they do not consume/decrement CTR.
The relocation branch`80003240:4200FFF0` is BO16/BI0 `bdnz`, decrementing
CTR modulo32bits and jumping to`80003230` while nonzero, independently of
CR. No paired-single, FP, FPSCR, MSR, SDA2, MMIO or cache operation occurs
on the ordinary nondebug/metadata path.

## 4. Register and memory consequences

Let B be the fresh low-memory pointer; D the live debugword; O the live
offset; P=B+O; N=`[P]` when nonzeroO. The no-debug/offsetzero path performs
two real low-memory reads, one debugword read and one offset read.

**PROVEN important details:**

* `800031BC` sets r5=0, but `800031CC` unconditionally sets r5=1 before
  the D3branch. Thus D0 leaves r5=1 until the second pointer load.
* `80003194` puts D in r7. No later baseline instruction erases it.
* `80003200` reads B afresh into r5, not a cached first-read variable.
  Debug callees can change pointer memory on presently unproven paths.
* `8000320C` overwrites r6 with O. For O0, r6 remains0. For O!=0/N0,
  r6 remainsP; both skipbranches set r14=r15=0. These cases differ.
* For N>0, `80003228` sets r15=P+4, `8000322C` overwrites CTR=N, and
  `80003230..40` reads each word, adds B modulo32bits, writes it back and
  decrements CTR. The loop must retain exact read→add→store order.
* `80003244/48` then replace r5 with80000034;
  `8000324C:55E70034` masks r15 byFFFFFFE0 and writes that **array start**
  to the arena-high word at`80003250`. It does not publish the array end.
* For N>0, r14=N/r15=P+4 survive, CTR=0, r6=P+4N, r7=aligned(P+4),
  CR0=GT+liveXER.SO from the countcompare. For skipped paths CTR is
  unchanged; no generic ABI clobber may replace it withzero.
* `add`/`addi` here have no Rc/OE/carry effects. XER, including the CRT
  fill's produced CA, is unchanged. Comparisons replace only CR0 and
  copy live XER.SO; later metadata leaf leaves CR unchanged.
* D4 helper`80003140:38000001` sets r0=1;
  `80003144:980D5AF0` stores exactly onebyte at`805F1FF0`, thenreturns.
  The rest of its word must retain the CRT-produced neighboring bytes.
* D2/D3 indirect target is **PROVEN**, not arbitrary:
  `3CC0803A;38C6F8E0;7CC803A6;4E800021` materialize8039F8E0 and invoke
  through LR. Its effects and normal-return assumption remainUNKNOWN.
  Stop before this call until the context-transfer body is validated.

**PROVEN metadata leaf ordered stores:**

| Original PC / rawword | Address | Value | Producer / lifetime |
|---|---|---|---|
| `80370BFC:900D5A18` | `805F1F18` | `80000040` | r0 from`lis r4,8000;addi r0,r4,40`; replaces actual CRT-zero bytes. |
| `80370C08:90040048` | `80000048` | `00370C60` | r3 from`lis8037;addiC60`; `addis r0,r3,8000` subtracts80000000 modulo32bits. |
| `80370C10:900D5A1C` | `805F1F1C` | `00000001` | explicit `li r0,1`; replaces CRT-zero bytes. |

At the ordinary D0/O0 stop`80003264`: r0=1,r3=80370C60,r4=80000000,
r5=B,r6=0,r7=0,r14=r15=0,LR=80003264. CTR remains its entry value;
CR0=EQ+SO from the offsetcompare. SP remains8060C5E8; no new stack
frame was allocated. FPR lanes, FPSCR, all other SPRs and the validated
sync/L2 state remain unchanged.

The metadata stores at18/1C and debug4 byte at1FF0 alias the middle
CRT-zero range. A native backend must update that same produced storage
(or a coherent view) rather than leave an allzero range snapshot beside
separate nonzero globals. Subsequent OS reads must see the current bytes.

## 5. Next proven guard and unresolved clock producer

**PROVEN conditional continuation:** `80003264:4836DC05` calls80370E68
with LR=80003268. Its prologue stores that LR at8060C5EC, allocates24bytes
at8060C5D0, stores r31/r30/r29 atE4/E0/DC, then reads actual
`[r13+5A40]`=`[805F1F40]`. The prior exact CRT clear producedzero there,
and no metadata/debug4 store aliases it. This must be read from produced
bytes, not replaced by a checkpoint constant.

`80370E84..94` set r31=80586C40/r30=805610F8 and perform signedzero
compare. `80370E98:40820494` takes the first-call body because guardzero;
`80370E9C/EA0` set guardword1. The nextcall
`80370EA4:480087A5` reaches80379648, which obtains dynamic timebase and
pre-entry offset words. The clock dependency is genuinely unresolved,
not permission to install a fixedclock/zerooffset or skip the call.

### Exact clock entry up to the first unknown read

**PROVEN:** the direct call itself and its ordinary prologue/EE-disable
operations can execute without reading the clock. `80379668` calls
`80379628` and sets LR=8037966C. Stop **before** that first time-base
read, with all these prerequisite effects already committed.

| VA / file offset / rawword | Exact effect |
|---|---|
| `80379648 /00372F08 /7C0802A6` | `mflr r0`: r0=actual incoming LR80370EA8. |
| `8037964C /00372F0C /90010004` | `stw r0,4(r1)`: save LR at8060C5D4. |
| `80379650 /00372F10 /9421FFE0` | `stwu r1,-20(r1)`: write oldSP8060C5D0 to8060C5B0, then SP=8060C5B0. |
| `80379654 /00372F14 /93E1001C` | Spill currentr31=80586C40 at8060C5CC. |
| `80379658 /00372F18 /93C10018` | Spill currentr30=805610F8 at8060C5C8. |
| `8037965C /00372F1C /93A10014` | Spill currentr29=0 at8060C5C4. |
| `80379660 /00372F20 /4BFFCABD` | `bl8037611C`: LR=80379664; actual EE-disable helper. |
| `80379664 /00372F24 /7C7F1B78` | `mr r31,r3`: save the returned oldEE0/1. |
| `80379668 /00372F28 /4BFFFFC1` | `bl80379628`: LR=8037966C; first clock producer next. |
| `80379628 /00372EE8 /7C6D42E6` | **STOP before** `mftbu r3`, op31/XO371/TBR269. |

Let M be live MSR before this clock helper, E=(M>>15)&1. The independently
audited EE leaf changes only r3/r4/MSR: r3 becomesE, r4 and MSR become
M&FFFF7FFF. It does not change CR/XER. Therefore at the stop:

```text
PC =80379628        LR =8037966C       MSR=M&FFFF7FFF
r0 =80370EA8        r1 =8060C5B0       r3 =E
r4 =M&FFFF7FFF      r29=0             r30=805610F8
r31=E              CR0=EQ+liveXER.SO
```

r5/r6/r7/r14/r15 retain their exact BI2-path results; CTR retains the
previous value on skipped-relocation paths or the producedzero after a
nonempty relocation. XER.CA from the proven CRT fills is retained, as are
all other XER bits, other CR fields, FPR lanes/FPSCR and cache/GQR state.
The OSguard's actual zero comparison has replaced the relocation-path
CR0.GT before this stop; a translation that retains it is wrong.

The OSprologue changes paired-stack word8060C5EC to80003268; it must
replace the prior CRT return80003170 in both authoritative storage and
the paired-stack view. The lower sentinel8060C5E8 remainsFFFFFFFF, and
8060C5F4 remains the older hardware saved-return8000341C. Clock spills
atB0/C4/C8/CC/D4 similarly replace prior handler/CRT scratch contents;
unknown bytes adjacent to them must remain unknown.

**PROVEN downstream clock shape, not execution:** `80379628/2C/30`
read TBU/TBL/TBU (TBR269/268/269); signedcmp`80379634` tests upperwords,
`80379638` retries if unequal, leaving CR0.EQ+SO on stable return.
`80379670/74` then read live low-memory offsets`800030DC/800030D8`;
`addc80379678` adds lowwords and creates CA; `adde8037967C` includes that
carry in highword addition. The helper subsequently restores prior EE and
returns the high/low result for ordered OSstores`805F1F54` then`805F1F50`.
The time-base samples, offset inputs, stable-read retries and native clock
contract are **UNKNOWN**, so none of these read/add/store effects should
be fabricated merely to pass the next checkpoint.

## 6. Required falsification and promotion gates

Before connected reconstruction, capture the **full8192byte live BI2** at
original entry and immediately before its first read. Verify identity or
document every deliberate pre-entry change. Capture both fresh F4loads,
debugcompare edges, O/countreads, every actual relocated word and arena
write, metadata bytes and the OSguard producer/read/write.

Useful controlled entry experiments:

* Different nondebug Dwords (0,1,5,FFFFFFFF) expose unsignedcompare and
  r7retention; D4 tests the real onebyte helper. D2/D3 must stop at their
  resolved but unvalidated context-transfer call.
* O0 versus knownO/count0 exposes distinct r6provenance.
* Small count1/3 with complete known array bytes exposes CTR/word ordering,
  moduloaddition and publishing aligned **start**, notend.
* ZeroB requires independently provided arena-high and optional fallback
  debugword; unknown fallback input must stop at its exact read.
* Unknown/partial/unmapped/unaligned pointers, out-of-bounds O/count and
  overlapping mutable input/code/stack regions must fail closed.
* Relocated arrays may alias fields within the same BI2 block; if an
  admitted array does so, stores must affect later reads in that same
  owner. If ownership overlap is unproven, reject the profile explicitly.

At the initial static-audit stage these live data/chain comparisons were
**UNKNOWN**. Section7 records the subsequently obtained reference evidence.
Complete repository/gameplay gates remain coordinator responsibilities;
raw decode and static field identities alone do not advance the frontier.

## 7. Independent connected implementation and reference audit

**PROVEN for this finite interval:** independently reviewed current
`NativeBi2Prefix.cpp` against the 100 original DOL instruction words above,
including the five-word EE leaf inherited from the validated CRT prefix.
The helper independently recomputes all 19 direct branch targets from raw
LI/BD encodings and checks a separate disassembler. No GPR, CR, XER, LR,
CTR, MSR, stack-alias or same-owner BI2-relocation discrepancy was found.
Debug2/3 preserve their resolved target and stop before executing `blrl`;
their context-transfer effects remain **UNKNOWN**. The ordinary path stops
before reading TBU at `80379628`, preserving the unresolved producer.

**PROVEN observed input identity:** in `build/bi2-zero-40.json`, the full
8192-byte live blob at the unmodified entry, connected entry `80003188`,
`80003194`, `8000320C`, `80003264` and `80379628` matches original
`sys/bi2.bin` byte for byte. SHA-256 is
`8bbadffc2a7f79eda5982aa2fe30831969c2702aa9206f621b930aae0b70af6b`.
This supersedes the earlier missing-blob limitation for this capture only;
it does not make another uncaptured live blob known.

**PROVEN measured HLE-reference agreement:** independently reran the native
executable with six generated explicit entry fixtures in
`build/bi2-validation-40/`. The comparison did not import
`validate_native_bi2.py`. It preserved ordered repeated-PC occurrences and
checked all 113 machine fields plus L2CR, handler slot and low-memory44,
both stack views with the exact raw OS/clock spill ledger, each complete
BI2 blob, all four SDA observations and low-memory words. An unproduced
arena/low-memory48 value had to remain identical to its entry observation.

| Controlled profile | New checkpoints | Machine-field comparisons | Known stack bytes | Complete BI2 bytes | Global comparisons | Actual stop |
|---|---:|---:|---:|---:|---:|---|
| Original debug0 / O0 | 26 | 3016 | 2536 | 212992 | 116 | `80379628` |
| Debug4 byte helper | 29 | 3364 | 2824 | 237568 | 128 | `80379628` |
| O100 / count0 | 28 | 3248 | 2728 | 229376 | 124 | `80379628` |
| Unsigned debugFFFFFFFF / O100 / count3 | 40 | 4640 | 3880 | 327680 | 188 | `80379628` |
| Debug4 / O8 / count8, control-field alias | 63 | 7308 | 6088 | 516096 | 280 | `80379628` |
| Debug2 context target | 5 | 580 | 480 | 40960 | 20 | `800031F4` |
| **Total** | **191** | **22156** | **18536** | **1564672** | **856** | |

The alias reference profile proves that the earlier debug4 decision is
retained after relocation overwrites `[B+C]`: the consumed Dword was4,
and the stored relocated value becomes `B+4`. O8 is also the count word8;
neither O nor count is reread during the loop. The resulting state at the
time-base frontier has r6=`817E5508`, r14=8, r15=`817E54EC`, arena/r7=
`817E54E0` and actual debug byte1. The final complete relocated blob has
SHA-256 `c9cb3d1e78b0d98c5a15303fbe3deafe2de5b41e0fcb1c2bdaba12c184516bbf`.
The debug2 reference stops at `800031F4` with LR/r6=`8039F8E0`, r5=0;
there are no metadata, OS or clock side effects on that admitted interval.

### Tooling defect, correction and independent falsification

**PROVEN tooling defect:** the initial BI2 capture implementation indexed
`state['gpr'][7]`, `[6]` and `[14]` as though the register bank were a list.
It is a packed256-character hexadecimal string, so those expressions read
single nibbles instead of architectural words and could select an invalid
breakpoint plan. The coordinator corrected the production accessor to
validate the full bank and extract exactly `8*n:8*n+8`, and added production
regressions in `tests/test_bi2_tools.py` for distinct r6/r7/r14/r31 values
and malformed banks. This was a capture-tool error; no native behavior was
patched to match its erroneous output.

`tools/agent_handler_bi2.py` now also carries an independent finite-effect
ledger derived from the raw BI2 field/branch proofs and live `80003188`
input. It reconstructs the expected final register state, modulo32 word
relocations, exact ten stack stores, four SDA words and arena/metadata
effects without importing native source or expected native output. All six
reference profiles pass it. For the original profile, separately perturbing
every one of the 113 machine fields, selected changed/untouched stack and
BI2 bytes, both low-memory words and all four SDA words rejected all
**124** altered results. These are tool-falsification experiments, not
additional observed game executions.

Reproduction, with no generated output or cache writes:

```powershell
python -B reverse/boot/tools/agent_handler_bi2.py <original-sys-directory> build/bi2-alias-40.json
```

**UNKNOWN outside this audit:** physical time-base behavior, offset/timing
producers, debug context-transfer behavior, asynchronous hardware effects
outside the admitted interval, and the complete repository/gameplay gate
result. This audit supplies independent evidence; only the coordinator's
full regression protocol may promote the connected frontier.

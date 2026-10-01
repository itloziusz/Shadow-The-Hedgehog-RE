# Checkpoint 40: live BI2 through the first OS clock entry

Target: Shadow the Hedgehog, PAL GUPP8P. DOL SHA256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
Original BI2 SHA256
`8bbadffc2a7f79eda5982aa2fe30831969c2702aa9206f621b930aae0b70af6b`.
Original inputs remain read only and uncommitted.

**STRONG bounded connected reconstruction:** `NativeBi2Prefix.cpp` calls the
unchanged earlier native sync/L2 prefix through checkpoint39 CRT, then follows
the admitted BI2/debug/relocation, metadata, first OS guard, clock frame and
EE-disable paths. It stops **before `80379628: mftbu r3`**, with no tick value,
clock result, retry decision or offset injected. Retail/hardware timing and
complete game/first-frame parity remain **UNKNOWN**.

## Raw authority, regions and CFG

`BINARY_BI2_OS_PREFIX_40.md` preserves 95 exact instruction/stop words with
VA, file offset, four bytes, big-endian word, bitfields and independent decode.
All 95 mutations decline. Its registered raw gate checks 19 direct targets
and four SPR encodings. The independent audit additionally checks the reused
five-word EE leaf and the 14-word apploader request producer.

| Region | Classification | Required state and successors |
|---|---|---|
| `80003188..8000325C` | boot metadata / CRT | Two fresh low-memory pointer reads; unsigned debug/offset/count tests; optional byte leaf or in-place relocation. Unknown inputs stop at their consumers. |
| `80003140..80003148` | debug flag | D=4 writes exactly one SDA byte; returns through the actual LR. D=2/3 resolves `8039F8E0` but stops before the unvalidated context-transfer call at `800031F4`. |
| `80370BF0..80370C14` | boot metadata | Three ordered stores; updates the same CRT-produced SDA storage. Return target is the actual LR `80003264`. |
| `80370E68..80370EA4` | OS first-call guard | Actual CRT-produced guard read, real spills, zero comparison, guard store and direct clock call. Other initialization body is not recovered here. |
| `80379648..80379668` and `8037611C..2C` | clock call / EE state | Saved LR, 32-byte frame, three spills, EE clear and saved prior bit; call the first live time-base reader. |

```text
80003154 -> validated sync/L2 -> handler return -> exact CRT
  ->80003188 read B=[800000F4]
    B=0: stop before unprovided arena fallback read800031A4
    unknown blob: stop before debugword load80003194
    D=2/3: materialize actual target; stop before800031F4
    D=4: one-byte leaf -> second fresh B read80003200
    other D: second fresh B read80003200
      O=0 or N=0: distinct skip states, no CTR/arena fabrication
      N>0: ordered N word additions/stores and CTR countdown
  ->metadata leaf ->OS frame/guard ->clock frame/EE leaf
  ->STOP before80379628, first live time-base high-word read
```

## Inputs and exact data flow

The apploader publishes `FST_base-2000` and requests 8192 bytes at disc offset
440. **PROVEN bytes/conditional producer; OBSERVED synthetic live mapping:**
all six fresh reference runs observe B=`817E54E0`, and their complete original
loaded blobs equal the original BI2. That address is an explicit measured
input, not a backend default. Original and controlled snapshots remain
separate. Only labelled pre-entry word writes are permitted, each with an
immediate readback and complete entry-byte comparison.

Native owns a complete typed BI2 buffer, the earlier bounded stack and current
CRT-produced ranges. It rejects misalignment, wrapped/unknown table addresses,
incomplete blobs, out-of-owner counts and aliases with existing loaded code,
low memory, stack or CRT ranges. Unknown B stops at3188; B=0 stops before31A4.
There is no generic GameCube RAM, interpreter or fabricated object state.

Both F4 reads are explicit. Their owner has no intervening admitted writer;
debug4 writes only `805F1FF0`. D=2/3 sets r6/LR=`8039F8E0`, preserves r5=0/1,
and stops before BLRL. D=4 preserves adjacent bytes after writing01.

For O!=0, P=B+O and N=[P]. N>0 sets r14=N, r15=P+4 and CTR=N, then performs
each ordered read/add/store with **32-bit word addition**. Address/count
extents use wider arithmetic before consumption. Final r6=P+4N, CTR=0;
`80003250` publishes **aligned array start**, `(P+4)&FFFFFFE0`, not its end.
Offset/count zero leave CTR unchanged and have different r6 outputs. O=8/N=8
aliases the debug word; the already-consumed value stays in its register while
the single owned blob changes. Relocated opaque words are not assumed valid
call targets or pointers.

No FP, paired-single, FPSCR, HID/GQR or cache operation occurs in this tail.
Integer additions do not change XER; comparisons replace CR0 and retain SO.
The OS guard is read from bytes actually cleared earlier, then written1.
Metadata stores `805F1F18=80000040`, `80000048=00370C60`, `805F1F1C=1` precede
that guard. Native's current storage and all captured views remain coherent;
the earlier completed-fill snapshots record history rather than current state.

OS spills overwrite EC/E4/E0/DC; the clock saves LR=`80370EA8` atD4 and
r31/r30/r29 atCC/C8/C4 before lowering SP to `8060C5B0`. Both stack views
retain these aliases. At the ordinary stop LR=`8037966C`, r0=`80370EA8`,
r3/r31=prior EE, r4=current MSR with EE cleared, r30=`805610F8`; r5/r6/r7/
r14/r15 retain the selected BI2 route's results. CR0=EQ+SO from the guard;
XER retains the prior CRT-produced carry. Nothing consumes the time base yet.

## Multi-pass evidence and regression

| Controlled pre-entry profile | New CP | CPU/L2/slot/low44 fields | Known stack bytes | Complete BI2 bytes |
|---|---:|---:|---:|---:|
| Original BI2, L2E=0, slot0 |26|3016|2536|212992|
| Debug4, L2E=1, finite paired seed, slotDEADBEEF |29|3364|2824|237568|
| Offset100/count0, MSR30, varied source/configuration |28|3248|2728|229376|
| DebugFFFFFFFF, offset100/count3, wraparound words |40|4640|3880|327680|
| Debug4, offset8/count8, same-owner field alias |63|7308|6088|516096|
| Debug2, exact context-call stop |5|580|480|40960|
| **Total** |**191**|**22156**|**18536**|**1564672**|

Two independent audits replayed the compiled native executable without
importing the coordinator validator. All fields, actual stacks, full blobs,
856 primary SDA/produced-global observations and 83 word/two byte effects
agree. An independent finite effect ledger rejects124 altered results.
Each reference profile also reruns the complete earlier L2 and51-point CRT
comparison, including491272 cleared bytes and34 ordered stores. Current
native owners are checked across all491272 bytes again after the new writes.

Capture identities (generated build artifacts are excluded from publication):

| Capture | SHA256 |
|---|---|
|zero|4f56ce20e842c08acd5567ba74c7aeca453196c541341497f6abdfc6a000e967|
|debug4|11b6670cdbbb9ed369cb28a80b9de8d1b064bc26b1e27e98429f354f926de496|
|count0|f8182330df3f55ff8d2832b4e9a65a2512ccc607f3c6c9dfa2df8d98850f38fb|
|array3|57fbe3920b344f1e207de150dee3ee2fc230debb86db7322a35d13a902c54b1c|
|alias8|4cb90a7e391b4de0be396e091dce727cd65ba7577dbc2a8c58dd603f652a9e93|
|context2|0f7dbbced027abdf6e4e87d8c93b125464ee6d428025b1a375af98cff6f28c6a|

Native executable SHA256
`b23860e196c96907999b5344892d2ca072d68c32cca317b8e03b3c7a5bce6b09`;
private oracle SHA256
`1b37d68d6ea92d4eca1407823616c8c1b9a59153a2809c97615f09fcfacd959f`;
SYS-only disc SHA256
`a8a526ef7006a2d2a8d41f536ef30b208ff197c185c05e9b2276e6e93cb41a8e`.
Each local validation JSON records the explicit entry-fixture digest too.
This startup mode is conditional HLE evidence, not a retail IPL capture.

Final full MSVC Release **56/56 CTest**, standalone gameplay **27/27**, and
`RESULT: Dark mission cleared -> next stage index 6 (stg0200)` pass. Original
sync/L2 sources, declines, runtime and pixel requirements remain intact. No
first-frame or completed-game claim is made. Native tests cover both L2E
branches, signedness, stale state, aliases, unknown/null/partial mappings,
overflow/extent failures, context stops and95 raw-word mutations.

## Tooling feedback and genuine defects fixed

`capture_bi2_state.py`, `validate_native_bi2.py` and `test_bi2_tools.py` automate
full owned-byte capture, input-derived repeated-PC plans, register/global/stack
diffs, independent ordered store ledgers and precise unknown-input stops.
They preserve the old capture path and rerun prior validation first.

Independent review caught a real capture bug: packed GPR hex was indexed as a
register list. Strict bank/slice parsing plus distinct r6/r7/r14/r31 and
malformed-bank regressions prevent recurrence. The Markdown byte checker
also incorrectly rejected original BO13 equality branches; it now permits
only the prediction hint difference and rejects actual predicate changes.
Checkpoint39 report hardening rejects contradictory seeded-canary views,
relabeled stack windows and malformed fill labels. The recognizer's stale
proof, CTR/conditional-return, rotate/insert and SO fixes remain tested.
Diagnostic probes preserve byte-store readbacks and canonicalize lower hex
earlier stops, so debug2 reports its actual stop rather than claiming the
clock frontier was reached. Malformed/mismatched byte effects decline.

Reusable raw detectors now scan1216776 original text words in one batch:
two eight-word fill motifs (`80005498`, `8039D95C`), one EE leaf (`8037611C`)
and two high/low/high stable time-base motifs (`80379628`, `804035D4`). New
candidate records stay **UNKNOWN** with **STRUCTURAL_MATCH** hypotheses;
the second fill/sampler is not promoted. Four solved scoped examples feed
the seed database. The learning batch records274 unknown regions,123 learned n-grams,
971 structural hits and six complete shared-pattern family matches; these
counts measure tooling output, not semantic completion or game percentage.
Recording the new unresolved clock frontier subsequently adds one UNKNOWN
record (275); it does not demote a solved region or change connected progress.

## Exact next dependency

**UNKNOWN:** live tick producer/units and observed high-low-high retry/rollover,
the producer of low-memory offsets `800030D8/DC`, interrupt delivery and later
timing/device consumers. Static words prove retry until the high samples agree;
they do not supply a clock. Another consumer at `80376EBC/C0` measures a low
tick delta, so arbitrary host units or a copied sample would be unjustified.

Research41 follow-up: `CLOCK_RESEARCH_41.md` records source/offset/units,
actual rollover, carry and research parity through before80373AC4. Production
still stops80379628; native elapsed-time/event ownership remains UNKNOWN.

Historical next step: capture original clock inputs and offsets, trace their producers and
downstream units, test retry/rollover and carry independently, then express
the proven clock consequence in native code. `OS_ENTRY_NEXT_BOUNDARY.md`
retains the exact subsequent addc/adde, restoration and low-before-high store
proof. No forward native progress past80379628 is justified yet.

## Reproduction

Use original read-only `<PAL-main.dol>`/`<PAL-bi2.bin>` and the existing private
SYS-only oracle; all generated paths below stay in ignored repository build/.

```powershell
python -B reverse/boot/tools/validate_native_bi2.py <PAL-main.dol> <PAL-bi2.bin> `
  build/reverse/boot/Release/shadow_boot_native_bi2.exe `
  build/reverse/boot/Release/shadow_boot_native_crt.exe `
  build/reverse/boot/Release/shadow_boot_native_l2.exe `
  build/bi2-zero-40.json build/bi2-validation-40
python -B reverse/boot/tools/scan_boot_motifs.py <PAL-main.dol> `
  build/boot-recognizer-hidden-state-36.sqlite build/raw-motifs-40.json
ctest --test-dir build -C Release --output-on-failure
```

See capture CLI `--help` for labelled debug/offset/array controls. Each run
requires a fresh private user directory; none writes code or mid-chain state.

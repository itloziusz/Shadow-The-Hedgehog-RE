# Checkpoint39 — connected handler return and exact CRT ranges

PAL GUPP8P DOL SHA256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
The binary remains the authority. `NativeCrtPrefix.cpp` calls the unchanged
checkpoint38 runner from `80003154`; checkpoint37 sync remains untouched.
This is a STRONG finite closed-owner reconstruction, not retail IPL parity.

Follow-up: checkpoint40 reruns this entire prefix in six more profiles; final
repository gate is56/56. Shared report tests now32/32 and recognizer33/33.
Seeded-canary contradictions, relabeled stack windows and malformed fill
labels have explicit rejection regressions. Historical counts below describe
the original checkpoint39 captures, not a new retail/physical claim.

## Bytes, CFG, effects and native representation

**PROVEN:** all216 continuation fingerprint words are reread from the original
DOL, including41 descriptor words and the first unexecuted BI2 load. Every
word mutation declines. Per-word raw bytes/offsets/encodings are in
`BINARY_CACHE_HANDLER.md`, `BINARY_CRT_PREFIX.md` and the independent
`HANDLER_BINARY_AUDIT_39.md`/`CRT_CONNECTED_AUDIT_39.md` records.

```text
80372904 ->80373378 selector1 installer
  frame/spills ->8037611C disable EE, return old EE bit
  read opaque oldword80586CB4 ->store connected handler803726D8
  selector1 !=16 ->80373564 ->80376144 restore old EE bit
  load real saves ->80372908 ->leaf logger ->cache epilogue
  ->8000341C saved r31 return ->8000315C
  sentinels ->80003340 copy/zero descriptor walker
    ten source==destination entries: no copy/cache calls
    three calls8000540C->8000543C: aligned zero-word groups/remainders
  saved-byte return ->80003170
  write80000044=0 ->STOP before80003188 read800000F4
```

The original register helper explicitly clears r28/r29 (80003314/18); no ABI
default is used. The installer selector is a connected immediate1. Its old
slot is an explicit pre-entry opaque word, never dereferenced or invoked.
Unknown slot state stops before `803733B4` after actual frame spills. Known
zero, arbitrary noncode and other words retain their exact value in r30/r3
until the caller overwrites r3. `NativeHandlerSlot` binds every read/store
to80586CB4 and reads back through the same owner; no generic guest RAM exists.

EE leaves clear/set only8000, preserve the other current MSR bits and return
the previous EE bit. Selected connected inputs have EE0; all32 one-bit
projections and both pure restore branches are tested, without claiming live
EE1 delivery parity. The private backend has no pending/asynchronous source.
Final CR0 comes from the saved-EE comparison, not selector comparison; all
compares retain live XER.SO and the remaining CR fields. FPR lanes, FPSCR,
CTR, SDA/SDA2, HID/GQR state are carried unchanged in this section.

The installer r28 spill overwrites the earlier helper backchain at8060C5D0;
the skipped logger f8 save must retain these newer bytes. A shared bounded
stack owner applies stores and supplies actual saved-word loads. Its paired
stack snapshot is an alias view refreshed from those bytes, including the
sentinel overwrites. The caller logger writes a backchain/r3..10 and clears
CR bit6 before taking the known no-FPR-store path. Its function has no output
device call. All34 new word stores have immediate same-owner readbacks and
are checked in raw-instruction order against independent captured inputs.

**PROVEN exact CRT ranges (exclusive ends):**

| Address | Size | End | Consequence |
|---|---:|---|---|
|8056FE00|74700|805E4500|Erases the earlier handler slot after registration/return.|
|805EF020|375C|805F277C|Erases the old FPR source after the earlier loads; FPR lanes retain loaded values.|
|805FC540|AC|805FC5EC|Leaves the stack and adjacent loaded-data tail untouched.|

All ranges are disjoint from DOL-loaded sections. Unknown prior bytes are
overwritten without being read. The native vectors are not exposed until all
their bytes receive explicit ascending group/remainder fills; allocation zero
initialization is not input evidence. Size/cursor values come from the pinned
owned descriptors. Every preceding native write is disjoint from the table;
external writers and interrupts are excluded. Modified relocation descriptors,
leading/trailing-byte and nonzero/short-fill paths decline, not silently skip.
The DOL header BSS envelope is never blanket-zeroed. Gaps805F277C..7F and
805FC5EC onward remain unknown/preserved.

Actual counter decrements set XER.CA; the last remainder decrement1→0 yields
carry1. SO/OV carry through. The final volatile r4 is the last word address
805FC5E8; r6 becomes805FC5EB, then the proven postCRT immediates produce
r0=0/r6=800000F4. Saved LR/r29/r30/r31 are read from applied bytes.

## Four passes and perturbation evidence

Identification and independent falsification reread raw words, branches,
mask operands, descriptor bounds, storage aliases and every consumer.
Three full-entry controlled HLE captures vary L2 branch/configuration,
MSR32 versus30, FPR sources and opaque handler0/DEADBEEF/FFFFFFFF. Sixteen
nonzero pre-entry seed words poison interior/boundary bytes; every seed has
immediate and per-checkpoint readbacks. Six exact outside word canaries retain
their value; zero dumps compare all produced bytes. No native intermediate
register, branch, completion or expected zero result is supplied.

| Capture | New CP | New state fields | Known stack byte comparisons | Produced zero bytes | Stores |
|---|---:|---:|---:|---:|---:|
|crt-zero-39.json|51|5866|4612|491272|34|
|crt-word-39.json|51|5866|4100|491272|34|
|crt-config-39.json|51|5866|4612|491272|34|
|Total|153|17598|13324|1473816|102|

The unchanged L2 prefix is replayed first and all emitted prefix states must
equal that run exactly:93 earlier CP/10602 fields/2228 known-memory comparisons
across these scenarios. Combined246 CP/28200 fields match. The independent
CRT auditor executed the candidate and obtained the same153 tail CP and
17595 fields (its count excludes the three final lowmem44 words).

Capture digests (ignored local build artifacts, no raw inputs committed):

| Capture | SHA256 |
|---|---|
|zero|8e8676b60cab5562128cdd2109d7d5e6a1918d345021e7e246745bed65170e07|
|word|fd7dfc92cca890d4780872c0e1812d290693f2573acc3f82af00ccfe4b13c273|
|config|90a061a769ec5f59474a4ddd6bc9329b7569b2bbdbda1048a29fbfdcf8c377be|

Native executable SHA256
`b5b5a97f31ded02fc87e0ccba034b27764b4159d48585086a7b1d64c81ec0797`.
Oracle/disc identity is inherited from checkpoint38's pinned manifest.
Full MSVC Release build/CTest **53/53**; gameplay **27/27**, stg0100 Dark35/35
→stageindex6. Ten opaque-slot/L2 variants, missing-slot/wrong-address declines,
216 raw/data mutations and32-bit EE projections pass. All previous gates remain.

## Automation learned and genuine defects repaired

`boot_state_diff.py` shares strict113-field raw layouts and115/118/120 native
schemas, occurrence-ordered trace cursors and independent byte-validity checks.
Capture callbacks reuse the validated L2 plan. Unknown/malformed numeric fields,
symbolic readbacks, truncated arrays, missing seeds/canaries and reordered equal
PCs fail. The recognizer now checks every native state token and effect readback.
Reusable raw detectors identify a register-renamed EE clear/return motif and
an eight-word group fill; their results are only STRUCTURAL_MATCH.

Independent audit fixed stale VALIDATED evidence reuse for changed raw bodies,
missing CTR decrements/BO predicates, lost conditional-return fallthrough,
symbolic rlwimi's discarded old destination and omitted XER.SO dependencies.
An rfi or unsupported branch target remains UNKNOWN. Focused recognizer32/32
and state/capture schema25/25 pass. See `BOOT_TOOL_AUDIT_39.md`.

## Next frontier and unknowns

**UNKNOWN live dependency:** `80003188` reads `[800000F4]`, the BI2 pointer.
Its producer is the apploader's FST placement/request completion; disk BI2
zero fields alone cannot establish live bytes. Trace and capture both fresh
pointer reads, debug/relocation branches and the OS first-call consumer next.
`BI2_CONNECTED_AUDIT_40.md` is static evidence only at this checkpoint.
Physical interrupts/cache/timing, retail entry state, full OS/constructors,
application startup and first-frame/pixel parity remain unresolved.

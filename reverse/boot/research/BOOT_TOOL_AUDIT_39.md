# Boot capture and recognizer audit after checkpoint 38

Date: 2026-10-01. Scope: the connected continuation from `80372904`, handler
exchange at `80373378`, and the following CRT walker/fill path. The original
PAL DOL SHA-256 remains
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
This audit changes analysis safeguards; it does not advance native boot.

## Concrete defects found and repaired

**PROVEN, tooling:** `Store.upsert` previously preserved a row's `VALIDATED`
label/evidence while replacing its range, binary analysis, raw fingerprint,
and semantic family. Thus an unrelated span under the same ID could inherit
an earlier proof. Validated rows now pin `(start, end, binary digest, raw-body
digest)`, reject a changed binding, and preserve all reviewed contents on a
same-span rescan. Explicit promotion remains the evidence-review operation.
Tests perturb the VA, endpoint, binary digest, and raw digest independently;
another test attempts to replace reviewed effects/name without changing bytes.

**PROVEN, ISA:** conditional branches using CTR did not decrement the tracked
counter. A raw `li r3,2; mtctr r3; bdnz +4; mfctr r4` consequently propagated
2 rather than 1. The recognizer now models the wrapping decrement when BO2
is zero and retains both exact BO predicates: the updated CTR zero/nonzero
test and the CR bit test. All 32 BO values against four counter inputs,
including zero wraparound, have independent regression assertions. The CFG
retains both conditional edges rather than using these expressions to force
a branch.

**PROVEN, ISA/CFG:** conditional `bclr` was treated as an unconditional return,
silently omitting the fallthrough. Both the external return state and local
successor are now retained. This is directly relevant to original
`800054E0 / 4D820020: beqlr`: a nonzero tail continues to `800054E4..800054EC`.
The PAL regression checks that successor and that unmodeled `addic.`/`stbu`
remain explicit unsupported operations. Unconditional `bctr` also no longer
receives an impossible fallthrough. `rfi` cannot fabricate an LR-based
return: SRR semantics remain unsupported. A `bcctr` encoding requesting a
CTR decrement is rejected as an invalid form.

**PROVEN, data dependency:** symbolic `rlwimi` discarded the prior destination
even though unmasked bits survive. The expression now retains both operands;
two distinct destination producers cannot become a false equal-state join.
Compare/record CR expressions now retain XER.SO provenance and project
numeric XER mask `80000000` exactly. SO, OV, and other XER bits are not
interchangeable. Adversarial cases include SO-only, OV-only, and all-bit input.

No native `sync`, L2 ownership implementation, or reference checkpoint was
changed. Focused invocation:

```powershell
python -B reverse/boot/tests/test_recognizer.py <PAL-main.dol>
```

Result: **31/31 tests passed**, including original PAL constructor, pointer,
frontier, and memory gates. `git diff --check` passed. Full repository and
gameplay regression remain coordinator gates before frontier promotion.

## Capture/state-diff findings

**PROVEN, bounded L2 gate:** `validate_native_l2.py` independently checks
native checkpoint order, raw instruction words, exact array lengths, labelled
pre-entry changes, byte-validity timelines, and ordered L2 read/write events.
Its duplicate-PC rejection is correct for the admitted one-visit prefix;
repeated loop samples need explicit occurrence indices, not dictionary
deduplication.

**PROVEN, diagnostic gap reported to coordinator:** `run_native_probe(entry)`
checked stop line count/PC but exported arbitrary other tokens as
`known_fields`; a 118-token line with `r3=UNKNOWN` was accepted diagnostically.
Malformed legacy `WRITE` lines could become empty dictionaries. Neither
bypasses the separate strict L2 parity validator. A shared raw-state parser
should nevertheless reject either before frontier state is reused.

**STRONG, helper design:** use a versioned state schema shared by capture and
native diagnostics. Require all scalar widths, 32 GPRs, 32 PS0/PS1 lanes,
eight GQRs, exact byte-window lengths, validity masks, and checkpoint sequence
before comparing anything. Match `(PC, occurrence)` where a path repeats.
Validate controlled changes against an explicit pre-entry allowlist; preserve
the unmodified snapshot and write/readback records. Report the first differing
register, CR bit, memory address, or ordered event. Observed extra fields must
not silently reduce required comparison coverage.

**STRONG, memory dependency helper:** merge overlapping memory windows into
one address-keyed byte view before comparing stores/loads. Reject inconsistent
overlap bytes. Maintain each byte's last writer `(PC, occurrence, event index,
input provenance)` and a known/unknown bit. A multi-byte load is concrete only
if every consumed byte has a producer. Update-form stores must snapshot the
old register operands before writing the updated base. Compare stores before
later restores/overwrites, then compare the final known bytes as well. This
prevents skipped FPR logger slots from being misclassified as unwritten when
they alias an earlier helper backchain.

The recognizer's existing `effects` list is **static PC order**, not a runtime
write timeline: mutually exclusive stores, repeated iterations, and joined
values are not an executed transaction sequence. The new helper should take
reference/native ordered observations, never infer that sequence by sorting
static effects. Calls stay opaque until independently summarized.

## Reusable raw-evidence detector candidates

**PROVEN anchor / STRUCTURAL candidate:** interrupt-guarded pointer exchange.
Original `8037339C` calls `8037611C`; `803733B4` loads the prior word and
`803733C0` stores the supplied word through the same r4 address; the selected
edge reaches `80373564` and calls `80376144`. Match this load/store operand
identity and bracket calls, then inspect the bracket callees for exact
`mfmsr` / mask `00008000` / `mtmsr` behavior. A generic bracket-call shape
alone is insufficient to classify interrupts or guarantee atomic visibility.
Minimal experiment: break before disable, before/after slot write, before
restore, and at caller return; perturb EE and the pre-entry old slot.

**PROVEN anchor / STRUCTURAL candidate:** fill loops. At
`80005498..800054BC` one induction pointer writes eight same-register words,
advances 32 bytes, and branches back from a decrement-record predicate;
`800054C8..800054D0` handles words and `800054E4..800054EC` handles bytes.
Recognize the induction/write/count motif, but distinguish constant byte
fill, replicated nonzero fill, and copy by producer provenance. Zero cannot
be inferred from the store register number or a matching opcode n-gram.
Perturb lengths at 0, 1, 3, 4, 31, 32, 33 plus destination alignment and
high-bit fill inputs; verify carry/CR effects and address aliases.

**PROVEN anchor / STRUCTURAL candidate:** descriptor walkers at
`80003340..800033FC`, tables `80005544..800055E7`. Recover the size field
used as terminator, fixed descriptor stride, pointer producers, skip/call
edges, and table-region provenance. Source==destination copying is a proven
path condition, not permission to erase descriptor reads/control effects.
Input/descriptor mutations must remain labelled experiments; only the raw
real-table run can validate the original connected path.

**UNKNOWN:** physical interrupt arrival, retail IPL slot provenance, cache
timing, and whether future callers execute the selector-16 handler branch.
These detector candidates neither assume nor bypass those dependencies.

## Independent audit of shared state diff and opt-in CRT capture

Reviewed `tools/boot_state_diff.py`, `tools/capture_crt_state.py`, and the
optional extension callbacks added to `capture_l2_state.py`. The default L2
flow retains its original checkpoint/perturbation logic; observation,
pre-entry controls, and tail stepping are opt-in callbacks. Only the
controlled entry is changed; no tail callback writes registers or memory.

New `tests/test_boot_state_diff.py` provides **32 focused test cases** for
115/118/120-field native schemas, symbolic/short/extra raw fields, truncated
reference arrays, independent memory masks, first differing register/address,
repeated-PC occurrences, missing/extra samples, and capture seed provenance.
These synthetic tests validate tooling contracts, not game semantics.

Adversarial counterexamples exposed equal-value noncanonical tokens in
`compare_fields` and whitespace-separated bytes in `compare_memory`. The
coordinator strengthened both to use exact raw-field shapes; those tests now
pass. ACK-but-missing handler/poison writes and unknown MSR readback also
decline after coordinator readback checks. Two later failing cases were
reported and corrected by the coordinator: missing required `handler=None`, and
MSR readbacks with seven/nine digits, `0x` prefix, or `+` prefix. A numeric
`int(...,16)` comparison alone is not a raw-word shape check. The required
handler is now validated unconditionally, and MSR readback uses the shared
eight-digit raw-field parser. **32/32 tests pass** after those corrections
and the report-gate additions below.

**PROVEN, additional report-gate repairs:** `checked_report` accepted a
contradictory view of a canary address: seed write/readback and repeated seed
observations could say nonzero while all six canary samples said zero. It now
requires equal values for seeded boundary addresses across both views. Tail
stack observations must explicitly name `8060C570`, provide 144 bytes, and
provide the separate 16 paired-stack bytes; a different base or truncated
window cannot relabel data. Fill range address/size labels now require exact
eight-digit raw words rather than accepting prefixed or overlong numeric
strings. The positive capture API and state schema are unchanged.

Regressions explicitly remove/add canary values, remove/add/reorder boundary
ranges, shift a boundary by one byte, alter its width, insert UNKNOWN/short
words, conflict overlapping views, and alter stack base/window lengths.
All three real `crt-zero-39`, `crt-word-39`, and `crt-config-39` captures pass
the strengthened report gate with **51 tail occurrences each**. Full native
differential replay and repository/gameplay regressions remain coordinator
requirements for checkpoint promotion.

**PROVEN observation, not native parity:** independently parsed
`build/crt-zero-39.json`: all **90 checkpoint occurrences / 10,170 raw
register fields** satisfy the shared reference schema, and all 90 captured
instruction words match the pinned DOL. Its 51-occurrence handler/CRT tail
matches the explicit capture plan without deduplicating repeated PCs. Eleven
copy-table stops, four zero-table stops, and three fill-call sequences are
retained separately. Three complete fill observations cover **491,272 bytes**
and contain only zero. All six adjacent canary words match their labelled
pre-entry values through the final `80003188` snapshot. There are 16 labelled
pre-entry word writes, each with its requested/readback value. The initial
handler slot is zero on this input; its later first-fill clearing is an
observable overwrite, not permission to omit registration.

The full captured byte arrays remain ignored build artifacts. The capture
contains synthetic controls and supplies evidence only for this private HLE
scenario. The coordinator must compare the actual native run, validate each
input label and raw descriptor, reject unknown state, and run complete
regressions before promoting the connected boot frontier.

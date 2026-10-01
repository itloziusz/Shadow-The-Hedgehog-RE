# Independent L2 evidence and validation audit — checkpoint 38

**2026-10-01.** This audit independently reads the existing captures, original
PAL bytes and reference implementation. It does not launch an oracle, alter
the checkpoint-37 sync code or declare the repository regression gate passed.
The coordinator must complete that gate before advancing the checkpoint.

Original DOL SHA-256:
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.

## Provenance and independent observations

**PROVEN, binary:** all **282 checkpoint instruction observations** across
the eight zero/enabled/config/IP/second-poll/I/test/reserved captures match
the raw DOL words at their recorded PCs. This counts repeated observations,
not 282 distinct newly recovered instructions. The original raw catalogue
remains `BINARY_CACHE_HANDLER.md`.

**PROVEN, capture provenance:** every capture retains the untouched
`0x80003154` entry, then records the controlled HID0 `0011C064` and its
controlled L2CR readback before execution. The two positive source variations
are pre-entry experiments, with matching source bytes at every checkpoint.
None is a claimed retail IPL input. The second-poll experiment carries both
its mid-chain flag and a marker on the modified snapshot; it must never be
used as full-entry parity evidence.

| Controlled L2CR | Observation and limit |
|---|---|
| `00000000` | `803728A0` falls through; both polls fall through; final L2CR `80000000`; reaches `80372904`. |
| `C0480000` | `803728A0` takes `803728F8`; no L2 writes, helper or logger; configuration remains `C0480000`. |
| `40480000` | Disabled path retains all three admitted configuration bits; final `C0480000`; reaches `80372904`. |
| `00000001` | `80372684` takes its first back-edge to `80372678`. Capture stops after proving that edge; no completion is invented. |
| Mid-chain bit 0 set at `803726B4` | `803726C0` takes `803726A8`; logger runs, then a new read and busy branch occur. This is branch falsification, not a natural hardware status transition. |
| Incoming I, test-support or reserved bit | HLE executes its word echo. These captures expose unsupported states; the native owner rejects them. They do not establish physical validity of those inputs. |

**PROVEN, binary; observed selected HLE state:** three bits directly govern
this slice: L2E `80000000` at `803728A0`, L2I `00200000` in the request and
clear expressions, and L2IP `00000001` at `80372684` and `803726C0`. Fresh
reads at `80372688` and `803728DC` are distinct operations even when their
readbacks equal earlier values. The two busy branches have different
successors and neither original loop has a timeout.

**STRONG, selected architectural-state evidence:** independently checked
**75 helper/logger stored-word observations** against the captured bytes.
The helper stores LR at `8060C5E4`, its backchain at `8060C5D0` and r31 at
`8060C5DC`. The normal logger stores its backchain at `8060C570` and r3–r10
at `8060C578..594`. The caller clears CR bit 6, so the logger skips its FPR
stores. Across the L2 slice, both FPR lanes, FPSCR, composed live XER, CTR,
GQRs, SDA registers, HID0 and HID2 remain unchanged in all eight captures.

The nested second-poll logger uses SP `8060C560`, below the fixed L2 window
starting `8060C570`. Its backchain and first two argument stores are checked
through the separate dynamic stack window; the remaining argument stores are
inside the fixed window. Do not claim that the fixed window alone observes
all effects of that negative path.

## What the reference establishes

**PROVEN, reference source:** the pinned interpreter's generic `mtspr`
assignment writes the entire operand to `spr[index]` before the L2CR switch
case, which performs no additional work. Source SHA-256:
`698f153ad3a9e457c7d51f9a84358b86140c29b879f35dc39fe70412f23f8556`.
The original GDB L2CR reader/writer already exposes that stored word. Thus
IP=1 stays set in this reference; the trace cannot prove read-only hardware
status behavior, cache-tag invalidation, physical completion or timing.

The unchanged archived reference and the read-only export executable were
independently hash-checked against the manifest. A zero-input comparison
matches **819 shared JSON fields at 39 ordered checkpoints**. Executable
digests are respectively
`db536025ee02190c20ccb9bfa94ee36ce28bb996f2e88574e46734b353bdc526`
and `1b37d68d6ea92d4eca1407823616c8c1b9a59153a2809c97615f09fcfacd959f`.
This excludes an observed export-induced divergence in these fields; it does
not convert the archived interpreter into a physical Gekko oracle.

## Native implementation and adversarial checks

**STRONG, bounded native equivalence:** `NativeL2Prefix.cpp` obtains the
checkpoint-37 state by calling the unchanged full-entry runner. Only entry
L2CR is supplied; no intermediate poll result, acknowledgment or expected
output is an input. Imported stack bytes retain validity, helper returns load
their actually stored words, and the logger preserves the proven selected
CR/FPR path. Incoming I/IP, test-support and reserved modes decline even if
L2E would otherwise skip the helper. The common successor constructs selector
1 and handler address `803726D8`, then stops before the live handler-slot call
at `80372904`.

The minimal completion applies to an owner with authoritative RAM, immutable
native code and no inherited cache payload, device queue or external observer.
It reproduces the required store visibility and configuration consequences
without a tag array or full L2 emulation. Physical pending state, dirty console
cache inheritance and timing remain **UNKNOWN**; these are not silently
represented by the word-only reference or by the closed native contract.

The independent audit found validation gaps and the coordinator fixed them:

- Controlled source writes require entry readback equality.
- Duplicate PCs cannot disappear through dictionary construction.
- Native and reference checkpoints require the original CFG order and exact
  coverage, rather than a count or set alone.
- Every read/write/barrier/completion effect requires exact order and identity.
  Equal readbacks do not make fresh read sites interchangeable.
- Validity masks are derived from the raw store timeline. All-zero masks or
  omitted L2 windows cannot suppress intermediate memory comparisons.
- Raw scalar, GPR, FPR-lane and GQR shapes are checked before comparisons;
  truncated arrays cannot reduce coverage through `zip`.

`tests/test_l2_capture.py` passes **27 focused cases**, without an emulator or
DOL dependency. They falsify unlabelled entry changes, source mutations,
wrong raw words, repeated PCs, mid-chain forcing, reordered equal-valued
reads, omitted fresh reads, misplaced/wrong-generation completion, unknown
effects and suppressed/invented byte validity. These tests complement the
coordinator's native mutation/state tests and complete regression runs.

## Remaining proof obligations

**UNKNOWN:** retail IPL handoff, physical L2 tag/poll history and timing,
interrupt/device interleavings, dirty-cache inheritance, subsequent handler
ownership and invocation, later L2 consumers, and first game-frame parity.
No new claim about these follows from reaching `80372904`. Full root and
standalone gameplay regressions remain required before checkpoint advancement.

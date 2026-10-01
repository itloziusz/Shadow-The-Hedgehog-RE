# Checkpoint 38 — bounded native L2 consequence

Target: PAL GUPP8P `main.dol`, SHA-256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
Addresses identify original instructions; semantic names below are recovered.
This extends checkpoint 37's **closed immutable native owner**, not the retail
IPL or a physical cache model. `NativeBootPrefix.cpp` and its validated sync
implementation are unchanged. Acceptance of this checkpoint requires the full
repository and standalone gameplay gates recorded in `PROGRESS.md`.

## 1. Binary → consumers

**PROVEN:** `BINARY_CACHE_HANDLER.md` contains all raw bytes, offsets, encoding
fields and independently decoded instructions for this slice. The full raw
gate checks 305 words, 56 branches and 14 SPR operands. The new native module
fingerprints 95 additional words: caller `80372894..80372904`, helper
`80372640..803726D4`, MSR/L2 accessors and all twenty logger words. Mutating
each of these words independently rejects the run, including unadmitted
retry edges and the next unexecuted call.

| Consumed field | Producer and instruction | Consumer / exact consequence |
|---|---|---|
| L2E, numeric `80000000` | Live SPR1017 `mfspr` at `80370AFC`, called at `80372894`; `54600000` at `80372898` masks only bit31 numerically. | `cmplwi` at `8037289C`, `bne` at `803728A0`: set → `803728F8`; clear → `803728A4`. |
| L2IP, numeric `00000001` | Fresh read called at `80372678`; `546007FE` at `8037267C`. | `cmplwi 80372680`, `bne 80372684`: set → retry `80372678`; clear → `80372688`. No timeout. |
| L2IP, numeric `00000001` | Fresh read called at `803726B4`, after the guest clear; mask at `803726B8`. | `cmplwi 803726BC`, `bne 803726C0`: set → logger `803726A8`, then another read; clear → `803726C4`. |
| L2E | Fresh reads at caller `803728C0` and helper `80372654`. | `5463007E` clears only E before writes at `803728C8` and `8037265C`. |
| L2I, numeric `00200000` | Fresh read at `80372664`; `64630020` ORs I. | Write at `8037266C` requests global invalidation while disabled. Fresh read at `80372688`; wrap mask `FFDFFFFF` at `8037268C`; write at `80372690` clears I. |
| All other bits | Each **fresh read**, not a remembered software operand. | The masks retain them in the write operands. Their hardware acceptance/meaning must be established separately. |
| Final configuration | Fresh read at `803728DC`, after MSR restoration. | `64608000` at E0 sets E into r0; `540302D2` at E4 clears I into r3; E8 writes it. |

**PROVEN CFG:**

```text
80372894 live read → E test
    E=1 ───────────────────────────────────────────────┐
    E=0 → save MSR / sync / MSR=30 / sync / sync        │
          → fresh read / disable E / sync              │
          → helper: save LR, backchain, r31 / sync      │
             → fresh read / disable E / sync           │
             → fresh read / request I                  │
             → poll IP ──1→ poll IP                    │
                       0→ fresh read / clear I          │
             → poll IP ──1→ logger → poll IP           │
                       0→ read saved LR/r31 / return    │
          → restore saved MSR / fresh read             │
          → set E, clear I / logger ────────────────────┤
                                               803728F8
                                  r4=803726D8; r3=1
                                               80372904
                                  STOP before handler call
```

The first caller logger (`803728F4 → 80370C8C`) **does not output anything**
in this binary: it commits a stack backchain and r3–r10, restores SP and
returns. `crxor 6,6,6` at `803728F0` clears numeric CR bit `02000000`, so
`40860024` at `80370C90` skips all eight FPR stores. It preserves the other
CR fields, XER, CTR, FPR lanes and FPSCR. Unsigned zero comparisons replace
CR0 and copy the live XER.SO; no signed approximation is used.

**PROVEN memory provenance:** main SP is the connected producer's
`8060C5E0`. Helper stores `[8060C5E4]=803728D4`,
`[8060C5D0]=8060C5E0`, `[8060C5DC]=80561380`. Return loads those actual
bytes, not cached expected values. Caller logger stores its backchain at
`8060C570` and r3–r10 at `8060C578..594`. Unwritten bytes remain unknown;
skipped FPR stores preserve any prior known bytes. In particular the skipped
f8 slot starts at `8060C5D0`, already containing the helper backchain in its
first four bytes. Every new store is applied before its
readback and independently compared with reference memory. No SDA/SDA2,
heap, object, device, code write or indirect call occurs before the new stop.

## 2. Required native consequence

Primary source: IBM *Gekko RISC Microprocessor User's Manual*, v1.2,
Table 2-19 (printed 2-25/26), §§9.1.2–9.1.4 (printed 9-3/4):
[original manual](https://doc.kodewerx.org/documents/gekko_user_manual.pdf).
L2IP is **read-only**. Invalidation clears tag data/status/LRU while L2 is
disabled. The required ordering finishes prior stores and pending L2 work,
starts invalidation, waits for completion, clears I, then enables E. It does
not zero authoritative RAM or require rebuilding a 256KiB cache in native C++.

**STRONG, bounded native contract:** `RunImmutableNativeL2Prefix()` calls
the unchanged checkpoint-37 entry runner. Its private RAM already contains
all accepted stores; compiled code is immutable and has no L2-resident
payload. There are no shadow data/tag arrays, inherited dirty lines, devices,
external observers, DMA/write-gather writes, interrupts, ECC fault injection,
cache-test pushes, performance-counter or time-base reads in this interval.
The native owner's visibility completion can therefore be synchronous:

1. Retain the live configuration word and the actual E-selected branch.
2. Verify committed same-owner bytes at each bounded store-completion point.
3. Disable E before the invalidate command. Record its ordered request.
4. Complete the native visibility generation: no cache-owned payload exists
   to discard or expose. RAM bytes are unchanged and independently read back.
5. Derive IP from **outstanding owned work**, separately from the software
   operand; I remains set until its explicit guest clear.
6. Perform both fresh reads and their exact predicates. Any unexplained busy
   state fails closed. Restore the saved live MSR and enable from the final
   fresh read. Preserve all ordinary register/stack/logger effects.

No supplied acknowledgment, reference checkpoint constant, forced branch,
host fence, interpreter or emulated hardware participates. The private request
and completed generations express the observable completion dependency; they
do not pretend to count Gekko cycles. The two zero polls are consequences of
this owner's completed synchronous operation, not a claim that retail IP is
always zero.

**Scope:** initial `L2CR & ~C0480000 == 0`: E, CE, DO and WT may vary.
CE/DO/WT are retained because their cache/ECC mechanism has no consumer in
this closed finite interval. WT is not changed after enable. Each of the
sixteen E/config combinations is exercised. Incoming I, IP, TS and every
reserved bit are rejected, including on the E=1 branch. TS can change memory
visibility; pending status has unexplained history; reserved echo is not
proof of a legal hardware state. Future ECC/timing/cache-policy consumers
require another proof rather than inheriting this finite contract.

**UNKNOWN/excluded:** pre-entry retail cache tags/dirty data, fault/pending
exceptions, physical completion duration and retries, external memory
observers, DMA, test mode, other cache/mode profiles and retail IPL state.
Do not feed a physical dirty cache snapshot into this cache-free native owner.
The manual also recommends disabling DPM during L2 initialization. This
particular slice changes MSR but does not clear HID0[DPM]; its physical power
conditions are not settled by HLE. The native owner has no console power/
clock mechanism or timing observer, so this is not a claim that the original
code satisfies that hardware recommendation for every inherited state.

## 3. Perturbation and falsification

The pinned HLE reference assigns the software word to SPR1017 and then does
nothing in its L2 case. It does not simulate L2 tags, read-only-IP write
acceptance, invalidation duration or completion. These source facts and the
manual limit what an observed HLE zero can prove.

`capture_l2_state.py` uses existing GDB writers **before `80003154`** for
clean HID0 `0011C064` and controlled L2CR; original and controlled entry are
both retained. Positive runs take all checkpoint-37 points first. No code,
branch or intermediate native state is patched. A fixed reference stack
window includes the helper, caller and logger; it is never a native input.

| Controlled input | Observation / native treatment |
|---|---|
| L2CR `0`, zero source | Disabled branch; I visible at first poll; software clear before second; final E set. Full native comparison. |
| L2CR `C0480000`, finite source | Enabled branch; one L2 read, no L2 write, MSR disable, invalidate or logger. Retained CE/DO/WT. Full native comparison. |
| L2CR `40480000`, changed scalar and signed-zero paired source | Disabled branch preserves CE/DO/WT through all five writes and eight reads. Full native comparison. |
| L2CR `1` | HLE first poll reads `00200001`; one step at `80372684` returns to `80372678`. No forced completion. Native rejects entry. |
| IP set only at second poll, explicit mid-chain **negative** experiment | `803726C0 → 803726A8 → logger → 803726B4 → 803726C0`; still busy. This trace cannot validate native chain advancement. |
| Incoming I `00200000` | HLE proceeds without proving a new hardware request/completion history. Native rejects. |
| TS `00040000` / reserved `2` | HLE echoes these bits without their hardware memory consequences. Native rejects. |

The first test-mode oracle launch did not open GDB; a fresh private launch
succeeded. This was a capture failure, not evidence of a semantic L2 stop.
An initial capture readback check incorrectly compared hex letter case; it
was corrected to numeric comparison before using any capture.

## 4. Differential results and reproducibility

`shadow_boot_native_l2 <PAL-DOL> <explicit-entry-with-l2.txt>` accepts only
the checkpoint-37 entry fields plus **one pre-entry L2CR word**. It emits the
unchanged earlier states, new pre-instruction checkpoints, validity-tracked
memory, ordered reads/writes and applied store readbacks. Missing/extra or
overflowing input fields decline. `validate_native_l2.py` compares full entry
replays and rejects mid-chain perturbations, stale/repeated checkpoints,
identity/source mismatches and reordered equal-valued reads/writes.
Independent review found that a comparison based only on counts/sets could
accept reordered checkpoints or erase memory validation with empty masks.
It now enforces exact native/reference CFG order and independently derives
each validity mask from the store timeline, including retained stack aliases;
empty or missing new memory windows fail. The Python regression covers these
validation failures as well as raw-word and input-provenance corruption.

| Capture | Native checkpoints | Raw state fields | Known memory bytes | New committed words |
|---|---:|---:|---:|---:|
| `l2-zero-38.json` | 36 | 4,104 | 984 | 12 |
| `l2-enabled-38.json` | 21 | 2,394 | 260 | 0 |
| `l2-config-38.json` | 36 | 4,104 | 984 | 12 |
| Total | 93 | **10,602** | **2,228** | **24** |

All twelve earlier cache-frame words also agree. The unchanged original
oracle and read-only export agree on **819 shared JSON fields at 39 points**
for the same zero-entry experiment. Arrays count as one JSON field here;
the native table above counts individual raw state fields.

Local capture SHA-256 (outputs remain ignored in `build/`):

| File | SHA-256 |
|---|---|
| `l2-zero-38.json` | `42745002c6d01c246a7aa96b999cf7496af4cdc9997a4d755a7a0548526d3b23` |
| `l2-enabled-38.json` | `4935edb5b9cbd8e65f645d1bed6c361ea6f6716d66a71c2d525c82e3ff108025` |
| `l2-config-38.json` | `b7e24fa320f9bb1f659460dd9bd03c8fd61ea28b4b5bd0f7f9ef8253a890bd06` |
| `l2-ip-38.json` | `589bd4ae739381fd0f7050152e0e6f43ade2c864f98b2970b65f141f5fd43342` |
| `l2-poll2-38.json` | `933191655ca04d4b7a2e3150148ba77e05c915e33cb8a070484cd3fae833c371` |
| `l2-stock-38.json` | `25287f8495a472f3cbc3e2ef6d2aa9e0e57cad648a89f83a833044d9d4876223` |

Native tests cover all sixteen admitted profiles, every unsupported input
bit with E clear/set, all 32 single-bit branch perturbations, SO/other CR
retention, MSR/CTR, unknown/partial/unaligned/outside memory, saved-return
provenance, enabled-path absence of work, command versus status, fresh reads,
stale-state isolation and 95 raw-word mutations. Earlier 166-word sync tests
and three full checkpoint-37 replays remain required.

## 5. Next frontier

**PROVEN:** both admitted branches produce handler selector r3=1 and address
r4=`803726D8` at **`80372904: bl 80373378`**. Native stops before executing
that call. Installer reads the live previous handler slot at `80586CB4`;
the static DOL/BSS value is not a valid live default. Resolve its producer,
interrupt save/restore and slot registration/consumer behavior next, then
compare the hardware-wrapper return and later CRT entry. No first game frame,
pixel equivalence or full boot completion is claimed.

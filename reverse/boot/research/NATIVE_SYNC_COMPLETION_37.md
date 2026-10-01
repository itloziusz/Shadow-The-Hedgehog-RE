# Bounded native completion of the first sync — checkpoint 37

**2026-10-01.** PAL GUPP8P DOL SHA-256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
The new executable runs from `0x80003154` through `0x80371730`, the GQR
tail, both-lane FPR seed, and the enabled ICE/DCE checks. It stops **before**
`0x80372894: bl 0x80370AFC`, whose callee reads live L2CR. This is a
bounded native semantic reconstruction with explicit entry inputs, not
retail IPL, physical cache timing, or complete game boot parity.

## 1. Identification: binary and control/data-flow

**PROVEN-BINARY:** the complete word/offset/encoding catalogues are
`BINARY_BOOT_PREFIX.md`, `BINARY_FPR_PREFIX.md`, `BINARY_CACHE_HANDLER.md`
and `SYNC_GQR_CHAIN.md`. No newly guessed function boundary is used. The
connected regression mutates all **166 checked words**, including the next
unexecuted stop word; every mutation declines.

| Reached range / CFG position | Inputs and transformation | Required next consumer |
|---|---|---|
| `80003154`, helper `800032B0..333C`, `80003158` | Exact GPR clear/stack/SDA constants; LR direct calls. No RAM or FP effects. | wrapper `80003400` |
| `80003400..3410` | Supervisor MSR read, FP-enable OR/write, save outer LR in r31, direct paired-setup call. | `80371714` |
| `80371714..1728`, HID2 leaves `80370BA8..BB4` | LR save; BE32 stores `[8060C5F4]=80003414`, `[8060C5E8]=8060C5F0`; SP decremented 8. Live HID2 OR `A0000000` enables LSQE/PSE, retains WPE. | instruction visibility, then first `psq_l` |
| `8037172C`, `803725F4..2600` | Live HID0 read, OR `800`, full-word write, return. r3 retains the command operand; readback is a separate value. | `80371730: sync` |
| `80371734..1764` | Zero r3; eight ordered GQR writes; saved LR is read from **applied bytes**, SP restored, aligned LR return. | `80003414: bl 80370CDC` |
| `80370CDC..0E00` | Read MSR/HID2, derive CR0 with live XER.SO, take paired edge from the produced PSE bit; read explicit live source at `805F1F38` and SDA scalar `805F1F30`; paired sweep, scalar sweep retaining PS1, derive FPSCR, return. | `80003418: bl 80372838` |
| `80372838..2860`, HID0 leaf `80370AEC..AF0`, `80372874..2880` | Four BE32 frame stores; r31 from lis/addi; two **live derived** HID0 reads; exact masks `8000`/`4000`, unsigned comparisons with XER.SO and bne conditions. Both taken branches are tested; next call is not entered. | `80372894`, live L2CR |

All relevant original GPR/CR/XER/LR/CTR/MSR/FPSCR, two-lane values, GQRs and
stack writes are preserved in checkpoint records. No indirect dispatch is
unresolved inside this prefix: each blr target comes from the actual LR
producer or written/restored stack bytes. No MMIO, callback, DMA submission,
write-gather port write, time-base read, code store or exception handler is
reached. The two pre-sync stores are disjoint from both DOL text intervals
`80003100..80005600` and `80008D40..804AAC60`, all initialized DOL sections,
and the later source bytes. The original stack loads remain loads from the
written bytes, rather than a remembered return constant.

## 2. Falsification: the old HLE HID0 word is unsuitable

**PROVEN-MANUAL:** consult the original [IBM Gekko manual](https://doc.kodewerx.org/documents/gekko_user_manual.pdf),
HID0 Table 2-4 (printed 2-11), §3.3.5.1–2 (3-10), §3.4.1.1/4,
HID2 Table 2-6 and the sync entry (12-239). ICFI clears instruction-cache
validity/replacement state when ICE is enabled and self-clears. DCFI also
self-clears when DCE is enabled and can discard modified data without a
writeback. ABE governs sync broadcast. Local ordered memory dependencies
remain significant; sync is not a blanket writeback of every dirty line.
The paired enable changes require instruction-cache invalidation before the
corresponding graphics instructions.

**PROVEN-BINARY + OBSERVED-HLE:** old input `0011C464` includes **DCFI `400`**
with DCE enabled, in addition to ICE. The full HID0 write at `803725FC`
would reissue that data-cache command. The pinned interpreter implementation
only implements ICFI and leaves DCFI set; it also ignores sync. This conflicts
with the documented completed physical readback and can conceal lost dirty
stack data on hardware. The unchanged historical probe continues to report
the original **request-only** stop. The new native backend **rejects** this
input; it never silently masks DCFI or calls the old capture retail evidence.

Read-only source: `Interpreter_SystemRegisters.cpp` SHA-256
`698f153ad3a9e457c7d51f9a84358b86140c29b879f35dc39fe70412f23f8556`,
HID0 case; `Interpreter_LoadStore.cpp` SHA-256
`dbfc463df0e83aa989e7728bec097d3d95f05ec2e0fa2a409291e3b985234553`,
sync body. Neither source nor its original executable was changed.

## 3. Portable replacement: complete the owned effects

**STRONG / bounded native equivalence:** `RunImmutableNativeBootPrefix()`
uses a private completion object created only from an entry run. No caller
can inject a post-sync state, completion=true flag, callback or interpreter
answer. The only mutable memory during this finite interval is owned bytes;
the DOL view and compiled C++ bodies have no code-write interface. Every
ordinary prior store is applied and read back before the barrier is accepted.
The corresponding accepted HID2 controls are retained. ICE=1 ICFI clears its
command bit in a separately derived HID0 word, while r3 retains the issued
operand. Native paired operations use the validated C++ implementation and
cannot retain a stale pre-enable guest instruction decode. This preserves
the instruction-visibility consequence; guest tags/PLRU are not emulated.

The scope has **no inherited native device queue or external observer**.
ABE=0 and no port/device write occurs before sync. Therefore there is no
external completion service to invent; ordinary same-owner C++ sequencing
and the verified committed memory preserve the ordering needed by all
reached consumers. An arbitrary host fence would not establish these facts.
The barrier remains an explicit completion boundary, and the consumer checks
require the produced paired enable/visibility/GQR state.

Admitted HID0 bits are the documented DPM/NHR, ICE/DCE, DCFA/BTIC/BHT subset
`0011C064`, with both cache enables required. Other bits, incoming ICFI/DCFI,
ABE, locks and unvalidated modes decline. HID2 admits only LSQE/WPE/PSE;
locked cache, DMA queue/error/reserved state declines. Exceptional MSR modes
(power, privilege, external interrupts, trace, FP traps, little endian) and
unsupported FP/source states decline. Cache-disabled optional helper/logger
paths also decline. Neither pending physical pre-entry work nor a damaged
mapping is guessed to be complete: those retail paths remain **UNKNOWN**.

The C++ core uses semantic operations over owned arrays/status, not a PPC
interpreter, hardware cache simulator, Dolphin bridge or host guest-memory
execution. Diagnostic registers/addresses are retained for proof. Later
subsystems cannot yet consume these diagnostic representations as a full
native engine initialization contract.

## 4. Behavioral and chain validation

Three **controlled initial-state experiments** used the same separate
read-only-export oracle from checkpoint 36. The existing GDB HID0 writer
sets an explicit *input before any DOL instruction runs*, not a register
at a failing branch. The unmodified original entry snapshot and readback of
the controlled entry are retained. The finite/FPSCR BSS inputs are likewise
set before the entry. No instruction/DOL byte is patched. These experiments
test the admitted profile; they do **not** establish retail initial values
or repair the oracle's DCFI behavior.

| Capture in ignored build/ | Initial HID0 | Entry source bytes `805F1F30..3F` | SHA-256 capture |
|---|---|---|---|
| `sync-state-zero-37.json` | `0011C064` | all zero, read from entry | `a5e53306164bfb24480c654021f59c0b4811b2bca13fa8aae3d950d7e0244543` |
| `sync-state-finite-37.json` | `0000C000` | `400A0000000000003FC00000C0100000` | `725740dc64925bb413d1aae03896601908d5658b2ebdcb9f020db688a84e75f1` |
| `sync-state-fpscr-37.json` | `0011C060` | `3FF00000010000808000000000000000` | `37f8d5e485bc67eb6533d547cecc2f60e39a425ed53d872e4ecfed33ab87fc37` |

Each native run receives **entry inputs only**. No GPR/PC/LR at an intermediate
checkpoint, expected branch result, expected output or completion token is
serialized. Source bytes are checked unchanged across the entire reference
interval. Seventeen matched checkpoints per run compare **1,921 state fields
and 148 known stack bytes**: **5,763 fields + 444 bytes** total. Unwritten
bytes remain unknown. Cache frame stores overwrite the same earlier saved-LR
and backchain addresses, and the native validity-tracked snapshots reflect
those aliases. Both later HID0 readers return the derived input-specific
readback, with exact branch masks, CR0 and LR. The scalar/FPSCR variant derives
`61000080` at mtfsf and preserves the signed-zero paired lanes.

Native executable SHA-256 at validation:
`0bc6b998143f820ca9d8cdb8d6b3a1c430e792747b3c75b8af89ff0175bcb4b9`.
Oracle executable and SYS-only disc remain those pinned in checkpoint 36;
the manifest is embedded in each capture. The HLE state deltas validate
architectural consumers; the manual and closed native effect inventory
justify the narrower completion replacement. HLE sync execution itself
cannot prove physical ordering.

The final probe also reads back all four committed cache-frame destinations
from its owned byte arrays, in original store order. All twelve frame-word
comparisons across the three runs match reference memory, including the new
backchain at `8060C5E0` outside the earlier paired window. An unchanged stock
oracle repeat agrees with the export build on **all 380 shared JSON fields
at 20 checkpoints**, including GPR packets, PS0 arrays and memory windows.
Stock capture SHA-256:
`6a3bc2d8200c9ca911aeaff7de463368b415419c3beca717ca55ea925e2ab0f5`.

Adversarial C++ cases also begin at entry: PSE/LSQE initially absent, eight
nonzero incoming GQRs, nonzero distinct incoming FPRs, varied CR/XER.SO/CTR,
different passive HID0 bits, signed zero, repeated runs, overlapping later
stack stores, all 166 word mutations, and unsupported-mode declines. The
full MSVC Release gate passed **49/49 CTest suites** after this implementation.
The legacy request-only gates preserve the old synthetic stops/inputs.
The mandatory standalone gameplay script also passed **27/27** and printed
`RESULT: Dark mission cleared -> next stage index 6 (stg0200)`.

Reproduce (all outputs remain in ignored build/; supply your read-only DOL):

```powershell
python -B reverse/boot/tools/capture_boot_machine_state.py `
  build/readonly-boot-oracle/Dolphin.exe build/boot-oracle-startup.iso `
  build/sync-oracle-new build/sync-state-new.json `
  --initial-hid0 0011C064
python -B reverse/boot/tools/validate_native_prefix.py <PAL-main.dol> `
  build/reverse/boot/Release/shadow_boot_native_prefix.exe `
  build/sync-state-new.json build/sync-validation
build/reverse/boot/Release/shadow_boot_native_prefix.exe <PAL-main.dol> `
  build/sync-validation/sync-state-new.entry.txt
ctest --test-dir build -C Release --output-on-failure
```

## 5. Exact remaining boundary

**UNKNOWN:** `80372894` calls `80370AFC`, whose first instruction reads live
L2CR. No value is provided by this native backend, so it stops before the
call. L2 enabled/status/configuration, both completion polls, physical error
and logger branches need their own producer/consumer completion contract.
Earlier disabled-cache, ABE=1, DCFI and retail pre-entry paths remain separate
unresolved profiles. CRT, constructors, OS timebase, game initialization,
runtime and first-frame/pixel gates remain required.

Recognizer bookkeeping also had a genuine bug: it considered numerical PC
increase necessary for progress, even though the proven FPR call descends in
address. Advancement now requires matching previous-stop evidence and a
different stop, with separate profile names; a focused lower-address/stale
proof regression prevents recurrence. A scan never advances automatically.
The reused database now has 277 regions (2 bounded VALIDATED, 7
STRONGLY_SUPPORTED, 268 UNKNOWN), 123 learned patterns and 971 existing
structural hits. No constructor/family is promoted. The new frontier's raw
four words are `4BFFE269 54600000 28000000 40820058`; the last branch targets
`803728F8`. Its condition remains symbolic after the untraced callee. The
recognizer proposes capture at `80370AFC` and return `80372898`, with live
L2CR/register/memory state, rather than inferring a zero return.

# Finite native entry-to-first-TB work, research 42

## Scope and admission

The finite native producer is closed for every path that the unchanged
`RunImmutableNativeBi2Prefix` proves through **before `80379628`**. It consumes
`NativeBi2Inputs`, executes the existing C++ semantic prefix, then derives work
from immutable CRT descriptor sizes and the original input-selected L2/BI2
paths. It consumes no frontier cycle count, captured PC list, instruction
interpreter, per-read TB result or host timestamp.

This is an explicit **source-conditioned research work model**. The matching
private continuous interpreter assigns operation costs; its cost table does
not establish physical Gekko latency. `BootElapsedWork::connected_clock_admitted`
is false. Clock admission additionally requires the original source scheduling
mode, HLE hook decisions, complete epoch/event/exception ownership and the
event consequences proved by the coordinator's independent source audit.

The C++ implementation calls the unchanged native semantic producer and
requires its final PC to equal `80379628`. It derives neither successful
completion nor known bytes from a caller acknowledgment. The result retains
the actual `NativeBi2Run` in `.prefix`, a semantic `.ledger`,
`.original_operations`, `.source_operation_work`, and `.stop_pc`.

Files:

- `include/shadow/boot/BootElapsedWork.hpp`
- `src/BootElapsedWork.cpp`
- `tests/test_boot_elapsed_work.cpp`
- `tools/agent_native_work_42.py`

## Authorities and raw gates

Original read-only PAL DOL, supplied explicitly with `--dol`, SHA256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.

Matching private source `Source/Core/Core/PowerPC/PPCTables.cpp`, supplied
explicitly with `--source-tables`, SHA256
`4814b6c63088c0227eadf2fefa4f32fe2109afde7319b88697b35a7b67dcf771`.

The independent Python proof checks that source identity before decoding static
metadata. It does not execute guest instructions. The C++ implementation
contains explicit per-unit constants and compares exact original words; it has
no runtime PPCTables dependency or opcode decoder.

All **65 unit hashes** are preserved identically in Python and C++. Each hash is
SHA256 of `addressBE32 || originalWordBE32` for the unit's declared reached
operations. There are **488 declared word occurrences / 482 unique code words**.
The C++ read-only gate inventory also includes 41 CRT descriptor words and the
first unconsumed `7C6D42E6` word: **524 exact gates**. The unchanged native prefix
additionally gates relevant untaken variants and enforces its existing owners.

Most source operation costs are1; `mtspr` including `mtlr/mtctr` costs2;
`sync` and `mtfsf` cost3. These are explicitly private-source annotations.
A `mtspr` metadata end-block flag is not an interpreter retirement decision.
Actual original branches and `mtmsr` retire the block; ordinary LR/CTR/HID/GQR
writes do not.

## Derivation before comparison to outputs

No candidate 494/610 residual was passed to the producer. The finite unit sums
give the following decomposition for ordinary debug and offset0:

| Semantic section | Operations, L2 enabled | Source work, L2 enabled | Additional disabled operations | Additional disabled work |
| --- | ---: | ---: | ---: | ---: |
| Entry/register/hardware/L2/handler through hardware return | 243 | 261 | 98 | 116 |
| CRT walker and descriptor overhead, excluding fills | 167 | 168 | 0 | 0 |
| Three zero leaves and wrappers | 153633 | 153636 | 0 | 0 |
| BI2, metadata, OS guard/frame, clock frame and EE leaf | 65 | 65 | 0 | 0 |
| Total before first TB | 154108 | 154130 | 98 | 116 |

Thus the non-fill remainder is `261+168+65 = 494`; disabled L2 adds116,
giving610. The totals are outputs of the semantic work derivation.

### CRT loops

The ten immutable source==destination copy descriptors each execute the
ten-operation walker path. They skip both copy and cache-maintenance helpers.
The terminator and descriptor overhead are separate finite units.

Every immutable zero descriptor has an aligned destination, zero fill,
size>=32 and size divisible by4. For size `S`, the zero leaf executes:

`19 + 10*(S>>5) + 3*((S>>2)&7)` operations/work.

The 19 fixed reached operations skip alignment bytes, nonzero-byte replication
and final byte stores. Every 32-byte group has ten operations; every residual
four-byte word has three. Each wrapper has12 operations /13 work because its
`mtlr` costs2.

| Destination | Size | Groups | Residual words | Leaf work |
| --- | ---: | ---: | ---: | ---: |
| `8056FE00` | `74700` /476928 | 14904 | 0 | 149059 |
| `805EF020` | `375C` /14172 | 442 | 7 | 4460 |
| `805FC540` | `AC` /172 | 5 | 3 | 78 |

Leaf work is153597; three wrappers add39, giving153636.

### Disabled L2

The original admitted synchronous owner produces one clear visit at each
busy check. No guessed polling duration is admitted. The caller
`803728A4..803728F4` contributes29 work (21 operations with four `sync`
cost adjustments). Its MSR/read/write/accessor/logger calls contribute28.
The invalidation helper body contributes40; its five reads and three writes
contribute19. Hence `29+28+40+19=116` extra work /98 extra operations.

Incoming I/IP, test or reserved L2 state declines. This finite synchronous
owner is distinct from a physical console invalidation latency.

### Generic BI2 branches

The owned 8192-byte BI2 input supplies its original debug word at+12, offset
at+8, and, for nonzero offset, count before relocation writes. The producer
uses those bytes, including aliases, rather than separately supplied labels.

Relative to ordinary debug and offset0:

| Actual owned input path | Additional operations | Additional source work |
| --- | ---: | ---: |
| Debug4 leaf and call/return | 6 | 6 |
| Nonzero offset, count0 | 4 | 4 |
| Nonzero offset, countN>0 | `9+5*N` | `10+5*N` |

The count distinction includes `mtctr` costing2. Relocation has exactly five
operations/work per item. Original loaded count is retained across same-owner
writes; opaque values are relocated modulo32 without becoming callable
pointers. Debug2/3 transfers to the unresolved context call and declines.

With `D=(initial L2 E==0)`, `B=(original debug==4)`, and `A` from the table:

`W = 153636 + 494 + 116*D + 6*B + A`.

The scalar expression is a separately written falsification check of the unit
ledger; C++ produces work through the individual finite semantic units.

## Input conditions and earliest declines

The unchanged native prefix must prove its existing input contract. Python
independently declines exceptional `MSR & 4CF01`, missing ICE/DCE or unsupported
HID0 bits, DMA/locked/reserved HID2, invalid L2 state, reserved FPSCR bit20,
unknown FPR source and unsupported paired/scalar representations. FPR source
is the existing16-byte owner at `805F1F30`, and the fixed startup produces its
SDA address and GQR0. Input FPR values and CR/XER/CTR remain native state inputs;
work counts do not replace their semantic validation.

| Missing/unproved dependency | Earliest consumer/stop |
| --- | --- |
| Incoming mode or hardware-owner contract | Entry/hardware preparation `80003400` |
| FPR source/representation | `80370CF8` / scalar `80370D7C` |
| L2 pending/invalidate/test/reserved history | Before L2 call `80372894` |
| Old handler-slot word | Load `803733B4` |
| BI2 pointer | Read `80003188` |
| Null BI2 fallback arena | `800031A4` |
| Missing full BI2 blob | `80003194` |
| Debug2/3 context effects | Call `800031F4`, target `8039F8E0` |
| Nonzero offset unknown extent/wrap | Before `8000321C` |
| Relocation source count/extent | Before `80003234` |
| Live TB producer | **Before `80379628`** |

BI2 must be word-aligned, fully owned, within MEM1 and disjoint from DOL,
produced CRT ranges, low-memory and stack owners. An unexplained state cannot
satisfy the first-clock completion requirement.

## Independent passive-output falsification

The producer finishes before opening any reference stream. The independent
comparison then checks the complete hook/enter/exit triples through the first
TB hook, exact DOL words, actual source costs, no active HLE hook/replacement,
pendingexceptions0, overclock inverse1.0, final caller block retirement, and
the full PC multiplicities predicted by semantic units.

| Passive42d input | Predicted operations | Predicted work | Observed source elapsed | Unique predicted/reached PCs |
| --- | ---: | ---: | ---: | ---: |
| L2 disabled, zero profile | 154206 | 154246 | 154246 | 460 |
| L2 disabled, host pause profile | 154206 | 154246 | 154246 | 460 |
| L2 enabled, `C0480000` | 154108 | 154130 | 154130 | 398 |

Zero/paused stream SHA256:
`030d18eb37836fc2b76cdf1cc1c92a8565b6a3be2ef5c9ae59806baf68f68a8b`.
Enabled stream:
`8959e4d60567d92cdbe6125db5d797292c3e7b8893a07ad464e7847a91714043`.

These captures use original **continuous** scheduling and no mid-chain clock
writes. At the first-TB boundary the final original `80379668` branch has
retired its block, so all prefix work is reflected in observed GetTicks.
The four unretired operations at the later `80373AC4` endpoint, identified by
the earlier full-stream audit, are not part of this producer's endpoint.

The entry capture GetTicks is2676493 (`28D70D`), whereas the independent raw
preentry work reported by the binary agent is2656493. Source inspection confirms
that fresh CoreTiming has factor0, initializes downcount with that factor before
refreshing it, and leaves slice_length20000/downcount0. First Advance commits
20000; subsequent boot SingleStep resets expose one cycle per step. Thus
`20000 + N`, with N produced from original apploader work, has a separate real
scheduler/phase derivation. It is never inferred by fitting an entry getter.

Final42d event stream identities, now including the expanded callback records:

| Profile | Event TSV SHA256 |
| --- | --- |
| zero | `a026cd7fbddf1b24a444b35bffcbf170ecbc2130d7eea1558d7e6d81f5eedd8f` |
| paused | `b5a8a0c8cfde10f1100b7fd0d8b0bc37dd1ec3674d16eb9db7b10ccf52ae3bcb` |
| enabled | `631b2f3ffa66f1dbfccba117a8f22e53fa8f54a4419533903e6475c2bec0a1f3` |

The42d instruction streams retain the same hashes as42c. Expanded callback
evidence belongs to the independent event-owner audit; identical instruction
streams alone cannot establish absence of external device consequences.

## Perturbation/falsification and validation status

Independent Python command:

```text
python reverse/boot/tools/agent_native_work_42.py --dol <original-PAL-main.dol> --source-tables <matching-source>/Source/Core/Core/PowerPC/PPCTables.cpp --compact --self-test --capture build/passive-timing-zero-42d.json --capture build/passive-timing-paused-42d.json --capture build/passive-timing-enabled-42d.json
```

It passes141 raw/input mutation declines,200 closed-form input-selected paths
and one same-owner BI2 metadata/count alias check, before any supplied output
total. Its three passive comparisons then match exact first-clock operation
multiplicities and work/elapsed outputs. Thirty-six post-prediction mutations
(12 per reference) reject altered totals, unit counts, frontier/admission
claims, entry cycle/exception metadata and step/forced timing controls.

The C++ test covers25 semantic paths,524 exact raw-word mutations and33 input
declines (557 total rejections). These include maximal admitted count1983,
L2 passive bits, unsigned debug values, array addition wrapping, aliases,
unknown owners, extents, exceptional modes and unsupported FP sources.
The coordinator registered/built the executable. Independent execution of
`build/reverse/boot/Release/shadow_boot_elapsed_work_tests.exe` against the
read-only original DOL passes all25 paths and557 rejections.

Mandatory stages at this independent handoff:

1. Reference capture: complete unchanged passive42d first-clock evidence passes
   for ordinary debug0/offset0, both L2 paths and host pause.
2. Semantic reconstruction: finite Python and portable C++ work producer closed
   for already-admitted native semantic paths, with source-conditioned costs.
3. Perturbation/falsification: independent Python probes pass; C++ focused tests
   compile and pass in actual execution. Original continuous debug4/nonzero-array
   evidence is additionally required before promoting generic elapsed-clock
   conditions, because new reached PCs need original HLE/block/event checks.
4. Checkpoint parity and full repository/gameplay regression: remain mandatory
   coordinator gates before any connected frontier promotion.

Unknown preentry SI future-channel state may remain unknown until its actual
consumer. Its non-consumption in this bounded prefix does not establish a
default channel state or authorize a later consumer. No physical/retail clock,
arbitrary host timing or complete downstream event state is claimed here.

## Read-only review of produced-work clock research

The coordinator's `research-produced-work` CLI mode requires a dash for the
frontier argument and epoch-cycle origin0. It reads the original apploader,
boot, BI2 and DOL header artifacts, executes their finite work producer and
this native producer, then derives `20000 + apploaderN + prefixW`. It rejects
the unwitnessed debug/array timing paths. It remains explicitly labelled
`NO_NATIVE_CLOCK_PROVIDER`; event ownership has not been promoted by the work
derivation.

Read-only inspection of the continuous projection confirms the inspected
source phase: its three sampler getters share one cycle value, comparison
branch `80379638` retires5 work, return `8037963C` retires1, `803796A4` is the
two-cost LR write, and the actual branch/mtmsr boundaries retire the remaining
finite units. Memory/global stores retain their original order. The first
unprovided pointer load `80373AC4` is not executed.

The four operations `80373AB4/AB8/ABC/AC0` have pending work at that final
boundary. The terminal projection drops the local pending accumulator, which
matches the observed GDB endpoint's block-discard behavior. A future
uninterrupted continuation must persist those four units or explicitly prove
the debugger-resume contract; architectural parity at the stop cannot choose
between those timing contracts.

An early-unknown CLI issue was reported separately: when unknown handler input
stops the nested CRT run at `803733B4`, the BI2 checkpoint list is empty. The
old final-PC selection in the general clock CLI used `.back()` on that empty
list. Produced-work mode already declines before it. General research modes
must select an actual earlier stop or report a clear decline.

## Exact semantic unit inventory

First/last columns identify the declared word boundaries; some units contain
explicit disjoint subsets. The actual word lists/hashes are preserved in both
implementations. These are static proof annotations, never executed PC steps.

| Semantic unit | First word | Last word | Gate words | Source work/unit |
| --- | --- | --- | ---: | ---: |
| `entry_register_call` | `80003154` | `80003154` | 1 | 1 |
| `register_seed` | `800032B0` | `8000333C` | 36 | 36 |
| `hardware_call` | `80003158` | `80003158` | 1 | 1 |
| `hardware_prepare` | `80003400` | `80003410` | 5 | 5 |
| `paired_frame` | `80371714` | `80371720` | 4 | 4 |
| `hid2_read` | `80370BA8` | `80370BAC` | 2 | 2 |
| `paired_enable_call` | `80371724` | `80371728` | 2 | 2 |
| `hid2_write` | `80370BB0` | `80370BB4` | 2 | 3 |
| `icfi_call` | `8037172C` | `8037172C` | 1 | 1 |
| `icfi_leaf` | `803725F4` | `80372600` | 4 | 5 |
| `sync_gqr_return` | `80371730` | `80371764` | 14 | 25 |
| `fpr_call` | `80003414` | `80003414` | 1 | 1 |
| `fpr_seed` | `80370CDC` | `80370E00` | 74 | 76 |
| `cache_call` | `80003418` | `80003418` | 1 | 1 |
| `cache_first_check` | `80372838` | `80372860` | 11 | 11 |
| `hid0_read` | `80370AEC` | `80370AF0` | 2 | 2 |
| `cache_second_check` | `80372874` | `80372880` | 4 | 4 |
| `l2_check` | `80372894` | `803728A0` | 4 | 4 |
| `l2_read` | `80370AFC` | `80370B00` | 2 | 2 |
| `l2_write` | `80370B04` | `80370B08` | 2 | 3 |
| `msr_read` | `80370ADC` | `80370AE0` | 2 | 2 |
| `msr_write` | `80370AE4` | `80370AE8` | 2 | 2 |
| `l2_disabled_caller` | `803728A4` | `803728F4` | 21 | 29 |
| `l2_invalidate_body` | `80372640` | `803726D4` | 35 | 40 |
| `logger` | `80370C8C` | `80370CD8` | 12 | 12 |
| `handler_arguments` | `803728F8` | `80372900` | 3 | 3 |
| `handler_call` | `80372904` | `80372904` | 1 | 1 |
| `handler_frame` | `80373378` | `8037339C` | 10 | 10 |
| `disable_ee` | `8037611C` | `8037612C` | 5 | 5 |
| `handler_select` | `803733A0` | `803733C4` | 10 | 10 |
| `handler_restore_call` | `80373564` | `80373568` | 2 | 2 |
| `restore_ee_zero` | `80376144` | `80376164` | 7 | 7 |
| `handler_return` | `8037356C` | `8037358C` | 9 | 10 |
| `handler_logger_call` | `80372908` | `80372910` | 3 | 3 |
| `cache_return` | `80372914` | `80372928` | 6 | 7 |
| `hardware_return` | `8000341C` | `80003420` | 2 | 3 |
| `sentinel_walker_call` | `8000315C` | `8000316C` | 5 | 5 |
| `descriptor_frame` | `80003340` | `80003368` | 11 | 11 |
| `identity_copy_descriptor` | `8000336C` | `800033A8` | 10 | 10 |
| `copy_terminator` | `8000336C` | `80003374` | 3 | 3 |
| `zero_table_prepare` | `800033AC` | `800033BC` | 5 | 5 |
| `zero_descriptor` | `800033C0` | `800033E0` | 9 | 9 |
| `zero_wrapper` | `8000540C` | `80005438` | 12 | 13 |
| `zero_leaf_fixed` | `8000543C` | `800054E0` | 19 | 19 |
| `zero_group` | `80005498` | `800054BC` | 10 | 10 |
| `zero_remaining_word` | `800054C8` | `800054D0` | 3 | 3 |
| `zero_terminator` | `800033C0` | `800033C8` | 3 | 3 |
| `descriptor_return` | `800033E4` | `800033FC` | 7 | 8 |
| `crt_entry_tail` | `80003170` | `80003184` | 6 | 6 |
| `bi2_pointer_nonzero` | `80003188` | `80003198` | 5 | 5 |
| `bi2_debug_ordinary` | `800031BC` | `800031D8` | 8 | 8 |
| `bi2_debug4_call_return` | `800031DC` | `800031E4` | 3 | 3 |
| `bi2_debug4_leaf` | `80003140` | `80003148` | 3 | 3 |
| `bi2_offset_check` | `800031F8` | `80003214` | 8 | 8 |
| `bi2_no_array` | `80003258` | `8000325C` | 2 | 2 |
| `bi2_count_check` | `80003218` | `80003224` | 4 | 4 |
| `bi2_array_prepare` | `80003228` | `8000322C` | 2 | 3 |
| `bi2_relocate_item` | `80003230` | `80003240` | 5 | 5 |
| `bi2_array_publish` | `80003244` | `80003254` | 5 | 5 |
| `metadata_call` | `80003260` | `80003260` | 1 | 1 |
| `metadata_leaf` | `80370BF0` | `80370C14` | 10 | 10 |
| `os_call` | `80003264` | `80003264` | 1 | 1 |
| `os_first_guard_frame` | `80370E68` | `80370EA4` | 16 | 16 |
| `clock_frame_call_disable` | `80379648` | `80379660` | 7 | 7 |
| `clock_sample_call` | `80379664` | `80379668` | 2 | 2 |

# Independent binary audit — selector1 handler installation and hardware return

Target: PAL GUPP8P `main.dol`, SHA256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
This audit rereads original bytes independently of the proposed native module.
`PROVEN` below means binary structure and conditional architectural effects;
it does not promote a static result to connected execution parity.

## Raw evidence and reproducibility

`tools/agent_handler_binary.py <original main.dol>` validates the hash, reads
the original section headers, prints file offsets, BE words, opcode/register/
mask fields and independent Capstone5.0.7 plus project-decoder interpretations.
Its branch target computation sign extends the original BD/LI fields and
checks the project decoder; register names are not used to locate code. It
also scans every text word for direct installer callers and candidate low
address immediates. These candidates are deliberately not classified as
proven cross references without their producer slice.

**PROVEN ranges, inclusive:**

| Range | Words | Required behavior |
|---|---:|---|
| `80372904..80372928` | 10 | Direct installer call, final leaf logger, restore cache frame, return. |
| `80373378..803733C4` | 20 | Save six stack words; disable EE; form slot; load old word; compare selector; store new word; take selector1 edge. |
| `80373564..8037358C` | 11 | Restore EE; copy old word to r3; load actual saved registers/LR; return. |
| `8037611C..8037612C` | 5 | Read full MSR; clear only numeric `8000`; write MSR; return prior EE as0/1. |
| `80376144..80376164` | 9 | Signed compare of EE argument; read full current MSR; set/clear EE; write MSR; return previous current EE as0/1. |
| `8000341C..80003420` | 2 | Restore LR from live r31, aligned return. |
| `8000315C..8000316C` | 5 | Produce `FFFFFFFF`; allocate8bytes; write two sentinels; next direct call toCRT. |

These62 words include the next unexecuted call at`8000316C`. The default
script additionally prints`80003170`, outside this selected execution prefix.
The20-word logger`80370C8C..80370CD8` is unchanged from checkpoint38 and must
remain pinned. All handler/caller/helper rows above except the final wrapper
and entry sentinel already have detailed raw-field rows in
`BINARY_CACHE_HANDLER.md`; rereading them produced agreement, with only
the equivalent `crxor6,6,6`/Capstone`crclr cr1eq` alias.

The text1 offset rule, independently recovered from the DOL header, is
`file=2600+VA-80008D40`; text0 is `file=100+VA-80003100`.

| VA | File offset | BE bytes/word | Independent decode |
|---|---:|---|---|
| `8000341C` | `0000041C` | `7F E8 03 A6` / `7FE803A6` | `mtlr r31`, SPR8, XO467. |
| `80003420` | `00000420` | `4E 80 00 20` / `4E800020` | `bclr20,0,0`, XO16; PC=`LR&FFFFFFFC`. |
| `8000315C` | `0000015C` | `38 00 FF FF` / `3800FFFF` | `addi r0,0,-1`; RA0 means literal zero. |
| `80003160` | `00000160` | `94 21 FF F8` / `9421FFF8` | `stwu r1,-8(r1)`; store old SP before updating r1. |
| `80003164` | `00000164` | `90 01 00 04` / `90010004` | `stw r0,4(r1)`. |
| `80003168` | `00000168` | `90 01 00 00` / `90010000` | `stw r0,0(r1)`; overwrites the preceding backchain. |
| `8000316C` | `0000016C` | `48 00 01 D5` / `480001D5` | `bl80003340`, displacement`1D4`, AA0/LK1. |

## CFG and exact data flow

**PROVEN:** only one text direct call to installer`80373378` exists, at
`80372904`. The cache routine containing it has the early wrapper caller
`80003418` and a later OS caller`80371014`; each invocation is distinct.
The selected early path has no virtual, constructor or indirect call.

```text
80372904 bl ->80373378 (LR=80372908)
  prologue ->8037339C bl ->8037611C
  return803733A0 -> slot readB4 -> compareB8 -> slot writeC0
  selector1:803733C4 bne ->80373564
  80373568 bl ->80376144
     EE0:7614C ->76158 ->7615C
     EE1:7614C ->76150 ->76154 ->7615C
  return7356C -> old-slot copy -> actual stack restores ->blr72908
72908 message/CR6clear ->72910 bl ->70C8C
  70C90 bne CR1 ->70CB4 (skip all8FPR stores)
  r3..r10 stores ->restoreSP ->blr72914
72914 savedLR/r31/r30 loads ->restoreSP ->blr8000341C
8000341C mtlr r31 ->80003420 blr8000315C
8000315C..68 sentinels ->STOP before8000316C bl80003340
```

Let `T=8060C5E0` be live main SP at checkpoint38, `M` its live full MSR,
`E=(M>>15)&1`, and `P` the explicitly known BE32 bytes at`80586CB4`.
Let `A28..A31` be live incoming installer register values. The current
connected producer establishes r3=1 and r4=`803726D8`; r31=`80561380`.
**PROVEN:** A28=A29=0 because the entry helper explicitly executes
`80003314:3B800000` (`li r28,0`) and `80003318:3BA00000`
(`li r29,0`); no selected intervening instruction replaces them. These
values do not depend on inherited registers or a C++ zero-initialization
assumption. All36helper words at`800032B0..8000333C` were reread during
the adversarial native review.

**PROVEN instruction effects:**

* `80373378..98`: r0 receives actual call LR. The prologue commits LR,
  backchain and r31/r30/r29/r28 **in that order**; r29 receives selector1
  and r28 receives handler`803726D8` after saving their incoming values.
* `8037611C..2C`: r3=M, r4=`M&FFFF7FFF`, MSR=r4, then
  r3=`rotl32(M,17)&1`=E. Masks and rotates do not change CR/XER.
* `803733A0..B4`: r4=`80580000`, r5=`rotl32(1,2)&0003FFFC`=4,
  r0=`80586CB0`, r6=`1&FFFF`=1, r4=r0+r5=`80586CB4`, r30=P.
* `803733B8`: unsigned compare1 against16 writes CR0.LT and live XER.SO;
  other CR fields are preserved. `803733BC` sets r29=E.
  `803733C0` commits the supplied handler to the calculated address.
  `803733C4` uses CR0.EQ, so selector1 takes the non16 edge.
* `80373564..68`: r3=E, call LR=`8037356C`.
  `80376144` compares E **signed** against zero, replacing CR0 with EQ
  whenE0 or GT whenE1 and copying XER.SO. `80376148` reads current MSR
  afresh into r4. With no asynchronous writer/exception in this bounded
  interval, r4=`M&FFFF7FFF`; r5 becomes M through either exact branch,
  MSR becomes r5, and r3 receives previous current EE=0.
* `8037356C` immediately replaces r3 with P. The epilogue restores all
  four saved nonvolatile registers and actual LR bytes, SP=T, LR=`80372908`.
  Neither P nor the new handler is dereferenced or called in this slice.
* `80372908`: r3=`r31+1FC`=`8056157C`; P is no longer live in r3.
  `8037290C` clears only numeric CR bit`02000000`; `80372910` sets
  LR=`80372914`. The leaf logger commits the backchain and r3..r10.
  Its skipped FPR slots retain all previous byte contents and validity.
* `80372914..28`: r0 reads `[T+14]`, r31 reads`[T+C]`, r30 reads
  `[T+8]`, SP becomes T+10=`8060C5F0`; `mtlr` installs that loaded r0
  and `blr` clears the bottom2bits in the destination. The proven main
  frame producer has `[T+14]=8000341C`, `[T+C]=8000315C`.
* `8000341C..20` subsequently restores LR from the **restored live r31**,
  producing LR=`8000315C` and PC=`8000315C`.
* `8000315C..68` makes r0=`FFFFFFFF`, SP=`8060C5E8`; two sentinels
  remain at`8060C5E8` and`8060C5EC` before the next direct CRT call.

**PROVEN unchanged state:** r2/r13 SDA bases, r7..r27, CTR, XER,
all FPR paired lanes, FPSCR, HID0/HID2/GQRs and L2CR are unchanged by
this new selected slice. r28/r29 are restored; r30/r31 additionally restore
their main-frame values. Final r4 is the fresh disabled-MSR read, r5 is
the restored MSR, r6 remains1. Final CR0 comes from EE restoration,
**not** the selector comparison; CR1.EQ is cleared by the caller.

## Ordered memory effects and aliases

All writes below are BE32, not host-native stores. The old bytes are only
known when an earlier verified writer or explicit entry evidence says so.

| Order / PC | Address | Written value / next reader |
|---|---|---|
| 1 `8037337C` | `8060C5E4` | `80372908`; helper return reads at`80373570`. Overwrites L2 saved-return word. |
| 2 `80373380` | `8060C5B0` | T=`8060C5E0`; backchain. |
| 3 `80373384` | `8060C5DC` | A31=`80561380`; restored at`80373574`. |
| 4 `80373388` | `8060C5D8` | A30; restored at`80373578`. L2-off A30=M, L2-on retains preceding producer's value. |
| 5 `8037338C` | `8060C5D4` | A29; restored at`8037357C`. |
| 6 `80373394` | `8060C5D0` | A28; restored at`80373580`. **Overwrites the L2 helper backchain.** |
| 7 `803733C0` | `80586CB4` | `803726D8`, after actual old read`803733B4`. |
| 8 `80370C8C` | `8060C570` | T, caller logger backchain. |
| 9–16 `80370CB4..D0` | `8060C578..594` | r3..r10 in increasing order; first4are`8056157C`, disabled freshMSR, restoredMSR,1. |
| 17 `80003160` | `8060C5E8` | `8060C5F0`; entry backchain, immediately superseded. |
| 18 `80003164` | `8060C5EC` | `FFFFFFFF`; overwrites old saved mainr31 only after it has been loaded. |
| 19 `80003168` | `8060C5E8` | `FFFFFFFF`; overwrites entry backchain. |

The skipped logger f8 slot is`8060C5D0..D7`. Its first4bytes now contain
the **incoming r28 spill**, and its next4bytes the r29 spill. A native or
diff validator that preserves the old L2 backchain, clears this slot,
or marks these bytes unknown is wrong. The saved main LR at`8060C5F4`
remains`8000341C`; it must not be replaced by the later wrapperLR.

## Slot producer, consumers and lifetime

**PROVEN:** populated DOL sections do not contain`80586CB4`. It lies in
the first BSS gap. No static DOL word supplies its live pre-entry value.
The already audited apploader's conditional BSS fill includes it;
completed handoff without later overlapping external writes would make it
zero. That conditional chain is **not** evidence of a retail live zero,
nor a native entry default. A native extension must receive actual known
entry bytes and retain them unchanged until the first read; all selected
earlier native writes are stack writes and do not alias the slot.

**PROVEN later clear:** CRT descriptor`800055C8..CC` gives first zero
range`8056FE00`+`74700`=`805E4500`. The later CRT fill erases the installed
slot. Its zero cannot justify the earlier old-slot read. OS caller
`80371014` can later reinstall it through the same cache routine. This is
write→clear→conditional rewrite, not a permanent preCRT registration.

**PROVEN conditional consumer:** `80373594/803735B8` form tablebase
`80586CB0` in r30. For dispatcher selector1, `803736D0` word`570015BA`
rotates selector by2 and masks`000003FC`; `803736D4` word`7EDE0214`
forms slot, `803736D8` word`80160000` reads it, `803736DC/E0` null tests.
On nonzero path, after the intervening call`803736E4`, `803736E8`
word`81960000` **freshly reloads** the pointer; `803736F8` word`7D8803A6`
writes LR; `80373704` word`4E800021` invokes through LR. Dispatcher input,
context, intervening call effects and asynchronous exception entry are
outside this finite boot path. The installed target`803726D8..80372834`
contains HID2 reads, context+19C loads, multiple diagnostic calls and
conditional context/halt behavior. Do not classify it as a no-effect stub.

Other resolved base materializations read slot15 at`80376A10` (table+3C)
and slot16 at`80378B14` (table+40), so those are not slot1 consumers.
The low-immediate scan is not a proof that no alias-derived consumer exists.

## Minimal portable consequence and fail-closed scope

**STRONG bounded semantic boundary:** preserve an owned handler registry
word with explicit input provenance; save/disable prior EE; perform the
actual read and replacement; restore EE through the freshly read current
MSR. Preserve all stack bytes, register outputs and branch decisions needed
by the next native section. These effects require neither interrupt hardware
emulation nor invocation of the registered callback in this interval.

Portable intent, conditional on the proven private owner:

```cpp
const bool was_enabled = interrupt_state.enabled();
interrupt_state.disable();
const auto old_word = owned_handlers.read(1); // explicit known entry bytes
owned_handlers.replace(1, machine_check_handler); // proven code identity
interrupt_state.restore(was_enabled);
return old_word; // raw word is forwarded, never invoked by this caller
```

The assembly-derived register/memory projection is additionally required
at checkpoints. This pseudocode alone is not enough to validate execution.
An arbitrary known32bit old word, including noncode/unaligned bits, is safe
**as copied data** in this interval; interpreting it as a call target or
rejecting it merely because it lacks a code address would invent behavior.
Unknown/partially known old bytes must still decline.

**UNKNOWN/excluded:** an external/pending interrupt or machine-check source,
asynchronous MSR or registry writers, physical EE visibility timing,
exception entry, callback execution, selector16 path, retail loader state.
Reenabling EE with pending work requires another proof. The private owner
must reject external async provenance instead of silently assuming absence.
The validated sync/L2 implementation should remain untouched.

## Falsification and validation evidence

**PROVEN static tests:** raw re-read/field decode and Capstone/project
disassembly agree on all selected62words plus the printed nextword;
all direct branch displacements match independent sign-extension.
The executable audit checks96one-bit projections,65536selector-low16
offsets and36full-MSR disable/restore cases against independent bit-selection
formulas. This catches rotate, wrap-mask, and EE preservation mistakes.

**Required differential experiments:** vary incoming EE0/1, XER.SO0/1,
CR1 state, arbitrary old-slot word (zero, installed pointer, unmapped or
unaligned word), both L2 branches and original varied FPR sources. Capture
before/after each EE write, old read/store, fresh restore read, return, logger,
main frame loads, wrapper and sentinel overwrite. Compare all observable
state and independently derived memory masks; reject unknown states.
Only the coordinator's full reference comparisons and complete regression
gates may promote the connected frontier. This note claims no dynamic pass
on its own. The next genuine dependency after this conditional return is
`8000316C → 80003340`, the CRT descriptor walkers and fill semantics.

## Native candidate adversarial review

The coordinator's `NativeCrtPrefix.cpp` handler/tail implementation was
reviewed against the raw effects above, independently of its tests.
**STRONG, bounded static equivalence:** no value, CR/MSR, LR, signedness,
ordered-spill or saved-return mismatch was found on selector1. Signed
`cmpwi` is correctly expressed by its unsigned comparison helper only
because the prior rotate proves the argument is exactly0/1. Unknown
old-slot input stops before the actual `803733B4` load after executing all
preceding stack/EE effects; a knownzero optional remains present data.

The paired-stack snapshot is refreshed from the authoritative L2 stack
bytes at each new checkpoint, preserving the r28/r29 alias and both
sentinel overwrites. The new registry owner now checks the actual read/
store address equals`80586CB4`, rejects unknown reads and reads back from
that same owner after storing. This closes a robustness gap in the first
candidate, where the effect ledger could name a wrong calculated address
while always updating the unbound scalar. No actual arithmetic divergence
was observed in that earlier fixed-selector candidate.

**UNKNOWN here:** connected dynamic parity, complete regression results
and frontier promotion remain coordinator-owned gates. This review does
not substitute a static conclusion for those passes.

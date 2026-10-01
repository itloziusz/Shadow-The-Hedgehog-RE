# PAL OS first-call clock boundary, binary-first audit

Current continuation: checkpoint40 validates the connected prologues through
**80379628**, before the first TB read. Research41 (`CLOCK_RESEARCH_41.md`)
now checks the reference producer, phase/units/offset, actual rollover retries,
carry/return/stores and next frame in an executed C++ projection. Production
still stops80379628: native elapsed-time/event ownership remains UNKNOWN.
The static/historical evidence below does not itself establish that provider.
Statements about the earlier sync stop describe this historical audit's scope.

Scope: the first-call prefix of `0x80370E68`, its direct clock helper
`0x80379648..0x803796A8`, stable time-base reader `0x80379628..3C`,
and the two interrupt-state leaves `0x8037611C..2C` and
`0x80376144..64`. This is a **static state-transition proof**, not connected
native execution. The connected native boot probe currently stops before
`sync` at `0x80371730`; the intervening hardware, CRT, and entry path remain
prerequisites. The source is the read-only PAL `main.dol`, SHA-256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
I parsed its DOL header directly: text1 maps VA `0x80008D40` to file offset
`0x00002600`. Thus `0x80370EA4` is file `+0x36A764`, and `0x80379678` is
file `+0x372F38`. The raw words below were read independently as big-endian
32-bit values and then compared with the local `ppc.py` decoder. The cached
`q.py` lookup cannot run in this checkout because `gameplay/data/classes.json`
is absent; none of the claims here depend on that cache.

## Caller prefix and exact CFG

`r13=0x805EC500` after the entry register helper is a prerequisite. The
following words are the complete prefix up through the two clock-result
stores; addresses, bytes/words, direct targets, and register operands are
**PROVEN** by the pinned DOL. Hex words are the four raw bytes in file order.

| VA | BE word | Instruction and effect |
|---|---|---|
| `80370E68` | `7C0802A6` | `mflr r0`; capture caller LR. |
| `80370E6C` | `90010004` | `stw r0,4(r1)`; old-stack return slot. |
| `80370E70` | `9421FFE8` | `stwu r1,-0x18(r1)`; write old r1 as back-chain, then lower r1. |
| `80370E74` | `93E10014` | `stw r31,0x14(r1)`. |
| `80370E78` | `93C10010` | `stw r30,0x10(r1)`. |
| `80370E7C` | `93A1000C` | `stw r29,0x0C(r1)`. |
| `80370E80` | `800D5A40` | `lwz r0,0x5A40(r13)` = `[0x805F1F40]`. |
| `80370E84` | `3C608058` | `lis r3,0x8058`; pointer formation. |
| `80370E88` | `3BE36C40` | `addi r31,r3,0x6C40` = `0x80586C40`. |
| `80370E8C` | `2C000000` | `cmpwi cr0,r0,0`. |
| `80370E90` | `3C608056` | `lis r3,0x8056`; pointer formation. |
| `80370E94` | `3BC310F8` | `addi r30,r3,0x10F8` = `0x805610F8`. |
| `80370E98` | `40820494` | `bne 0x8037132C` if guard was nonzero. |
| `80370E9C` | `38000001` | `li r0,1` on first-call arm. |
| `80370EA0` | `900D5A40` | `[0x805F1F40] = 1`, before clock read. |
| `80370EA4` | `480087A5` | `bl 0x80379648`; LR becomes `0x80370EA8`. |
| `80370EA8` | `908D5A54` | Store **low** returned word r4 to `[0x805F1F54]`. |
| `80370EAC` | `906D5A50` | Store **high** returned word r3 to `[0x805F1F50]`. |

The first-call guard is in the CRT zero interval
`[0x805EF020,0x805F277C)`; on the already-described CRT path, if no
intervening writer changes it, the branch falls through. The earlier direct
callee `0x80370BF0..0x80370C14` stores at `[0x805F1F18]`,
`[0x80000048]`, and `[0x805F1F1C]`; it does not write this guard. This is
a **conditional first-call proof**, not an observation that native reached
OS initialization. The alternative guard-nonzero successor is
`0x8037132C`; it skips this clock computation. There is no indirect call
in this bounded prefix; `bl` at `0x80370EA4` has one decoded target.

The OS prologue stack effect is ordered: at incoming `SP`, it first writes
caller LR to `[SP+4]`, then writes the old `SP` to `[SP-0x18]`, then spills
r31/r30/r29 at `[SP-4]`, `[SP-8]`, `[SP-12]`. The loaded old values and
later unwind are not asserted here because connected native execution has
not passed the CRT/entry prerequisites. The clock result stores are also
ordered **low first, high second**, after interrupt restoration; an
interrupt-enabled observer could see the intermediate pair. Collapsing the
two stores into an assumed atomic 64-bit host assignment would lose that
order.

## Time-base reader and clock helper

The `bl` hierarchy is `0x80370EA4 -> 0x80379648`, then
`0x80379660 -> 0x8037611C`, `0x80379668 -> 0x80379628`, and
`0x80379684 -> 0x80376144`. All are direct I-form calls. Return edges use
saved LR; no CTR dispatch occurs in these functions.

| VA | BE word | Instruction-level effect |
|---|---|---|
| `80379628` | `7C6D42E6` | `mftbu r3`, first time-base high read. |
| `8037962C` | `7C8C42E6` | `mftb r4`, low read. |
| `80379630` | `7CAD42E6` | `mftbu r5`, second high read. |
| `80379634` | `7C032800` | `cmpw cr0,r3,r5`. |
| `80379638` | `4082FFF0` | `bne 0x80379628` if high changed. |
| `8037963C` | `4E800020` | `blr`, returning stable high r3 and sampled low r4. |
| `80379648` | `7C0802A6` | `mflr r0`. |
| `8037964C` | `90010004` | Save caller LR at `4(old SP)`. |
| `80379650` | `9421FFE0` | 32-byte stack frame, back-chain store. |
| `80379654` | `93E1001C` | Spill r31. |
| `80379658` | `93C10018` | Spill r30. |
| `8037965C` | `93A10014` | Spill r29. |
| `80379660` | `4BFFCABD` | `bl 0x8037611C`, disable EE and return previous EE bit. |
| `80379664` | `7C7F1B78` | `r31 = prior_EE` (0 or 1). |
| `80379668` | `4BFFFFC1` | `bl 0x80379628`, stable high/low sample into r3/r4. |
| `8037966C` | `3CC08000` | `r6 = 0x80000000`. |
| `80379670` | `80A630DC` | `r5 = BE32[0x800030DC]`, offset low. |
| `80379674` | `800630D8` | `r0 = BE32[0x800030D8]`, offset high. |
| `80379678` | `7FA52014` | `addc r29,r5,r4`, low 32-bit sum and carry to XER.CA. |
| `8037967C` | `7FC01914` | `adde r30,r0,r3`, high sum including low carry; updates XER.CA. |
| `80379680` | `7FE3FB78` | `r3 = saved prior_EE`. |
| `80379684` | `4BFFCAC1` | `bl 0x80376144`, restore prior EE bit. |
| `80379688` | `7FA4EB78` | Return low sum in r4. |
| `8037968C` | `7FC3F378` | Return high sum in r3. |
| `80379690` | `80010024` | Reload saved caller LR to r0. |
| `80379694` | `83E1001C` | Restore r31. |
| `80379698` | `83C10018` | Restore r30. |
| `8037969C` | `83A10014` | Restore r29. |
| `803796A0` | `38210020` | Restore incoming SP. |
| `803796A4` | `7C0803A6` | Restore LR. |
| `803796A8` | `4E800020` | Return to `0x80370EA8`. |

**PROVEN formula conditional on the actual reads:** if the successful
time-base loop samples `T_hi,T_lo`, and the low-memory words are
`O_hi=BE32[0x800030D8]`, `O_lo=BE32[0x800030DC]`, then

```text
lo      = (T_lo + O_lo) mod 2^32
carry   = floor((T_lo + O_lo) / 2^32)
hi      = (T_hi + O_hi + carry) mod 2^32
XER.CA  = carry out of the high addition
```

This is a 64-bit addition modulo `2^64`, with the low word in r4 and high
word in r3 on return. `addc` and `adde` do **not** update CR0 here (no
record bit); the later restore helper does. A plain 64-bit host addition
can represent the pair only after the inputs, carry, wrapping, and ordered
side effects have been validated. The helper makes no MMIO access, FPR
operation, paired-single operation, or indirect call. Its normal RAM
effects are only its own frame spills, followed by the caller's two stores.

## Interrupt leaves and machine state

| VA | BE word | Instruction-level effect |
|---|---|---|
| `8037611C` | `7C6000A6` | `mfmsr r3`; capture current full MSR. |
| `80376120` | `5464045E` | `rlwinm r4,r3,0,17,15`; clear exactly EE bit `0x00008000`. |
| `80376124` | `7C800124` | `mtmsr r4`; install EE-cleared MSR. |
| `80376128` | `54638FFE` | `rlwinm r3,r3,17,31,31`; return old EE as 0 or 1. |
| `8037612C` | `4E800020` | `blr`. |
| `80376144` | `2C030000` | `cmpwi cr0,r3,0` on saved EE. |
| `80376148` | `7C8000A6` | `mfmsr r4`; use **current** MSR, not the entry word. |
| `8037614C` | `4182000C` | `beq 0x80376158` if saved EE was zero. |
| `80376150` | `60858000` | Nonzero: `r5 = current_MSR | 0x8000`. |
| `80376154` | `48000008` | `b 0x8037615C`. |
| `80376158` | `5485045E` | Zero: `r5 = current_MSR & ~0x8000`. |
| `8037615C` | `7CA00124` | `mtmsr r5`, restoring previous EE while keeping other current bits. |
| `80376160` | `54838FFE` | `r3 = EE bit of MSR read at 0x80376148`. |
| `80376164` | `4E800020` | `blr`. |

This pair saves **only the old EE bit**, not a copy of all entry MSR bits.
The restore routine preserves other bits of its own `mfmsr` read. It may
return the EE bit seen just before restoration in r3, but the caller
overwrites r3 with the high clock sum at `0x8037968C`. CR0 at helper return
is from `cmpwi prior_EE,0` in the restore leaf; the preceding stable-TB
`cmpw` CR0 result is not retained. XER.CA from the high `adde` is not
clobbered by these leaves. The actual external interrupt/callback and
time-base consequences of `mtmsr` are **UNKNOWN** without same-run trace.

## Unknown producers and native contract

The first live values that cannot be derived from DOL bytes alone are
the MSR at `0x8037611C`, the three dynamic time-base reads at
`0x80379628/2C/30`, and the two low-memory offset words at
`0x80379670/74`. `0x800030D8..DF` is below the DOL's first loaded text
section (`0x80003100`); no DOL file byte initializes those words.
Earlier apploader/IPL or OS state is the possible producer, and its exact
value at this read is **UNKNOWN**. It is unsafe to assume either offset
word is zero or that a synthetic HLE clock equals retail hardware. The
time-base values need a **same-run ordered sample**, not merely a wall-clock
timestamp taken later. A breakpoint after the stable loop plus reads at
`0x80379678` can supply `r3/r4/r0/r5` and prior EE saved in r31; a
checkpoint at `0x80370EA8` and memory snapshots after `EA8` and `EAC`
can validate the return pair and store order. RSP's cached XER register
must not be treated as architectural carry proof; use an architectural
`mfxer` or a debugger accessor that assembles XER.CA.

A native service boundary, once measured, must provide a stable ordered
time-base pair, exact offset words from guest low memory, and interrupt
state transitions with callback ordering. It must expose the wrapped
high/low result and preserve the caller's **low then high** stores. This
describes the required service contract, **not an implemented service**.
No native boot checkpoint advances because of this static audit. If any
of the inputs or the post-call state is unavailable, forward native
execution must still stop at this exact boundary.

The next instruction after these stores, `0x80370EB0`, makes a second
direct call to the disable-EE leaf before the low-memory pointer decision
at `0x80370EB8 -> 0x80373AB4` (reads `[0x800030F0]`). The later OS
initializer also contains device polls and an indirect call documented in
`OS_STARTUP.md`; their live target set and return behavior remain separate
unresolved boundaries. No claim that OS initialization returns to
`0x80003268` follows from the clock slice.

## Validation status

- **Pass 1, binary:** SHA-256 and DOL header mapping checked directly;
  all listed BE words re-read from file. I-form call and B-form conditional
  targets were checked from displacement fields and against the local decoder.
- **Pass 2, ISA/data flow:** register, CR0, XER.CA, LR, stack, MSR and
  ordered memory effects above follow the decoded instruction stream.
  Exact live TB/MSR/offset values and console-specific interrupt effects
  remain **UNKNOWN**.
- **Pass 3, state:** no same-run original OS clock checkpoint was taken
  for this note. No native state comparison is claimed.
- **Pass 4, execution:** the connected native probe has **not** reached
  this slice; the full earlier prefix must rerun and pass before lowering.
- **Adversarial check:** zero offsets, fixed TB, assumed EE=0, atomic
  pair store, preserved TB-loop CR0, or a full-MSR restore would each
  contradict at least one cited instruction or missing input.

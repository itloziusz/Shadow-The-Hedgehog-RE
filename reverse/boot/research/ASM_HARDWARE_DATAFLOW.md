# PAL DOL hardware-prefix register and memory data flow

Scope: entry `0x80003154` through return from `0x80003400` to
`0x8000315C`, including the reachable nested calls. Source is the read-only
PAL GUPP8P `main.dol` (SHA-256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`),
queried instruction by instruction with `q.py dis`. **PROVEN** below means
instruction-backed; **OBSERVED-HLE** refers only to the separate synthetic
startup-only Dolphin disc and cannot establish a retail IPL handoff. This
is an assembly data-flow record, not a connected native implementation.

## Entry and caller-owned state

| Range | Exact effects and data flow | Successor |
|---|---|---|
| `0x80003154` | `bl 0x800032B0` puts `0x80003158` in LR. No other input. | `0x800032B0` |
| `0x800032B0..0x80003320` | Twenty-nine `li` instructions zero r0, r3..r12, r14..r31. They do not set CR. r2 and r13 are not touched here; r1 is set below. | `0x80003324` |
| `0x80003324..0x8000333C` | `lis`/`ori` make r1=`0x8060C5F0`, r2=`0x805FA780`, r13=`0x805EC500`; `blr` returns with LR still `0x80003158`. No memory access or stack frame. Initial CR/XER/CTR/MSR/FPSCR/FPRs and hardware SPRs survive. | `0x80003158` |
| `0x80003158` | `bl 0x80003400` puts `0x8000315C` in LR. | `0x80003400` |

The observed synthetic Dolphin stop at `0x80003400` has r1/r2/r13 above,
other GPRs zero, LR=`0x8000315C`, MSR=`0x00002032`, CR=`0x20000000`,
CTR=XER=FPSCR=0, HID0=`0x0011C464`, HID1=`0x80000000`, L2CR=0, and
`0x805F1F30..3F` all zero. These are **OBSERVED-HLE** inputs for that one
launch mode. HID2 and GQR state were not captured at that stop.

Let `S = 0x8060C5F0`, the r1 value on entry to `0x80003400`.

## Direct callee: `0x80003400..0x80003420`

| Instructions | Register/state effects | Memory/branch effects |
|---|---|---|
| `0x80003400..0x80003408` | `r0 = MSR_in | 0x2000`; `mtmsr r0`. In the synthetic input the bit is already set, so the written word is `0x2032`. | No RAM access. An MSR write is still an ordered hardware effect. |
| `0x8000340C` | `r31 = LR = 0x8000315C`. This **persists** when the function returns; the entry helper had set r31=0. | None. |
| `0x80003410`, `0x80003414`, `0x80003418` | Direct calls in order to `0x80371714`, `0x80370CDC`, `0x80372838`; each `bl` replaces LR with the following address. | No conditional dispatch here. |
| `0x8000341C..0x80003420` | `mtlr r31; blr` returns to `0x8000315C`. r0/r3 and CR may have changed in nested callees; they are not restored by this wrapper. | No own stack frame. |

No ABI-preservation assumption is needed for r31: the explicit `mflr` and
`mtlr` carry the return address through nested calls.

## Paired-single setup: `0x80371714..0x80371764`

`0x80371714..0x8037171C` save caller LR (`0x80003414`) at `S+4`, then
`stwu r1,-8(r1)` writes old `S` at `S-8` and sets r1=`S-8`.
`0x80371720` calls `0x80370BA8`, whose two instructions read HID2 to r3
and return. `0x80371724` ORs `0xA0000000` into that r3, and
`0x80371728` calls `0x80370BB0`, whose two instructions write r3 to HID2
and return. The output is exactly `HID2_in | 0xA0000000`; unforced bits
still depend on the incoming SPR. `0x8037172C` calls `0x803725F4`:
`mfspr hid0; ori 0x0800; mtspr hid0; blr`, so the **written word** is
`HID0_in | 0x00000800`. Bit `0x800` is ICFI, a self-clearing instruction-cache
invalidate command when ICE is enabled (Gekko manual Table 2-4); do not use
that write word as later HID0 readback. The synthetic run reads back
`0x0011C464`, equal to its initial HID0. `0x80371730` issues `sync`; `0x80371734` makes
r3 zero; `0x80371738..0x80371754` write zero to GQR0 through GQR7 in
order. `0x80371758..0x80371764` reload LR from `(S-8)+0xC = S+4`,
restore r1=S and return to `0x80003414`.

There is no conditional or indirect call in this region. Direct hardware
writes are not interchangeable with initial-state assumptions: the final
HID2/HID0 words are only partially determined by the forced masks.

## FPR/FPSCR initialization: `0x80370CDC..0x80370E00`

`0x80370CDC..0x80370CE4` read MSR into r3, OR `0x2000`, and write it
back. `0x80370CE8..0x80370CF0` read HID2, rotate/mask its source bit 2
into a CR0 test, and branch to `0x80370D7C` only if that result is zero.
The previous OR of `0xA0000000` forced source bit 2, so **PROVEN** the
branch is *not* taken if the HID2 write/read has its architectural effect.
The selected paired-single block is therefore `0x80370CF4..0x80370D78`.

`lis/addi` at `0x80370CF4..0x80370CF8` form r3=`0x805F1F38`.
`psq_l f0,0(r3),W=0,GQR0` at `0x80370CFC` reads eight bytes
`0x805F1F38..3F` as two quantized lanes, with GQR0 previously cleared.
Each `ps_mr` at `0x80370D00..0x80370D78` copies f0 to one of f1..f31.
Next `lfd f0,0x5A30(r13)` at `0x80370D7C` reads eight bytes at
`0x805F1F30..37` because r13=`0x805EC500`.
Each `fmr` at `0x80370D80..0x80370DF8` targets one of f1..f31.
`mtfsf 0xFF,f0` at `0x80370DFC` writes all writable FPSCR fields from
f0's source bits, with architectural derived summary bits; `blr` returns.
Exact PS1 lane preservation across `lfd`/`fmr` and exceptional source
encodings require CPU-level evidence. They must not be replaced by a guessed
single binary64 value. For the synthetic input all 16 source bytes are zero,
but the final FPR-lane/FPSCR state still needs checkpoint comparison.

These 16 bytes are outside populated DOL sections and inside a *later* CRT
zero-table range. Neither a DOL section nor later zeroing proves their value
at this earlier read. Their retail IPL input is **UNKNOWN**.

## Cache and handler path: `0x80372838..0x80372928`

On entry LR=`0x8000341C`, r1=S, r31=`0x8000315C`.
`0x80372838..0x80372850` save LR at `S+4`, allocate 16 bytes at
`S-0x10`, save r31 at `S-4` and r30 at `S-8`, then form
r31=`0x80561380`, the base of diagnostic strings.

| Branch/region | Producer and exact effect | Selected synthetic path |
|---|---|---|
| `0x80372854..0x80372870` | `0x80370AEC` reads HID0. Mask bit 16 (`0x00008000`); if set, branch `0x80372860 -> 0x80372874`. Otherwise `0x80372604..14` performs `isync`, reads HID0, ORs `0x8000`, writes HID0, then diagnostic call. | Observed HID0 readback=`0x0011C464`; its `0x8000` bit is set, so branch taken. |
| `0x80372874..0x80372890` | Re-read HID0, mask bit 17 (`0x00004000`); if set, branch `0x80372880 -> 0x80372894`. Otherwise `0x803724F4..0x80372504` does `sync`, reads HID0, ORs `0x4000`, writes HID0, then diagnostic call. | Bit set, so branch taken. |
| `0x80372894..0x803728A0` | Read L2CR via `0x80370AFC`; test top bit `0x80000000`. If set, branch to `0x803728F8`. | L2CR=0, so enter initialization path. |
| `0x803728A4..0x803728CC` | Save MSR to r30, issue `sync`, write MSR=`0x30`, two more `sync`, read L2CR, clear top bit, write L2CR, then `sync`. | Initial MSR from HLE is `0x2032`; exact interim asynchronous hardware effects unobserved. |
| `0x803728D0` | Call invalidate helper `0x80372640`, detailed below. | Required; helper termination depends on L2CR hardware response. |
| `0x803728D4..0x803728F4` | Restore MSR from r30, read L2CR, set top bit and clear bit 10 (`0x00200000`), write L2CR, then diagnostic call. | Required after invalidate returns. |
| `0x803728F8..0x80372910` | r4=`0x803726D8`, r3=1, call handler install `0x80373378`, then pass message pointer `0x8056157C` to diagnostic call. | Selected for either L2 branch. |
| `0x80372914..0x80372928` | Restore LR from `S+4`, r31 from `S-4`, r30 from `S-8`, r1=S, return. Thus r31 again equals `0x8000315C`. | Returns to `0x8000341C`. |

Every diagnostic call here is to `0x80370C8C`, a stack-only variadic sink:
caller `crxor crb6,crb6,crb6` clears CR1.EQ, so `bne cr1` at
`0x80370C90` skips the FPR spills; `0x80370CB4..0x80370CD0` save r3..r10
to its temporary 0x70-byte frame, then it unwinds and returns. This changes
CR1.EQ and transient stack bytes but has no demonstrated external output.

### L2 invalidate helper: `0x80372640..0x803726D4`

Its frame saves caller LR at `T+4` (where `T=S-0x10`), allocates 16 bytes
and saves r31. `0x80372650..0x8037266C` issue `sync`, read L2CR, clear
its top bit, write L2CR, issue `sync`, read L2CR, set bit 10
(`0x00200000`), and write L2CR. The direct branches at `0x80372670/74`
lead to the first poll. `0x80372678..0x80372684` repeatedly read L2CR
until low bit 0 is zero; the exact number of iterations is hardware input.
`0x80372688..0x80372690` read L2CR, clear bit 10, and write it.
`0x80372698..0x803726A4` form diagnostic pointer `0x80561380` and
branch to the second poll. At `0x803726B4..0x803726C0`, a nonzero low
bit causes `0x803726A8..0x803726B0` diagnostic call followed by another
read. The function restores LR/r31/r1 at `0x803726C4..0x803726D4`.
No finite iteration count or successful return is established by static
assembly alone. Both loops read live L2CR state.

### Handler registration: `0x80373378..0x8037358C`

With caller r1=`T=S-0x10`, r3=1, r4=`0x803726D8`,
`0x80373378..0x80373398` save LR at `T+4`, allocate 0x30 bytes,
save r31/r30/r29/r28, and retain selector in r29 and handler in r28.
`0x8037339C` calls `0x8037611C`: it reads MSR, clears bit 16
(`0x00008000`, EE) via `mtmsr`, and returns the old EE bit as 0/1 in r3.
`0x803733A0..0x803733C0` form table base `0x80586CB0`, offset
`((1 << 2) & mask(14..29)) = 4`, load old slot value to r30, compare
selector low 16 bits with `0x10`, retain old EE in r29, and store
`0x803726D8` at **`0x80586CB4`**. The `bne` at `0x803733C4` is taken
because 1 != 16. Thus `0x803733C8..0x80373560` are **unreachable on this
entry call**; their low-memory list walk and extra state cannot be imported
into this selected path. `0x80373564..0x80373568` pass old EE to
`0x80376144`, which reads current MSR and sets/clears EE according to that
argument. It returns previous current EE bit in r3, ignored here.
`0x8037356C..0x8037358C` return the old slot value in r3 and restore
LR/r28..r31/r1. The caller then overwrites r3 with a diagnostic pointer,
so the slot's old *value* does not control a branch or pointer dereference
in this prefix; the slot read itself is still a real memory effect.

The slot is in the later CRT zero range `0x8056FE00..0x805E4500`.
That later write must be tracked in sequence; this store is not assumed to
persist into the game. The handler target is a concrete function address,
not an indirect call on this path.

## Live inputs, outputs and fail-closed boundary

| State | Producer in this prefix | Defined output / unresolved input |
|---|---|---|
| r1/r2/r13 | `0x80003324..38` | Fully defined constants above; r1 returns to S. |
| r31/LR | `0x8000340C/1C` | r31 and LR both `0x8000315C` on return. |
| r0/r3/r4..r12, CR | Nested calls and conditional compares; no blanket restore. | A full checkpoint must compare these rather than assuming ABI clobber equivalence. CR1.EQ is cleared if a diagnostic call occurs. |
| MSR | Incoming hardware state, OR FP, possible L2 temporary `0x30`, EE disable/restore. | On the selected HLE L2 path, final value is derived from incoming MSR if all calls return; actual asynchronous hardware effects need observation. |
| HID0 | Incoming HID0; write `HID0_in | 0x0800` as an ICFI command, then read back after hardware effects; optional cache-bit writes. | Synthetic path initial and later readback both `0x0011C464`; both L1 branches skip. Retail readback and cache effects UNKNOWN. |
| HID2/GQR0..7 | Incoming HID2 OR `0xA0000000`; all GQRs zero. | Exact unforced HID2 bits UNKNOWN; paired-single branch forced selected. |
| L2CR | Incoming L2CR and live poll transitions. | Branch and termination hardware-dependent; synthetic initial value 0 only proves initial branch. |
| FPR0..31/FPSCR | Two eight-byte reads, 31 `ps_mr`, 31 `fmr`, `mtfsf`. | Retail bytes and exact final paired lanes UNKNOWN; HLE bytes zero, still compare final state. |
| Handler slot | Prior RAM value at `0x80586CB4`, then concrete store. | Old value forwarded briefly, not consumed by next branch; slot later zeroed by CRT. |

No indirect branch or virtual call is selected in this prefix. The only
loops are the L2CR polls. The **first unsafe unknown for full state parity**
is initial MSR/HID0/HID2/L2CR (and their asynchronous effects); the first
non-DOL RAM read on the forced FPR path is `0x805F1F38`. A native block
must receive evidence-backed entry state and ordered hardware service
results, then compare a same-run PPC checkpoint at `0x8000315C`. The
synthetic Dolphin observation can validate only its own HLE launch mode.

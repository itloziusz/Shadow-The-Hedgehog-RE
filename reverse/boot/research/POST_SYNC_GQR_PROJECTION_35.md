# Conditional post-`sync` GQR tail projection

**Scope:** PAL GUPP8P `main.dol`, `0x80371734..0x80371764`, immediately
after the unresolved ICFI/`sync` at `0x80371730`. Original read-only DOL
SHA-256: `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
`PROVEN-BINARY` means raw word and instruction fields; `OBSERVED-HLE` means
the archived startup-only Dolphin interpreter capture; `UNKNOWN` marks
uncaptured hardware effects or retail inputs. This is a conditional
prediction, **not a newly connected native boot checkpoint**.

## Byte and state chain

The DOL section maps file `0x2600` to VA `0x80008D40`. Its exact thirteen
big-endian words at file `0x36AFF4..0x36B024` are:

```text
38600000 7C70E3A6 7C71E3A6 7C72E3A6 7C73E3A6
7C74E3A6 7C75E3A6 7C76E3A6 7C77E3A6 8001000C
38210008 7C0803A6 4E800020
```

**PROVEN-BINARY:** the first word is `li r3,0`. The following eight words
are `mtspr GQR0..GQR7,r3` (XO467; split SPR fields decode to 912..919).
The final four are `lwz r0,12(r1)`, `addi r1,r1,8`, `mtlr r0`, `blr`.
There is no conditional branch, SDA access, FPR operation or ordinary RAM
store in this tail. The only ordinary RAM read is the saved LR word. See
`SYNC_GQR_CHAIN.md` for each address and raw byte.

**PROVEN-ISA conditional transition:** after a successful preceding `sync`
and successful supervisor GQR writes, let incoming SP be `S-8` and let
`M=BE32(S+4)` at the time of the `lwz`. At `0x80371758`, r3 is zero and
the eight ordered GQR write operands are zero; r0, SP and LR still have
their entry values. The epilogue then sets r0=`M`, SP=`S`, LR=`M` and
PC=`M & ~3` (`bclr` forms the target from LR's upper 30 bits, as specified
by the [IBM PowerPC instruction reference](https://www.ibm.com/docs/en/aix/7.2.0?topic=set-bclr-bcr-branch-conditional-link-register-instruction)). It does not
change CR, XER, CTR, FPSCR, FPRs or r2/r4..r31.
The native projection reads the *current* validity-tracked stack bytes;
it does not substitute the earlier saved LR constant.

**OBSERVED-HLE:** the archived same-run capture at `0x80371758` has
r0=`0x80003414`, SP=`0x8060C5E8`, r3=`0`, LR=`0x80371730`, CR=`0x20000000`,
XER=`0`. At the following `0x80370CDC` entry, r0=`0x80003414`,
SP=`0x8060C5F0`, r3=`0`, LR=`0x80003418`; the wrapper's intervening
`bl 0x80370CDC` accounts for the latter LR/PC. The GDB stub did not expose
GQR readback, cache tags or PS1. This validates the exposed GPR path for
one synthetic run; it does not validate acceptance of all eight SPR writes
on retail hardware.

## Native projection and falsification

`PredictPostSyncGqrTail()` verifies **all thirteen** PAL instruction words
and the preceding `sync` word, requires the pre-`sync` PC/LR, matching ICFI
request operand and supervisor mode, then returns two
predicted CPU states and eight ordered SPR write operands. Its name and
result type deliberately do not mark `sync` complete or update the connected
boot probe. The first state is before `lwz` at `0x80371758`; the second is
after `blr`, with PC derived from the stack load. The prediction remains
conditional on the unresolved ICFI/cache/barrier consequence at
`0x80371730`.

The native test checks exposed HLE GPR state, each GQR number/operand, and
preservation of other GPRs. It changes the saved LR bytes and verifies that
the predicted PC changes, varies incoming r3, rejects an unwritten saved
return byte, verifies that an unaligned loaded LR retains its low bits while
the branch PC clears them, rejects user mode, mismatched ICFI operand and wrong entry PC,
and mutates each of the fourteen raw words including `sync`. These checks
refute fixed checkpoint values and a
single undifferentiated GQR flag. They do not invent HID0 readback or a
`sync` completion acknowledgement.

The adversarial low-bit check found an actual first-version bug: the
projection set PC to the entire loaded LR. `blr` retains the full word in
LR but clears its low two bits for the branch target. The C++ projection
and the recognizer's symbolic indirect LR/CTR targets were corrected, with
odd-LR negative cases in both tests. The ordinary captured LR was aligned,
so that run alone could not reveal the error.

**Next connected obligation:** establish a stateful ICFI/`sync` completion
contract and independently sourced post-command HID0 readback for the
selected start state; verify text visibility and ordered stack effects.
Then cross the barrier in the full native entry run, compare `0x80371758`
and the first GQR-dependent `psq_l` at `0x80370CFC`, and preserve explicit
declines for ICE=0, ABE=1, interrupted and retail-unknown starts.

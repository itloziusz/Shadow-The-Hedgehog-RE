# Checkpoint 36 — measured cache/GQR/PS1 state and bounded FPR C++

The connected native prefix still stops **before `sync` at `0x80371730`**.
This checkpoint removes an observation gap, adds a conditional C++ projection,
and fixes a reproduced FPSCR error. It does not bypass the barrier or establish
retail IPL, physical cache/bus timing, first-frame or pixel parity.

## Identity and confidence

- **PROVEN-BINARY:** PAL DOL SHA-256
  `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
  `BINARY_BOOT_PREFIX.md` and `BINARY_FPR_PREFIX.md` remain the complete raw-word
  catalogues; their existing CTest gates reread the binary.
- **OBSERVED-HLE:** startup-only disc SHA-256
  `a8a526ef7006a2d2a8d41f536ef30b208ff197c185c05e9b2276e6e93cb41a8e`.
  `make_boot_oracle_disc.py` reproduces this exact disc by hash-checking five
  read-only SYS inputs and placing them at the original header offsets, with
  zero-filled ordinary game-file space. It is not a complete game disc.
- **OBSERVED-HLE:** original oracle executable SHA-256
  `db536025ee02190c20ccb9bfa94ee36ce28bb996f2e88574e46734b353bdc526`;
  extended final executable SHA-256
  `1b37d68d6ea92d4eca1407823616c8c1b9a59153a2809c97615f09fcfacd959f`.
  The intermediate PS1-only export build was
  `9141553a357fef16177bb219e1f682c1a01b024eb9460f10424b89204a2518dc`.
- **STRONGLY_SUPPORTED, bounded:** the native FPR projection agrees with raw
  observed state on the supported paired edge. Generic Gekko internal format,
  exceptional conversions, the skipped paired edge and physical timing remain
  **UNKNOWN**. The recognizer seed retains this scope and is not promoted to
  connected boot validation.

The builder compiles one copied `GDBStub.cpp` translation unit and links it
before the original read-only archives/objects. It changes only `ReadRegister`:
IDs 143–174 expose PS1 raw binary64, 175–182 GQR0–7, 183 HID2, 184–215 the 128
instruction-cache valid bytes, 216–247 the 128 replacement bytes, 248 the
cache-disabled flag and 249 the *composed live XER*. Original register 69 reads
the legacy SPR slot and can be stale; it is retained separately, never used as
the architectural XER oracle. No instruction handler, guest writer, reset,
branch or loader is changed. A local manifest pins copied source and every
link input. Compiler/user state stays under ignored `build/`.

At 15 ordered checkpoints, all shared register, memory, instruction and PS0
fields of the intermediate export matched a byte-identical copy of the
original executable. It was necessary to run that unchanged copy in the
private build directory: launching directly from the original build directory
timed out without opening GDB. That failed launch supplied no state evidence.

## Cache command, barrier and GQR observations

Raw `0x803725FC = 7C70FBA6` issues the live r3 word to HID0. Single-stepping
this word, before fetching the next instruction, produced:

| Pre-instruction PC | HID0 readback | Cache valid bytes | Replacement metadata |
|---|---|---|---|
| `803725FC` | `0011C464` | 13 nonzero bytes, each `01` | nonzero |
| `80372600` | `0011C464` | all 128 bytes zero | all 128 bytes zero |
| `80371730` | `0011C464` | byte 48=`01`; other bytes zero | subsequent fetch state |
| `80371734` | `0011C464` | bytes 48 and 57=`01`; other bytes zero | subsequent fetch state |

At the command, r3 was `0011CC64`; readback was separately `0011C464`. The
cache-disabled flag was zero. This measurement distinguishes the reset
consequence from later refills; expecting zero tags at an arbitrary later
breakpoint would be incorrect. The paired stack bytes remained visible at
`0x8060C5E8..F7`, including BE32 backchain `8060C5F0` and saved LR `80003414`.
All eight GQRs read back zero at `0x80371758`, before `psq_l`, and afterwards.
They were also zero initially in this selected HLE run; the measurement does
not independently demonstrate a transition from a nonzero GQR input. The
eight exact `mtspr` words independently establish the ordered writes.

**Falsification:** the inspected reference interpreter's HID0 handler clears
ICFI and calls `iCache.Reset()` whenever ICFI is set, without testing ICE.
`Cache::Reset()` clears validity and replacement metadata. Its `sync` handler
is explicitly ignored. Therefore this HLE capture cannot prove ICE=0 hardware
behavior or physical `sync` ordering. The [Gekko manual](https://doc.kodewerx.org/documents/gekko_user_manual.pdf)
specifies ICE-dependent invalidation and ABE-dependent ordering; the narrower
ICE=1/ABE=0 observation does not validate other paths. No cache mechanism is
copied into the native executable, and no completion is fabricated.

Read-only reference source digests:

| File | SHA-256 |
|---|---|
| `Interpreter_SystemRegisters.cpp` | `698f153ad3a9e457c7d51f9a84358b86140c29b879f35dc39fe70412f23f8556` |
| `Interpreter_LoadStore.cpp` | `dbfc463df0e83aa989e7728bec097d3d95f05ec2e0fa2a409291e3b985234553` |
| `PPCCache.cpp` | `8027ff33c074b2e30f55553d720216ca41c44060473424deff23ecbab1c2d05e` |

## FPR region: exact dependency → native projection

Address range: `0x80370CDC..0x80370E00`, file `0x36A59C..0x36A6C3`, 74 words.
Category: CPU/FPR state, with explicit prior HID2/GQR/MSR dependencies.
`FPR_LANE_PROVENANCE.md` contains individual opcode fields and the complete
sweep formulas; `BINARY_FPR_PREFIX.md` is the checked raw catalogue.

| Original region | Inputs and exact observable effect | Successor |
|---|---|---|
| `80370CDC..CE4` | live MSR → r3; OR `2000`; write full MSR | HID2 read |
| `80370CE8..CF0` | live HID2 bit29 → r3; CR0 from zero/positive result and XER.SO; tested branch | paired edge or `80370D7C` |
| `80370CF4..CFC` | `lis/addi` → `805F1F38`; `E0030000` consumes two BE32 words, W=0/I=0/GQR0=0 | `80370D00` |
| `80370D00..D78` | 31 exact `ps_mr fn,f0` words copy both lanes | `80370D7C` |
| `80370D7C` | `C80D5A30`: live r13=`805EC500`; BE64 at `805F1F30` replaces f0 PS0, measured PS1 retained | `80370D80` |
| `80370D80..DF8` | 31 exact scalar `fmr fn,f0` words replace PS0; measured PS1 retained | `80370DFC` |
| `80370DFC..E00` | `FDFE058E`: low 32 source bits → all FPSCR fields, with derived FEX/VX and reserved bit cleared; `blr` preserves LR and aligns return PC | live LR target |

There is no stack frame, nested call, ordinary memory write, destructor or
vtable action in this routine. Its memory reads are confined to the two
explicit source spans on the supported edge. Incoming GPRs other than r3,
LR/CTR, XER and lower seven CR fields are preserved. CR1 is not updated
because these FP words have Rc=0. No source bytes are inferred from later CRT
zeroing or static section gaps.

`PredictFprSeed()` takes explicit entry GPR/lane/CR/XER/FPSCR/MSR state, live
HID2, GQR0, and 16 live source bytes with their address. It fingerprints every
word. Integer bit operations widen normal/signed-zero binary32 inputs exactly;
host float arithmetic and host rounding cannot mask a missing guest effect.
It exposes eight intermediate states rather than just a final result.
It declines wrong PC/SDA/source provenance, user mode, POW/EE/FE0/FE1,
trace/little-endian modes, a reserved bit in incoming FPSCR, absent
paired/quantized enable, nonzero GQR0, paired subnormals/nonfinite values and
nonfinite scalar input. The skipped paired edge remains unsupported, not
silently replaced by a lane default. This callable projection is **not used
by `shadow_boot_probe`**.

### Distinct-lane experiment

At routine entry `0x80370CDC`, a separately labelled debugger write changed
only live BSS `[805F1F30..3F]` to
`400A000000000000 3FC00000 C0100000`: scalar double 3.25, paired singles 1.5
and -2.25. The original input DOL and instructions stayed read-only.

| Checkpoint | f0 PS0 | f0 PS1 | f1/f31 effect |
|---|---|---|---|
| `80370D00` | `3FF8000000000000` | `C002000000000000` | still incoming lanes |
| `80370D7C` | same | same | both lanes copied into all 31 destinations |
| `80370D80` | `400A000000000000` | `C002000000000000` | destinations still paired values |
| `80370D84` | same | same | f1 PS0 replaced; f31 still paired PS0 |
| `80370DFC` | same | same | all PS0 replaced; all PS1 retained |
| `80003418` | same | same | FPSCR zero; return state agrees |

Both lanes of all 32 FPRs were read at every checkpoint. This defeats the ambiguity of
the previous zero-only/PS0-only capture. It proves the selected HLE model,
not the physical internal encoding of a Gekko register.

### Reproduced FPSCR bug and fix

Adversarial source `3FF0000000000800` is a finite scalar whose low word sets
numeric `0x800`, IBM FPSCR bit20. Native projection initially returned
`00000800`; the observed return FPSCR was `00000000`. The first divergence
was the FPSCR effect of `mtfsf` at `0x80370DFC`, observed at `0x80003418`.
The reference's `UReg_FPSCR` masks this reserved bit on assignment; its
`mtfsf` then derives FEX/VX. Both the new projection and the older
reference-only `DeriveGekkoFPSCR()` had omitted that mask. Both now clear
`0x60000800` before deriving summaries. Dedicated regression cases cover the
reserved bit in both implementations. The exact recorded counterexample
now agrees at every checkpoint; no register or branch was forced.

Other controlled cases independently measured signed -0/+0 lane separation,
raw FEX with no exception flags (derived result zero), and invalid flag plus
VE (derived result `61000080`). The [manual's `mtfsf` entry, p. 12-137](https://doc.kodewerx.org/documents/gekko_user_manual.pdf#page=473)
specifies derivation of FEX/VX rather than copying their source bits.

## Validation, feedback and next proof

Six fresh HLE runs — original zero sources, distinct finite lanes, raw FEX,
signed zeros, VX/VE and reserved bit — each matched eight native checkpoints
and **816 raw fields**, total **4,896 comparisons**. The fields are PC/MSR/LR,
CR/composed-XER/FPSCR, 32 GPRs and both lanes of 32 FPRs. This is bounded
state-projection validation; it does not extend the connected prefix.

The full Release CTest gate passed **48/48** after the final fixes. Native
tests independently mutate all 74 words, vary incoming state and source
values, check every sweep element, check signed/exponent boundaries and
reject exceptional/unsupported inputs. Repeated calls test stale-state
leakage. RSP regression tests cover the genuine `E0030000`/error ambiguity,
short reads, disconnects and fragmented checksums.

Recognizer feedback: one bounded FPR seed is `STRONGLY_SUPPORTED`. A fresh
full constructor-table scan stored 276 regions: 2 bounded `VALIDATED`, 7
`STRONGLY_SUPPORTED`, 267 `UNKNOWN`; 123 learned patterns, 971 structural
hits and six complete held-out MotionImpl family matches. Unknown targets
were not promoted. The native probe and recognizer both kept the frontier
at `0x80371730`.

Local raw-capture hashes (ignored `build/`, no binaries or private paths committed):

| Capture | SHA-256 |
|---|---|
| `boot-machine-state-zero-36e.json` | `217b04bfdd0381ffb2283aba7c7194a3831755aa410db0ce18e7a4ee0c63dffb` |
| `boot-machine-state-finite-36e.json` | `666c3c5ef8a8e87b1c3f5b3afbab9132cf7fd64affa9a76a8bbb33947e11f38d` |
| `boot-machine-state-fpscr-36e.json` | `d7244cd92c5a862d71530c5351eac926696b156f6f78d13fe887f7e1a83fa6f2` |
| `boot-machine-state-signedzero-36f.json` | `cb6cd9156b63183f24ced486c9e1f191010b1905724016a984da757337444378` |
| `boot-machine-state-vx-36f.json` | `2104db2510261fce6a21bd009207536fab246449ac420883f55ffa9aea074d4d` |
| `boot-machine-state-reserved-36g.json` | `50097d314b2ab96c42ff5df4c87031100b069be1d6a9915e2034e2c2e7192458` |

**Next:** prove and implement a bounded native immutable-code instruction
visibility/completion contract before connecting sync and the GQR tail.
Keep ICE=0, ABE=1, interrupts, self-modifying code and retail handoff declined
until separately understood. Then feed the measured live source/lane state
into this projection and compare the downstream cache/HID0 consumers from
the boot entry. Physical PS1 representation and exceptional FP semantics
remain separate obligations. A HLE `sync` no-op is not a native proof.

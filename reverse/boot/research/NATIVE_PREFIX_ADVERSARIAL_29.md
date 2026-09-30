# Native boot prefix adversarial audit — checkpoint 29

Scope: connected C++ path from PAL GUPP8P DOL entry `0x80003154` to the
pre-`sync` stop `0x80371730`. The test input is the original read-only
`main.dol` (SHA-256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`).
This audit checks the code independently of the checkpoint 29 author's
positive synthetic/HLE run. It does not assert retail IPL state.

## Finding A — privileged `mfmsr` silently executes for a user-mode input

**PROVEN BINARY:** DOL file `0x400`, guest `0x80003400`, bytes `7C 00 00 A6`,
word `0x7C0000A6` encode `mfmsr r0`. At `0x80003404/08`, bytes
`60 00 20 00` / `7C 00 01 24` encode `ori r0,r0,0x2000` and `mtmsr r0`.
The first privileged instruction is therefore `0x80003400`, before the
paired setup call and before any native stack write.

**PROVEN ISA for the 750-family architectural rule:** The MPC750 RISC
Microprocessor Family User's Manual, NXP
([primary manual](https://www.nxp.com/docs/en/reference-manual/MPC750UM.pdf)),
Table 4-4, defines MSR[PR] (architecture bit 17, numeric mask `0x00004000`):
one permits only user-level instructions. Its instruction-related exception
section lists `mfmsr` and `mtmsr` as supervisor-level; attempting one in
user mode invokes the privileged-instruction program exception. **STRONG
GEKKO TRANSFER:** Gekko is 750-derived, and the existing native writer
guards use this same numeric PR mask. A retail same-state exception trace
is not available.

**PROVEN NATIVE DIVERGENCE, reproducible before any patch:**

```powershell
& 'build\reverse\boot\Release\shadow_boot_probe.exe' `
  'H:\ShadowTheHedgehogAssetLoaderRE\sys\main.dol' `
  --observed-msr 00004000
```

The command exited `0` and reported `CHECKPOINT pc=0x80371714`,
`msr=0x00006000`, two stack `WRITE` events, and `STOP pc=0x80370BA8`.
`EnterPairedSetupCall()` in `BootFoundation.cpp` ORs the supplied MSR with
`0x2000` without a PR guard. The earliest correct boundary for this input
is the privileged-instruction exception at `0x80003400`; reaching
`0x80371714` is a native semantic error. An exact exception vector cannot
be claimed without the live exception state, so the safe bounded native
behavior is to decline before the `mfmsr` transition.

**Required regression:** a user-mode MSR input must be rejected by
`EnterPairedSetupCall()` at `0x80003400`. A probe with the same input must
exit nonzero without a later checkpoint or stack events. The normal
supervisor synthetic input `0x00002032` must retain every prior checkpoint.

## Finding B — public HID2 read transition lacks its own privilege guard

**PROVEN BINARY:** guest `0x80370BA8`, DOL file `0x36A468`, bytes
`7C 78 E2 A6`, word `0x7C78E2A6` encode `mfspr r3,HID2` (SPR 920),
followed by `blr` at `0x80370BAC`. HID2 is a supervisor SPR. The same
MPC750 manual says attempting a supervisor-level `mfspr` in user mode takes
a program exception.

**PROVEN CODE PATH:** `ReturnFromHid2Read()` checks PC/LR and word
fingerprints, then copies caller-supplied HID2 into r3 and returns. It does
not check `state.machine.msr & 0x4000`. Both later writers do check PR.
Because `PairedSetupStackPrefix` is public and mutable, an otherwise valid
prefix with its MSR changed to user mode can be accepted and fabricated into
a completed HID2 read. The same direct guard belongs at this first SPR read,
irrespective of the earlier normal-entry check.

**Required regression:** mutate only the MSR PR bit of the valid
`EnterHid2ReadCall()` result, invoke `ReturnFromHid2Read()`, and require a
decline before copying a measured HID2 value. Keep the supervisor read
and the alternate nonconstant HID2 input tests passing.

## Observed nonclaims and next adversarial targets

The `MemoryWrite32` records are correctly described as ordered events, not
applied guest memory. The native structs do not carry CR, XER, CTR, FPR,
FPSCR, paired PS1, cache completion, actual HID0 readback, or exception
state. No full-state parity claim is supportable at `0x80371730`; the
published bounded PC/LR/GPR and command-operand comparison is narrower.
The supplied `0x00002032` MSR and HID2/HID0 values are synthetic/HLE
observations, not proven retail power-on state. Downstream cache and paired
consumers must validate live readback rather than reusing command operands.

After privilege guards, independently test `mtmsr` effects under other
admissible supervisor MSR inputs and the first post-`sync` hardware state.
Do not infer a game boot or retail initial conditions from the current
prefix.

## Coordinator resolution

`EnterPairedSetupCall()` now declines MSR[PR]=1 before the first `mfmsr`.
`ReturnFromHid2Read()` independently declines it before `mfspr HID2`.
The direct former counterexample now exits nonzero without a later checkpoint
or stack-write event. API negative cases and the dedicated
`boot_pal_user_mode_decline` CLI gate pass; the complete Release CTest passes
**44/44**. These guards decline an exceptional path; they do not implement
the guest program-exception vector or advance the connected boot stop.

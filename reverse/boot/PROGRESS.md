# Native boot execution log — PAL GUPP8P

This log tracks the **connected** native boot prefix. Static maps and callable
research slices do not count as a new checkpoint until original-state and
end-to-end execution comparisons pass. Original fixture:
`main.dol` SHA-256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.

## 2026-09-30 — latest run

| Field | Result |
|---|---|
| Last fully validated checkpoint | Entry `0x80003154` and all 36 instructions of register helper `0x800032B0..0x8000333C`. Native PC/LR stop at `0x80003158`; r1=`0x8060C5F0`, r2=`0x805FA780`, r13=`0x805EC500`. |
| New connected checkpoint reached | None. |
| First stop / original continuation | Native stops **before** call `0x80003158 -> 0x80003400`. Original callee starts `mfmsr` at `0x80003400`, sets FP enable and calls `0x80371714`, `0x80370CDC`, `0x80372838`. |
| Root cause | No original same-run pre-entry MSR/HID0/HID2/L2CR and relevant memory/branch trace is available. The later FPR source `0x805F1F30..3F` is absent from DOL sections. Apploader state 6 clears the containing BSS interval on the ordinary completed DOL path, but later FST placement and external writes depend on IPL state, so final entry bytes are not yet established. |
| Native behavior and evidence | `shadow_boot_probe` runs the exact pinned entry/helper prefix and declines before hardware. `research/APPLOADER_PATH.md`, `HARDWARE_PATH.md` and `OS_STARTUP.md` map the original dependencies. A proposed separate FPR note was rejected by automatic approval review under the earlier gameplay-only path restriction; its evidence was reported in this task instead. |
| Execution evidence | Direct Release `shadow_boot_probe` output: `STOP pc=0x80003158 lr=0x80003158 r1=0x8060C5F0 r2=0x805FA780 r13=0x805EC500`. Full public MSVC Release CTest passed **33/33** against read-only game data, including the new `boot_fpscr_arithmetic` gate; the direct probe was rerun after the gate. |
| Remaining unknowns | Original IPL low memory `[0x80000028]`, final FST destination, hardware SPR values and branch outcomes, pre-entry FPR source after all loader writes, time base and OS device responses, constructor effects, application event outcomes and frame/pixel parity. |

The next attempt must start from the same entry, compare the original state at
`0x80003158` and inside `0x80003400`, then implement only the effects whose
inputs and ordered outputs are known. Run the full prefix and CTest after each
correction. Do not jump to the structurally mapped application event loop to
claim boot progress.

### Diagnosed reference-only bug

The archived `hardware_semantics.cpp` copied the low 32 bits of `f0` directly
to FPSCR for `mtfsf 0xFF,f0` at `0x80370DFC`. The IBM Gekko manual specifies
that FEX and VX are derived summary bits. The corrected helper clears source
bits 1/2 and derives them from the exception flags and enables. The executable
unit regression includes finite `f0=0x3FF0000040000000`: raw-copy yields
`0x40000000`, but the defined FPSCR result is zero. This corrects arithmetic
in a reference-only component and does **not** advance the connected boot PC.
The same helper now declines exceptional PS1 source encodings, including
denormals, rather than silently treating raw source bits as a proven
post-`psq_l` result. The 33/33 full gate and direct boot probe were rerun after
that change.

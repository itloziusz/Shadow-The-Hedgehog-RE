# Native boot frontier — PAL GUPP8P

These are the next proof obligations after the buildable register-startup slice.
`PROVEN` means directly checked against the PAL DOL or SYS bytes; `UNKNOWN`
means the present public evidence does not settle the runtime result. Do not
turn an unresolved value into a default in native code.

| Priority | Question | Current evidence and boundary |
|---|---|---|
| P1 | What guest-visible state is established before DOL entry `0x80003154`? | `research/PREENTRY_STATE.md` independently checks the disc inputs and apploader branches. **PROVEN:** the bounded register helper needs no low-memory reads; header word `[0x430]=0x803E74E0` overlaps DOL text and is not a safe final FST base. **UNKNOWN:** IPL-provided low memory, selected apploader branch, final FST placement and full pre-entry register/memory image; capture the original path. |
| P1 | Which effects of `0x80003400` must survive native translation? | **PROVEN static:** `research/HARDWARE_PATH.md` maps HID2/GQR, FPR, cache and handler calls; the first non-DOL RAM read is `0x805F1F38` at `0x80370CFC`, before CRT zero. The buildable entry slice stops before `0x80003400`. **UNKNOWN:** original source bytes, hardware branch inputs, resulting FPR/FPSCR and observed exception/timing effects; acquire the specified parity oracle. |
| P1 | When and how are CRT zero/copy tables applied? | **PROVEN:** `0x80003340` processes the table at `0x80005544` and zero ranges listed at `0x800055C8`; the DOL header BSS envelope overlaps initialized data6/data7. `REFINED_BOOT_GRAPH.md` records the ranges. **UNKNOWN:** all original pre-entry writes and their observed ordering. Never blanket-zero the header envelope. |
| P1 | Which constructors and indirect targets run before application setup? | **PROVEN static:** text0 calls the event-loop wrapper `0x800510C0`, after constructor walker `0x803796CC`; its recurring phase is mapped in `research/APPLICATION_LOOP.md`. `research/CRT_TO_GAME.md` counts 282 in-text constructor targets. **UNKNOWN:** original runtime order/results of constructors and OS initialization, first event/frame reachability, and complete dispatch effects without a trace. `experimental_native_boot/` is reference-only. |
| P2 | How should disc lookup be represented after boot? | **PROVEN:** the original FST has 3,615 entries (see `BOOT_PROCESS.md`). Native `assets/` has FST/ONE readers. **UNKNOWN:** full game-observed DVD ABI behavior, async completion and offset identity; these remain separate from the bounded entry proof. |

The next implementation should extend one proven original branch or service
with a fixture fingerprint and focused negative tests, then run the full root
CTest gate. A playable or fully boot-equivalent claim requires later runtime
and pixel gates; the current boot probe is only the first entry transition.

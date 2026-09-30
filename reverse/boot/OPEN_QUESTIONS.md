# Native boot frontier — PAL GUPP8P

These are the next proof obligations after the buildable register-startup slice.
`PROVEN` means directly checked against the PAL DOL or SYS bytes; `UNKNOWN`
means the present public evidence does not settle the runtime result. Do not
turn an unresolved value into a default in native code.

| Priority | Question | Current evidence and boundary |
|---|---|---|
| P1 | What guest-visible state is established before DOL entry `0x80003154`? | `boot.bin`, `bi2.bin` and apploader stores are documented in `BOOT_PROCESS.md` and `MINIMAL_BOOT_FOUNDATION.md`. **PROVEN:** the bounded helper needs no low-memory reads. **UNKNOWN:** selected apploader branch, final low-memory/FST placement, and full pre-entry register and memory image; obtain an independent original-path snapshot. |
| P1 | Which effects of `0x80003400` must survive native translation? | **PROVEN:** entry branch at `0x80003158` calls `0x80003400`. The current module stops before it. `DOL_STARTUP.md` maps subsequent OS/CRT calls. Hardware, exception and timing dependencies need address-backed effect inventory and a parity oracle. |
| P1 | When and how are CRT zero/copy tables applied? | **PROVEN:** `0x80003340` processes the table at `0x80005544` and zero ranges listed at `0x800055C8`; the DOL header BSS envelope overlaps initialized data6/data7. `REFINED_BOOT_GRAPH.md` records the ranges. **UNKNOWN:** all original pre-entry writes and their observed ordering. Never blanket-zero the header envelope. |
| P1 | Which constructors and indirect targets run before application setup? | **PROVEN:** text0 target `0x800510C0` follows startup. **UNKNOWN:** its precise role and all intervening constructor/OS effects without trace. `experimental_native_boot/` is reference-only and cannot be treated as a complete executable. |
| P2 | How should disc lookup be represented after boot? | **PROVEN:** the original FST has 3,615 entries (see `BOOT_PROCESS.md`). Native `assets/` has FST/ONE readers. **UNKNOWN:** full game-observed DVD ABI behavior, async completion and offset identity; these remain separate from the bounded entry proof. |

The next implementation should extend one proven original branch or service
with a fixture fingerprint and focused negative tests, then run the full root
CTest gate. A playable or fully boot-equivalent claim requires later runtime
and pixel gates; the current boot probe is only the first entry transition.

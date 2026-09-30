# Native boot frontier — PAL GUPP8P

These are the next proof obligations after the buildable register-startup slice.
`PROVEN` means directly checked against the PAL DOL or SYS bytes; `UNKNOWN`
means the present public evidence does not settle the runtime result. Do not
turn an unresolved value into a default in native code.

| Priority | Question | Current evidence and boundary |
|---|---|---|
| P1 | What guest-visible state is established before DOL entry `0x80003154`? | `research/PREENTRY_STATE.md` and `research/APPLOADER_PATH.md` check disc/apploader instructions. `research/ASM_HARDWARE_VALIDATION.md` records a verified startup-only Dolphin/HLE run with `[0x80000028]=0x01800000`, FST base `0x817E74E0`, and zero FPR source bytes. **UNKNOWN for retail IPL:** its low-memory word, selected FST placement/order and complete pre-entry register/memory image. Keep the HLE oracle conditional. |
| P1 | Which effects of `0x80003400` must survive native translation? | **PROVEN bytes/assembly:** `research/BINARY_BOOT_PREFIX.md`, `BINARY_FPR_PREFIX.md` and `BINARY_CACHE_HANDLER.md` record the bounded helper, FPR and cache/handler instruction words and direct branches; the first non-DOL read is `0x805F1F38` at `0x80370CFC`, before CRT zero. **OBSERVED HLE:** the independent run resolves the paired/L1/L2 branches and handler store through return `0x8000315C`. Native C++ currently reaches only `0x80371714` with supplied MSR. **UNKNOWN:** retail SPR/source inputs, full PS1/cache effects and native parity beyond this point. |
| P1 | What does the HID0 `0x800` write mean? | **PROVEN bytes/ISA:** `0x803725F8/FC` OR and write HID0[ICFI], a self-clearing I-cache invalidate command when ICE is enabled (`research/BINARY_BOOT_PREFIX.md`, `GEKKO_PREFIX_SEMANTICS.md`). **OBSERVED HLE:** written word `0x0011CC64`, later HID0 readback `0x0011C464`. The earlier persistent-OR interpretation was corrected. **UNKNOWN:** retail cache/tag timing and precise native replacement after tracing downstream consumers. |
| P1 | When and how are CRT zero/copy tables applied? | **PROVEN bytes:** `research/BINARY_CRT_PREFIX.md` checks 111 instruction words and 41 descriptor words. `0x80003340` processes the table at `0x80005544` and zero ranges listed at `0x800055C8`; the DOL header BSS envelope overlaps initialized data6/data7. **UNKNOWN:** all original pre-entry writes and their observed ordering, and connected native CRT parity. Never blanket-zero the header envelope. |
| P1 | Which constructors and indirect targets run before application setup? | **PROVEN static:** text0 calls the event-loop wrapper `0x800510C0`, after constructor walker `0x803796CC`; its recurring phase is mapped in `research/APPLICATION_LOOP.md`. `research/CRT_TO_GAME.md` counts 282 in-text constructor targets; `research/OS_STARTUP.md` maps the preceding OS calls. **UNKNOWN:** original runtime order/results of constructors and OS initialization, first event/frame reachability, and complete dispatch effects without a trace. `experimental_native_boot/` is reference-only. |
| P2 | How should disc lookup be represented after boot? | **PROVEN:** the original FST has 3,615 entries (see `BOOT_PROCESS.md`). Native `assets/` has FST/ONE readers. **UNKNOWN:** full game-observed DVD ABI behavior, async completion and offset identity; these remain separate from the bounded entry proof. |

The next implementation should extend one proven original branch or service
with a fixture fingerprint and focused negative tests, then run the full root
CTest gate. A playable or fully boot-equivalent claim requires later runtime
and pixel gates; the current boot probe is only the first entry transition.

# PAL GUPP8P boot archaeology index

The read-only `main.dol` binary (SHA-256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`)
is the authority for DOL instruction identity. The retail IPL handoff is a
separate, unresolved input. An archived startup-only disc gave a repeatable
Dolphin/HLE PPC trace; it is a conditional validation oracle for that launch
mode and does not establish retail pre-entry state.

## Evidence ladder

For each region, record: file offset and VA; four raw bytes and BE word;
encoding class and fields; independently decoded PPC instruction; CFG edge;
register, SPR and memory input provenance; ordered state and memory effects;
downstream readers; Gekko architectural and hardware consequences; portable
semantic projection; native test; and unresolved dependencies. An unknown
pointer, branch input or hardware read stops forward native translation.

`B` = raw bytes/word and bitfields checked, `A` = instruction and CFG effects
checked, `S` = same-run state/branch observation, `N` = connected native C++
state comparison for the modeled fields. A stage marked `partial` is not a
completed proof. Full equivalence also requires downstream consumer, memory,
hardware and pixel/runtime gates.

`tools/verify_binary_note.py` is the executable raw-byte gate. Four PAL notes
now check 568 DOL instruction rows, all 90 direct branch targets in those
rows, and 32 SPR encodings. A missing branch annotation or wrong row count
fails. The checked regions are bounded; this is not complete boot coverage.

| Region and category | B | A | S | N | Evidence and next boundary |
|---|---|---|---|---|---|
| DOL entry `0x80003154`, register helper `0x800032B0..0x8000333C` — CRT/CPU | yes | yes | HLE PC/GPR/LR | PC/GPR/LR | `research/BINARY_BOOT_PREFIX.md`, `MINIMAL_BOOT_FOUNDATION.md`; other incoming state is preserved but not represented by `StartupState`. |
| Direct call `0x80003158 -> 0x80003400` — CPU | yes | yes | HLE PC/GPR/LR | PC/GPR/LR | Raw relative branch and link bit tested; native stops before callee without supplied MSR. |
| Wrapper `0x80003400..0x80003410` — CPU/OS | yes | yes | HLE PC/LR/r0/r31/MSR | those fields with supplied MSR | `research/BINARY_BOOT_PREFIX.md`, `ASM_HARDWARE_VALIDATION.md`; native stops at `0x80371714`. `mtmsr` consequences beyond modeled fields require consumer analysis. |
| Paired setup `0x80371714..0x80371764` and HID accessors — CPU/cache | yes | yes | HLE HID2 via GPR, HID0 readback, stack | no | `research/BINARY_BOOT_PREFIX.md`, `ASM_HARDWARE_CFG.md`, `GEKKO_PREFIX_SEMANTICS.md`. HID0 ICFI is a self-clearing command; no persistent-OR shortcut. |
| FPR/FPSCR `0x80370CDC..0x80370E00` — CPU | yes, 74 words | yes for opcodes/branch; PS1 effect open | HLE branch and PS0/FPSCR; PS1 unavailable | no | `research/BINARY_FPR_PREFIX.md`, `ASM_HARDWARE_VALIDATION.md`; full two-lane and exceptional state validation remain. |
| Cache and handler bounded routine/helper/accessor paths — CPU/OS | yes, 305 words | yes for recorded instructions/branches; cache consequence open | HLE selected branches, L2CR, slot | no | `research/BINARY_CACHE_HANDLER.md`, `ASM_HARDWARE_VALIDATION.md`; hardware poll/cache consequences and selector-16 inputs remain. |
| CRT entry/walker/fill leaf and descriptors — CRT/memory | yes, 111 instruction and 41 descriptor words | yes for recorded code and direct branches | no full boundary comparison | no | `research/BINARY_CRT_PREFIX.md`, `ASM_CRT_ENTRY.md`; stack/zero intervals mapped, connected native depends on hardware return. |
| OS `0x80370E68` onward — OS/device/timing | partial | partial | no | no | `research/OS_STARTUP.md`; live time base, pre-entry words and device responses remain. |
| Constructor walker `0x803796AC..0x8037971C` and table — C++ runtime/game | partial | yes for walker and first 12 bodies | no | no | `ASM_CONSTRUCTORS.md`; 282 table pointers checked, effects for indices 12–281 unresolved. |
| RenderWare, GX, audio and game application transition — middleware/game | partial | partial | no | no | `research/CRT_TO_GAME.md` and other subsystem docs are navigation only until binary-first region records and connected tests exist. |

The next connected native work starts from the existing `0x80371714` stop.
Read the raw record and the Gekko ICFI/cache semantics before coding. Capture
the first same-run state and RAM-write divergence, fix its producer, then run
from `0x80003154` and the complete CTest suite again.

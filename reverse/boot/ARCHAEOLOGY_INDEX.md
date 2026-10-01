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
rows, and 32 SPR encodings. An independent 29-word paired-setup note overlaps
the entry note and has its own gate. Exact expected VA spans now reject
omitted or substituted rows; the negative gate proves a valid-word swap fails.
The checked regions are bounded; this is not complete boot coverage.

| Region and category | B | A | S | N | Evidence and next boundary |
|---|---|---|---|---|---|
| DOL entry `0x80003154`, register helper `0x800032B0..0x8000333C` — CRT/CPU | yes | yes | HLE PC/GPR/LR | PC/GPR/LR | `research/BINARY_BOOT_PREFIX.md`, `MINIMAL_BOOT_FOUNDATION.md`; other incoming state is preserved but not represented by `StartupState`. |
| Direct call `0x80003158 -> 0x80003400` — CPU | yes | yes | HLE PC/GPR/LR | PC/GPR/LR | Raw relative branch and link bit tested; native stops before callee without supplied MSR. |
| Wrapper `0x80003400..0x80003410` — CPU/OS | yes | yes | HLE PC/LR/r0/r31/MSR | those fields with supplied supervisor MSR | `research/BINARY_BOOT_PREFIX.md`, `NATIVE_PREFIX_ADVERSARIAL_29.md`; the user-mode `mfmsr` path now declines, while other `mtmsr` consequences need consumer analysis. |
| Paired setup `0x80371714..0x80371764` and HID accessors — CPU/cache | yes | yes for bounded words | HLE stack/return/GQR/HID2, separate cache reset/refill | connected under explicit immutable native profile | `research/NATIVE_SYNC_COMPLETION_37.md`: committed owned bytes, paired enable/visibility and derived HID0; three full entry replays pass the tail. Old HLE DCFI input declines, and legacy request-only stop stays `80371730`. Physical timing/retail profiles remain unresolved. |
| FPR/FPSCR `0x80370CDC..0x80370E00` — CPU | yes, 74 words | yes for supported finite/zero paired edge | both lanes/status in six standalone and three entry runs | connected under same bounded profile | `HIDDEN_BOOT_STATE_36.md`, `NATIVE_SYNC_COMPLETION_37.md`: 4,896 earlier projection fields plus new full-chain comparisons. All-word mutations and FPSCR bug regression. Physical format, exceptional inputs and skipped paired edge remain unresolved. |
| Cache and handler bounded routine/helper/accessor paths — CPU/OS | yes, 305 words | enabled ICE/DCE prefix complete; later consequence open | HLE branch consumers, both later L2 polls | connected through `80372894` only | `NATIVE_SYNC_COMPLETION_37.md` compares owned frame writes and actual derived HID0 readers/masks/branches. Stop before live L2CR call. `L2_STATUS_ORACLE.md` zero polls do not prove retail timing/tags. |
| CRT entry/walker/fill leaf and descriptors — CRT/memory | yes, 111 instruction and 41 descriptor words | yes for recorded code and direct branches | HLE return r4/r31 checked, XER RSP field stale | no | `research/BINARY_CRT_PREFIX.md`, `CRT_MEMORY_EXECUTION.md`; return projection has its own negative test, connected native still depends on hardware return. |
| OS `0x80370E68` onward — OS/device/timing | partial, 63 new raw words | bounded clock/EE CFG | no connected state | no | `research/OS_STARTUP.md`, `OS_ENTRY_NEXT_BOUNDARY.md`; live time base, low-memory offset `0x800030D8/DC`, pre-entry words and device responses remain. |
| Constructor walker `0x803796AC..0x8037971C` and table — C++ runtime/game | partial | yes for walker and first 16 bodies; bounded structural scan of 16–281; exact global-copy graph for eight NPC MotionImpl bodies | no | bounded copy projection only | `ASM_CONSTRUCTORS.md`, `CONSTRUCTOR_NEXT_TARGETS.md`, `SEMANTIC_RECOGNIZER_33.md`, `MOTION_TABLE_CONSTRUCTORS_34.md`; 282 static table pointers checked. Live targets, source values, entry/return deltas and connected effects remain unresolved. |
| RenderWare, GX, audio and game application transition — middleware/game | partial | partial | no | no | `research/CRT_TO_GAME.md` and other subsystem docs are navigation only until binary-first region records and connected tests exist. |

The next connected immutable native work starts at `0x80372894`, before the
live L2CR read. Read checkpoint 37 and the L2 raw records before coding.
The older request-only profile still stops at `0x80371730`; its HLE DCFI
input cannot silently enter the new backend. Capture
the first same-run state and RAM-write divergence, fix its producer, then run
from `0x80003154` and the complete CTest suite again.

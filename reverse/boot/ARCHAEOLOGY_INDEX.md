# PAL GUPP8P boot archaeology index

The read-only `main.dol` binary (SHA-256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`)
is the authority for DOL instruction identity. The retail IPL handoff is a
separate, unresolved input. An archived startup-only disc gave a repeatable
Dolphin/HLE PPC trace; it is a conditional validation oracle for that launch
mode and does not establish retail pre-entry state.

## Evidence ladder

Latest research42 adds independent original apploader and prefix work producers,
continuous clock phase/cached/global parity and a complete event lifecycle audit.
See `research/CLOCK_PRODUCTION_RESEARCH_42.md`: the source-specific arithmetic
connection is executed. Producer-bound fresh DTK research stops before GPU
AllowSleep; the unbound mode still stops before the live DTK logging read.
Original Entry/callee receipts now validate an11-word native frame slice.
Neither slice nor declared-control completion establishes a complete native
event/device owner or a clock/scheduler connection.
Production remains before80379628. F0/copy/alarm/DEC facts are static or
conditional-source evidence until their connected execution gates pass.

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

`tools/verify_binary_note.py` is the executable raw-byte gate. Six PAL notes
now check734 DOL instruction rows, all121 direct branch targets in those
rows, and40 SPR encodings. An independent 29-word paired-setup note overlaps
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
| Cache and selector1 handler paths — CPU/OS | yes, 305 words | L2 predicates/reads, EE masks, registry and real epilogues checked | varied inputs; independent reference comparisons | bounded owner connected through hardware return | `NATIVE_L2_COMPLETION_38.md`, `NATIVE_CRT_COMPLETION_39.md` and audits. Explicit old-slot input; unknown stops at803733B4. No physical cache, retail or asynchronous delivery claim. |
| CRT entry/walker/fill leaf and descriptors — CRT/memory | yes, 111 instruction and41 descriptor words | exact identity descriptors, zero-group/remainder and carry | full three ranges, hostile seeds and boundary canaries | connected from entry through80003188 | `NATIVE_CRT_COMPLETION_39.md`:153 original new points,1473816 full byte comparisons,102 ordered stores. Repeated in all six checkpoint40 profiles. No blanket BSS zero. |
| BI2/debug/relocation/metadata `80003188..3264` — CRT/metadata | yes, registered ledger | two fresh reads, all admitted branch/alias/CTR effects | six complete live-blob profiles; resolved context stop | connected ordinary/debug4 paths | `BINARY_BI2_OS_PREFIX_40.md`, `NATIVE_BI2_COMPLETION_40.md`, two independent audits. New191 points/22156 fields/1564672 BI2 bytes. Debug2/3 and null fallback stop before unknown effects. |
| OS/clock prologues — OS/EE | yes, registered ledger plus static later clock record | produced guard, applied frames, EE disable | connected state and stack aliases | through80379628, before first TBU | `NATIVE_BI2_COMPLETION_40.md`, `OS_ENTRY_NEXT_BOUNDARY.md`. Research41/42 proves bounded clock units/wrap/phase and offset consequences; production source ownership and later OS/device responses remain UNKNOWN. |
| Clock sampler/offset/return/store and next frame — timing/OS | yes,71 words plus complete loaded-text inventory | exact TB halves, signed CR/SO, carry, MSR and aliases | four rollover/stepped profiles; three continuous derived-work captures | research41:182 CP; research42:129 clock/117 machine CP; production remains80379628 | `CLOCK_RESEARCH_41.md`, `CLOCK_PRODUCTION_RESEARCH_42.md` and independent audits. Before80373AC4 in research. Native event/progressive-memory ownership and physical elapsed conversion remain UNKNOWN. Fresh source Clear owns F0 zero conditionally, not for restored/retail state. |
| Original apploader Entry81200258..260 and nested8120039C..3B4 — pre-entry/runtime | yes,11 words plus next raw boundary | SP/LR/store operands, call target and ascending stmw | first-three8 rows/224 fields; nested18 rows/612 fields;10 ordered stores, clear/code/mapping provenance | executed finite research22 snapshots/10 stores; no clock/scheduler; native connector closed | `APPLOADER_ENTRY_RECEIPT_42.md`, `OWNER_RECEIPT_ADVERSARIAL_42.md`. Before812003B8; next unowned range is callback publication at812003C4. Current event Complete is declared-only. Reset extras are source-derived, not separately observed. |
| Fresh source first Advance — reference timing/device boundary | source/library pins, not DOL game code | due/FIFO queue and callback branch/effect chains | complete capture audit; exported init/queue/VI rows; DTK ctor/writer/read/config receipts | owned fresh DTK stops GPU AllowSleep; original unbound stops Mixer.cpp:253; declared-control completion only | `DTK_OWNER_RECEIPT_42.md`, `INITIAL_EVENT_OWNER_42.md`, independent audits. Live worker/async/CP, Movie/frame-step/achievement ownership remains UNKNOWN. Enabled dump stops at IsAudioDumpStarted before host time/WAV. No source hardware callback is promoted to native game behavior. |
| Constructor walker `0x803796AC..0x8037971C` and table — C++ runtime/game | partial | yes for walker and first 16 bodies; bounded structural scan of 16–281; exact global-copy graph for eight NPC MotionImpl bodies | no | bounded copy projection only | `ASM_CONSTRUCTORS.md`, `CONSTRUCTOR_NEXT_TARGETS.md`, `SEMANTIC_RECOGNIZER_33.md`, `MOTION_TABLE_CONSTRUCTORS_34.md`; 282 static table pointers checked. Live targets, source values, entry/return deltas and connected effects remain unresolved. |
| RenderWare, GX, audio and game application transition — middleware/game | partial | partial | no | no | `research/CRT_TO_GAME.md` and other subsystem docs are navigation only until binary-first region records and connected tests exist. |

The next connected immutable native work starts at `0x80379628`, before the
first live time-base read. Read research42c, research41 and checkpoint40 together. Clock
research does not promote this frontier: prove the native source-time/event
contract before coding a connected continuation.
The older request-only profile still stops at `0x80371730`; its HLE DCFI
input cannot silently enter the new backend. Capture
the first same-run state and RAM-write divergence, fix its producer, then run
from `0x80003154` and the complete CTest suite again.

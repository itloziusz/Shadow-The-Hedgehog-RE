# HID0 invalidate request → `sync`: independent boundary audit

**Scope:** PAL GUPP8P `main.dol` at `0x8037172C..0x80371764`, its HID0 leaf `0x803725F4..0x80372600`, and the first connected HID0 branch consumers at `0x80372854..0x80372880`. The connected native probe currently stops **before** `sync` at `0x80371730`. This is an instruction/cache-state audit, not a C++ implementation or a retail-console trace. Original read-only DOL SHA-256: `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.

Confidence labels: **PROVEN-BINARY** means independently re-read DOL bytes and decoded fields; **PROVEN-MANUAL** means the [IBM Gekko User's Manual v1.2](https://doc.kodewerx.org/documents/gekko_user_manual.pdf) specifies the effect; **OBSERVED-HLE** means the archived startup-only Dolphin interpreter capture, not retail IPL or silicon; **UNKNOWN** means no equivalent evidence exists for the live input or hardware effect.

## Byte → instruction → state

The DOL text section maps file `0x002600` to VA `0x80008D40`; all offsets below are `0x2600 + VA - 0x80008D40`. I re-read these words directly from the SHA-pinned file, rather than copying a disassembler listing. `mfspr/mtspr` decode the split field as `((w >> 16) & 31) | (((w >> 11) & 31) << 5)`, yielding SPR1008 for HID0 and SPR912–919 for GQR0–7. Opcode 31/XO598 is `sync`, exact encoding `7C0004AC`.

| VA / file | Raw bytes | Decoded local effect |
|---|---|---|
| `8037172C` / `36AFEC` | `48 00 0E C9` | `bl 0x803725F4`; LR=`0x80371730`. |
| `803725F4` / `36BEB4` | `7C 70 FA A6` | `mfspr r3,HID0`; read live HID0 into r3. |
| `803725F8` / `36BEB8` | `60 63 08 00` | `ori r3,r3,0x800`; no CR/XER change. |
| `803725FC` / `36BEBC` | `7C 70 FB A6` | `mtspr HID0,r3`; issue *full* HID0 write word. |
| `80372600` / `36BEC0` | `4E 80 00 20` | `blr` to `0x80371730`. |
| `80371730` / `36AFF0` | `7C 00 04 AC` | `sync`; no architected GPR, FPR, CR, XER, LR, CTR or FPSCR output. |
| `80371734` / `36AFF4` | `38 60 00 00` | `li r3,0`. |
| `80371738..54` / `36AFF8..36B014` | `7C70E3A6` through `7C77E3A6` | Eight `mtspr GQR0..GQR7,r3` writes, in increasing register order. |
| `80371758..64` / `36B018..36B024` | `8001000C 38210008 7C0803A6 4E800020` | Restore LR from old stack `+4`, restore r1 by 8, `blr` to wrapper `0x80003414`. |
| `80372854..60` / `36C114..36C120` | `4BFFE299 54600420 28000000 40820014` | Read HID0 via `0x80370AEC`; test numeric bit `0x8000` (ICE); branch to `0x80372874` if set. |
| `80372874..80` / `36C134..36C140` | `4BFFE279 54600462 28000000 40820014` | Read HID0 anew; test numeric bit `0x4000` (DCE); branch to `0x80372894` if set. |

**PROVEN-BINARY:** Let `H` be the first HID0 read at `0x803725F4`. The issued write operand is `H | 0x800`, and r3 retains that operand at `0x80371730`; the leaf does not write ordinary memory, FPRs or GQRs. `sync` does not change those registers. It is therefore wrong to infer the later *HID0 register* from r3 alone. The earlier stack stores at `0x80371718/1C` are pending effects that also fall before this barrier.

## Hardware consequence and counterexamples

**PROVEN-MANUAL:** HID0 numeric `0x00000800` is IBM bit 20, ICFI. The manual's HID0 Table 2-4 and §3.4.1.4 specify a flash invalidate of L1 instruction-cache valid/PLRU state when HID0[ICE] is enabled. Cache access is blocked during the invalidate; ICFI automatically clears in the next cycle when ICE is enabled. The manual does **not** license treating the command bit as a persistent software flag. It says ICE must be enabled for the invalidation; ICE=0 at power-up. Exact retained ICFI/readback behavior for an ICE=0 write followed by a later ICE enable is not established by this trace. The later optional ICE setter at `0x80372604..14` rereads and rewrites the *whole* HID0 word, so retaining or clearing ICFI prematurely can alter its write operand.

**PROVEN-MANUAL:** The preceding `mtspr HID2` at `0x80371728` forces LSQE and PSE. HID2 Table 2-6 explicitly requires instruction-cache invalidation after newly setting those enable bits and before using the corresponding graphics instructions. The actual order is HID2 write → ICFI request → `sync` → GQR writes → paired-single instructions in the following callee. This explains the sequence, but does not measure its hidden cache tags.

**PROVEN-MANUAL:** `sync` (instruction entry on p. 12-239) completes only when earlier instructions appear complete and prior external accesses have been performed with respect to other memory-access mechanisms; no later instruction begins before it completes. It is execution synchronizing and changes no architectural register. HID0[ABE], IBM bit 28/numeric `0x8`, controls whether `sync` is broadcast on the 60x bus; with ABE=1, the manual says completion waits for a successful broadcast (HID0 Table 2-4, p. 2-12). Thus a universal native `PC += 4` or host compiler fence is insufficient evidence for the Gekko barrier. The manual does not give this audit a measured completion timestamp or an independently captured cache-tag state.

**OBSERVED-HLE:** `build/agent-boot-hardware-capture.json` records synthetic startup-only HID0=`0x0011C464` before the leaf (ICE=1, DCE=1, ICFI=0, ABE=0). At PC `0x80371730`, r3=`0x0011CC64` while the exposed HID0 register is again `0x0011C464`. At the later branch checkpoints `0x80372860/80`, the preceding live HID0 accessor calls have returned `0x0011C464`, resulting in nonzero ICE/DCE masks and the corresponding taken branches. This independently refutes a model that keeps `0x0011CC64` as the post-write HID0 word for this selected run. It does not prove retail initial HID0, cache-tag changes, physical barrier timing, or other startup paths.

## Decision at the native stop

**Fail closed at `0x80371730` for a *validated connected checkpoint*.** Merely emitting `ICFI_REQUEST` or `SYNC_REQUEST` and advancing would leave an unacknowledged effect that governs the next PPC instruction fetch, the later live HID0 reads, and, depending on HID0[ABE], external-bus ordering. The current probe has the *write operand* and no completed-cache/barrier interface or post-command HID0 state. Calling `0x80371734` a parity checkpoint on that basis would be fake parity.

There is a narrower possible **conditional implementation path**, not yet a validated advance: a platform contract can accept the exact live pre-write HID0 and MSR, issue the ICFI operation, provide a completion acknowledgment for `sync`, and expose a separately derived *post-command* HID0 readback. Its native x64 implementation may semantically collapse guest instruction-cache tags if (a) the corresponding translated code is immutable over this interval, (b) no guest instruction visibility/exception effect is lost, and (c) all later HID0 reads, cache branches and ordered memory consumers compare against a same-run reference. For a synthetic ICE=1/ABE=0 case, the manual and HLE readback support a targeted candidate; they do not justify a retail-general constant. Unknown ICE=0, ABE=1, interrupted, or self-modifying paths must remain explicit conditional declines until separately proven.

The next proof step is therefore a *stateful* completion boundary, not a log-only event: capture `0x80371730` then `0x80371734` and `0x80372854/74` in one reference run, retain the r3/HID0 distinction, compare stack-memory visibility and GQR writes, and adversarially test ICE=0 and ABE=1 inputs. Re-run the full native prefix from `0x80003154` and the full CTest suite before claiming progress.

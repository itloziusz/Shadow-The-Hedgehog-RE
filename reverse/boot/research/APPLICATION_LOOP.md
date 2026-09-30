# PAL GUPP8P application loop, bounded native slice

Source: read-only `main.dol`, SHA-256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`. The instructions below were checked with `q.py dis` and independently mapped through the DOL section header; the full fixture hash is enforced by CTest. `docs/renderware/evidence/03_closure.md` independently documents the downstream frame graph. **PROVEN** here denotes static instruction/control-flow facts. Live boot and frame presentation remain **UNKNOWN** without an original runtime trace.

## Exact recurring path

| Confidence | Original address | Effect |
|---|---|---|
| PROVEN | `0x800032A8` | Startup calls `0x800510C0` after the OS/constructor path. The earlier setup can fail or branch. |
| PROVEN | `0x800511E0` | Unconditional branch to `0x800511F4`; the exit flag is tested before the first recurring event. |
| PROVEN | `0x800511E4..0x800511EC` | Dispatches event `0x12` with null argument through `0x80051978`. The return value is ignored by this loop. |
| PROVEN | `0x800511F0` | Calls `0x8032D444` after dispatch, even if dispatch set the exit flag. Its complete side effects are not lifted. |
| PROVEN | `0x800511F4..0x800511FC` | Repeats while the word at `0x80576DBC` is zero. Any nonzero word exits. |
| PROVEN | `0x80051200..0x80051214` | On normal loop exit, dispatches events `0x0E` then `0x11`, each with null argument, then returns zero. |

`0x80051978` calls prefilter `0x80050FA8`, then, on its value 2 branch, `0x80048EB8`. The event `0x12` jump-table slot at `0x8051E628` targets case slice `0x80049078`. That slice calls `Game_FrameStep` at `0x801E2210`; if the returned low byte is zero it sets `0x80576DBC` to 1 at `0x80049090`. Separately, event `0x15` sets that exit word at `0x800519C0`. **PROVEN static route; UNKNOWN live reachability and callback state.** `0x801E23A4` and `0x8004ECAC` lead into the documented task traversal conditionally; this does not prove a displayed frame or pixel gate.

The initialization part `0x800510C0..0x800511DC` sets fields, emits events `0x10`, `0x1C`, `0x0D`, `0x01`, and `0x00`, and uses OS/clock services. Those operations are not included in the native function. Their argument handling, callback effects, clock values, failure branches and original runtime outcomes require separate proof and tests.

## Native boundary and checks

`include/shadow/boot/ApplicationLoop.hpp` and `src/ApplicationLoop.cpp` translate exactly the recurring and normal-exit slice `0x800511E0..0x80051218`. `ApplicationLoopHooks` exposes the event dispatcher, post-event service and exit word explicitly. They are unresolved engine hooks; the native slice is not an executable game main function. The loop is entered only after successful original-equivalent initialization.

The `boot_pal_application_loop` CTest requires the exact DOL SHA-256, checks the source branch/call/load/event instruction words, and verifies zero, one and three event passes. It also verifies that a dispatch which sets the exit word is still followed by the service call before the next exit check. A separate checked fixture regression pins the `0x803733C0` hardware handler store path to `0x80586CB4`, correcting an older diagnostic's wrong address.

For full parity, capture an original boot through startup into `0x800510C0`, including event return values, callbacks, `0x80576DBC`, the call to `Game_FrameStep`, task traversal, and eventual VI/XFB presentation. Compare state/event traces at the same original addresses before claiming a native boot or pixel match.

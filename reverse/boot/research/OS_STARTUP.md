# PAL GUPP8P OS startup from `0x80370E68`

Scope: the original `sys/main.dol` (SHA-256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`),
from the direct call at `__start+0x110` (`0x80003264`) until its return to
`0x80003268`. This is a static call and dependency audit. No original-path
execution trace or device response was available. The earlier
`experimental_native_boot/src/runtime_route.cpp` is a reference-only
prelude projection; it explicitly leaves the OS body unexecuted.

## First-call guard and prelude

**PROVEN:** `0x80370E80` reads word `0x805F1F40`; `0x80370E98`
branches to the epilogue if it is nonzero. The preceding CRT routine
`0x80003340` clears `0x805EF020..0x805F277C`, including this word, so a
normal first call after that routine returns takes the initialization body.
`0x80370E9C..0x80370EA0` sets the word to one. The application wrapper
at `0x80051028` calls this initializer again from `0x80051040`; the guard
provides a fast return then if nothing has reset the word. This is a
conditional statement, not proof that application entry was reached.

**PROVEN:** the first body call at `0x80370EA4` invokes `0x80379648`.
That helper disables interrupts (`0x8037611C`), obtains a stable 64-bit
time-base read (`0x80379628`), adds low-memory words
`0x800030D8/0x800030DC` (`0x80379674/0x80379670`), restores the prior
interrupt state (`0x80376144`), and returns the resulting pair into
`0x805F1F50/+54` (`0x80370EA8..0x80370EAC`). **UNKNOWN:** original time
base and pre-entry offset words, so this stored pair has no static value.

### First divergence for a fail-closed native prefix

This log is **conditional** on independently satisfying the earlier
`0x80003400` hardware and `0x80003340` CRT gates. It describes where a
native continuation could advance and where it must decline today:

```text
PROVEN  0x80003264 -> 0x80370E68     direct call; prior CRT returned
PROVEN  0x80370E80                   guard 0x805F1F40 was CRT-cleared
PROVEN  0x80370EA0                   first-call branch writes guard = 1
STOP    0x80370EA4 -> 0x80379648     original OS clock/interrupt effect unavailable
READ    0x8037611C                   current MSR, especially prior EE state
READ    0x80379628..0x80379638      original stable time-base pair
READ    0x80379670/+74              pre-entry 0x800030DC/+D8
CHECK   0x80370EA8/+AC              compare original 0x805F1F54/+50 writes
```

**UNKNOWN:** no original observation supplies those reads or the call's
ordered interrupt-state changes. Do not use a fixed clock, zero offset, or
assumed interrupt bit to pass this boundary. The minimum oracle is an
original same-run snapshot of MSR/EE, the time-base sample or a trace of
the helper loop, both low-memory offset words, and the two stored result
words with call/return order. The next independent stop is the
`0x800030F0` source-pointer decision at `0x80373AC4`; its source bytes
and resulting `0x80586C90..AB` image also need a same-run observation.

**PROVEN:** after another interrupt disable at `0x80370EB0`,
`0x80370EB8` calls `0x80373AB4` with destination `0x80586C90`. It reads
low-memory pointer `0x800030F0` at `0x80373AC4`. If that pointer compares
at or above `0x80000000`, the helper copies `0x1C` bytes from it; otherwise
it clears only the first destination word (`0x80373AC8..0x80373AE0`).
**UNKNOWN:** the pointer and source bytes at original entry; treating the
result as a 28-byte zero block would be unsupported.
The initializer does not restore that prior interrupt state in this prelude;
it calls the enable helper `0x80376130` at `0x803712D8` after the device
and arena work. Callback ordering across that interval needs observation.

**PROVEN:** `0x80370EBC..0x80370EF0` writes zero through six performance
monitor SPR setters (`0x80370B30..0x80370B58`), changes a cache-related
SPR via `0x80370BC0`, and sets an FPSCR bit via `0x80370BE8`. The exact
game-observable outcome of these hardware writes is **UNKNOWN** without a
trace or a native replacement contract.

## Low-memory and arena branch lattice

**PROVEN:** `0x80370EF8..0x80370F68` stores boot-info base `0x80000000`
in `0x805F1F20`, then reads BI2 pointer `0x800000F4` at `0x80370F08`.
For nonzero BI2 pointer it saves `BI2+0x0C` as the debug-field pointer,
reads `BI2+0x24`, and copies low bytes of those two words to
`0x800030E8/+E9` (`0x80370F14..0x80370F3C`). For zero BI2 pointer it
conditionally reads `0x80000034` and preexisting bytes
`0x800030E8/+E9` (`0x80370F44..0x80370F64`). The branch chosen and
resulting bytes are **UNKNOWN** without the original pre-entry image.

**PROVEN:** arena low is selected from `0x80000030` at `0x80370F74`;
zero substitutes `0x8060E600` (`0x80370F80..0x80370F88`). A second
conditional pass (`0x80370F90..0x80370FCC`) replaces it with
`ALIGN32(0x8060C5F0) = 0x8060C600` only when the original boot-info
arena-low word is zero and the selected debug-field word is below two.
Both paths write through `0x8037222C` to `0x805EEBE8`.
Arena high comes from `0x80000034` at `0x80370FD0`; zero substitutes
`0x817A0000` at `0x80370FDC..0x80370FE0`, then `0x80372224` stores it
to `0x805F1F70`. These are branch formulas, not asserted PAL results.

## Ordered direct calls in the first initialization body

The addresses in each row are the **PROVEN** direct calls from the
`0x80370E68` instruction stream. Names below describe observed effects,
not recovered original function names. Return, device, or nested callback
behavior is not inferred from call presence.

| Call site(s) | Direct target(s) | Checked effect or unresolved boundary |
|---|---|---|
| `0x80370FEC..0x8037100C` | `0x80371348`, `0x803782BC`, `0x803717A8`, `0x80376988`, `0x80376198`, `0x80376168` | Exception-vector installation, context copy, handler setup, three low-memory zeros at `0x800030C8/+CC/+D0`, interrupt setup and handler registration. `0x80371348` branches on pre-entry word `0x80000060` and debugger state; exact vector image is conditional. |
| `0x80371010..0x80371028` | `0x80373184`, `0x80372838`, `0x803B4CFC`, `0x803B633C`, `0x803776F4`, `0x80378324`, `0x80372260` | FPU exception hook, cache/handler setup, and hardware/device initialization. `0x803B4CFC` polls registers based at `0xCC006800` (`0x803B4D20..0x803B4D4C`); `0x803B633C` polls `0xCC006400` (`0x803B638C..0x803B6398`); `0x80372260` has several waits on `0xCC005000` (`0x803722CC..0x803723F0`). Their return is not guaranteed by DOL bytes. `0x80378324` makes an indirect call via `0x805EEC10` at `0x803783D0..0x803783DC`. The loaded DOL word there is `0x80378320`; its value at the call remains a runtime property. |
| `0x8037102C..0x80371044` | `0x80370BA8`, `0x80370BB0`, optional `0x80376B60` | HID2 read/modify/write, then optional hardware/interrupt initialization according to `0x805F1F48`. That helper reads low-memory `0x800000F0` and `0x80000028` at `0x80376B74`, `0x80376BF4`. |
| `0x80371050..0x803711D4` | repeated `0x803731CC`, `0x80372214`, `0x8037221C`, `0x8037177C`, optional `0x8039FEFC`, `0x803771B8` | Debug reporting and arena query. `0x803771B8` chooses between saved context `0x80586C90/+94` and a hardware register at `0xCC003024`; the following arena-clear branches depend on its result (`0x803711D4..0x803712D8`). |
| `0x803712D8..0x80371328` | `0x80376130`, optional `0x8037B304`, optional `0x80372508`, `0x8037D27C` | Enables interrupts, conditionally initializes a disc/FST path and a local context/thread-like structure. The optional `0x8037B304` checks a boot header word at `0x80000020` (`0x8037B384..0x8037B3C0`) against `0x0D15EA5E` or an alternate magic; one branch calls FST loader `0x8037E114`. |

The row labels are **STRONG** only where they summarize multiple observed
stores and strings. The addresses and call order are **PROVEN**. In
particular, the hardware waits, indirect call, exception vectors, and
conditional arena clear must not be flattened into a success stub.

## Return to constructors

**PROVEN:** after the initializer returns, `__start` reads halfword
`0x800030E6` at `0x80003270`, may call `0x80003100` (which reads
`0x800030E4`), and reads the CRT-cleared debug byte at `0x805F1FF0`
through `0x8000314C` at `0x8000328C`. It can conditionally invoke
`0x8039F978`. The direct constructor-walker call at `0x8000329C`
is `0x803796AC` -> `0x803796CC`; the latter dispatches the 282-entry
`.ctors` table described in `CRT_TO_GAME.md`. **UNKNOWN:** the actual
original runtime path through these post-OS branches and every constructor
effect. No call to `0x800510C0` is justified in a native path until those
effects and return conditions are established.

## Evidence needed for a faithful native boundary

1. Capture `0x800030D8/+DC/+F0`, pointer target, time base, MSR,
   `0x80000020/+28/+30/+34/+60/+F0/+F4`, BI2 bytes, and the selected
   arena/debug state before `0x80370E68`. Include DOL and SYS hashes.
2. Trace ordered call/return events and side effects from
   `0x80370E68..0x80371344`, including first MMIO polling completion,
   interrupt/exception callbacks and the indirect call at `0x803783DC`.
   Compare an original run at exit `0x80003268`: low-memory words, arena,
   vectors, handlers, context structures, timers, and device readiness.
3. Keep hardware-specific operations behind explicit native services only
   after each observable effect and callback order is known. A table of
   direct call addresses alone is not an executable or a parity gate.

## Independent checks

- Queried `q.py dis` and `q.py callees` against the existing original DOL
  caches; inspected the named direct callees and their branch sites.
- Independently read selected PowerPC words in raw `main.dol` text1,
  including `0x80370E80/0x80370E98`, arena branches, direct calls,
  low-memory reads and MMIO polls. No cache or source generator was run.
- Rechecked the predecessor CRT zero interval and successor constructor
  call in the DOL. This is a static evidence check only; no original-path
  runtime parity result is claimed.

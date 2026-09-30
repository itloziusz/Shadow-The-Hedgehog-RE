# PAL boot L2 cache state machine: binary to native boundary

Scope is the L2 decision in `0x80372894..0x803728F8`, its direct helper
`0x80372640..0x803726D4`, and the L2CR/MSR accessors they call. This note
does **not** extend the connected native boot prefix. `PROVEN-BINARY` means
the PAL GUPP8P DOL bytes and their instruction consequences; `OBSERVED-HLE`
means the separate startup-only synthetic Dolphin run. Neither establishes
the retail IPL's incoming state. Original DOL SHA-256:
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.

The complete per-word records for this range already exist in
[`BINARY_CACHE_HANDLER.md`](BINARY_CACHE_HANDLER.md). Text section 1 maps
file `0x2600` to VA `0x80008D40`, so the relevant offset is
`VA - 0x80008D40 + 0x2600`. I separately read the DOL bytes at the anchors
below; no generated IR or mnemonic was used to choose their words.

| VA / file offset | Raw BE word | Decoded effect / independently checked field |
|---|---|---|
| `80372894` / `36C154` | `4BFFE269` | `bl 80370AFC`, read L2CR. |
| `80372898` / `36C158` | `54600000` | `rlwinm r0,r3,0,0,0`, isolate numeric `0x80000000`. |
| `803728A0` / `36C160` | `40820058` | `bne 803728F8` if L2 enable bit is set. |
| `803728A4` / `36C164` | `4BFFE239` | `bl 80370ADC`, read current MSR. |
| `803728B4` / `36C174` | `4BFFE231` | `bl 80370AE4`, write MSR from r3 (`0x30` here). |
| `803728D0` / `36C190` | `4BFFFD71` | `bl 80372640`, enter nested invalidate helper. |
| `80372668` / `36BF28` | `64630020` | `oris r3,r3,0x20`, OR numeric `0x00200000`. |
| `8037267C` / `36BF3C` | `546007FE` | `rlwinm r0,r3,0,31,31`, isolate numeric bit 0. |
| `80372684` / `36BF44` | `4082FFF4` | `bne 80372678`, first live-status poll. |
| `8037268C` / `36BF4C` | `546302D2` | `rlwinm r3,r3,0,11,9`, mask `0xFFDFFFFF`. |
| `803726B8` / `36BF78` | `546007FE` | Isolate numeric bit 0 for second poll. |
| `803726C0` / `36BF80` | `4082FFE8` | `bne 803726A8`, second live-status poll. |
| `803728E4` / `36C1A4` | `540302D2` | Same mask `0xFFDFFFFF` after OR of L2 enable bit. |
| `80370AFC` / `36A3BC` | `7C79FAA6` | `mfspr r3,1017` (split SPR field), L2CR read. |
| `80370B04` / `36A3C4` | `7C79FBA6` | `mtspr 1017,r3`, L2CR write. |

## State transition and CFG

Let `R()` mean a **fresh** L2CR read and `W(v)` a write request; the read
result must not be silently replaced by the last software-written value.
At `0x80372894..A0`, the input is `R()`. `r0=R() & 0x80000000`, then
`cmplwi r0,0` sets CR0. If nonzero, the code jumps directly to
`0x803728F8`; it does not disable, invalidate, or re-enable L2 on that
branch. The following handler registration still executes.

On the zero branch, `0x803728A4..CC` saves the **current** MSR in r30,
executes `sync`, writes `MSR=0x30`, executes two `sync` instructions, clears
L2E in a fresh L2CR read (`R() & 0x7FFFFFFF`), writes it, then `sync`s.
`0x30` is the literal PPC MSR word, not a value inferred from HLE input.
The nested helper performs:

1. `sync`; `W(R() & 0x7FFFFFFF)`; `sync` at `0x80372650..60`.
2. `W(R() | 0x00200000)` at `0x80372664..6C`, requesting global L2
   invalidation while L2E is clear.
3. Repeated `R()` at `0x80372678`: `cmplwi (R() & 1),0` followed by
   `bne 0x80372678` at `0x80372684`. Exit requires a **live** zero status.
4. `W(R() & 0xFFDFFFFF)` at `0x80372688..90`, explicitly clearing the
   request bit. The PPC wraparound mask clears bit 10 only; it retains other
   bits of that read, subject to actual SPR write behavior.
5. After the two direct `b` edges, repeatedly read status at
   `0x803726B4..C0`. A nonzero bit branches to `0x803726A8`, calls the
   local logger, then re-reads. A zero restores LR/r31/r1 and returns.

The first poll exits before the software clear; the second poll is a new
read after that write. A hardware status transition between them is possible
in the architectural model, so the second poll is not removable merely
because the first exited. Neither loop has a software timeout. The logger's
`crxor 6,6,6` clears CR bit 6 before its call; the logger's selected `bne
cr1` skips FPR frame stores. Its temporary GPR/stack effects still exist.

The caller restores `MSR=r30` at `0x803728D4..D8`, then **re-reads** L2CR
at `0x803728DC`. `0x803728E0..E8` writes
`(R() | 0x80000000) & 0xFFDFFFFF`: L2E set, L2I clear, other bits from
this last read preserved by the software expression. The logging call at
`0x803728F4` precedes the common successor `0x803728F8`. This symbolic
form deliberately does not assert that L2IP is writable or that hardware
returns a simple register echo.

The main routine saves LR at old r1+4, then saves r31/r30 in a 16-byte
frame at new r1+12/+8. The helper saves its caller LR at parent r1+4,
then saves r31 in its own 16-byte frame at new r1+12. With the observed
synthetic old r1 `0x8060C5F0`, these are main save addresses
`0x8060C5F4`, `0x8060C5EC`, `0x8060C5E8`, and helper save addresses
`0x8060C5E4`, `0x8060C5DC`. The helper returns r1 to `0x8060C5E0`;
the main routine later returns it to `0x8060C5F0`. The path directly
modifies r0/r3/r30/r31, LR, CR0 and CR bit 6; no instruction in this L2
slice changes XER, CTR, FPR, FPSCR or SDA. The accessors use SPR 1017 and
MSR, not ordinary RAM. Stack and optional logger stores are the ordinary
memory writes; the cache-tag change is a hardware state effect.

## Gekko consequence and observed conditional path

IBM's [Gekko User's Manual, L2CR Table 2-19 and sections 9.1.3–9.1.4](https://doc.kodewerx.org/documents/gekko_user_manual.pdf)
identifies numeric `0x80000000` as L2E, `0x00200000` as L2I, and numeric
`0x1` as read-only L2IP. Global invalidation is an on-chip state machine
which clears tag/status/LRU bits. The manual calls for disabling L2,
ordering with `sync`, setting L2I, polling L2IP, clearing L2I, then enabling
L2. It gives approximately 32K core clocks for invalidation. This makes an
instant zero L2IP in an interpreter trace a conditional branch observation,
not a timing or cache-tag proof for Gekko hardware.

Two fresh synthetic/HLE captures (summarized in
[`ASM_HARDWARE_VALIDATION.md`](ASM_HARDWARE_VALIDATION.md)) show initial
L2CR `0`, so `0x803728A0` falls through. Their measured MSR is `0x2032`
before, `0x30` during, and `0x2032` after this block. At `0x80372678/84`,
L2CR is `0x00200000`, yet `(L2CR & 1)=0`, so the first back-edge is not
taken. At `0x803726B4/C0`, L2CR is `0`, and the second back-edge is not
taken. The intervening guest `mtspr` at `0x80372690` explains this zero;
no hardware auto-clear claim is needed. The readback at `0x803728EC` is
`0x80000000`. GDB exposed the register values and branch checkpoints but
not L2 tags, cycle count, bus visibility or exceptional interleavings.
Retail IPL initial L2CR/MSR, initial cache tags and poll history remain
**UNKNOWN**.

## First consumers and native decision

The first consumer of the helper's outcome is the caller's L2CR read at
`0x803728DC`, which supplies its final enable/configuration word. On the
selected path, the next known boot reader of final L2E is a **later direct
call site** at `0x80371014` (`48001825`, `bl 0x80372838`), which would
take the skip branch at `0x803728A0` if L2E remained set. That OS startup
call is statically mapped but has not been reached by connected native
execution; its input may have changed before arrival. A further candidate
consumer, context-save `mfspr 1017` at `0x8039F6F0` (`7FF9FAA6`), has no
proven normal-boot reachability. The first validated full hardware return
in the synthetic oracle is `0x8000315C`, not a native cache-state proof.

Classification: this slice creates **CPU-specific and GameCube OS/cache
state**. No direct game object or RAM configuration value is computed by
the L2 instructions. A future native platform operation may replace the
hardware mechanism only after its observable consequences are established:
the incoming L2E branch, ordered state-dependent writes and polls, MSR
interval, handler setup that follows, exceptions/ordering relevant to
consumers, and later L2 readback. A native implementation that hard-codes
zero poll status, forces the L2E branch, writes `0x80000000` regardless of
input, or treats Dolphin's instant completion as retail timing would be
false parity. **No portable C++ equivalent is proven yet.**

## Checks and falsifiers

- The repository's PAL `boot_pal_raw_cache_handler` gate verifies 305
  per-word records, 56 direct branches and 14 SPR fields, including all
  words above. This note adds anchor and bitfield checks, not duplicate
  per-word coverage.
- An independent direct byte read reproduced the 15 anchor words. Decoding
  `SH/MB/ME` from both `0x546302D2` and `0x540302D2` yields `(0,11,9)`;
  their mask is `0xFFDFFFFF`. The two signed BD displacements resolve to
  `0x80372678` and `0x803726A8`. Executable assertions confirmed both
  targets and the `0x00200001 -> 0x00000001` mask counterexample.
- Falsify the timing assumption by recording every L2CR read around
  `0x8037266C..0x803726C0` on a retail-path oracle, including a case with
  L2IP high. If the second poll never becomes high after the first exits,
  that may support a later simplification; the DOL and current captures do
  not prove it. Also compare L2CR at `0x803728DC`, `0x803728EC`, and the
  later `0x80371014` call, together with cache-tag/exception consequences
  where exposed.

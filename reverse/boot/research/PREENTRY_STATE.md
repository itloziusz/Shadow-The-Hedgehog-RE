# PAL GUPP8P pre-entry state: static evidence and oracle boundary

2026-09-30. Scope: the original disc/loader handoff immediately **before** the
first instruction at DOL entry `0x80003154`. Addresses in `0x8120....` below are
apploader runtime addresses; its code word at `0x8120xxxx` occurs at file offset
`0x20 + (0x8120xxxx - 0x81200000)` in `apploader.img`. `PROVEN` means checked
against the five raw `sys/` files, not against a console execution. A proposed
value conditional on an IPL input is not an observed final value.

## Checked input identity

The raw extracted files have these SHA-256 hashes:

| File | Bytes | SHA-256 |
|---|---:|---|
| `boot.bin` | 1,088 | `7d387ce9162342a93a3becac1d2df6db1cccf6a2439f3fc97b29b2f7047d8e88` |
| `bi2.bin` | 8,192 | `8bbadffc2a7f79eda5982aa2fe30831969c2702aa9206f621b930aae0b70af6b` |
| `apploader.img` | 122,456 | `8a1fe9487b029b2518a6633b1e0eb9ec0922f5f5eb5afdd7eba1267a85f096fe` |
| `main.dol` | 5,773,024 | `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af` |
| `fst.bin` | 101,145 | `0ceb019b93db37a0b359f0918cd97c262069638fcab62726684d268c09a64db0` |

**PROVEN, raw bytes:** `boot.bin` words at `0x420..0x438` are respectively
`0x00020300`, `0x005A1A00`, `0x00018B19`, `0x00018B19`, `0x803E74E0`,
`0x005C0000`, `0x56A98000`. BI2 at `+0x00/+0x04` contains `0` and
`0x01800000`. The DOL entry is `0x80003154`; its nonempty text1 section covers
`0x80008D40..0x804AAC60` (DOL header text slot 1).

### Correction to the old header-word label

**PROVEN conflict:** `boot.bin[0x430] = 0x803E74E0` lies *inside* loaded DOL
text1. Placing the `0x18B19`-byte FST there would replace DOL code at
`0x803E74E0..0x803FFFF8`. The old `SYS_LAYOUT.json` / `BOOT_PROCESS.md` label
“FST memory address” is therefore unsafe as a **final pre-entry placement**.
The word's producer/actual purpose remains **UNKNOWN**. Apploader state 3
computes and overwrites its internal placement at `0x81201900+0x30`
(`0x81200A20..0x81200A94`), and state 9 consumes that field
(`0x81200FF8`), so native startup must use a measured final value rather than
the header word. This observation does not alter the known FST *disc* offset
`0x005A1A00` or its original bytes.

## What the apploader code actually determines

| Confidence | Site | Observation |
|---|---|---|
| **PROVEN** | `0x81200820..0x8120083C` | AplMain requests 32 bytes at disc offset `0x420` into loader internal `+0x20`. Those bytes include DOL/FST offsets, lengths and the `0x430` word. |
| **PROVEN** | `0x81200898..0x812008B4` | It next requests 32 BI2 bytes at disc offset `0x440` into internal `+0x180`. |
| **PROVEN** | `0x812008C8..0x81200908` | State 3 stores BI2 first word to low memory `0x800000E8`, derives `0x800000EC` from pre-existing `0x80000028` and that word, and stores BI2 second word to `0x800000F0`. Thus the **input** `0x80000028` is essential and is absent from disc files. |
| **PROVEN** | `0x81200990..0x81200AA8` | With BI2 second word nonzero, state 3 uses `0x800000F0` and the derived `0x800000EC` to set internal `+0x30`, then sets `0x800000F4 = internal[+0x30] - 0x2000` for the following full BI2 read. The branch and placement depend on pre-existing low-memory state; no unconditional final address follows from the disc bytes alone. |
| **PROVEN** | `0x812004EC..0x812004F0`, `0x81201224..0x81201254` | Init reads a *pointer* from low memory `0x800030F0`; if it passes the address check, it copies 28 bytes from that pointed-to block into loader internal `+0x14C`, otherwise it stores zero there. State 4 branches on `internal[+0x14C]` at `0x81200ADC..0x81200AE4`. The original pointer, block and selected branch are **UNKNOWN**. |
| **PROVEN** | `0x81200B44..0x81200B70`, `0x81200FEC..0x8120101C` | State 4 derives a FST/DOL order flag at internal `+0x194`. State 9 conditionally requests an aligned FST read at the computed internal `+0x30`, using FST size `internal[+0x28]` and disc offset `internal[+0x24]`. Final request sequence remains **UNKNOWN** without the IPL-dependent branch trace. |
| **PROVEN** | `0x8120108C..0x81201098`, `0x81200774..0x812007B4` | Finalization publishes `0x0D15EA5E` to low `+0x20`, `1` to `+0x24`, `0` to `+0x30`, loader `+0x30` to both low `+0x34/+0x38`, and loader `+0x2C` to low `+0x3C`. These are stores at that point, not independently measured values at DOL entry. |
| **PROVEN** | `0x8120109C..0x812010C0` | If internal `+0x14C == 0`, a hardware query (`0x812002D8`) may write a halfword at low `0x800030E4`. That query's result is **UNKNOWN** from static files. |

**Conditional calculation, not an oracle:** If an original IPL run supplies
`[0x80000028]=0x01800000` and follows the ordinary nonfatal state-3 branch,
the checked instructions give `E8=0`, `EC=0x81800000`, `F0=0x01800000`,
loader `+0x30=align_down_32(0x81800000-0x18B19)=0x817E74E0`, and
`[0x800000F4]=0x817E54E0`. The aligned FST read length would be `0x18B20`,
ending at `0x81800000`. This is **STRONG as a conditional evaluation** of
`0x812008C8..0x81200AA8`, but the input word and full path have not been
observed; the final pre-entry values remain **UNKNOWN**.

## Why static files cannot prove 100% pre-entry parity

The disc fixes DOL/FST/BI2 bytes and apploader instructions. It does not fix
IPL-created low-memory values at `0x80000028`/`0x800030F0`, hardware query
outcomes, device state, initial registers, timing, or every memory write made
by IPL and callbacks. The DOL `0x80003154..0x8000333C` register helper does
not read those fields, which is why the current bounded native proof remains
valid. The next call at `0x80003158 -> 0x80003400` cannot be declared equivalent
from this static evidence. **UNKNOWN:** an exact full pre-entry state and the
selected conditional branch for this disc on an original console boot path.

## Independent original-path oracle procedure

1. Use an unmodified `GUPP8P` retail image matching all five hashes above and
   the actual original boot path (hardware with a debugger, or a pinned
   emulator run with a dumped original IPL). Record the console/IPL identity,
   debugger version, launch mode and disc-image hash. A DOL-only launcher or
   emulator HLE boot is a *separate* oracle: it may synthesize low memory and
   cannot independently establish the retail IPL handoff.
2. Start from a cold reset. Break **before** instruction `0x80003154`, not
   after the first startup call. Log each AplMain request `(destination,
   length, disc offset)` and its return value; capture the branch inputs and
   decision at `0x81200ADC`, `0x81200B5C`, `0x81200FEC` and `0x8120109C`.
   Record before/after values for writes from `0x812008C8..0x81200AA8` and
   `0x81200774..0x812007B4`.
3. At the entry stop, export PC/LR/CTR/CR/XER/MSR, GPRs, FPRs/FPSCR and
   relevant SPRs; all `0x80000000..0x800030FF` bytes; `[0x800000F4]` and the
   following `0x2000` BI2 bytes; `[0x80000038]` and length `[0x8000003C]`
   FST bytes; every nonempty DOL section; and the three later CRT zero-table
   ranges as *pre-CRT observations*. Log low-memory word provenance and
   timestamps/order, not only a final screenshot. Keep raw dumps outside
   version control; publish hashes, selected field values and analysis only.
4. Verify section dumps byte-for-byte against `main.dol` and FST bytes against
   `fst.bin`; check that each DVD request lies within the original disc
   layout. Repeat clean boots, classify any changing bytes as runtime inputs
   or undefined memory, and define parity only for guest-observed state.
   Preserve a mismatch log. A zero diff for the required defined ranges and
   observed branches is the acceptance gate for the native pre-entry recipe.

This procedure can close the pre-entry *observable-state* question. It cannot
by itself prove the rest of CRT, hardware services, constructors, game loop,
or pixel/runtime parity; those have their own gates.

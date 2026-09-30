# PAL GUPP8P hardware-startup frontier

Scope: DOL entry's second call, `0x80003158 -> 0x80003400`, through its return to `0x8000315C`. The source is the read-only PAL `main.dol` of 5,773,024 bytes, SHA-256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`. Addresses below come from an independent `q.py dis` pass against that DOL; the older `experimental_native_boot/` source was checked afterward and is reference only. This original static audit preceded the later **synthetic Dolphin/HLE observation** in `ASM_HARDWARE_VALIDATION.md`. The path has still not been observed on an original console or retail IPL boot.

## Direct call graph and ordered effects

| Confidence | Address | Instruction-backed result |
|---|---|---|
| PROVEN | `0x80003400..0x80003420` | Reads MSR, sets `0x2000` (FP enable), saves LR in r31, calls `0x80371714`, `0x80370CDC`, `0x80372838` in that order, restores LR and returns. Initial MSR value is UNKNOWN. |
| PROVEN | `0x80371714..0x80371764` | Reads HID2 at `0x80370BA8`, ORs `0xA0000000` at `0x80371724`, writes HID2 at `0x80370BB0`; `0x803725F4..0x80372600` reads HID0 and writes `HID0 | 0x0800` as a self-clearing ICFI command when ICE is enabled; clears GQR0 through GQR7 at `0x80371738..0x80371754`. Stack writes restore from their own frame. The HID0 write word must not be treated as persistent readback. Initial retail HID0/HID2 values are UNKNOWN. |
| PROVEN | `0x80370CDC..0x80370E00` | Enables FP again, reads HID2, branches at `0x80370CF0`, loads paired single bytes from `0x805F1F38` at `0x80370CFC` on the selected path, distributes to FPR1..31, then loads a double from `0x805F1F30` at `0x80370D7C`, distributes it to FPR1..31, writes FPSCR using `mtfsf` at `0x80370DFC`. |
| PROVEN | `0x80372838..0x80372928` | Reads HID0 twice and conditionally initializes L1 I/D caches; reads L2CR and conditionally initializes L2. Then passes selector 1 and handler pointer `0x803726D8` to `0x80373378`. Logging calls to `0x80370C8C` only write their own temporary stack frame; its routine returns without output. |
| PROVEN | `0x80373378..0x803733C4`, `0x80373564..0x8037358C` | Selector 1 writes the handler pointer `0x803726D8` at `0x80586CB4`; because `1 != 0x10`, the long exception/vector branch `0x803733C8..0x80373560` is skipped. The wrapper temporarily clears/restores the MSR interrupt bit through `0x8037611C/0x80376144`. |

The handler-table address is independently arithmetically checked: `0x80586CB0 + ((1 << 2) & mask(14..29)) = 0x80586CB4`. The older `experimental_native_boot/src/hardware_frontier.cpp` report said `0x80580000`; that diagnostic and the related CRT address check have been corrected. The old value omitted both the `addi +0x6CB0` at `0x803733A8` and the selector offset at `0x803733A4`.

## Branch dependencies and first unknown data

1. **PROVEN:** `0x80371724` forces HID2 bits `0xA0000000`. For the bit tested by `rlwinm.` at `0x80370CEC`, rotating that forced value by three yields a nonzero masked bit. Therefore the paired-single path `0x80370CF4..0x80370D78` is selected regardless of the initial HID2 value, provided the preceding hardware writes behave as their instructions specify. Initial HID2 is still needed for full register-state parity.
2. **PROVEN:** the *first non-DOL RAM content consumed* on that path is the eight bytes at `0x805F1F38..0x805F1F3F` by `psq_l` at `0x80370CFC`. The later `lfd` at `0x80370D7C` consumes `0x805F1F30..0x805F1F37`. Both ranges fall inside the CRT's *later* zero-table range `0x805EF020..0x805F277B`, and outside all ten DOL sections. `q.py xref` shows reads at `0x80370CFC/0x80370D7C`, no DOL write to these addresses. The post-CRT zero image is **not** evidence of their value at this earlier read. Their pre-entry bytes are UNKNOWN.
3. **PROVEN:** the first state-dependent branches of the third callee are `0x80372860` (HID0 mask from `rlwinm` bit 16), `0x80372880` (HID0 mask bit 17), and `0x803728A0` (L2CR top bit). Initial HID0/L2CR and the selected cache paths are UNKNOWN. The optional L2 invalidate at `0x80372640..0x803726D4` polls L2CR until a status bit clears; a static DOL image cannot prove termination or timing. The common handler-table store above follows these branches if they return.
4. **LIKELY candidate:** native platform initialization can replace L1/L2 cache operations where no translated code observes their original register state or timing. Exceptional behavior and later direct SPR reads have not been exhaustively audited. No full boot-parity claim follows from this local trace.

There are earlier **unknown hardware register reads**: MSR at `0x80003400`, HID2 at `0x80370BA8`, and HID0 at `0x803725F4`. The helper makes the paired-single branch determinate despite unknown initial HID2; it does not make the resulting complete hardware state determinate.

## Sound native boundary and required oracle

A native implementation can represent the proven ordered intent as explicit platform calls: enable the required FP operation, establish the paired-single format/GQR semantics, seed the FPR/FPSCR state from **observed original pre-entry bytes**, establish cache and exception facilities through native platform services, and register the handler at its proven table slot if a translated consumer still reads that table. The exact paired lane result after the `lfd`/`fmr` sequence is **UNKNOWN** without a CPU-level observation; older reference comments disagree about whether lane 1 survives. The implementation must decline to supply FPR constants or claim cache/interrupt parity without evidence. It should pin the helper instructions and branch targets, and test both branch choices where initial hardware state remains an input.

The next independent oracle should boot this exact PAL image through the original loader path and pause **before `0x80003154`**. Capture:

- DOL/disc hash and the exact stop PC, full MSR, HID0, HID2, L2CR, FPSCR, GQR0..7 and relevant cache state;
- the 16 raw bytes at `0x805F1F30..0x805F1F3F`, with source/memory-map metadata; the low-memory handoff and stack page;
- a branch/write trace from `0x80003400` to return `0x8000315C`, including the `0x80372860/0x80372880/0x803728A0` choices, any polling iterations, the handler-table word at `0x80586CB4`, FPR lanes and FPSCR;
- a second checkpoint immediately before `0x8000316C` and after the CRT zero loop, to prove ordering rather than infer the earlier bytes from the later image.

Until then, the buildable boot foundation correctly stops at `0x80003158`. No produced state here is a substitute for that external parity oracle.

# HID0 ICFI command and its first boot consumers

**Scope:** PAL GUPP8P `main.dol` call `0x8037172C`, HID0 leaf
`0x803725F4..0x80372600`, synchronization at `0x80371730`, and the first
connected HID0 reads in cache setup `0x80372854..0x80372880`. This note does
not translate cache hardware into C++. Source binary is read-only
`sys/main.dol`, SHA-256
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
The DOL text mapping for all rows is file `0x2600` at VA `0x80008D40`, so
`file_offset = 0x2600 + VA - 0x80008D40`. Each word below was read as a
big-endian `uint32` from that mapped byte span, then decoded from its fields.

Here **PROVEN-BINARY** means DOL bytes and instruction encoding,
**PROVEN-GEKKO** means the [IBM Gekko User's Manual v1.2](https://doc.kodewerx.org/documents/gekko_user_manual.pdf),
**OBSERVED-HLE** means the separate synthetic-disc Dolphin interpreter run,
and **UNKNOWN** means an unmeasured retail input or hidden hardware state.
An HLE observation is neither retail IPL state nor Gekko silicon evidence.

## Exact instruction records

The three SPR operations have primary opcode 31. Bits 1..10 are XO 339 for
`mfspr` and 467 for `mtspr`. The split SPR operand decodes as
`((word >> 16) & 31) | (((word >> 11) & 31) << 5) = 1008` for HID0.
`ori` is primary opcode 24, source/destination r3, immediate `0x0800`,
without a CR record bit. Branch LI/BD displacements are sign extended byte
displacements relative to each instruction because AA=0.

| VA | File offset | Raw bytes / BE word | Decode and local effect |
|---|---:|---|---|
| `8037172C` | `36AFEC` | `48 00 0E C9` / `48000EC9` | op18, disp `+0xEC8`, LK=1: → `bl 0x803725F4`; LR=`0x80371730`. |
| `803725F4` | `36BEB4` | `7C 70 FA A6` / `7C70FAA6` | op31 XO339 SPR1008: `mfspr r3,HID0`; r3=live HID0 readback `H`. |
| `803725F8` | `36BEB8` | `60 63 08 00` / `60630800` | op24: `ori r3,r3,0x0800`; r3=`H | 0x00000800`. |
| `803725FC` | `36BEBC` | `7C 70 FB A6` / `7C70FBA6` | op31 XO467 SPR1008: `mtspr HID0,r3`; issues the full computed write word. |
| `80372600` | `36BEC0` | `4E 80 00 20` / `4E800020` | op19 XO16 BO20 BI0: `blr` to LR=`0x80371730`. |
| `80371730` | `36AFF0` | `7C 00 04 AC` / `7C0004AC` | op31 XO598: `sync`, ordering/completion barrier; no GPR result. |
| `80372854` | `36C114` | `4B FF E2 99` / `4BFFE299` | op18, disp `-0x1D68`, LK=1: → `bl 0x80370AEC`. |
| `80370AEC` | `36A3AC` | `7C 70 FA A6` / `7C70FAA6` | op31 XO339 SPR1008: `mfspr r3,HID0`; *new* live read. |
| `80372858` | `36C118` | `54 60 04 20` / `54600420` | op21: `rlwinm r0,r3,0,16,16`; r0=`r3 & 0x00008000` (ICE), Rc=0. |
| `8037285C` | `36C11C` | `28 00 00 00` / `28000000` | op10: `cmplwi r0,0`; CR0 is set from unsigned compare. |
| `80372860` | `36C120` | `40 82 00 14` / `40820014` | op16 BO4 BI2 disp `+0x14`: → `bne 0x80372874` if ICE bit was nonzero. |
| `80372864` | `36C124` | `4B FF FD A1` / `4BFFFDA1` | op18, disp `-0x260`, LK=1: if ICE was zero, → `bl 0x80372604`. |
| `80372874` | `36C134` | `4B FF E2 79` / `4BFFE279` | op18, disp `-0x1D88`, LK=1: → `bl 0x80370AEC`, a new HID0 read. |
| `80372878` | `36C138` | `54 60 04 62` / `54600462` | op21: `rlwinm r0,r3,0,17,17`; r0=`r3 & 0x00004000` (DCE), Rc=0. |
| `8037287C` | `36C13C` | `28 00 00 00` / `28000000` | op10: `cmplwi r0,0`; refresh CR0. |
| `80372880` | `36C140` | `40 82 00 14` / `40820014` | op16 BO4 BI2 disp `+0x14`: → `bne 0x80372894` if DCE bit was nonzero. |
| `80372884` | `36C144` | `4B FF FC 71` / `4BFFFC71` | op18, disp `-0x390`, LK=1: if DCE was zero, → `bl 0x803724F4`. |
| `80372604` | `36BEC4` | `4C 00 01 2C` / `4C00012C` | `isync` before a possible ICE enable. |
| `80372608` | `36BEC8` | `7C 70 FA A6` / `7C70FAA6` | op31 XO339 SPR1008: `mfspr r3,HID0`; read current word after the branch. |
| `8037260C` | `36BECC` | `60 63 80 00` / `60638000` | `ori r3,r3,0x8000`; preserve other read bits and request ICE. |
| `80372610` | `36BED0` | `7C 70 FB A6` / `7C70FBA6` | op31 XO467 SPR1008: `mtspr HID0,r3`; write complete updated word. |
| `80372614` | `36BED4` | `4E 80 00 20` / `4E800020` | `blr` to cache caller. |
| `803724F4` | `36BDB4` | `7C 00 04 AC` / `7C0004AC` | `sync` before a possible DCE enable. |
| `803724F8` | `36BDB8` | `7C 70 FA A6` / `7C70FAA6` | op31 XO339 SPR1008: `mfspr r3,HID0`; read current word after the branch. |
| `803724FC` | `36BDBC` | `60 63 40 00` / `60634000` | `ori r3,r3,0x4000`; preserve other read bits and request DCE. |
| `80372500` | `36BDC0` | `7C 70 FB A6` / `7C70FBA6` | op31 XO467 SPR1008: `mtspr HID0,r3`; write complete updated word. |
| `80372504` | `36BDC4` | `4E 80 00 20` / `4E800020` | `blr` to cache caller. |

The two read accessor calls each return through its unlisted adjacent `blr`
at `0x80370AF0`. The post-helper logging calls at `0x80372868..0x80372870`
and `0x80372888..0x80372890` do not substitute for an HID0 read; they are
included in the full cache CFG in `BINARY_CACHE_HANDLER.md`. Both conditional
branch destinations above are byte-derived, not inferred from HLE direction.

## Machine-state and hardware consequence

Let `H` be the value returned by `mfspr` at `0x803725F4`, not a presumed
reset constant. In the ordinary uninterrupted path, the leaf's register
transition is `r3 := H`, then `r3 := H | 0x800`; LR is the caller's
`0x80371730`; `blr` returns there; `sync` changes neither r3 nor CR/XER.
The HID0 `mtspr` writes an SPR, not ordinary RAM. There is no leaf stack
frame, SDA/SDA2 access, FPR operation, GQR change, or pointer calculation.
The `bl` changes LR, so the leaf's GPR/SPR effects and its return target must
both survive any translation. The leaf does not explicitly mask MSR or
interrupts. HID0 is a supervisor SPR, so the successful trace requires
privileged execution; a user-mode access would take a program exception
([manual SPR encoding table and program-exception discussion](https://doc.kodewerx.org/documents/gekko_user_manual.pdf)).

**PROVEN-GEKKO:** HID0 `0x00000800` is IBM bit 20, ICFI. Per Table 2-4 and
§3.4.1.4 of the manual, with HID0[ICE] enabled, setting ICFI starts a whole
L1 instruction-cache flash invalidate, clears cache-block valid bits and
the replacement-state bits, blocks access during the operation, and hardware
clears the command bit when invalidation begins (normally the next cycle).
It does not write modified cache blocks to memory. Consequently the
*issued* word `H | 0x800` is **not** a proven later HID0 readback word.
When ICE=0, the same manual conditions the invalidate and self-clear on ICE;
the precise retained-command/readback interaction across a later ICE enable
is **UNKNOWN** here. The original `ori` preserves all other HID0 bits in its
write operand but does not prove that all bits are writable or unchanged by
hardware.

The preceding boot code enables HID2 graphics-extension bits at
`0x80371724..0x80371728`; the manual §2.1.2.4 requires an instruction-cache
invalidate after newly enabling those bits and before executing the related
graphics instructions. The binary's order is HID2 write → HID0 ICFI request
→ `sync` → GQR writes → FPR/paired-single seed. The manual defines `sync` as
an ordering point at which earlier instructions appear complete before later
ones start. This establishes the intended ordering, but neither DOL bytes
nor an RSP HID0 read exposes exact cache tags, instruction queue contents,
or interrupt timing. There is no DOL-authored ordinary memory store in this
leaf. The cache-state mutation is a hardware effect, not a RAM write to be
manufactured in native C++.

## Write → read and control-flow consumers

| Producer or state | First connected consumer | Exact dependency | Confidence |
|---|---|---|---|
| HID0 ICFI request `0x803725FC` | Instruction fetches after request, including `sync` at `0x80371730` and later paired instructions | Cache validity/latency and instruction visibility if ICE was enabled. No cache-tag readback in the captured interface. | **PROVEN-GEKKO** mechanism; actual retail tags **UNKNOWN**. |
| HID0 live read `0x803725F4` plus OR/write | `mfspr` at `0x80370AEC`, called by `0x80372854` | Reads the *hardware-updated* HID0, then `0x80372858..60` tests ICE bit `0x8000`. If zero, calls optional I-cache helper; if nonzero, branches over it. | **PROVEN-BINARY** CFG; selected retail edge **UNKNOWN**. |
| Optional `0x80372604..14` ICE setter | second `mfspr` at `0x80370AEC`, called by `0x80372874` | This is a new live read after possible ICE change; `0x80372878..80` tests DCE bit `0x4000`. | **PROVEN-BINARY** CFG; exact post-write retail readback **UNKNOWN**. |
| Optional DCE setter `0x803724F4..0x80372504` | later cache/L2 setup | Changes HID0[DCE] through an SPR write only on its zero branch. It does not prove any L2CR branch outcome. | **PROVEN-BINARY** local instructions; path/input **UNKNOWN**. |

A static scan of *text sections only* for op31, XO339/467, SPR1008 finds
later HID0 operations at `0x80376E9C/0x80376EA4`,
`0x8037829C/0x803782A4/0x803782B0`, and
`0x8039F5C4/0x8039F864`. Those are candidate downstream consumers, not
proven connected executions from this entry. The scan is not a data-flow
proof for them. The first connected reads are the cache setup calls above.

**OBSERVED-HLE:** the synthetic startup-only trace begins with
HID0=`0x0011C464` (ICE and DCE both set, ICFI clear). At PC `0x80371730`,
r3=`0x0011CC64`, the issued write operand, while the exposed HID0 register
already reads `0x0011C464`. At PC `0x80372860`, the accessor returned
r3=`0x0011C464`; mask r0=`0x00008000`, CR0 indicates nonzero, and the
branch goes to `0x80372874`. At `0x80372880`, the second read's DCE mask
is nonzero and the optional D-cache helper is skipped. These values are
consistent with the manual's enabled-cache self-clear. The capture does
not measure the internal I-cache invalidation, and the startup-only HLE
disc does not establish retail IPL HID0 state.

## Falsification and validation frontier

1. **Binary gate:** re-read all table spans from the SHA-pinned DOL; assert
   four-byte equality, independent branch displacements, XO 339/467 and
   SPR1008. Any mismatch invalidates this note before interpreting hardware.
2. **State transition:** breakpoint immediately before and after
   `0x803725FC`; compare r3 write operand with live HID0 at
   `0x80371730` and each `0x80370AEC` call. Do not equate r3 to the SPR
   after the write. Capture all intervening HID0 writes and exceptions.
3. **ICE counterexample:** test one controlled reference start with ICE=1
   and one with ICE=0, holding other relevant initial state fixed. The
   former should expose the specified self-clear; for the latter, record
   actual readback and the optional ICE setter instead of guessing.
4. **Cache-effect counterexample:** compare instruction cache valid/tag or
   carefully controlled fetch behavior before/after ICFI on a qualifying
   oracle. HID0 readback equality alone cannot prove invalidate completion.
5. **Native adversarial test:** a model retaining `0x800` as a persistent
   flag must fail the ICE=1 readback comparison. A model forcing the ICE
   branch from synthetic `0x0011C464` for every start must fail a zero-ICE
   input. Rerun the entire native prefix after any future implementation.

**Unresolved:** retail IPL HID0 and privilege/interrupt state at this call;
cache tags, completion and fetch visibility; exact ICE=0 readback; whether
an asynchronous exception intervenes; and connected native parity through
these hardware effects. There is no established portable C++ replacement
for ICFI. The native probe now reaches the issued request at `0x80371730`
with caller-supplied HID0, but stops before `sync` and any cache-effect
consumer; that does not validate a portable hardware replacement.

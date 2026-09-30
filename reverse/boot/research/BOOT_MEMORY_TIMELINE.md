# PAL GUPP8P boot memory archaeology

Scope: the supplied `sys/apploader.img`, `sys/bi2.bin`, and `sys/main.dol`,
through the connected DOL hardware call and the statically mapped CRT/OS
handoff. This is a **producer → store → reader** ledger, not a claim that the
native reconstruction has executed the CRT or OS path. `PROVEN` means the
specified raw bytes and their conditional instruction consequences; `OBSERVED
SYNTHETIC` means a separate startup-only Dolphin HLE disc run. Neither label
proves a retail IPL input. All addresses below are guest virtual addresses;
all ordinary `lwz`/`stw` words are four bytes in big-endian memory. `lfd`
reads eight big-endian bytes, `psq_l` here reads two four-byte lanes, and
`memset` writes bytes regardless of host endianness.

The read-only `main.dol` SHA-256 is
`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`;
`apploader.img` is
`8a1fe9487b029b2518a6633b1e0eb9ec0922f5f5eb5afdd7eba1267a85f096fe`;
`bi2.bin` is
`8bbadffc2a7f79eda5982aa2fe30831969c2702aa9206f621b930aae0b70af6b`.
I read those files independently as bytes, decoded the relevant 32-bit
big-endian words, and compared DOL instruction interpretations with `q.py
dis`. `q.py` has no apploader function map, so its `0x8120....` queries fail;
the apploader evidence below is explicitly from raw file words, not a
purported `q.py` decode. Apploader runtime word `A` is at file offset
`0x20 + (A - 0x81200000)`. DOL text0 maps file `0x100` to `0x80003100`;
text1 maps file `0x2600` to `0x80008D40`.

## Raw-word anchors and width

| Guest address | Raw file offset / bytes | Decode and memory consequence |
|---|---|---|
| `0x812008DC` | apploader `+0x8FC`: `80 7A 00 28` | `lwz r3,0x28(r26)`: reads the pre-existing low-memory word `M=[0x80000028]` on the mapped state-3 path. |
| `0x81200A94` | apploader `+0xAB4`: `90 1F 00 30` | `stw r0,0x30(r31)`: stores the computed FST placement in loader state. The other state-3 arm stores at `0x81200A38` (`90 1F 00 30`, file `+0xA58`). |
| `0x81200AA4` | apploader `+0xAC4`: `90 03 00 F4` | `stw r0,0xF4(r3)`: publishes the full-BI2 destination at low memory `0x800000F4`. |
| `0x812007A4/AC` | apploader `+0x7C4/+0x7CC`: `90 03 00 34` / `90 03 00 38` | Two separate `stw` stores publish loader `+0x30` to low memory `+0x34` and `+0x38`. |
| `0x81200D94..A0` | apploader `+0xDB4..+0xDC0`: `80 7F 01 18`, `38 80 00 00`, `80 BF 01 1C`, `4B FF F2 61` | Load DOL header BSS address/size, choose fill byte zero, then call the apploader fill routine at `0x81200000`. The header itself holds `0x8056FE00` and `0x0008C7EC`. |
| `0x80370CFC` | DOL `+0x36A5BC`: `E0 03 00 00` | `psq_l f0,0(r3),W=0,GQR0`: eight-byte read at computed `0x805F1F38..3F` on the selected HID2 path. |
| `0x80370D7C` | DOL `+0x36A63C`: `C8 0D 5A 30` | `lfd f0,0x5A30(r13)`: eight-byte read at `0x805F1F30..37` with established r13=`0x805EC500`. |
| `0x803733B4/C0` | DOL `+0x36CC74/+0x36CC80`: `83 C4 00 00` / `93 84 00 00` | Read prior handler word then store r28 at the computed four-byte slot. For selector 1 the slot is `0x80586CB4`; caller supplies `0x803726D8`. |
| `0x80003160/64/68` | DOL `+0x160/+0x164/+0x168`: `94 21 FF F8`, `90 01 00 04`, `90 01 00 00` | Stack back-chain store then two sentinel word stores; the last overwrites the back-chain at `0x8060C5E8`. |
| `0x80003344/48` | DOL `+0x344/+0x348`: `90 01 00 04` / `94 21 FF E8` | Save caller LR at `0x8060C5EC`, then create a 24-byte CRT frame. |
| `0x800055C8..DF` | DOL `+0x25C8..+0x25DF`: three big-endian `(address,length)` pairs | CRT zero ranges: `(8056FE00,74700)`, `(805EF020,375C)`, `(805FC540,AC)`. Pair `(0,0)` at `0x800055E0` terminates on zero **length**. |

The raw DOL copy table at guest `0x80005544` (file `+0x2544`) has ten
nonzero `(source,destination,length)` triples, all with `source=destination`,
then `(0,0,0)` at `0x800055BC`. `0x8000336C..A8` tests length first and
skips `memmove` and cache work for equal source/destination. The checked PAL
image therefore performs no copy-table destination writes at this step;
altered descriptors would require a new proof.

## Ordered memory timeline

| Stage and writer | Old state and exact write | First relevant reader / lifetime |
|---|---|---|
| Pre-apploader IPL | Initial `[0x80000028]` is **UNKNOWN** in the five supplied files. It is a four-byte input, not a DOL constant. | Apploader `lwz` at `0x812008DC`, again at `0x812009A0/B4/D4`, `0x81200A48/58/74`; later OS path `0x80376BF4` also reads it. Source must be measured on a retail-path oracle. |
| Apploader state 3 | Reads `BI2+0=0`, `BI2+4=0x01800000` from raw `bi2.bin`. Writes low-memory `+E8=0`, `+EC=align_down_32(0x80000000+M)`, `+F0=0x01800000`; computes a loader-local `+0x30` from `M`, BI2 size and FST maximum `0x18B19`, and stores low-memory `+F4=FST_base-0x2000` at `0x81200AA4`. Old contents of every target are **UNKNOWN**. | Full BI2 DVD read uses `[+F4]` as destination, then DOL entry `0x80003188` and `0x80003200` read it twice. OS `0x80370F08` reads it again. The pointer cannot become a native host pointer without range and contents checks. |
| Apploader state 6 | `0x81200D94..A0` clears DOL header BSS envelope `[0x8056FE00,0x805FC5EC)` to zero, **before** section requests on a completed ordinary state path. Old bytes are **UNKNOWN**. | Data6 `[0x805E4500,0x805EF020)` and data7 `[0x805F2780,0x805FC540)` are then loaded. The resulting zero gaps are `[0x8056FE00,0x805E4500)`, `[0x805EF020,0x805F2780)`, `[0x805FC540,0x805FC5EC)`, provided no subsequent overlapping external write. |
| Apploader state 9 and handoff | The FST request writes to loader `+0x30` either before DOL sections or after them according to state-4 flag `+0x194`. At `0x812007A0..B4`, the final writer stores `FST_base` to `[0x80000034]` and `[0x80000038]`, and FST size `0x18B19` to `[0x8000003C]`; old low-memory words are **UNKNOWN**. | Entry `0x800031A4` may read `[+34]` on the no-BI2 path; `0x80003250` may overwrite `[+34]` after relocation. OS arena read `0x80370FD0` consumes its then-current value. `[+38]` persists at least until any later writer; alternate OS disc/FST loader `0x8037E198` can replace it on a different boot-magic path. |
| DOL entry helper | `0x800032B0..33C` sets r1=`0x8060C5F0`, r2=`0x805FA780`, r13=`0x805EC500`; no ordinary RAM write. | The following hardware and CRT stack effective addresses, and `lfd` SDA address, depend on these established register values. |
| Hardware FPR seed | `0x80370CFC` reads eight bytes at `0x805F1F38..3F`; `0x80370D7C` reads eight bytes at `0x805F1F30..37`. Neither writes the source bytes. | Both reads occur **before** the CRT zero call. The source interval lies in the apploader's middle zero gap if it completed and remained unoverwritten. At DOL entry the final retail bytes are **UNKNOWN** because FST placement and external writes are not observed. |
| Hardware handler install | `0x803733B4` reads old `[0x80586CB4]`; `0x803733C0` writes `0x803726D8` there. The selector-1 path does not enter the special selector-16 vector branch. Old handler word is **UNKNOWN** from the DOL alone. | The first CRT zero range contains this slot and erases the installed value. The hardware store is transient across that CRT call; it cannot be treated as the permanent handler registration. |
| CRT zero walker | `0x800033C0..E0` calls `memset` three times with exactly the table lengths. Writes zero to `[0x8056FE00,0x805E4500)`, `[0x805EF020,0x805F277C)`, `[0x805FC540,0x805FC5EC)`. Old bytes at this point include the just-installed handler. | Clears handler slot and FPR source **after** their hardware-stage use; also clears OS first-call flag `0x805F1F40`. Its middle end is four bytes before data7 start; the apploader BSS clear was four bytes longer there. OS startup sees the cleared first-call flag at `0x80370E80`. |
| DOL entry after CRT | `0x8000317C` writes `[0x80000044]=0`. If BI2 relocation count is nonzero, `0x80003234..40` adds BI2 base to each relocation word and `0x80003250` overwrites `[0x80000034]` with aligned *start* of the relocation array. Then `0x80370BF0..C14` writes `[0x805F1F18]=0x80000040`, `[0x80000048]=0x00370C60`, `[0x805F1F1C]=1`. | Debug and BI2 logic reads low-memory `[+F4]` twice at `0x80003188/0x80003200`, and `[+34]` at `0x800031A4` if needed. OS startup at `0x80370E68` consumes the resulting state; branch count/address validity remains conditional. |
| OS startup on first-call branch | `0x80370EA0` writes `[0x805F1F40]=1` after reading CRT-cleared zero. `0x80371014` calls the cache/handler routine again; if reached and it takes its selector-1 path, `0x803733C0` writes `0x803726D8` to `[0x80586CB4]` **after** CRT. | This is the later possible reinstallation, distinct from the transient pre-CRT store. Device polling, callbacks and OS return have not been validated in connected native execution, so a final persistent handler claim is **UNKNOWN**. |

For ordinary nonfatal state-3 arithmetic with `M<=0x01800000`, the FST
destination is
`align_down_32(0x80000000 + min(M,0x01800000) - 0x18B19)`.
`M=0x01800000` yields `FST_base=0x817E74E0` and
`BI2_base=0x817E54E0`; `M=0x01000000` yields
`0x80FE74E0` and `0x80FE54E0`. Those are **conditional evaluations**, not
retail IPL observations. The old disc-header word `boot.bin[0x430] =
0x803E74E0` cannot be substituted for the final FST base: that address
overlaps DOL text1. `M`, state-4 ordering and any external writer remain
inputs to the final pre-entry memory image.

## Startup stack ledger: `0x8060C5E8..0x8060C5F4`

Let `S=0x8060C5F0`, the proven r1 after `0x800032B0`. All four tracked
addresses have **UNKNOWN** old RAM content before the first cited store.
The following values are conditional on calls returning along the audited
path; transient earlier words must not be replaced with their final value
when checking an intermediate checkpoint.

| Instruction / step | `E8` | `EC` | `F0` | `F4` |
|---|---|---|---|---|
| `0x80371718/1C`, paired-single setup | `S` (`stwu r1,-8`) | old unknown | old unknown | `0x80003414` saved LR (`stw r0,4(S)`) |
| `0x80371758`, before setup return | back-chain `S` | old unknown | old unknown | `lwz r0,12(S-8)` reads `0x80003414` |
| `0x8037283C/40`, cache setup | still `S` | old unknown | old unknown | overwritten with `0x8000341C`; `0x80372914` later reads it |
| `0x80003160/64/68`, entry sentinel | first `S`, then `0xFFFFFFFF` | `0xFFFFFFFF` | no established write | retains `0x8000341C` from hardware call |
| `0x80003344/48`, CRT frame | `0xFFFFFFFF` | overwritten with `0x80003170` (call return LR) | no established write | unchanged by this frame |
| `0x800033E4..FC`, CRT return | `0xFFFFFFFF` | `lwz r0,0x1C(0x8060C5D0)` reads `0x80003170` | no established write | unchanged by this frame |
| `0x80370E6C/70`, OS first-call frame, **if reached** | `0xFFFFFFFF` | overwritten again with `0x80003268` | no established write | not altered by these two instructions |

The `0x80003348` CRT frame starts at `0x8060C5D0`; its r29/r30/r31
spills are at `DC/E0/E4`, below the requested interval. The cache routine
uses `S-0x10`, the handler installer and cache invalidation use still lower
frames. This ledger does not assert that no later OS or game code ever writes
`F0/F4`; it records the cited boot prefix and the additional OS prologue
overwrite. A two-sentinel snapshot after CRT return would be wrong: only
`E8` remains `-1` at that point.

## FPR source and handler write → read chains

The DOL header BSS envelope is `0x8056FE00 + 0x8C7EC`, exclusive end
`0x805FC5EC`. Data6 and data7 divide it. The FPR source
`0x805F1F30..3F` is absent from every populated DOL section: it lies
between data6 end `0x805EF020` and data7 start `0x805F2780`. On a completed
ordinary apploader path, its producer is the state-6 zero fill, unless
overwritten by a later FST/other write. Its **first DOL readers** are the
paired load at `0x80370CFC` and double load at `0x80370D7C`; the hardware
routine does not store back to that RAM. The second producer is the CRT
`memset` on `[0x805EF020,0x805F277C)`. The first read therefore cannot be
justified by the later CRT zero. A native implementation needs explicit
pre-entry bytes for the earlier read and must not use the post-CRT zero as
their provenance.

The handler word is absent from DOL sections and lies in the first BSS
interval. The selector-1 address calculation at `0x803733A0..B0` is
`0x80586CB0 + rlwinm(1,2,14,29) = 0x80586CB4` (offset 4).
`0x803733B4` consumes the old word, `0x803733C0` installs
`0x803726D8`, and the selected path returns that old word in r3. The CRT
first zero range erases it. OS startup's later `0x80371014` direct call to
the same cache routine can reinstall it. This is a **write → clear →
conditional rewrite** chain; eliding either store or the clear changes
intermediate observable RAM and can affect future handler lookup.

## Adversarial checks and observations

- **PROVEN raw BI2 facts:** `BI2+8` and `BI2+0x0C` are both four-byte zero
  words. If the full BI2 request placed these bytes intact at the pointer
  stored in `[0x800000F4]`, and later loading/CRT did not overwrite them,
  `0x80003194` reads debug word zero, so the `D=2/3/4` debug paths are not
  selected; `0x8000320C` reads relocation offset zero, so the relocation
  loop and `[0x80000034]` overwrite are skipped. This tightens the ordinary
  *conditional* path but does not prove retail pointer placement or exclude
  intervening writes.
- **OBSERVED SYNTHETIC, never retail:** a startup-only HLE disc containing
  the verified five `sys/` files reached DOL entry with
  `[0x80000028]=0x01800000`, `[0x80000038]=0x817E74E0`, and all 16 FPR
  source bytes zero. In the same synthetic hardware trace the handler slot
  was zero before `0x803733C0`, held `0x803726D8` after it, and still held
  that pointer at `0x8000315C`. These observations are captured in ignored
  `build/` artifacts and summarized in `ASM_HARDWARE_VALIDATION.md`.
  The trace does **not** pass the subsequent CRT/OS boundary.
- **SPR caveat:** `0x803725F4..0x80372600` issues an HID0 OR-`0x800`
  write, but this ICFI command bit can self-clear with instruction cache
  enabled. The synthetic readback remains `0x0011C464` despite the issued
  `0x0011CC64` write. A RAM ledger must not model that SPR as a persistent
  ordinary word or infer later cache branch predicates from the written
  value alone.
- **Potential overlap:** the high-memory sufficient condition in
  `APPLOADER_PATH.md` is `M>=0x0060AA59`, making FST start after
  `0x805F1F3F` on that arm. No supplied file proves retail `M`, so the FPR
  source at hardware entry remains **UNKNOWN**. A DOL-only zero-fill test
  cannot close this loader-state dependency.
- **Further required observations:** retail IPL low-memory producer for
  `[+28]`, state-4 FST order, exact `[+F4]` BI2 bytes at entry, the stack
  words at hardware/CRT checkpoints, FPR paired lane state, and the
  post-CRT handler slot after OS initialization. Preserve separate
  synthetic/retail provenance for each capture.

No C++ implementation is introduced by this note. The connected native
checkpoint is the bounded hardware entry stated in `../PROGRESS.md`; the
memory facts above are static or synthetic-oracle evidence until the
corresponding native prefix executes and matches ordered original state.

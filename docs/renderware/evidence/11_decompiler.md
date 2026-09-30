# Specialist 11: conservative motion-blur decompiler draft

## 1. Scope, provenance, and an important verification limitation

Owned deliverable: `analysis/motion_blur/evidence/11_decompiler.md` only. No final reconstructed source, original file, prior artifact, or other specialist report was edited. The C-like blocks below are analysis notation, not a compilable or runtime-tested replacement.

I read `AGENTS.md` (lines 1-10). The documented target is `main.dol`, 5,773,024 bytes, SHA-256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`. The existing inventory repeats that baseline (`motion_blur_analysis/inventory.json:2-4`), as does `analysis/motion_blur/evidence/binary_baseline.json:2-7`.

**All four attempted Python commands were denied by background-agent approval policy.** Consequently I could not independently re-hash the current DOL, run fresh Capstone, read raw constant/table words, or execute `pointers`/`xrefs`. I instead read bounded, address-located ranges of the existing local `full_disassembly.txt`, and searched the smaller existing `branches.tsv`, `strings.tsv`/`relevant_strings.tsv`, and reference map. I did not grep the oversized full disassembly. The pre-existing full disassembly includes instruction words, permitting independent instruction/branch decoding, but its correspondence to today's DOL remains a parent revalidation item.

Evidence labels used here:

- **I**: instruction words and addresses checked in bounded existing local disassembly; not a new raw-DOL read.
- **B**: existing local direct-branch cross-reference artifact.
- **S**: existing local string inventory.
- **H**: hypothesis requiring raw table/constant or runtime verification; not promoted to proof merely because the reference map names it.

For this artifact, executable section 1 has a useful checked line relation: `line = 1 + (VA - 0x80006840) / 4`. For example `0x8042E254` is `full_disassembly.txt:1089158`. The inventory's text-1 mapping gives file offset `VA - 0x80006740`. These are artifact/inventory relations, not a fresh integrity check.

## 2. Principal findings

1. The best-supported local DrawBlur core is **`[0x8042E254, 0x8042E838)`**, size `0x5E4`, 377 PPC instructions. Cached file-offset range: **`[0x00427B14, 0x004280F8)`**. It ends in `blr` at `0x8042E834`; the next function starts at `0x8042E838`.
2. The associated task execution chain is **`0x801D27FC -> 0x800A35D8 -> 0x8042F16C -> 0x8042E254`**, with the singleton call returning the manager supplied to the wrapper. The task-vtable entry connecting the named task to `0x801D27FC` still needs a raw pointer check.
3. The draw core builds **one four-vertex textured strip**, with four straight-line position/UV/color assignments. There is **no blur tap loop, sample count, accumulation loop, `fctiwz`, or fused multiply-add in this core**. The relevant backend loop emits vertices, not successive blur samples.
4. UVs depend on signed 16-bit raster offsets, signed 32-bit offset-plus-size additions, and **unsigned** parent raster dimensions. Geometry expands/contracts about the raster center using three separate single-precision operations: subtract the scale baseline, multiply by the dimension, then multiply by the half-pixel-style constant. The literal values of those constants are not verified here.
5. State ordering matters. The core saves seven RenderWare states, changes texture plus those states, renders, restores the seven states in the same order, and ends the camera update. It **does not save or restore texture state 1**. This is not equivalent to an invented universal GX save/restore helper.
6. Actual truncating float-to-byte control occurs at **`0x800A4804`** and in the blur-only branch of **`0x80439A8C`**, at **`0x80439F74`**. The latter explicitly uses `fmuls` followed by `fmadds`, then `fctiwz`, then `stb`; preserve those distinctions.
7. `SaveScreen` has its own explicit capture/copy sequence, and sets `+0x78` at the end even if its camera-begin calls returned zero. Do not strengthen that byte into a guarantee of successful GPU capture, and do not invent a temporal-history count or ping-pong buffer.

## 3. Identity chain: local strings, code, and the unresolved vtable word

### 3.1 Named constructor and task ownership

`relevant_strings.tsv:134-143` records:

| Local VA | Existing string |
|---|---|
| `0x804D0F18` | `DrawBlur` |
| `0x804D0F24` | `SaveScreen` |
| `0x804D1058` | `Effect::PJS::EffectDrawBlurTask` |
| `0x804D1034` | `Effect::PJS::EffectSaveScreenTask` |

The constructor **`[0x801D2058, 0x801D20D8)`** supplies code evidence rather than relying on the map's address:

| Instructions | Meaning |
|---|---|
| `801D206C bl 800461A0`; `801D2078 lwz r4,0x2c(r4)`; `801D207C bl 8004F014` | Obtain scheduler/parent and attach this task to its `+0x2C` task pointer; the human scheduling-level name remains a hypothesis. |
| `801D2080 lis r3,0x8054`; `801D2088 addi r0,r3,-0x2388`; `801D208C stw r0,0x18(r31)` | Install task vtable pointer **`0x8053DC78`**. |
| `801D2084 lis r4,0x804d`; `801D2090 addi r0,r4,0xf18`; `801D2098 stw r0,0(r31)` | Store the local **`DrawBlur`** name pointer. |
| `801D209C lhz`; `801D20A0 ori ...,0x100`; `801D20A4 sth` | OR bit `0x100` into a 16-bit task flags field. |
| `801D2094 li r3,0x28`; `801D20A8 bl 803A1380`; `801D20B0 beq 801D20BC`; `801D20B4 bl 801D20D8` | Allocate/construct a second task if allocation succeeds. |
| `801D20BC stw r0,0x28(r31)` | Keep its pointer at the DrawBlur task's `+0x28`. |

The second constructor **`[0x801D20D8, 0x801D213C)`** stores `0x804D0F24` (`SaveScreen`), installs vtable `0x8053DC5C`, and obtains its parent task from scheduler `+0x58` (`801D20F8`). Evidence: `full_disassembly.txt:470535-470591`.

The destructor **`[0x801D2788, 0x801D27FC)`** reinstalls `0x8053DC78`. If task `+0x28` is non-null, it ORs `1` into that child's 16-bit flags (`801D27B0-801D27C4`). This requests task disposal; it is not an immediate buffer deletion. Evidence: `full_disassembly.txt:470995-471023`.

### 3.2 Execution wrapper and exact branch decoding

`[0x801D27FC, 0x801D2820)` contains:

```text
801D2808  4BED0DD1  bl 0x800A35D8
801D280C  4825C961  bl 0x8042F16C
801D281C  4E800020  blr
```

`[0x8042F16C, 0x8042F198)` contains:

```text
8042F178  80630040  lwz    r3,0x40(r3)
8042F17C  28030000  cmplwi r3,0
8042F180  41820008  beq    0x8042F188
8042F184  4BFFF0D1  bl     0x8042E254
8042F194  4E800020  blr
```

The words independently decode correctly: `0x801D280C + 0x25C960 = 0x8042F16C`; `0x8042F184 - 0xF30 = 0x8042E254`. Sources: `full_disassembly.txt:471024-471032,1090124-1090134`; `branches.tsv:62663,138316`.

The named constructor's `0x8053DC78` vtable and the candidate Exec at `0x801D27FC` are highly consistent with the task family and destructor. **The vtable contents and RTTI pointer chain have not been read here.** The parent must verify that table and pointers to `0x801D27FC` before treating the entire name-to-core chain as raw-binary proven.

The reference map puts the task constructor at `0x801D1CB8`, Exec at `0x801D245C`, and a `0x5E4` function at `0x8042D204` (`reference_GUPE8P.map:15950,15973,32847`). Local constructor/Exec are `+0x3A0`, while the matching core is `+0x1050`. This expressly disproves applying one universal map delta.

## 4. Conservative data layout

The following is an offset model for the **32-bit PPC target**, not a native-host pointer layout. Naming `auxiliary_08/0c` avoids inventing meanings for values the core does not read.

```cpp
struct BlurControl16 {
    uint8_t enabled_00;
    uint8_t alpha_01;
    uint8_t untouched_02_03[2];
    float geometry_scale_04;
    float auxiliary_08;       // interpolated as float elsewhere; copied as a word
    float auxiliary_0c;       // interpolated as float elsewhere; copied as a word
}; // 0x10

struct Im2DVertex24 {
    float x, y, z;            // +0x00, +0x04, +0x08
    uint8_t r, g, b, a;       // +0x0C..+0x0F, in memory/FIFO order
    float u, v;              // +0x10, +0x14
}; // 0x18

struct BlurState13C {
    BlurControl16 control;              // +0x00
    uint32_t owner_address_10;           // target pointer, NOT host uintptr_t
    Im2DVertex24 draw_vertices_14[4];     // +0x14..+0x73
    uint32_t capture_raster_address_74;  // raster descriptor, not GXTexObj/image bytes
    uint8_t capture_flag_78;
    uint8_t untouched_79_7b[3];
    Im2DVertex24 copy_vertices_7c[4];     // +0x7C..+0xDB
    Im2DVertex24 copy_vertices_dc[4];     // +0xDC..+0x13B
}; // 0x13C
```

Evidence: allocation `8042F224 li r3,0x13c`; constructor range `[8042EA50,8042F084)`; draw stores in `8042E4F4-8042E600`; direct FIFO reads in `80491F14-80491FA8`. Parameters `+0x08/+0x0C` are used as floats in the separate interpolation branch `80439F9C-80439FCC`; that does not establish what they mean.

Constructor cautions:

- `8042EA6C/EA78/EA7C` initialize enabled/alpha to zero and scale from `0x8051C38C`; `EA80` stores owner. Failed manager/camera resolution returns early (`EAA8-EAD0`) without constructing all later fields. Do not add a whole-object zero fill.
- On the successful path, `EAF4-EB88` obtains each draw vertex's `z` from `(*0x805F265C)+0x18`, and sets initial RGBA bytes to `0xFF`; `EB8C/EB90` clear raster pointer/capture flag.
- `EE44/EE48` and `EE4C/EE54` are `srawi` plus `addze`: **signed divide by two, truncating toward zero**, not an unconditional arithmetic right shift. These produce the dimensions used by the smaller copy quad and raster allocation.
- Raster creation/association is `8042F00C -> 8048AEC4(0,0,0,0x505)`, `8042F024 -> 8048AEC4(halfWidth,halfHeight,0,0x505)`, then `8042F034 -> 8048AE14(firstRaster,secondRaster,&stackRect)`.
- With no supplied prefix, `F050-F068` select enabled `0`, alpha `0x80`, scale `F32[0x8051C38C]`; no stores initialize the two auxiliary prefix fields.

## 5. Core decompilation: exact guards, arithmetic, and geometry

### 5.1 Notation and floating/integer requirements

`F32[A]`/`F64[A]` denote big-endian values loaded from the actual target address. `FSUBS`, `FADDS`, `FMULS`, `FDIVS`, `FMADDS`, `FNEG`, and `FCTIWZ` below are **PPC-instruction notation**, not invented original helper functions. Every `...S` arithmetic instruction has its own single-precision rounding point. `FMADDS` is fused, with a single rounding for that instruction. Preserve FPSCR/NaN semantics if an executable emulator-style reconstruction is required.

Constants read by DrawBlur:

| Address | Use | Status |
|---|---|---|
| `0x8051C388` | `h`, added before UV division and multiplied into geometry displacement | H for literal value; I for address/use. Likely `0.5f`, not raw-verified. |
| `0x8051C38C` | `k`, baseline subtracted from geometry scale | H for literal value; I for address/use. Likely `1.0f`, not raw-verified. |
| `0x8051C390` | double subtracted from `0x43300000 : (word ^ 0x80000000)` | Standard signed-conversion-bias pattern; raw double still needs checking. |
| `0x8051C398` | double subtracted from `0x43300000 : word` | Standard unsigned-conversion-bias pattern; raw double still needs checking. |
| `0x8051C378`, `0x8051C380` | pairs of words copied into temporary UV pairs before all four words are overwritten | Do not mistake these initial copies for a second UV input. |

For literal-independent exact arithmetic define the following *notation*:

```text
SignedWordFloat(w) = FSUBS(bitcast_f64(0x4330000000000000 | (uint32(w) ^ 0x80000000)), F64[0x8051C390])
UnsignedWordFloat(w) = FSUBS(bitcast_f64(0x4330000000000000 | uint32(w)), F64[0x8051C398])
wrap_add32(a,b) = uint32(a) + uint32(b), modulo 2^32
```

After verifying the two biases as `0x4330000080000000` and `0x4330000000000000`, respectively, these reduce to rounded signed/unsigned 32-bit conversions. Do not silently make the parent dimensions signed just because normal dimensions are positive.

### 5.2 Guard/call skeleton with real addresses, not fictional helper identities

```text
DrawBlur_8042E254(s):
    if byte[s+0x00] == 0: return                         // E268-E270
    if byte[s+0x01] == 0: return                         // E274-E27C
    p = CALL 0x8042B620()                               // E280: returns *0x805E25C8
    p = CALL 0x803A1AFC(p, 0, 0x8056BCE4, 0x8056BCD0, 0)// E284-E29C
    if p == 0: return                                  // E2A0-E2A4
    camera = CALL [ [p+0x00] + 0x24 ](p)               // E2A8-E2B4
    if camera == 0: return                             // E2B8-E2BC
    if byte[s+0x78] == 0: return                        // E2C0-E2C8

    GET_DEVICE_STATE(10, stack+0x24)                    // actual slot +0x24, E2E8
    GET_DEVICE_STATE(11, stack+0x20)                    // E308
    GET_DEVICE_STATE(12, stack+0x1C)                    // E328
    GET_DEVICE_STATE( 6, stack+0x18)                    // E348
    GET_DEVICE_STATE( 8, stack+0x14)                    // E368
    GET_DEVICE_STATE(14, stack+0x10)                    // E388
    GET_DEVICE_STATE( 9, stack+0x0C)                    // E3A8

    raster = word[camera+0x60]                         // E3AC
    CALL 0x8048AB7C(raster, stack+0x0A, stack+0x08)     // E3BC
    // Perform exactly the position/UV/color work documented below.

    textureRaster = word[ word[s+0x74] + 0 ]            // E604-E608; NO pointer guard
    if CALL 0x8048652C(camera) == 0: return             // E60C-E614; work above has occurred
    SET_DEVICE_STATE( 1, textureRaster)                // E634
    SET_DEVICE_STATE(10, 5)                            // E654
    SET_DEVICE_STATE(11, 6)                            // E674
    SET_DEVICE_STATE(12, 1)                            // E694
    SET_DEVICE_STATE( 6, 0)                            // E6B4
    SET_DEVICE_STATE( 8, 0)                            // E6D4
    SET_DEVICE_STATE(14, 0)                            // E6F4
    SET_DEVICE_STATE( 9, 1)                            // E714
    CALL_DEVICE_SLOT_30(4, s+0x14, 4)                  // E738: primitive, vertex pointer, count
    restore 10,11,12,6,8,14,9 from the saved words      // E758,E778,E798,E7B8,E7D8,E7F8,E818
    CALL 0x80486504(camera)                            // E820
```

Here `GET_DEVICE_STATE` means indirect call through `word[word[0x805F265C]+0x24]`, and `SET_DEVICE_STATE` uses `+0x20`. This is a **function table**, not a C++ virtual call with a hidden device `this` argument: `r3` is the numeric state and `r4` is its value/output pointer. Every call reloads the global table pointer. `CALL_DEVICE_SLOT_30` similarly loads `+0x30`; arguments are exactly `r3=4,r4=s+0x14,r5=4`.

`0x8048AB7C` is locally simple: `lha [raster+0x1C] -> sth [outX]`, `lha [raster+0x1E] -> sth [outY]`, `blr` (`full_disassembly.txt:1183952-1183956`). Camera wrappers `0x8048652C` and `0x80486504` call camera members `+0x18` and `+0x1C` respectively (`full_disassembly.txt:1179442-1179461`); conventional BeginUpdate/EndUpdate names are corroborated, not guessed arbitrary helpers.

### 5.3 Readable arithmetic reconstruction

Let `ox,oy` be the sign-extended 16-bit returned offsets; `w,hgt` the words at current raster `+0x0C,+0x10`; `pw,ph` the words at its immediate parent pointer `word[raster+0]`, offsets `+0x0C,+0x10`. The core does **one parent dereference**, not a loop seeking an ultimate root.

```text
h = F32[0x8051C388]
k = F32[0x8051C38C]

// Integer addition occurs before conversion, at E3D8 and E3E8.
u0 = FDIVS(FADDS(h, SignedWordFloat(ox)),                 UnsignedWordFloat(pw))
u1 = FDIVS(FADDS(h, SignedWordFloat(wrap_add32(ox,w))),     UnsignedWordFloat(pw))
v0 = FDIVS(FADDS(h, SignedWordFloat(oy)),                 UnsignedWordFloat(ph))
v1 = FDIVS(FADDS(h, SignedWordFloat(wrap_add32(oy,hgt))),   UnsignedWordFloat(ph))

// Ordered PPC operations; not (scale*w - w)*0.5 and not a fused expression.
width_f  = SignedWordFloat(w)
height_f = SignedWordFloat(hgt)
scale_delta = FSUBS(float[s+4], k)
height_product = FMULS(scale_delta, height_f)             // E5AC, before width product
width_product  = FMULS(scale_delta, width_f)              // E5B4
height_again = SignedWordFloat(hgt)                      // E5BC
half_height_delta = FMULS(h, height_product)              // E5C0
half_width_delta  = FMULS(h, width_product)               // E5C4
negative_y = FNEG(half_height_delta)                     // E5D0
negative_x = FNEG(half_width_delta)                      // E5D4
width_again = SignedWordFloat(w)                         // E5D8
positive_y = FADDS(height_again, half_height_delta)       // E5DC
positive_x = FADDS(width_again, half_width_delta)         // E5E4
```

The final vertex order is:

| Vertex/address | x | y | z | RGBA | u | v |
|---|---|---|---|---|---|---|
| 0, `s+0x14` | `negative_x` | `negative_y` | unchanged | `255,255,255,byte[s+1]` | `u0` | `v0` |
| 1, `s+0x2C` | `negative_x` | `positive_y` | unchanged | same | `u0` | `v1` |
| 2, `s+0x44` | `positive_x` | `negative_y` | unchanged | same | `u1` | `v0` |
| 3, `s+0x5C` | `positive_x` | `positive_y` | unchanged | same | `u1` | `v1` |

Do not add current raster offsets to these x/y coordinates in DrawBlur: they are absent from the stored positions. The backend's matrix translation adds raster offsets separately (`804916FC-80491754`). Do not clamp scale, alpha, UVs, or denominators. Do not normalize a zero displacement to positive zero: `fneg` can produce `-0.0` at the left/top edges.

### 5.4 Exact arithmetic/store instruction correspondence

Sources: `full_disassembly.txt:1089244-1089393`. This table deliberately retains original registers/operation order, including duplicate conversions.

| VA | Original instruction / effect |
|---|---|
| `8042E3C0` | `lha r0,0xA(r1)` -> signed x offset |
| `8042E3C8` | `lwz r10,0xC(r29)` -> current width |
| `8042E3D4` | `lha r8,8(r1)` -> signed y offset |
| `8042E3D8` | `add r0,r0,r10` -> 32-bit wrapping x+width |
| `8042E3DC` | `lwz r11,0x10(r29)` -> current height |
| `8042E3E8` | `add r0,r8,r11` -> 32-bit wrapping y+height |
| `8042E3F0` | `lwz r9,0(r29)` -> parent raster |
| `8042E400/E408` | `lwz r12,0xC(r9)` / `lwz r29,0x10(r9)` -> parent dimensions |
| `8042E418/E430/E438` | load signed bias / h / unsigned bias |
| `8042E42C` | `fsubs f3,f2,f0` -> signed x |
| `8042E440` | `fadds f6,f1,f3` -> h+x |
| `8042E44C` | `fsubs f3,f2,f0` -> signed x+width |
| `8042E45C` | `fadds f7,f1,f3` -> h+x+width |
| `8042E464` | `fsubs f3,f2,f0` -> signed y |
| `8042E474` | `fsubs f2,f2,f0` -> signed y+height |
| `8042E480` | `fadds f5,f1,f3` -> h+y |
| `8042E48C` | `fadds f3,f1,f2` -> h+y+height |
| `8042E494` | `fsubs f2,f4,f9` -> unsigned parent width |
| `8042E4A0` | `fdivs f8,f6,f2` -> u0 |
| `8042E4B8` | `fsubs f6,f4,f9` -> unsigned parent width again |
| `8042E4C0` | `fsubs f4,f2,f9` -> unsigned parent height |
| `8042E4CC` | `fsubs f2,f2,f9` -> unsigned parent height again |
| `8042E4D0` | `fdivs f6,f7,f6` -> u1 |
| `8042E4E0` | `fdivs f4,f5,f4` -> v0 |
| `8042E4E8` | `fdivs f2,f3,f2` -> v1 |
| `8042E4F4/E504` | store u0,v0 to vertex 0 `+24,+28` |
| `8042E514/E518` | store u0,v1 to vertex 1 `+3C,+40` |
| `8042E51C/E520` | store u1,v0 to vertex 2 `+54,+58` |
| `8042E524/E528` | store u1,v1 to vertex 3 `+6C,+70` |
| `8042E52C-E590` | white RGB stores; alpha reloads at E538/E550/E570/E588 and byte stores at E540/E558/E578/E590 |
| `8042E564` | `fsubs f4,f2,f0` -> signed current width |
| `8042E598` | `lfs f6,4(r30)` -> geometry scale |
| `8042E59C` | `fsubs f3,f2,f0` -> signed current height |
| `8042E5A4` | `fsubs f5,f6,f5` -> scale minus baseline |
| `8042E5AC` | `fmuls f3,f5,f3` -> height product |
| `8042E5B4` | `fmuls f4,f5,f4` -> width product |
| `8042E5BC` | `fsubs f2,f2,f0` -> signed current height again |
| `8042E5C0` | `fmuls f6,f1,f3` -> h*height product |
| `8042E5C4` | `fmuls f5,f1,f4` -> h*width product |
| `8042E5D0` | `fneg f4,f6` -> negative_y |
| `8042E5D4` | `fneg f3,f5` -> negative_x |
| `8042E5D8` | `fsubs f0,f1,f0` -> signed current width again |
| `8042E5DC` | `fadds f1,f2,f6` -> positive_y |
| `8042E5E0/E5E8` | store vertex 0 x/y to `+14,+18` |
| `8042E5E4` | `fadds f0,f0,f5` -> positive_x |
| `8042E5EC/E5F0` | store vertex 1 x/y to `+2C,+30` |
| `8042E5F4/E5F8` | store vertex 2 x/y to `+44,+48` |
| `8042E5FC/E600` | store vertex 3 x/y to `+5C,+60` |

The stack UV-pair stores at `E4BC/E4C8/E4D4/E4D8` initialize locals copied from constants; `E4DC/E4E4/E4EC/E4F0` overwrite all four elements. They can be identified as dead initial values at the C-dataflow level, not mistaken for a hidden geometry helper or history input.

## 6. RenderWare state contract and compound GX interpretation

### 6.1 Certain callsite contract; API names are decoded conventions

| ID | Conventional RenderWare state | Saved at | Set for draw | Restored at |
|---|---|---|---|---|
| 1 | texture raster | **not saved** | E634: parent of raster `s+74` | **not restored** |
| 10 | source blend | E2E8 | E654: 5, source alpha | E758 |
| 11 | destination blend | E308 | E674: 6, inverse source alpha | E778 |
| 12 | vertex alpha enable | E328 | E694: 1 | E798 |
| 6 | Z test enable | E348 | E6B4: 0 | E7B8 |
| 8 | Z write enable | E368 | E6D4: 0 | E7D8 |
| 14 | fog enable | E388 | E6F4: 0 | E7F8 |
| 9 | texture filter | E3A8 | E714: 1, nearest | E818 |

No direct `bl GX...` occurs in `[8042E254,8042E838)`. The backend below is a locally corroborated candidate for those slots, **pending raw function-table/pointer validation**. It must not be represented as a statically proven unconditional call graph until that last link is checked.

### 6.2 Backend candidates, addresses, and nontrivial state coupling

- State getter: **`[804984C4,804986B8)`**; jump table **`0x8056FA58`** (`full_disassembly.txt:1197858-1197982`).
- State setter: **`[80498954,80498F74)`**; jump table **`0x8056FAD4`** (`1198150-1198541`).
- State cache: **`0x805E42D0`**. Current texture descriptor pointer: **`word[0x805F270C]`**, whose `+0` is the raster and `+0x50` carries filter/address bits.
- Source/destination mapping table: **`0x8056FA28`**. The setter's blend calls pass `(1,T[src],T[dst],0)` to **`0x80399D44`**. Table entries 5/6 are conventionally GX `SRCALPHA=4` / `INVSRCALPHA=5`; **their raw words remain H here**.
- Source state arm `80498D38-80498DBC` compares cache `+34`, combines requested source with cached destination `+38`, calls at **`80498DA0`**, then updates cache `+34`.
- Destination arm `80498DC0-80498E2C` compares cache `+38`, combines cached source `+34` with requested destination, calls at **`80498E10`**, then updates cache `+38`. Preserve this sequencing: the first blend change may still use the old other factor.
- Z write arm `80498C38-80498CA0` updates cache `+0`, and calls **`GXSetZMode(1,cachedCompare,writeByte)`** at EQUIVALENT local sites **`80498C5C`/`80498C8C`**. It does not change the compare field.
- Z test arm `80498CA4-80498D34` updates cache `+4`, compare cache `+8`, and calls `0x80399DF0` at **`80498CD0`** for `(1,3,cachedWriteByte)` or **`80498D14`** for `(1,7,cachedWriteByte)`.
- Therefore disabling RW Z-test then RW Z-write yields a compound **`GXSetZMode(1,7,0)`** state when those changes are necessary: GX testing stays enabled with `GX_ALWAYS`, rather than an invented `GXSetZMode(0,...)`. Calls may be skipped when caches already match.
- The candidate vertex-alpha support is effectively always-on: getter arm **`80498630-8049863C`** writes `1`, setter arm **`80498E48-80498E4C`** returns the passed value without a stored enable. The state-ID-to-arm link needs the raw jump table, but this is an important likely no-op, not a reason to invent a new GX alpha toggle.
- Fog-disable arm `80498A38-80498A74` conditionally calls **`0x80399A0C`** with mode 0 and saved fog color/scalars, then clears cache `+10`.
- Filter arm `80498BFC-80498C14` changes the low byte of descriptor `+50`. It is applied through texture loading; it is not a stand-alone GX filter call in DrawBlur.

GX entrypoint identities verified against their cached instruction bodies:

| Local entry/range | Hardware-level evidence |
|---|---|
| `80399D44-80399D98`, GXSetBlendMode | merges mode/source/dest/logic into cached BP blend word `GX+1D0`, emits `0x61` then word; preserves unrelated cached bits (`full_disassembly.txt:937282-937302`). |
| `80399DF0-80399E24`, GXSetZMode | merges enable bit 0, compare bits 1-3, update bit 4 into `GX+1D8`, emits BP command (`937325-937337`). |
| `80399E24-80399E58`, GXSetZCompLoc | updates bit 6 of cached `GX+1DC`, emits BP (`937338-937350`). |
| `80399778-803997BC`, GXSetAlphaCompare | builds BP register `0xF3`, packs two low-byte refs, compare functions and combine operation (`936911-936927`). |
| `80396934-80396A04`, GXBegin | flushes dirty state, optionally calls `80396A04`, then emits byte `(primitive | vtxfmt)` and **16-bit** vertex count (`933950-934001`). |

Do not discard compound state or restore the entire GX context. Blend/alpha/depth/color-update bits reside in shared words; these functions merge fields. The effect's explicit state restoration is narrower than complete hardware restoration.

### 6.3 Im2D setup and texture-dependent alpha/Z-compare state

Candidate draw function **`[0x80491E80,0x8049245C)`**, size `0x5DC`, matches the map's Im2D family only after local address checking. It calls **`[8049148C,80491774)`** before vertex emission and **`[80491774,80491B08)`** afterward.

Textured setup path (`full_disassembly.txt:1190676-1190750`):

| Call PC -> local target | Actual arguments / conventional GX decode |
|---|---|
| `8049149C -> 80395758` | GXClearVtxDesc() |
| `804914A8 -> 8039530C` | GXSetVtxDesc(9,1): position direct |
| `804914C0 -> 80395790` | GXSetVtxAttrFmt(0,9,1,4,0): XYZ, F32 |
| `804914CC -> 8039530C` | GXSetVtxDesc(11,1): color 0 direct |
| `804914E4 -> 80395790` | GXSetVtxAttrFmt(0,11,1,5,0): RGBA, RGBA8 |
| `804914EC -> 803999E4` | GXSetNumTevStages(1) |
| `804914F4 -> 80397C80` | GXSetNumChans(1) |
| `80491514 -> 80397CBC` | GXSetChanCtrl(4,0,0,1,0,0,2) |
| `80491534 -> 80397CBC` | GXSetChanCtrl(5,0,0,0,0,0,2) |
| `80491550 -> 8039530C` | GXSetVtxDesc(13,1): texture 0 direct |
| `80491568 -> 80395790` | GXSetVtxAttrFmt(0,13,1,4,0): ST, F32 |
| `80491574 -> 803992EC` | GXSetTevOp(0,0): MODULATE |
| `8049157C -> 80395FC8` | GXSetNumTexGens(1) |
| `80491598 -> 80395D48` | GXSetTexCoordGen2(0,1,4,0x3C,0,0x7D) |
| `804915AC -> 80399848` | GXSetTevOrder(0,0,0,4) |
| `804915B0 -> 80498854` | texture flush/load and alpha/Z-compare-location coupling, described next |

These setup calls establish one texture stage modulated by vertex color. They do not establish an independently programmable multi-tap convolution or an additive accumulation loop. If the current texture raster is null, the backend selects its distinct untextured branch `804915B8-804915DC`, and the emitter omits UVs; DrawBlur itself does not guard a null parent word before binding it.

**Texture flush `[80498854,80498910)`** (`full_disassembly.txt:1198086-1198132`) calls **`8049AC88(currentTextureDescriptor,0)`** at `8049887C`. It reads a raster plugin word at raster plus `word[0x805F2700]+0x14`, extracts bit 0, and computes `newLoc = bit ^ 1` (`8049889C-804988A4`). When cache `0x805E42D0+0x3C` differs:

- If `newLoc == 1`, call **`GXSetAlphaCompare(7,0,0,7,0)`** at `804988CC` (always/AND/always).
- Otherwise call `GXSetAlphaCompare` using cached `+40,+4C,+48,+44,+4D`, at `804988E8`.
- Then call **`GXSetZCompLoc(uint8_t(newLoc))`** at `804988F0`, and cache the new value.

Thus alpha compare, texture format/plugin flags, and Z comparison location are coupled. A blanket claim that DrawBlur always installs one fixed alpha-compare tuple would be unsupported.

The setup also changes viewport, projection, matrix state: `80491638/80491680 -> 8039A444/8039A49C`; `804916EC -> 8039A1E4`; `804916F8 -> 8039A158`; `80491754 -> 8039A22C`; `8049175C -> 8039A2CC`. The exact matrix constants require raw reads. The post function restores projection and handles viewport/scissor branches using raster/display state (`80491774-80491B08`); it is not a complete descriptor/TEV/channel-state reset.

### 6.4 Loop and primitive mapping, without inventing a blur loop

At `80491EA4` the backend calls setup. At `80491EAC-80491EC0` it reads **`word[0x8056F120 + primitive*4]`**, masks count to 16 bits, and calls `0x80396934(tableEntry,0,uint16_t(count))`. For the core, primitive/count are **4/4**. RenderWare convention identifies 4 as triangle strip and predicts table entry **`0x98`**, but the raw table word must still be verified. Do not call this `GX_QUADS` merely because it has four vertices.

For primitive 4, control selects `80491EE8`. With a non-null current texture raster:

```text
80491EF8-80491F10: if count==0, skip; CTR = uint32(count) >> 1
80491F14-80491F58: emit vertex at current pointer
80491F5C-80491FA4: emit next vertex; pointer += 0x30
80491FA8:          bdnz 80491F14
80491FAC-80491FB0: if (count & 1)==0, skip tail
80491FB4-80492004: one-vertex tail, stride 0x18
```

The core's count 4 gives **two iterations of the pair-unrolled loop and no tail**. Each vertex emits, to **`0xCC008000`**, three `stfs` in x/y/z order, four `stb` in RGBA order, then two `stfs` in u/v order. This is 24 bytes per vertex, 96 bytes for the four vertex payloads. Attribute fetches within a vertex are scheduled in a different order; FIFO stores define emission order. The loop's counter is a 32-bit GPR/CTR value even though GXBegin consumes a 16-bit count. Do not globally truncate the emitter's loop count in a general backend reconstruction.

The SDK GXBegin itself can emit a pre-primitive flush at `803969D0 -> 80396A04` depending on cached state. This additional hardware maintenance must not be mistaken for a blur sample either.

## 7. SaveScreen: bounded reconstruction, no invented frame history

Named sibling execution candidate **`[801D2764,801D2788)`** calls `800A35D8`, then **`[8042F140,8042F16C)`**, which obtains manager `+40` and calls **`[8042E838,8042E9C4)`**. Sources: `full_disassembly.txt:470986-470994,1090113-1090123,1089535-1089633`.

```text
SaveScreen_8042E838(s):
    byte[s+0x78] = 0                                  // E850, before gates
    if byte[s] == 0 or byte[s+1] == 0: return          // E854-E868, separate tests
    resolve object/camera exactly as DrawBlur          // E86C-E8A8
    if either resolution fails: return

    sourceRaster = word[ word[ word[s+0x10] + 0x34 ] ] // E8AC-E8B8
    CALL 0x80496208(sourceRaster,0)                    // E8C0
    if CALL 0x8048652C(camera) != 0:                   // E8C8
        SET_DEVICE_STATE(1,sourceRaster)               // E8F0
        SET_DEVICE_STATE(10,2)                        // E910: ONE
        SET_DEVICE_STATE(11,1)                        // E930: ZERO
        CALL_DEVICE_SLOT_30(4,s+0x7C,4)               // E954
        CALL 0x80486504(camera)                        // E95C

    CALL 0x80496208(word[s+0x74],0)                    // E968
    if CALL 0x8048652C(camera) != 0:                   // E970
        CALL_DEVICE_SLOT_30(4,s+0xDC,4)               // E99C; no new texture-state call
        CALL 0x80486504(camera)                        // E9A4
    byte[s+0x78] = 1                                  // E9AC, regardless of begin results
```

This records the actual EFB/raster operations and two draw submissions. It is not evidence for an arbitrary number of retained frames or a runtime-confirmed scheduler/frame order. In particular, the second draw has no texture rebind at its callsite; do not insert one.

The directly called **`[80496208,80496410)`** is locally corroborated as camera-texture flush/copy (`full_disassembly.txt:1195635-1195764`):

| PC -> target | Actual behavior relevant to the calls above (`r4=0`) |
|---|---|
| `80496248 -> 80397228` | copy-filter setup with four zero arguments |
| `8049629C -> 80396C50` | texture-copy source `(uint16(offsetX),uint16(offsetY),uint16(width),uint16(height))` |
| `804962B8 -> 80396D00` | texture-copy destination dimensions from parent, low 16 bits; format from raster plugin `+0C`; mipmap byte 0 |
| `804963B8 -> 803975AC` | GXCopyTex(image pointer plus calculated offset, low byte of `word[0x805F2688]`) |
| `804963BC -> 80396568` | pixel-mode synchronization candidate |
| `804963D4 -> 80397228` | restore filter from display configuration; argument 3 is 1 |
| `804963E4 -> 8039868C` / `804963EC -> 803987B8` | texture-region/all texture-cache invalidation candidates depending on plugin pointer `+2C` |

The source/destination/copy arithmetic uses signed raster offsets, packed low-16-bit dimensions, and depth-dependent tiled image offset arithmetic. It is not a CPU pixel-history loop. Exact format-to-offset switch entries at `0x8056F544` and backend function names beyond the locally inspected GX packing remain raw/table confirmation items.

## 8. Control getters/setters and exact truncation/interpolation

### 8.1 Prefix copy APIs

| Local original range | Conservative reconstruction |
|---|---|
| `8042E9C4-8042E9F0` | Copy bytes +0/+1, float +4, and raw words +8/+C from r4 to r3. Does not copy padding +2/+3. |
| `8042E9F0-8042EA1C` | Same memberwise prefix assignment, differently scheduled; source is not null-checked. |
| `8042EA1C-8042EA50` | If r4 is null, return; otherwise same prefix copy. |
| `8042F0E8-8042F114` | Read manager r3+40; if non-null call `8042E9F0(object,r4)`. |
| `8042F114-8042F140` | Read manager r3+40; if non-null call `8042EA1C(object,r4)`. |
| `8042F084-8042F0E8` | r3 is destination prefix, r4 is manager; if manager+40 exists, call `8042E9C4(out,object)`. Otherwise set enable/alpha to 0 and scale from `8051C38C`; copy two **uninitialized local stack words** to output +8/+C (`F0C0-F0D0`). Do not fabricate zero auxiliary defaults. |

Sources: `full_disassembly.txt:1089634-1089668,1090066-1090112`. None of these prefix setters changes capture byte `+78`, binds a texture, clears a buffer, or validates/clamps floating values.

Known direct callers from `branches.tsv` and bounded instructions:

- Nullable-source wrapper `8042F114`: `800A356C`, `800A375C`, `800A481C`.
- Assignment wrapper `8042F0E8`: `8020475C`, `80205624`, `8020635C`, `80439FD8`, `8043A544`.
- Getter `8042F084`: `80204720`, `802055E8`, `80206320`, `8043A908`.
- Object prefix assignment `8042EA1C`: constructor `8042F048` and wrapper `8042F12C`.
- Draw wrapper `8042F16C`: only direct branch recorded is `801D280C`; core `8042E254`: only direct branch recorded is `8042F184`. Direct-branch artifacts do not enumerate arbitrary function-pointer calls.

The three `8020...` caller pairs read a prefix, copy it to a second local, replace **only enabled with zero**, and write it back. They preserve alpha, scale, and auxiliary words. Relevant exact spans: `80204714-8020475C` (`full_disassembly.txt:522166-522184`), `802055DC-80205624` (`523112-523130`), `80206314-8020635C` (`523958-523976`).

### 8.2 Direct on/off control and task-driven alpha conversion

Startup proves cached small-data bases **`r2=0x805FA780`**, **`r13=0x805EC500`** (`8000332C-80003338`, `full_disassembly.txt:140-143`). Therefore:

| r2 displacement | Resolved VA | Role in this controller; literal not raw-verified |
|---|---|---|
| `-0x6BB4` | `805F3BCC` | default prefix scale / fade upper bound |
| `-0x6BB0` | `805F3BD0` | installed geometry scale |
| `-0x6BAC` | `805F3BD4` | fade step divisor |
| `-0x6BA4` | `805F3BDC` | enable threshold / fade lower bound |
| `-0x6BA0` | `805F3BE0` | alpha multiplier |

**`[800A3528,800A35D8)`** installs `(enabled=0, alpha=0x80, scale=F32[805F3BD0])` through `8042F114`; **`[800A3714,800A37A4)`** installs `(enabled=1, alpha=0x80, sameScale)`. They also perform non-blur work not reconstructed here. Sources: `full_disassembly.txt:160571-160614,160694-160729`.

**`[800A47B0,800A4830)`**, size `0x80`, is the dynamic controller's prefix application:

```text
FCMPO(float[task+0x38], F32[805F3BDC])                  // 800A47D8, word FC010040
prefix.enabled = !(CR0.LT || CR0.EQ)                  // 47DC cror; 47E0 bne; 47E8 li 1
product = FMULS(F32[805F3BE0], float[task+0x38])        // 47FC
prefix.scale = F32[805F3BD0]                          // 4800
integer_bits = FCTIWZ(product)                       // 4804
prefix.alpha = low8(low32(integer_bits))              // 4808 stfd; 480C lwz +4; 4810 stb
manager = CALL 800A35D8()                             // 4814
CALL 8042F114(manager,&prefix)                        // 481C
```

`full_disassembly.txt:161757-161788` prints `FC010040` as `.byte`; direct PPC encoding identifies it as **`fcmpo cr0,f1,f0`**, not integer data. The comparison-plus-`cror` makes an unordered comparison choose enabled 1; replacing it with C `value > threshold` differs for NaN. For finite in-range products `fctiwz` truncates toward zero; **there is no saturation-to-byte/clamp** before the low byte is stored. A plain C float-to-int cast does not define PPC invalid/out-of-range results or FPSCR flags. Retain the PPC conversion operation, including invalid conversion semantics, rather than choosing fabricated fallback values.

**`[800A4830,800A492C)`** contains the task's fade update and other gameplay work. Blur-relevant operations:

- If byte `task+34` is nonzero, `800A4858 fdivs` computes step `delta/F32[805F3BD4]`, `4860 fsubs` subtracts it from `task+38`, and `4864` stores it. `486C fcmpo` / `4870 bge` clamp only if **LT** to the lower bound (`4874`). Unordered does not take this clamp.
- Otherwise `4884 fdivs`, `488C fadds`, `4890 stfs` add the step; `4898 fcmpo` / `489C ble` clamp when **GT** (or unordered, because this branch tests GT clear) to `F32[805F3BCC]` at `48A0`. For an executable exact model, implement the branch bits rather than assuming all C comparisons are interchangeable.
- `800A48C4` calls `800A47B0`. The later `800A48E8 fctiwz` is for a different gameplay call and is not a second blur conversion.

### 8.3 Global-parameter interpolation: preserve the fused instruction

Original full function **`[80439A8C,8043A4B8)`**, size `0xA2C`; only its **blur case `[80439EBC,80439FE0)`** is reconstructed here. The dispatch at `80439AE0-80439B0C` selects that case for parameter type **1**. Common gate calls `8042B63C`, checks bits of `self+18`, then resolves manager by `803A1AFC`. Source: `full_disassembly.txt:1100948-1100980,1101216-1101288`.

The object's saved prefix is at **`+14C`**, desired prefix at **`+15C`**, progress float at **`+100`**. Desired prefix construction is separately visible at `8043A900-8043A974`: getter fills saved values, a flags bit selects desired enabled, parameter bytes/float words populate desired alpha/scale/auxiliaries (`full_disassembly.txt:1101873-1101902`).

```text
out = memberwise_copy(self+0x15C)                      // EBC-EE8; padding not copied
FCMPO(float[self+0x100], F32[0x8051CB58])               // EF0
if CR0.LT:                                           // EF4 bge skips if LT is clear, including unordered
    out.enabled = (out.enabled != 0 || byte[self+0x14C] != 0) ? 1 : 0
    a = byte[self+0x14D]                              // saved alpha
    b = out.alpha                                    // desired alpha
    if a != b:                                       // F28/F2C
        t = float[self+0x100]
        c = FSUBS(F32[0x8051CB5C], t)                 // F54
        af = FSUBS(bitcast_f64(0x4330000000000000 | a), F64[0x8051CB60]) // F5C
        weighted_a = FMULS(af,c)                      // F68
        bf = FSUBS(bitcast_f64(0x4330000000000000 | b), F64[0x8051CB60]) // F6C
        mixed = FMADDS(bf,t,weighted_a)               // F70: fused bf*t + weighted_a
        out.alpha = low8(low32(FCTIWZ(mixed)))        // F74-F80

    t = float[self+0x100]                            // F88, reloaded
    c = FSUBS(F32[0x8051CB5C], t)                     // F94, recomputed
    out.scale = FMADDS(out.scale,t,FMULS(float[self+0x150],c))     // FA4/FA8/FAC
    out.aux08 = FMADDS(out.aux08,t,FMULS(float[self+0x154],c))     // FB4/FB8/FBC
    out.aux0c = FMADDS(out.aux0c,t,FMULS(float[self+0x158],c))     // FC4/FC8/FCC
CALL 0x8042F0E8(manager,&out)                         // FD8
```

Do not replace this with `a + (b-a)*t`, `std::lerp`, a clamp, or a double-precision formula. Equal alpha bytes bypass conversion entirely. When the blend block is skipped, enabled is the original desired byte, not forcibly normalized to 0/1. `8051CB58` (comparison threshold), `8051CB5C` (complement baseline), and `8051CB60` (unsigned conversion bias) need raw values; do not assume the threshold and complement baseline are identical merely from their roles.

The destructor **`[8043A4B8,8043A5A4)`**, type-1 arm at `8043A540-8043A548`, writes **desired prefix +15C** through `8042F0E8` (`full_disassembly.txt:1101633-1101635`). It does not restore the saved prefix +14C in this arm. Calling this an automatic restore of the previous blur state would be incorrect.

## 9. Globals and dependency confidence

| Global/data address | Evidence and conservative meaning |
|---|---|
| `805E25C8` | manager/service pointer returned directly by `8042B620-8042B630` (`full_disassembly.txt:1086329-1086332`) |
| `805EF2D8` | lazy manager singleton pointer, r13+2DD8 in `800A35D8-800A360C` (`160615-160627`) |
| `805EF629` | singleton initialization byte r13+3129; distinct from blur enable |
| `8057798C` | static manager storage returned by creator `800A360C-800A3664` |
| `805F265C` | RenderWare engine/device pointer; absolute loads in core; r13+615C in backend |
| `805F270C` | current texture descriptor pointer, r13+620C |
| `805F2700` | raster plugin offset, r13+6200 |
| `805F26F4` | display configuration pointer, r13+61F4 |
| `805F2688` | low-byte clear argument read by camera texture flush, r13+6188 |
| `805E42D0` | compound RenderWare GX state cache (blend/depth/fog/alpha-compare fields) |
| `805FB758` | SDK GX state/context pointer loaded at r2+FD8 |
| `8056BCE4/8056BCD0` | cast/type descriptor addresses passed to `803A1AFC`; names not established here |
| `8053DC78/8053DC5C` | task vtables installed by DrawBlur/SaveScreen constructors; actual entries pending |
| `8056FA28/8056FA58/8056FAD4/8056F120` | blend translation/getter switch/setter switch/primitive translation tables; raw entries pending |
| `8051C388/8051C38C/8051C390/8051C398` | exact draw constant addresses; values pending |
| `8051CB58/8051CB5C/8051CB60` | exact interpolation constant addresses; values pending |

Confidence summary:

- **High, conditional on the cached artifact matching main.dol:** core address/range, direct branch destinations, guard order, member widths/offsets, arithmetic instruction order, no draw tap loop, four-vertex layout, low-byte truncation sites, no texture restoration, SaveScreen flag behavior, wrapper/null behavior, fused interpolation order.
- **Medium-high:** conventional RenderWare/GX identities corroborated by function shape and hardware word packing. Numeric state IDs and call arguments themselves are high-confidence instruction evidence.
- **Medium:** named task-to-Exec connection, actual currently installed device slots, switch state-ID-to-arm links. Raw vtable/dispatch/jump tables were inaccessible to this background run.
- **Unverified here:** literal floating values, blend/primitive translation words, FPSCR runtime setting, current camera state, scheduler/frame order, image provenance across frames, and every runtime/visual claim.

## 10. Parent validation actions before integration

1. Re-hash current `main.dol` and compare with the documented size/hash. No successful hash command ran in this specialist.
2. Freshly disassemble the critical ranges. The existing tool is read-only for these commands; do not use `all` or execute `analyze_dol.main()`:

```text
python -B motion_blur_analysis/ppc_tools.py disasm 0x801D2058 0x801D213C
python -B motion_blur_analysis/ppc_tools.py disasm 0x801D2764 0x801D2820
python -B motion_blur_analysis/ppc_tools.py disasm 0x8042E254 0x8042F258
python -B motion_blur_analysis/ppc_tools.py disasm 0x800A47B0 0x800A492C
python -B motion_blur_analysis/ppc_tools.py disasm 0x80439EBC 0x80439FE0
python -B motion_blur_analysis/ppc_tools.py disasm 0x8049148C 0x80491774
python -B motion_blur_analysis/ppc_tools.py disasm 0x80491E80 0x8049200C
python -B motion_blur_analysis/ppc_tools.py disasm 0x804984C4 0x80498F74
python -B motion_blur_analysis/ppc_tools.py disasm 0x80496208 0x80496410
python -B motion_blur_analysis/ppc_tools.py xrefs 0x8042E254
python -B motion_blur_analysis/ppc_tools.py xrefs 0x8042F16C
python -B motion_blur_analysis/ppc_tools.py pointers 0x804D1058
python -B motion_blur_analysis/ppc_tools.py pointers 0x801D27FC
python -B motion_blur_analysis/ppc_tools.py pointers 0x80491E80
python -B motion_blur_analysis/ppc_tools.py pointers 0x80498954
python -B motion_blur_analysis/ppc_tools.py pointers 0x804984C4
```

3. Import `analyze_dol` with `python -B`, and use its existing `data`, `fileoff`, `u32`, `va` mapping without running its main routine. Required raw windows:

```text
8053DC50..8053DC94     task vtable / nearby RTTI; connect DrawBlur name to Exec
8051C358..8051C3A4     draw/constructor scalar constants, signed/unsigned biases
8051CB58..8051CB70     interpolation threshold, complement, unsigned/signed biases
805F3BCC..805F3BEC     player-controller constants
8056F120..8056F13C     primitive table; verify entry 4 == 0x98
8056FA28..8056FA58     blend table; verify entries 5/6 and retained factors
8056FA58..8056FB50     getter/setter state jump tables; verify IDs 1,6,8,9,10,11,12,14
8056F544...           camera-texture flush depth switch if integrating that dependency
```

4. Check actual device-table registration/pointers for slots `+20/+24/+30`; do not assume a map's backend table or a single constant map delta. The BSS device pointer's runtime value is not recoverable merely by reading its initial bytes.
5. When translating to compilable C/C++, use 32-bit target addresses/integers, signed 16-bit origins, modulo-32-bit integer addition, separate single-precision operations in DrawBlur, an explicit fused operation at `80439F70/FA8/FB8/FC8`, and a defined PPC `fctiwz` model rather than undefined host out-of-range casts. Preserve `fcmpo` condition bits, including unordered behavior if modeling nonfinite inputs.
6. Do not claim runtime/image validation from this report. A real captured trace would be needed to validate backend installation, GX state before/after, buffer contents, and temporal scheduling.

## 11. Work/change log

Successful terminal commands: `ls` in the workspace; `ls "analysis/motion_blur/evidence"` to verify the owned report's parent directory. Both were read-only listings.

Denied before execution:

```text
python -B motion_blur_analysis/ppc_tools.py pointers 0x804D1058
python -B motion_blur_analysis/ppc_tools.py pointers 0x804D0F18
python -B motion_blur_analysis/ppc_tools.py disasm 0x801D1C50 0x801D1D60
python -B -c "... import analyze_dol; print size/hash/sections/string bytes/vtable words ..."
```

Other work used specialized read/grep/glob tools on existing guidance and analysis artifacts. One required initial Fast Context search returned no relevant code. No further delegated investigation was used. A mistyped map path produced a read/search error and was corrected; no files were changed by that error.

Created: **`analysis/motion_blur/evidence/11_decompiler.md`**. No optional `.cpp` was created because unresolved literal/table data should not masquerade as a build-ready implementation. No build, test suite, emulator, or runtime trace was run; all original/prior files remained read-only.

# Role 10: motion-blur constants, tables, and address evidence

## 1. Evidence contract

Scope is motion blur and its directly required input, task, raster, Im2D, and GX dependencies only. No DOL, prior analysis, Sol file, tool, or manifest was modified. This report owns only `evidence/10_constants.md`, `evidence/10_requests.json`, and `motion_blur_original_addresses.md`.

- Target: `main.dol`, 5,773,024 bytes, SHA-256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`.
- **V** means original bytes supplied by the parent's newly verified manifests, independently decoded here. It does not mean this role reran the parent's DOL verifier.
- **L** means a bounded read of the existing local `motion_blur_analysis/full_disassembly.txt`, with the actual printed VA and instruction word checked. It is not fresh extraction from the DOL by this role.
- **U** means the address/use is established, but raw data or a runtime value remains unavailable. Missing raw data is never filled in from a plausible instruction mnemonic, reference-map name, or SDK convention.
- **R** means a runtime address/value must be resolved through a pointer or register; it is not a fixed original data value.
- Ranges are half-open `[start,end)`. Numbers prefixed `0x` are hexadecimal. Sizes without a prefix in JSON are bytes in decimal.
- `DM` = `analysis/motion_blur/evidence/data_manifest.json`; `AM` = sibling `address_manifest.json`; `FM` = sibling `function_manifest.json`; `D` = `motion_blur_analysis/full_disassembly.txt`; `O/<file>` = `analysis/motion_blur/evidence/original/<file>`.
- Initial snapshot: 42 byte-verified exported instruction ranges, 25 data records, including 17 file-backed data records (1,636 bytes) and eight zero-backed runtime slots. Three of the 42 instruction ranges contain multiple original functions; see section 10. A verified byte interval does not prove the supplied function boundary/name.
- Bounded D addressing used `line = 1 + (VA - 0x80006840)/4`. Examples actually checked: line 161757 is `800A47B0`; 1101216 is `80439EBC`; 1194278 is `80494CD4`; 1196807 is `80497458`; 1200403 is `8049AC88`. No search over the >4 MB full disassembly was used.

## 2. Actual float bits, not mnemonic guesses

### 2.1 Core geometry pool: V

`DM:63-92` supplies `[0x8051C358,0x8051C3A4)`, file `[0x00519358,0x005193A4)`, 0x4C bytes.

| Address/range | Actual big-endian bits | Independent decoding/use |
|---|---|---|
| `8051C358..8051C368` | four `00000000` words | Constructor rectangle template. X/Y remain zero; W/H are subsequently overwritten. |
| `8051C368..8051C370` | two `00000000` words | Constructor U endpoint temporary template; both endpoints subsequently overwritten. |
| `8051C370..8051C378` | two `00000000` words | Constructor V endpoint temporary template; subsequently overwritten. |
| `8051C378..8051C380` | two `00000000` words | Draw U endpoint temporary template, not a stored velocity vector. |
| `8051C380..8051C388` | two `00000000` words | Draw V endpoint temporary template, subsequently overwritten. |
| `8051C388` | `3F000000` | f32 **0.5**, exponent 126, fraction 0. Used for UV half-texel numerators and half screen expansion. |
| `8051C38C` | `3F800000` | f32 **1.0**, exponent 127, fraction 0. Neutral/default scale. |
| `8051C390..8051C398` | `4330000080000000` | f64 **2^52 + 2^31 = 4503601774854144**. Signed-integer conversion bias after `xoris ...,0x8000`. Do not decode its two words as two float constants. |
| `8051C398..8051C3A0` | `4330000000000000` | f64 **2^52 = 4503599627370496**. Unsigned-integer conversion bias. |
| `8051C3A0` | `00000000` | f32 **positive zero**, for rectangle positions. |

Consumers independently checked: `O/8042EA50_MotionBlur_Construct.asm:14,48-56,87-123,202,220-263`; `O/8042E254_MotionBlur_Draw.asm:107-179`. The fact that the zero templates are loaded before overwritten does not establish a whole-object memset or initialize the opaque parameter words.

### 2.2 Device and Im2D data: V

`DM:488-554` establishes the following initialized **mutable data objects**, not heap pointers:

- Device descriptor `[8056F50C,8056F544)`, 0x38 bytes. Its f32 fields are `+00=3F800000` (1.0), `+08=00000000` (+0.0), `+0C=3F7FFFFF` (**1 - 2^-24 = 0.999999940395355224609375**). The last is not exactly 1.0. The scalar at +00 need not be relabelled as a depth value; near/far values are at +08/+0C.
- The descriptor is copied into the engine's device area at `engine+0x10`; therefore the constructor's `lfs engine+0x18` reads descriptor+0x08, initially **Z=+0.0**. A future runtime mutation is distinct from this initial value. Constructor consumers: `O/8042EA50_MotionBlur_Construct.asm:38-83`; descriptor return is `O/804960F0_RwDevice_GetGameCubeDevice.asm`; copy slice is D:1194521-1194542 at `804950A0-804950F4`.
- GX projection vector `[8056F13C,8056F158)` = f32 **`[1,1,-1,1,1,-1,-1]`**, with negative one bits `BF800000`.
- Position matrix `[8056F158,8056F188)` = row-major **`[[1,0,0,0],[0,1,0,0],[0,0,-1,0]]`**. This is not a 3D identity matrix: its Z sign is negative.

The SDA Im2D pool (`DM:557-577`) is `[805FC3F8,805FC420)`, file `[00581598,005815C0)`:

| VA | r2 displacement | Actual bits/value |
|---|---|---|
| `805FC3F8` | `+1C78` | `00000000`, +0.0f |
| `805FC3FC` | `+1C7C` | `3F800000`, 1.0f |
| `805FC400` | `+1C80` | `40000000`, 2.0f |
| `805FC404` | `+1C84` | `C0000000`, -2.0f |
| `805FC408` | `+1C88` | `3F000000`, 0.5f |
| `805FC40C` | `+1C8C` | zero word; alignment/padding status not promoted to a consumed constant |
| `805FC410..418` | `+1C90` | `4330000000000000`, unsigned f64 bias |
| `805FC418..420` | `+1C98` | `4330000080000000`, signed f64 bias |

`Im2D_Prepare` overwrites projection+4 with `f32(2 / u16(mode+4))`, projection+0xC with `f32(-2 / u16(mode+8))`, position+0xC with `f32(0.5 + signed view.x)`, and position+0x1C with `f32(0.5 + signed view.y)`. It saves/restores projection separately. Evidence: `O/8049148C_Im2D_Prepare.asm:131-183`. Mode+8, not blindly mode+6, is used in this setup. Do not replace these runtime denominators with a presumed display resolution.

### 2.3 Geometry/UV reconstruction consequences

Let `S(n)` and `U(n)` denote the actual signed and unsigned conversion sequences, including single rounding by `fsubs`; let each arithmetic operation below be the corresponding f32 PPC operation. For a camera view `(x,y,w,h)` and root `(pw,ph)`:

```text
u0 = (0.5 + S(x))       / U(pw)
u1 = (0.5 + S(x + w))   / U(pw)
v0 = (0.5 + S(y))       / U(ph)
v1 = (0.5 + S(y + h))   / U(ph)
q  = state.scale - 1.0
dx = 0.5 * (q * S(w))
dy = 0.5 * (q * S(h))
positions = [(-dx,-dy),(-dx,S(h)+dy),(S(w)+dx,-dy),(S(w)+dx,S(h)+dy)]
UVs       = [(u0,v0),(u0,v1),(u1,v0),(u1,v1)]
```

`x+w` and `y+h` are **32-bit integer additions before conversion**. Signed offsets/geometry and unsigned root denominators are different paths. Order is TL, BL, TR, BR. The four 0x18-byte vertices begin at state+0x14, +0x2C, +0x44, +0x5C; layout is `f32 x,y,z; u8 r,g,b,a; f32 u,v`. RGB=255, alpha is the byte at state+1. Default alpha 0x80 is 128, not a raw float 0.5 and not exactly 50% under division by 255.

The constructor's full-screen and half-screen capture quads have the same half-texel UV endpoints `0.5/U(W)` through `(0.5+S(W))/U(W)` and similarly for H. Full quad is state+0xDC; half quad is state+0x7C. Half dimensions use **`srawi; addze`**, signed divide by two truncated toward zero, not an arbitrary fixed size. The copy/draw code does not clamp these UVs to [0,1] or omit the 0.5 bias. See `O/8042EA50_MotionBlur_Construct.asm:87-164,220-263` and `02_powerpc.md:89-126,183-200` for full operation ordering, now with its former symbolic H/O/Z constants resolved by actual bytes.

## 3. GX/RW lookup and dispatch tables

### 3.1 Blend and primitive tables: V

`[8056FA28,8056FA54)` is eleven big-endian u32 words (`DM:94-115`). **RW values are indices, not GX factor values.**

| RW index | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| GX word | 0 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 2 | 3 |

- Main Draw requests RW source/destination **5/6**, which becomes GX **4/5** (SRCALPHA/INVSRCALPHA).
- Save requests RW **2/1**, which becomes GX **1/0** (ONE/ZERO).
- GX raw values 2/3 have source/destination-color meanings depending on which factor field is used. The repeated table values at RW 3/9 and 4/10 are real; don't invent a one-to-one enum conversion.
- Setter validation differs for source and destination; source rejects RW 3/4 and out-of-range values, destination accepts 1..8. This does not change the blur's accepted 5/6 and 2/1 pairs.
- Actual GX writer is `[80399D44,80399D98)`: source field is bits 8..10, destination 5..7, operation mode 1, logic-op argument 0 on these paths. It preserves other cached fields, emits to `CC008000`, and updates cached BP state. Do not claim a complete fixed blend register without the inherited write masks. `O/80498954_RwRenderState_Set.asm:254-314`; `O/80399D44_GXSetBlendMode.asm:5-25`.

`[8056F120,8056F13C)` is seven u32 primitive opcodes (`DM:117-134`):

```text
index:  0    1    2    3    4    5    6
word:   00   A8   B0   90   98   A0   B8
```

The blur submits RW primitive **4**, count **4**, so it reaches **GX 0x98 triangle strip**, not a triangle fan. VAT argument is zero. Actual table element address is `8056F130`; consumer `80491EB8-80491EC0` in `O/80491E80_Im2D_RenderPrimitive.asm`.

### 3.2 Setter dispatch decisively corrects old state-name assumptions: V

`[8056FAD4,8056FB50)`, 31 code-pointer words indexed directly by state ID (`DM:136-177`; `O/80498954_RwRenderState_Set.asm:5-19`). These are **interior branch destinations, not individual original function starts**.

| Effect state ID | Raw target | Actual local action |
|---|---|---|
| 1 | `80498C18` | Compare/bind raster in the runtime texture object's first word; call `8049819C` if changed. |
| 6 | `80498CA4` | Z-test request. Disabling changes compare to GX ALWAYS=7 while preserving the cached write flag; GX enable arg remains 1. |
| 8 | `80498C38` | Z-write request, preserving cached compare. |
| **9** | **`80498BFC`** | **Replace low byte of `texture+0x50`; this is the filter selector. Not culling, not flat shading.** |
| 10 | `80498D38` | Validated source blend lookup/cache. |
| 11 | `80498DC0` | Validated destination blend lookup/cache. |
| **12** | **`80498F58`** | **Default failure exit, r6 was initialized 0. No vertex-alpha-enable mutation occurs here.** |
| 14 | `80498990` | Fog enable/disable path. |
| 20, for disambiguation | `80498E50` | The actual cull-mode request: calls `80396B54(value-1)`. Blur does not request this ID. |

The full raw target vector for reproducibility is:

```text
0:F58 1:C18 2:B6C 3:BA4 4:BD0 5:E48 6:CA4 7:E30
8:C38 9:BFC 10:D38 11:DC0 12:F58 13:E40 14:990 15:A80
16:AF8 17:B64 18:F58 19:F58 20:E50 21:F58 22:F58 23:F58
24:F58 25:F58 26:F58 27:F58 28:F58 29:E78 30:F18
```

Here append the three hex digits to prefix `80498`, e.g. `990 -> 80498990`.

Get-state has a **different unexported table at `8056FA58`**; its target for ID 12 must be dumped before claiming the stack output is initialized. The getter body includes unsupported exits with no output store (`O/804984C4_RwRenderState_Get.asm:31-32,94-95,128-129`). Draw ignores return values and later reloads its saved slots. **Do not silently initialize/restore a fabricated state-12 value.** The setter's ID-12 no-op is already proven independently of that remaining getter-table question.

## 4. Copy-depth dispatch and copy filtering

### 4.1 Depth jumps: V

`[8056F544,8056F5B8)`, file `[0056C544,0056C5B8)`, 29 u32 pointers (`DM:179-218`). Index is unsigned `(root.depth - 4)`; values above 28 take the error path before indexing.

| Root depth | Index / table VA | Raw destination | Byte offset from extension pixel base |
|---|---|---|---|
| 4 | 0 / `8056F544` | `804962E4` | arithmetic `((roundUp8(W)*y) + 8*x) >> 1` |
| 8 | 4 / `8056F554` | `8049630C` | `roundUp8(W)*y + 4*x` |
| 16 | 12 / `8056F574` | `80496330` | `(roundUp4(W)*y + 4*x) << 1` |
| 32 | 28 / `8056F5B4` | `80496358` | `(roundUp4(W)*y + 4*x) << 2` |
| Every other index 0..28 | all other entries | `80496380` | Report error and exit without GXCopyTex. |

`roundUp8(W)=(W+7)&~7`, `roundUp4(W)=(W+3)&~3`; the original adds/multiplies/shifts are 32-bit operations. Inputs x/y are signed 16-bit view origins. These are tile-address formulas, not ordinary linear `(y*W+x)*bytesPerPixel` formulas. All four targets are labels **inside** `RwRaster_CopyEFB`, not new functions. Evidence: `O/80496208_RwRaster_CopyEFB.asm:50-113`.

### 4.2 Direct copy arguments and filter constants

The two Save calls use helper argument zero. On that path (`O/80496208_RwRaster_CopyEFB.asm:17-49,109-126`):

- Source = `u16(view.x),u16(view.y),u16(view.width),u16(view.height)` to `80396C50`.
- Destination dimensions = root width/height narrowed to u16; format = `rootExtension+0x0C`; scale/mipmap = low byte of helper argument (zero) to `80396D00`.
- Actual destination = `be32(rootExtension+0x1C) + offsetAbove`.
- **Clear = low byte of be32(805F2688)**, not the helper argument. The initial zero-backed slot does not establish its value at capture time.
- `803975AC` is GXCopyTex; `80396568` is GXPixModeSync, despite the shifted map's contrary label. Their verified ranges are separate.
- AA/vfilter setup is first `(0,null,0,null)`; afterwards it is restored using mode+0x19, mode+0x1A, literal vfilter-enable 1, and mode+0x32. The two renders perform half-resolution reduction; helper scale=0 is not a hardware mipmap-downsample request.

The disabled-AA branch explicitly constructs BP words `01666666,02666666,03666666,04666666`. The disabled-vfilter branch packs coefficients **`[0,0,21,22,21,0,0]`**, sum 64, into BP 0x53/0x54. These are supported by the **actual immediate instruction words**, not an absent data table (`O/80397228_GXSetCopyFilter.asm:70-77,113-130`). The restored mode sample pattern/filter bytes still require the selected mode descriptor; do not substitute the disabled branch for runtime restored values.

## 5. Task names, vtables, and render-level records

### 5.1 String/vtable objects: V

| Data object | Exact range | Actual contents |
|---|---|---|
| Draw task name | `[804D0F18,804D0F21)` | `44 72 61 77 42 6C 75 72 00` = `DrawBlur\0` |
| Save task name | `[804D0F24,804D0F2F)` | `53 61 76 65 53 63 72 65 65 6E 00` = `SaveScreen\0` |
| Save vtable **prefix only** | `[8053DC5C,8053DC6C)` | `805E9AF8,00000000,801D270C,801D2764` |
| Draw vtable **prefix only** | `[8053DC78,8053DC88)` | `805E9B00,00000000,801D2788,801D27FC` |

`DM:3-53`. The first word is metadata-shaped and its pointee was not dumped; the second is zero; +8/+0xC are destructor/Exec function pointers. These pointers and constructor installation at task+0x18 close the callback identities without shifting the map. **Do not label 0x10 as the full vtable size**: only the consumed prefix was exported. Constructor evidence: `O/801D2058_DrawBlurTask_Construct.asm:10-30` and `O/801D20D8_SaveScreenTask_Construct.asm:10-24`.

Draw's scheduler parent is render-level manager+0x2C. Its separately allocated child Save task is registered under manager+0x58, **not under Draw**. Both task constructors set flag 0x100. Draw owns a pointer to Save at task+0x28; scheduling parent and ownership pointer are different relationships.

### 5.2 All 23 construction records independently decoded: V

Table `[804D24EC,804D26B8)`, 0x1CC bytes = 23 records * 0x14 (`DM:220-345`). Name pool `[804D22E8,804D24EC)`, 0x204 bytes (`DM:347-486`). Layout consumed by constructor: `+00 name pointer; +04 u8 draw-setup flag; +05..07 padding; +08 mask u32; +0C auxiliary selector u32; +10 root selector s32`. **Raw word `01000000` means flag byte 1 plus zero padding, not the integer 1 at +04.** Unexplained selector meanings are kept numeric.

| i | Record VA | Name VA / exact string suffix after `RenderLevel_` | flag | mask | aux | root |
|---|---|---|---|---|---|---|
| 0 | `804D24EC` | `804D22E8` PreRender | 0 | 0 | 0 | 0 |
| 1 | `804D2500` | `804D2300` OpeqWorld | 0 | 0 | 0 | 1 |
| 2 | `804D2514` | `804D2318` Opeq | 1 | 0x20 | 0 | 1 |
| 3 | `804D2528` | `804D232C` PunchWorld | 0 | 0 | 0 | 1 |
| 4 | `804D253C` | `804D2344` Punch | 1 | 0x800 | 0 | 1 |
| 5 | `804D2550` | `804D2358` TransWorld | 0 | 0 | 0 | 1 |
| 6 | `804D2564` | `804D2370` Trans | 1 | 0x40 | 0 | 1 |
| 7 | `804D2578` | `804D2384` AddWorld | 0 | 0 | 0 | 1 |
| 8 | `804D258C` | `804D239C` Add | 1 | 0x80 | 0 | 1 |
| 9 | `804D25A0` | `804D23AC` GlareWorld | 0 | 0 | 0 | 1 |
| 10 | `804D25B4` | `804D23C4` Glare | 1 | 0x80 | 0 | 1 |
| **11** | **`804D25C8`** | **`804D23D8` PostGlare** | **0** | **0** | **0** | **1** |
| 12 | `804D25DC` | `804D23F0` Sprite3D | 1 | 0x20 | 0 | 1 |
| 13 | `804D25F0` | `804D2408` Last | 0 | 0 | 0 | 1 |
| 14 | `804D2604` | `804D241C` Sprite | 1 | 0x418 | 1 | 2 |
| 15 | `804D2618` | `804D2430` OnFade | 1 | 0x418 | 0 | 2 |
| 16 | `804D262C` | `804D2444` Gindows | 1 | 0x418 | 0 | 2 |
| 17 | `804D2640` | `804D2458` SpriteLast | 0 | 0 | 2 | 2 |
| 18 | `804D2654` | `804D2470` PostRender | 0 | 0 | 0 | 3 |
| 19 | `804D2668` | `804D2488` SpriteBack | 0 | 0 | 0 | -1 |
| 20 | `804D267C` | `804D24A0` SpriteMiddle | 0 | 0 | 0 | -1 |
| 21 | `804D2690` | `804D24BC` SpriteFront | 0 | 0 | 0 | -1 |
| **22** | **`804D26A4`** | **`804D24D4` PostEffect** | **0** | **0** | **0** | **-1** |

`Opeq` and `Gindows` preserve the actual ASCII spelling. Do not silently correct them to conventional English names.

The verified constructor creates root nodes in slots +0x5C,+0x60,+0x64,+0x68; creates standalone +0x58 below +0x64; loads **record 22's name** for that standalone node; creates level indices 0..18 using root selectors; attaches 19..21 beneath level 14. Thus **DrawBlur attaches to level 11, PostGlare, root selector 1**, and **SaveScreen attaches to standalone PostEffect beneath root slot +0x64**. Record 22's -1 is not used as a normal `roots[-1]` index. Evidence: `O/801E4B0C_RenderLevels_Construct.asm:23-116,179-203`.

Four root name pointers are formed at `804D26C8,804D26DC,804D26F0,804D2704`, but these strings are outside the exported name pool. They are requested rather than guessed. Construction hierarchy is established; frequency per frame, multi-camera traversal, or captured execution order is **not** runtime-verified by this report.

## 6. SDA, static objects, runtime pointers, and GPU addresses

`DM:579-613` supplies actual startup words `3C40805F 6042A780 3DA0805E 61ADC500`, proving **r2=805FA780, r13=805EC500**. A D-form displacement is sign-extended; `lis` followed by `addi` must not be treated as concatenated unsigned halves.

Actual zero spans from startup table are `[8056FE00,805E4500)`, `[805EF020,805F277C)`, `[805FC540,805FC5EC)`. Initialized SDA `[805E4500,805EF020)` and SDA2 `[805F2780,805FC540)` overlap the enclosing header BSS interval but **are file-backed**, not zero-filled. The four-byte alignment gap `[805F277C,805F2780)` is neither exported initialized data nor one of those explicit zero spans.

| Fixed address / relative offset | Storage classification | Meaning and limits |
|---|---|---|
| `805EF2D8 = r13+2DD8` | zero-backed **pointer slot** | Lazy effect-manager cache. `800A35D8..800A360C` reads/writes it. |
| `805EF629 = r13+3129` | zero-backed guard byte | Lazy manager guard; D:160628-160649. |
| `8057798C` | zero-backed **static manager object**, not heap | Lazy initializer returns this address. Only relevant accessed extent, including +0x40, is claimed, not a guessed full class size. |
| `805779C0 = manager+34` | zero-backed pointer field | Points to a texture/wrapper whose **first word is another raster pointer**. This is not the saved history pixel buffer or a direct pixel address. |
| `805779CC = manager+40` | zero-backed pointer field | Points to the allocated 0x13C blur state; the pointee's absolute heap address is R. |
| `805EF060 = r13+2B60` | zero-backed pointer slot | Render-level manager cache. |
| `805EF208 = r13+2D08` | zero-backed guard byte | Render-level manager guard. |
| `80574384` | zero-backed static object | Render-level manager, observed slots through +0x68 (minimum consumed extent 0x6C). `800461A0..8004620C`, D:65113-65139. |
| `805F265C = r13+615C` | zero-backed engine pointer slot | Its live pointee contains device callbacks and allocator/standard callbacks. Not the device descriptor itself. |
| `805F2688 = r13+6188` | zero-backed dynamic word | Copy-clear control; copy uses low eight bits. |
| `805F2694 = r13+6194` | zero-backed dynamic word | Mode selector; code can assign 0,1,2 or 0x2A (custom), not a compile-time display size. |
| `805F26F4 = r13+61F4` | zero-backed descriptor pointer slot | Can point at default DOL descriptor or mutable custom descriptor at `805E4250`. |
| `805F2700 = r13+6200` | zero-backed integer slot | Raster extension offset registered at runtime. Not an extension pointer. |
| `805F2708 = r13+6208` | zero-backed pointer slot | Backend fallback raster, not the effect's history. |
| `805F270C = r13+620C` | zero-backed pointer slot | Live **texture object** used as render-state texture holder; +0 raster pointer, +0x50 filter/wrap word. Distinguish it from the separate static GX/RW state cache. |
| `805F2718 = r13+6218` | zero-backed integer slot | Texture extension offset; texture+offset holds GXTexObj/cache data. |
| `805E42D0` | zero-backed static state cache | At least `[+0,+0x4F)` consumed: +0 z-write, +4 z-test, +8 GX compare, +0xC cull selector, +0x10 fog, +0x34/+0x38 RW blend factors, +0x3C Z-compare location, +0x40..4E alpha-compare state. |
| `805FB758 = r2+0FD8` | initialized pointer **slot**, raw missing | GX writers dereference it, then access cached BP fields. Do not infer its pointee or call it a heap allocation without bytes. |
| `CC008000` | MMIO FIFO | GX command/data port; not a DOL data object, runtime heap, or texture pixel pointer. |

Blur state relative atlas (pointee R): `[+0,+0x10)` parameter storage, +0 enabled u8, +1 alpha u8, +4 scale f32, +8/+0xC opaque copied words; +0x10 owner; `[+0x14,+0x74)` draw vertices; +0x74 history **view** pointer; +0x78 valid byte; `[+0x7C,+0xDC)` half-size vertices; `[+0xDC,+0x13C)` full-size vertices. No live numeric value or unobserved padding initialization is asserted.

Raster R: `+0` root/parent; +0xC/+0x10 width/height words; +0x14 depth; +0x1C/+0x1E signed view origins. Root extension at `root + be32(805F2700)` has +0xC GX format, +0x14 flag consumed by alpha-compare setup, +0x18 allocation base, +0x1C 32-byte-aligned texture pixel base, +0x2C optional region, +0x32 max mip level byte. Runtime view/root pointers and allocated pixels are three distinct addresses, never aliases for the absolute slot `805779CC`.

## 7. Remaining input-control constants: exact addresses, U values

### 7.1 Dynamic and fixed writers

Actual writer `[800A47B0,800A4830)`, D:161757-161788, forms a parameter-shaped stack block and calls `8042F114`. Actual updater `[800A4830,800A492C)`, D:161789-161851, updates scalar+0x38 before calling it. Both were independently followed from instruction words.

| Data VA | r2 displacement | Actual consumer/use | Value status |
|---|---|---|---|
| `805F3BCC` | `-6BB4` | Temporary default scale at `800A47B8`; updater upper clamp at `800A4888` | U, do not assume 1.0 |
| `805F3BD0` | `-6BB0` | Submitted scale at `800A47F8`, fixed writers `800A3540/800A372C` | U, do not assume neutral or a zoom factor |
| `805F3BD4` | `-6BAC` | Scalar delta divisor at `800A4850/800A487C` | U, do not invent a fade duration |
| `805F3BD8` | `-6BA8` | Gap within requested bounded pool; not consumed in the blur slice inspected | U/unidentified |
| `805F3BDC` | `-6BA4` | Enable/lower-bound comparisons | U, do not infer zero from comparison shape |
| `805F3BE0` | `-6BA0` | Alpha multiplier at `800A47F0` | U, do not substitute 128 or 255 |

The actual alpha sequence is `fmuls -> fctiwz -> stfd -> lwz low word -> stb`. For finite in-range products it truncates toward zero then keeps **low eight bits**; no saturation to 0..255 is implemented. `FC010040` is an ordered FP comparison word even where D prints `.byte`. `cror LT,EQ` makes unordered input select enabled=1. The fixed writers have proven immediate alpha=0x80, enabled=0 at `[800A3528,800A3570)` and enabled=1 at `[800A3714,800A3760)`; those are **slices**, not their whole function extents. All these writers leave parameter+8/+0xC stack words uninitialized in the inspected construction.

The updater also references `805F3BE4` and `805F3BE8` for non-blur side effects outside the parameter writer. They are deliberately not requested as motion-blur constants. This report does not derive a velocity input or analyze unrelated gameplay.

### 7.2 Generic effect interpolation

`[80439EBC,80439FE0)` is a **type-1 interior block** of `[80439A8C,8043A4B8)`, not a new function. D:1101216-1101288 proves old parameters at object+0x14C, target at +0x15C, progress at +0x100; setter at `80439FD8 -> 8042F0E8`.

- Threshold f32 **8051CB58**, loaded `80439EC4`: U. Branch is ordered-less; unordered skips mixing.
- One-minus-t base f32 **8051CB5C**, loaded `80439F48/F8C`: U. Do not assume it equals the threshold.
- Unsigned alpha conversion f64 **8051CB60..8051CB68**, loaded `80439F4C`: U. The load/add pattern is not a raw byte dump.
- Mixing uses `fmuls(old,base-t)` followed by **single-precision fused `fmadds(target,t,oldProduct)`**, then alpha `fctiwz`/low-byte storage when endpoint alpha bytes differ. Three float words +4,+8,+0xC are interpolated, although only +4 is consumed by the blur passes.
- Direct shared progress updater `[8042B63C,8042B6CC)`, D:1086336-1086371, uses f32 **8051BFA4** as elapsed/duration comparison offset/bound, and **8051BFA8** as fallback/maximum progress. Both U. Preserve its ordered comparisons and f32 operation order.

### 7.3 The 8020xxxx callers add no new data constants

Independent bounded reads establish exactly these getter/disable/setter slices:

| Slice (not function) | Getter PC | Setter PC | Evidence |
|---|---|---|---|
| `[80204714,80204760)` | `80204720` | `8020475C` | D:522166-522184 |
| `[802055DC,80205628)` | `802055E8` | `80205624` | D:523112-523130 |
| `[80206314,80206360)` | `80206320` | `8020635C` | D:523958-523976 |

Each copies the current parameter fields, writes **immediate enabled=0**, and resubmits. Alpha/scale/opaque words are preserved from `8042F084`; there is no new float load from a constant pool or lookup-table access in these slices. The absent-state getter initializes enabled=0, alpha=0, scale=`8051C38C` (now verified 1.0), but transfers +8/+0xC from uninitialized stack words (`O/8042F084_MotionBlur_GetParameters.asm:15-24`). Do not invent a `8020xxxx` strength table or zero the opaque payload.

## 8. Backend texture filter and format: resolved addresses, bounded limits

### 8.1 Filter/wrap/LOD path

`80498BFC` writes the selector to the low byte of the texture holder+0x50. Initial backend setup `[804981E8,8049847C)` sets that byte to **2** and U/V nibbles to **1/1** with raw `ori ...,2` and `ori ...,0x1100` (`O/8049819C_RwTexture_SetRaster.asm:158-174`). This is an initialization default, not proof of Save's live inherited filter.

The actual realization function is **`[8049AC88,8049B568)`**, not the 0x1C-byte `8049819C` setter. It receives the texture and texture-unit 0 from `80498854`. D:1200403-1200970 proves:

- Filter conversion table **`[8056FC90,8056FCC8)`**, seven `{u32 min,u32 mag}` pairs, raw values U. Nonpaletted path indexes `8 * low8(texture+0x50)` (`8049B2FC-8049B310`); RW selector 1 address is **8056FC98/8056FC9C**. Do not fill in GX `(0,0)` solely because selector 1 conventionally means nearest.
- Wrap conversion table starts **8056FCC8**, before cache at **8056FCDC**, giving 0x14 bytes of candidate table extent. Code extracts U/V nibbles from +0x50 and uses these words as GXInitTexObj arguments (`8049B2C4-8049B2F8`). Values U, including initial index-1 word at 8056FCCC.
- Texture-unit cache starts **8056FCDC**, 8 pointer slots by the scoped code/map context; only unit 0 write at `8049B540/544` is directly needed. Its live contents are mutable, not recovered texture addresses.
- Texture extension holds a GXTexObj; raster root extension supplies +0xC format and +0x1C pixels. Nonpaletted initialization calls **80397E34** and LOD setup calls **803980C8**. +0x32 supplies max mip level, converted with f64 at **805FC4A0**; zero/minLOD/bias input f32 is **805FC498**. These constants are raw-missing.
- GXInitTexObjLOD `[803980C8,8039822C)` uses **byte table 805EED30** (`r13+0x2830`) indexed by GX min-filter value, then inserts its three-bit hardware encoding. This table too is raw-missing; a RW table value is not yet the final BP encoding. D:935459-935547.
- GX LOD clamp/fixed-point constants are in **805FB7C0..805FB7F8**; actual values U, including bias decode dependencies at `8039824C..8039828C`.
- Main Draw explicitly requests selector 1 and later restores the prior selector. Save requests **no filter state at all**, so its reduction/restoration draws inherit the previous value. A categorical nearest/bilinear claim for both passes is therefore unjustified even after the table dump.

### 8.2 Requested raster flags 0x505 are not GX format 0x505

`RwRaster_Create` at `[8048AEC4,8048AFC0)` supplies flags to engine standard callback +0x58 with `(r3=0,r4=raster,r5=flags)` (D:1184162-1184224). The candidate local backend `[80497CA8,80498058)` forwards r5 to **`[80497458,80497A70)`**, the format resolver, via `80497D18`. The standard callback installation table **8051D400**, 27 `{index,pointer}` records, is still raw-missing; it is copied/installed by `804954F8-804955FC` (D:1194799-1194863). Thus that indirect edge must remain conditional until the table is dumped.

Within the actual local resolver, the **0x505 path needs no format jump-table guess**:

1. Low three flag bits give raster type 5 (`8049746C-80497490`). Format byte mask gives 0x0500.
2. No palette flags are present; `804974BC-804974C0` reaches normal format handling.
3. Nonzero explicit format bypasses the default-depth jump table (`804974F0-F4`), compares 0x0500 (`8049768C-90`), and selects **80497850**.
4. Its nonpalette branch **80497884-8049789C** writes root extension+0xC = **6**, extension+0x14 = **1**, raster depth+0x14 = **32**. Tail stores format byte 5 at raster+0x23.

This proves **conditional on that installed backend**, 0x505 means type-5 camera-texture, explicit RW 0x0500, yielding GX format **6 (RGBA8)** and depth **32**, not RGB565/depth16. Evidence: D:1196807-1196846,1196936-1196953,1197061-1197080,1197190-1197196,1197339-1197371. The history view has zero dimensions at first creation; its separately allocated parent has half-camera-root dimensions. Backend zero-size creation sets no-pixels flag 0x80 (`80498024-30`). No literal history resolution or heap address is recovered.

The depth-32 copy table destination is already V (`8056F5B4 -> 80496358`). GX copy tile-count helper `[80397D6C,80397E34)` uses unexported format jump table **805689D0**, 61 words; GXInitTexObj `[80397E34,80398080)` uses **80568AC4**, 15 words. Their format-6 destinations must be obtained from raw entries before asserting complete tile-count/texture-cache setup. The RW default-depth resolver table `8056F96C` is **not needed for explicit 0x505** and is not requested.

## 9. Default mode dimensions: located, not guessed

Actual selection function `[80494CD4,80494EB4)`, D:1194278-1194397:

- Nonnull requested descriptor is copied into mutable BSS object **805E4250**, size 0x3C, and its address stored at **805F26F4**. Width/height for the custom RW video-info record at **8056F450** are copied from descriptor halfwords +4/+6. Runtime custom values cannot be dumped from the original file.
- Null descriptor queries `80380138`, then selector 0 chooses **805688C0**, selector 1 or 5 chooses **80568938**, selector 2 chooses **805688FC**. Each is a 0x3C-byte original GX render-mode descriptor. Their dimension/sample/filter bytes were **not** in the initial DM.
- RW video-mode query copies 0x18-byte records at **8056F408**, **8056F420**, **8056F438**, or custom **8056F450**, D:1194434-1194510.
- Required raw requests are `[805688C0,80568974)` and `[8056F408,8056F468)`. **No 640x448, 640x480, or other default dimension is claimed until those bytes arrive.** Even a verified mode's defaults would not establish the live camera-root dimensions used by blur.
- GX mode fields directly consumed here: +4 width, +6 EFB height (custom info), +8 XFB/viewport height, +0x18 field-rendering flag, +0x19 AA, +0x1A sample-pattern bytes, +0x32 seven vfilter bytes. The raw bytes will decide the actual values and selected-mode names.

## 10. Original function boundaries versus verified export containers

Three current export labels describe too-large intervals. Raw words/prologues/epilogues prove the following partition **without changing those original artifacts or manifests**:

| Export container | Actual constituent original functions | Evidence |
|---|---|---|
| `8049819C..804984C4` | `8049819C..804981B8` SetRaster (0x1C); `804981B8..804981E8` backend SubRaster (0x30); `804981E8..8049847C` backend Open (0x294); `8049847C..804984C4` backend Close (0x48) | `O/8049819C_RwTexture_SetRaster.asm:5-24,188-206` |
| `804984C4..80498854` | `804984C4..804986B8` state Get (0x1F4); `804986B8..804987C8` fog-enable adapter (0x110); `804987C8..80498854` Z-compare-location adapter (0x8C) | `O/804984C4_RwRenderState_Get.asm:128-132,192-205,227-232` |
| `80498854..80498954` | `80498854..80498910` texture FlushState (0xBC); `80498910..80498954` direct RW blend-pair adapter (0x44) | `O/80498854_RwTexture_FlushState.asm:46-68` |

The export byte hashes remain useful and are not disputed; the **one-export-equals-one-function** interpretation is. Parent should split/reclassify the manifests and source coverage, not mark 0x328 bytes as one SetRaster function or 0x100 as one FlushState function.

Semantic names throughout this report are analysis aliases for original routines/objects. Interior state/copy jump targets and the `8020xxxx` / `80439Fxx` slices are not independent functions. Helpers such as `ReadBE`, `S`, `U`, `roundUp4`, or a proposed `MixBlurParameters` are analysis notation with **no original function VA** unless an actual routine range is separately cited.

## 11. Coverage, follow-up, and changes

- The complete address atlas, including all 42 initial export intervals, corrected original boundaries, data objects, runtime pointer-relative fields, and additional local dependencies, is `analysis/motion_blur/motion_blur_original_addresses.md`.
- No reconstruction `.cpp/.c/.h/.hpp` existed in `analysis/motion_blur/` when Role 10 checked. Source-function coverage is therefore **pending source availability**, not assumed from reports or Sol. The atlas deliberately separates complete assembly coverage from source reconstruction coverage. Parent should update/confirm this column when new source exists; prior/Sol source does not count as reviewed current reconstruction.
- Machine-friendly missing data and narrow code-verification requests: `evidence/10_requests.json`. Highest priority: input pools, getter dispatch, filter pairs/wrap/GX encoding, standard callback table, and default mode records. Runtime requests (actual heap pointers, selected camera/raster dimensions, Save filter, clear flag, task frequency) cannot be resolved by a DOL dump.
- Parent should propagate two corrections immediately: **state 9 is filter**, and **state 12 is an unsupported setter request in this backend**, rather than validating conventional RW names by assumption. Preserve uncertain getter output until raw getter dispatch is read.
- Files created by this role only: `evidence/10_constants.md`, `evidence/10_requests.json`, `motion_blur_original_addresses.md`. Successful shell commands were only `pwd` and `ls -la` in the scoped `sys` directory. No disassembler/hash/export/build/emulator/configuration command was run by this role; no denied exec was retried. Data decoding used the supplied raw words; other investigation used read/glob/grep and bounded D reads. No runtime validation is claimed.

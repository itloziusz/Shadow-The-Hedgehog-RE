# Specialist 07: matrix, camera motion, fullscreen geometry

## 1. Result and evidence status

**The isolated blur draw is a fixed-center, screen-space rectangle expansion sampling a retained raster. It is not a reconstructed-camera-velocity or per-object-velocity calculation.** The strongest positive matrix finding is a **same-draw-call projection save/override/restore**, not a previous-frame camera matrix.

Important qualifications:

- The complete draw body and its save/constructor bodies were read instruction-by-instruction. Within those bodies, the copied parameters at state+0x08/+0x0C do not feed coordinates, UVs, or a matrix. They must not be named/implemented as an active radial center or direction merely because they look like two floats.
- There is retained image state. Absence of previous-camera matrices does **not** imply absence of temporal image reuse. This role does not establish the image's precise age or scheduler order.
- **Fresh Python disassembly and pointer commands were denied by background execution permissions.** I continued with bounded reads of the existing `motion_blur_analysis/full_disassembly.txt`, including its raw instruction words, without grepping that oversized file or rewriting it. No fresh DOL hash or raw data-constant dump was executed by this role.
- Consequently, **exact data-resident float bits remain an explicit integration blocker**, not values silently inferred from the map. Formulas below are given both in exact loaded-constant form and, separately, their likely conventional specialization. The latter is conditional until the listed data bytes are read.
- The matrix/backend indirect edges require the raw device table to close completely. Local backend bodies strongly match the expected functions, and their internal direct calls are established; that is not equivalent to having freshly read the function-pointer installation.

All addresses refer to this local DOL. Code ranges are end-exclusive. `FD:Lx-Ly` means lines in `motion_blur_analysis/full_disassembly.txt`. Confidence labels refer to static evidence, not runtime validation.

### Provenance and address correspondence

- Read `AGENTS.md:1-10` and the helper sources. The import-only byte/mapping contract is `motion_blur_analysis/analyze_dol.py:5-24`; disassembly and direct-branch/pointer helpers are `motion_blur_analysis/ppc_tools.py:8-25`.
- Recorded baseline: 5,773,024 bytes, SHA-256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af` (`analysis/motion_blur/evidence/binary_baseline.json:2-7`; `motion_blur_analysis/inventory.json:2-4`). **Not independently recomputed here.**
- Startup establishes **r2=0x805FA780, r13=0x805EC500** at `0x8000332C-0x80003338` (FD:140-143). This validates the small-data addresses listed below.
- In the main text section, `file_offset = VA - 0x80006740`; for example draw `0x8042E254 -> 0x00427B14`. The bounded-read line index used here is `1 + (VA - 0x80006840)/4`, checked against actual address columns at each read, not used as a substitute for checking them.
- A late cross-check found the parent's `address_manifest.json:276-283` and `evidence/original/8042E254_MotionBlur_Draw.asm:1-15`; their start, size, raw initial words, and DOL offset agree with the independently read body. This is corroboration, not my own fresh hash/decode.

## 2. Narrowly anchored blur call chain

| Local address | Observation | Evidence / confidence |
|---|---|---|
| `0x801D2058-0x801D20D8` | Constructor stores task-name address `0x804D0F18` and table `0x8053DC78`. The local string is `DrawBlur`. | FD:470535-470566; `relevant_strings.tsv:135`; high for constructor/name |
| `0x801D27FC-0x801D2820` | Calls singleton getter `0x800A35D8`, then `0x8042F16C`. | FD:471024-471032; high for outgoing calls; task-vtable edge still needs raw table |
| `0x8042F16C-0x8042F198` | Loads manager+0x40, null-checks, calls `0x8042E254`. | FD:1090124-1090134; high |
| `0x8042E254-0x8042E838` | Complete blur draw body: gates, current raster dimensions/offsets, UVs, expansion, state setup, four-vertex submission. | FD:1089158-1089534; high |
| `0x8042E838-0x8042E9C4` | Save path; clears valid byte, two copy calls and two optional screen-quad submissions, then sets valid. | FD:1089535-1089633; high |
| `0x8042EA50-0x8042F084` | Constructor builds three four-vertex arrays and the retained raster/subraster arrangement. | FD:1089669-1090065; high |

The reference map's DrawBlur constructor/callback addresses are `0x801D1CB8/0x801D245C` (`reference_GUPE8P.map:15950,15973`), whereas these local bodies are +0x3A0. Backend map shifts differ. No universal relocation delta was assumed.

## 3. Exact blur coordinate and UV data flow

### 3.1 Inputs and notation

At draw `0x8042E3AC`, `r29 = currentCamera->raster` from camera+0x60. Define:

- `R = *(currentCamera+0x60)`; `P = *(R+0x00)`.
- `ox = s16(R+0x1C)`, `oy = s16(R+0x1E)`.
- `W = s32(R+0x0C)`, `H = s32(R+0x10)`.
- `TW = u32(P+0x0C)`, `TH = u32(P+0x10)` as used by the unsigned conversion sequences in this body.
- `S = f32(state+0x04)`.
- `h = f32[0x8051C388]`; `k = f32[0x8051C38C]`.

The offset helper is verified, not inferred from its map name: `0x8048AB7C-0x8048AB90` directly loads signed halfwords +0x1C/+0x1E and stores them to its two output pointers (FD:1183952-1183956). It does **not** accumulate parent offsets or read any camera history.

**Address-sign warning:** `lis ...,0x8052; lfs ...,-0x3C78(...)` addresses **0x8051C388**, not 0x8052C388. Likewise backend tables loaded with `lis ...,0x8057; addi ...,-0xEC4` are at **0x8056F13C**, not 0x8057F13C.

### 3.2 UV formulas

At `0x8042E3C0-0x8042E4F0`, the observed operation graph is:

```text
u0 = f32((f32(ox)     + h) / f32(TW))
u1 = f32((f32(ox + W) + h) / f32(TW))
v0 = f32((f32(oy)     + h) / f32(TH))
v1 = f32((f32(oy + H) + h) / f32(TH))
```

`ox+W` and `oy+H` are integer adds before signed conversion. Numerator adds and divides are `fadds`/`fdivs`. Parent dimensions are converted using the no-`xoris` double-bias idiom; signed dimensions/offsets use `xoris ...,0x8000`. The exact loaded double biases still require raw verification (section 9).

Evidence: FD:1089244-1089325 (`0x8042E3AC-0x8042E4F0`). Stores into the four vertices occur at `0x8042E4F4-0x8042E528` (FD:1089326-1089339).

**Interpretation:** UVs are selected by the current raster's rectangle within its parent, not by camera angular/translational velocity. There is no previous-frame projection of a world point, homogeneous divide of such a point, depth reconstruction, or motion-vector lookup in this computation.

### 3.3 Rectangle expansion formula and operation order

`0x8042E598-0x8042E600` implements, retaining the single-precision operation order:

```text
d  = f32(S - k)
aH = f32(d * f32(H))
aW = f32(d * f32(W))
dy = f32(h * aH)
dx = f32(h * aW)
L  = -dx
T  = -dy
R  = f32(f32(W) + dx)
B  = f32(f32(H) + dy)
```

Evidence: load S at `0x8042E598`, subtract at `0x8042E5A4`, H/W products at `0x8042E5AC/E5B4`, h-products at `0x8042E5C0/E5C4`, negations at `0x8042E5D0/E5D4`, final coordinate stores at `0x8042E5E0-0x8042E600` (FD:1089367-1089393).

This is symmetric around **(W/2,H/2) in local screen coordinates** in real-number algebra, regardless of the unknown numerical value of h. Floating-point rounding can perturb the arithmetic midpoint; do not claim bit-exact midpoint equality after rounded stores.

Conditional conventional specialization, **not raw-bit verified by this role**:

```text
if h == 0.5f and k == 1.0f:
    dx = 0.5f * ((S - 1.0f) * W)
    dy = 0.5f * ((S - 1.0f) * H)
    rectangle = (-dx, -dy, W+dx, H+dy)
    width/height in real arithmetic = S*W, S*H
```

The effective source position along x, ignoring rounding and the backend's constant pixel-origin adjustment, is `ox + h + W*(x-L)/(R-L)`; y is analogous. In the conventional specialization this is a centered zoom sample, not a free directional displacement. The code does **not** add state+0x08/+0x0C to its center.

## 4. Vertex ordering, persistent layouts, and stack objects

### 4.1 Four primary vertices

Each record is **0x18 bytes**. The effect's memory order is **left-top, left-bottom, right-top, right-bottom**:

| i | State record base | x | y | z | RGBA | u | v |
|---|---|---|---|---|---|---|---|
| 0 | +0x14 | L at +0x14 | T at +0x18 | +0x1C | +0x20..23 | u0 at +0x24 | v0 at +0x28 |
| 1 | +0x2C | L at +0x2C | B at +0x30 | +0x34 | +0x38..3B | u0 at +0x3C | v1 at +0x40 |
| 2 | +0x44 | R at +0x44 | T at +0x48 | +0x4C | +0x50..53 | u1 at +0x54 | v0 at +0x58 |
| 3 | +0x5C | R at +0x5C | B at +0x60 | +0x64 | +0x68..6B | u1 at +0x6C | v1 at +0x70 |

- z is copied from `(*0x805F265C)+0x18` during construction at `0x8042EAF4/EB04`, `EB34/EB3C`, `EB58/EB5C`, `EB74/EB78` (FD:1089710-1089743). This fits RenderWare's screen near-Z device value; its numerical runtime value is not proved by the instruction alone.
- Draw writes RGB bytes to 0xFF and A to the current state+1 for every record, `0x8042E52C-0x8042E590` (FD:1089340-1089365).
- Submission is exactly **r3=4, r4=state+0x14, r5=4**, through driver slot +0x30 at `0x8042E738` (FD:1089463-1089471).
- The candidate backend `0x80491E80` streams these records sequentially to FIFO `0xCC008000`: xyz float32, four color bytes, then uv float32; it advances by 0x18 per record, with a two-record unroll (FD:1191350-1191410, `0x80491F14-0x80492004`). It does not project a velocity vector on the CPU.
- RenderWare primitive 4 conventionally means triangle strip; local backend obtains GX primitive from `u32[0x8056F120 + 4*4] = u32[0x8056F130]` at `0x80491EAC-0x80491EC0`. **That table word must be read to promote GX_TRIANGLESTRIP/0x98 from ABI-backed interpretation to exact raw proof.** Under that mapping, the two triangles are strip vertices (0,1,2) and (2,1,3); this is not perimeter-order QUADS.

### 4.2 State layout (allocated size 0x13C)

Allocation size and constructor call: `0x8042F224-0x8042F240` (FD:1090170-1090177).

| Offset | Size | Proven use |
|---|---:|---|
| +0x00 | 1 | enable gate |
| +0x01 | 1 | alpha gate and all four main vertex alpha bytes |
| +0x02 | 2 | no semantic use established |
| +0x04 | 4 | scalar expansion S |
| +0x08,+0x0C | 8 | copied parameter words, interpolated as floats upstream, **not read by draw/save** |
| +0x10 | 4 | owner/manager pointer |
| +0x14..+0x73 | 0x60 | main expanded quad |
| +0x74 | 4 | retained raster/subraster pointer; draw uses its +0 parent/root |
| +0x78 | 1 | saved-image validity gate |
| +0x79..+0x7B | 3 | no semantic use established |
| +0x7C..+0xDB | 0x60 | first save-path quad, half-sized position rectangle |
| +0xDC..+0x13B | 0x60 | second save-path quad, full-sized position rectangle |

There is no 3x4 or 4x4 matrix storage region in this recovered object layout. This is a **local layout result**, not proof that the entire game has no camera history elsewhere.

### 4.3 Constructor save-quads

The constructor obtains the then-current camera raster's parent dimensions `RW,RH` at `0x8042EB94-0x8042EBA8` (FD:1089750-1089755). It computes:

```text
U0 = h/RW; U1 = (RW+h)/RW
V0 = h/RH; V1 = (RH+h)/RH
```

It makes the +0xDC array cover `(z0,z0), (z0,RH), (RW,z0), (RW,RH)`, where `z0 = f32[0x8051C3A0]` is the loaded coordinate-zero candidate, with UV order `(U0,V0),(U0,V1),(U1,V0),(U1,V1)`. Constructor field stores: `0x8042EDAC-0x8042EE38` (FD:1089884-1089919). Initial z/color copies precede those coordinate overwrites (`0x8042EC24-0x8042EDA8`).

Then integer `srawi 1; addze` at `0x8042EE3C-0x8042EE58` computes **signed division by 2, truncating toward zero**. The +0x7C array is a record-for-record copy of +0xDC, retaining its full-range UVs and z/colors, with x/y overwritten to the half-sized position rectangle `(z0,z0), (z0,RH/2), (RW/2,z0), (RW/2,RH/2)` at `0x8042EF84-0x8042F008` (FD:1089920-1090035). These are constructor-time geometry copies, **not previous-camera matrices**.

Save submits +0x7C at `0x8042E954`, then +0xDC at `0x8042E99C`, both type=4/count=4 (FD:1089598-1089624). Neither save quad is rebuilt from player velocity or a camera delta in `0x8042E838-0x8042E9C4`.

### 4.4 Stack layouts

**Blur draw `0x8042E254`, frame size 0xB0:**

| SP-relative | Use |
|---|---|
| +0x08 / +0x0A | signed 16-bit y/x raster offsets |
| +0x0C,+0x10,+0x14,+0x18,+0x1C,+0x20,+0x24 | saved render states 9,14,8,6,12,11,10 respectively |
| +0x28,+0x2C | v0,v1 pair |
| +0x30,+0x34 | u0,u1 pair |
| +0x38..+0x97 | twelve aligned 8-byte integer-to-double conversion temporaries |
| +0xA4,+0xA8,+0xAC | saved r29,r30,r31 |
| +0xB4 | saved LR in caller linkage area |

**There is no stack matrix object in this blur draw.** The many `lfd/stfd` temporaries are conversion scratch, not a previous-view/projection matrix. The first UV-pair stores copy template words and then overwrite them with calculated floats; do not interpret template loads as historical coordinates.

**2D backend init `0x8049148C`, frame size 0x30:** +0x08..+0x27 contains integer-conversion scratch, +0x2C saved r31, +0x34 LR. Its actual projection vector and position matrix are globals (section 5), not this stack scratch.

## 5. Direct fullscreen backend matrices and UV generation

### 5.1 Backend linkage and attribute contract

The local implementation at `0x80491E80-0x8049245C` is consistent with `_rwDlIm2DRenderPrimitive` (reference-map hypothesis at `0x80490E30`, map:34076). Its code itself proves:

1. Init call `0x80491EA4 -> 0x8049148C` before GX begin/FIFO output.
2. Primitive lookup and count forwarding at `0x80491EAC-0x80491EC0`.
3. Sequential 0x18-byte xyz/RGBA/uv streaming described above.
4. Cleanup call `0x80492438 -> 0x80491774` before return (FD:1191679-1191687).

Device-handle getter `0x804960F0-0x804960FC` returns **0x8056F50C** (FD:1195565-1195567). The expected device primitive slot is **0x8056F52C** (device+0x20); the runtime engine's device starts at +0x10, explaining the effect's engine+0x30 call. **Read the table word/install edge before treating this linkage as fully closed.**

`0x8049148C-0x80491598` configures the expected direct vertex formats:

- Position attribute 9, direct, component selector 1 (XYZ), type 4 (F32), frac 0.
- Color attribute 11, direct, component selector 1 (RGBA), type 5 (RGBA8).
- Textured branch: attribute 13, direct, ST/F32; one texgen.
- Texgen call at **0x80491598 -> 0x80395D48** has `(r3,r4,r5,r6,r7,r8) = (0,1,4,0x3C,0,0x7D)`, conventionally `GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY, false, GX_PTIDENTITY`.

Evidence: FD:1190676-1190743. This is texture-coordinate passthrough/identity generation, not a camera-derived texture matrix. Discrete GX names use the expected SDK ABI; raw call arguments above are the authoritative evidence.

### 5.2 Screen projection override

`0x80491684-0x8049175C` (FD:1190802-1190856) does the following:

```text
mode = *(r13 + 0x61F4)
MW = u16(mode+4)
MH = u16(mode+8)
proj = (float*)0x8056F13C        // seven-word GX projection-vector format
pos  = (float*)0x8056F158        // 3x4 position matrix
save = (float*)0x805E4218        // seven-word current-projection backup

proj[1] = f32(f32[0x805FC400] / f32(MW))   // store at 0x804916D0
proj[3] = f32(f32[0x805FC404] / f32(MH))   // store at 0x804916E8
GXGetProjectionv(save)                     // 0x804916EC -> 0x8039A1E4
GXSetProjectionv(proj)                     // 0x804916F8 -> 0x8039A158

R = (**(r13+0x615C))->raster                // via current camera +0x60
pos[0][3] = f32(f32(s16(R+0x1C)) + f32[0x805FC408])
pos[1][3] = f32(f32(s16(R+0x1E)) + f32[0x805FC408])
GXLoadPosMtxImm(pos, 0)                    // 0x80491754 -> 0x8039A22C
GXSetCurrentMtx(0)                         // 0x8049175C -> 0x8039A2CC
```

The untouched entries in `proj` and `pos` are data-table literals. A conventional orthographic specialization would have coefficients `2/MW` and `-2/MH`, position basis identity, and constant pixel-origin adjustment. **This role cannot assert their exact numeric bits, projection-type marker, constant translations, or depth coefficients without reading 0x8056F13C..0x8056F187 and 0x805FC3F8..0x805FC41F.** In particular, do not copy a guessed -0.5 or +0.5 origin bias into source.

There is **no call to C_MTXOrtho, C_MTXPerspective, or a frustum helper** in this 2D preparation. Therefore there is no literal `(top,bottom,left,right,near,far)` call tuple to recover here. The exact API is a seven-float projection vector, built by the stores above. Any equivalent orthographic helper call must be derived from the verified static vector, not invented.

### 5.3 Viewport arguments

The preceding init code at `0x804915E0-0x80491680` uses the same render-mode MW/MH. Normal viewport call `0x80491680 -> 0x8039A49C` receives:

```text
f1=f2=f5=f32[0x805FC3F8]
f3=f32(MW)
f4=f32(MH)
f6=f32[0x805FC3FC]
```

The field-rendering branch calls `0x8039A444` at `0x80491638`, with the same float tuple and integer jitter selector `VIGetNextField() XOR 1` from the `0x80380004` result. This is a VI/render-mode origin/jitter decision, **not a motion-blur camera-velocity parameter**. Evidence: FD:1190761-1190801.

### 5.4 The apparent previous projection is same-call state preservation

The key sequence is fully ordered by direct calls:

```text
0x80491EA4   begin Im2D preparation
0x804916EC   write current GX projection to 0x805E4218
0x804916F8   install screen projection
0x80491754   install screen position matrix
0x80491EC0   GX begin, then quad vertices
0x80492438   cleanup
0x80491AF0   reload projection from 0x805E4218
```

Restore evidence: `0x80491AE8-0x80491B04`, FD:1191083-1191090. This directly proves the backup's use in this call is **save/restore around a draw**, not comparison against a prior frame.

The helper bodies corroborate the identities:

- `0x8039A1E4` reads current GX context field +0x4D8 (projection type), writes one float marker, then copies the current six values from +0x4DC to the supplied buffer (FD:937578-937595).
- `0x8039A158` reads the supplied marker/six floats and emits XF command header **0x00061020** plus projection values/type (FD:937543-937577).
- `0x8039A22C` emits a 12-float matrix upload headed by `0x000B0000 | (matrixIndex<<2)` (FD:937596-937615).

**Decoder caution:** Capstone's text in these helpers shows some Gekko paired-single loads as `.byte` and paired-single stores as modern VSX mnemonics such as `xsaddsp`. The words `E0430000`, `E0230008`, `E0030010` are `psq_l` pairs; corresponding `F0...` words are `psq_st`, not arithmetic combining current/previous matrices. Interpreting those disassembly labels as matrix math would be a false positive.

## 6. Current-camera preparation is not camera-motion estimation

The blur calls camera begin/end wrappers at `0x8048652C/0x80486504`. These directly dispatch camera+0x18/+0x1C respectively (FD:1179442-1179461), so the callback installation is an additional indirect edge for final verification.

The strongly matching local GameCube camera-begin backend is **0x804956F4-0x80495CAC** (map hypothesis `__rwDlCameraBeginUpdate`, reference map:34117). Its relevant calculations, read locally at FD:1194926-1195001, distinguish ordinary **current** camera state from motion history:

1. Stores the incoming camera pointer in r13+0x61F8 at `0x8049571C`.
2. Reads `oxView=f32(camera+0x78)`, `oyView=f32(camera+0x7C)` and constructs a **0x40-byte RenderWare matrix at SP+0x08**.
3. Gets `camera->frame` at +4, calls `0x804882E8`. That helper optionally updates dirty frame hierarchy, then returns **frame+0x50**, not an indexed previous transform (FD:1181355-1181370).
4. Calls `0x80480100(dst=0x805E428C, src=currentFrameLTM)` at `0x80495788`; the local orthonormal branch transposes the 3x3 and forms negative translation dot products (FD:1173084-1173122), corroborating matrix inversion rather than history lookup.
5. Calls `0x8048077C(dst=0x805E428C, src=SP+8, combine=2)` at `0x8049579C`. Combine-2 locally invokes the matrix multiply with `(out=SP+0x10, left=dst, right=src)` then copies back (`0x804808CC-0x804808F8`, FD:1173540-1173551). Both operands were produced/read in this same call.

### 6.1 Camera-begin stack matrix

Let `Z=f32[0x805FC428]` (r2+0x1CA8), `I=f32[0x805FC42C]` (r2+0x1CAC). Their likely 0.0/1.0 specialization still needs raw data. The actual stack stores at `0x80495730-0x8049576C` form:

| SP offsets | RwMatrix component | Stored vector |
|---|---|---|
| +0x08,+0x0C,+0x10 | right | (I,Z,Z) |
| +0x14 | flags | integer 0 |
| +0x18,+0x1C,+0x20 | up | (Z,I,Z) |
| +0x28,+0x2C,+0x30 | at | (-oxView,+oyView,I) |
| +0x38,+0x3C,+0x40 | pos | (+oxView,-oyView,Z) |

The padding words at +0x24,+0x34,+0x44 are not initialized by the listed matrix construction; they are not another basis or previous position. Whole function frame is 0x70; +0x48..+0x67 are later coordinate conversion scratch, +0x6C saved r31, +0x74 LR.

This matrix's translation/shear-like terms are from **view offsets**, not displacement between camera positions. Even this current-view matrix is not used to derive the blur UV/rectangle: Im2D subsequently uploads its own screen-position matrix.

### 6.2 Ordinary camera projection arguments, before Im2D overrides them

At `0x804957A0-0x80495820` the camera backend writes vector **0x8056F4F0** and calls `GXSetProjectionv`:

```text
p[1] = f32(camera+0x70)     // reciprocal view-window x candidate
p[3] = f32(camera+0x74)     // reciprocal view-window y candidate
n = f32(camera+0x80)
f = f32(camera+0x84)

if u32(camera+0x14) == 2:
    p[0] = I
    p[5] = f32(D / f32(f-n)), D=f32[0x805FC438]
    p[6] = f32(f * p[5])
else:
    p[0] = Z
    p[5] = f32((-n) / f32(f-n))
    p[6] = f32(f * p[5])
```

Untouched p[2]/p[4] are static data. Under expected `I=1,Z=0,D=-1`, these are ordinary orthographic/perspective branches of the camera projection. The actual field reads and operations are high-confidence; constant bits, static vector, and callback installation remain to verify. **No prior n/f, prior view window, camera speed, or previous matrix occurs in this slice.** Do not mistake a saved/restored ordinary scene projection for a velocity-projection input to blur.

## 7. Parameter origins relevant to camera/player/radial hypotheses

### 7.1 Only scalar S and alpha reach geometry/composite

The three copy helpers `0x8042E9C4`, `0x8042E9F0`, `0x8042EA1C` copy enable, alpha, S, and the two opaque words (FD:1089634-1089668). Complete draw/save bodies show no load from state+0x08/+0x0C. Therefore their possible design-time names do not alter this platform's recovered coordinate calculation.

### 7.2 Player-effect writes are explicit settings/fade, not a velocity vector in these slices

Direct callers of `0x8042F114` are `0x800A356C`, `0x800A375C`, `0x800A481C` (`branches.tsv:20745,20758,20890`):

- `0x800A3528-0x800A356C`: constructs **enabled=0, alpha=0x80, S=f32[0x805F3BD0]**. FD:160571-160588.
- `0x800A3714-0x800A375C`: constructs **enabled=1, alpha=0x80, S=the same constant**. FD:160694-160712.
- `0x800A47B0-0x800A482C`: scalar `t=f32(this+0x38)` determines enable by comparison with `f32[0x805F3BDC]`; computes alpha as the **low byte of truncation toward zero of `f32(f32[0x805F3BE0]*t)`**, and writes **the same constant S**. FD:161757-161788.
- The immediately associated update `0x800A4830-0x800A48C4` changes t by **incoming f1 / f32[0x805F3BD4]**, adding or subtracting according to byte this+0x34, and clamps to `f32[0x805F3BDC]` / `f32[0x805F3BCC]` (FD:161789-161826). This is a fade/tween-like scalar. Calling incoming f1 elapsed time is ABI/context-consistent, but its ultimate caller was not followed here; no player velocity vector is read in this dependency slice.

The fixed-setting stack parameter blocks do not initialize their last two copied words. That is a further reason **not** to implement those words as active center/direction inputs based on guesswork; the render body demonstrably ignores them.

### 7.3 Effect-resource interpolation is a second source, not previous-camera state

At `0x8043A900-0x8043A974` (FD:1101873-1101902):

- Copies current blur parameters via `0x8042F084` into effect+0x14C..0x158.
- Target enable goes at +0x15C, alpha at +0x15D.
- Target S comes from the resource pointer at effect+0x108, resource+0x0C, into +0x160.
- Resource+0x10/+0x14 become target opaque floats +0x164/+0x168.

At `0x80439EBC-0x80439FD8` (FD:1101216-1101287), a stack block at SP+0x14 starts with that target, then blends it with the saved parameter snapshot using `t=f32(effect+0x100)` and `K=f32[0x8051CB5C]`:

```text
q = f32(K - t)
Sout = fmadds(Starget, t, f32(Ssaved*q))
opaque0out = fmadds(opaque0target, t, f32(opaque0saved*q))
opaque1out = fmadds(opaque1target, t, f32(opaque1saved*q))
```

Alpha uses the analogous single-precision mix followed by `fctiwz` and byte store, only when endpoint bytes differ. Enable is ORed while the blend branch runs. The branch threshold is `f32[0x8051CB58]`, **not proven equal to K** until raw constants are read. Final setter is `0x80439FD8 -> 0x8042F0E8`. Teardown uses target block +0x15C at `0x8043A544` (FD:1101633-1101635).

The saved data here is a **16-byte effect-parameter snapshot for a transition**, not a camera matrix. The opaque pair is indeed interpolated as floats, but still does not feed this blur backend's center/direction.

Three additional setter pairs only disable enable while preserving the current other fields: `0x80204714-0x8020475C`, `0x802055DC-0x80205624`, `0x80206314-0x8020635C` (FD:522166-522184;523112-523130;523958-523976). No vector/matrix calculations occur between their getter and setter. Their unrelated surrounding gameplay was not traced further.

## 8. Previous-camera-state conclusions: precisely bounded

### Proven within the inspected bodies (high static confidence)

1. Draw coordinates/UVs depend on **current raster offsets/dimensions, parent dimensions, scalar S, and constants**. No camera-position difference or previous view/projection matrix is read in `0x8042E254-0x8042E838`.
2. Draw/save do not consume the state+0x08/+0x0C pair. There is no free radial-center or directional-vector term in these bodies.
3. The recovered 0x13C effect object contains parameters, raster/valid state, and three four-vertex arrays, not a previous-view/projection matrix.
4. The candidate Im2D projection backup at 0x805E4218 is overwritten from the **current GX projection immediately before the draw**, then restored after it. It is not used to compute velocity or compare matrices.
5. Current-camera preparation obtains the current frame LTM, applies inversion/current view offsets, and uses current clip/view-window parameters. Its stack matrix is same-call scratch.

### Absence of evidence, not proven global absence

- No previous-camera-state mechanism was found **feeding the isolated blur calculation**. This does **not** prove that the camera system, generic RenderWare layer, or every indirect writer has no historical state.
- Direct-branch references alone do not exhaust function-pointer callers. Raw task/device/camera callback edges and any mutable table rewrites remain for integration.
- Retained image history can contain scene/camera motion even though the blur effect never estimates camera motion. A static-screen zoom blend can visually look radial or speed-related without velocity-driven math.
- The texture's prior-frame versus same-frame age, first-valid contents, per-camera multiplicity, and whether a stationary camera still shows history depend on scheduler/capture evidence and runtime traces, not matrix absence. No runtime trace was captured here.

## 9. Exact-float/indirect-edge verification handoff

**Unresolved because raw-data execution was denied.** These are exact local data addresses recovered from the instructions, with the value/status deliberately separated. Parent/constant specialist should read big-endian bytes with imported `analyze_dol.data/fileoff`; do not run the helper's main or `ppc_tools.py all`.

| Address/range | Needed interpretation | Evidence/status |
|---|---|---|
| `0x8051C388` | h, UV bias and half-expansion factor | Loaded at 0x8042E430; likely **0x3F000000 / 0.5f**, **not raw-verified here** |
| `0x8051C38C` | k, scale baseline/default | 0x8042E50C,0x8042EA74; likely **0x3F800000 / 1.0f**, **not raw-verified here** |
| `0x8051C390..397` | signed conversion double bias | Expected standard **0x4330000080000000**, not read as data here |
| `0x8051C398..39F` | unsigned conversion double bias | Expected standard **0x4330000000000000**, not read as data here |
| `0x8051C3A0` | save-quad coordinate-zero literal | 0x8042ED64; likely **0x00000000 / +0.0f**, not raw-verified |
| `0x8051C358..387` | constructor UV/rectangle templates | Many words are subsequently overwritten; inspect before naming persistent defaults |
| `0x8056F120..13B` | GX primitive translation table | Index 4 reads **0x8056F130**, expected 0x98 |
| `0x8056F13C..157` | complete seven-float Im2D projection vector | Need exact type/depth/translation literals, not just dynamic coefficients |
| `0x8056F158..187` | complete 3x4 Im2D position-matrix template | Only x/y translations are rewritten in init |
| `0x805FC3F8..41F` | Im2D viewport/coefficient/bias floats and conversion doubles | r2+0x1C78,+1C7C,+1C80,+1C84,+1C88,+1C90,+1C98 |
| `0x805FB838..83F` | GX projection-vector type-marker floats | r2+0x10B8/+0x10BC in 0x8039A158/0x8039A1E4 |
| `0x805FC428..447` | camera-begin Z,I,D and conversion constants | r2+0x1CA8,+1CAC,+1CB8,+1CC0 |
| `0x8056F4F0..50B` | ordinary camera projection-vector template | Dynamic entries established in section 6.2 |
| `0x8056F50C..53B` | local RwDevice table | Returned by 0x804960F0; expected Im2D primitive pointer at +0x20 = **0x8056F52C** |
| `0x805F3BCC..3BE3` | player fade/scale constants | r2-0x6BB4,-6BB0,-6BAC,-6BA4,-6BA0; do not replace S by guessed 1.0 or a velocity formula |
| `0x8051CB58..77` | resource blend threshold/unit/conversion constants | Threshold and K are different loaded addresses |
| `0x8053DC78..87` | DrawBlur task-vtable prefix | Expected callback edge still needs raw pointer proof |

The exact instruction-derived values **already established without these dumps** include record stride 0x18, four vertices, expanded-quad/UV order, alpha byte 0x80 for the two fixed setters, RGB bytes 0xFF, zero/one enable values, texture-generation integer tuple `(0,1,4,0x3C,0,0x7D)`, position matrix index 0, XF upload command 0x00061020, current r2/r13 bases, and matrix/stack addresses above.

Float-bit pitfalls: signed `fneg` can generate negative zero; retain operation order. The expansion uses separate rounded multiplies, not one reassociated expression. The resource blend uses `fmadds`, not two independently rounded products plus addition. Alpha ends in truncation and byte storage, not an invented saturating float-to-u8 conversion.

## 10. Actions, changes, and remaining responsibility

### Commands actually executed

- `pwd` in the historical workspace: succeeded in `<historical-game-root>/sys`.
- `ls "analysis/motion_blur/evidence"`: succeeded before report creation; verified the existing intended parent directory.
- Attempted `python -B motion_blur_analysis/ppc_tools.py disasm 0x801d23e8 0x801d2780`: **denied before execution**.
- Attempted `python -B motion_blur_analysis/ppc_tools.py pointers 0x804d1058`: **denied before execution**.

No Python import/main, full-disassembly generation, build, emulator, DOL patch, hash recomputation, or runtime validation was performed by this role. Reads/searches were restricted to instructions/data descriptions needed for this effect and its immediate dependencies. No `grep` of the oversized full-disassembly artifact was used.

### Files changed

- **Only created `analysis/motion_blur/evidence/07_matrix_camera.md`**, this report.
- No original DOL, prior analysis, helper, manifest, or another investigator's report was edited.

### Parent should handle

1. Raw constant and function-pointer checks from section 9, followed by exact numeric orthographic/vector formulas. This report is **not float-bit-complete** until that evidence is integrated.
2. Fresh final original-DOL SHA-256 and instruction/raw-data address checks. Existing manifests corroborate this role's addresses but do not replace the final integrity check.
3. Cross-check device/camera callback linkage, draw/save scheduler order, raster age, and multiple-camera behavior with the other scoped reports. Keep image history separate from camera-matrix history in the final source and explanation.

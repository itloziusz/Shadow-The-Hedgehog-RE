# Motion blur — storage, globals and lifetimes

## Address and ownership model

All addresses refer to the original 32-bit big-endian PowerPC image, SHA-256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`. A pointer slot, the object it points to, a raster view, an owning raster root, a plugin extension, and GPU pixels are **different storage objects**. The [atlas](motion_blur_original_addresses.md) gives all **80 data records**, exact extents and file offsets; this document explains their use and the runtime allocations that cannot be DOL records.

Evidence: [09 dataflow](evidence/09_dataflow.md), [05 lifecycle closure](evidence/05_closure.md), [04 bottom-up closure](evidence/04_closure.md), [12 input closure](evidence/12_closure.md), [raw data](evidence/data_manifest.json). Earlier reports' unread provider/standard-table/format requests are superseded by the current raw records and closures. Static layouts do not establish live pointer values.

```text
[805EF2D8] -> effect manager M, normally static 8057798C
  M+34 [805779C0] -> scratch view -> Sroot -> extension -> scratch pixels
  M+40 [805779CC] -> heap blur B (0x13C bytes)
    B+10 -> M, borrowed
    B+74 -> history view -> Hroot -> extension -> retained pixels

[805E25C8] -> installed provider, vptr 8053E93C
  vslot+24 -> 80028730 -> 8000DCC0
    [805EF068] -> camera manager 8056FF1C
      manager+0 -> selected camera C
        C+60 -> current view V -> root P
```

M+38 is a separate scratch peer created by the shared manager routine; **the blur reads M+34 only**. It is neither H's second buffer nor a ping-pong selector. The image named H below is the **single allocation owned by the root of B+74**.

## 1. Complete blur allocation layout

`8042F20C` requests **0x13C bytes** through `803A1380`; `8042EA50` constructs in that storage. Parameters occupy 0x10 bytes. Offsets are from B; ranges include their last byte in this table.

| Offset/span | Type/size | Writes | Reads / meaning |
|---|---|---|---|
| +00 | u8 enable | Constructor, parameter helpers | Separate nonzero Draw/Save gate; raw copies do not normalize it. |
| +01 | u8 alpha | Constructor, helpers | Nonzero gate and four Draw vertex alpha bytes. |
| +02..03 | 2 padding bytes | No store by these helpers/constructor | Not transferred; no invented zero value. |
| +04 | f32 scale | Constructor, helpers | Current Draw's centered XY expansion. No clamp in the core. |
| +08 / +0C | two 4-byte words | Helpers; resource producer uses LFS/STFS/interpolation | Copied out/in; **no Draw/Save GPU consumer**. Meaning unresolved, not active center/direction. |
| +10 | pointer32 M | Constructor | Save follows M+34; borrowed, not separately freed. |
| +14..73 | 4 x 0x18 composite vertices | Constructor supplies Z and white/opaque RGBA; eligible Draw overwrites XY/UV/RGBA | One strip per successful Draw. XY/UV need not have initialized values immediately after construction. |
| +74 | pointer32 history view | Constructor on camera-success path, destructor | Save destination is view; Draw binds its root. Owns view/root lifetime, not a second image. |
| +78 | u8 valid | Constructor on camera-success path: 0; reached Save: 0 then qualifying tail: 1 | Draw tests it but never consumes/clears it. A software latch, not GPU success/fence. |
| +79..7B | 3 padding bytes | No established store | Not another flag or camera ID. |
| +7C..DB | 4 x 0x18 half-size vertices | Construction copies full block, rewrites XY to signed-truncated half extents | First optional Save draw. UVs/Z/colors remain construction-time values. |
| +DC..13B | 4 x 0x18 full-size vertices | Construction copies initial draw records, then overwrites XY/UV | Second optional Save draw; samples scratch with full-root-size geometry. |

Vertex layout: **x/y/z F32 at +0/+4/+8, RGBA bytes at +C..F, u/v F32 at +10/+14**, size 0x18. Vertex order is **TL, BL, TR, BR**. Draw's array begins at +14; its alpha bytes are B+23/+3B/+53/+6B. This object has no previous-camera matrices, motion vectors, sample-count loop, frame ID, history age, or per-camera key.

The three parameter helpers `8042E9C4/E9F0/EA1C` copy only **0/1/4/8/C** in their respective instruction order, never padding or B+78. Getter `8042F084` uses output in r3/manager in r4. Setters use manager in r3/source in r4; `8042F114` additionally permits null source. Manager itself is not null-guarded.

### Construction and indeterminate storage

Before camera lookup, the constructor stores **enable=0, alpha=0, scale=1, owner=M only** (`8042EA6C..EA80`). An absent provider/cast/camera returns the nonnull allocation before history/valid/vertices are initialized; Create nevertheless installs that allocation at M+40. This is not a transactional failure return.

After camera success it initializes Draw Z/RGBA, writes B+74=0 and B+78=0 at `8042EB8C/EB90`, builds fixed capture geometry, creates and attaches rasters, then copies supplied parameters or establishes **enable=0, alpha=128, scale=1**. Default +8/+C and padding remain unwritten. History allocation/subraster results are not all checked. There is no whole-B memset or pixel seed.

Original first-full-vertex transfers interleave with height arithmetic at **8042EC24..8042ECB4**. The integrated source retains that order, including divide **8042EC78** before blue/alpha stores **8042EC84/EC94**; the resolved audit's **24/24 optimized-IR event check** is documented in [source audit resolution](evidence/source_audit_resolution.md). This is not proof of original stack/register/exception ABI.

Absent-B getter fallback writes `(enable=0,alpha=0,scale=1)` but moves unwritten original stack words into output+8/+C. Fixed/ramp producers also leave their auxiliary words unwritten. RW12's failed getter leaves its saved stack word untouched. Such values are **machine indeterminates**, not zero defaults or recovered caller values.

## 2. Rasters, plugins and pixel storage

Only observed prefixes are described; prefix size does not imply a complete RenderWare allocation size.

| Raster offset | Type / role |
|---|---|
| +00 | pointer32 root/self; attachment flattens to parent.root. Copy follows exactly one link. |
| +04 / +08 | CPU pixel/palette pointers in metadata; these are not the type-5 GPU backing pointer. |
| +0C / +10 / +14 | 32-bit width/height/depth words; signed or unsigned interpretation depends on consumer. |
| +18 | stride word copied by subraster backend. |
| +1C / +1E | signed 16-bit source/view origins. |
| +20 / +21 / +23 | type / flags / format bytes. Flags bit 0x80 marks no-data/empty view. |
| `+[805F2700]` | Registered **0x34-byte raster plugin** (ID 0x40C); dynamic byte offset, not a pointer. |

Let **X = root + u32[805F2700]**:

| X offset | Type / meaning on the nonempty blur root path |
|---|---|
| +0C | u32 GX texture format **6 = RGBA8** |
| +10 | u32 format auxiliary; initialized 0xFF here, not a blur control |
| +14 | u32 alpha-presence flags, bit0=1; does not mean every sampled alpha is opaque |
| +18 | pointer32 **owned allocator base A**, used by destruction |
| +1C | pointer32 **aligned pixels D**, `(A+31)&~31` |
| +20 | auxiliary image pointer, initialized zero on this path |
| +24 / +28 | Shared extension fields without a required blur interpretation |
| +2C | texture-cache region pointer; null selects global invalidation/callback-based loading |
| +30 | u16 last-use token, initialized from 805F2720 then stamped from 805EEFF0 during realization |
| +32 | max LOD byte: 0 for the allocated no-mip root; empty view's own value starts 0xFF |
| +33 | byte initialized 0xFF; not a validity counter |

The nonempty creation flags are **0x505**: type5/format0x500, no palette, GX6, depth32, alpha bit1, format byte05 with no mipmap bit. Scratch's **empty view** is created with flags5/depth0 and temporarily follows the default GX4/depth16 path; attachment copies parent stride/depth/type/format, so its use is not an independent RGB565 image. History's empty view uses 0x505. Neither empty view owns pixels.

| Buffer | Dimension source at construction | Captured/sampled identity | Lifetime |
|---|---|---|---|
| Scratch S | `80433638` reads **C.raster view** width/height, not view.root | Save copy1/binding use **Sroot** through M+34 | Shared manager allocation; created before B, destroyed after B; reused by neighboring effect work |
| Retained H | `8042EA50` reads **C.raster.root** W/H, halves each signed word toward zero (`srawi;addze`) | Save copy2 uses **Hview**; Draw binds **Hroot** | Private blur image; same root allocation overwritten on each qualifying Save |
| Current EFB | Hardware/runtime state | Current rendering target and temporary half-size downsample surface | Not an owned B allocation; not XFB history |

For positive nonempty W/H with no original 32-bit overflow:

```text
payload P = align32(4 * align4(W) * align4(H))
          = 64 * ceil(W/4) * ceil(H/4)
request   = P + 31 bytes, allocation hint 0x00030411
owner A   = engine allocator at +108(request,hint)
pixels D  = (A + 31) & ~31
```

`80496478` depth32 case `80496548` and GX format6 tile shifts `(2,2)` independently establish **4x4 tiles of 64 bytes**. These formulas exclude raster metadata. Actual dimensions/addresses are not captured. All native sums/products wrap at 32 bits; no host size_t overflow protection is implied. `80372508` performs **DCInvalidateRange (`dcbi`)**, not memset, dcbz or image initialization. Initial/padded texels are unspecified.

`8048AE14` requires the no-data view flag, copies rectangle dimensions, adds signed parent origins with halfword truncation, invokes **engine+78 -> 804981B8**, then installs parent.root. The backend callback copies metadata and clears view CPU pixels; it allocates nothing and does not copy an independent pixel owner.

## 3. Texture descriptor and backend cache

**T=ptr32[805F270C]** is a shared texture descriptor, not a pixel buffer. T+0 is the bound raster; T+50 packs filter in low8, U wrap in bits8..11 and V wrap in bits12..15. **T+u32[805F2718]** contains a 0x20-byte GX texture object and cached sampler/flags at +20: dirty `0x01000000`, region `0x02000000`.

`8049819C` writes T+0 and **assigns** dirty flags `0x01000000` (not OR). Texture flush/realization follows T+0 -> raster.root -> X+1C. That is the **same allocation** targeted by EFB copies; there is no CPU pixel upload or swap. Dirty realization initializes the GX descriptor; sampler-only changes preserve live LOD-bias/clamp/edge/aniso values via SDK getters. `8056FCDC` remembers the last loaded map0 descriptor, not the previous image.

Render-state cache **805E42D0**, observed extent 0x50:

| Cache field | Role |
|---|---|
| +00/+04/+08 | Z write / Z test / effective GX compare (3 LEQUAL or 7 ALWAYS) |
| +0C | Cull state, not reset by blur |
| +10/+14 | Fog enable/type |
| +18/+1C/+20 | Fog packed color-related word / RGBA bytes / end override |
| +24/+28/+2C/+30 | Fog start/end/near/far |
| +34/+38 | RW source/destination blend values |
| +3C | GX Z-compare-before-texture cache |
| +40/+44/+48 | Alpha compares and combine operation |
| +4C/+4D | Alpha reference bytes |

This cache is distinct from T. State ID12 has no implemented get/set storage. [GX state](motion_blur_gx_state.md) details the six implemented restore requests and inherited tests/masks/samplers.

## 4. Shared live globals required by this path

**Z** means startup-zero-backed, not live-zero; **I** initialized DOL bytes, mutable where stated; **R** runtime storage identified by instruction use. All 80 exported data extents, including constants/tables, are in the atlas; rows here also describe necessary non-exported live storage without pretending to add manifest records.

| Address | Storage class | Required interpretation |
|---|---|---|
| 805EF2D8 | Z pointer | Lazy effect manager cache; getter 800A35D8 |
| 8057798C; 805779C0/805779CC | Z manager / fields | Normal M; scratch+34 / B+40 |
| 805E25C8 | Z pointer | Camera-provider service, getter/setter 8042B620/B630 |
| 805EF068; 8056FF1C | Z pointers | Camera manager cache / selected camera at its field0 |
| 805F265C | Z pointer | RenderWare engine; current camera+0, device+10, standards+48, pixel allocator+108/free+10C |
| 805F26F8 | Z pointer | Backend current camera; begin sets, end clears |
| 805F25B0 | Z u32 offset | World-camera plugin extension; saved callback slots+10/+14 |
| 805F2700 / 805F2718 | Z u32 offsets | Raster / texture plugin offsets, not pointers or image widths |
| 805F270C | Z pointer | Shared current texture descriptor T |
| 805F2688 | Z u32 | **Live low byte** controls GXCopyTex clear independently of selector0; alias-complete writer exclusion is not proved |
| 805F26F4 | Z pointer | Current render-mode descriptor; default or mutable custom 805E4250 |
| 805F2698 / 805EEFD8 / 805F26F0 | R words | Conditional viewport/scissor mode, split-half selector and split-height source |
| 8056F13C / 8056F158 | I mutable arrays | Im2D projection 0x1C bytes / position matrix 0x30 bytes |
| 805E4218 | Z vector, 0x1C observed | **Same-call** GX projection snapshot, not temporal state |
| 8056F4F0 / 805E428C | I template / R matrix | Current camera projection / camera-derived matrix, not previous-camera storage |
| 805FB758 | I pointer, initial 805A2F60 | SDK GX context; cache contents at execution remain live |
| 805EEFF0 / 805F2720 | I u16 initially1 / Z u16 | Current use token / cached create-completion token; not blur frame counters |
| 8056FCDC | I pointer initially0 | Last map0 descriptor loaded |
| 80571C6C / 80574384 | Z singleton bodies | Task manager K / render-level manager R, separate from M |
| 8057E7C0..8057E7D3 | Z 5 words | Div mode, two camera inputs, selector, view index |
| 805EF270 / 805EF274 / 805EF27C | Z u32/u32/u8 | Traversal depth / profiling / pause |
| 805F0A54 / 805F0A58 | Z u8/f32 | Frame-driver initialized flag / supplied traversal delta |
| 8058E76C | R u32 | Wrapping countdown decrement in disable container B, not blur strength |
| CC008000 | MMIO | GX FIFO destination, not ordinary RAM or a DOL-backed object |

Render-mode observed fields: +0 mode word, +4 u16 framebuffer width, +6 u16 EFB height, **+8 u16 height actually used by Im2D**, +18 field flag, +19 AA, +1A twelve sample pairs, +32 seven vertical-filter taps. Known default descriptors include 640x480/640x528; they do not establish the active view/root sizes or a universal allocation size.

Startup **r2=805FA780, r13=805EC500** is established at 8000332C..8000333C. The DOL BSS envelope overlaps initialized SDA sections; the actual zero table at 800055C8, not the envelope alone, determines zero backing. Initial zeros are not captured runtime values.

## 5. Input/controller storage

| Owner / field | Type and lifetime / use |
|---|---|
| Ramp task allocation 0x40; +18 vptr | `80522A78`, Exec word at +0C is `800A4830` |
| Ramp +28 | Actor reference word, copied on construction; not a motion-estimation input to blur math |
| Ramp +34 / +38 | u8 decreasing / f32 strength, both start 0; direction update is separate from B's enable |
| Ramp +4 bit0 | Deferred deletion request; set after post-callback strength reload is ordered <=0 |
| Controller E+0C / +18 | f32 age / u32 execution flags: active1, advance2, inhibit4; base starts age0/active1 only |
| E+1C / +44 | Borrowed manager cast source / source descriptor |
| E+FC | Source flags copied from descriptor+2C; **bit16** supplies target enable; not execution flags |
| E+100 / +104 / +108 | f32 weight / duration from descriptor+30 / borrowed resource pointer from descriptor+34 |
| E+14C / +15C | Starting / target 0x10-byte prefixes; finalizer writes target, not start |
| Resource +0/+8/+C/+10/+14 | s32 kind1 / alpha byte / scale / two opaque float words; +4 is not target enable |

Outer factory descriptor type **8** allocates a **0x1F4-byte controller** and calls `8043A5A4`; the observed blur prefix ends at 0x16C, not the allocation end. Resource lifetime, timer-flag writers, concrete delta units and competing-writer order are caller contracts. The exact immediate predicates and arithmetic are in [dataflow](motion_blur_dataflow.md).

## 6. Destruction and synchronization

History teardown `8042F198` reads B+74, destroys a distinct parent root first, reloads the view pointer, destroys the view, clears B+74, frees B with `803A1334`, then clears M+40. Shared `804335C8` later releases scratch roots/views and clears M+34/+38. Draw's task destructor merely schedules Save's deletion; task removal and buffer destruction are separate mechanisms.

Owning-root backend `80498058` checks root/self and no-data flag. Type5 roots use the u16 last-use token: if it equals the current token, a fence is emitted (`80396428`) and current token advances modulo **0xE000**. Completion is polled through **8049B7DC**; matching T+0 is unbound at **80498138**, and **X+18 allocator base**, not X+1C aligned pixels, is freed. Views do not own duplicate pixels. CopyEFB itself has pixel-mode synchronization and texture-cache invalidation but **no CPU token-poll wait**.

Normal ownership/order is proven; partial construction, allocator failure, arbitrary external aliases, service replacement, re-entrancy and real GPU completion are not thereby guaranteed safe. No pixel clear, automatic camera-cut reset, resize repair, reference counting or hidden fallback is inferred.

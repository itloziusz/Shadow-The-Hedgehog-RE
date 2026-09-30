# Role 05 closure: EFB/XFB, raster pixels, ownership

## Evidence contract

- Only this report is owned/written. `AGENTS.md`, originals, prior reports, and `GPT_SOL_analysis/` were read-only; no DOL patch or runtime experiment.
- `O` = `analysis/motion_blur/evidence/original/`; `DM` = `analysis/motion_blur/evidence/data_manifest.json`; `FD` = `motion_blur_analysis/full_disassembly.txt`. All address ranges below are local VA, start inclusive/end exclusive.
- Read-only `python -B analysis/motion_blur/tools/dol_evidence.py verify` **PASS**: baseline SHA-256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`, **81 function ranges / 7,249 instruction words / 3,626 file-backed data bytes**; DM contains 55 records.
- Additional FD windows used `line = 1 + (VA-0x80006840)/4`; the displayed VAs/words were compared with fresh hash-checked bounded `dol_evidence.py disasm` output. Supplementary data reads and `sda`/`xrefs` did not export or alter artifacts. Map names were hypotheses only.

## 1. The formerly missing standard-table edge is closed

- Local engine-open body `[80487254,804874FC)` obtains device **8056F50C** through `804872B0 -> 804960F0`, retaining it in r30. Device+4 is **80494EDC**, not a guessed relocation (`FD:1180317-1180322`; `DM:487-512`).
- `80487304-80487324` calls device system op 4 with `(engine+0x10,engine+0x108,0)`; `804950A0-80495124` copies the device descriptor there (`FD:1180338-1180345,1194521-1194553`). This supplies the later render-state/draw slots as well.
- **`804873A0-804873C0` calls the same device+4 with `(r3=11,r4=engine+0x48,r5=0,r6=29)`** (`FD:1180377-1180384`). Dispatch `[80494EDC,80494F18)` indexes **8056F494**; entry 11 at **8056F4C0 = 804954F8** (`FD:1194408-1194422`; `DM:1123-1155`).
- Installer arm `[804954F8,804955FC)` copies 27 records from **8051D400**, fills defaults with **80494CCC**, then installs record targets at `base+4*index` (`FD:1194799-1194863`). Its repeated range check uses the first copied index (1); supplied count 29 admits all actual indices 1..28. Do not silently rewrite this check as a per-record check.

| Record address: `(index,target)` | Installed engine slot | Directly relevant consumer |
|---|---|---|
| `8051D400: (1,804956F4)` | `+4C` camera begin | default begin adapter |
| `8051D408: (10,80495CAC)` | `+70` camera end | default end adapter |
| `8051D448: (4,80497CA8)` | `+58` raster create | `8048AF24/8048AF58` |
| `8051D440: (5,80498058)` | `+5C` raster destroy | `8048AC60/8048AC68` |
| `8051D4B0: (12,804981B8)` | `+78` subraster attach | `8048AE84/8048AE8C` |

Raw pairs: `DM:1057-1120`. This closes the **installed local backend**, not a guarantee against subsequent indirect pointer mutation.

## 2. Views, roots, and the two different dimension sources

- Let `M` be manager, `B=ptr32[M+0x40]`, `C` current camera, `V=ptr32[C+0x60]`. `R=*raster` is the root; `E=R+u32[805F2700]` is its backend extension. Startup r13=805EC500 makes SDA+6200 = **805F2700** (`DM:578-593,654-661`). Registration `[80496440,80496478)` requests extension size **0x34**, ID **0x40C**, stores the returned offset (`O/80496440_RwRaster_PluginRegister.asm:5-18`).
- `RwRasterCreate [8048AEC4,8048AFC0)` allocates a raster object through engine+118, initializes root=self, dimensions/depth, zero offsets and CPU pixel/palette pointers, then calls engine+58 `(0,raster,flags)`; plugin constructors follow at `8048AF98 -> 804830EC` (`O/8048AEC4_RwRaster_Create.asm:10-59`). This metadata allocation is separate from the pixel allocation below.
- `RwRasterSubRaster [8048AE14,8048AEC4)` requires view flags+21 bit **0x80**; copies rectangle dimensions, adds parent's signed offsets with halfword truncation, calls +78, then sets `view.root=parent.root` on success (`O/8048AE14_RwRaster_SubRaster.asm:12-41`). Backend `[804981B8,804981E8)` copies stride/depth/type/format and clears view CPU pixels+4; it does **not** allocate pixels (`O/804981B8_RwRaster_BackendSubRaster.asm:5-16`).
- Scratch `[80433638,80433754)`: reads **V+0C/+10**, without dereferencing V.root, at **8043368C-804336A4**. Creates M+34's zero-size flags-5 view (`804336D8`), a `(V.width,V.height,0,0x505)` root (`804336F0`), then attaches `(0,0,V.width,V.height)` (`80433700`). Template **8051C5E8..8051C5F8** freshly reads four zero words (`O/80433638_EffectManager_CreateScratch.asm:26-55`).
- The same routine creates M+38's peer; **SaveScreen reads only M+34**, not a toggled +34/+38 pair (`CreateScratch.asm:56-70`; `O/8042E838_MotionBlur_SaveScreen.asm:34-39`).
- History constructor `[8042EA50,8042F084)` instead reads **V.root+0C/+10** at **8042EB94-8042EBA8**. `8042EE44/48` and `8042EE4C/54` use `srawi;addze`: signed division by two, truncating toward zero (`O/8042EA50_MotionBlur_Construct.asm:86-118,256-263`).
- It creates `(0,0,0,0x505)` view at B+74 (`8042F00C/10`), a `(rootW/2,rootH/2,0,0x505)` root (`8042F024`), then attaches the half-size rectangle (`8042F034`). Rectangle origin comes from zero template **8051C358/+4**, not camera offsets (`Construct.asm:45-55,103-116,354-382`; `DM:62-92`). For successfully allocated, unmodified nonempty roots, both copy offsets start at pixel-base offset zero.
- Thus scratch size is **current view size at scratch construction**; history size is **camera-root size/2 at blur construction**. They need not match a common “screen width,” nor remain compatible after a camera/mode change. No fixed 640x448 allocation is justified.

## 3. 0x505 format and exact pixel-allocation request

- Resolver `[80497458,80497A70)` decodes `0x505 & 7 = type 5`, allocation flag byte `0x505 & 0xF8 = 0`, format mask `0x500`. Type-5 palette rejection masks **0x6000**; those bits are absent. The `0x500` branch reaches **80497884-8049789C**, writing **E+0C=6, E+14=1, raster+14=32**; `80497A54/58` stores format byte **5** (`O/80497458_RwRaster_ResolveFormat.asm:8-31,43-44,134-150,259-278,388-390`).
- This is **32-bit GX_RGBA8 (GX format 6), alpha-presence flag bit 0 = 1**, not RGB565, GX format 0x505, or proof that the EFB itself has eight stored alpha bits.
- Backend create `[80497CA8,80498058)` calls that resolver at **80497D18**. For nonempty allocating roots, format byte 5 makes mipmap test `((byte<<8)&0x8000)` false: exactly **one level**, E+32 max-LOD byte **0**, no palette allocation (`O/80497CA8_RwRaster_BackendCreate.asm:33,57-78,79-122`). Zero width **or** zero height instead sets raster+21=**0x80**, keeps its own E+32=**0xFF**, and performs no pixel allocation; later view sampling follows the root extension (lines 31-32,38-43,228-231).
- The level-sum loop invokes **80496478(raster,0)** once, at **80497E58**. Size helper `[80496478,804965A8)` uses depth-minus-four table **8056F5B8**; freshly read entry **8056F628 = 80496548** selects depth 32. It rounds W/H each to a multiple of 4, multiplies by 4 bytes, then rounds the result to 32 (`FD:1195791-1195866`, especially 1195843-1195849,1195861-1195862).

```text
For a positive nonempty unmodified 0x505 root, absent 32-bit overflow:
P = align32(4 * align4(W) * align4(H)) = 64 * ceil(W/4) * ceil(H/4)
allocation request = P + 31 bytes; hint = 0x00030411
A = engine.malloc(+108)(P+31, hint); E+18 = A
D = (A+31) & ~31; E+1C = D                 // 32-byte-aligned pixel base
```

- Exact allocation/alignment/call sites: **80497F30-80497FB4**, especially indirect malloc **80497F48**, aligned store **80497F94**, and type-5 `80497FAC -> 80372508(D,P)` (`BackendCreate.asm:167-199`). The byte count excludes raster-object/freelist metadata; actual W/H and A are runtime values. Native instructions use 32-bit arithmetic, not overflow-checked host size_t.
- Independent GX layout corroboration: **805689E8 = 80397DA4** selects format-6 tile shifts **2,2**; `[80397D6C,80397E34)` returns **two 32-byte units per tile** for format 6 (`O/80397D6C_GX_GetTextureTileCounts.asm:5-21,30-54`; `DM:1158-1228`). **80568ADC = 80397FF0** selects the format-6 GX texture-object arm (`O/80397E34_GXInitTexObj.asm:95-120`; `DM:1231-1255`).
- **Pixel clear is not proved.** `[80372508,80372534)` is a `dcbi` loop over 32-byte cache lines, not memset/dcbz (`FD:896819-896829`). It invalidates CPU cache; it does not zero allocated pixels. Backend create zeros metadata, and GXInitTexObj's `8000540C(...,0,0x20)` clears the **descriptor**, not D (`GXInitTexObj.asm:9-19`). Allocator contents and untouched/padded texels remain unspecified.

## 4. Save is two EFB copies and two conditional draws, not ping-pong

`[8042E838,8042E9C4)` clears B+78 first, gates on enable/alpha/provider/camera, then follows this order (`O/8042E838_MotionBlur_SaveScreen.asm:8-98`):

| Site | Operation | Resource / qualification |
|---|---|---|
| `8042E8C0` | `80496208(scratchRoot,0)` | root obtained through B+10 -> M+34 -> root |
| `8042E954` | primitive 4, four vertices at B+7C | after BeginUpdate succeeds; bind scratchRoot at E8F0, request ONE/ZERO factors |
| `8042E968` | `80496208(historyView,0)` | B+74; still executed if first BeginUpdate failed |
| `8042E99C` | primitive 4, four vertices at B+DC | after second BeginUpdate succeeds; no rebind; intended scratch restore if first bind ran, otherwise inherited texture/blend |

- Begin/end adapters: `[8048652C,80486554)` / `[80486504,8048652C)`, calls at E8C8/E95C and E970/E9A4. **B+78=1 at E9AC is reached even when either begin fails**; it is not GPU-completion or successful-image evidence. On the successful draw path, half-size acquisition is produced by the B+7C draw; both copy selectors are **0**, not alternating buffers or GX mipmap downsampling.
- `CopyEFB [80496208,80496410)` always finds root/extension from its raster argument. For selector 0, **source** is `(u16(s16 view.x),u16(s16 view.y),u16 view.W,u16 view.H)`; **destination setup** uses `(u16 root.W,u16 root.H,E.format,u8 selector)` (`O/80496208_RwRaster_CopyEFB.asm:17-49`). Nonzero selector doubles the source inputs, but blur does not select it.
- Consequently copy 1 uses scratch-root coordinates/dimensions, **not the current camera view's offsets**. Copy 2 uses history-view rectangle/offsets but the history root's destination layout. For depth 32, jump entry **8056F5B4 = 80496358** proves this exact offset (CopyEFB.asm:50-59,89-113; `DM:178-218`):

```text
x = s16(view+1C); y = s16(view+1E)
byteOffset = 4 * (4*x + align4(root.width)*y)    // native 32-bit shifts/mullw/add
GXCopyTex(E.pixels + byteOffset, u8(u32[805F2688]))
```

- Do not replace this tiled-offset expression with `4*(x+width*y)`, unsigned x/y, or a generic “copy entire root” for all views. At the constructor's zero origin the offset is zero; a changed view can copy into an interior region while later binding the root base.
- Source helper `[80396C50,80396CCC)` caches BP **49/4A**, encoding 10-bit x/y and `(W-1,H-1)` fields. Destination helper `[80396D00,80396E30)` calls tile helper **80397D6C** at **80396DC8**, caches BP **4D** stride payload `(2*ceil(max(u16(rootW),1)/4)) & 0x3FF` in 32-byte units, and writes selector bit 0 to copy-control bit 9 (`O/80396C50_GXSetTexCopySrc.asm:5-35`; `O/80396D00_GXSetTexCopyDst.asm:45-74`). It is not arbitrary resizing to root W/H.
- Actual local `GXCopyTex [803975AC,80397738)` emits BP **49,4A,4D,4B,52**; BP4B payload is `(destination>>5)&0x1FFFFF`, and BP52 bit 14 is forced **0** for texture copy (`O/803975AC_GXCopyTex.asm:52-84`). This is EFB-to-texture memory, **not an XFB history transfer**.

## 5. Clear provenance, filters, cache invalidation, and the same sampled pixels

- **Clear is not selector.** **804963AC/804963B4** load/low-byte-mask **[r13+6188] = [805F2688]** into GXCopyTex r4. The helper's r4=0 controls source scaling/copy mipmap bit, not EFB clearing (`CopyEFB.asm:109-114`). This corrects `GPT_SOL_analysis/motion_blur_analysis.md:154` without modifying it.
- Own hash-checked `sda 0x6188` confirmed only loads at **80495648,80495DF0,80495EEC,80496024,804963AC**. Parent additionally reports no D-form writer or address-literal/pointer match. Startup zero-fill span **805EF020+375C** covers the slot (`DM:594-612`), but these finite scans do **not** exclude indirect/indexed/aliased writes or establish the live clear argument. No constant-clear=false claim.
- At **80496248**, `GXSetCopyFilter [80397228,80397430)` gets `(AA=0,NULL,vfilter=0,NULL)`. Actual fallback emits sample words **01666666/02666666/03666666/04666666**, taps **[0,0,21,22,21,0,0]**, BP words **53595000/54000015**, not an invented zero-tap filter (`O/80397228_GXSetCopyFilter.asm:70-77,113-130`).
- At **804963BC**, `[80396568,8039658C)` writes cached pixel-mode BP state; **it is not a CPU busy-wait or GXCopyTex** (`O/80396568_GXPixModeSync.asm:5-13`).
- **804963C0-804963D4** restores copy filtering from mode pointer **u32[805F26F4]**: `(mode.byte19,mode+1A,1,mode+32)`. This is a **mode-derived restoration**, not saved incoming filter state (`CopyEFB.asm:115-120`). Defaults at **805688C0/805688FC/80568938** have AA=0, twelve `(6,6)` samples, taps **[8,8,10,12,10,8,8]**; custom mode remains possible (`DM:964-1019`; `O/80494CD4_Rw_RenderModeSelect.asm:13-53,82-110`).
- At **804963D8-804963EC**, non-null E+2C calls **GXInvalidateTexRegion `[8039868C,803987B8)`**, otherwise **GXInvalidateTexAll `[803987B8,80398800)`**. Region path builds BP66 from region fields; all path emits **66001000 / 66001100**; both bracket it with `[803992C8,803992EC)` cached texture-mode writes (`FD:935828-935920,936611-936619`). E+2C starts null at create; it is cache-region metadata, not another image.
- Sampling uses **the same allocation D**: Draw loads `B+74 -> root` at **8042E604/08**, binds that root at **8042E634** (`O/8042E254_MotionBlur_Draw.asm:241-253`). Save binds scratchRoot at E8F0. State-1 arm **80498C18-80498C38** invokes `[8049819C,804981B8)` to put raster in T+0 and mark its GX descriptor dirty (`O/80498954_RwRenderState_Set.asm:182-189`; `O/8049819C_RwTexture_SetRaster.asm:5-11`).
- Draw realization chain: **80491EA4 -> 8049148C**, **804915B0 -> 80498854**, **8049887C -> 8049AC88(T,0)**. `[8049AC88,8049B568)` follows `T+0 -> raster.root -> E` and on the nonpaletted dirty arm passes **E+1C** to **80397E34** at **8049B2F8**, using raster dimensions/format 6; upload occurs at **8049B524 -> 8039843C** (alternative region load **8049B538 -> 803982C0**). `O/8049AC88_RwTexture_RealizeGXTexture.asm:22-32,350-437,551-564`.
- SDK load closure: `[8039843C,80398490)` obtains a cache region through GX-context+4C8, then calls `[803982C0,8039843C)` at **80398474**. That loader emits GX-object+0C at **803983A4/A8** to FIFO, retaining `(D>>5)&0x1FFFFF` from **80397F90/94** and adding the unit's BP tag. Fresh byte **805EED20=94** proves unit 0 emits **BP94** with the same pixel address (`FD:935585-935700`; `O/80397E34_GXInitTexObj.asm:91-93`). This is descriptor binding, not a second pixel upload/copy.
- Texture descriptor plugin offset **u32[805F2718]** and current T pointer **u32[805F270C]** are distinct from pixels. No CPU pixel copy/reallocation intervenes. A root binding gives root dimensions; a generic view binding still points at root pixel-base, not the CopyEFB interior byteOffset.
- No mipmaps also survives realization: `8049B2CC` extracts zero mip bit; **8049B314-8049B348** passes minLOD=0, maxLOD=E.byte32=0, initial bias=0 to `[803980C8,8039822C)`. RW constants **805FC498=0.0**, **805FC4A0=0x4330000000000000** (`DM:932-946`). Filter table **8056FC90** maps RW filter 1 to GX `(0,0)`; GX encoding **805EED30[0]=0** (`DM:853-904`). Save does not itself force that filter; current T filtering/wrap state is inherited, so no unconditional bilinear-downsample claim.

## 6. Synchronization and teardown, not a second history buffer

- Create initializes E+30 from cached completed-token slot **805F2720** at **80497D08/0C**; texture realization stamps it from current-token **805EEFF0** at **8049ACE0/8049ACE8**. These are u16 tokens, not image pointers (`BackendCreate.asm:29-30`; `RealizeGXTexture.asm:27-29`).
- Root destroy `[80498058,8049819C)` first requires `raster.root==raster` and no **0x80** no-data flag. For an owning type-5 root it compares E+30 with current token; **only if equal**, **804980CC -> 80396428** emits a fence token and **80498104** advances current token modulo **0xE000** (magic **0x92492493**; checked arithmetically for all u16+1 inputs). Then **80498108-80498114** polls regardless of whether a new token was emitted (`O/80498058_RwRaster_BackendDestroy.asm:11-52`). The spin may be unnecessary/already complete; no observed wait duration is claimed.
- Fence `[80396428,803964DC)` emits BP **48/47**, flushes pending/FIFO state; completion predicate `[8049B7DC,8049B87C)` calls token-reader `[803964DC,803964E8)` at **8049B7F0**, caches its u16 result, rejects values >=**E000**, and tests the wrap window against current/last-use tokens (`FD:933627-933674,1201128-1201167`). Reader is `lhz` from `[r13+5E00]+0xE`.
- CopyEFB itself contains **no token-poll CPU wait**. Its pixel-mode synchronization and GPU texture-cache invalidation must not be replaced with a claim of synchronous CPU-readable pixels. The separate lock backend saves input r5 in r23 at **80496CBC**; its flag-bit-0 gated token wait is **80497010-80497090** (`FD:1196314-1196337,1196533-1196564`). It is not called by these blur copies/draws.
- After the owning-root wait, destroy unbinds matching T+0 via **80498138 -> 8049819C(T,0,0)**, frees **E+18 (A, not aligned D)** through engine+10C at **8049814C** (`BackendDestroy.asm:53-66`). Views neither wait nor free backing pixels.
- Raster destroy adapter `[8048AC2C,8048ACA4)` runs plugin destructors **80483194**, invokes engine+5C, then frees the raster metadata via engine+11C (`FD:1183996-1184025`). History destructor `[8042F198,8042F20C)` destroys root if distinct, then view, clears B+74, frees B via **803A1334**, clears M+40 (`O/8042F198_MotionBlur_Destroy.asm:10-28`). Scratch `[804335C8,80433638)` destroys roots then views and clears M+34/+38 (`O/804335C8_EffectManager_DestroyScratch.asm:10-27`).
- Known lifecycle slices order scratch create **801D1A68** before blur create **801D1A80**, and blur destroy **801D19B8** before scratch destroy **801D19D8** (`FD:470110-470119,470154-470161`; fresh bounded DOL comparison). No pointer swap/alternating history selector occurs in the reviewed blur paths. This is a scoped result, not a game-wide assertion that XFB/display buffers never exist.

## 7. Actionable remaining limits

1. **Live clear/pixels:** trace/watch **805F2688** and r4 at **804963B8**, plus actual EFB pixel format/write masks, before claiming clear=false, untouched EFB restoration, or an exact visual result. Startup zero and a load-only syntactic scan are insufficient. Read allocated D only with appropriate GPU completion; its initial/padded contents are not proven zero.
2. **Resize/camera transitions:** neither Draw nor Save allocates, reattaches, or resizes rasters; they use construction-time storage and fixed save quads. Trace lifecycle re-entry/indirect mutations of M+34, B+74 and raster dimensions/offsets if resize safety or multi-camera compatibility is required. Current-camera acquisition can differ between construction/save/draw. Creation/attachment failures are not robustly checked; B+78 is not success validation.
3. **Mode/dimensions:** default GX descriptors contain 640x480 or 640x528, but `[80494CD4,80494EB4)` can install a copied custom mode at **805E4250**. Do not substitute default/video-mode dimensions for the actual V/root dimensions or EFB state (`DM:964-1055`; `Rw_RenderModeSelect.asm:13-71`). Numeric allocation size therefore remains a formula, not a captured byte count.
4. **Parent manifest follow-up (not missing instruction semantics):** optionally export newly checked `[80487254,804874FC)` and `[804954F8,804955FC)` for the closed install edge; `[80496478,804965A8)`, `[80372508,80372534)`, `[8048AC2C,8048ACA4)`, `[803982C0,80398490)`, `[8039868C,80398800)`, `[803992C8,803992EC)`, `[80396428,803964E8)`, `[8049B7DC,8049B87C)`; data **8056F628+4**, **8051C5E8+0x10**, **805EED20+4**. This role deliberately did not write manifests/exports. Re-run `verify` after any parent export.

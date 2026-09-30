# Role 09 — local blur data flow and pointer ownership

## Scope and evidence contract

Only the local `main.dol` blur and its immediate parameter, camera, raster, and device dependencies are covered. No gameplay-system reconstruction, binary patch, reference-map relocation assumption, or runtime validation is claimed.

- Baseline recorded by fresh `address_manifest.json:2-80` and `data_manifest.json`: 5,773,024 bytes; SHA-256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`. This role did not recompute it.
- `O/<name>` below means `analysis/motion_blur/evidence/original/<name>`. These bounded exports include VA, file offset, and raw instruction word.
- `FD:Lx-Ly` means **bounded reads only** of `motion_blur_analysis/full_disassembly.txt`. Each read's address column was checked. For this file's text1 portion, line = `1 + (VA - 0x80006840)/4`; the DOL file offset is instead `VA - 0x80006740`.
- Addresses/ranges are local VAs, ranges end-exclusive. All serialized/native pointers and instruction-sized words here are **32-bit big-endian**, not host-sized pointers. `lfs/stfs` fields are IEEE binary32; `lbz/stb` fields are bytes; `lha/sth` offsets are signed 16-bit.
- The three owned output files were absent on both pre-write read attempts. They were created, not substituted for a prior report.

**Result:** manager, blur/config layout, scratch/history ownership, all 12 direct parameter callsites, provider allocation/installation, and camera callback wrapping code are traced. **Provider vslot+0x24's actual target/RTTI ancestry and the platform standard-function table remain raw-data closure blockers**, explicitly itemized in section 7. Do not silently turn their likely type names into proved pointer edges.

## 1. Manager and effect object

```text
805EF2D8 : manager cache (zero-backed pointer slot)
    800A35D8 -> 800A360C -> M = 8057798C (static zero-backed object)
    M+34 = [805779C0] -> shared full-size scratch view -> root
    M+40 = [805779CC] -> B (0x13C-byte heap allocation)
    B+10 -> M                         borrowed owner pointer
    B+74 -> retained half-size view -> root -> raster plugin -> GPU pixels
```

Evidence: singleton `800A35D8-800A3664`, FD:160615-160649; guard byte `805EF629` at `800A3618/800A3648`; constructor `800A36CC-800A3714`, FD:160676-160693. The static object is passed to `80433A14`, which calls its base and explicitly clears M+2C/+30/+34 (`80433A14-80433A60`, FD:1094774-1094792). Its other initial zero values must not be attributed to an invented whole-object memset. The final manager vptr installed by this constructor is `8053D680`.

`8042F20C-8042F258` allocates **0x13C** through `803A1380`, calls `8042EA50(B,M,optionalConfig)`, then stores the allocation at M+40 (`O/8042F20C_MotionBlur_Create.asm:5-23`). Both manager draw/save wrappers null-check only M+40 before forwarding (`O/8042F16C_MotionBlur_ManagerDraw.asm:5-15`, `O/8042F140_MotionBlur_ManagerSaveScreen.asm:5-15`).

### B layout: complete allocated span, no matrix members

| B offset | Width | Proven role / access |
|---|---:|---|
| +00 | u8 | enable/nonzero gate; not a vptr |
| +01 | u8 | alpha/nonzero gate; copied into all four main-quad alpha bytes |
| +02..03 | 2 bytes | padding/unassigned semantics; parameter bridges do not copy it |
| +04 | f32 | centered rectangle expansion scale S |
| +08,+0C | 2 x 4 bytes | parameter words; bit-copied by bridges, **interpolated as floats upstream, not consumed by draw/save** |
| +10 | pointer32 | borrowed manager/owner |
| +14..73 | 4 x 0x18 | dynamically updated composite vertices |
| +74 | pointer32 | owned retained raster view; its +0 reaches backing root |
| +78 | u8 | capture-path valid/gate byte, not a success counter |
| +79..7B | 3 bytes | padding/unassigned semantics |
| +7C..DB | 4 x 0x18 | constructor-built half-size save quad |
| +DC..13B | 4 x 0x18 | constructor-built full-size restore quad |

Each vertex is `{f32 x,y,z; u8 r,g,b,a; f32 u,v}` at offsets `0,4,8,0xC..0xF,0x10,0x14`. Main vertex bases are +14,+2C,+44,+5C, in LT,LB,RT,RB order. Z is initialized from engine+18; RGB becomes 255 and alpha B+1 during draw. Evidence: `O/8042EA50_MotionBlur_Construct.asm:46-85`; `O/8042E254_MotionBlur_Draw.asm:173-240,310-318`.

Draw reads signed view offsets and signed view width/height; its UV denominator conversion treats parent width/height as **unsigned words**. It computes UVs from current camera-view rectangle / current camera root dimensions, and XY from S and view width/height. It does **not** substitute retained-root dimensions for those UV denominators. With the manifest's verified half/one constants, `dx=0.5f*((S-1.0f)*W)`, `dy=0.5f*((S-1.0f)*H)`; XY is `(-dx,-dy),(-dx,H+dy),(W+dx,-dy),(W+dx,H+dy)`, subject to the actual single-precision operation order. `O/8042E254_MotionBlur_Draw.asm:91-240`; constants `data_manifest.json:63-92`.

No B field stores a previous view/projection matrix, camera displacement, velocity vector, radial center used by draw, or ping-pong history index.

## 2. Parameter transfer and every direct input branch

The parameter record has extent **0x10**, but helpers transfer only +0/+1/+4/+8/+C (14 bytes of fields, excluding padding). Getter uses **r3=hidden output-record address, r4=manager**; setters use **r3=manager, r4=source-record address**. Do not mis-declare `8042F084` as merely returning a record pointer in r3.

| Entry / range | Data flow and null contract |
|---|---|
| `8042E9C4-8042E9F0` | B/config -> output record: two `lbz/stb`, one `lfs/stfs`, two `lwz/stw` |
| `8042E9F0-8042EA1C` | source record -> B, same fields; no source-null check |
| `8042EA1C-8042EA50` | same assignment only if source is nonnull |
| `8042F084-8042F0E8` | hidden output + M; if M+40 exists, calls copy helper; else writes enable=0, alpha=0, scale=1 and copies **uninitialized local stack words** from SP+10/+14 into output+8/+C |
| `8042F0E8-8042F114` | if M+40 exists, invokes unconditional source copy |
| `8042F114-8042F140` | if M+40 exists, invokes null-source-checked copy |

Evidence: same-named `O/8042E9C4...`, `8042E9F0...`, `8042EA1C...` assembly files, lines 5-17; `O/8042F084_MotionBlur_GetParameters.asm:5-29`; setter exports lines 5-15. No bridge invalidates B+78 or changes raster ownership.

`motion_blur_analysis/branches.tsv:20745,20758,20890,69553-69555,69734-69736,69944-69946,138929,138942,138965` contains precisely these direct incoming references:

| Immediate slice (inclusive callsite shown separately) | Calls | Parameter source / change |
|---|---|---|
| `800A3528-800A3570` | F114 at **800A356C** | stack+10: enable=0, alpha=0x80, S=`f32[805F3BD0]`; +8/+C not initialized in this slice |
| `800A3714-800A3760` | F114 at **800A375C** | stack+10: enable=1, alpha=0x80, same S; last two words not initialized |
| `800A47B0-800A4830` | F114 at **800A481C** | stack+8: enable from comparison of object+38 with `f32[805F3BDC]`; alpha=low byte of `fctiwz(f32(f32[805F3BE0]*f32(object+38)))`; same fixed S; last two words not initialized |
| `80204714-80204760` | F084 **80204720**, F0E8 **8020475C** | stack+30 -> stack+40 snapshot, replace enable only with 0 |
| `802055DC-80205628` | F084 **802055E8**, F0E8 **80205624** | stack+20 -> stack+48 snapshot, replace enable only with 0 |
| `80206314-80206360` | F084 **80206320**, F0E8 **8020635C** | stack+30 -> stack+40 snapshot, replace enable only with 0 |
| `8043A900-8043A97C` | F084 at **8043A908** | controller snapshot into +14C; resource-derived target into +15C |
| `80439EBC-80439FE0` inside `80439A8C` | F0E8 at **80439FD8** | controller interpolates snapshot/target into stack+14 |
| `8043A4B8-8043A5A4` | F0E8 at **8043A544** | controller teardown/finalization assigns **target +15C**, not the saved starting record |

Address-verified instructions: FD:160571-160588,160694-160712,161757-161788,522166-522184,523112-523130,523958-523976,1101873-1101903,1101216-1101288,1101599-1101657. Surrounding gameplay branches were not pursued.

### Immediate upstream controller fields

| Controller offset | Type / source |
|---|---|
| +18 | u32 flags: tested active bit 0 and inhibit bit 2 before parameter update |
| +1C | borrowed polymorphic manager pointer, cast before setting parameters |
| +FC | u32 source flags; bit `0x10000` supplies target enable |
| +100 | f32 interpolation weight t |
| +104 | f32 duration-like input used by common weight updater |
| +108 | borrowed resource-record pointer; resource+0 selects effect kind; **kind 1 is blur** |
| +14C..15B | starting parameter snapshot: enabled/alpha/scale/opaque pair |
| +15C..16B | target parameter record |

`8043A900-8043A978` maps resource+08 u8 -> target alpha, +0C f32 -> scale, +10/+14 f32 -> the two opaque words; target enable comes from +FC bit 16, not resource+0's kind value. The controller getter/target setup retains its own snapshot; B itself does not retain two parameter banks.

For the interpolation branch, `q=f32(K-t)`, `K=f32[8051CB5C]`; scale and both opaque floats use `fmadds(target,t,f32(saved*q))`. Alpha is mixed similarly only if endpoint bytes differ, then `fctiwz` and `stb` truncate to a byte, **not saturate**. Enable is ORed while interpolating. A `bge` comparison against the distinct address `8051CB58` bypasses interpolation and uses the target. Those upstream constant bits are not in the current data manifest: do not silently assume both are 1.0. Evidence: FD:1101216-1101288,1101873-1101903. Common weight input/normalization is `8042B63C-8042B6CC`, FD:1086336-1086371.

The separate direct fade object updates only its scalar +38 using incoming f1 and `f32[805F3BD4]`, gated by byte +34; it clamps against `805F3BDC/805F3BCC` and calls the parameter writer (`800A4830-800A48C8`, FD:161789-161826). This is an immediate scalar dependency, not evidence for camera/player velocity. Unwritten +8/+C words in other inputs and the getter fallback must remain indeterminate/inactive in a faithful reconstruction, not invented center coordinates.

## 3. Current-camera provider: installation traced, final data edge open

```text
[805E25C8] --8042B620--> P
    cast(P, vptrOffset=0, targetRTTI=8056BCE4,
         sourceRTTI=8056BCD0, throwFlag=0) --803A1AFC--> P'
    call pointer32[ pointer32[P'+0] + 24 ](P') --> camera C
```

Draw: `8042E280-8042E2C0`, `O/8042E254_MotionBlur_Draw.asm:16-31`; save: `8042E86C-8042E8AC`, `O/8042E838_MotionBlur_SaveScreen.asm:18-33`; constructor: `8042EA84-8042EAD4`, constructor export lines 18-37. Scratch initialization uses the same chain but duplicate descriptor addresses **target 8056BDC8 / source 8056BDB4** (`8043364C-8043368C`, FD:1094532-1094547).

- Getter `8042B620-8042B630` loads the pointer slot; setter `8042B630-8042B63C` stores r3 directly. No refcount, ownership transfer helper, or camera matrix is involved (FD:1086329-1086335).
- The branch index has **one direct call** to that setter: `801E28EC` (`branches.tsv:64668`). At `801E28D0-801E28F0`, startup allocates **0x10** with `803A1380`, constructs through **801E293C**, and installs the result; allocation failure installs null. FD:487461-487468.
- Constructor **801E293C** calls **801E2978**, then stores final vptr **8053E93C** at object+0 (`801E2954-801E2964`). The preceding constructors install **8051DB30** and, in the innermost body, **8053E8E8 then 8053E8C0**, all at the same object address (FD:487488-487524). These are concrete installations, not map-derived relocated addresses.
- Local strings identify `Effect::PJS::EffectSystemPJS` at **804D21B0**, and `Effect::RW3::EffectSystemRW3` at **804D2170/8051C3A8/8051C5A8** (`strings.tsv:19667,19669,27673,27684`). The map labels the matching-shaped constructor sequence PJS / USA / System (`reference_GUPE8P.map:16576-16578`). **The exact ancestry/descriptor-to-name connection is still conditional until the table/RTTI words are dumped.** A constructor label is not proof of the vslot target.
- `803A1AFC-803A1D60` is a real RTTI walk: reads vptr/RTTI, adds the table's +4 adjustment, compares type-name strings and walks base records. With r7=0, failure returns null. FD:945328-945480. It is neither a camera update nor an interpolation/matrix function.

**Do not equate this provider slot with engine+0 or with backend slot 805F26F8.** The provider's final selected-camera function is the remaining open edge. Constructor/capture/draw each resolve a camera independently; B owns no persistent camera pointer. Actual P lifetime after installation, all possible indirect replacements, and returned-camera selection/lifetime need that final method/data edge, not a guessed global-camera API.

## 4. Required camera callback adapters and actual installations

| Layer | Verified code / pointer installation |
|---|---|
| Blur adapters | `8048652C` calls C+18; `80486504` calls C+1C. Each export lines 5-14. Three begin callsites are E60C/E8C8/E970; matching end callsites E820/E95C/E9A4. |
| Default camera constructor | `80486AA8-80486C1C`: stores **804862DC at C+10**, **804863A4 at C+18**, **80486340 at C+1C**; C+60/+64 initially null. Then invokes camera plugin constructors through `804830EC`. FD:1179803-1179895, especially 1179825-1179840/1179889. |
| World camera plugin registration | `80462EAC-80462ED4`: registers size **0x1C**, ID **0x509**, init **80461C98**, destroy **80461D50**, copy **80461CF4**, through `80486A08`; result stored at **805F25B0**. FD:1143196-1143205. |
| World camera plugin installation | `80461C98-80461CF4`: E=C+u32[805F25B0]; saves old C+18/+1C/+10 at E+10/+14/+18, then installs **80461B6C / 80461BB8 / 80461BF4** in C+18/+1C/+10. E+0C is its world pointer, initially null. FD:1142039-1142061. |
| World begin/end forwarding | `80461B6C-80461BB8` writes current world to engine+4, increments engine+8 u16, then calls saved E+10; `80461BB8-80461BF4` clears engine+4 and calls E+14. FD:1141964-1141997. |
| Default begin/end forwarding | `804863A4-80486410` sets engine+0=C, syncs frame objects, calls **engine+4C(0,C,0)**; on success calls 8049C198 and returns C. `80486340-804863A4` calls **engine+70(0,C,0)**, clears engine+0 on success, returns C or null. FD:1179329-1179380. |
| Platform backend bodies | **804956F4-80495CAC** takes C in r4, stores **805F26F8=C**, prepares only current camera state, returns 1. **80495CAC-80495CBC** clears 805F26F8 and returns 1. FD:1194926-1195295. **Their engine+4C/+70 table membership still needs section 7's raw standard table.** |

Two other direct camera-plugin registration constructors were checked: **80453780-80453794** clears only its 8-byte extension; **80457240-8045725C** clears only its 12-byte extension. They do not wrap C+18/+1C. Registration calls at `80453ED0/80457BC0`; FD:1127837-1127847,1131735-1131747,1127377-1127381,1131137-1131143. This does not prove absence of all indirect camera-field rewrites.

Minimum C fields reached here: +04 frame pointer32; +10 sync callback32; +14 projection-kind word; +18/+1C begin/end callbacks32; +60 raster-view pointer32; +68/+6C view-window f32, +70/+74 reciprocal-window f32, +78/+7C current view-offset f32, +80/+84 near/far f32. Blur directly needs +60; callbacks require the rest of their normal current-camera state. No field in this trace is a previous-frame camera transform.

The standard-function installer copies **27 (index,function) records from 8051D400**, fills default targets with **80494CCC**, and stores selected function pointers (`804954F8-804955FC`, FD:1194799-1194863). The table is distinct from the already exported **8056F50C RwDevice** descriptor. That descriptor proves engine+20 -> 80498954, +24 -> 804984C4, +30 -> 80491E80 after device copying (`data_manifest.json:488-512`, FD:1194521-1194553). It does **not** alone prove engine+4C/+70 or raster-create slots.

## 5. Raster/view/plugin/pixels chain and width distinctions

```text
V = pointer32[B+74]             // retained view, not pixels
R = pointer32[V+0]              // root; root's own +0 is self
P = u32[805F2700]               // registered BYTE OFFSET, not pointer
E = R + P                      // raster extension
A = pointer32[E+18]             // allocation-owner address
D = pointer32[E+1C]             // 32-byte-aligned GPU-readable pixels
```

Core create initializes root+0=self at `8048AF48`. SubRaster assigns view+0=parent+0 only after its device callback succeeds (`O/8048AE14_RwRaster_SubRaster.asm:17-42`). Thus this is a **flattened root pointer**, not a recursive parent walk. `8048AB7C` simply returns the stored signed offsets; it does not add ancestors.

| Object field | Width / direct use |
|---|---|
| raster +0 | pointer32 to root/self |
| raster +0C,+10 | dimension words; signed arithmetic/halving in constructors and draw positions, unsigned conversion where explicitly noted for UV denominators; low16 at GX copy boundary |
| raster +14 | 32-bit depth word; copy switch recognizes 4/8/16/32 |
| raster +1C,+1E | s16 offsets; SubRaster adds parent's signed offsets and truncates back to 16 bits |
| raster +20,+21,+23 | u8 type, allocation-related flags, packed format byte |
| E+0C | u32 GX texture format, copied to GXSetTexCopyDst |
| E+14 | u32 flags; bit 0 also influences backend alpha/Z state |
| E+18 | pointer32 owning allocation base |
| E+1C | pointer32 aligned pixel base used by **both GX copy and texture initialization** |
| E+2C | optional texture-cache-region pointer32, not another image-history buffer |
| E+30 | u16 last-use/synchronization token |

Raster plugin registration `80496440-80496478` requests **0x34 bytes**, ID 0x40C, and stores returned offset at 805F2700 (FD:1195777-1195790). No fixed numeric offset is assumed.

In verified backend `80497CA8`, extension fields are initialized, then format helper `80497458` decodes flags. **For 0x505**, the actual branch reaches `80497884-804978A0`, sets E+0C=**6**, E+14=**1**, raster+14=**32** (FD:1196807-1196831,1196936-1196949,1197074-1197080). This is the conventional GX RGBA8 / RW 0x500 format. The remaining standard-table edge must be checked before claiming every possible runtime callback uses this backend.

Nonzero-dimension roots allocate pixels through engine+108; allocation address is saved at E+18 and `(A+31)&~31` at E+1C (`80497F30-80497FB4`, FD:1197501-1197533). Zero-sized views take the no-data path (`80498024-80498038`, FD:1197562-1197566). These allocations are runtime memory, not DOL texture data. No particular width/height or total byte allocation is imposed here.

### EFB copy uses the view and the root for different purposes

`80496208-80496410` takes `(view,scaleFlag)`. It obtains R=view+0 and E=R+P, builds **source** rectangle from view offsets/dimensions, **destination** dimensions from R, and format from E+0C. Blur passes scaleFlag=0 at both copy callsites. Actual GPU destination is **E+1C + depth-dependent byte offset**; for depth 32 it is `E+1C + 4*(4*s16(view+1C) + align4(u32(R+0C))*s16(view+1E))`, with 32-bit native arithmetic. `O/80496208_RwRaster_CopyEFB.asm:17-49,89-113`.

The clear argument is **low8(u32[805F2688])**, not the helper's second argument. `803975AC` encodes the supplied pointer's address/32 into GX BP state (`O/803975AC_GXCopyTex.asm:59-62`). Post-copy sync and cache invalidation follow at `804963BC/804963E4/804963EC`. Live value/writers of 805F2688 were not closed by this role.

### The same backing pixels reach sampling

Draw binds **R=pointer32[pointer32[B+74]+0]**, not V or D (`8042E604-8042E634`). State 1 updates `T=pointer32[805F270C]`, then `T+0=R` through **8049819C** (`O/80498954_RwRenderState_Set.asm:182-189`). **805F270C is concretely a current texture-descriptor pointer in these paths**, despite the manifest's broad `Rw_render_state_pointer` name; the separate shared state block is **805E42D0**.

Texture flush `80498854` passes T to **8049AC88(T,0)**. That function follows `T+0 -> raster+0 -> root+P`, and its nonpaletted branch passes **E+1C** to **80397E34** at **8049B2F8**, with low16 raster width/height and E+0C format. GX texture object lives in T's extension, not in the pixels: texture extension offset **u32[805F2718]**, registered size **0x24** at `8049AC48-8049AC88` (FD:1200387-1200402). Its first 0x20 bytes are the GX object; +20 holds dirty/config bits. Upload is `8049B524 -> 8039843C`, or preloaded-region path `8049B538 -> 803982C0`; FD:1200949-1200962. Pixel alias evidence: FD:1200420-1200430,1200801-1200815; flush export lines 10-25.

## 6. Initialization, capture, teardown, and lifetime boundaries

- Task setup calls shared scratch init **801D1A68 -> 80433638** before blur create **801D1A80 -> 8042F20C**, passing optional config=null. Teardown calls **801D19B8 -> 8042F198** before shared scratch teardown **801D19D8 -> 804335C8**. FD:470110-470124,470147-470161; branch index:62534,62542,62549,62553.
- Scratch init `80433638-80433754`: resolves C, takes **C+60 view's** width/height, allocates zero-size view with flags 5, full-size root with flags 0x505, attaches the rectangle, stores view at M+34. It also makes M+38, which blur does **not** read. FD:1094527-1094597. Scratch destructor frees M+34's root, then view, clears M+34; analogous peer cleanup for +38 (`804335C8-80433638`, FD:1094499-1094526).
- Blur constructor instead takes **C+60->root dimensions**, halves each signed dimension with `srawi; addze` (truncation toward zero), makes a zero-size 0x505 view and a half-size 0x505 root, attaches, and stores view at B+74. Constructor export:86-118,256-263,354-382. The two allocations are distinct under successful allocation; there is no two-history ping-pong array.
- Initial enable/alpha=0, scale=1, owner=M are written **before** camera resolution. B+74/B+78 and geometry are initialized **only after** successful resolution. Early return still returns the allocated B. Default successful completion sets enable=0, alpha=0x80, scale=1; it does not initialize B+8/+C. Allocation/subraster results are not robustly checked in this caller. Do not invent blanket zeroing or all-or-nothing construction. Constructor export:5-37,84-85,372-395.
- Save clears B+78 **before all gates**, checks enable/alpha/provider/camera, copies full scratch root from EFB (**E8C0**), optionally draws scratch with B+7C (**E954**), copies retained view from EFB (**E968**), optionally restores scratch with B+DC (**E99C**), then sets B+78=1. This last store happens even if either BeginUpdate returned null. `O/8042E838_MotionBlur_SaveScreen.asm:5-103`.
- Draw requires B+78 nonzero but does not consume/clear it. It updates the main quad, begins C, binds retained root, renders, restores seven RW states, ends C. It does not save/restore texture state 1. Raster age/frame ordering belongs to scheduler evidence, not to a guessed previous-frame count.
- Blur destructor `8042F198-8042F20C` frees root first if root!=view, frees view, clears B+74, releases B with 803A1334, clears M+40. `O/8042F198_MotionBlur_Destroy.asm:9-28`. Scratch remains manager-owned until its separate teardown.
- Backend raster destruction only releases pixels for root==self with owned data; it waits against the recorded u16 GPU-use token, detaches matching current texture, and frees **E+18**, not aligned E+1C (`80498058-8049819C`, FD:1197575-1197655). A view release therefore must not independently free the root's pixels. Core raster release additionally invokes plugin destructors and releases the raster object (FD:1183996-1184025). Standard-table membership remains the explicit data check below.

## 7. Concrete unclosed pointers/ranges for parent verification

These are bounded, local, actionable gaps. Existing fresh manifests close core constants/task/RwDevice tables, **not these missing table contents**.

| Priority | Exact missing local data/code | Required verification |
|---|---|---|
| **P1 provider** | **8053E93C** vtable; candidate +24 word **8053E960**; dump **8053E8C0-8053E980**, **8051DB30-8051DB60**, then follow first-word RTTI/base records | Close actual installed PJS/USA/RW3/System ancestry, offset-to-top, and vslot+24 target. If cast adjusts P, inspect the resulting base vtable too. Disassemble the returned target and its direct camera getter/storage accesses; that is the only missing camera-selection edge, not permission to trace gameplay. |
| **P1 cast descriptors** | **8056BCD0-8056BCEC** and scratch aliases **8056BDB4-8056BDD0** | Read descriptor name/parent pointers and verify source/target names and duplicate-type equivalence. Local string candidates are 8051C3A8 / 8051C5A8, but string proximity alone is not a pointer proof. |
| **P1 RW standard functions** | **8051D400-8051D4D8** (27 x 8-byte records), system selector table **8056F494-8056F4F0**; installer **804954F8-804955FC** | Verify engine+4C -> 804956F4, +70 -> 80495CAC, +58 -> 80497CA8, +5C -> 80498058, +78 -> 804981B8, including the engine standard-table base/install call. RwDevice 8056F50C alone cannot close these slots. |
| P2 provider lifetime | Writes to **805E25C8**, especially indirect stores not indexed as branches; installed provider vslots +8/+C used at **801D1A60/801D19EC** | Direct setter has one incoming branch, not proof of all writes. Determine whether teardown releases only provider-owned heap state or the provider itself; do not invent a refcount. |
| P2 exact input constants | **805F3BCC-805F3BE4**, **8051CB58-8051CB78** | Decode BE f32/f64 values used by fixed/fade/controller inputs. Source +8/+C remains inactive in draw regardless of values. |
| P2 live scalar/pointers | **805F2688** clear word; **805F2700/805F2718/805F25B0** registered offsets; C/raster dimensions and allocation addresses | Code establishes types/uses, not live values. Need bounded writer xrefs/data dump or runtime trace if exact environment values are claimed. |

Required extra disassembly exports for repeatable raw-DOL comparison: `801E28D0-801E29D0`, `8042B620-8042B63C`, `803A1AFC-803A1D60`, `804335C8-80433754`, `80486AA8-80486C1C`, `80462EAC-80462ED4`, `80461B6C-80461CF4`, `80486340-80486410`, `804954F8-804955FC`, `80496440-80496478`, `80497458-804974F8`, `8049765C-804976A0`, `80497850-804978A0`, `80497CA8-8049819C`, plus the nine input slices in section 2. Existing FD words were address-checked, but this role did not freshly decode/export them from the DOL.

## 8. Actions and deliverables

- Shell command actually run: **`pwd && ls`**, succeeded in the requested workspace. No Python, background process, build, hash recomputation, binary write, or runtime trace was executed by this role.
- Read AGENTS, fresh manifests/bounded original assemblies, and bounded address-indexed disassembly; used `branches.tsv` only as a branch index, with callsite instructions independently read. An initial context lookup returned no relevant results; normal bounded READ/GREP supplied the evidence. Never grepped the huge disassembly.
- Created only **`analysis/motion_blur/evidence/09_dataflow.md`**, **`analysis/motion_blur/motion_blur_dataflow.md`**, **`analysis/motion_blur/motion_blur_globals.md`**. Original/prior/GPT_SOL artifacts, helpers, manifests, and other roles' files remain untouched.
- Parent must close section 7 before claiming full provider/callback closure, run final hash/instruction/data manifest verification, and separately obtain runtime evidence before claiming actual image age or behavior. No history matrices were invented to fill any gap.

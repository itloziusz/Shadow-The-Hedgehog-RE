# Backend source reconstruction notes

Scope/ownership: only `backend_reconstructed.inc` and this file. Original DOL, existing exports, prior analyses, GPT_SOL files, and `reconstructed/motion_blur.h/.cpp` are read-only for this role.

## Integration contract

- Include/append `backend_reconstructed.inc` **outside** `namespace motion_blur`, **after** the core `.cpp`, so the existing `ppc::Fpr`, loads/stores, and instruction-level arithmetic helpers are already defined. The fragment reopens `motion_blur`.
- This is a freestanding 32-bit big-endian PowerPC C++17 source model, not a host program, linkable replacement renderer, binary-equivalence claim, or runtime test.
- SDK declarations are explicit **real-address adapter boundaries**. Their names identify exact original entry addresses. An integration must implement/bind their ABI; no successful no-op stubs are supplied.
- Mixed floating/integer adapters specify original `rN` and `fN` argument intent, using `Fpr` for actual FPR contents. They are not claimed to be the original SDK C headers or the original compiler's struct/varargs ABI.
- The complete CopyEFB wrapper defines the existing `original::RwRaster_CopyEFB_80496208` declaration. Other functions remain in `backend`; complete bodies and extracted paths are explicitly distinguished. The complete additional bodies are SetRaster, RestoreProjectionViewport, and BackendSubRaster. Render-state, Im2D prepare/emission, realization, format/create and level-size entries are **scoped extractions**, not a full generic renderer.
- `require_extracted_path` (`backend_reconstructed.inc:286-294`) is a **non-original scope assertion** implemented with `__builtin_trap`. It is not attributed to an original address and is not an invented RenderWare failure/success result. Do not install path-scoped entries as generic engine callbacks. Unknown render-state IDs, non-strip4/count4 primitives, palettes, mipmaps, null-texture fallback, and unrelated format/create requests are intentionally outside these source functions' domains.
- Only a source integration is supplied: current engine callbacks in the core are still real original-address dependencies. Merely including the fragment does not install callback pointers or prove a live callback binding.

## Completed evidence mapping: CopyEFB

`original/80496208_RwRaster_CopyEFB.asm:5-134`, original `[0x80496208,0x80496410)`, size `0x208`, DOL `[0x0048FAC8,0x0048FCD0)`.

| Blocks / call sites | Reconstructed meaning |
|---|---|
| `80496238..244`; `80496248 -> 80397228` | Exactly one `raster->parent`; dynamic raster-plugin offset at `805F2700`; disable copy sample/vertical filtering with all-zero arguments |
| `80496254..278` / `8049627C..29C` | Signed 16-bit source offsets truncated to low16, optionally doubled along with source width/height; calls to `80396C50` at `80496274` / `8049629C` |
| `804962A0..2B8` | Destination low16 parent dimensions, plugin GX format, low8 of downsample argument; `80396D00` |
| `804962BC..3A8` | Depth dispatch at `8056F544`: depth4 -> `804962E4`, 8 -> `8049630C`, 16 -> `80496330`, 32 -> `80496358`; all other depths -> error at `80496380` |
| `80496394`, `804963A0` | Error code `0x8000000C` through `8047EC78`, then error record through `8047EBD4`; no copy/filter restoration/invalidation on this path |
| `804963A8..3B8` | Plugin image+tile offset; independent clear word `805F2688`, low byte only; `GXCopyTex_803975AC` |
| `804963BC` | `GXPixModeSync_80396568`, not a GPU draw-completion wait |
| `804963C0..3D4` | Render-mode pointer `805F26F4`: AA byte `+19`, sample pattern `+1A`, vertical-filter enable **1**, taps `+32`; `80397228` |
| `804963D8..3EC` | Plugin `+2C` non-null => `GXInvalidateTexRegion_8039868C`; null => `GXInvalidateTexAll_803987B8` |

All tile-address arithmetic is modulo 2^32. With `A4=(parentWidth+3)&~3`, the RGBA8 offset is `4*(4*signExtend16(x)+A4*signExtend16(y))` modulo 2^32. This is **not** linear scanline byte addressing. Depth4 uses an arithmetic right shift of the wrapped sum. CopyEFB downsample does not set copy-clear, does not change samplers, and has no cull/write-mask resets.

## Additional hash-checked bounded evidence (not new exports)

- `python -B analysis/motion_blur/tools/dol_evidence.py data 0x8056FA18 0x8056FA28`: fog-type table words `0,2,4,5`, DOL offset `0x0056CA18`. **Parent should add this small data record to the manifest if complete manifest provenance is desired**; the 55-record snapshot omitted it.
- `... disasm 0x8039868C 0x80398804`: region invalidation `[8039868C,803987B8)` and global invalidation `[803987B8,80398800)` corroborate the actual SDK boundary names (BP register `0x66` invalidation words and texture-state flushes).
- `... disasm 0x80496478 0x804965F0`: level-size helper `[80496478,804965A8)` includes level0 RGBA8 tile allocation at `80496548..564` plus final 32-byte rounding `80496590..598`. The requested window also contains a following function prefix; no claim of that following function's reconstruction.
- `... data 0x8056F96C 0x8056F970`: depth-zero format-dispatch word `8049751C`, DOL `0056C96C`, proves the initial empty scratch view's default type5/RGB565 arm.
- `... data 0x8056F628 0x8056F62C`: depth32 size-dispatch word `80496548`, DOL `0056C628`.
- `... disasm 0x8039822C 0x80398490`: LOD bias `[8039824C,8039828C)`, bias clamp `[8039828C,80398298)`, edge LOD `[80398298,803982AC)`, anisotropy `[803982AC,803982B8)`, preloaded load `[803982C0,8039843C)`, ordinary load `[8039843C,80398490)`. Ordinary load calls the GX context's region callback `+4C8`, then the preloaded loader; no fake texture binding is supplied by the fragment.
- `... disasm 0x8039A158 0x8039A300`: SetProjectionv `[8039A158,8039A1E4)`, GetProjectionv `[8039A1E4,8039A22C)`, position matrix load `[8039A22C,8039A27C)`, current matrix `[8039A2CC,8039A300)`. These are SDK adapter boundaries; unrelated normal-matrix instructions within the requested window are not reconstructed.
- `... disasm 0x8039A3B4 0x8039A59C`: viewport helper `[8039A3B4,8039A444)`, jitter `[8039A444,8039A49C)`, viewport `[8039A49C,8039A4E4)`, scissor `[8039A4E4,8039A55C)`, scissor-box offset `[8039A55C,8039A59C)`. SDK jitter subtracts 0.5 from Y only when its field argument is zero; the fragment passes the exact `VIGetNextField() ^ 1` instead of doing an extra Y adjustment.
- `... disasm 0x80372508 0x80372554`: `[80372508,80372534)` is `DCInvalidateRange`, with `dcbi` and 32-byte increments. Its following flush-function prefix was read only to confirm the boundary, not reconstructed.

**Manifest handoff:** adding `[80496478,804965A8)` and the three small data spans `8056FA18..FA28`, `8056F96C..F970`, `8056F628..F62C` would put the fragment's extra required bounded evidence under the parent's normal export/verify ledger. I did not modify manifest files or exports. Optional SDK adapter ranges above can also be exported if desired; they are not claimed to be reconstructed SDK bodies.

## Delivered source inventory and exact coverage

All source references below are to `backend_reconstructed.inc`. Original ranges are half-open, not a claim that every branch of a ranged function is supplied.

| Source | Original function range / size | Evidence file and coverage |
|---|---|---|
| `211-283` | `80496208..80496410`, `0x208` | `original/80496208_RwRaster_CopyEFB.asm:5-134`; **complete**, including all four depth-offset arms and real error reporting |
| `331-335` | `8049819C..804981B8`, `0x1C` | `original/8049819C_RwTexture_SetRaster.asm:5-11`; **complete**, raster pointer store then plugin flags **assignment** `0x01000000`, not OR |
| `341-361` | `804984C4..804986B8`, `0x1F4` | `original/804984C4_RwRenderState_Get.asm:5-129`; only IDs 1,6,8,9,10,11,12,14 |
| `367-475` | `80498954..80498F74`, `0x620` | `original/80498954_RwRenderState_Set.asm:5-396`; all input words for those same eight IDs, including exact equality/validation and failure handling |
| `483-505` | `80498854..80498910`, `0xBC` | `original/80498854_RwTexture_FlushState.asm:5-51`; full Flush control logic, with its callee explicitly scoped to RGBA8/no-mip/map0 |
| `549-619` | `8049148C..80491774`, `0x2E8` | `original/8049148C_Im2D_Prepare.asm:5-190`; common + textured setup, both viewport choices, all projection/matrix setup; **untextured arm omitted** |
| `652-705` | `80491774..80491B08`, `0x394` | `original/80491774_Im2D_RestoreProjectionViewport.asm:5-233`; **complete** root/view, ordinary/doubled and split-scissor branches |
| `709-744` | `80491E80..8049245C`, `0x5DC` | `original/80491E80_Im2D_RenderPrimitive.asm:5-81,371-379`; only textured primitive4/count4, pair-loop CTR=2 and no odd tail |
| `795-857` | `8049AC88..8049B568`, `0x8E0` | `original/8049AC88_RwTexture_RealizeGXTexture.asm:22-32,350-362,403-470,511-564`; nonpaletted RGBA8/no-mipmap, map0, non-null descriptor; dirty, sampler-change and unchanged branches plus both final load paths |
| `877-895` | `80497458..80497A70`, `0x618` | `original/80497458_RwRaster_ResolveFormat.asm:5-63,134-147,259-278,388-394`; `0x505` and observed empty scratch `(0,0,depth0,flags5)` only |
| `901-913` | `80496478..804965A8`, `0x130` | Fresh bounded original bytes; **RGBA8 level0 only**, not full depth/mip-size helper |
| `921-972` | `80497CA8..80498058`, `0x3B0` | `original/80497CA8_RwRaster_BackendCreate.asm:5-43,54-59,73-87,110-122,167-213,228-240`; root initialized by outer wrapper; selected format paths, zero-size result, single-level pixel allocation and both allocation-error reports |
| `979-988` | `804981B8..804981E8`, `0x30` | `original/804981B8_RwRaster_BackendSubRaster.asm:5-16`; **complete**, prefix metadata copy only |

The corresponding DOL file spans, in the same function order, are `0048FAC8..0048FCD0`, `00491A5C..00491A78`, `00491D84..00491F78`, `00492214..00492834`, `00492114..004921D0`, `0048AD4C..0048B034`, `0048B034..0048B3C8`, `0048B740..0048BD1C`, `00494548..00494E28`, `00490D18..00491330`, `0048FD38..0048FE68`, `00491568..00491918`, `00491A78..00491AA8`.

### Render-state semantics

Raw dispatch record snapshots are at source `297-309`. Getter/setter entries come from `data_manifest.json:136-176,709-750`, not RenderWare enum-name guesses.

| ID | Getter block | Setter block | Actual behavior |
|---|---|---|---|
| 1 | `804985A8` | `80498C18` | Current texture descriptor's raster pointer; change calls SetRaster (`80498C2C`). No sampler change and no immediate texture upload |
| 6 | `804985D0` | `80498CA4` | Cache `805E42D0+4`; GX **enable=1**, compare `3`/`7`, update=low8(cache+0); calls `80498CD0` / `80498D14` |
| 8 | `804985BC` | `80498C38` | Cache+0; GX **enable=1**, compare=cache+8, update `1`/`0`; calls `80498C5C` / `80498C8C` |
| 9 | `80498590` | `80498BFC` | Low8 of texture+50; setter replaces low8, preserves upper24, no filter validation and no immediate GX call |
| 10 | `804985E8` | `80498D38` | Cache+34. Equality returns 1 **before** validation. Unequal accepted values 1,2,5..10; table `8056FA28`; `GXSetBlendMode(1,src,dst,0)` at `80498DA0`; other unequal values return 0 |
| 11 | `80498600` | `80498DC0` | Cache+38. Equality returns 1 before validation. Unequal accepted values 1..8; blend call at `80498E10` |
| 12 | `804986B0` | `80498F58` | **Unsupported in both directions. Getter returns 0 with no output write/dereference; setter returns initial `r6=0`, no state change or GX call** |
| 14 | `804984E4` | `80498990` | Cache+10; enabling may refresh camera `+80/+84/+88` planes (honoring cache+20 override and guarded camera null); translated fog type; calls at `80498A28` / `80498A6C`; disable floats exactly 5,10,0.05,10 |

State12's saved stack word in `reconstructed/motion_blur.cpp:181-190,293-299` stays raw residue. The later core `ppc::load_word` is retained and passes that residue to the ineffective setter. No fictitious saved vertex-alpha value is initialized. The core's descriptive `vertex alpha` call comments must not be read as proof that the backend supports state12.

### Im2D / GX path

- Source `550-575`: exact position/color/UV direct descriptors and VAT0: `(POS9,1,4,0)`, `(CLR0=11,1,5,0)`, `(TEX0=13,1,4,0)`. Calls match original `8049149C..80491568`.
- Exactly one TEV stage and one channel; channel calls `(4,0,0,1,0,0,2)` and `(5,0,0,0,0,0,2)` at `80491514/534`. Stage0/op0 modulates. Texgen call `(0,1,4,0x3C,0,0x7D)` at `80491598`; order `(0,0,0,4)` at `804915AC`; flush at `804915B0`.
- `data_manifest.json:1258-1295` and `original/803992EC_GXSetTevOp.asm:5-39`: selected color/alpha words are `C008F8AF` and `C108F2F0`. Color selectors encode `(ZERO,TEXC,RASC,ZERO)`; alpha selectors `(ZERO,TEXA,RASA,ZERO)`. Thus default alpha is **sampled texture alpha × vertex alpha**, not unconditional `blurAlpha/255`. `GXSetTevOp` keeps alpha-word low four swap-selector bits; inherited swap state can affect the inputs. No identity-swap reset is added.
- Source `577-596`: full-frame viewport uses render mode `+4` and **`+8`**, not `+6`. Field mode calls `VIGetNextField_80380004` then jitter with XOR1; otherwise normal viewport. Depth endpoints are exact +0 and 1.
- Source `598-619`: persistent projection at `8056F13C` changes only `xx=2/W`, `yy=-2/H`; old GX projection saved at `805E4218`; matrix `8056F158` changes only x/y translation to `0.5+signedRasterOffset`; z scale remains -1. Actual single-rounding PPC helpers preserve conversion/add/divide operations; no convenient host expression replaces them.
- Source `652-705`: root camera raster restores projection only. Views reconstruct viewport/scissor from current camera fields. In `805F2698!=0` mode, normal viewport doubles Y and height as integer words; jitter branch does **not** double them. Split-scissor branches intentionally pass the original unchanged limit words in several height arguments: no invented `limit - y` clipping formula. Original calls at `804918BC`, `80491A10/A34/A48/A88/AB8/AD4`, `80491A54/AE4`, and final projection `80491AF0` are all represented.
- Source `709-744`: RW4 -> table word `8056F130=98`, `GXBegin_80396934(0x98,0,4)`, then exactly four vertices in original order. Each FIFO vertex is x/y/z f32, R/G/B/A bytes, u/v f32 to **`CC008000`**. Source preserves z/y/x and v/u load order, emits the hardware-required order, and retains the pair-loop structure. No `GXEnd` is invented; original shared restore call is `80492438`.
- There are **no added cull-none, color/alpha write-mask, indirect-stage, TEV-direct, TEV-swap, automatic-coordinate-scale, texture-bind or sampler resets**. Restore does not restore matrix0/current-matrix, descriptors, channels, or TEV. These absences materially limit any visibility claim.

### Texture realization / storage

- Source `749-764` preserves exact raw filter pairs, wrap mapping and GX LOD constants from `data_manifest.json:853-961`. Filter1 -> `(GX_NEAR,GX_NEAR)`; filter2 -> `(GX_LINEAR,GX_LINEAR)`. U/V words are table lookups from inherited sampler nibbles, not forced clamp modes.
- **Capture sampler is inherited.** The core SaveScreen sets raster and ONE/ZERO blend only (`reconstructed/motion_blur.cpp:335-347`). The draw alone sets filter state9=1 before realization. Open's one-time filter2/U1/V1 initialization at `original/804981E8_RwRenderState_Open.asm:137-155` is not a per-capture reset.
- Source `795-857`: realization stamps parent-plugin `+30` from `805EEFF0`; dirty bit `01000000` triggers object recreation with LOD bias0, clamp1, edgeLOD1, anisotropy0. Sampler-only change first preserves max-anisotropy, edgeLOD, bias-clamp, and bias through real SDK getters at `8049B37C/B388/B394/B3A0`. Unchanged sampler skips both Init calls but still loads the texture.
- `GXInitTexObj` uses **bound raster width/height** (low16), parent-plugin image/format, inherited translated U/V wraps, and mipmap0; `GXInitTexObjLOD` uses table min/mag, minLOD0 and parent maxLOD (0 on scope). Flags then copy sampler low16 while preserving upper16 and clearing only dirty. Region flag `02000000` chooses ordinary callback-based load (`8049B524`) versus preloaded region load (`8049B538`); slot `8056FCDC` stores the last-loaded descriptor (`8049B544`).
- Flush re-reads current binding, tests **that raster's own plugin** `alpha_flags&1`, and sets `beforeTexture = alphaBit ^ 1`. The relevant core calls bind roots, but do not replace that observed direct-plugin access with a parent search. If the cache changes, beforeTexture1 uses `(ALWAYS,0,AND,ALWAYS,0)`; beforeTexture0 reinstalls inherited alpha compare operands, then calls ZCompLoc. This is not state12 support.
- Source `877-895`: `0x505` resolves to GX format6, alpha flag1, depth32, format byte05 (no mipmap). Scratch's empty view is actually `(0,0,0,5)` at `original/80433638_EffectManager_CreateScratch.asm:29-46`; its default depth0/type5 arm resolves format2/GX4/depth16/alpha0 before empty allocation handling. Backing creation at that file `47-55` is `0x505`. Only manager+34 scratch is considered; unrelated manager+38 effects are not reconstructed.
- Source `921-972`: zero width or height -> flags21=`80`, success, no pixel allocation. Nonzero 0x505 root -> maxLOD0, one level, RGBA8 tile allocation `round32(4*round4(W)*round4(H))` under u32 wrapping. Allocation is **real indirect** engine+108, arguments `(imageBytes+31,0x30411)`. Base allocation pointer remains plugin+18; aligned image pointer goes +1C. `DCInvalidateRange_80372508` is called, **not memset/clear/upload**. Failure makes both original error reports and returns 0. Generic outer freelist allocation/registry initialization is still the core's existing real-address `RwRaster_Create_8048AEC4` boundary.
- Source `979-988`: SubRaster callback copies stride/depth/type/format prefix fields and zeros pPixels+4 only. It does not copy plugin alpha metadata or backing ownership. Outer wrapper (`original/8048AE14_RwRaster_SubRaster.asm:17-40`) installs offsets/sizes and, after callback success, `view->parent=parent->parent`.
- Source `995-1002`: selected raw standard records `(4,80497CA8)`, `(8,8049819C)`, `(12,804981B8)` are from `data_manifest.json:1057-1120`. Outer Create uses engine+58 at `8048AF58`; SubRaster uses engine+78 at `8048AE8C`. These data/call sites are not a live callback-installation claim. Raster registration at `original/80496440_RwRaster_PluginRegister.asm:7-14` proves extension size34, plugin ID40C, and dynamic offset slot805F2700.

## Persistent fields and unresolved live state

Source `18-153` declares only observed prefixes, with layout assertions. It does not allocate or initialize parallel "default" globals.

| Absolute address | Observed role |
|---|---|
| `805F265C` (`r13+615C`) | Runtime engine/global pointer; current camera+0, typed indirect pixel allocator+108; existing core device table shares this object |
| `805E42D0` | Render-state cache: zwrite+0, ztest+4, zfunc+8; fog+10..30; blend+34/+38; Z-comp location+3C; inherited alpha-test+40..4D |
| `805F2700` (`+6200`) | Registered raster-plugin byte offset, **not** a constant raster object size |
| `805F270C` (`+620C`) | Current texture descriptor pointer; raster+0 and packed sampler+50 |
| `805F2718` (`+6218`) | Registered texture-plugin byte offset; GXTexObj+0, cached sampler/flags+20 |
| `805F2688` (`+6188`) | Independent copy-clear word; low8 reaches GXCopyTex |
| `805F26F4` (`+61F4`) | Render mode: width+4, Im2D height+8, field flag+18, AA+19, sample pattern+1A, taps+32 |
| `805F2698`, `805EEFD8`, `805F26F0` | View/scissor branch selector, split-half selector, and split-height arithmetic source; no invented startup/live values |
| `805F2720` (`+6220`) | Halfword loaded for plugin create stamp |
| `805EEFF0` (`+2AF0`) | Halfword loaded for texture-use stamp; not itself a completion wait |
| `8056F13C`, `8056F158` | Mutable projection/matrix templates, initial raw words preserved as source snapshots |
| `805E4218` | Saved GX projection vector for Im2D restoration; not previous-frame motion data |
| `8056FCDC` | Observed map0 last-loaded texture pointer slot |

No runtime pointer, plugin offset, screen resolution, frame age, sampler, alpha-write policy, TEV swap, FPSCR, cache/register consistency, or allocator success is inferred merely from BSS/static data. Source annotations record exact call sites, but source factoring does not reproduce original compiler prologues, register-save helper calls, scheduling, exception unwinding, or original stack addresses.

## Validation and command log

Only the two owned deliverables were created/edited. No DOL patch, installation, configuration change, core edit, original export rewrite, or runtime GPU capture was performed.

1. Directory/orientation commands: `pwd`, `ls -la`, `ls -la analysis/motion_blur/evidence` in the requested workspace. Read `AGENTS.md`, the core files, manifests and bounded original evidence using read/search tools.
2. Read-only hash-checked `dol_evidence.py data/disasm` commands are enumerated in the additional-evidence section above. `-B` was used on every Python command to prevent bytecode cache creation.
3. Incremental syntax: stdin included core `.cpp` then fragment; CopyEFB-only stage passed, then state/Im2D stage passed with `-Wall -Wextra -Werror`. Full final syntax command below also passed (exit0):

   ```bash
   "/c/Program Files/LLVM/bin/clang++.exe" --target=powerpc-unknown-eabi \
     -std=c++17 -ffreestanding -Wall -Wextra -Werror -fsyntax-only \
     -include analysis/motion_blur/reconstructed/motion_blur.cpp \
     -x c++ analysis/motion_blur/evidence/backend_reconstructed.inc
   ```

   This is parsing/type/layout validation, **not** PPC machine code generation, original ABI validation or linking. Installed clang's machine-backend absence was not worked around or hidden by host compilation.
4. `python -B analysis/motion_blur/tools/dol_evidence.py verify`: PASS for the 81-function snapshot, **7,249 instruction words**, **3,626 data bytes**, target SHA-256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`. It verifies original exports/data, not execution of this C++ fragment.
5. `python -B analysis/motion_blur/tools/test_motion_blur_semantics.py`: all 5 tests PASS, including the existing 90 geometry fixtures, raw constants, absolute position recomputation, and independent clear-word provenance. This suite tests original/static core semantics, not backend GPU output.
6. One-off **stdin-only synthetic backend check** (`python -B - <<'PY' ...`): interpreted the actual CopyEFB offset-arm instructions (`lwz`, `lha`, `addi`, `rlwinm`, `mullw`, `add`, `srawi`) from the DOL and compared them to the readable formulas used here. **1,960 cases PASS**: four depths × ten widths `[0,1,3,4,7,8,640,641,7FFFFFFF,FFFFFFFF]` × seven signed X/Y offsets `[-32768,-17,-1,0,1,7,32767]`. Also verified all eight getter/setter table pairs, the getter12 `li r3,0; blr` and setter initial `li r6,0` words, and exact filter/wrap/min-filter tables. This did not execute the C++ or emulate GPU behavior. The first stdin attempt had a malformed hex-literal typo and failed Python parsing before any analysis; corrected rerun passed and no files were written.

## Parent integration / remaining limits

- Append outside the namespace after core helper definitions, or transplant bodies with the stated namespace/adapter boundaries. Keep path names and scope assertions or otherwise make restricted domains unmistakable.
- Supply verified SDK, mixed-FPR/GPR, varargs error, and allocator ABI/address bindings if anything beyond static syntax is later attempted. Real `GXLoadTexObj` depends on SDK context callbacks and hardware state; replacing it with success is not acceptable.
- Consider manifest additions listed above before upgrading all source paths to export-ledger provenance. Preserve current manifests until the parent owns that change.
- The generic raster allocation/freelist/registry/destruction pipeline, null-texture fallback, other renderer cases, palettes and mipmaps are intentionally not reconstructed here. Camera begin/end and scheduling remain the core's existing external dependencies; they were not reset/rewritten to make this effect "work".
- Existing core state12 call-site names describe a requested RW concept, not an effective backend operation. Keep getter residue and ineffective restore unchanged; clarify comments during integration if needed.
- No runtime validation: visible blending, EFB alpha meaning, successful resources, completion timing and temporal image age remain unproved.

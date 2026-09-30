# Role 04 — independent bottom-up closure

## Scope, method, and source notation
- Started at the actual FIFO-writing routines `803975AC` and `80399D44`, enumerated their machine branches, and followed raster ownership, indirect-call installation, and emitted vertices back to `8042E838` / `8042E254`. Export names are navigation labels, not identity evidence; no other specialist interpretation is required for this proof.
- `O/<entry>` means the unique `analysis/motion_blur/evidence/original/<entry>_*.asm`; its instruction line is `5+(VA-entry)/4`. `D` means `analysis/motion_blur/evidence/data_manifest.json`. `FD` means `motion_blur_analysis/full_disassembly.txt`, read only in bounded ranges; line is `1+(VA-80006840)/4` here. Addresses/offsets are hexadecimal.
- Read-only `python -B analysis/motion_blur/tools/dol_evidence.py verify`: **PASS**, SHA-256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`, 81 function ranges, 7,249 instruction words, 3,626 file-backed data bytes. Fresh manifest has 55 data records. Additional `disasm`, `data`, `xrefs`, and `pointers` commands hash-check the same DOL and write no artifacts.

## 1. Hardware endpoints first, then their callers
- **Copy endpoint:** `80397668..803976E8` emits BP command byte `61` and words to `CC008000`; `80397684` encodes destination `r3>>5` into register `4B`, and `803976C0` puts the low-byte Boolean of argument `r4` into copy-control bit 11 before emitting register `52`. This is actual texture-copy command construction, not a symbol-map guess (`O/803975AC:52-84`).
- Fresh direct-branch enumeration finds exactly one direct caller of `803975AC`: `804963B8`, raw `4BF011F5`, inside `80496208`. At `804963A8..B4`, its destination is `[root+pluginOffset+1C]+computedOffset`; clear is **low8([805F2688])**, not the caller's second argument. `804963BC` immediately calls `80396568` (`4BF001AD`) for pixel-mode synchronization (`O/80496208:109-120`).
- **Blend endpoint:** `80399D44..94` modifies GX context word `+1D0`, inserts source `r4` into bits 8..10 and destination `r5` into bits 5..7, and emits the word through BP/FIFO. `r3=1` enables additive blend, not logic/subtraction (`O/80399D44:5-25`).
- Its ten direct call sites are `8032DA0C`, `80394830`, `804382A4`, `804385D8`, `804982E0`, `804982FC`, `80498940`, **`80498DA0`**, **`80498E10`**, `80499F38`. Only the bold pair is needed to recover the blur's factor-setting path; a shared GX endpoint alone does not identify an effect.
- `80498DA0=4BF00FA5` and `80498E10=4BF00F35` call the blend endpoint from setter `80498954`. Its jump table `8056FAD4` indexes the unmodified state ID: entries `0A -> 80498D38`, `0B -> 80498DC0`. Both paths fetch factors from `8056FA28`, call with `r3=1,r6=0`, and cache the RW values (`O/80498954:14-19,254-309`; `D:94-114,136-176`).
- Raw factor table entries `[5]=4` and `[6]=5` therefore make RW `(source=5,dest=6)` become GX `(SRCALPHA,INVSRCALPHA)`; `[2]=1,[1]=0` instead gives replacement `(ONE,ZERO)`. Cache-hit paths may omit redundant GX calls, not change these intended factors.

## 2. Engine/device installation — the indirect edges are closed
- Startup words `3DA0805E / 61ADC500` establish `r13=805EC500`; hence `r13+615C=805F265C` (`D:579-592`). This is the same engine-global slot loaded absolutely by both core routines.
- Getter `804960F0` is exactly `3C608057;3863F50C;4E800020`, returning initialized descriptor **8056F50C** (`O/804960F0:5-7`). Caller `804872B0=4800EE41` retains it in `r30`.
- At `804872D8..E4`, the prior engine's `+108` allocation hook is called with size `[8056F070]`; `804872EC=906D615C` installs the returned engine pointer. The success path copies `12C` bytes of bootstrap state, then issues this device call (`FD:1180317-1180345`):
```text
80487308 38600004   r3 = 4
8048730C 819E0004   r12 = [descriptor+04] = 80494EDC
80487314 38850010   r4 = engine+10
80487318 38A50108   r5 = engine+108
80487320 4E800421   bctrl
```
- Driver `80494EDC` uses `table[r3]` without an index bias: `slwi/lwzx/mtctr/bctr` at `80494F04..14`. `8056F494[4]=804950A0`; that case calls the getter and copies every descriptor word `+00..34` to the supplied destination, then retains the allocation-hook block pointer (`FD:1194408-1194422,1194521-1194553`; `D:1123-1155`).
- The initialized bytes and explicit copy establish these exact live-slot assignments on that installation path (`D:488-511`):

| Descriptor word | Raw target | Installed engine slot | Effect use |
|---|---|---|---|
| `8056F51C` (`+10`) | `80498954` | `+20` | render-state setter |
| `8056F520` (`+14`) | `804984C4` | `+24` | render-state getter |
| `8056F52C` (`+20`) | `80491E80` | `+30` | primitive emitter |

- **Correction to the requested “op9” lead:** the standard-table call is **operation `0B` (11), not 9**. After driver operation 0 succeeds, `804873A4=3860000B`, `804873B0=38840048`, `804873B4=38C0001D`, and `804873BC=4E800421` pass `(r3=0B,r4=engine+48,r5=0,r6=1D)` through the same `descriptor+04` (`FD:1180375-1180384`).
- `8056F494[0B]=804954F8`. Index 9 instead contains `804954F0`, merely `li r3,1; b 80495620`. No missing op9 registration edge remains (`D:1129-1143`; `FD:1194797-1194798`).
- `804954F8..5520` copies 27 `(index,target)` records from **8051D400** onto its stack. `80495534..55B4` initializes the supplied 29 slots with `80494CCC`; `804955D4..55E0` loads each actual pair, shifts its index by 2, and performs `stwx target,r31,index*4`, with `r31=engine+48`. The reverse loop installs all 27 records (`FD:1194799-1194863`).
- Exact-machine caveat, not an open edge: `804955C0` tests the *first* copied index `[sp+88]=1`, not the current record index. For this call `r6=29`, the guard passes and every actual table index is in range; do not rewrite this as a general per-record bounds check.

| Raw record pair at 8051D400 + offset | Installed slot | Machine consumer |
|---|---|---|
| `+00: 1,804956F4` | `engine+4C` | `804863D4/DC` camera begin indirect call |
| `+08: A,80495CAC` | `engine+70` | `80486360/6C` camera end indirect call |
| `+48: 4,80497CA8` | `engine+58` | `8048AF24/58` raster creation |
| `+40: 5,80498058` | `engine+5C` | raster destruction backend assignment |
| `+B0: C,804981B8` | `engine+78` | `8048AE84/8C` subraster attachment |
| `+58: 8,8049819C` | `engine+68` | standard raster-binding assignment |

- These are literal pairs, not API-name matching (`D:1057-1119`; `O/804863A4:13-19`, `O/80486340:10-16`, `O/8048AEC4:25-42`, `O/8048AE14:32-40`).
- Core calls `8048652C/80486504`; their exact adapters load camera `+18/+1C` and `bctrl` at `80486540/80486518` (`O/8048652C:8-10`, `O/80486504:8-10`). Camera construction writes `804863A4/80486340` into those slots (`80486B08..3C`, `O/80486AA8:29-42`). World-plugin construction saves them in extension `+10/+14`, installs wrappers `80461B6C/80461BB8`, and those wrappers call the saved slots (`O/80461C98:13-25`, `O/80461B6C:17-19`, `O/80461BB8:13-15`). Thus the normal wrapper chain reaches the registered standard functions; it is not a guessed direct camera-to-GX call.

## 3. Independent convergence on the two core routines
```text
copy:  803975AC <- 804963B8 in 80496208
                     <- 8042E8C0 / 8042E968 in 8042E838
blend: 80399D44 <- 80498DA0 / 80498E10 in 80498954
                     <- installed engine+20, used by 8042E254 and 8042E838
draw:  FIFO <- 80491E80 <- installed engine+30
                     <- 8042E738 (vertices object+14, primitive 4, count 4)
                     <- 8042E954 / 8042E99C (object+7C / +DC, 4, 4)
```
- At `8042E654/674`, `8042E254` sets IDs `0A/0B` to **5/6**. At `8042E910/930`, `8042E838` sets the same IDs to **2/1**. These independently distinguish temporal compositing from the capture routine's replacement draws (`O/8042E254:254-269,310-318`; `O/8042E838:52-76`).
- Direct core callers are `8042F158 -> 8042E838` (`4BFFF6E1`) and `8042F184 -> 8042E254` (`4BFFF0D1`); both first load manager `+40` and test it. Upstream `801D2774 -> 8042F140` and `801D280C -> 8042F16C` independently connect the two task execs to that same object (`O/8042F140:8-11`, `O/8042F16C:8-11`, `O/801D2764:8-9`, `O/801D27FC:8-9`).
- Only after establishing that graph: task constructors install vtables `8053DC5C/8053DC78` and literal names **804D0F24 = SaveScreen** / **804D0F18 = DrawBlur**. Their vtable `+0C` words are `801D2764/801D27FC` (`O/801D20D8:15-21`, `O/801D2058:15-21`; `D:3-52`). This supplies local naming evidence for the machine-discovered pair.

## 4. Buffer, geometry, and state agreement
- `8042E838` copies first into the root of `[object+10]->manager+34` (`8042E8AC..C0`), then into **object+74** (`8042E960..968`). `8042E254` later loads **[object+74]->root**, binds it via state 1, and submits `object+14` vertices (`8042E604..634`). The capture destination and subsequent sampled allocation therefore agree by pointer provenance, not naming.
- Creation `8042F00C/F024/F034` makes an empty raster view, a root with half camera-root width/height, and attaches the view to it; both creation flags are `505`. The half dimensions come from `srawi;addze` at `8042EE44..58`, i.e. signed division by two toward zero. Manager scratch creation separately allocates a full-camera-sized `505` root and attaches manager `+34` (`O/8042EA50:256-263,354-382`; `O/80433638:26-55`). This is not a two-buffer temporal ping-pong.
- `505 -> type 5, format 500 -> GX format 6 (RGBA8), depth 32`: explicit branch `8049768C/90` reaches `80497884..98`, storing 6 at root-plugin `+0C`, 1 at `+14`, and 32 at raster `+14` (`O/80497458:146-147,259-278`). Backend allocation retains its raw allocation at plugin `+18` and a 32-byte-aligned pixel pointer at `+1C` (`80497F48..94`, `O/80497CA8:173-192`).
- Subraster attachment writes the parent's root pointer to view `+00` (`8048AE98..A0`). Copy consumes that root's plugin `+1C`; binding `8049819C` puts the same root in the current texture descriptor and marks it dirty. `8049887C -> 8049AC88`, then the nonpaletted path `8049B2EC..F8` passes that same root-plugin `+1C` and format `+0C` to `80397E34` (`O/8048AE14:38-40`, `O/8049819C:5-9`, `O/80498854:10-15`, `O/8049AC88:353-361,403-417`).
- For depth 32, raw dispatch `8056F544[1C]=80496358`. Copy's byte offset is `4*(4*x + alignUp(rootWidth,4)*y)`; source rect uses the view's signed offsets and dimensions, destination dimensions/format use its root (`O/80496208:34-59,89-113`; `D:179-216`). Blur passes second argument **0** at both copy calls: no GX half-scale copy; the intermediate **draw** performs the shrink.
- Successful capture sequence within the configured rectangles: EFB -> camera-sized scratch root; replacement draw of scratch using `object+7C` into a half-sized rectangle; EFB -> half-sized private raster; replacement draw of the still-bound scratch using the full-size `object+DC` block to restore the image. Both capture vertex blocks have four `18`-byte vertices, white/opaque copied colors, and root-covering UV endpoints; constructor rewrites their respective XY extents to half/full size (`O/8042EA50:119-255,264-371`; `O/8042E838:34-98`). Camera-begin failure can skip a draw while copying/setting validity still continues, so restoration is not unconditional; camera-raster, camera-root, and video-mode extents must not be silently equated.
- Draw requires `object[0]!=0`, `object[1]!=0`, a successful provider/camera lookup, and `object[78]!=0`. Save clears `+78` on entry and sets it after reaching `8042E9A8..AC`; no copy-completion result is tested, so that flag is not a runtime-success trace (`O/8042E254:10-34`; `O/8042E838:8-17,97-98`).
- Readable geometry from `8042E3AC..E600`: camera-raster offset `(ox,oy)`, extent `(w,h)`, root extent `(W,H)`, scale `s=[object+4]`; UV endpoints are `((ox+.5)/W,(ox+w+.5)/W)` and `((oy+.5)/H,(oy+h+.5)/H)`. With `dx=(s-1)*w*.5`, `dy=(s-1)*h*.5`, XY is `(-dx,-dy),(-dx,h+dy),(w+dx,-dy),(w+dx,h+dy)`. This is algebraic notation; the original separate single-precision operations remain authoritative (`O/8042E254:91-240`; `D:63-90`).
- Each of those four vertices receives RGB `FF,FF,FF` and alpha **object byte +1**, not a hardcoded blend weight (`8042E52C..590`, `O/8042E254:187-212`). Vertex Z was initialized from `engine+18`; no previous-camera matrix or per-object motion vector enters this geometry block.
- `80491E80` calls prepare at `80491EA4`; table `8056F120[4]=98` makes its `80491EC0 -> 80396934` request a four-vertex triangle strip. The textured path emits XYZ, four color bytes, and UV from each `18`-byte record directly to `CC008000` (`O/80491E80:14-79`; `D:117-133`). It is not a multi-tap velocity shader.
- Prepare explicitly requests one TEV stage, one color channel, unlit vertex material source for channel 0, texture coordinate/map 0 and raster color channel 0; `80491574 -> 803992EC` receives `(stage=0,op=0)` (`O/8049148C:28-78`). The new raw tables close the earlier TEV-data gap: **80568B00=C008F8AF**, **80568B28=C108F2F0** (`D:1258-1286`).
- `803992FC..308` selects exactly those two words; `80399340..68` installs their combiner bits. Color `(A,B,C,D)=(15,8,10,15)` and alpha `(7,4,5,7)` mean `(0,texture,raster,0)`, add, zero bias, scale one, clamp, previous-register destination. Thus combiner RGB is texture*vertex RGB and source alpha is **texture alpha * vertex alpha**; blend uses that source alpha, not necessarily `object[1]/255` alone. The SDK preserves existing swap-selector bits rather than installing them (`O/803992EC:9-12,20-37`).
- Draw saves/restores requested IDs `A,B,C,6,8,E,9`, sets `(5,6,1,0,0,0,1)`, and binds state 1 to the private root. Tables resolve depth-test 6 -> `80498CA4` (disabled means GX compare ALWAYS), depth-write 8 -> `80498C38`, fog E -> `80498990`, filter 9 -> `80498BFC`. Filter 1 selects min/mag `(0,0)` in `8056FC90` (nearest). **State C is unsupported:** getter -> `804986B0` returns 0 without filling output; setter -> `80498F58` has no state effect. Do not claim seven effective states were restored or that state C enables the vertex alpha (`D:136-176,709-749,853-875`; `O/80498954:175-253`; `O/804984C4:128-129`).

## 5. Registered PJS camera provider — not an unresolved virtual slot
- `801E28D0..EC` allocates `10` bytes, conditionally constructs at `801E293C`, then **calls `8042B630` with that object** (`FD:487461-487468`). Constructor's `801E295C=3804E93C;801E2960=901F0000` installs **8053E93C** after the base constructors (`FD:487488-487524`). Setter stores it at **805E25C8**; getter `8042B620` loads exactly that slot (`O/8042B630:5-7`, `O/8042B620:5-8`).
- Raw `D:1370-1427` gives **[8053E93C+24]=[8053E960]=80028730**. Core calls at `8042E2B4` and `8042E8A0` load vtable `+24` only after their cast/null checks. `8002873C=4BFE5585` calls `8000DCC0`; `80028744=80630000` then returns **[returned manager+0]**, the camera, not the manager itself (`FD:34749-34757`).
- The cast also closes for this constructed provider. Hash-checked supplemental raw data: `[8053E93C]=805E9D98`; `[805E9D98..9C]` (file `572698..9C`) is `(804D21B0,8053E920)`. The name is `Effect::PJS::EffectSystemPJS`; its base list at `8053E920` contains `(805E9D88,0)`. `[805E9D88]` (file `572688`) points to **804D2170**, raw string `Effect::RW3::EffectSystemRW3`.
- Core target descriptor `8056BCE4` points to **8051C318**, the byte-identical RW3 type name (`D:820-835,1473-1479`). `803A1AFC` compares type-name bytes, traverses that base list, and adds the listed zero adjustment on a match (`O/803A1AFC:13-46,112-137`). Different descriptor addresses therefore do not leave a failed/unproved pointer-identity cast.
- `8000DCC0` lazily constructs manager **8056FF1C** using `80272598`, caches it at `r13+2B68=805EF068`, and returns that cached pointer (`FD:7457-7483`). This identifies the registered provider's camera source statically; it does not assert a captured live camera pointer or exclude deliberate later replacement of global services/callbacks.

## 6. Separate the other copy sources; bound the conclusion
- All **17** direct calls to shared copier `80496208` partition as follows; the grouping is an exclusion inventory of these calls, not an entire-game effect audit:

| Caller sites | Distinction from the motion-blur pair |
|---|---|
| `8042E8C0,8042E968` | Proven capture routine and private `+74` history raster above. |
| `8042F41C,F630,F7E8,F9C4,FF40; 804305A8,80430810,80430B58,80430FAC` | Separate bloom/glare candidate cluster. At `8042F258`, gate is object `+E1`, raster is selected from `+10+4*[+3C]`, geometry is `+48`, and RGB is intensity-weighted while alpha is `FF` (`FD:1090183-1090308`). “BloomMath” in the reference map is only a hypothesis label; none of these calls is evidence of blur history. |
| `80435FA4` | Shimmer draw, not blur: raw vtable `8056C028+10=8043531C`, RTTI `8056C020 -> 8051C93C` spells `Effect::RW3::EffectShimmerRW3`; its copier call follows distinct nested geometry loops (`80435F70..A4`, `FD:1097165-1097178`). Raw vtable/RTTI/string were independently read from DOL. |
| `80436EEC;80438370,80438460;80438B18;80439124` | Other shared screen-copy clients, not promoted to blur on API similarity. Checked operands include `[[object+4]]`, `[object+70]`, `[object+10]` respectively (`FD:1098143-1098162,1099948-1099965,1100336-1100352`); these are not the discovered capture/draw object's `+74` chain. |

- **Confirmed closure:** actual GX copy and blend callers converge independently on `8042E838/8042E254`; engine `+20/+24/+30`, standard camera/raster slots, and the installed PJS provider `+24` now have explicit instruction-and-table provenance. The old unread-table/registration gaps are closed, including the corrected standard-operation number.
- **Remaining limits, not invented callgraph gaps:** actual frame cadence, active camera, successful allocations/camera begins, captured EFB contents/alpha, and inherited GPU masks/tests/swap selectors require runtime evidence. The copy-clear control is a dynamic global, not implied false by the blur's `r4=0`. This proves last-saved-image screen-space compositing on the shown path, not a universal previous-frame timing guarantee, all possible callback replacements, or the behavior of every other effect.
- Changes: only this report was created. Original DOL, exports, earlier analyses, `GPT_SOL_analysis/`, tools, and configuration were not modified. No export/build/runtime command was run; validation was read-only static evidence checking.

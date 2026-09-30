# Specialist 08 — persistent images, temporal semantics, and ordering

## Verdict and confidence — raw-data continuation

**The normal task tree draws the previous saved image before capturing its successor: this is recursive temporal feedback with an optional centered scale, not a same-frame-only radial filter.** The newly available big-endian word at **`0x804D25D8` is `1`**. Record 11 at `0x804D25C8` names **`RenderLevel_PostGlare`**, via `0x804D23D8`, not the contaminated-map assignment `RenderLevel_GlareWorld`. Both DrawBlur/SaveScreen Exec slots now resolve to the examined image operations. Section 7 re-derives the chronology from constructor attachments, table indexing, tail insertion, and depth-first dispatch; it does not infer chronology from names.

**There is one persistent half-size history backing image.** Hview and Hroot are a subraster and its parent, not alternating histories. The manager's separate full-size scratch Sroot is captured afresh, used for downsampling and nominal EFB restoration, and also borrowed by a neighboring effect. Neither that shared allocation nor the separately allocated SaveScreen task is a second blur history.

- **High static confidence:** normal Draw-before-Save chronology, the two task callbacks, separate enable/alpha/valid state, exact geometry constants, capture/downsample/capture/restore command order, ownership and no ping-pong in the examined family. Parent verification supplies the target-byte/hash correspondence; this continuation independently reread the relevant instructions and raw-data manifest.
- **Not a runtime assertion:** one invocation per VI frame, the active camera/mode, successful resource allocation/camera begins/copies, GPU completion, or an exact bit-for-bit restored EFB. The specific remaining conditional issues are in sections 5, 8, and 9. The old selector/vtable/geometry/hash proof obligations are closed, not pending.

## 0. Full normal timeline first

Here a **cycle** is a normal traversal of the constructed render-task tree, **not a proven VI field/frame or presentation**. Assume fully initialized B/H/S, the usual compatible camera dimensions, stable parameters during the cycle, reached tasks, supported raster copies, and successful camera begins/draws. `E=B[0]`, `A=B[1]`, `V=B[0x78]`; H is the persistent half-size image. Save always resets V on entry when B exists.

| Phase | DrawBlur, earlier in Div / PostGlare | SaveScreen, later in Static / PostEffect | Result / history age |
|---|---|---|---|
| Construct, normal null-parameter path | B is installed at M+0x40; E=0, A=0x80, scale=1.0, V=0 | H storage is allocated, not seeded with a current frame | No valid history; there is no initial image-clear/capture in the constructor |
| Initially disabled cycles | E=0 suppresses drawing | V=0 is written, then E=0 returns before capture | Old/uninitialized pixels are not sampled |
| First enabled cycle, E!=0 and A!=0 | V=0 suppresses history overlay; it does not capture here | V=0; Sroot captures current EFB; scratch is drawn half-size; that rectangle is copied to H; scratch is drawn full-size; V=1 | First usable history is produced only at this later save |
| Steady enabled cycle | Sample H from the preceding qualifying save, alpha-blend one scaled quad into the current view; H and V are unchanged | Capture the EFB **after** the earlier overlay and any intervening rendering, replace H by its downsample, nominally restore EFB from Sroot, set V=1 | H can contain its own earlier contributions: feedback, not an unblurred previous-scene snapshot |
| Disable, or set A=0 | Prefix update alone changes E/A/scale, not H or V; the next Draw skips | The next reached Save first clears V, then returns without changing H pixels | Invalidation occurs at Save, not at the setter; stored pixels are not erased |
| Re-enable after such a disabled/zero-alpha Save | V=0: again no overlay on the first enabled Draw | First fresh capture reseeds H; V=1 | Overlay resumes on the following cycle |
| Disable/re-enable entirely between Save calls | If V was still 1, the next enabled Draw may reuse the old H immediately | A later normal Save replaces H | No forced one-cycle warm-up unless a Save actually invalidated V |
| Destroy / recreate | Stage marks tasks; `0x8042F198` destroys Hroot/Hview, frees B, and nulls M+0x40; subsequent manager draw/save wrappers return for null B | Shared scratch is destroyed later by its manager owner, not by B | A normal later construction starts invalid again; no history is transferred |

If Save is **not invoked at all**, it cannot clear V; history can be older than one cycle or presentation. Camera-begin/copy failures do not obey the successful-image rows above even when V ends up 1. See section 5 rather than treating V as a success or completion fence.

## 1. Evidence provenance and limitations

Workspace: `<historical-game-root>/sys`.

`AGENTS.md:1-12` was reread. The target is 5,773,024 bytes, SHA-256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`; `motion_blur_analysis/inventory.json:2-4` and `analysis/motion_blur/evidence/binary_baseline.json:2-6` agree. **The parent reports that the original bytes/hash and exported evidence have now been verified. I did not execute Python, calculate a new hash, or run the export verifier in this continuation.** Target/artifact correspondence is parent-verified provenance, not a remaining request and not an independently executed command attributed to me.

Preserved provenance of the original report: `python -B motion_blur_analysis/ppc_tools.py pointers 0x804D1058` and `python -B motion_blur_analysis/ppc_tools.py disasm 0x801D1B00 0x801D1F00` were attempted in the earlier background session and automatically denied. That session used bounded reads of existing disassembly and searches of the smaller branch/string inventories instead. **Those denied probes were not retried.** Their original conditional conclusion is superseded by the newly readable raw data, not retroactively represented as a successful probe.

This continuation read **DM = `analysis/motion_blur/evidence/data_manifest.json:1-670`**, including raw hex and big-endian words, and bounded portions of the original report's attachments. Key new closures are DM:23-53 (task vtables), 63-92 (geometry constants), 220-486 (actual render table and names), and 488-512 (initialized backend descriptor). No original/prior/Sol artifact, DOL, script, configuration, or other specialist report was modified; no subagent was used.

Below, **D** means `motion_blur_analysis/full_disassembly.txt`; **B** means `motion_blur_analysis/branches.tsv`. These are pre-existing read-only attachments. Every additional disassembly read was bounded, using **`L = 1 + (VA - 0x80006840)/4`** for the examined text1 ranges, and checking the returned printed VA rather than accepting a calculated line blindly. Examples independently reread: D:489652 prints `0x801E4B0C`, D:1089158 prints `0x8042E254`, D:1089535 prints `0x8042E838`, D:1195635 prints `0x80496208`. This is equivalent to the original report's line formula. The section mapping in `inventory.json:9-21` implies text1 DOL file offset `VA - 0x80006740`: `0x8042E254 -> 0x00427B14`, `0x8042E838 -> 0x004280F8`. No whole-game scan or oversized-file grep was performed.

The reference-map hazards are concrete: its `0x801D1CB8` blur-constructor hypothesis is not the local constructor, and its `GXCopyTex` label at `0x80396568` is not the local copy implementation. Local `0x80396568` emits a cached BP word; the actual copy submission traced here is `0x803975AC`.

## 2. Locally identified entry points and owners

| Local code/data | Identification supported by examined instructions | Source |
|---|---|---|
| `0x800A35D8` | Get manager; cached pointer at `r13+0x2DD8` | D:160615-160627 |
| `0x800A360C`, `0x800A36CC` | Static-manager creation/constructor; manager body **M = `0x8057798C`** | D:160628-160693 |
| `0x8042F20C` | Allocate `0x13C` bytes, construct B, store B at `M+0x40` | D:1090164-1090182 |
| `0x8042EA50` | Blur-state constructor, including three vertex arrays and the half-size raster | D:1089669-1090065 |
| `0x8042E254` | Composite B's saved image into the current camera | D:1089158-1089534 |
| `0x8042E838` | Save current EFB through full-size scratch and half-size history | D:1089535-1089633 |
| `0x8042F16C`, `0x8042F140` | Manager wrappers for composite and save, respectively; both load `M+0x40` | D:1090113-1090134 |
| `0x8042F198` | Destroy history parent/subraster and B; null `M+0x40` | D:1090135-1090163 |
| `0x801D2058` | Task constructor stores `DrawBlur` string `0x804D0F18`, vtable base `0x8053DC78`; allocates separate SaveScreen task | D:470535-470566 |
| `0x801D20D8` | Task constructor stores `SaveScreen` string `0x804D0F24`, vtable base `0x8053DC5C` | D:470567-470591 |
| `0x801D27FC` / call `0x801D280C` | DrawBlur Exec, raw vtable +0x0C confirmed; calls manager composite wrapper | D:471024-471032; B:62663; DM:39-53 |
| `0x801D2764` / call `0x801D2774` | SaveScreen Exec, raw vtable +0x0C confirmed; calls manager save wrapper | D:470986-470994; B:62659; DM:23-37 |

The task constructor string identities are not map guesses: `strings.tsv:19491-19492` contains those local strings, and `strings.tsv:19502-19503` contains the corresponding full class-name strings at `0x804D1034` / `0x804D1058`. **The vtable bases and the individual Exec/destructor slots are now closed facts** (section 7). No inference from absence of direct branches is needed for their callback identity.

Startup fixes `r2 = 0x805FA780`, `r13 = 0x805EC500` at `0x8000332C-0x80003338` (D:140-143). Thus the manager cache is at `0x805EF2D8`, the concrete blur pointer slot is **`0x805779CC`**, the scratch-view slot is **`0x805779C0`**, and the second manager raster-view slot is `0x805779C4`.

## 3. Persistent state: exact read/write inventory in this implementation family

Let `B = *(M+0x40)`. This is a heap object, not a fixed-address framebuffer.

| B offset | Meaning | Observed writes | Observed reads/use |
|---|---|---|---|
| `+0x00`, byte | Requested enable, not history validity | Constructor `0x8042EA6C`; default `0x8042F058`; prefix-copy routines below | Composite gate `0x8042E268`; save gate `0x8042E854` |
| `+0x01`, byte | Strength/alpha | Constructor `0x8042EA78`; default `0x8042F064` writes `0x80`; prefix copies | Composite gate `0x8042E274`; save gate `0x8042E860`; copied to four vertex alphas at `0x8042E538-0x8042E590` |
| `+0x04`, float | Centered screen-quad scale, enlargement only if >1; not an accumulation counter | Constructor `0x8042EA7C` and normal default `0x8042F068` both write **1.0** (DM:63-92); prefix copies | `0x8042E598`, arithmetic `0x8042E5A4-0x8042E5E4`; no range/finite-value clamp |
| `+0x08`, `+0x0C`, words | Extra parameter payload; not consumed by the examined draw/save math | Prefix copies | Prefix-export copies only within this family; do not label as previous camera or motion vectors |
| `+0x10`, pointer | Back-pointer to M | `0x8042EA80` | Save `0x8042E8AC`, then `M+0x34` |
| `+0x14..+0x73` | Four 24-byte vertices for blur composition | Initial z/color setup `0x8042EAF4-0x8042EB88`; UV/color/xy rewritten `0x8042E4F4-0x8042E600` | Draw pointer `0x8042E71C`; constructor clones its initial vertex properties into other arrays |
| `+0x74`, pointer | Owned history subraster Hview, whose `[+0]` is Hroot | Null `0x8042EB8C`; assign `0x8042F010`; null after teardown `0x8042F1E4` | Composite `0x8042E604-0x8042E608` selects Hroot; save `0x8042E960` selects Hview; teardown `0x8042F1BC-0x8042F1DC` |
| `+0x78`, byte | Capture-valid latch | Initialize zero `0x8042EB90`; **clear at every save entry** `0x8042E850`; set one `0x8042E9AC` | Composite prerequisite `0x8042E2C0-0x8042E2C8` |
| `+0x7C..+0xDB` | Four vertices for half-size scratch-to-EFB draw | Constructor `0x8042EE5C-0x8042F008` | Save primitive pointer `0x8042E938` |
| `+0xDC..+0x13B` | Four vertices for full-size scratch-to-EFB restoration | Constructor `0x8042EC18-0x8042EE38` | Save primitive pointer `0x8042E980` |

The source for these rows is D:1089158-1090065 and D:1090135-1090182. No toggle/index selecting a second Hroot occurs in these routines. There is no per-frame allocation, time-step argument used by the image routines, previous-matrix array, velocity field, or loop over multiple radial samples in the examined implementation.

### Parameter updates do not reset image validity

`0x8042E9C4`, `0x8042E9F0`, and null-guarded `0x8042EA1C` copy exactly bytes `+0,+1`, float `+4`, and words `+8,+0xC`; they do **not** copy or reset `+0x74/+0x78` (D:1089634-1089668). The manager interfaces are:

- `0x8042F084(out, M)`: exports the prefix from `M+0x40`; when B is null, writes enable/alpha zero and a default float (the two extra output words come from stack locations, not history state).
- `0x8042F0E8(M, in)`: imports the prefix through `0x8042E9F0` if B exists.
- `0x8042F114(M, in)`: imports through `0x8042EA1C` if B exists.

Source: D:1090066-1090112. All direct incoming branch sites found in B for these wrappers are:

- `F114`: `0x800A356C`, `0x800A375C`, `0x800A481C` (B:20745,20758,20890).
- `F084`: `0x80204720`, `0x802055E8`, `0x80206320`, `0x8043A908` (B:69553,69734,69944,138965).
- `F0E8`: `0x8020475C`, `0x80205624`, `0x8020635C`, `0x80439FD8`, `0x8043A544` (B:69555,69736,69946,138929,138942).

The first two F114 callers explicitly distinguish enable from strength: `0x800A3528-0x800A356C` submits enable `0`, alpha `0x80`; `0x800A3714-0x800A375C` submits enable `1`, alpha `0x80`, with the same zoom constant at `0x805F3BD0` (D:160571-160588,160694-160712). `0x800A47B0-0x800A481C` derives enable from a float comparison and separately converts a scaled float to the alpha byte (D:161757-161784); `0x800A48C4` calls it after updating that scalar (D:161794-161826). Their broader gameplay roles were intentionally not audited.

**Result:** disabling through the prefix setter does not immediately erase H or clear the valid latch. Draw is suppressed by enable/alpha. The next invocation of SaveScreen clears validity even if the enable/alpha test then returns. A disable/re-enable entirely between saves can retain an older valid H. The same rules apply to zero-alpha/re-nonzero-alpha transitions. A strength-only or scale-only change can immediately reuse existing H; no camera identity, timestamp, or reset counter accompanies it. `0x8042F114(M,null)` is a no-op, not a reset; `0x8042F0E8` assumes its input pointer is valid when B exists.

A skipped **Draw** never invalidates H; a skipped **Save task** never reaches the invalidating write. With B null, the manager draw/save wrappers at `0x8042F140/0x8042F16C` return without dereferencing B (D:1090113-1090134). Constructor initialization, per-Save invalidation, and stage destruction are different mechanisms and must not be collapsed into an imagined setter-side history reset.

## 4. Ownership, allocation, and destruction

### Object graph

```text
static M @ 0x8057798C
  +0x40 -> B, heap allocation 0x13C
             +0x10 ------------------------> M (borrowed back-pointer)
             +0x74 -> Hview (owned raster object)
                         +0 -> Hroot (owned backing raster object)
                                  +0 -> Hroot (root self-parent)
  +0x34 -> Sview (manager-owned full-size scratch view)
             +0 -> Sroot (manager-owned full-size scratch storage)
  +0x38 -> another manager raster/view pair; NOT selected by blur draw/save

DrawBlur task, allocation 0x30
  +0x28 -> SaveScreen task, allocation 0x28
             This pointer is a TASK, not an image or a ping-pong buffer.
```

H construction:

1. `0x8042F224-0x8042F240` allocates B and installs it in M.
2. The constructor obtains the current camera through the effect-system pointer `0x805E25C8` (`0x8042B620`) and virtual slot `+0x24`, after RTTI processing using local descriptors `0x8056BCE4` / `0x8056BCD0` at `0x8042EA84-0x8042EAC0`.
3. `0x8042EB94-0x8042EBAC` obtains the camera raster's root dimensions W,H. `0x8042EE3C-0x8042EE58` halves them with signed division rounding toward zero. For positive dimensions, the request is floor(W/2), floor(H/2).
4. `0x8042F00C` calls raster-create `0x8048AEC4` for a zero-size raster object with flags `0x505`, stores it at `B+0x74`; `0x8042F024` creates its parent using the half-size dimensions, depth argument `0`, flags `0x505`; `0x8042F034` attaches the first to the second through `0x8048AE14` and a rectangle.
5. The root relation is instruction-proven, not inferred from the number of allocations. Raster-create sets `[raster+0] = raster` at `0x8048AF48` (D:1184195). Subraster attachment copies the parent's root pointer into the child at `0x8048AE98-0x8048AEA0` (D:1184151-1184153), after assigning its rectangle at `0x8048AE44-0x8048AE7C`.

There is **one history backing store**. A wrapper plus its parent does not supply two independent images. Hardware pixel format, allocation padding, and exact heap/GPU addresses need platform/raw-data or runtime verification; flags `0x505` and the requested dimensions are the facts established here.

S construction is separate and precedes H setup: `0x801D1A68 -> 0x80433638` runs before `0x801D1A80 -> 0x8042F20C` (D:470154-470161). `0x8043368C-0x80433700` obtains the **camera raster's own** width/height, creates Sview/Sroot, and attaches them; unlike H construction it does not first follow the camera raster's root pointer. Normal full-screen setup makes these the full-image dimensions. Equality with H's source-root dimensions is therefore a normal-camera precondition, not a universal guarantee for arbitrary subcamera construction. Sview is installed at M+0x34 (`0x804336DC`); another pair at M+0x38 (`0x80433718`) is not selected by either blur image routine. Source: D:1094527-1094597.

**Shared-scratch cross-check, bounded to its direct co-owner:** the neighboring object G at M+0x3C is constructed between S and B (`0x801D1A74 -> 0x80431A58`, D:470155-470161,1092743-1092761). Its constructor borrows `M+0x34 -> G+8` at `0x804316C0-0x804316D0` and `M+0x38 -> G+0xC` at `0x8043174C-0x8043175C` (D:1092513-1092517,1092548-1092552). Its use path reattaches those views to the current view rectangle (`0x80430E3C-0x80430E54`, D:1091968-1091974), and another path actually copies EFB into G+8/Sview (`0x80430B50-0x80430B58`, D:1091781-1091783). Thus S's content cannot be treated as persistent blur history, nor can the second manager pair be counted as a second H.

Blur Save explicitly follows Sview to **Sroot** before copying and binding it (`0x8042E8B4-0x8042E8C0`); it does not use a possibly changed Sview rectangle as its capture region. Its qualifying save overwrites Sroot before either of its two scratch-sampling draws. G has private-raster fallback branches if the manager slots are absent, but **B's Save has no analogous fallback or null check for M+0x34**. G's destructor destroys its owned `+0x20` array / `+0x44` raster, not the borrowed `+8/+0xC` views (D:1092694-1092742). The normal stage tears G down at `0x801D19B0`, B at `0x801D19B8`, and shared S last at `0x801D19D8` (D:470108-470119). No unrelated effect-family audit is implied.

H teardown `0x8042F198` loads Hview; if its root differs from the view, destroys root first (`0x8042F1C8-0x8042F1D4`), destroys view (`0x8042F1D8-0x8042F1DC`), nulls B's pointer, frees B, and nulls `M+0x40` at `0x8042F1F4`. Sole direct caller found: `0x801D19B8` (B:62534).

Stage teardown then destroys the manager scratch pairs via `0x801D19D8 -> 0x804335C8`, after H teardown, so B does not own or free Sroot. `0x804335E8-0x804335FC` destroys Sroot/Sview and clears `M+0x34`; `0x8043360C-0x80433620` handles the second pair (D:1094499-1094526).

Task teardown is different again: `0x801D2788-0x801D27F8` sets the stored SaveScreen task's task-flags mask `0x0001` at `0x801D27BC-0x801D27C4`, then runs the base task destructor (D:470995-471023). The generic child walker handles marked-task deletion at `0x8004ECE8-0x8004ED58` (D:74027-74055). No image allocation is freed by treating task `+0x28` as a texture.

## 5. SaveScreen is capture, downsample, second capture, restore

This ordering is directly established inside `0x8042E838`, independently of its position within a frame:

| Step | Instructions | Ownership / image state |
|---|---|---|
| 1 | `0x8042E844-0x8042E850` | `B.valid = 0` before any enable, alpha, context, or camera test |
| 2 | `0x8042E854-0x8042E8A8` | Require enable, nonzero alpha, effect-system cast, and camera; otherwise return with invalid history |
| 3 | `0x8042E8AC-0x8042E8C0` | Load `M=*(B+0x10)`, `Sview=*(M+0x34)`, `Sroot=*(Sview+0)`; call `0x80496208(Sroot, 0)`: EFB -> full-size Sroot |
| 4 | `0x8042E8C8-0x8042E95C` | Begin camera; bind Sroot using interface state `1`; set states `0xA=2`, `0xB=1`; submit four vertices at `B+0x7C`; end camera. The supplied backend/table lowers these to GX ONE/ZERO replacement (section 6); this is a half-size draw of the full scratch image into EFB |
| 5 | `0x8042E960-0x8042E968` | Call `0x80496208(Hview, 0)`: the just-rendered half-size EFB rectangle -> persistent Hroot |
| 6 | `0x8042E96C-0x8042E9A4` | Begin camera; submit four vertices at `B+0xDC`; end camera. No new texture bind intervenes: still sample Sroot, restoring the full-size EFB, not drawing the new Hroot |
| 7 | `0x8042E9A8-0x8042E9AC` | Set `B.valid = 1` after the command sequence |

Source: D:1089535-1089633. The two draws are fixed-size image-management passes, not an iteration over increasingly enlarged radial samples. Both calls to the copy helper receive second argument **zero**. Halving is implemented by the intermediate quad, not requested through the copy helper's optional source-size doubling path.

### The helper really emits an EFB-to-RAM texture copy

At `0x80496208` (D:1195635-1195764):

- `0x80496238`: obtains root from `[raster+0]`; `0x80496240-0x80496244`: adds the platform plugin offset stored at **`0x805F2700`** (`r13+0x6200`).
- `0x8049627C-0x8049629C`: for argument zero, uses the passed raster's x,y at `+0x1C/+0x1E` and width,height at `+0x0C/+0x10`, calls `0x80396C50`.
- `0x804962A0-0x804962B8`: uses ROOT dimensions and plugin format for `0x80396D00`.
- `0x804963A8-0x804963B8`: takes the plugin pixel pointer at `+0x1C`, adds the tile/rectangle-dependent offset, and calls **`0x803975AC`**. Its second argument comes from global `0x805F2688`, not from SaveScreen's second argument. Thus do not infer the EFB-clear policy from the literal zero at the SaveScreen call site.
- `0x80396C50` constructs BP source registers `0x49/0x4A` (D:934149-934179). `0x803975AC` emits source/stride/destination (`0x4B`) and copy-execute (`0x52`) BP commands to `0xCC008000`; see especially `0x80397668-0x803976E8` (D:934795-934827).
- `0x804963BC` calls local `0x80396568`, which emits the cached BP pixel-format/control word (D:933707-933715). `0x804963D8-0x804963EC` then chooses region invalidation `0x8039868C` or all-texture invalidation `0x803987B8`. The latter emits `0x66001000` / `0x66001100` at `0x803987D8-0x803987E8` (D:935903-935920).

This is not CPU pixel accumulation. Sampling and overwriting the single Hroot are scheduled as GPU commands. A CPU-side wait for completed history generation is not visible in this helper; validity is a software scheduling latch, not proof of GPU completion.

### Independently checked failure and lifetime paths

| Condition | Actual path / consequence | Evidence |
|---|---|---|
| B allocation returns null | M+0x40 receives null; manager Draw/Save wrappers do nothing | `0x8042F22C-0x8042F240`, `0x8042F14C-0x8042F184`; D:1090116-1090130,1090172-1090177 |
| Constructor effect-system cast or camera is null | Returns the allocated B with enable=0, alpha=0, scale=1.0, back-pointer written, **before initializing Hview or valid**. Heap bytes are not proven zero; later enable/destruction is not a safely handled partial-initialization path | `0x8042EA6C-0x8042EAD0`, before `0x8042EB8C/90`; D:1089676-1089701,1089748-1089749 |
| Raster allocation or subraster attachment fails | H/S constructors do not check their returned raster/attachment results. Raster-create itself can return null; attachment can return zero. No rollback, safe H fallback, or error-validity protocol is supplied by B | D:1090036-1090046,1094567-1094592,1184125-1184155,1184180-1184218 |
| Draw enable/alpha/context/camera/valid gate fails | No copy, overlay, or valid reset | `0x8042E268-0x8042E2C8`; D:1089163-1089187 |
| Draw camera begin fails | Return at `0x8042E614`; history/valid remain unchanged. Its state setters and primitive are not reached | D:1089394-1089398 |
| Reached Save has E=0, A=0, null cast, or null camera | It has **already written valid=0**, then returns before capture | `0x8042E850-0x8042E8A8`; D:1089541-1089563 |
| Missing scratch/history after the guards | Save dereferences Sview/root without a null check; copy helper dereferences its raster/root. Valid is not an allocation-safety check | D:1089564-1089569,1089609-1089611,1195643-1195650 |
| First Save camera begin fails | Branch straight to `0x8042E960`: no half-size draw **and no Sroot bind / ONE-ZERO setup**, but H capture still occurs. If the second begin succeeds, its purported restore uses inherited texture/blend state, not a guaranteed Sroot restore | `0x8042E8CC-0x8042E8D0`, `0x8042E960-0x8042E99C`; D:1089572-1089573,1089609-1089624 |
| Second Save camera begin fails | Skip full-size restore but still set valid=1 | `0x8042E974-0x8042E978 -> 0x8042E9A8`; D:1089614-1089628 |
| Copy helper sees unsupported root depth | Only depths 4/8/16/32 select the four offset/copy paths. Other depths reach `0x80496380`, report an error, and return without the copy submission; Save does not test a result and may still set valid=1 | DM:179-218; D:1195680-1195743 |
| State/primitive/end callback fails or GPU work is not complete | Save ignores those return values and sets the latch after command submission; it does not prove completed, correctly rendered pixels | D:1089574-1089628 |
| Repeated setup without matching destruction | `0x8042F20C` installs a newly allocated B without freeing/guarding an old M+0x40; scratch setup likewise assumes its normal lifetime. Do not invent an idempotent reset/reallocation path | D:1090164-1090182,1094527-1094597 |

Consequently **valid=1 means the guarded save body reached its final store, not that both begins, all copies, restoration, or GPU execution succeeded**. It does not distinguish a bad/partial capture from a good one. The fully initialized successful path, not these exceptions, underlies the normal timeline.

The allocation Sroot persists across cycles, but the content needed by blur is live only from its fresh copy at `0x8042E8C0` through the full-size scratch draw at `0x8042E99C`. The task walker is sequential; no other sibling task is invoked by B between those operations. GPU ordering/cache behavior and callback reentrancy are not runtime-tested. Copy-helper texture invalidation is present; no CPU wait proving completion or drain-before-destruction appears in the examined B routines. The final draw is a **nominal restoration from a captured texture**, not proof of byte-identical EFB color/depth/alpha preservation after format conversion, sampling, or a copy-clear operation. Save also does not save/restore all inherited render states; it leaves its texture/blend setup on the normal path.

## 6. Composite uses the saved image, not an immediate new capture

`0x8042E254` does **not** call the copy helper, change Hroot, or invoke SaveScreen. It returns unless enable, alpha, camera, and `B+0x78` are nonzero (`0x8042E268-0x8042E2C8`). It obtains the current camera/view rectangle and computes normalized UVs against the full camera-raster root (`0x8042E3AC-0x8042E4F0`). These UVs select the corresponding portion of the common saved image.

`0x8042E52C-0x8042E590` writes RGB=255 and B's byte `+1` into all four vertex alphas. DM:63-92 supplies **`u32(0x8051C388)=0x3F000000` (0.5), `u32(0x8051C38C)=0x3F800000` (1.0), `u32(0x8051C3A0)=0` (0.0)**, plus the signed/unsigned integer-conversion biases at `0x8051C390/398`. Thus the scale arithmetic is no longer conditional on unread constants:

```text
s = B.scale; w,h = current camera-raster/view dimensions
W,H = current camera-raster root dimensions; x,y = view offsets
u0=(x+0.5)/W; u1=(x+w+0.5)/W
v0=(y+0.5)/H; v1=(y+h+0.5)/H
dx=(s-1.0)*w*0.5; dy=(s-1.0)*h*0.5
quad corners: (-dx,-dy), (-dx,h+dy), (w+dx,-dy), (w+dx,h+dy)
```

These are the CPU floating-point operations, subject to their single-precision rounding, not an invented multi-tap filter. View offsets come directly from raster `+0x1C/+0x1E` through `0x8048AB7C` (D:1183952-1183956); UV/xy instructions are D:1089244-1089393. Scale=1.0 is an unexpanded overlay; >1 expands, <1 shrinks, and no input range/finite-value clamp occurs here. H dimensions and both scratch quads were fixed at construction; only the composite's current-view geometry is recomputed.

`0x8042E604-0x8042E634` selects Hroot and binds state 1. It requests `0xA=5`, `0xB=6`, `0xC=1`, and exactly one primitive `(type=4, vertices=B+0x14, count=4)` at `0x8042E738`. Seven previously queried states (`0xA,0xB,0xC,6,8,0xE,9`) are restored afterward, **not the texture binding**. The saved image and valid latch remain untouched (D:1089394-1089534).

### Backend constants and callbacks: newly closed static decoding

DM:488-512 contains the initialized device descriptor at `0x8056F50C`. In its engine+0x10 layout it supplies engine `+0x20 -> 0x80498954` (state-set), `+0x24 -> 0x804984C4` (state-get), and `+0x30 -> 0x80491E80` (Im2D primitive). These exact initialized pointers are available; they are not an outstanding raw-data request. The live engine pointer at `0x805F265C` is BSS/runtime-owned (DM:55-61), so this is static decoding of the supplied backend, not a live callback trace or a claim that this specialist executed engine initialization.

- The setter dispatches through `0x8056FAD4` (`0x80498978-0x8049898C`, D:1198159-1198164; DM:136-177). State `0xA` reaches `0x80498D38`, state `0xB` reaches `0x80498DC0`. Both index the **actual** blend table `0x8056FA28` and call `0x80399D44` (D:1198399-1198460).
- DM:94-115 maps values 5/6 to GX factors **4/5 (SRCALPHA/INVSRCALPHA)**, and values 2/1 to **1/0 (ONE/ZERO)**. `0x80399D44` packs those factors into the cached blend BP word and emits it to `0xCC008000` (D:937282-937302). This is stronger than guessing RW enum names from the effect call sites.
- Primitive type 4 indexes `0x8056F120` to **`0x98` (GX triangle strip)** (DM:117-134; D:1191322-1191329). The textured path streams position, all four RGBA bytes, and UV for each 24-byte vertex (D:1191350-1191410). This includes B's per-vertex alpha; there is no per-pixel velocity input.
- **Correction to a merely conventional reading:** state `0xC` maps directly to the setter epilogue `0x80498F58`, returning its initial zero, not to a separate hardware vertex-alpha enable operation (DM:157; D:1198153,1198535-1198540). The caller ignores this return. Alpha arrives through the actual vertex/color and blend paths, not proof inferred from `0xC=1` alone.

For opaque sampled history the conceptual color blend is `(1-a)*current + a*scaled_history`, with `a=vertex_alpha/255`; texture/TEV alpha, EFB format, quantization, and inherited state must be accounted for before claiming an exact pixel equation. Im2D setup selects its textured TEV path when the backend texture slot is nonnull (`0x80491538-0x804915B4`, D:1190719-1190750). No numeric gameplay zoom preset is needed to establish the centered-scale or temporal ordering result.

The **spatial operation is one scaled translucent saved-image overlay**. Section 7, now closed by actual table/vtable data, supplies the independent temporal fact that the normal save follows this overlay. Camera begin/end wrappers still dispatch through live camera fields `+0x18/+0x1C` (D:1179442-1179461); active cameras and callback success are runtime-dependent.

## 7. Task-tree chronological proof — selector and blur task slots closed

The renderer singleton body is **R=`0x80574384`**, established by `0x800461A0-0x80046208` (D:65113-65139). R is not the image manager M. The following proof concerns the normally constructed, un-reparented tree under its standard child traversal, with its tasks reached rather than deleted/skipped.

### 7.1 List insertion and traversal establish execution order

`0x8004F014` stores the supplied parent at task+0x10 and appends the new task to its child list: parent+0x14 remains the first child; that first child's +8 tracks the tail; tail+0x0C points to the appended child; a new tail has next=0 (`0x8004F074-0x8004F0C4`, D:74254-74274). This is **tail append, not front insertion or priority sorting**.

Walker `0x8004ECAC` begins at parent+0x14, dispatches child `vtable+0x0C`, recursively walks that child's children, and then follows child+0x0C (`0x8004ECD4`, `0x8004ED90-0x8004EDB4`; D:74022,74069-74078). Its profiled wrapper dispatches the same Exec slot before returning to the same recursion (D:74114-74117). Therefore the earlier appended root's normal subtree is visited before the later sibling root. Task low-flag bits can skip/delete tasks, and globals influence profiling/paused execution (D:74024-74078); construction order is not a claim that every possible runtime traversal executes every task.

### 7.2 Four roots, then a special first Static child

`0x801E4B0C` creates these roots under the **same task index 5**, in this order. The index lookup itself is `*(manager+4+index*4)` (`0x801EB614-0x801EB620`, D:496502-496505).

| Order | R slot | Directly written name / string address | Constructor span |
|---|---|---|---|
| 1 | +0x5C | RenderAttach_Start / `0x804D26C8` | `0x801E4B28-0x801E4B64` |
| 2 | +0x60 | RenderAttach_Div / `0x804D26DC` | `0x801E4B68-0x801E4BA0` |
| 3 | +0x64 | RenderAttach_Static / `0x804D26F0` | `0x801E4BA4-0x801E4BDC` |
| 4 | +0x68 | RenderAttach_End / `0x804D2704` | `0x801E4BE0-0x801E4C18` |

Source: D:489652-489719; `strings.tsv:19705-19708`. The root constructors themselves call the same tail-append base constructor (D:489883-489900,489996-490013,490015-490032,490045-490062).

Before ordinary level creation, `0x801E4C1C-0x801E4C38` appends **R+0x58 as the first child of R+0x64** (D:489720-489727). Its name word loaded at `0x801E4C40` is `u32(0x804D26A4)=0x804D24D4`, raw table record 22, naming **RenderLevel_PostEffect** (DM:339-343,479-484; `strings.tsv:19703`). Its record's group=-1 is not used to index the four roots; the parent was explicitly supplied.

### 7.3 Recompute record 11 from instructions and actual words

The loop at `0x801E4C68-0x801E4CC8` creates records 0..18, stores their task pointers at R+4*i, and selects a parent from `*(R+0x5C+4*g)`, where `g=u32(T+0x14*i+0x10)` and **T=`0x804D24EC`** (D:489739-489763). DrawBlur's own constructor passes `*(R+0x2C)` as parent (`0x801D2078-0x801D207C`, D:470543-470544); thus i=0x2C/4=11, with no reference-map translation:

```text
record 11 = 0x804D24EC + 11*0x14 = 0x804D25C8
actual words = 804D23D8 00000000 00000000 00000000 00000001
name pointer at 0x804D25C8 = 0x804D23D8 -> RenderLevel_PostGlare
group word   at 0x804D25D8 = 1
selected root = *(R+0x5C+4*1) = *(R+0x60) = Div
```

DM:220-345 contains the 23 raw records; **DM:284-288 is record 11**. The string is independently visible in DM:416-421 and `strings.tsv:19692`. `RenderLevel_GlareWorld` is instead record **9**, R+0x24, pointing to `0x804D23AC` (DM:274-278; `strings.tsv:19690`). Applying that name to DrawBlur's parent was the contaminated-map error.

The remaining groups delimit the capture cut: record 0 -> Start; 1..13 -> Div; 14..17 -> Static; 18 -> End (DM:229-323). Records 19..21 are subsequently appended under R+0x38, the record-14 Sprite node, by a separate loop (`0x801E4DC4-0x801E4E24`, D:489826-489850); they are not extra attachment roots. R+0x58/PostEffect precedes the later-created Static level nodes, regardless of their larger/smaller numeric level labels.

### 7.4 Resolve task identity through actual virtual slots

| Task | Constructor-installed vtable | Actual +0x08 destructor word | Actual +0x0C Exec word | Local Exec call chain |
|---|---|---|---|---|
| DrawBlur | `0x8053DC78` | `u32(0x8053DC80)=0x801D2788` | `u32(0x8053DC84)=0x801D27FC` | `0x801D280C -> 0x8042F16C -> 0x8042E254` |
| SaveScreen | `0x8053DC5C` | `u32(0x8053DC64)=0x801D270C` | `u32(0x8053DC68)=0x801D2764` | `0x801D2774 -> 0x8042F140 -> 0x8042E838` |

Raw data: DM:23-53. Constructor installs: D:470545-470560,470577-470586. Bodies/destructors: D:470964-471032; manager wrappers: D:1090113-1090134. These are **observed words**, not “should be” requests. No direct-branch-to-Exec hypothesis is being used instead of the actual virtual dispatch.

SaveScreen's constructor explicitly attaches it to `*(R+0x58)` at `0x801D20F8-0x801D20FC` (D:470575-470576). DrawBlur creates it at `0x801D20B4` and stores its pointer in task+0x28, **but that pointer does not make it a DrawBlur child**. It is a separate task under the later Static subtree.

### 7.5 Normal chronological slice and its consequence

```text
task index 5: children appended and traversed in this order
  Start (+0x5C)
    level 0 PreRender
  Div (+0x60)
    levels 1..10, including 9 GlareWorld and 10 Glare
    level 11 PostGlare (+0x2C)
      earlier-created task named PostGlare
      DrawBlur -> composite old H                  [no capture here]
    level 12 Sprite3D; level 13 Last
  Static (+0x64)
    first child: PostEffect (+0x58)
      SaveScreen -> capture/downsample/replace H   [after Div]
      later-created task named DrawScreenColor
    levels 14 Sprite (children 19..21), 15 OnFade, 16 Gindows, 17 SpriteLast
  End (+0x68)
    level 18 PostRender
```

This is the relevant ordered slice, not an assertion that no other tasks exist within these lists. PostGlare's constructor attaches to R+0x2C (D:470592-470616) and is invoked at `0x801D1AD4`, before DrawBlur at `0x801D1AF0` (D:470182-470189). SaveScreen is created inside DrawBlur before the later `0x801D1B0C` construction of DrawScreenColor; that constructor also attaches to R+0x58 (D:470196,470510-470529). Tail append proves Save precedes that same cycle's later color task.

**Conclusion:** Div/PostGlare DrawBlur precedes Static/PostEffect SaveScreen in the normal tree. The latter's input is the EFB after the old-history overlay plus intervening rendering, not an unblurred pre-Draw snapshot. The saved cut excludes that traversal's later DrawScreenColor and subsequent Static/End rendering; it is not automatically the final presented image. The formerly open g=2/3 same-frame ordering alternative is **rejected for this construction**. Arbitrary manual callback order, runtime task mutation, or skipped tasks are outside that normal-tree claim, not evidence that its selector remains unknown.

## 8. Feedback, shared storage, and camera / VI limits

### Normal image dependency timeline

The first-frame/steady/disabled/re-enable/destroy transitions are in section 0. Expanding one successful save gives this ordered GPU dependency sequence, not a CPU pixel-processing loop:

```text
earlier Draw: sample old H into current EFB; H itself is unchanged
Save entry:   valid := 0; Sroot's prior contents may belong to another effect
copy 1:       Sroot := EFB at the Save cut                   [0x8042E8C0]
draw 1:       half-size EFB rectangle := resample(Sroot)     [0x8042E954]
copy 2:       same Hroot := that half-size EFB rectangle     [0x8042E968]
draw 2:       nominal full-size EFB redraw from Sroot        [0x8042E99C]
Save exit:    valid := 1                                    [0x8042E9AC]
next Draw:    sample the newly scheduled H, if guards pass
```

Hview/Hroot alias one pixel storage; Sroot is a different allocation. **No history-pointer swap, parity index, second H image, or exchange of scratch/history roles occurs.** The newly written H is not what the restore draw intentionally samples. Other tasks may subsequently overwrite shared S without erasing H. FIFO scheduling plus the copy helper's texture invalidation provides the intended dependency chain; no measured GPU completion time is inferred from the software valid store.

For normal successive qualifying cycles, let C[n] be the EFB before Draw, T_s the centered-scale sampling, P[n] any rendering between Draw and Save, and D_half the actual capture/downsample operation. A suitable semantic recurrence is:

```text
Q[n]   = Composite(C[n], T_s(H[n-1]), B.alpha)
H[n]   = D_half(P[n](Q[n]))
EFBout = nominal full-size sample of Sroot, then later rendering
```

For an opaque source, the idealized color part of Composite is `(1-a)*C + a*T_s(H)` as qualified in section 6. P[n] need not be identity; full overwrites or failures can remove earlier contributions, so “feedback” describes the established dependency/implementation, not a guarantee of visible trails for every pixel and setting. The normal image saved at n is **not restricted to the unblurred C[n]**. Radial-looking trails can arise from repeated centered expansion across cycles, while there is only one history overlay per reached DrawBlur call and no intra-call radial-sample loop.

### Per-camera storage and size limits

Both task Exec bodies call the same manager getter and then M+0x40; B owns just one H pointer and one valid byte. Draw and Save separately query the **then-current** effect-system camera through virtual +0x24. Neither function accepts a persistent camera-history key, and B+0x10 is the manager back-pointer, not a previous camera. Draw's current-view UV rectangle selects a portion of the common root image. **There is no independently aged or ping-ponged history per camera in this implementation.**

The H allocation and the save quads are sized at construction. Draw recomputes current-view UV/xy each call, but neither draw/save nor the prefix setters resize H/S or invalidate on a camera cut, camera switch, projection change, or render-mode/dimension change. Correct whole-screen capture/restoration assumes compatible stage/camera dimensions; section 4 records the scratch-versus-history source-dimension distinction. Runtime camera identity, viewport arrangement, and behavior after mode/size changes remain conditional; one valid byte cannot establish their compatibility.

### Bounded multi-view finding, with its exact linkage limit

The **body** at `0x801E4EF0` was independently read (D:489901-489995). It uses the mode at `0x8057E760+0x60` and selects views through calls to `0x8027211C` / `0x80271F9C`:

- Mode 0 selects view 0 and returns without an internal child traversal.
- Mode 1 selects view 1, explicitly traverses this task's children at `0x801E4FA0`, then selects view 2 and returns.
- Mode 2 similarly selects view 3, traverses children at `0x801E501C`, then selects view 4 and returns.
- The outer generic walker subsequently performs its ordinary child recursion at `0x8004EDB0`. If this body is Div's Exec, modes 1/2 therefore execute Div's children for two views, not two successive saved histories.

The nearby body `0x801E4E40` resets the view/index state and selects view 0 (D:489857-489882). The Div and Static constructors install vtables `0x8053EA70` and `0x8053EA54` respectively (D:490004-490005,489891-489892). **DM does not export their Exec words**; the exact body-to-root assignments are therefore conditional here, unlike the now-confirmed DrawBlur/SaveScreen assignments. The two missing optional linkage words are specified in section 9, rather than silently promoting constructor adjacency into vtable proof.

If those root slots select the examined bodies, both Div view overlays consume the same earlier H, followed by one Static save for that parent-tree traversal. Switching views does not allocate, swap, or reset H. Actual mode/camera selection and whether the Save is reached still require live observation.

### Do not equate a traversal with a VI frame

The bounded proof establishes **relative task-tree order**, not the call frequency of task index 5 versus VI fields, XFB swaps, displayed frames, simulation ticks, or an emulator presentation event. The core image routines consume no delta-time argument and maintain no frame ID/timestamp. They can reuse H multiple times, leave it older when Save is skipped, or update it multiple times before a display event if the caller traverses more often. First-use warm-up is measured in reached Draw/Save cycles, not certified milliseconds or display frames. A static ordering proof is not a captured runtime chronology.

## 9. Narrow coverage, closed facts, and genuinely remaining evidence

The original report's B-based direct-call inventory is preserved below; the core chains, task constructors, and normal resource-lifetime call sites were independently reread in this continuation. This is direct-branch coverage of the scoped family, **not** proof of all absolute pointers, dynamic task mutation, or arbitrary indirect/manual calls elsewhere:

- Composite: `0x801D280C -> 0x8042F16C`, `0x8042F184 -> 0x8042E254` (B:62663,138316).
- Save: `0x801D2774 -> 0x8042F140`, `0x8042F158 -> 0x8042E838` (B:62659,138315).
- H construction/destruction: `0x801D1A80 -> 0x8042F20C`, `0x8042F23C -> 0x8042EA50`, `0x801D19B8 -> 0x8042F198` (B:62553,138321,62534).
- SaveScreen task construction: `0x801D20B4 -> 0x801D20D8`; DrawBlur task construction: `0x801D1AF0 -> 0x801D2058` (B:62614,62565).
- Shared scratch setup/teardown: `0x801D1A68 -> 0x80433638`, `0x801D19D8 -> 0x804335C8` (B:62549,62542).
- Copy helper has other post-processing users (B:138325 onward). The bounded G/S alias cross-check in section 4 demonstrates shared **scratch**, not a second or aliased B history. Other effect internals were not audited.

### Closed: do not request these again

- `0x804D25D8=1`; record-11 name pointer/string = PostGlare; special R+0x58 name = PostEffect; actual table grouping and insertion order (section 7).
- Blur task Exec/destructor words at `0x8053DC64/68/80/84` (DM:23-53).
- Geometry half/one/zero and conversion-bias words at `0x8051C388..0x8051C3A0`, including default scale=1.0 (DM:63-92).
- Initialized backend descriptor, blend-factor and primitive lookup values (DM:94-177,488-512), with the scoped downstream instruction checks in section 6.
- Original DOL byte/hash correspondence: **parent verified**, not a failed probe that still needs to be repeated by this specialist. No new Python execution is claimed.

### Exact remaining evidence, only for stronger conditional/runtime claims

1. **Optional static multi-view linkage, not the closed normal Draw-before-Save selector question:** the current DM does not include `u32(0x8053EA7C)` (Div vtable +0x0C, to test linkage to the examined `0x801E4EF0` body) or `u32(0x8053EA60)` (Static vtable +0x0C, to test linkage to `0x801E4E40`). These are the two exact missing raw words for promoting that specific camera-traversal assignment; the already supplied DrawBlur/SaveScreen vtables must not be requested again. Even resolving these words would not establish which mode is active at runtime.
2. **Presentation-age claim:** a bounded trace correlating task-index-5 traversals and `0x8042E254/0x8042E838` entries with a VI field/frame or presentation counter. Include task skip/delete flags, M+0x40, B's enable/alpha/valid/Hview, the selected camera/view, and the mode at `0x8057E7C0`. This is what could establish actual “one previous displayed frame” behavior; the static family has no timestamp to supply it.
3. **Successful-pixel / failure / teardown claim:** live camera +0x18/+0x1C targets and return values, the actual engine pointer/callbacks, Hroot/Sroot plugin pixel pointers and formats/dimensions, copy-clear control `0x805F2688`, and a GPU command/image trace through first enable, steady operation, disabled Save, re-enable, and two-view rendering. A drain/fence observation would be needed to certify GPU-safe resource destruction. The software paths are documented without asserting that failures occurred in a run.

**No further render-table, blur-vtable, geometry-constant, or hash evidence is needed for the normal temporal classification.** The outstanding items qualify runtime cadence, precise camera linkage, pixel fidelity, and exceptional-path behavior; they do not resurrect the rejected normal same-frame ordering alternative.

## 10. Changes and commands

Original report provenance retained: that earlier session recorded successful `ls` and `ls analysis/motion_blur/evidence`, plus the two denied, unexecuted Python probes in section 1. They were not retried or relabeled as successful.

This continuation edited only `analysis/motion_blur/evidence/08_temporal.md` and created the explicitly assigned `analysis/motion_blur/motion_blur_temporal_state.md`. Original/prior/Sol files, DOL, manifests, other specialist reports, tools, and configuration remain read-only. The continuation's only successful shell command was `pwd` in the supplied workspace; all evidence acquisition afterward used bounded READ calls on the named existing attachments. No Python, export/verify, hash, build/test, emulator, or runtime-validation command was executed by this continuation. Review/checks were static cross-checks of returned printed addresses, instruction paths, raw big-endian words, ownership, and report consistency.

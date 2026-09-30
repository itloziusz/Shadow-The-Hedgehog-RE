# Specialist 15 — independent integration baseline

## 1. Status, provenance, and limitations

This is an **initial integration checkpoint**, not the final requested extraction or source. Scope is the candidate motion-blur subsystem and dependencies reached from its own calls. No whole-game audit was attempted.

- Read `AGENTS.md` and independently searched the pre-existing map/strings, then examined local instructions before looking for specialist reports.
- Background permissions denied the requested `python -B .../ppc_tools.py pointers`, bounded `disasm`, and a read-only `analyze_dol`/SHA-256 import command. I did not retry through another execution route or change permissions. Consequently, “instruction-verified” below means verified by bounded **reads of the pre-existing local `motion_blur_analysis/full_disassembly.txt`**, including its original instruction words, not by a fresh decode/hash of `main.dol` during this role.
- The local utility documents how that artifact is produced (`motion_blur_analysis/ppc_tools.py:8-25,31-36`); address/offset mapping and original-byte import are in `motion_blur_analysis/analyze_dol.py:5-24`. The map was used only to locate hypotheses.
- Existing baseline SHA-256 is `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`, size 5,773,024 (`evidence/binary_baseline.json:2-7`; `motion_blur_analysis/inventory.json:2-4`). **Parent must perform final fresh integrity/address verification; I did not independently recompute the hash.**
- First evidence-directory inspection contained only `binary_baseline.json`. Final checkpoint also had `function_manifest.json`, which I read: its core function ranges agree with the independently read ranges (`function_manifest.json:5-23`); its vtable/device addresses are consistent hypotheses, not new raw-pointer verification (`:28-30`). There were **no completed numbered specialist reports to compare at either checkpoint**. No other investigator's findings are claimed here. This report is intended to be resumed after those reports exist.

All code addresses below are original local virtual addresses. Ranges are end-exclusive unless a single instruction address is given. Source line references to `full_disassembly.txt` are abbreviated `FD:Lx-Ly`.

## 2. Independently isolated candidate, with non-symbol proof

### 2.1 Task identity and outgoing entrypoints

1. Local strings identify `DrawBlur` at **0x804D0F18**, `SaveScreen` at **0x804D0F24**, and their class names at **0x804D1058 / 0x804D1034** (`relevant_strings.tsv:134-142`).
2. **0x801D2058-0x801D20D8** constructs a task, puts `0x804D0F18` in task+0 at `0x801D2090/2098`, installs `0x8053DC78` at task+0x18, and sets flag 0x100. Its parent is the object returned by **0x800461A0**, field **+0x2C**. It allocates 0x28 bytes and calls **0x801D20D8**, retaining that child task at task+0x28 (FD:470535-470566).
3. **0x801D20D8-0x801D213C** installs `SaveScreen`, vtable candidate `0x8053DC5C`, and flag 0x100. Its scheduling parent is the same manager's **+0x58**, **not the DrawBlur task itself** (FD:470567-470591).
4. Candidate exec stub **0x801D27FC-0x801D2820** calls **0x800A35D8** at `0x801D2808`, then **0x8042F16C** at `0x801D280C`; the latter null-checks manager+0x40 and calls **0x8042E254** at `0x8042F184` (FD:471024-471032;1090124-1090134).
5. Candidate save exec **0x801D2764-0x801D2788** calls the same singleton getter, then **0x8042F140**, which null-checks manager+0x40 and calls **0x8042E838** (FD:470986-470994;1090113-1090123).
6. **Still required to close task dispatch:** raw vtable words at `0x8053DC78+0x0C` and `0x8053DC5C+0x0C` must prove these exec callbacks. The constructors and callback bodies are independently established; the intervening data-pointer edge was not freshly read by this role.

Map danger is demonstrated, not hypothetical: the reference-map constructor `0x801D1CB8` and callback `0x801D245C` are **not these local entrypoints**. In this region their local counterparts are +0x3A0, but the same delta must not be generalized. Map `GXCopyTex` at `0x80396568` is also wrong for this DOL: local `0x80396568` is the nine-instruction BP-write synchronization body, whereas the copy-control body is at `0x803975AC` (FD:933707-933715;934748-934846).

### 2.2 Allocation and ownership

- **0x800A35D8-0x800A360C** lazily obtains the manager through r13+0x2DD8. **0x800A360C-0x800A3664** constructs static object **0x8057798C** and registers its destructor; `0x800A36CC` calls `0x80433A14` (FD:160615-160693).
- **0x801D1A1C-0x801D1C30** is the directly relevant initialization body. It calls `0x8042F20C(manager, 0)` at **0x801D1A80**, allocates the 0x30-byte DrawBlur task at **0x801D1AE0**, constructs it at **0x801D1AF0**, and retains it in owner+0x24 at **0x801D1AF8** (FD:470136-470268). Direct caller found in `branches.tsv:62639`: **0x801D24DC -> 0x801D1A1C**; its upstream context still needs closure.
- **0x8042F20C-0x8042F258** allocates **0x13C bytes**, calls **0x8042EA50(state, manager, config)**, and stores state in manager+0x40 (FD:1090164-1090182).
- **0x8042F198-0x8042F20C** frees the parent of state+0x74 if distinct, frees the child raster, then frees state and zeroes manager+0x40. Its direct caller is **0x801D19B8** (`branches.tsv:62534`; FD:1090135-1090163).
- **0x801D2788-0x801D27FC** marks the retained SaveScreen task for deferred deletion by ORing bit 1 into child+4; then calls base task destruction. It does **not** itself release the history raster/state (FD:470995-471023).

## 3. Coverage criteria derived from exact instructions

A “complete readable extraction” must satisfy **all** rows, not just identify a blur-named task or one GX copy function. Required edges are intentionally limited to this subsystem.

| Area | Independently established instructions | Evidence required for final closure |
|---|---|---|
| Entry/ownership | Task constructors and outgoing callbacks above; manager+0x40; 0x13C state | Raw task vtable entries, initialization caller at 0x801D24DC, teardown ordering and ownership, no invented constructor success guarantees |
| Scheduling | Parents +0x2C versus +0x58; intrusive append in 0x8004F014; recursive dispatch in 0x8004ECAC | Raw render-level table fields and task vtables; order relative to camera begin/end, post-effects and save; top-level per-frame/root edge; per-camera multiplicity |
| Inputs/strength | Config copy helpers 0x8042E9C4, 0x8042E9F0, 0x8042EA1C and wrappers 0x8042F084/F0E8/F114 | Exact callers/producers, enabled/alpha/scale math, timing/reset conditions and any opaque copied fields; do not substitute generic speed-based blur |
| Capture | Save 0x8042E838 calls 0x80496208 at 0x8042E8C0 and 0x8042E968, both with r4=0 | Both source/destination rasters, format mapping and allocations, EFB rectangle and tiled destination offset, clear flag provenance, GPU synchronization |
| Texture | Draw uses *(state+0x74) as raster-state value; save first binds manager+0x34's root | Driver slot +0x20 -> raster-state case -> texture loader, GXTexObj format/wrap/filter/LOD/TMEM settings; sampler validity after EFB copy |
| Geometry/composite | Three separate four-vertex arrays at +0x14, +0x7C, +0xDC; driver slot +0x30 with type=4,count=4 | Raw constants, coordinate/UV math and primitive translation/winding, stream stride and GX vertex formats, TEV/alpha/blend/depth/cull behavior |
| Restore | Draw snapshots IDs 10,11,12,6,8,14,9, restores those seven, then ends camera | Driver getter/setter semantics and dirty-state behavior; distinguish restored state from texture state/copy-filter defaults; save has different restoration behavior |
| History/lifecycle | Save clears valid+0x78 before gates, sets it late; draw requires valid; one retained raster/subraster pair | Initial allocation contents, first frame and disabled/re-enabled behavior, camera/resize/multi-view assumptions, early-failure paths; do not invent ping-pong or CPU history |
| Fidelity/integrity | Existing instruction words, branches and baseline | Fresh DOL SHA-256, bounded re-decodes, exact raw data/indirect edges, readable source coverage review; runtime behavior must remain explicitly unvalidated without a captured trace |

### 3.1 Scheduler observations and limits

- `0x8004F014-0x8004F0F0` stores parent at task+0x10 and appends using parent+0x14, next+0x0C, and backlink/tail+0x08 (FD:74230-74284).
- `0x8004ECAC-0x8004EDF0` tests low task flags, handles deferred deletion, dispatches ***(task+0x18)+0x0C** at **0x8004EDA4**, then recurses at **0x8004EDB0**. A profiling alternative calls **0x8004EDF0** at **0x8004ED88**. The global pause-like flag is tested at **0x8004ED60**; task flag 0x100 provides the relevant bypass (FD:74012-74092). Do not omit the alternative dispatch path or silently erase skip/delete behavior.
- `0x800461A0` constructs the render-level manager via **0x801E4B0C** (FD:65113-65139). That constructor uses a **0x14-byte table rooted at 0x804D24EC**, allocating scheduling parents according to table+0x10, then writes names and flags; the standalone +0x58 parent is linked below manager+0x64 (FD:489652-489763). Raw entries are necessary before asserting names/order.
- `0x801E4EF0-0x801E506C` has mode-dependent camera selection and explicit child execution calls at **0x801E4FA0 / 0x801E501C** before selecting a second camera (FD:489901-489995). Therefore **one blur/save invocation per VI frame is not established**. Multiple traversal/camera passes must be accounted for, not “simplified” away.

## 4. Local renderer behavior already visible

### 4.1 Config and state layout

| State offset | Locally justified meaning |
|---|---|
| +0x00 | enabled/gate byte |
| +0x01 | alpha/strength byte; copied to all four primary vertex alphas |
| +0x04 | floating expansion/scale input |
| +0x08,+0x0C | copied config words; meaning unresolved here, no read observed in draw/save |
| +0x10 | owner/manager pointer |
| +0x14..+0x73 | primary draw vertices, four records of 0x18 bytes |
| +0x74 | retained raster/subraster pointer; its +0 points to texture root |
| +0x78 | history-valid byte |
| +0x7C..+0xDB | save/restore quad |
| +0xDC..+0x13B | second save/restore quad |

`0x8042E9C4-0x8042EA50` copies exactly bytes +0,+1, float +4, words +8,+0xC. The null-config helper at `0x8042EA1C` returns without changes; it does **not** reset enabled/history. `0x8042F084` (get config) has an absent-state default path with +8/+0xC copied from stack words, not initialized to zero in the shown body (FD:1089634-1089668;1090066-1090112). A readable reconstruction must label these opaque/indeterminate bytes instead of inventing semantics.

Constructor `0x8042EA50-0x8042F084` initializes enable=0, alpha=0, scale from **0x8051C38C**, and owner before camera cast. Its early exits at `0x8042EAB0 / EAD0` occur **before** history pointer and valid initialization at `0x8042EB8C/EB90`. Normal default completion at `0x8042F050-F068` sets enable=0, alpha=0x80, scale from 0x8051C38C. Two raster creation calls at **0x8042F00C / F024** use flags **0x505**, first (0,0,0,0x505), then root dimensions; **0x8042F034 -> 0x8048AE14** links the first raster as a subraster of the second (FD:1089669-1089755;1089964-1090065). The first creation result and second root allocation must not be flattened to an assumed full-screen raw buffer without reproducing ownership/rectangle behavior.

### 4.2 Draw core: 0x8042E254-0x8042E838

Full body read at FD:1089158-1089534.

1. Exits unless enabled, alpha nonzero, successful camera-provider cast (**0x8042B620 -> 0x803A1AFC**), camera returned by virtual +0x24 nonnull, and history-valid nonzero.
2. Snapshots render-state IDs **10,11,12,6,8,14,9** through **(*0x805F265C)+0x24** (`0x8042E2CC-E3A8`).
3. Gets camera raster at camera+0x60; **0x8048AB7C** supplies signed x/y offsets, width/height come from raster+0x0C/+0x10, root dimensions from raster->parent. UVs use those offsets plus **float 0x8051C388**, divided by root width/height (`0x8042E3AC-E528`). Fresh raw-constant verification is still needed before naming that float “half pixel.”
4. Writes RGB=255 and A=state+1 to all four primary vertices (`0x8042E52C-E590`). Screen expansion uses **(scale - float[0x8051C38C]) * width/height**, multiplied by **float[0x8051C388]**; negative expansion gives left/top, original dimension plus expansion gives right/bottom (`0x8042E598-E600`). Exact floating operation order is visible and should be retained.
5. Calls **0x8048652C(camera)**; on success sets raster state **(1, *(state+0x74))**, then state pairs **(10,5),(11,6),(12,1),(6,0),(8,0),(14,0),(9,1)**. Driver entrypoint identities must be resolved before translating these numeric enums.
6. Calls **(*0x805F265C)+0x30** with **r3=4, r4=state+0x14, r5=4** at **0x8042E738**. Restores the seven saved states at `0x8042E73C-E818`, calls **0x80486504(camera)**, returns.

Important: **state ID 1 (raster) is not saved/restored by this function**. “Restores all GX state” would be false. No EFB-copy call, history swap or validity write occurs in this draw body. The likely temporal use follows from its separation from SaveScreen, but exact frame age depends on proven scheduler order.

### 4.3 Save core: 0x8042E838-0x8042E9C4

Full body read at FD:1089535-1089633.

1. Clears valid+0x78 immediately, even if subsequent enable/alpha/camera gates fail.
2. Gets **manager+0x34 -> parent/root**, calls **0x80496208(root,0)** at **0x8042E8C0**.
3. If camera begin succeeds, binds that root with state ID 1, sets **(10,2),(11,1)**, submits **(4,state+0x7C,4)**, and ends the camera.
4. Calls **0x80496208(state+0x74,0)** at **0x8042E968**.
5. If camera begin succeeds, submits **(4,state+0xDC,4)** and ends the camera. No new texture bind or seven-state snapshot is present in this second part.
6. Writes valid=1 at **0x8042E9AC**, **even if either camera-begin call returned zero**, provided the earlier gates reached this region. Do not “correct” this to valid only on successful rendering without marking a behavioral change.

This is not merely “capture final EFB once.” There are **two copy calls and two optional draws**, preserving/reconstituting framebuffer content while updating the retained history. Exact contents require both constructor-built quads and the driver state semantics. No ping-pong index is present in this body.

### 4.4 Copy dependency: 0x80496208-0x80496410

Full body read at FD:1195635-1195764.

- Entry resolves raster parent/root and plugin offset from r13+0x6200, disables copy filtering through **0x80397228(0,0,0,0)**.
- Caller r4 is a size/downsample selector: the r4=0 path uses signed raster x/y at +0x1C/+0x1E and dimensions +0x0C/+0x10, truncated to 16-bit GX arguments, calling **0x80396C50** at `0x8049629C`. The alternative doubles source dimensions; **blur's two calls choose zero, not that doubling path**.
- Destination setup **0x80396D00** at `0x804962B8` receives root dimensions and runtime raster-plugin format. A jump table at **0x8056F544** dispatches tiled destination offsets by root depth, with four packing formulas. Exact table entries/format still need raw verification.
- At **0x804963B8**, **0x803975AC** receives plugin pixel base + computed offset, and **low byte of r13+0x6188** as its second argument. This is the copy/clear control argument and is **not** the earlier r4=0 supplied to the raster wrapper.
- **0x80396568** follows at `0x804963BC` and emits the cached BP PE-control word; local `0x803975AC` emits BP copy registers, physical destination >>5 and copy trigger (FD:933707-933715;934748-934846). Names `GXPixModeSync` and `GXCopyTex`, respectively, fit the local implementations; direct reuse of the map addresses would reverse/misidentify this behavior.
- Copy-filter setup is reinstated from the runtime render-mode descriptor at `0x804963C0-D4`, then **0x8039868C(region)** or **0x803987B8()** is selected from plugin+0x2C. A reconstruction must include cache/texture visibility and not replace this with a CPU memcpy.

## 5. Upstream inputs that must be closed, not guessed

The complete direct branch set in the existing branch artifact exposes these immediate control writers:

- **0x8042F114** is called at **0x800A356C, 0x800A375C, 0x800A481C** (`branches.tsv:20745,20758,20890`). Independently read `0x800A3528-0x800A35D8`: constructs enable=0, alpha=0x80, scale from r2-0x6BB0 and submits at `0x800A356C` before other cleanup (FD:160571-160614). The active/update counterparts must be read rather than inferring a motion/speed curve from the map.
- **0x8042F084** getter and **0x8042F0E8** setter are paired at **0x80204720/0x8020475C**, **0x802055E8/0x80205624**, **0x80206320/0x8020635C** (`branches.tsv:69553-69555,69734-69736,69944-69946`). Their exact enclosing function bodies and trigger/dispatch edges are needed to establish effect-driven overrides and alpha truncation/clamping/order, not a whole-game search.
- The copy helpers accept a 16-byte layout with inactive padding/opaque data; no generic public “motion blur strength float” interface is proven. At least an enable byte, an alpha byte and a scale float must remain separate.
- Direct branch searches do not prove absence of function-pointer callers. Relevant parameter-writer address-pointer checks and vtables remain part of final coverage.

## 6. Proposed evidence-backed subsystem boundary

### Include as readable reconstructed game logic

- DrawBlur/SaveScreen constructors, verified exec stubs/vtables, DrawBlur deferred child teardown, the relevant initialization/destruction slices, and singleton/state ownership.
- **0x8042E254-0x8042F258**: draw, two-stage save, parameter-copy helpers, constructor, wrappers, release/recreate. This contiguous range is a defensible **core boundary**, not the entire effects engine. Be explicit about the two copied opaque config words and failure behavior.
- Immediate blur control-producer functions and their actual scheduling/callback adapters listed in section 5; narrowly follow upstream data inputs that determine blur, not unrelated particle/fog behavior wholesale.
- Only render-tree construction/dispatch and camera-phase hooks needed to prove relative DrawBlur/SaveScreen ordering and multiplicity.

### Include as exact, scoped backend dependencies or audited adapter contracts

- Camera-provider/cast/get-camera contracts; **0x8048652C/0x80486504** begin/end behavior actually used here.
- Raster root/subraster creation/destruction and offset-query helpers **0x8048AEC4, 0x8048AE14, 0x8048AC2C, 0x8048AB7C**; manager+0x34 scratch/screen raster initialization and lifetime.
- **0x80496208-0x80496410**, relevant raster plugin allocation/format settings, the state getter/setter/primitive driver functions installed under **0x805F265C**, and reachable texture/GX setup/emission code.
- External SDK calls may be documented as named adapters only **after** their local target and arguments are proven. The custom state translation, texture setup, copy offset math, vertex emission and TEV/blend behavior cannot all be hidden behind an unspecified `drawFullscreenQuad()` and still called complete extraction.

### Exclude except where an actual data dependency is shown

Separate glare/bloom/shimmer implementations, generic scene rendering, particle systems, unrelated fog/tint logic, generic allocator internals, SDK-wide implementation dumps, and startup/gameplay audits not required to prove the immediate blur trigger. The adjacent function beginning **0x8042F258** is **outside this verified core boundary** unless a specialist establishes a needed shared dependency; its proximity and post-effect-like operations are not proof that it is motion blur.

## 7. Initial integration blockers and questions for final pass

No cross-specialist disagreement can yet be judged because no completed numbered reports existed at inspection. The subsequently available `function_manifest.json` agrees on core addresses/ranges but does not itself close the unresolved dependency/data edges below. Check the following before anyone claims a complete reconstruction:

1. **Indirect dispatch closure:** task vtables `0x8053DC5C/78`, camera-provider vslot+0x24 after the actual RTTI cast, renderer device at `0x805F265C` and its slots +0x20/+0x24/+0x30. Required target bodies are currently not resolved here, not assumed absent from the DOL.
2. **Phase/order closure:** raw 0x804D24EC render-level table; manager+0x2C versus +0x58 order; root traversal, camera begin/end and multiple-camera loops. Exact upstream initialization edge `0x801D24DC` and teardown `0x801D19B8` contexts.
3. **Inputs:** exact bodies containing `0x800A375C`, `0x800A481C`, `0x80204720-0x8020475C`, `0x802055E8-0x80205624`, `0x80206320-0x8020635C`; callback/pointer edges and constants; precedence if multiple producers write in a frame.
4. **Data:** raw words/floats at **0x8051C358-C39F** (geometry templates, scale/UV constants and integer conversion constants), actual source/parent rectangle and format, jump table **0x8056F544**, driver pointers/vtables. Do not silently initialize constructor fields read before explicit writes.
5. **Shared screen raster:** exact initialization/resize/destruction of manager+0x34, used by save before history copy. This is directly required lifecycle evidence, not unrelated renderer scope.
6. **GX/composite:** exact installed getter/setter/primitive body, state numeric translation, texture loader/init/load targets and GX arguments, TEV equations, primitive mode/vertex layout/winding and dirty-state realization. A list of GX names without this call-chain closure is insufficient.
7. **Restore/history nuances:** preserve draw's missing raster-state restore; save's different state behavior; late valid=1 despite optional camera-begin failures; copy clear flag versus downsample selector; first enabled frame and subsequent camera passes. Avoid unsupported “cleared history,” “always full state restore,” “one pass per frame,” “always no clear,” or “ping-pong buffers.”
8. **Fresh verification:** rehash original DOL and directly re-decode key entrypoint/instruction words when permissions permit. Current role has static artifact verification only, no runtime trace and no executable source validation.

## 8. Actions performed

- Successful shell commands: `pwd`, `ls -la`, `ls -la motion_blur_analysis`, `ls -la analysis/motion_blur/evidence` in the requested workspace. Read/grep tools examined AGENTS, bounded disassembly slices, strings, branch table, map hypotheses and baseline/tooling files. The oversized full disassembly was **not grepped**.
- Denied commands (not executed): `python -B motion_blur_analysis/ppc_tools.py pointers 0x804D0F18`; `python -B motion_blur_analysis/ppc_tools.py disasm 0x801D1C00 0x801D1EC0`; a `python -B -c` read-only import/hash/inventory command.
- Only created/edited file: **`analysis/motion_blur/evidence/15_integration.md`**. No DOL, prior artifact, other evidence report, final requested document or reconstructed source was changed. No implementation subagents were created.

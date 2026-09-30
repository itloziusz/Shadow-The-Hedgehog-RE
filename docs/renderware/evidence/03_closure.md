# Role 03 — top-down render/task closure

## Verdict and provenance

**The ordinary main-loop-to-render-group-to-blur path is statically closed.** The recurring event dispatch reaches the scheduler's root; its group 5 is actually named `Render`. Its constructed child order is Start, Div, Static, End. DrawBlur is below Div / level **11 PostGlare**, before SaveScreen below Static / PostEffect. Div modes 1/2 visit their children twice before Static, not once per camera-history capture. This establishes program order, **not measured execution or one invocation per VI/display frame**.

Only this report is written. `AGENTS.md:1-14` was read; original/prior/Sol, other role-03 files, manifests, tools, configuration and final documents are unchanged. Prior `08_temporal.md:227-354` and `14_adversarial.md:15-27` supplied navigation leads; the instructions and table words below were independently read rather than treating their conclusions or reference-map names as evidence.

Sources: **D** = `motion_blur_analysis/full_disassembly.txt`; **BR** = `motion_blur_analysis/branches.tsv`; **O/** = `analysis/motion_blur/evidence/original/`; **DM** = `analysis/motion_blur/evidence/data_manifest.json`. Core shorthand `Draw.asm` / `SaveScreen.asm` means O/`8042E254_MotionBlur_Draw.asm` / `8042E838_MotionBlur_SaveScreen.asm`. All bare addresses/displacements are hexadecimal unless an index/mode is stated. D reads were bounded using `line = 1 + (VA - 0x80006840)/4`, with returned printed VAs checked. This formula is used only for text1, not startup text0.

Executed read-only `python -B analysis/motion_blur/tools/dol_evidence.py verify`: **PASS**, SHA-256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`, **81 ranges / 7,249 instruction words / 3,626 file-backed data bytes** in the fresh 55-record data export. This authenticates the exported ranges/data, not every additional legacy D slice. Supplemental `data` queries below also enforce that target hash; none exported or changed artifacts.

## 1. Complete relevant path, top down

```text
800510C0 main/event-loop containing function
  800511EC -> 80051978(event=0x12)
  800519A8 -> 80048EB8 application event dispatcher
  jump word [8051E628]=80049078; 80049078 -> 801E2210 frame driver
  ordinary 801E23A4 -> 801EB680; 801EB698 -> 8004ECAC(manager.root)
    root's child group 5 = *(manager+0x18), "Render"
      RenderAttach_Start  R+5C
      RenderAttach_Div    R+60 -> Exec 801E4EF0
        level 11 PostGlare R+2C
          DrawBlur -> Exec 801D27FC -> 8042F16C -> core 8042E254
      RenderAttach_Static R+64 -> Exec 801E4E40
        first child PostEffect R+58
          SaveScreen -> Exec 801D2764 -> 8042F140 -> core 8042E838
      RenderAttach_End    R+68
```

Other groups/tasks/levels exist; this is the blur-relevant ordered subtree, not an exclusive whole-game call graph. All virtual edges use the constructor-installed tables, under the ordinary unmodified task tree.

## 2. Recurring frame driver and actual group-5 root

| Containing function / slice | Instruction proof and exact edge | Evidence |
|---|---|---|
| `800510C0..80051230` (end exclusive) | After initialization, loop `800511E4..800511FC` supplies `r3=0x12,r4=0`, calls `80051978` at **800511EC**, then `8032D444` at 800511F0, and repeats while `u32(80576DB0+0xC)==0`. | D:76321-76412; BR:8971 |
| `80051978..80051B18` | Prefilter `80050FA8` called at 80051990 returns 2 for event 0x12 (comparison is against 0x1E); then **800519A8 -> 80048EB8**, preserving the event. | D:76251-76267,76879-76891; BR:9054 |
| `80048EB8..800490B0` | Bounds event at 0x12, indexes table **8051E5E0** at 80048ED0-80048ED8, `bctr` at 80048EE0. Raw entry `8051E5E0+4*0x12=8051E628` is **80049078**, which calls **801E2210**. `80049078` is a case slice, NOT a separate function. | D:67999-68009,68111-68124; BR:7915; hash-checked raw query below |
| `801E2210..801E2480` | Established path (`byte[r13+4554]!=0`) performs preparatory work, gets the task manager at **801E239C -> 8001C4F8**, supplies `f1=[r13+4558]`, then **801E23A4 -> 801EB680**. | D:487029-487139; BR:64541 |
| Same driver: initialization / transition alternatives | When `[r13+4554]==0`, live object's virtual +0xC at 801E225C gates execution: low byte zero exits without traversal; nonzero calls scheduler at **801E2278** and sets the byte. Nonzero virtual +0x10 result at 801E23C0 enters transition/teardown. If virtual +0x14 at 801E23DC has nonzero low byte, destruction is followed by **three additional scheduler calls at 801E241C/2428/2434**; otherwise **801E23E8 self-loops**. | D:487039-487059,487133-487184; BR:64529,64545-64549 |
| `8001C4F8..8001C564` | Lazy singleton body **K=80571C6C**; first use calls **801EB734** at 8001C524 before caching K at `[r13+2BD0]`. The same getter is used by frame dispatch and render-root constructors. | D:22319-22345 |
| `801EB734..801EB790`, `801EB53C..801EB5D4` | Creates parentless base task, stores K+0 at **801EB764**; calls group constructor at **801EB76C**. That loops over six actual words at **8053ED6C..8053ED84 = 0,1,2,3,4,5**, appending each under `*(K+0)` at 801EB580 and storing at `K+4+4*index` at 801EB598. Group-5 name pointer **[8053ED34]=805F7150 -> "Render"**. | D:496448-496485,496574-496596; raw queries below |
| `801EB614..801EB624`, `801EB680..801EB6AC` | Lookup returns `*(K+4+4*r4)`, so index 5 is K+0x18. Scheduler loads **K+0**, returns if null, otherwise **801EB698 -> 8004ECAC**. It does not call a separate group-5-only loop; group 5 is reached by child recursion from the common root. | D:496502-496505,496529-496539; BR:65896 |

The recurring event-loop edge is closed without relying on a displaced map symbol. BR:15 additionally records `800032A8 -> 800510C0`, but startup text0 is outside this bounded instruction pass and is not needed to infer the loop's recurring behavior. No VI/XFB correspondence follows from calling this a frame driver.

## 3. Append, recursion, skip/deletion, profile and pause semantics

- Base constructor **8004F014..8004F0F0** stores parent at task+0x10, child head at parent+0x14, and **tail-appends**, not prepends: 8004F094-8004F0C4 uses head+8 as tail, writes old-tail+0xC=new task, updates head+8, and terminates new next with zero (D:74230-74284). Its installed base vtable is **8051E768** (8004F030/38); actual +0xC word **8051E774=8004EA80**, whose body is `blr` (D:73873). Thus group and plain level Exec do not themselves reverse or consume their child lists.
- Walker **8004ECAC..8004EDF0** loads parent+0x14 at **8004ECD4**; normal Exec dispatch is `task+0x18 -> vtable+0xC -> bctrl` at **8004ED98-8004EDA4**, then **8004EDB0 recursively walks that task's children**, then **8004EDB4** follows next sibling+0xC (D:74012-74092). This is the ordering argument, not phase names alone.
- Let `F=u16(task+4)`, `P=byte[r13+2D7C]`. **F&0xF != 0** ordinarily skips that task and its subtree to 8004EDB4. If bit 0 is set and P==0, the deletion path clears bit 0 at 8004ECFC-8004ED00, tests **F&0x20** at 8004ED08 (set: skip), otherwise drains/recurses through children and calls destructor **vtable+8 at 8004ED58**. P!=0 defers this deletion path (D:74024-74056). Do not describe all low-flag outcomes as a successful Draw/Save.
- With low nibble clear, **P!=0 and !(F&0x100)** suppresses this task's Exec but **still recurses into its children** (8004ED60-8004ED70 -> 8004EDA8). Pause does NOT universally freeze the render subtree. Group creation, four phase constructors, level creation, DrawBlur and SaveScreen explicitly OR **0x100** into F (D:496474-496476,489893-489895,490006-490008,490025-490027,490055-490057; O/801E4B0C_RenderLevels_Construct.asm:87-89,110-112; task constructors :22-24).
- **word[r13+2D74]!=0** chooses profile wrapper **8004EDF0** at 8004ED88. It invokes the **same vtable+0xC at 8004EE44-8004EE50**, measures/stores timing at task+0x20/+0x24, and returns to the same recursion (D:74062-74138). `[r13+2D70]` is incremented/decremented traversal depth, not a blur frame ID.
- Pause setters **801EB674/801EB668** store 1/0 at `[r13+2D7C]` (D:496523-496528). In containing function **801E41D4**, request field +0x48 equal 1/2 calls them at **801E4208/801E4214**, then clears the request; ordinary driver calls this at **801E23AC**, AFTER its scheduler traversal (D:489062-489085,487131-487132). Live flags/requests are not read from initialized DOL data.

## 4. Construction table and resolved phase callbacks

R is the separate render-level singleton **80574384**, constructed through **800461CC -> 801E4B0C** before its getter 800461A0 returns (D:65113-65139). The four phases each get `8001C4F8; r4=5; 801EB614`, then call an append-based constructor in this order:

| Phase / R slot | Constructor call site -> containing constructor | Installed vtable; actual Exec slot -> body |
|---|---|---|
| Start / +0x5C | **801E4B4C -> 801E5130** | **8053EAA8; [8053EAB4]=801E5100** |
| Div / +0x60 | **801E4B88 -> 801E506C** | **8053EA70; [8053EA7C]=801E4EF0** |
| Static / +0x64 | **801E4BC4 -> 801E4EA8** | **8053EA54; [8053EA60]=801E4E40** |
| End / +0x68 | **801E4C00 -> 801E50B8** | **8053EA8C; [8053EA98]=801E50B4 (`blr`)** |

Sources: O/801E4B0C_RenderLevels_Construct.asm:12-72; D:489883-490062; actual phase words **DM:1330-1367**, names **DM:1298-1327**. No missing phase-vtable linkage remains.

- The ordinary level loop **801E4C68-801E4CC8** uses **T=804D24EC**, record stride 0x14, parent `*(R+0x5C+4*u32(T+0x14*i+0x10))`, and stores the child at `R+4*i` (O/:92-116).
- Draw's constructor loads **R+0x2C** at 801D2078: **i=11**. Actual record **804D25C8 = {804D23D8,0,0,0,1}**, hence **804D25D8=1** and parent **R+0x60=Div**. Name bytes at **804D23D8** spell **RenderLevel_PostGlare** (DM:284-288,416-421). `GlareWorld` belongs to index 9 / R+0x24, not this node.
- Before that loop, **801E4C1C-801E4C38** explicitly creates **R+0x58 under R+0x64/Static** using 8004F014 at 801E4C30. Name lookup **801E4C40** loads record-22 name **[804D26A4]=804D24D4**, spelling **RenderLevel_PostEffect** (O/:73-91; DM:339-343,479-484). Its selector -1 is NOT used to choose a phase. It is Static's first child, before loop-created levels 14..17.

## 5. Div's two-view traversal, then Static's reset

Containing function **801E4EF0..801E506C** saves incoming f1, reads mode **u32(8057E760+0x60)=u32(8057E7C0)**, and initializes view index +0x70 to zero (D:489901-489928). The table above proves this is Div's Exec, not an adjacent unlinked routine.

| Mode | Within Div Exec | After Div Exec returns to generic walker |
|---|---|---|
| 0 | Sets selector +0x6C=0; calls **8027211C at 801E4F38**, then **80271F9C at 801E4F58** with selection from +0x64. No internal child traversal. | Ordinary **8004EDB0** visits children once. |
| 1 | Selector 1: calls at **801E4F74/801E4F94**; **801E4FA0 -> 8004ECAC(this,f1)** visits children. Then selector 2 and increments +0x70; calls at **801E4FC0/801E4FD4**. | Ordinary **8004EDB0** visits children a second time, for the second selected view. |
| 2 | Selector 3: calls at **801E4FF0/801E5010**; **801E501C -> 8004ECAC(this,f1)** visits children. Then selector 4 and increments +0x70; calls at **801E503C/801E5050**. | Same second traversal. |

D:489929-489995; BR:65053,65059. `8027211C` actually updates current selector/cache `[r13+4D1C]/[r13+4D18]` when changed (D:634424-634446); +0x64/+0x68 select camera-parameter inputs to 80271F9C, not separate blur histories. Other mode values fall through without these setups; no unsupported-mode camera guarantee is made.

**Static Exec 801E4E40** resets +0x70 and +0x6C to 0 at **801E4E64/6C**, calls **8027211C at 801E4E70** with 0, then **80271F9C at 801E4E90**, and returns without internal recursion (D:489857-489882). The walker then visits PostEffect/Save once. Consequently modes 1/2 can give **two DrawBlur task callbacks before the later Save**, both using the pre-save history; the core's guards can still suppress either overlay. This is not two independent per-camera captures.

## 6. Initialization precedes registration; final bridges to established core

- Setup slice **80178520-80178548** gets task group 16, allocates and calls **801D2498**; within that containing constructor **801D24DC -> 801D1A1C** creates the effect bundle (D:378680-378691,470807-470835; BR:50983,62639). The enclosing function of the 80178520 setup slice is not renamed or boundary-inferred here.
- In **801D1A1C**, examined initialization slice **801D1A38-801D1B14** first calls provider setup virtual +8 at **801D1A60**, then **801D1A68 -> 80433638** (shared scratch), then **801D1A80 -> 8042F20C** with null initial parameters (history/state). Create calls **8042EA50 at 8042F23C** and stores the allocated pointer (r29), not a checked construction-success result, at manager+0x40 at **8042F240** (D:470143-470198; O/8042F20C_MotionBlur_Create.asm:11-18). This proves call order, not resource/camera success.
- Registration then calls **801D213C at 801D1AD4** (earlier task named PostGlare), **801D2058 at 801D1AF0** (DrawBlur), and **801D1FF4 at 801D1B0C** (later task named DrawScreenColor). The adjacent tasks independently append to R+0x2C at **801D2160** and R+0x58 at **801D2018** (D:470592-470616,470510-470534; `motion_blur_analysis/strings.tsv:19490-19493`), so Save precedes that later color task as well. DrawBlur's own constructor gets R, appends beneath **R+0x2C at 801D207C**, installs **8053DC78 at 801D208C**, then constructs SaveScreen at **801D20B4**. SaveScreen separately appends beneath **R+0x58 at 801D20FC**, installs **8053DC5C at 801D210C**. Draw's task+0x28 ownership pointer is NOT a parent-child scheduling edge (O/801D2058_DrawBlurTask_Construct.asm:10-30; O/801D20D8_SaveScreenTask_Construct.asm:10-24).
- Actual execution words **[8053DC84]=801D27FC**, **[8053DC68]=801D2764** (DM:23-53). **801D280C -> 8042F16C**, then **8042F184 -> 8042E254**; separately **801D2774 -> 8042F140**, then **8042F158 -> 8042E838**. Both task callbacks first call the same manager getter 800A35D8; both manager bridges test manager+0x40 for null (the four corresponding O/ Exec/ManagerDraw/ManagerSaveScreen files :5-15; BR:62659,62663,138315-138316).
- Core boundary only: Draw tests enable/alpha/current camera/ready at **8042E268-8042E2C8**; Save clears ready at **8042E850** before testing enable/alpha/current camera at **8042E854-8042E8A8** (respective O/ Draw.asm:10-34 and SaveScreen.asm:8-33). Scheduler reachability is not proof of pixels drawn/saved. Established geometry/GX/copy internals are intentionally not re-audited in this pass.

## 7. Limits, exact residual obligations, and reproducibility

**No required static edge remains unresolved between the recurring 800510C0 loop and the two blur cores under the constructed tables/tree.** Initialization failure, runtime task mutation, callback replacement, live mode/flags, and successful GPU/camera operations remain conditional. The exact unproved stronger edge is **a traversal beginning at 801E23A4 / core entries 8042E254 and 8042E838 -> a particular VI field/XFB presentation**. No captured trace is supplied; the initialization/transition scheduler calls and two-view recursion specifically prevent equating every task invocation with one displayed frame.

Raw supplements executed without export (start inclusive, end exclusive):
- `python -B analysis/motion_blur/tools/dol_evidence.py data 0x8053ED20 0x8053ED84` — group names/order; decisive `[8053ED34]=805F7150`, `[8053ED6C..8053ED80]=0..5`.
- `... data 0x8051E768 0x8051E778` — base task vtable; `[8051E774]=8004EA80`.
- `... data 0x8051E5E0 0x8051E62C` — application-event dispatch; `[8051E628]=80049078`.
- `... data 0x805F7150 0x805F7158` — `52656E64 65720000`, ASCII `Render`.

Other shell commands were `pwd`, workspace `ls`, the read-only verifier above, and two `python -B -c` arithmetic-only VA-to-line conversion commands. Evidence acquisition used bounded reads and targeted BR/manifest searches, not oversized-file grep. No export, patch, build, configuration change, emulator, or runtime test was performed. Only `analysis/motion_blur/evidence/03_closure.md` was created.

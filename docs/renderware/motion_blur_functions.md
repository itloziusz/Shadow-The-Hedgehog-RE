# Motion blur — function and source-coverage index

## Reading this index

Target: original `main.dol`, 5,773,024 bytes, SHA-256 `fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`. All ranges are local VA, **start inclusive/end exclusive**; sizes/addresses below are hexadecimal. Names are evidence-backed analysis aliases, not recovered source symbols. A case, call instruction or source factoring helper is not another original function.

This index contains **121 distinct assembly links/ranges: 19 core/task entries, 11 input containers and 91 shared dependencies**. The [address manifest](evidence/address_manifest.json) records per-range hash, file offset, outgoing and incoming direct branches; the [atlas](motion_blur_original_addresses.md) provides numeric offsets plus all80 data records. Verification passes **10,075 instruction words / 3,908 file-backed data bytes**. Direct-branch enumeration is not a complete enumeration of computed callers.

**Source key:** **C** = complete original-body mapping in the integrated CPP; **S** = explicitly selected original path/block, not a complete containing entry; **A** = assembly/analysis and/or real-address API boundary, no reconstructed body in this CPP. C does not imply linked/binary/fault/runtime equivalence. Source links use checked current definition lines in the **2,393-line CPP**, not the old integration report's lines. Header has456 lines. Core/task block1–808; backend810–1814; input1816–2393.

Core/task has19 C entries. Backend supplies **13 address-bearing definitions:4 C/9 S**. Input supplies **13 definitions:2 C/11 selected definitions**; three selected ramp definitions share **one original update function**, so input occupies11 original ranges here. Do not sum source definitions as distinct original functions. Most shared calls remain external SDK/ABI bindings; no generic callbacks are installed by including the source. **H static confidence** applies to authenticated bytes and the stated domain, not execution. Additional bounded closure windows are identified separately in §5.

## 1. Complete effect core: 13 functions, 0x1004 bytes

| Original function / assembly | Range / size | Source |
|---|---|---|
| [MotionBlur_Draw](evidence/original/8042E254_MotionBlur_Draw.asm) | 8042E254..8042E838 / 0x5E4 | C — [CPP168](reconstructed/motion_blur.cpp#L168) |
| [MotionBlur_SaveScreen](evidence/original/8042E838_MotionBlur_SaveScreen.asm) | 8042E838..8042E9C4 / 0x18C | C — [CPP319](reconstructed/motion_blur.cpp#L319) |
| [MotionBlur_CopyParameters](evidence/original/8042E9C4_MotionBlur_CopyParameters.asm) | 8042E9C4..8042E9F0 / 0x2C | C — [CPP363](reconstructed/motion_blur.cpp#L363) |
| [MotionBlur_AssignParameters](evidence/original/8042E9F0_MotionBlur_AssignParameters.asm) | 8042E9F0..8042EA1C / 0x2C | C — [CPP382](reconstructed/motion_blur.cpp#L382) |
| [MotionBlur_AssignParametersIfPresent](evidence/original/8042EA1C_MotionBlur_AssignParametersIfPresent.asm) | 8042EA1C..8042EA50 / 0x34 | C — [CPP403](reconstructed/motion_blur.cpp#L403) |
| [MotionBlur_Construct](evidence/original/8042EA50_MotionBlur_Construct.asm) | 8042EA50..8042F084 / 0x634 | C — [CPP428](reconstructed/motion_blur.cpp#L428) |
| [MotionBlur_GetParameters](evidence/original/8042F084_MotionBlur_GetParameters.asm) | 8042F084..8042F0E8 / 0x64 | C — [CPP590](reconstructed/motion_blur.cpp#L590) |
| [MotionBlur_SetParameters](evidence/original/8042F0E8_MotionBlur_SetParameters.asm) | 8042F0E8..8042F114 / 0x2C | C — [CPP622](reconstructed/motion_blur.cpp#L622) |
| [MotionBlur_SetParametersIfPresent](evidence/original/8042F114_MotionBlur_SetParametersIfPresent.asm) | 8042F114..8042F140 / 0x2C | C — [CPP636](reconstructed/motion_blur.cpp#L636) |
| [MotionBlur_ManagerSaveScreen](evidence/original/8042F140_MotionBlur_ManagerSaveScreen.asm) | 8042F140..8042F16C / 0x2C | C — [CPP648](reconstructed/motion_blur.cpp#L648) |
| [MotionBlur_ManagerDraw](evidence/original/8042F16C_MotionBlur_ManagerDraw.asm) | 8042F16C..8042F198 / 0x2C | C — [CPP659](reconstructed/motion_blur.cpp#L659) |
| [MotionBlur_Destroy](evidence/original/8042F198_MotionBlur_Destroy.asm) | 8042F198..8042F20C / 0x74 | C — [CPP673](reconstructed/motion_blur.cpp#L673) |
| [MotionBlur_Create](evidence/original/8042F20C_MotionBlur_Create.asm) | 8042F20C..8042F258 / 0x4C | C — [CPP697](reconstructed/motion_blur.cpp#L697) |

### Per-entry behavior, callers and dependencies

**8042E254 Draw.** Caller **8042F184** inside8042F16C. Calls **8042B620**, **803A1AFC**, provider virtual+24 (installed80028730), **8048AB7C**, **8048652C/80486504** and engine+24/+20/+30 (installed804984C4/80498954/80491E80). Reads B enable/alpha/scale/history/valid, current camera+60 view/root, provider805E25C8, engine805F265C, descriptors8056BCE4/BCD0, templates/constants8051C378..C3A0. Writes absolute current XY/UV and RGBA into B+14. One primitive4/count4; bind Hroot, blend RW5/6, unsupported12 request1, depth6/8 off, fog14 off, filter9=1. Issues **six implemented restores plus ignored12**, not raster1/allGX. No EFB copy/H/valid write. Begin failure occurs after snapshots/geometry. State12's saved word remains original stack residue.

**8042E838 Save.** Caller **8042F158** inside8042F140. Calls service/cast/provider+24, **80496208 twice**, two conditional Begin/End pairs and engine state/primitive slots. Reads B+10 -> M+34 -> Sroot, B+74 Hview, B+7C/B+DC quads; transitively clear805F2688, mode805F26F4, pluginOffset805F2700. Writes valid0 before gates; copy sites **8042E8C0/E968** both pass selector0. Optional first block binds scratch, requests RW2/1 replacement and draws half quad; optional second draws full quad **without rebind**. Valid1 at **8042E9AC** even if either/both Begin calls fail. No requested-state snapshot/restore or guaranteed pixel restoration.

**8042E9C4 CopyParameters.** Called by getter at **8042F0A4**; no callees/globals/GX. Destination r3, source r4; transfers bytes0/1, LFS/STFS scale4 and words8/C only, preserving original order. No padding copy, clamp, canonicalization or valid reset.

**8042E9F0 AssignParameters.** Called by setter at **8042F100**; no callees/globals/GX. Same five fields, its own load/store ordering; source is not null-checked. Not a whole-record memcpy.

**8042EA1C AssignParametersIfPresent.** Called at **8042F048** (constructor) and **8042F12C** (nullable-source bridge); no other callees/globals/GX. Null source is a no-op, not default initialization/reset; otherwise same five fields.

**8042EA50 Construct.** Caller **8042F23C**. Calls service/cast/provider+24, **8048AEC4 twice**, **8048AE14**, optionallyEA1C. Allocated B size0x13C; manager at+10, three4x0x18 arrays, history at+74, valid+78. Uses camera **root** dimensions, near-Z via four engine+18 loads, templates8051C358..C3A4. Prefix e0/a0/scale1/owner precedes camera lookup; failed lookup returns partial B before +74/+78/vertices. On the camera-success path, builds full/half capture geometry and history view/root (flags0x505), signed halving toward zero, no pixel initialization. Default completion e0/a128/scale1; auxiliary/padding bytes remain unwritten. Not all raster results are checked. Original first-vertex/height-FP interleaving is retained; **24/24 optimized-IR events match**, per [resolved audit](evidence/source_audit_resolution.md). No direct primitive/copy in this body.

**8042F084 GetParameters.** Direct callers **80204720,802055E8,80206320,8043A908**. CallsE9C4; output in r3, manager in r4. M+40 absent: enabled=alpha=0 and scale1 from8051C38C, but output8/C receives unwritten original SP+10/+14 residue. No GPU work or history reset; source models indeterminacy, not actual original stack addresses/bits.

**8042F0E8 SetParameters.** Direct callers **8020475C,80205624,8020635C,80439FD8,8043A544**. CallsE9F0 if M+40 nonnull. Manager r3/source r4; source not guarded. No direct global/GX; prefix-only write, not invalidation.

**8042F114 SetParametersIfPresent.** Direct callers **800A356C,800A375C,800A481C**. CallsEA1C if M+40 nonnull; underlying helper tolerates null source. Manager itself unguarded. No global/GX/reset.

**8042F140 ManagerSaveScreen.** Caller **801D2774**. Loads/guards M+40 and callsSave at **8042F158**. No separate globals or state policy; capture is transitive.

**8042F16C ManagerDraw.** Caller **801D280C**, a BL instruction, not entry. Loads/guards M+40 and callsDraw at **8042F184**. No separate globals or render policy.

**8042F198 Destroy.** Stage caller **801D19B8**. Calls **8048AC2C** on distinct history root then view, reloads B+74 after root destruction, clears it, frees B through **803A1334**, clears M+40. No direct globals/primitive/copy. GPU-use synchronization belongs to raster destruction. Does not repair a partially constructed pointer or free shared scratch here.

**8042F20C Create.** Setup caller **801D1A80**. Calls **803A1380(0x13C)** thenEA50 if allocation succeeds; stores the **allocation**, not a tested constructor-success result, at M+40. No old-state destruction or whole-object clear. Raster creation is transitive; no direct GX operation.

## 2. Complete effect tasks: six functions, 0x1F8 bytes

| Original function / assembly | Range / size | Source |
|---|---|---|
| [DrawBlurTask_Construct](evidence/original/801D2058_DrawBlurTask_Construct.asm) | 801D2058..801D20D8 / 0x80 | C — [CPP715](reconstructed/motion_blur.cpp#L715) |
| [SaveScreenTask_Construct](evidence/original/801D20D8_SaveScreenTask_Construct.asm) | 801D20D8..801D213C / 0x64 | C — [CPP735](reconstructed/motion_blur.cpp#L735) |
| [SaveScreenTask_Destroy](evidence/original/801D270C_SaveScreenTask_Destroy.asm) | 801D270C..801D2764 / 0x58 | C — [CPP751](reconstructed/motion_blur.cpp#L751) |
| [SaveScreenTask_Exec](evidence/original/801D2764_SaveScreenTask_Exec.asm) | 801D2764..801D2788 / 0x24 | C — [CPP768](reconstructed/motion_blur.cpp#L768) |
| [DrawBlurTask_Destroy](evidence/original/801D2788_DrawBlurTask_Destroy.asm) | 801D2788..801D27FC / 0x74 | C — [CPP782](reconstructed/motion_blur.cpp#L782) |
| [DrawBlurTask_Exec](evidence/original/801D27FC_DrawBlurTask_Exec.asm) | 801D27FC..801D2820 / 0x24 | C — [CPP802](reconstructed/motion_blur.cpp#L802) |

**801D2058 Draw constructor.** Caller801D1AF0 after0x30 allocation at801D1AE4. Calls800461A0/8004F014, allocates0x28 at801D20A8 via803A1380, calls801D20D8 at801D20B4. Stores name804D0F18, vtable8053DC78 at task+18, ORs flags+4 with0x100, keeps Save at+28. ParentR+2C is level11 PostGlare under Div. No GPU work/history allocation here.

**801D20D8 Save constructor.** Caller801D20B4; calls800461A0/8004F014. Stores name804D0F24, vtable8053DC5C, flags0x100; parentR+58 is **early Static/PostEffect, not the Draw task**. No direct GX.

**801D270C Save destructor.** Indirect caller word**8053DC64**, no direct incoming branch in the manifest. Reinstalls8053DC5C, null-this guard, calls8004EF18(this,0), optional803A1334 if signed low16 delete flag>0. Does not free B/H; no direct GPU work.

**801D2764 Save Exec.** Indirect word**8053DC68**, generic walker8004ECAC/profile8004EDF0. Calls800A35D8 at801D2770 and8042F140 at801D2774, ignoring incoming task for effect state. Manager cache805EF2D8 is transitive; capture belongs to coreSave, not another kernel.

**801D2788 Draw destructor.** Indirect word**8053DC80**. Reinstalls8053DC78; if Save+28 exists, ORs deletion bit0 into Save flags. Calls8004EF18(this,0), optional803A1334 on signed-low16>0. Does not free H or clear its Save pointer; task lifetime is distinct from manager image lifetime.

**801D27FC Draw Exec.** Indirect word**8053DC84**; walker/profile as above. Calls800A35D8 at801D2808,8042F16C at801D280C. Can execute twice before Save in Div modes1/2; no per-task/per-camera H. No direct GX beyond transitive coreDraw.

## 3. Input containers: 11 original ranges

The full original ranges are exported even where only selected blur blocks have source. All numeric predicates/preceding effects are in [12_closure](evidence/12_closure.md) and [dataflow](motion_blur_dataflow.md). No gameplay-label inference or fake non-blur tail is included.

| Original function / assembly | Range / size | Source scope and principal dependencies |
|---|---|---|
| [DisableFixedParameters](evidence/original/800A3528_MotionBlur_DisableFixedParameters.asm) | 800A3528..800A35D8 / 0xB0 | **S [CPP1897](reconstructed/motion_blur.cpp#L1897)**, prefix through356C setter, continuation3570. Caller800A2E28; getter800A35D8/F114; enable0/alpha128/scale805F3BD0, auxiliary residue. |
| [EnableFixedParameters](evidence/original/800A3714_MotionBlur_EnableFixedParameters.asm) | 800A3714..800A37A4 / 0x90 | **S [CPP1925](reconstructed/motion_blur.cpp#L1925)**, prefix through375C, continuation3760. Caller800A2E4C; same scale/alpha, enable1. |
| [ApplyRamp](evidence/original/800A47B0_MotionBlur_ApplyRamp.asm) | 800A47B0..800A4830 / 0x80 | **C [CPP1950](reconstructed/motion_blur.cpp#L1950)**. Caller800A48C4; reads T+38,805F3BD0/BDC/BE0; getter/F114 at481C. Alpha low8(FCTIWZ(64*x)), enable!(LT or EQ). |
| [RampTaskUpdate](evidence/original/800A4830_MotionBlur_RampTaskUpdate.asm) | 800A4830..800A492C / 0xFC | **S [CPP1994](reconstructed/motion_blur.cpp#L1994), [2025](reconstructed/motion_blur.cpp#L2025), [2034](reconstructed/motion_blur.cpp#L2034)**: advance, actual submit, post-callback retirement; continuations48A4/48C8/4918. Vslot80522A84. f1/0.5; T+34/38; calls800A47B0 plus omitted8007993C/8016F4B0 effects between slices. UN skips both clamps. |
| [UpdateProgress](evidence/original/8042B63C_EffectParameter_UpdateProgress.asm) | 8042B63C..8042B6CC / 0x90 | **C [CPP2053](reconstructed/motion_blur.cpp#L2053)**. Caller80439AA0; leaf. E+18 flags, age+C,duration104,t100; EPS8051BFA4/unitBFA8. StrictGT expiry clears active without t update. |
| [GlobalParam_Apply](evidence/original/80439A8C_EffectGlobalParam_Apply.asm) | 80439A8C..8043A4B8 / 0xA2C | **S [CPP2181](reconstructed/motion_blur.cpp#L2181)**: common progress, guards/cast/dispatch and kind1 block9EBC..9FE0, not other kinds/tail. Caller801FA600; setter80439FD8. E+14C/+15C, t100; snap8051CB58, unitCB5C; FMULS then FMADDS. |
| [GlobalParam_Destroy](evidence/original/8043A4B8_EffectGlobalParam_Destroy.asm) | 8043A4B8..8043A5A4 / 0xEC | **S [CPP2272](reconstructed/motion_blur.cpp#L2272)**: null/vptr/cast/kind guards and target writeA540..A54C; no common destructor/free replacement. Caller801FA8A8; setter8043A544; E+15C target, not start. |
| [GlobalParam_Construct](evidence/original/8043A5A4_EffectGlobalParam_Construct.asm) | 8043A5A4..8043AB68 / 0x5C4 | **S [CPP2124](reconstructed/motion_blur.cpp#L2124)**: A740..A7A4 guard/dispatch plus A900..A97C snapshot/target, not whole constructor. Calls8042B734/RTTI/F084 atA908; descriptor+2C/30/34 -> E+FC/104/108; resource kind1. |
| [DisableBlur_ControllerA](evidence/original/802046BC_DisableBlur_ControllerA.asm) | 802046BC..80204968 / 0x2AC | **S [CPP2327](reconstructed/motion_blur.cpp#L2327)**:470C..4760; preceding side effect8042659C, three distinct800A35D8 lookups, getter **80204720**, setter8020475C. Incoming self+4==0 and80044E38()==1 are containing-path predicates. |
| [DisableBlur_ControllerB](evidence/original/8020554C_DisableBlur_ControllerB.asm) | 8020554C..80205E3C / 0x8F0 | **S [CPP2352](reconstructed/motion_blur.cpp#L2352)**:55D4..5628; same side-effect/getter/setter structure. Predicate self+1C0, selector1, wrapping self+4 minus8058E76C becomes signed<=0. Table80544E74 closes state0 dispatch. |
| [DisableBlur_ControllerC](evidence/original/802062AC_DisableBlur_ControllerC.asm) | 802062AC..80206534 / 0x288 | **S [CPP2375](reconstructed/motion_blur.cpp#L2375)**:630C..6360, actual side-effect and repeated lookups; getter80206320/setter8020635C. Predicate as A with its distinct earlier calls. |

None has direct GX work. All parameter writes leave B+78 unchanged. Input continuations are **source annotations, not original return values or executable PC resumption**. Outer factory type8/resource kind1, ramp zero-start/vtable, immediate actor flags and fixed/decrease wrapper paths are closed supplementary evidence in role12, not additional source implementations.

## 4. Shared dependencies: 91 exported ranges

A = original assembly and scoped contract only. C/S markers explicitly identify backend source; their real SDK/ABI callees remain external. Shared whole-function exports do not turn unrelated callers/arms into motion blur.

### 4.1 Service, scheduling, provider and construction context (22)

| Function / assembly | Range / size | Scope / required edge or storage |
|---|---|---|
| [EffectService_Get](evidence/original/8042B620_EffectService_Get.asm) | 8042B620..8042B630 / 0x10 | A; load provider805E25C8 for core/scratch camera resolution |
| [EffectService_Set](evidence/original/8042B630_EffectService_Set.asm) | 8042B630..8042B63C / 0xC | A; store service; install call801E28EC, no ownership/refcount inferred |
| [Runtime_DynamicCast](evidence/original/803A1AFC_Runtime_DynamicCast.asm) | 803A1AFC..803A1D60 / 0x264 | A; type-name/base-list comparison/adjustment; core8056BCD0/BCE4, scratchBDB4/BDC8 |
| [RenderLevels_Construct](evidence/original/801E4B0C_RenderLevels_Construct.asm) | 801E4B0C..801E4E40 / 0x334 | A; table804D24EC, Start/Div/Static/End order, PostGlare11/earlyPostEffect22 |
| [EffectParameter_BaseConstruct](evidence/original/8042B734_EffectParameter_BaseConstruct.asm) | 8042B734..8042B7C0 / 0x8C | A; active-path descriptor flags/resource/duration transfer; earlier base80426F8C supplies age/flags/manager |
| [Main_EventLoop](evidence/original/800510C0_Main_EventLoop.asm) | 800510C0..80051230 / 0x170 | A; recurring800511EC event0x12, then8032D444, loop gate80576DB0+C |
| [Main_DispatchEvent](evidence/original/80051978_Main_DispatchEvent.asm) | 80051978..80051B18 / 0x1A0 | A; prefilter80050FA8; event0x12 forwards at800519A8 |
| [Application_DispatchEvent](evidence/original/80048EB8_Application_DispatchEvent.asm) | 80048EB8..800490B0 / 0x1F8 | A; table8051E5E0, case80049078 calls801E2210; case is not a new function |
| [Frame_DriveTasks](evidence/original/801E2210_Frame_DriveTasks.asm) | 801E2210..801E2480 / 0x270 | A; ordinary801E23A4 scheduler, initial/transition alternatives; flag805F0A54/delta805F0A58 |
| [TaskManager_Get](evidence/original/8001C4F8_TaskManager_Get.asm) | 8001C4F8..8001C564 / 0x6C | A; lazy K80571C6C, constructor801EB734, cache805EF0D0 |
| [TaskManager_Construct](evidence/original/801EB734_TaskManager_Construct.asm) | 801EB734..801EB790 / 0x5C | A; parentless root atK+0; group construction at801EB76C |
| [TaskManager_ConstructGroups](evidence/original/801EB53C_TaskManager_ConstructGroups.asm) | 801EB53C..801EB5D4 / 0x98 | A; six groups appended using8053ED6C order0..5; Render name805F7150 |
| [TaskManager_GetGroup](evidence/original/801EB614_TaskManager_GetGroup.asm) | 801EB614..801EB624 / 0x10 | A; indexed K+4+4*r4, group5 atK+18 |
| [TaskManager_Execute](evidence/original/801EB680_TaskManager_Execute.asm) | 801EB680..801EB6AC / 0x2C | A; null-check root then801EB698 ->8004ECAC |
| [Task_AppendConstruct](evidence/original/8004F014_Task_AppendConstruct.asm) | 8004F014..8004F0F0 / 0xDC | A; intrusive tail append, task parent+10/children+14/sibling+C; base vptr8051E768 |
| [Task_TraverseChildren](evidence/original/8004ECAC_Task_TraverseChildren.asm) | 8004ECAC..8004EDF0 / 0x144 | A; virtual+0C Exec, children then sibling; skip/delete/pause/profile fields805EF270/274/27C |
| [RenderAttach_DivExec](evidence/original/801E4EF0_RenderAttach_DivExec.asm) | 801E4EF0..801E506C / 0x17C | A; vslot8053EA7C, mode8057E7C0; modes1/2 extra child traversal before outer recursion |
| [RenderAttach_StaticExec](evidence/original/801E4E40_RenderAttach_StaticExec.asm) | 801E4E40..801E4EA8 / 0x68 | A; vslot8053EA60; reset selector/index then8027211C/80271F9C before Save |
| [EffectBundle_Construct](evidence/original/801D1A1C_EffectBundle_Construct.asm) | 801D1A1C..801D1C30 / 0x214 | A; scratch801D1A68 before B801D1A80 before task registration; other effects excluded |
| [EffectSystemPJS_GetCamera](evidence/original/80028730_EffectSystemPJS_GetCamera.asm) | 80028730..80028754 / 0x24 | A; installed provider+24 at8053E960; calls8000DCC0, returns its field0 |
| [EffectSystemPJS_Construct](evidence/original/801E293C_EffectSystemPJS_Construct.asm) | 801E293C..801E2978 / 0x3C | A; final vptr8053E93C after base construction; RTTI805E9D98 |
| [CameraManager_Get](evidence/original/8000DCC0_CameraManager_Get.asm) | 8000DCC0..8000DD2C / 0x6C | A; normal8056FF1C, cache805EF068, constructor80272598 |

### 4.2 Camera callback path (10)

| Function / assembly | Range / size | Required API scope |
|---|---|---|
| [RwCamera_BeginUpdate](evidence/original/8048652C_RwCamera_BeginUpdate.asm) | 8048652C..80486554 / 0x28 | A; camera+18 callback at80486540; preserve failure return |
| [RwCamera_EndUpdate](evidence/original/80486504_RwCamera_EndUpdate.asm) | 80486504..8048652C / 0x28 | A; camera+1C callback at80486518 |
| [RwCamera_Construct](evidence/original/80486AA8_RwCamera_Construct.asm) | 80486AA8..80486C1C / 0x174 | A; default callbacks, plugins, raster+60 |
| [RwCamera_DefaultBegin](evidence/original/804863A4_RwCamera_DefaultBegin.asm) | 804863A4..80486410 / 0x6C | A; engine current camera, frame sync, engine+4C callback |
| [RwCamera_DefaultEnd](evidence/original/80486340_RwCamera_DefaultEnd.asm) | 80486340..804863A4 / 0x64 | A; engine+70, clear current camera on success |
| [RwWorld_CameraPluginConstruct](evidence/original/80461C98_RwWorld_CameraPluginConstruct.asm) | 80461C98..80461CF4 / 0x5C | A; offset805F25B0, saved callbacks extension+10/+14, installs wrappers |
| [RwWorld_CameraBegin](evidence/original/80461B6C_RwWorld_CameraBegin.asm) | 80461B6C..80461BB8 / 0x4C | A; current-world/counter work, forwards saved begin |
| [RwWorld_CameraEnd](evidence/original/80461BB8_RwWorld_CameraEnd.asm) | 80461BB8..80461BF4 / 0x3C | A; clear current-world, forwards saved end |
| [RwCamera_BackendBegin](evidence/original/804956F4_RwCamera_BackendBegin.asm) | 804956F4..80495CAC / 0x5B8 | A; viewport/scissor/pixel format/current-camera projection;805F26F8,805E428C,8056F4F0; no motion estimation |
| [RwCamera_BackendEnd](evidence/original/80495CAC_RwCamera_BackendEnd.asm) | 80495CAC..80495CBC / 0x10 | A; clears805F26F8 |

### 4.3 Raster, copy and scratch lifetime (12)

| Function / assembly | Range / size | Source/API scope and required storage |
|---|---|---|
| [RwRaster_GetOffset](evidence/original/8048AB7C_RwRaster_GetOffset.asm) | 8048AB7C..8048AB90 / 0x14 | A; signed halfwords+1C/+1E, not ancestor walk |
| [RwRaster_SubRaster](evidence/original/8048AE14_RwRaster_SubRaster.asm) | 8048AE14..8048AEC4 / 0xB0 | A; rectangle/offsets, **engine+78**, then view.root=parent.root; metadata, not second image |
| [RwRaster_Create](evidence/original/8048AEC4_RwRaster_Create.asm) | 8048AEC4..8048AFC0 / 0xFC | A; metadata allocation/self-root; engine+58 create, plugins |
| [RwRaster_ResolveFormat](evidence/original/80497458_RwRaster_ResolveFormat.asm) | 80497458..80497A70 / 0x618 | **S [CPP1686](reconstructed/motion_blur.cpp#L1686)**; 0x505 ->GX6/depth32/alpha1, plus observed empty flags5/depth0 branch; not all formats |
| [RwRaster_LevelSize](evidence/original/80496478_RwRaster_LevelSize.asm) | 80496478..804965A8 / 0x130 | **S [CPP1710](reconstructed/motion_blur.cpp#L1710)**; RGBA8 level0 only, depth32 word8056F628 ->80496548; align32(4*align4W*align4H) |
| [RwRaster_BackendCreate](evidence/original/80497CA8_RwRaster_BackendCreate.asm) | 80497CA8..80498058 / 0x3B0 | **S [CPP1730](reconstructed/motion_blur.cpp#L1730)**; selected roots/empty views, true allocator and error reports; owner+18/aligned pixels+1C, no pixel clear |
| [RwRaster_BackendDestroy](evidence/original/80498058_RwRaster_BackendDestroy.asm) | 80498058..8049819C / 0x144 | A; root ownership, use-token wait, matching texture unbind, free allocation base |
| [RwRaster_BackendSubRaster](evidence/original/804981B8_RwRaster_BackendSubRaster.asm) | 804981B8..804981E8 / 0x30 | **C [CPP1788](reconstructed/motion_blur.cpp#L1788)**; stride/depth/type/format metadata only; outer root/rectangle attachment external |
| [RwRaster_PluginRegister](evidence/original/80496440_RwRaster_PluginRegister.asm) | 80496440..80496478 / 0x38 | A; plugin size0x34/ID0x40C, byte offset stored805F2700 |
| [RwRaster_CopyEFB](evidence/original/80496208_RwRaster_CopyEFB.asm) | 80496208..80496410 / 0x208 | **C [CPP1020](reconstructed/motion_blur.cpp#L1020)**; all four depth offsets/error path, selector versus clear805F2688, GX copy/filter/sync/invalidation. Only8042E8C0/E968 of17 clients are blur. |
| [EffectManager_CreateScratch](evidence/original/80433638_EffectManager_CreateScratch.asm) | 80433638..80433754 / 0x11C | A; caller801D1A68; M+34 uses construction **view** dimensions; M+38 peer not blur history |
| [EffectManager_DestroyScratch](evidence/original/804335C8_EffectManager_DestroyScratch.asm) | 804335C8..80433638 / 0x70 | A; caller801D19D8 after H teardown; roots then views, clear+34/+38 |

### 4.4 Device, state and texture (9)

| Function / assembly | Range / size | Source/API scope |
|---|---|---|
| [RwDevice_GetGameCubeDevice](evidence/original/804960F0_RwDevice_GetGameCubeDevice.asm) | 804960F0..804960FC / 0xC | A; returns8056F50C, separate from standard8051D400 |
| [RwRenderState_Get](evidence/original/804984C4_RwRenderState_Get.asm) | 804984C4..804986B8 / 0x1F4 | **S [CPP1150](reconstructed/motion_blur.cpp#L1150)**; IDs1/6/8/9/10/11/12/14;12 no output write;8056FA58 |
| [RwRenderState_Set](evidence/original/80498954_RwRenderState_Set.asm) | 80498954..80498F74 / 0x620 | **S [CPP1176](reconstructed/motion_blur.cpp#L1176)**; same IDs/all input-word branches; cache805E42D0/T805F270C;12 unsupported;9 filter; no generic callback |
| [RwTexture_SetRaster](evidence/original/8049819C_RwTexture_SetRaster.asm) | 8049819C..804981B8 / 0x1C | **C [CPP1140](reconstructed/motion_blur.cpp#L1140)**; T+0 assignment then dirty flag assignment; offset805F2718 |
| [RwTexture_FlushState](evidence/original/80498854_RwTexture_FlushState.asm) | 80498854..80498910 / 0xBC | **S [CPP1292](reconstructed/motion_blur.cpp#L1292)**; control logic with scoped RGBA8 realization; alpha-flag-dependent inherited compare/Z-location |
| [RwRenderState_Open](evidence/original/804981E8_RwRenderState_Open.asm) | 804981E8..8049847C / 0x294 | A; initial cache/descriptor setup; initial filter2/U1/V1 not Save-time policy |
| [RwRenderState_Close](evidence/original/8049847C_RwRenderState_Close.asm) | 8049847C..804984C4 / 0x48 | A; shared texture-descriptor teardown |
| [RwTexture_RealizeGXTexture](evidence/original/8049AC88_RwTexture_RealizeGXTexture.asm) | 8049AC88..8049B568 / 0x8E0 | **S [CPP1604](reconstructed/motion_blur.cpp#L1604)**; nonnull/nonpaletted RGBA8/no-mip/map0 dirty/sampler/unchanged/both load paths;8056FC90/FCC8,805EEFF0,8056FCDC |
| [Rw_RenderModeSelect](evidence/original/80494CD4_Rw_RenderModeSelect.asm) | 80494CD4..80494EB4 / 0x1E0 | A; modes805688C0/88FC/8938 or custom805E4250 ->805F26F4; no universal numeric size |

### 4.5 Immediate 2D backend (3)

| Function / assembly | Range / size | Source/API scope |
|---|---|---|
| [Im2D_Prepare](evidence/original/8049148C_Im2D_Prepare.asm) | 8049148C..80491774 / 0x2E8 | **S [CPP1358](reconstructed/motion_blur.cpp#L1358)**; textured path, direct descriptors, channel/texgen/TEV0, viewport mode+4/+8,8056F13C/158 matrices; untextured arm excluded |
| [Im2D_RestoreProjectionViewport](evidence/original/80491774_Im2D_RestoreProjectionViewport.asm) | 80491774..80491B08 / 0x394 | **C [CPP1461](reconstructed/motion_blur.cpp#L1461)**; root projection-only and full conditional view/split-scissor branches;805E4218 same-call snapshot; not all-state restore |
| [Im2D_RenderPrimitive](evidence/original/80491E80_Im2D_RenderPrimitive.asm) | 80491E80..8049245C / 0x5DC | **S [CPP1541](reconstructed/motion_blur.cpp#L1541)**; strictly RW4/count4/textured;8056F130=98, GXBegin80396934,96 vertex bytes FIFOCC008000, restore80492438; no other primitives |

### 4.6 GX SDK and cache endpoints (35)

All **A**, real local SDK boundaries with original assembly, not linked SDK source. Numeric arguments/selected tables and inherited-state limits are detailed in [GX state](motion_blur_gx_state.md). Context pointer805FB758 initially805A2F60; context contents are mutable.

| Function / assembly | Range / size | Blur-relevant endpoint |
|---|---|---|
| [GXPixModeSync](evidence/original/80396568_GXPixModeSync.asm) | 80396568..8039658C / 0x24 | Pixel-mode BP synchronization after copy; not CopyTex or CPU wait |
| [GXSetTexCopySrc](evidence/original/80396C50_GXSetTexCopySrc.asm) | 80396C50..80396CCC / 0x7C | Source origin and size-minus-one BP49/4A |
| [GXSetTexCopyDst](evidence/original/80396D00_GXSetTexCopyDst.asm) | 80396D00..80396E30 / 0x130 | Root dimensions/format6/selector0, tile stride/control |
| [GXSetCopyFilter](evidence/original/80397228_GXSetCopyFilter.asm) | 80397228..80397430 / 0x208 | Disabled filter then mode-derived restore; fallback taps0,0,21,22,21,0,0 |
| [GXCopyTex](evidence/original/803975AC_GXCopyTex.asm) | 803975AC..80397738 / 0x18C | Sole direct caller804963B8; BP49/4A/4D/4B/52, independent clear |
| [GXSetNumChans](evidence/original/80397C80_GXSetNumChans.asm) | 80397C80..80397CBC / 0x3C | One channel |
| [GXSetChanCtrl](evidence/original/80397CBC_GXSetChanCtrl.asm) | 80397CBC..80397D6C / 0xB0 | Unlit vertex material on selected channel |
| [GX_GetTextureTileCounts](evidence/original/80397D6C_GX_GetTextureTileCounts.asm) | 80397D6C..80397E34 / 0xC8 | Format6 table805689D0 ->80397DA4;4x4/two32-byte units |
| [GXInitTexObj](evidence/original/80397E34_GXInitTexObj.asm) | 80397E34..80398080 / 0x24C | Root pixel address/dimensions/wrap/format6/no-mip descriptor |
| [GXInitTexObjLOD](evidence/original/803980C8_GXInitTexObjLOD.asm) | 803980C8..8039822C / 0x164 | Filter/LOD packing,805EED30/805FB7C0 constants |
| [GXSetTevOp](evidence/original/803992EC_GXSetTevOp.asm) | 803992EC..80399378 / 0x8C | Stage0 op0 wordsC008F8AF/C108F2F0; swap bits preserved |
| [GXSetAlphaCompare](evidence/original/80399778_GXSetAlphaCompare.asm) | 80399778..803997BC / 0x44 | Cached alpha tests; no unconditional permissive policy |
| [GXSetTevOrder](evidence/original/80399848_GXSetTevOrder.asm) | 80399848..803999E4 / 0x19C | Coord0/map0/COLOR0A0,80568B50 channel table |
| [GXSetNumTevStages](evidence/original/803999E4_GXSetNumTevStages.asm) | 803999E4..80399A0C / 0x28 | One stage |
| [GXSetBlendMode](evidence/original/80399D44_GXSetBlendMode.asm) | 80399D44..80399D98 / 0x54 | RW5/6 ->GX4/5; Save2/1 ->GX1/0; masks inherited |
| [GXSetColorUpdate](evidence/original/80399D98_GXSetColorUpdate.asm) | 80399D98..80399DC4 / 0x2C | Write-mask dependency, not explicit core reset |
| [GXSetAlphaUpdate](evidence/original/80399DC4_GXSetAlphaUpdate.asm) | 80399DC4..80399DF0 / 0x2C | Alpha-write dependency, not explicit core reset |
| [GXSetZMode](evidence/original/80399DF0_GXSetZMode.asm) | 80399DF0..80399E24 / 0x34 | Combined Draw policy1,7,0 under coherent cache |
| [GXInvalidateTexRegion](evidence/original/8039868C_GXInvalidateTexRegion.asm) | 8039868C..803987B8 / 0x12C | Region-derived BP66 invalidation after EFB copy |
| [GXInvalidateTexAll](evidence/original/803987B8_GXInvalidateTexAll.asm) | 803987B8..80398800 / 0x48 | BP66001000/66001100 all invalidation |
| [GXGetTexObjLODBias](evidence/original/8039824C_GXGetTexObjLODBias.asm) | 8039824C..8039828C / 0x40 | Preserve bias on sampler-only realization change |
| [GXGetTexObjBiasClamp](evidence/original/8039828C_GXGetTexObjBiasClamp.asm) | 8039828C..80398298 / 0xC | Preserve bias-clamp state |
| [GXGetTexObjEdgeLOD](evidence/original/80398298_GXGetTexObjEdgeLOD.asm) | 80398298..803982AC / 0x14 | Preserve edge LOD |
| [GXGetTexObjMaxAniso](evidence/original/803982AC_GXGetTexObjMaxAniso.asm) | 803982AC..803982B8 / 0xC | Preserve anisotropy |
| [GXLoadTexObjPreLoaded](evidence/original/803982C0_GXLoadTexObjPreLoaded.asm) | 803982C0..8039843C / 0x17C | Descriptor/FIFO texture-image address, map0 BP94 |
| [GXLoadTexObj](evidence/original/8039843C_GXLoadTexObj.asm) | 8039843C..80398490 / 0x54 | GX-context+4C8 region callback then preloaded loader |
| [GXSetProjectionv](evidence/original/8039A158_GXSetProjectionv.asm) | 8039A158..8039A1E4 / 0x8C | Install 2D or same-call restored projection vector |
| [GXGetProjectionv](evidence/original/8039A1E4_GXGetProjectionv.asm) | 8039A1E4..8039A22C / 0x48 | Snapshot incoming projection into805E4218 |
| [GXLoadPosMtxImm](evidence/original/8039A22C_GXLoadPosMtxImm.asm) | 8039A22C..8039A27C / 0x50 | Position matrix0 upload; target/Gekko semantics, not velocity |
| [GXSetCurrentMtx](evidence/original/8039A2CC_GXSetCurrentMtx.asm) | 8039A2CC..8039A300 / 0x34 | Matrix index0 selection |
| [GXSetViewportJitter](evidence/original/8039A444_GXSetViewportJitter.asm) | 8039A444..8039A49C / 0x58 | Field-dependent Y half-pixel adjustment |
| [GXSetViewport](evidence/original/8039A49C_GXSetViewport.asm) | 8039A49C..8039A4E4 / 0x48 | Ordinary viewport adapter |
| [GXSetScissor](evidence/original/8039A4E4_GXSetScissor.asm) | 8039A4E4..8039A55C / 0x78 | Exact conditional view/split rectangle arguments |
| [GXSetScissorBoxOffset](evidence/original/8039A55C_GXSetScissorBoxOffset.asm) | 8039A55C..8039A59C / 0x40 | Restore split-mode scissor origin offset |
| [DCInvalidateRange](evidence/original/80372508_DCInvalidateRange.asm) | 80372508..80372534 / 0x2C | CPU cache-line `dcbi`, **not pixel memset** |

## 5. Supplementary closure and machine boundaries

The following positively read dependencies are **not additional entries in the121-function atlas**: engine-open80487254..804874FC and system-driver/installer case804954F8..804955FC; base-task profile8004EDF0/destructor8004EF18 and singleton800461A0; allocator803A1380/deallocator803A1334; raster destructor8048AC2C; provider install801E28D0..28F0; effect-manager800A35D8/360C/36CC; ramp creation800A34C8/800A4AB4 and immediate wrappers; resource factory80433798/type8; side-effect8042659C; teardown stage windows801D19B8/19D8; token endpoints80396428/803964DC/8049B7DC. [03](evidence/03_closure.md), [04](evidence/04_closure.md), [05](evidence/05_closure.md), [12](evidence/12_closure.md) distinguish fresh bounded queries and legacy reads. No full unrelated containing function is inferred from a case or call window.

Other real SDK boundaries include fog80399A0C/Z-location80399E24, descriptor/VAT/texgen APIs80395758/8039530C/80395790/80395FC8/80395D48, GXBegin80396934, VIGetNextField80380004 and error reporting8047EC78/8047EBD4. Their scoped arguments are decoded; they are not fabricated source bodies or a complete SDK extraction.

SetRaster ends at **804981B8**, GetState at **804986B8**, FlushState at **80498910**. Adjacent case blocks/functions must not be merged into oversized entries. The separate effect starting **8042F258**, other CopyEFB clients and unrelated resource kinds are outside blur scope.

`ppc::*`, vertex-copy/FIFO helpers, typed prefixes and `require_extracted_path` have **no original function address**. The last is a **non-original domain assertion** at CPP1100–1103. Input Continuation annotations do not execute omitted arms/tails or resume original PC/register/stack state. The [source audit resolution](evidence/source_audit_resolution.md) closes first-vertex FP/event order, not fault ABI or runtime equivalence. Strict syntax/default/O2 IR and23 synthetic tests pass; machine objects are blocked by installed LLVM, SDK/ABI binding remains external, and link/game/GPU behavior remains [UNVERIFIED](motion_blur_unknowns.md).

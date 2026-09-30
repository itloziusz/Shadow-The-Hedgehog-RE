# Motion blur — complete original address atlas

## Identity, mapping and scope

Original `main.dol`: **5,773,024 bytes (0x005816E0)**; SHA-256 **`fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af`**. Entry80003154. This atlas contains **all121 original function ranges and all80 data records** in the current [address manifest](evidence/address_manifest.json) / [data manifest](evidence/data_manifest.json), not just the reconstructed core. Verification authenticates **10,075 instruction words / 3,908 file-backed data bytes**. See [ledger](evidence/verification_ledger.json).

Every address/end/size/file-offset cell below is **hexadecimal**, even without `0x`. Ends are exclusive; file offsets are from byte0 of the unchanged DOL. Function file end is `file offset + size`. All121 functions are file-backed text1, with **fileOffset = VA - 80006740**. This relation is not a global relocation rule for data or startup text. Every function row links its actual assembly file; hashes and incoming/outgoing direct branches reside in the manifest. Names are analysis aliases, not recovered symbols or reference-map relocations.

Function source scope: **C** complete original-body mapping; **S** selected path/block(s), not a whole containing entry; **A** assembly/analysis or external API boundary only. **H** means high static confidence in authenticated bytes and the stated scoped role, not game execution or all possible callers. C/S classification agrees with [functions](motion_blur_functions.md). Current integrated source: CPP2,393/header456; core19 complete, backend13 definitions (4 complete/9 selected), input13 definitions (2 complete/11 selected). Several source definitions can map to one original function; this table counts each original range **once**.

Data backing: **I** initialized file bytes (possibly mutable), **Z** actual startup-zero-backed storage with **no file offset**. Type/role applies to the exported span, which can include padding, table entries or several adjacent constants rather than one source-language variable. **H/R** means high confidence in static slot/layout/use, but its live value is runtime-dependent. There are **61 I records / 19 Z records**; Z does not mean live-zero. Heap images and MMIO are not manufactured as file-backed records.

## 1. Original functions — 121 rows

| ID | Function / original assembly | VA start | VA end | Size | DOL file offset | Scope/confidence; required role |
|---|---|---|---|---|---|---|
| F001 | [DrawBlurTask_Construct](evidence/original/801D2058_DrawBlurTask_Construct.asm) | 801D2058 | 801D20D8 | 80 | 001CB918 | C/H; register Draw and own Save task |
| F002 | [SaveScreenTask_Construct](evidence/original/801D20D8_SaveScreenTask_Construct.asm) | 801D20D8 | 801D213C | 64 | 001CB998 | C/H; early Static/PostEffect registration |
| F003 | [SaveScreenTask_Destroy](evidence/original/801D270C_SaveScreenTask_Destroy.asm) | 801D270C | 801D2764 | 58 | 001CBFCC | C/H; task lifecycle, not pixel free |
| F004 | [SaveScreenTask_Exec](evidence/original/801D2764_SaveScreenTask_Exec.asm) | 801D2764 | 801D2788 | 24 | 001CC024 | C/H; manager capture bridge caller |
| F005 | [DrawBlurTask_Destroy](evidence/original/801D2788_DrawBlurTask_Destroy.asm) | 801D2788 | 801D27FC | 74 | 001CC048 | C/H; defer Save task deletion |
| F006 | [DrawBlurTask_Exec](evidence/original/801D27FC_DrawBlurTask_Exec.asm) | 801D27FC | 801D2820 | 24 | 001CC0BC | C/H; manager Draw bridge caller |
| F007 | [MotionBlur_Draw](evidence/original/8042E254_MotionBlur_Draw.asm) | 8042E254 | 8042E838 | 5E4 | 00427B14 | C/H; one scaled retained-image strip |
| F008 | [MotionBlur_SaveScreen](evidence/original/8042E838_MotionBlur_SaveScreen.asm) | 8042E838 | 8042E9C4 | 18C | 004280F8 | C/H; two copies, two optional quads |
| F009 | [MotionBlur_CopyParameters](evidence/original/8042E9C4_MotionBlur_CopyParameters.asm) | 8042E9C4 | 8042E9F0 | 2C | 00428284 | C/H; fields0/1/4/8/C only |
| F010 | [MotionBlur_AssignParameters](evidence/original/8042E9F0_MotionBlur_AssignParameters.asm) | 8042E9F0 | 8042EA1C | 2C | 004282B0 | C/H; prefix assignment |
| F011 | [MotionBlur_AssignParametersIfPresent](evidence/original/8042EA1C_MotionBlur_AssignParametersIfPresent.asm) | 8042EA1C | 8042EA50 | 34 | 004282DC | C/H; null-source no-op |
| F012 | [MotionBlur_Construct](evidence/original/8042EA50_MotionBlur_Construct.asm) | 8042EA50 | 8042F084 | 634 | 00428310 | C/H; B/history/fixed capture geometry |
| F013 | [MotionBlur_GetParameters](evidence/original/8042F084_MotionBlur_GetParameters.asm) | 8042F084 | 8042F0E8 | 64 | 00428944 | C/H; hidden output r3, manager r4 |
| F014 | [MotionBlur_SetParameters](evidence/original/8042F0E8_MotionBlur_SetParameters.asm) | 8042F0E8 | 8042F114 | 2C | 004289A8 | C/H; manager prefix bridge |
| F015 | [MotionBlur_SetParametersIfPresent](evidence/original/8042F114_MotionBlur_SetParametersIfPresent.asm) | 8042F114 | 8042F140 | 2C | 004289D4 | C/H; nullable-source bridge |
| F016 | [MotionBlur_ManagerSaveScreen](evidence/original/8042F140_MotionBlur_ManagerSaveScreen.asm) | 8042F140 | 8042F16C | 2C | 00428A00 | C/H; null-check M+40, capture |
| F017 | [MotionBlur_ManagerDraw](evidence/original/8042F16C_MotionBlur_ManagerDraw.asm) | 8042F16C | 8042F198 | 2C | 00428A2C | C/H; null-check M+40, composite |
| F018 | [MotionBlur_Destroy](evidence/original/8042F198_MotionBlur_Destroy.asm) | 8042F198 | 8042F20C | 74 | 00428A58 | C/H; H root/view then B free |
| F019 | [MotionBlur_Create](evidence/original/8042F20C_MotionBlur_Create.asm) | 8042F20C | 8042F258 | 4C | 00428ACC | C/H; allocation13C and install |
| F020 | [RwCamera_EndUpdate](evidence/original/80486504_RwCamera_EndUpdate.asm) | 80486504 | 8048652C | 28 | 0047FDC4 | A/H; callback adapter camera+1C |
| F021 | [RwCamera_BeginUpdate](evidence/original/8048652C_RwCamera_BeginUpdate.asm) | 8048652C | 80486554 | 28 | 0047FDEC | A/H; callback adapter camera+18 |
| F022 | [RwRaster_CopyEFB](evidence/original/80496208_RwRaster_CopyEFB.asm) | 80496208 | 80496410 | 208 | 0048FAC8 | C/H; complete copy wrapper, SDK external |
| F023 | [GXCopyTex](evidence/original/803975AC_GXCopyTex.asm) | 803975AC | 80397738 | 18C | 00390E6C | A/H; SDK EFB texture-copy FIFO |
| F024 | [GXSetBlendMode](evidence/original/80399D44_GXSetBlendMode.asm) | 80399D44 | 80399D98 | 54 | 00393604 | A/H; SDK blend-field insertion |
| F025 | [GXSetColorUpdate](evidence/original/80399D98_GXSetColorUpdate.asm) | 80399D98 | 80399DC4 | 2C | 00393658 | A/H; inherited write-mask boundary |
| F026 | [GXSetAlphaUpdate](evidence/original/80399DC4_GXSetAlphaUpdate.asm) | 80399DC4 | 80399DF0 | 2C | 00393684 | A/H; inherited alpha-write boundary |
| F027 | [GXSetZMode](evidence/original/80399DF0_GXSetZMode.asm) | 80399DF0 | 80399E24 | 34 | 003936B0 | A/H; depth compare/write realization |
| F028 | [GXPixModeSync](evidence/original/80396568_GXPixModeSync.asm) | 80396568 | 8039658C | 24 | 0038FE28 | A/H; pixel-mode BP sync, not CopyTex |
| F029 | [GXSetTexCopySrc](evidence/original/80396C50_GXSetTexCopySrc.asm) | 80396C50 | 80396CCC | 7C | 00390510 | A/H; source rectangle BP49/4A |
| F030 | [GXSetTexCopyDst](evidence/original/80396D00_GXSetTexCopyDst.asm) | 80396D00 | 80396E30 | 130 | 003905C0 | A/H; format/stride/selector packing |
| F031 | [GXSetCopyFilter](evidence/original/80397228_GXSetCopyFilter.asm) | 80397228 | 80397430 | 208 | 00390AE8 | A/H; copy sample/vfilter packing |
| F032 | [Im2D_Prepare](evidence/original/8049148C_Im2D_Prepare.asm) | 8049148C | 80491774 | 2E8 | 0048AD4C | S/H; textured setup only |
| F033 | [Im2D_RestoreProjectionViewport](evidence/original/80491774_Im2D_RestoreProjectionViewport.asm) | 80491774 | 80491B08 | 394 | 0048B034 | C/H; exact conditional restore |
| F034 | [Im2D_RenderPrimitive](evidence/original/80491E80_Im2D_RenderPrimitive.asm) | 80491E80 | 8049245C | 5DC | 0048B740 | S/H; textured RW4/count4 emitter |
| F035 | [RenderLevels_Construct](evidence/original/801E4B0C_RenderLevels_Construct.asm) | 801E4B0C | 801E4E40 | 334 | 001DE3CC | A/H; ordered render subtree |
| F036 | [RwRaster_GetOffset](evidence/original/8048AB7C_RwRaster_GetOffset.asm) | 8048AB7C | 8048AB90 | 14 | 0048443C | A/H; signed origin halfwords |
| F037 | [RwRaster_SubRaster](evidence/original/8048AE14_RwRaster_SubRaster.asm) | 8048AE14 | 8048AEC4 | B0 | 004846D4 | A/H; view/root attachment |
| F038 | [RwDevice_GetGameCubeDevice](evidence/original/804960F0_RwDevice_GetGameCubeDevice.asm) | 804960F0 | 804960FC | C | 0048F9B0 | A/H; descriptor8056F50C |
| F039 | [RwRenderState_Get](evidence/original/804984C4_RwRenderState_Get.asm) | 804984C4 | 804986B8 | 1F4 | 00491D84 | S/H; eight IDs,12 unsupported |
| F040 | [RwTexture_FlushState](evidence/original/80498854_RwTexture_FlushState.asm) | 80498854 | 80498910 | BC | 00492114 | S/H; RGBA8 realization boundary |
| F041 | [RwRenderState_Set](evidence/original/80498954_RwRenderState_Set.asm) | 80498954 | 80498F74 | 620 | 00492214 | S/H; eight IDs, not generic backend |
| F042 | [RwTexture_SetRaster](evidence/original/8049819C_RwTexture_SetRaster.asm) | 8049819C | 804981B8 | 1C | 00491A5C | C/H; binding/dirty assignment |
| F043 | [MotionBlur_ApplyRamp](evidence/original/800A47B0_MotionBlur_ApplyRamp.asm) | 800A47B0 | 800A4830 | 80 | 0009E070 | C/H; scalar -> enable/low8 alpha |
| F044 | [MotionBlur_RampTaskUpdate](evidence/original/800A4830_MotionBlur_RampTaskUpdate.asm) | 800A4830 | 800A492C | FC | 0009E0F0 | S/H; three source slices, one original |
| F045 | [MotionBlur_DisableFixedParameters](evidence/original/800A3528_MotionBlur_DisableFixedParameters.asm) | 800A3528 | 800A35D8 | B0 | 0009CDE8 | S/H; unconditional blur entry prefix |
| F046 | [MotionBlur_EnableFixedParameters](evidence/original/800A3714_MotionBlur_EnableFixedParameters.asm) | 800A3714 | 800A37A4 | 90 | 0009CFD4 | S/H; unconditional blur entry prefix |
| F047 | [EffectGlobalParam_Apply](evidence/original/80439A8C_EffectGlobalParam_Apply.asm) | 80439A8C | 8043A4B8 | A2C | 0043334C | S/H; progress/guards/kind1 only |
| F048 | [EffectGlobalParam_Destroy](evidence/original/8043A4B8_EffectGlobalParam_Destroy.asm) | 8043A4B8 | 8043A5A4 | EC | 00433D78 | S/H; kind1 target write, not shared tail |
| F049 | [EffectService_Get](evidence/original/8042B620_EffectService_Get.asm) | 8042B620 | 8042B630 | 10 | 00424EE0 | A/H; provider slot load |
| F050 | [EffectService_Set](evidence/original/8042B630_EffectService_Set.asm) | 8042B630 | 8042B63C | C | 00424EF0 | A/H; provider slot store |
| F051 | [EffectParameter_UpdateProgress](evidence/original/8042B63C_EffectParameter_UpdateProgress.asm) | 8042B63C | 8042B6CC | 90 | 00424EFC | C/H; flags/age/t, strictGT expiry |
| F052 | [RwRaster_Create](evidence/original/8048AEC4_RwRaster_Create.asm) | 8048AEC4 | 8048AFC0 | FC | 00484784 | A/H; metadata allocation/standard dispatch |
| F053 | [RwRaster_ResolveFormat](evidence/original/80497458_RwRaster_ResolveFormat.asm) | 80497458 | 80497A70 | 618 | 00490D18 | S/H; 0x505 and empty flags5 paths |
| F054 | [RwRaster_BackendCreate](evidence/original/80497CA8_RwRaster_BackendCreate.asm) | 80497CA8 | 80498058 | 3B0 | 00491568 | S/H; selected pixel ownership/empty/error paths |
| F055 | [RwRaster_BackendDestroy](evidence/original/80498058_RwRaster_BackendDestroy.asm) | 80498058 | 8049819C | 144 | 00491918 | A/H; use-token poll/unbind/free |
| F056 | [RwRaster_BackendSubRaster](evidence/original/804981B8_RwRaster_BackendSubRaster.asm) | 804981B8 | 804981E8 | 30 | 00491A78 | C/H; metadata callback |
| F057 | [RwRenderState_Open](evidence/original/804981E8_RwRenderState_Open.asm) | 804981E8 | 8049847C | 294 | 00491AA8 | A/H; initial cache/descriptor setup |
| F058 | [RwRenderState_Close](evidence/original/8049847C_RwRenderState_Close.asm) | 8049847C | 804984C4 | 48 | 00491D3C | A/H; descriptor teardown |
| F059 | [RwTexture_RealizeGXTexture](evidence/original/8049AC88_RwTexture_RealizeGXTexture.asm) | 8049AC88 | 8049B568 | 8E0 | 00494548 | S/H; RGBA8/no-mip/map0 only |
| F060 | [Rw_RenderModeSelect](evidence/original/80494CD4_Rw_RenderModeSelect.asm) | 80494CD4 | 80494EB4 | 1E0 | 0048E594 | A/H; default/custom mode selection |
| F061 | [GX_GetTextureTileCounts](evidence/original/80397D6C_GX_GetTextureTileCounts.asm) | 80397D6C | 80397E34 | C8 | 0039162C | A/H; SDK tile dimensions/units |
| F062 | [GXInitTexObj](evidence/original/80397E34_GXInitTexObj.asm) | 80397E34 | 80398080 | 24C | 003916F4 | A/H; SDK descriptor from pixel base |
| F063 | [GXInitTexObjLOD](evidence/original/803980C8_GXInitTexObjLOD.asm) | 803980C8 | 8039822C | 164 | 00391988 | A/H; sampler/LOD packing |
| F064 | [GXSetTevOp](evidence/original/803992EC_GXSetTevOp.asm) | 803992EC | 80399378 | 8C | 00392BAC | A/H; modulate table, preserve swaps |
| F065 | [GXSetAlphaCompare](evidence/original/80399778_GXSetAlphaCompare.asm) | 80399778 | 803997BC | 44 | 00393038 | A/H; cached alpha-test realization |
| F066 | [GXSetTevOrder](evidence/original/80399848_GXSetTevOrder.asm) | 80399848 | 803999E4 | 19C | 00393108 | A/H; stage texture/channel order |
| F067 | [GXSetNumTevStages](evidence/original/803999E4_GXSetNumTevStages.asm) | 803999E4 | 80399A0C | 28 | 003932A4 | A/H; stage count |
| F068 | [GXSetNumChans](evidence/original/80397C80_GXSetNumChans.asm) | 80397C80 | 80397CBC | 3C | 00391540 | A/H; color-channel count |
| F069 | [GXSetChanCtrl](evidence/original/80397CBC_GXSetChanCtrl.asm) | 80397CBC | 80397D6C | B0 | 0039157C | A/H; lighting/material source |
| F070 | [RwCamera_BackendBegin](evidence/original/804956F4_RwCamera_BackendBegin.asm) | 804956F4 | 80495CAC | 5B8 | 0048EFB4 | A/H; current-camera GX setup |
| F071 | [RwCamera_BackendEnd](evidence/original/80495CAC_RwCamera_BackendEnd.asm) | 80495CAC | 80495CBC | 10 | 0048F56C | A/H; clear backend current camera |
| F072 | [EffectManager_CreateScratch](evidence/original/80433638_EffectManager_CreateScratch.asm) | 80433638 | 80433754 | 11C | 0042CEF8 | A/H; camera-view-sized M+34 scratch |
| F073 | [EffectManager_DestroyScratch](evidence/original/804335C8_EffectManager_DestroyScratch.asm) | 804335C8 | 80433638 | 70 | 0042CE88 | A/H; shared scratch root/view free |
| F074 | [Runtime_DynamicCast](evidence/original/803A1AFC_Runtime_DynamicCast.asm) | 803A1AFC | 803A1D60 | 264 | 0039B3BC | A/H; name/base-list RTTI adjustment |
| F075 | [RwCamera_Construct](evidence/original/80486AA8_RwCamera_Construct.asm) | 80486AA8 | 80486C1C | 174 | 00480368 | A/H; default callback/plugin install |
| F076 | [RwCamera_DefaultBegin](evidence/original/804863A4_RwCamera_DefaultBegin.asm) | 804863A4 | 80486410 | 6C | 0047FC64 | A/H; engine+4C standard dispatch |
| F077 | [RwCamera_DefaultEnd](evidence/original/80486340_RwCamera_DefaultEnd.asm) | 80486340 | 804863A4 | 64 | 0047FC00 | A/H; engine+70 standard dispatch |
| F078 | [RwWorld_CameraBegin](evidence/original/80461B6C_RwWorld_CameraBegin.asm) | 80461B6C | 80461BB8 | 4C | 0045B42C | A/H; world-plugin forwarding wrapper |
| F079 | [RwWorld_CameraEnd](evidence/original/80461BB8_RwWorld_CameraEnd.asm) | 80461BB8 | 80461BF4 | 3C | 0045B478 | A/H; world-plugin forwarding wrapper |
| F080 | [RwWorld_CameraPluginConstruct](evidence/original/80461C98_RwWorld_CameraPluginConstruct.asm) | 80461C98 | 80461CF4 | 5C | 0045B558 | A/H; save/replace camera callbacks |
| F081 | [RwRaster_PluginRegister](evidence/original/80496440_RwRaster_PluginRegister.asm) | 80496440 | 80496478 | 38 | 0048FD00 | A/H; dynamic raster extension offset |
| F082 | [RwRaster_LevelSize](evidence/original/80496478_RwRaster_LevelSize.asm) | 80496478 | 804965A8 | 130 | 0048FD38 | S/H; RGBA8 level0 payload only |
| F083 | [EffectGlobalParam_Construct](evidence/original/8043A5A4_EffectGlobalParam_Construct.asm) | 8043A5A4 | 8043AB68 | 5C4 | 00433E64 | S/H; kind1 guards/snapshot/target |
| F084 | [DisableBlur_ControllerA](evidence/original/802046BC_DisableBlur_ControllerA.asm) | 802046BC | 80204968 | 2AC | 001FDF7C | S/H; selected side-effect/prefix block |
| F085 | [DisableBlur_ControllerB](evidence/original/8020554C_DisableBlur_ControllerB.asm) | 8020554C | 80205E3C | 8F0 | 001FEE0C | S/H; countdown-selected prefix block |
| F086 | [DisableBlur_ControllerC](evidence/original/802062AC_DisableBlur_ControllerC.asm) | 802062AC | 80206534 | 288 | 001FFB6C | S/H; selected side-effect/prefix block |
| F087 | [EffectParameter_BaseConstruct](evidence/original/8042B734_EffectParameter_BaseConstruct.asm) | 8042B734 | 8042B7C0 | 8C | 00424FF4 | A/H; descriptor flags/resource/duration |
| F088 | [Main_EventLoop](evidence/original/800510C0_Main_EventLoop.asm) | 800510C0 | 80051230 | 170 | 0004A980 | A/H; recurring event0x12 route |
| F089 | [Main_DispatchEvent](evidence/original/80051978_Main_DispatchEvent.asm) | 80051978 | 80051B18 | 1A0 | 0004B238 | A/H; prefilter/application dispatch |
| F090 | [Application_DispatchEvent](evidence/original/80048EB8_Application_DispatchEvent.asm) | 80048EB8 | 800490B0 | 1F8 | 00042778 | A/H; event0x12 case80049078 |
| F091 | [Frame_DriveTasks](evidence/original/801E2210_Frame_DriveTasks.asm) | 801E2210 | 801E2480 | 270 | 001DBAD0 | A/H; ordinary/initial/transition traversal |
| F092 | [TaskManager_Get](evidence/original/8001C4F8_TaskManager_Get.asm) | 8001C4F8 | 8001C564 | 6C | 00015DB8 | A/H; scheduler singleton K |
| F093 | [TaskManager_Construct](evidence/original/801EB734_TaskManager_Construct.asm) | 801EB734 | 801EB790 | 5C | 001E4FF4 | A/H; common root/group setup |
| F094 | [TaskManager_ConstructGroups](evidence/original/801EB53C_TaskManager_ConstructGroups.asm) | 801EB53C | 801EB5D4 | 98 | 001E4DFC | A/H; group order0..5/Render |
| F095 | [TaskManager_GetGroup](evidence/original/801EB614_TaskManager_GetGroup.asm) | 801EB614 | 801EB624 | 10 | 001E4ED4 | A/H; indexed K+4+4*i |
| F096 | [TaskManager_Execute](evidence/original/801EB680_TaskManager_Execute.asm) | 801EB680 | 801EB6AC | 2C | 001E4F40 | A/H; invoke common-root traversal |
| F097 | [Task_AppendConstruct](evidence/original/8004F014_Task_AppendConstruct.asm) | 8004F014 | 8004F0F0 | DC | 000488D4 | A/H; intrusive ordered append |
| F098 | [Task_TraverseChildren](evidence/original/8004ECAC_Task_TraverseChildren.asm) | 8004ECAC | 8004EDF0 | 144 | 0004856C | A/H; flags/Exec/recurse/sibling |
| F099 | [RenderAttach_DivExec](evidence/original/801E4EF0_RenderAttach_DivExec.asm) | 801E4EF0 | 801E506C | 17C | 001DE7B0 | A/H; one/two-view traversal |
| F100 | [RenderAttach_StaticExec](evidence/original/801E4E40_RenderAttach_StaticExec.asm) | 801E4E40 | 801E4EA8 | 68 | 001DE700 | A/H; selector reset before Save |
| F101 | [EffectBundle_Construct](evidence/original/801D1A1C_EffectBundle_Construct.asm) | 801D1A1C | 801D1C30 | 214 | 001CB2DC | A/H; scratch/B/task setup order |
| F102 | [EffectSystemPJS_GetCamera](evidence/original/80028730_EffectSystemPJS_GetCamera.asm) | 80028730 | 80028754 | 24 | 00021FF0 | A/H; provider virtual+24 target |
| F103 | [EffectSystemPJS_Construct](evidence/original/801E293C_EffectSystemPJS_Construct.asm) | 801E293C | 801E2978 | 3C | 001DC1FC | A/H; vptr8053E93C install |
| F104 | [CameraManager_Get](evidence/original/8000DCC0_CameraManager_Get.asm) | 8000DCC0 | 8000DD2C | 6C | 00007580 | A/H; field0 camera source manager |
| F105 | [GXInvalidateTexRegion](evidence/original/8039868C_GXInvalidateTexRegion.asm) | 8039868C | 803987B8 | 12C | 00391F4C | A/H; texture-region invalidation |
| F106 | [GXInvalidateTexAll](evidence/original/803987B8_GXInvalidateTexAll.asm) | 803987B8 | 80398800 | 48 | 00392078 | A/H; all-texture invalidation |
| F107 | [GXGetTexObjLODBias](evidence/original/8039824C_GXGetTexObjLODBias.asm) | 8039824C | 8039828C | 40 | 00391B0C | A/H; sampler-preservation getter |
| F108 | [GXGetTexObjBiasClamp](evidence/original/8039828C_GXGetTexObjBiasClamp.asm) | 8039828C | 80398298 | C | 00391B4C | A/H; sampler-preservation getter |
| F109 | [GXGetTexObjEdgeLOD](evidence/original/80398298_GXGetTexObjEdgeLOD.asm) | 80398298 | 803982AC | 14 | 00391B58 | A/H; sampler-preservation getter |
| F110 | [GXGetTexObjMaxAniso](evidence/original/803982AC_GXGetTexObjMaxAniso.asm) | 803982AC | 803982B8 | C | 00391B6C | A/H; sampler-preservation getter |
| F111 | [GXLoadTexObjPreLoaded](evidence/original/803982C0_GXLoadTexObjPreLoaded.asm) | 803982C0 | 8039843C | 17C | 00391B80 | A/H; texture descriptor/FIFO load |
| F112 | [GXLoadTexObj](evidence/original/8039843C_GXLoadTexObj.asm) | 8039843C | 80398490 | 54 | 00391CFC | A/H; region callback then load |
| F113 | [GXSetProjectionv](evidence/original/8039A158_GXSetProjectionv.asm) | 8039A158 | 8039A1E4 | 8C | 00393A18 | A/H; projection upload |
| F114 | [GXGetProjectionv](evidence/original/8039A1E4_GXGetProjectionv.asm) | 8039A1E4 | 8039A22C | 48 | 00393AA4 | A/H; same-call snapshot |
| F115 | [GXLoadPosMtxImm](evidence/original/8039A22C_GXLoadPosMtxImm.asm) | 8039A22C | 8039A27C | 50 | 00393AEC | A/H; matrix0 upload |
| F116 | [GXSetCurrentMtx](evidence/original/8039A2CC_GXSetCurrentMtx.asm) | 8039A2CC | 8039A300 | 34 | 00393B8C | A/H; select matrix0 |
| F117 | [GXSetViewportJitter](evidence/original/8039A444_GXSetViewportJitter.asm) | 8039A444 | 8039A49C | 58 | 00393D04 | A/H; field-jitter adapter |
| F118 | [GXSetViewport](evidence/original/8039A49C_GXSetViewport.asm) | 8039A49C | 8039A4E4 | 48 | 00393D5C | A/H; ordinary viewport adapter |
| F119 | [GXSetScissor](evidence/original/8039A4E4_GXSetScissor.asm) | 8039A4E4 | 8039A55C | 78 | 00393DA4 | A/H; conditional rectangle packing |
| F120 | [GXSetScissorBoxOffset](evidence/original/8039A55C_GXSetScissorBoxOffset.asm) | 8039A55C | 8039A59C | 40 | 00393E1C | A/H; split-scissor offset |
| F121 | [DCInvalidateRange](evidence/original/80372508_DCInvalidateRange.asm) | 80372508 | 80372534 | 2C | 0036BDC8 | A/H; CPU dcbi, not image zero fill |

## 2. Original data records — 80 rows

Record names exactly identify [data_manifest.json](evidence/data_manifest.json) entries; its raw hex, big-endian words and per-record hashes are authoritative. Z slots have no captured live value. Data size/file mappings use their own DOL sections, not the text1 formula.

| ID | Manifest record | VA start | VA end | Size | DOL file offset | Backing/type; role; confidence |
|---|---|---|---|---|---|---|
| D001 | DrawBlur_task_name | 804D0F18 | 804D0F21 | 9 | 004CDF18 | I char[9]; DrawBlur name; H |
| D002 | SaveScreen_task_name | 804D0F24 | 804D0F2F | B | 004CDF24 | I char[11]; SaveScreen name; H |
| D003 | SaveScreen_task_vtable | 8053DC5C | 8053DC6C | 10 | 0053AC5C | I pointer words; dtor801D270C/Exec801D2764; H |
| D004 | DrawBlur_task_vtable | 8053DC78 | 8053DC88 | 10 | 0053AC78 | I pointer words; dtor801D2788/Exec801D27FC; H |
| D005 | RenderWare_engine_global_pointer | 805F265C | 805F2660 | 4 | — | Z pointer32; live engine; H/R |
| D006 | MotionBlur_geometry_templates_and_constants | 8051C358 | 8051C3A4 | 4C | 00519358 | I zeros/f32/f64; 0.5/1/conversion biases, not B memset; H |
| D007 | RwBlend_to_GXBlend_table | 8056FA28 | 8056FA54 | 2C | 0056CA28 | I u32[11]; RW5/6 ->GX4/5; H |
| D008 | RwPrimitive_to_GXPrimitive_table | 8056F120 | 8056F13C | 1C | 0056C120 | I u32[7]; index4=98 strip; H |
| D009 | RwState_dispatch_table | 8056FAD4 | 8056FB50 | 7C | 0056CAD4 | I code pointers[31]; setter ID12 ->80498F58; H |
| D010 | Raster_copy_depth_dispatch_table | 8056F544 | 8056F5B8 | 74 | 0056C544 | I code pointers[29]; depth-4,32 ->80496358; H |
| D011 | Render_level_construction_table | 804D24EC | 804D26B8 | 1CC | 004CF4EC | I 23 x 0x14-byte records; record11 selector1,22 special name; H |
| D012 | Render_level_names | 804D22E8 | 804D24EC | 204 | 004CF2E8 | I strings/padding; PostGlare/PostEffect and ordering names; H |
| D013 | GameCube_RwDevice_table | 8056F50C | 8056F544 | 38 | 0056C50C | I mixed floats/pointers; copy to engine+10; H |
| D014 | Im2D_projection_vector | 8056F13C | 8056F158 | 1C | 0056C13C | I mutable f32[7]; width/height terms overwritten; H/R |
| D015 | Im2D_position_matrix | 8056F158 | 8056F188 | 30 | 0056C158 | I mutable f32[3][4]; diag1,1,-1, translations; H/R |
| D016 | Im2D_SDA_constants | 805FC3F8 | 805FC420 | 28 | 00581598 | I f32/f64; 0,1,2,-2,0.5/conversion biases; H |
| D017 | Startup_SDA_instruction_words | 8000332C | 8000333C | 10 | 0000032C | I 4 instruction words; r2=805FA780/r13=805EC500; H |
| D018 | Startup_zero_fill_table | 800055C8 | 800055E8 | 20 | 000025C8 | I address/size pairs; three actual zero spans/terminator; H |
| D019 | Effect_manager_cached_pointer | 805EF2D8 | 805EF2DC | 4 | — | Z pointer32; lazy manager cache; H/R |
| D020 | Effect_manager_blur_pointer | 805779CC | 805779D0 | 4 | — | Z pointer32; M+40 ->heap B; H/R |
| D021 | Effect_manager_screen_raster | 805779C0 | 805779C4 | 4 | — | Z pointer32; M+34 ->scratch view; H/R |
| D022 | Rw_copy_clear_flag | 805F2688 | 805F268C | 4 | — | Z u32; live low8 GXCopyTex clear, not selector; H/R |
| D023 | Rw_render_mode_pointer | 805F26F4 | 805F26F8 | 4 | — | Z pointer32; default/custom mode at use; H/R |
| D024 | Rw_raster_plugin_offset | 805F2700 | 805F2704 | 4 | — | Z u32 byte offset; raster extension, not pointer; H/R |
| D025 | Rw_current_texture_descriptor_pointer | 805F270C | 805F2710 | 4 | — | Z pointer32; current T, distinct from state cache; H/R |
| D026 | Blur_ramp_constants | 805F3BCC | 805F3BE4 | 18 | 00578D6C | I f32[6];1/1.05/.5/unused-.5/0/64; H |
| D027 | Blur_parameter_interpolation_constants | 8051CB58 | 8051CB78 | 20 | 00519B58 | I f32/f64; snap3F7FF972/unit1/biases/zeros; H |
| D028 | RwState_get_dispatch_table | 8056FA58 | 8056FAD4 | 7C | 0056CA58 | I code pointers[31]; ID12 ->804986B0 no output; H |
| D029 | GX_projection_type_and_viewport_constants | 805FB838 | 805FB848 | 10 | 005809D8 | I f32[4]; projection markers/jitter/hardware offset; H |
| D030 | Camera_begin_constants | 805FC428 | 805FC448 | 20 | 005815C8 | I f32/f64; camera projection conversion constants; H |
| D031 | Camera_projection_vector_template | 8056F4F0 | 8056F50C | 1C | 0056C4F0 | I mutable vector[7]; current camera, not previous frame; H/R |
| D032 | GX_context_pointer | 805FB758 | 805FB75C | 4 | 005808F8 | I pointer32 initially805A2F60; SDK mutable context; H/R |
| D033 | Effect_camera_provider_type_descriptors | 8056BCD0 | 8056BCEC | 1C | 00568CD0 | I RTTI/base links; core source/targetBCE4; H |
| D034 | Parameter_effect_progress_constants | 8051BFA4 | 8051BFAC | 8 | 00518FA4 | I f32[2]; EPS38D1B717/unit1; H |
| D035 | Rw_to_GX_filter_pairs | 8056FC90 | 8056FCC8 | 38 | 0056CC90 | I 7 u32 pairs; filter1 ->0/0,2 ->1/1; H |
| D036 | Rw_to_GX_wrap_table | 8056FCC8 | 8056FCDC | 14 | 0056CCC8 | I u32[5]={0,1,2,0,0}; inherited U/V lookup; H |
| D037 | GX_min_filter_encoding | 805EED30 | 805EED36 | 6 | 00577630 | I u8[6]={0,4,1,5,2,6}; BP encoding; H |
| D038 | GX_texture_LOD_constants | 805FB7C0 | 805FB7F8 | 38 | 00580960 | I f32/f64/padding; LOD/bias bounds and scales; H |
| D039 | Rw_texture_LOD_constants | 805FC498 | 805FC4A8 | 10 | 00581638 | I zero/padding/f64 unsigned bias; H |
| D040 | Rw_backend_color_fog_constants | 805FC448 | 805FC458 | 10 | 005815E8 | I FFFFFFFF/f32 5,10,.05; fog disable args5,10,.05,10; H |
| D041 | GX_default_render_modes | 805688C0 | 80568974 | B4 | 005658C0 | I 3 x 0x3C-byte mode descriptors; not live dimension guarantee; H/R |
| D042 | Rw_default_video_modes | 8056F408 | 8056F468 | 60 | 0056C408 | I 4 x 0x18-byte records; custom entry mutable; H/R |
| D043 | Rw_standard_callback_records | 8051D400 | 8051D4D8 | D8 | 0051A400 | I 27(index,target) pairs; camera/raster standard slots; H |
| D044 | Rw_driver_system_dispatch | 8056F494 | 8056F4F0 | 5C | 0056C494 | I code pointers[23]; op4 copy,op11 ->804954F8; H |
| D045 | GX_texture_tile_dispatch | 805689D0 | 80568AC4 | F4 | 005659D0 | I code pointers[61]; GX6 ->80397DA4; H |
| D046 | GX_texture_format_dispatch | 80568AC4 | 80568B00 | 3C | 00565AC4 | I code pointers[15]; format6 ->80397FF0; H |
| D047 | GX_TEV_operation_and_channel_tables | 80568B00 | 80568B70 | 70 | 00565B00 | I u32 programs/channel lookup; C008F8AF/C108F2F0; H |
| D048 | Render_root_phase_names | 804D26C8 | 804D2718 | 50 | 004CF6C8 | I strings/padding; Start/Div/Static/End; H |
| D049 | Render_attach_vtables | 8053EA54 | 8053EAC4 | 70 | 0053BA54 | I four task tables; DivExec801E4EF0/Static801E4E40; H |
| D050 | Camera_provider_vtables | 8053E8C0 | 8053E980 | C0 | 0053B8C0 | I base/derived pointers and base list; PJS8053E93C,+24=80028730; H |
| D051 | Camera_provider_RW3_vtable | 8051DB30 | 8051DB60 | 30 | 0051AB30 | I pointer words; RW3 base provider table; H |
| D052 | Scratch_camera_cast_descriptors | 8056BDB4 | 8056BDD0 | 1C | 00568DB4 | I duplicate RTTI/base links for scratch cast; H |
| D053 | Camera_provider_cast_type_names | 8051C318 | 8051C358 | 40 | 00519318 | I two type strings/padding; name-matched RW3 cast; H |
| D054 | Rw_texture_plugin_offset | 805F2718 | 805F271C | 4 | — | Z u32 byte offset; texture extension; H/R |
| D055 | Rw_world_camera_plugin_offset | 805F25B0 | 805F25B4 | 4 | — | Z u32 byte offset; saved world-camera callbacks; H/R |
| D056 | Rw_to_GX_fog_type_table | 8056FA18 | 8056FA28 | 10 | 0056CA18 | I u32[4]={0,2,4,5}; enabled-fog restore; H |
| D057 | Raster_default_depth_dispatch_entry | 8056F96C | 8056F970 | 4 | 0056C96C | I code pointer8049751C; empty scratch flags5 default; H |
| D058 | Raster_depth32_size_dispatch_entry | 8056F628 | 8056F62C | 4 | 0056C628 | I code pointer80496548; RGBA8 level size; H |
| D059 | Task_group_names_and_order | 8053ED20 | 8053ED84 | 64 | 0053BD20 | I pointer/u32 table; group5 Render/order0..5; H |
| D060 | Task_base_vtable | 8051E768 | 8051E778 | 10 | 0051B768 | I pointer words; Exec8004EA80 no-op body; H |
| D061 | Application_event_dispatch_table | 8051E5E0 | 8051E62C | 4C | 0051B5E0 | I code pointers[19]; event0x12 ->80049078; H |
| D062 | Render_group_name | 805F7150 | 805F7158 | 8 | 0057C2F0 | I string/padding; Render; H |
| D063 | Provider_RW3_RTTI | 805E9D88 | 805E9D90 | 8 | 00572688 | I name/base pointers; name804D2170 matches core target; H |
| D064 | Provider_PJS_RTTI | 805E9D98 | 805E9DA0 | 8 | 00572698 | I name/base pointers; name804D21B0/base8053E920; H |
| D065 | Camera_manager_cached_pointer | 805EF068 | 805EF06C | 4 | — | Z pointer32; camera-manager lazy cache; H/R |
| D066 | Camera_manager_camera_field | 8056FF1C | 8056FF20 | 4 | — | Z pointer32; camera-manager field0 selected camera; H/R |
| D067 | Ramp_task_vtable | 80522A78 | 80522A88 | 10 | 0051FA78 | I pointer words; Exec+0C=800A4830; H |
| D068 | Ramp_decrease_threshold | 805F3D18 | 805F3D1C | 4 | 00578EB8 | I f32 +0; immediate actor predicate threshold; H |
| D069 | Ramp_decrease_signed_conversion_bias | 805F3D20 | 805F3D28 | 8 | 00578EC0 | I f64 4330000080000000; signed counter conversion; H |
| D070 | Parameter_factory_kind8_entry | 8056BE28 | 8056BE2C | 4 | 00568E28 | I code pointer804338D4; outer descriptor type8, not resource kind1; H |
| D071 | Disable_controller_B_dispatch | 80544E74 | 80544E78 | 4 | 00541E74 | I code pointer80205584; state0 disable container B; H |
| D072 | Render_view_selection_state | 8057E7C0 | 8057E7D4 | 14 | — | Z u32[5]; mode/two inputs/selector/view index; H/R |
| D073 | Task_traversal_depth | 805EF270 | 805EF274 | 4 | — | Z u32; recursion depth, not image age; H/R |
| D074 | Task_profile_flag | 805EF274 | 805EF278 | 4 | — | Z u32; profiling route selector; H/R |
| D075 | Task_pause_flag | 805EF27C | 805EF27D | 1 | — | Z u8; pause, render bypass flag100; H/R |
| D076 | Frame_initialized_flag | 805F0A54 | 805F0A55 | 1 | — | Z u8; recurring driver first-use gate; H/R |
| D077 | Frame_delta | 805F0A58 | 805F0A5C | 4 | — | Z f32; supplied task traversal delta, units not measured; H/R |
| D078 | Texture_last_loaded_map0 | 8056FCDC | 8056FCE0 | 4 | 0056CCDC | I mutable pointer32 initially0; last descriptor, not history; H/R |
| D079 | Texture_last_use_token | 805EEFF0 | 805EEFF2 | 2 | 005778F0 | I mutable u16 initially1; GPU use token, not frame ID; H/R |
| D080 | Raster_create_token | 805F2720 | 805F2722 | 2 | — | Z u16; cached creation/completion stamp; H/R |

## 3. Runtime objects and supplementary windows are not extra records

The [globals/layout document](motion_blur_globals.md) describes B0x13C, parameters0x10, three4x0x18 vertex blocks, runtime S/H views/roots and their separate pixel allocations. Their VA/pixel content depends on allocation; no fixed file offset or invented startup image can be supplied. M normally8057798C, task manager80571C6C and render-level manager80574384 are distinct singleton bodies. Necessary live storage such as provider805E25C8, render cache805E42D0, projection backup805E4218, camera-derived matrix805E428C, backend camera805F26F8 and FIFO**CC008000** has scoped instruction provenance but is not an additional exported data record in this80-row inventory.

Engine open80487254..804874FC, system installer case804954F8..804955FC, provider install801E28D0..28F0, stage teardown windows, timer/actor wrapper windows and token predicate8049B7DC are bounded supplemental evidence in [03](evidence/03_closure.md), [04](evidence/04_closure.md), [05](evidence/05_closure.md), [12](evidence/12_closure.md). They are not padded into the121-function count. **80049078 is an application case;801D280C is a call instruction;8043A900 is an interior constructor block;8042F258 begins a separate effect.**

BSS envelope8056FE00..805FC5EC overlaps initialized SDA regions. The actual zero-fill table (D018), not the envelope, distinguishes Z from I. Startup words (D017) establish r2/r13; small-data addresses use signed displacements from those bases. The reference map has region-dependent offsets and is a hypothesis source only; [01 mapper](evidence/01_binary_mapper.md) and [02 PPC](evidence/02_powerpc.md) independently check this namespace distinction.

## 4. Confidence and verification availability

Per-function/per-data SHA-256 values live in the manifests to avoid a second manually copied hash authority. The unchanged whole-DOL hash above binds that evidence snapshot. [README](README.md) indexes all15 roles and their independent/permission-limited methods; not every role independently hashed the DOL or reviewed the final source. Current manifest verification supersedes historical31/42/81-range counts without retroactively certifying every legacy window.

Strict PPC syntax, default/O2 LLVM IR,23 synthetic tests and the constructor's24/24 object/FP event check pass. Source is not installed as a generic backend: domain assertions are non-original; input continuations are annotations, not PC-resume ABI. **Actual PPC object generation is blocked by the installed LLVM backend; SDK/ABI bindings, link, runtime/FIFO/GPU and fault equivalence remain UNVERIFIED.** High confidence in an address/table is not a live camera/pixel/VI-time observation.

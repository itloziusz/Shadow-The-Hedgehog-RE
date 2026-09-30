#ifndef SHADOW_MOTION_BLUR_RECONSTRUCTED_H
#define SHADOW_MOTION_BLUR_RECONSTRUCTED_H

// Static reconstruction of the local main.dol, not a replacement renderer.
// Target SHA-256: fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af
// Authoritative evidence: ../evidence/original/*.asm, address_manifest.json,
// data_manifest.json. No SDK or standard-library headers are required.
// This is a 32-bit, big-endian PowerPC model. It is NOT a host-pointer layout,
// an original compiler ABI reproduction, a linked game build, or runtime proof.

namespace motion_blur {

using u8 = unsigned char;
using s16 = signed short;
using u16 = unsigned short;
using s32 = signed int;
using u32 = unsigned int;
using u64 = unsigned long long;
using f32 = float;
using f64 = double;

static_assert(sizeof(u8) == 1 && sizeof(u16) == 2 && sizeof(s16) == 2);
static_assert(sizeof(u32) == 4 && sizeof(s32) == 4 && sizeof(u64) == 8);
static_assert(sizeof(f32) == 4 && sizeof(f64) == 8);
static_assert(sizeof(void*) == 4, "Use --target=powerpc-unknown-eabi, not host pointers");
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__)
static_assert(__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__, "Original target is big endian");
#endif

// The +8/+C parameters are transferred with lwz/stw, not float arithmetic in
// this core. Other producers interpret them as floats. A byte representation
// allows their genuinely indeterminate constructor/default values to remain
// unresolved without evaluating an uninitialized C++ float or inventing zero.
struct alignas(4) RawWord32 {
    u8 bytes[4];
};
static_assert(sizeof(RawWord32) == 4 && alignof(RawWord32) == 4);

struct BlurParameters {
    u8 enabled;                       // +0x00
    u8 alpha;                         // +0x01
    u8 untouched_02_03[2];             // NOT copied by any parameter helper
    f32 geometry_scale;               // +0x04
    RawWord32 auxiliary_08;            // +0x08: meaning unresolved; unused by draw/save
    RawWord32 auxiliary_0c;            // +0x0C: meaning unresolved; unused by draw/save
};

struct Im2DVertex {
    f32 x, y, z;                      // +0x00,+0x04,+0x08
    u8 red, green, blue, alpha;        // +0x0C..+0x0F: memory/FIFO RGBA order
    f32 u, v;                         // +0x10,+0x14
};

// Only the observed prefix is declared; sizeof(RasterPrefix) is NOT a claim
// about the complete RenderWare raster/plugin allocation. Width/height/depth
// are raw GPR words: the draw uses SIGNED current sizes and UNSIGNED parent sizes.
struct RasterPrefix {
    RasterPrefix* parent;             // +0x00: one dereference, not a root-search loop
    u8 unknown_04_0b[8];
    u32 width_word;                    // +0x0C
    u32 height_word;                   // +0x10
    u32 depth_word;                    // +0x14
    u8 unknown_18_1b[4];
    s16 offset_x;                      // +0x1C
    s16 offset_y;                      // +0x1E
    u8 unknown_20;
    u8 flags_21;                       // used by 0x8048AE14, not interpreted here
    u8 unknown_22_23[2];
};

struct RasterRect {
    s32 x, y, width, height;           // +0,+4,+8,+C; constructor's SP+0x18 record
};

struct CameraPrefix;
using CameraUpdateFn = CameraPrefix* (*)(CameraPrefix*);
struct CameraPrefix {
    u8 unknown_00_17[0x18];
    CameraUpdateFn begin_update;       // +0x18, called by 0x8048652C
    CameraUpdateFn end_update;         // +0x1C, called by 0x80486504
    u8 unknown_20_5f[0x40];
    RasterPrefix* raster;              // +0x60
};                                    // observed prefix only, not full camera size

struct CameraProviderPrefix;
using GetCameraFn = CameraPrefix* (*)(CameraProviderPrefix*);
struct CameraProviderVtablePrefix {
    u8 unknown_00_23[0x24];
    GetCameraFn get_camera;            // +0x24; explicit provider this in r3
};
struct CameraProviderPrefix {
    const CameraProviderVtablePrefix* vtable;
};

// These are plain callback-table functions, NOT virtual methods with a hidden
// device this. The second SetState argument is the raw 32-bit value/pointer word.
using StateSetFn = s32 (*)(u32 state, u32 value_word);
using StateGetFn = s32 (*)(u32 state, u32* output_word);
using PrimitiveFn = s32 (*)(u32 primitive, Im2DVertex* vertices, u32 count);
struct DeviceTable {
    f32 value_00;
    u32 system_callback_04;
    f32 screen_near_z;                 // +0x08 (engine+0x18)
    f32 screen_far_z;                  // +0x0C
    StateSetFn set_state;              // +0x10 = 0x80498954 in 0x8056F50C
    StateGetFn get_state;              // +0x14 = 0x804984C4
    u32 other_callback_18;
    u32 other_callback_1c;
    PrimitiveFn render_primitive;      // +0x20 = 0x80491E80
    u32 other_callbacks_24_34[5];
};
struct EnginePrefix {
    u8 unknown_00_0f[0x10];
    DeviceTable device;                // device descriptor copied to engine+0x10
};                                    // observed prefix, not full engine size

struct EffectManagerPrefix;
struct BlurState {
    BlurParameters parameters;        // +0x00
    EffectManagerPrefix* owner;        // +0x10
    Im2DVertex draw_vertices[4];        // +0x14..+0x73, TL/BL/TR/BR
    RasterPrefix* history_view;        // +0x74, its parent owns backing image
    u8 saved_valid;                    // +0x78: control byte, NOT a GPU success result
    u8 untouched_79_7b[3];
    Im2DVertex half_size_vertices[4];   // +0x7C..+0xDB
    Im2DVertex full_size_vertices[4];   // +0xDC..+0x13B
};
struct EffectManagerPrefix {
    u8 unknown_00_33[0x34];
    RasterPrefix* screen_view;         // +0x34; save uses this view's parent
    u8 unknown_38_3f[8];
    BlurState* blur;                    // +0x40
};                                    // observed prefix, not full manager size

// Plain storage, not C++ virtual classes: the original task vptr is at +0x18.
struct TaskBase {
    const char* name;                  // +0x00
    u16 flags;                        // +0x04: constructors OR 0x100, disposal ORs 1
    u8 unknown_06_17[0x12];
    const void* vtable;                // +0x18
    u8 unknown_1c_27[0x0C];
};
struct SaveScreenTask {
    TaskBase base;                     // original allocation: 0x28 at 0x801D2094
};
struct DrawBlurTask {
    TaskBase base;
    SaveScreenTask* save_task;          // +0x28
    u8 untouched_2c_2f[4];             // constructor/destructor do not initialize it
};
// DrawBlurTask allocation is 0x30 at 0x801D1AE0 (initial cached provenance:
// full_disassembly.txt:470185-470190). INTEGRATION PROVENANCE UPDATE: freshly
// hash-checked D[801D1AD0,801D1AFC) verifies li r3,0x30 at 1AE0, allocation BL
// 1AE4 and constructor BL 1AF0; this caller window is not a new manifest export.
struct RenderLevelsPrefix {
    u8 unknown_00_2b[0x2C];
    TaskBase* draw_parent;              // +0x2C
    u8 unknown_30_57[0x28];
    TaskBase* save_parent;              // +0x58
};

#define MB_FIELD(type, member, offset) \
    static_assert(__builtin_offsetof(type, member) == (offset), #type "." #member)
MB_FIELD(BlurParameters, enabled, 0x00);
MB_FIELD(BlurParameters, alpha, 0x01);
MB_FIELD(BlurParameters, untouched_02_03, 0x02);
MB_FIELD(BlurParameters, geometry_scale, 0x04);
MB_FIELD(BlurParameters, auxiliary_08, 0x08);
MB_FIELD(BlurParameters, auxiliary_0c, 0x0C);
static_assert(sizeof(BlurParameters) == 0x10);
MB_FIELD(Im2DVertex, x, 0x00);
MB_FIELD(Im2DVertex, y, 0x04);
MB_FIELD(Im2DVertex, z, 0x08);
MB_FIELD(Im2DVertex, red, 0x0C);
MB_FIELD(Im2DVertex, green, 0x0D);
MB_FIELD(Im2DVertex, blue, 0x0E);
MB_FIELD(Im2DVertex, alpha, 0x0F);
MB_FIELD(Im2DVertex, u, 0x10);
MB_FIELD(Im2DVertex, v, 0x14);
static_assert(sizeof(Im2DVertex) == 0x18);
MB_FIELD(RasterPrefix, parent, 0x00);
MB_FIELD(RasterPrefix, width_word, 0x0C);
MB_FIELD(RasterPrefix, height_word, 0x10);
MB_FIELD(RasterPrefix, depth_word, 0x14);
MB_FIELD(RasterPrefix, offset_x, 0x1C);
MB_FIELD(RasterPrefix, offset_y, 0x1E);
MB_FIELD(RasterPrefix, flags_21, 0x21);
static_assert(sizeof(RasterPrefix) == 0x24);
MB_FIELD(RasterRect, x, 0x00);
MB_FIELD(RasterRect, y, 0x04);
MB_FIELD(RasterRect, width, 0x08);
MB_FIELD(RasterRect, height, 0x0C);
static_assert(sizeof(RasterRect) == 0x10);
MB_FIELD(CameraPrefix, begin_update, 0x18);
MB_FIELD(CameraPrefix, end_update, 0x1C);
MB_FIELD(CameraPrefix, raster, 0x60);
static_assert(sizeof(CameraPrefix) == 0x64);
MB_FIELD(CameraProviderVtablePrefix, get_camera, 0x24);
static_assert(sizeof(CameraProviderVtablePrefix) == 0x28);
MB_FIELD(CameraProviderPrefix, vtable, 0x00);
static_assert(sizeof(CameraProviderPrefix) == 0x04);
MB_FIELD(DeviceTable, screen_near_z, 0x08);
MB_FIELD(DeviceTable, screen_far_z, 0x0C);
MB_FIELD(DeviceTable, set_state, 0x10);
MB_FIELD(DeviceTable, get_state, 0x14);
MB_FIELD(DeviceTable, render_primitive, 0x20);
static_assert(sizeof(DeviceTable) == 0x38 && sizeof(StateSetFn) == 4);
MB_FIELD(EnginePrefix, device, 0x10);
static_assert(sizeof(EnginePrefix) == 0x48);
MB_FIELD(BlurState, parameters, 0x00);
MB_FIELD(BlurState, owner, 0x10);
MB_FIELD(BlurState, draw_vertices, 0x14);
MB_FIELD(BlurState, history_view, 0x74);
MB_FIELD(BlurState, saved_valid, 0x78);
MB_FIELD(BlurState, untouched_79_7b, 0x79);
MB_FIELD(BlurState, half_size_vertices, 0x7C);
MB_FIELD(BlurState, full_size_vertices, 0xDC);
static_assert(sizeof(BlurState) == 0x13C);
MB_FIELD(EffectManagerPrefix, screen_view, 0x34);
MB_FIELD(EffectManagerPrefix, blur, 0x40);
static_assert(sizeof(EffectManagerPrefix) == 0x44);
MB_FIELD(TaskBase, name, 0x00);
MB_FIELD(TaskBase, flags, 0x04);
MB_FIELD(TaskBase, vtable, 0x18);
static_assert(sizeof(TaskBase) == 0x28 && sizeof(SaveScreenTask) == 0x28);
MB_FIELD(DrawBlurTask, save_task, 0x28);
MB_FIELD(DrawBlurTask, untouched_2c_2f, 0x2C);
static_assert(sizeof(DrawBlurTask) == 0x30);
MB_FIELD(RenderLevelsPrefix, draw_parent, 0x2C);
MB_FIELD(RenderLevelsPrefix, save_parent, 0x58);
static_assert(sizeof(RenderLevelsPrefix) == 0x5C);
#undef MB_FIELD

namespace constants {
// data_manifest.json:63-92: literal bytes, not inferred SDK defaults.
inline constexpr f32 half_8051C388 = 0.5f;          // 0x3F000000
inline constexpr f32 neutral_8051C38C = 1.0f;       // 0x3F800000
inline constexpr f32 zero_8051C3A0 = 0.0f;          // 0x00000000, positive zero
alignas(8) inline constexpr u64 signed_bias_8051C390 = 0x4330000080000000ULL;
alignas(8) inline constexpr u64 unsigned_bias_8051C398 = 0x4330000000000000ULL;
inline constexpr u32 rw_triangle_strip = 4;
inline constexpr u32 gx_triangle_strip = 0x98;     // table word 0x8056F130
inline constexpr u32 device_table_8056F50C = 0x8056F50Cu;
inline constexpr u32 engine_pointer_805F265C = 0x805F265Cu;
inline constexpr u32 copy_clear_word_805F2688 = 0x805F2688u;
} // namespace constants

// Explicit REAL-ADDRESS adapter boundaries. These declarations do not bind a
// linker symbol to an address or implement the SDK/shared functions. Integration
// must supply/verify that binding; calling this source is not a linkability claim.
// Types describe the r3/r4/... call contract used by this slice, not recovered
// SDK headers. Implementations, allocation/plugin behavior and live scheduling
// remain outside this core; there are no successful no-op fallbacks.
namespace original {
extern "C" {
void* Context_8042B620(); // returns pointer at 0x805E25C8
void* RuntimeCast_803A1AFC(void* object, s32 offset, const void* type_r5,
                           const void* type_r6, s32 flags);
CameraPrefix* RwCamera_BeginUpdate_8048652C(CameraPrefix* camera);
CameraPrefix* RwCamera_EndUpdate_80486504(CameraPrefix* camera);
RasterPrefix* RwRaster_GetOffset_8048AB7C(RasterPrefix* raster, s16* x, s16* y);
RasterPrefix* RwRaster_Create_8048AEC4(s32 width, s32 height, s32 depth, u32 flags);
RasterPrefix* RwRaster_SubRaster_8048AE14(RasterPrefix* view, RasterPrefix* parent,
                                       const RasterRect* rectangle);
s32 RwRaster_Destroy_8048AC2C(RasterPrefix* raster);
// 0x80496208..0x80496410: downsample controls doubled source dimensions and
// GXSetTexCopyDst's low-byte downsample argument. It is NOT copy-clear. Actual
// GXCopyTex clear = low8(word[0x805F2688]) at 0x804963AC/0x804963B4.
void RwRaster_CopyEFB_80496208(RasterPrefix* raster, s32 downsample);
void* Allocate_803A1380(u32 bytes);
void Deallocate_803A1334(void* storage);
RenderLevelsPrefix* RenderLevels_800461A0();
TaskBase* Task_Construct_8004F014(TaskBase* task, TaskBase* parent);
TaskBase* Task_Destroy_8004EF18(TaskBase* task, s32 delete_flag);
EffectManagerPrefix* EffectManager_800A35D8(); // lazy cache at 0x805EF2D8
}
} // namespace original

// Original function comments, call sites and block addresses are in the .cpp.
void MotionBlur_Draw_8042E254(BlurState* state);
void MotionBlur_SaveScreen_8042E838(BlurState* state);
BlurParameters* MotionBlur_CopyParameters_8042E9C4(BlurParameters* destination,
                                                  const BlurParameters* source);
BlurParameters* MotionBlur_AssignParameters_8042E9F0(BlurParameters* destination,
                                                    const BlurParameters* source);
BlurParameters* MotionBlur_AssignParametersIfPresent_8042EA1C(
    BlurParameters* destination, const BlurParameters* source);
BlurState* MotionBlur_Construct_8042EA50(BlurState* state, EffectManagerPrefix* owner,
                                       const BlurParameters* parameters);
// Original r3 is output storage and r4 is manager. No nominal r3 return value
// is invented. Absent-state +8/+C are unresolved ORIGINAL STACK RESIDUE, not zero.
void MotionBlur_GetParameters_8042F084(BlurParameters* output,
                                     const EffectManagerPrefix* manager);
void MotionBlur_SetParameters_8042F0E8(EffectManagerPrefix* manager,
                                     const BlurParameters* parameters);
void MotionBlur_SetParametersIfPresent_8042F114(EffectManagerPrefix* manager,
                                              const BlurParameters* parameters);
void MotionBlur_ManagerSaveScreen_8042F140(EffectManagerPrefix* manager);
void MotionBlur_ManagerDraw_8042F16C(EffectManagerPrefix* manager);
void MotionBlur_Destroy_8042F198(EffectManagerPrefix* manager);
void MotionBlur_Create_8042F20C(EffectManagerPrefix* manager,
                              const BlurParameters* parameters);

DrawBlurTask* DrawBlurTask_Construct_801D2058(DrawBlurTask* task);
SaveScreenTask* SaveScreenTask_Construct_801D20D8(SaveScreenTask* task);
SaveScreenTask* SaveScreenTask_Destroy_801D270C(SaveScreenTask* task, s32 delete_flag);
void SaveScreenTask_Exec_801D2764(SaveScreenTask* task);
DrawBlurTask* DrawBlurTask_Destroy_801D2788(DrawBlurTask* task, s32 delete_flag);
void DrawBlurTask_Exec_801D27FC(DrawBlurTask* task);

// Appended backend source is deliberately NOT installed in DeviceTable. The
// ...Path/...Cases names have restricted domains; see their .cpp block traces.
// Only original::RwRaster_CopyEFB_80496208 above gains an integrated definition.
namespace backend {
struct TexturePrefix;
s32 RwTexture_SetRaster_8049819C(TexturePrefix* texture, RasterPrefix* raster);
s32 RwRenderState_Get_BlurCases_804984C4(u32 state, u32* output);
s32 RwRenderState_Set_BlurCases_80498954(u32 state, u32 value_word);
void RwTexture_FlushState_RGBA8_Path_80498854();
void Im2D_Prepare_Textured_RGBA8_Path_8049148C();
void Im2D_RestoreProjectionViewport_80491774();
s32 Im2D_RenderStrip4_RGBA8_Path_80491E80(u32 primitive, Im2DVertex* vertices, u32 count);
void RwTexture_Realize_RGBA8_NoMip_Path_8049AC88(TexturePrefix* texture, u32 map);
s32 RwRaster_ResolveFormat_BlurCreatePaths_80497458(RasterPrefix* raster, u32 flags);
u32 RwRaster_Level0Size_RGBA8_Path_80496478(RasterPrefix* raster);
s32 RwRaster_BackendCreate_BlurPaths_80497CA8(void* unused_r3, RasterPrefix* raster, u32 flags);
s32 RwRaster_BackendSubRaster_804981B8(RasterPrefix* view, RasterPrefix* parent, u32 unused_r5);
} // namespace backend

namespace input {
// INPUT-ONLY observed layouts, not new gameplay classes or whole allocations.
// All offsets refer to the ORIGINAL object base. No constructor/zero fill is
// implied by these declarations. Raw auxiliary words keep the core's contract.
struct RampTaskPrefix {
    TaskBase task;                       // +0x00; task.flags at +4
    void* actor_reference_word;          // +0x28, not an input to blur strength math
    u8 unknown_2c_33[8];
    u8 decreasing;                       // +0x34; 0 strengthens, nonzero weakens
    u8 untouched_35_37[3];
    f32 strength;                       // +0x38; allocation is 0x40, prefix only 0x3C
};
struct BlurResourcePrefix {
    s32 kind;                           // +0x00; exactly kind 1 is the blur arm
    u32 opaque_04;                      // NOT the source of target.enabled
    u8 alpha;                           // +0x08
    u8 opaque_09_0b[3];
    f32 geometry_scale;                 // +0x0C
    RawWord32 auxiliary_10;              // +0x10; lfs/stfs in resource->target transfer
    RawWord32 auxiliary_14;              // +0x14; opaque float upstream, not draw coords
};
struct EffectParameterPrefix {
    const void* original_vtable;         // +0x00; not a C++ vptr for these functions
    u8 unknown_04_0b[8];
    f32 age;                            // +0x0C
    u8 unknown_10_17[8];
    u32 execution_flags;                // +0x18: bit0 active, bit1 advance, bit2 inhibit
    void* manager_base;                  // +0x1C: RTTI cast source, NOT already manager
    u8 unknown_20_fb[0xDC];
    u32 source_flags;                   // +0xFC: separate from execution_flags
    f32 weight;                         // +0x100
    f32 duration;                       // +0x104
    BlurResourcePrefix* resource;       // +0x108; kind discriminator is at resource+0
};
struct GlobalParameterControllerPrefix {
    EffectParameterPrefix common;
    u8 unrelated_kind_fields_10c_14b[0x40];
    BlurParameters saved;               // +0x14C: snapshot, NOT finalizer source
    BlurParameters target;              // +0x15C: desired, including enabled from FC bit16
};                                      // observed extent 0x16C, NOT allocation 0x1F4

#define MB_INPUT_FIELD(type, member, offset) \
    static_assert(__builtin_offsetof(type, member) == (offset), #type "." #member)
MB_INPUT_FIELD(RampTaskPrefix, task, 0x00);
MB_INPUT_FIELD(RampTaskPrefix, actor_reference_word, 0x28);
MB_INPUT_FIELD(RampTaskPrefix, decreasing, 0x34);
MB_INPUT_FIELD(RampTaskPrefix, strength, 0x38);
static_assert(sizeof(RampTaskPrefix) == 0x3C);
MB_INPUT_FIELD(BlurResourcePrefix, kind, 0x00);
MB_INPUT_FIELD(BlurResourcePrefix, opaque_04, 0x04);
MB_INPUT_FIELD(BlurResourcePrefix, alpha, 0x08);
MB_INPUT_FIELD(BlurResourcePrefix, geometry_scale, 0x0C);
MB_INPUT_FIELD(BlurResourcePrefix, auxiliary_10, 0x10);
MB_INPUT_FIELD(BlurResourcePrefix, auxiliary_14, 0x14);
static_assert(sizeof(BlurResourcePrefix) == 0x18);
MB_INPUT_FIELD(EffectParameterPrefix, original_vtable, 0x00);
MB_INPUT_FIELD(EffectParameterPrefix, age, 0x0C);
MB_INPUT_FIELD(EffectParameterPrefix, execution_flags, 0x18);
MB_INPUT_FIELD(EffectParameterPrefix, manager_base, 0x1C);
MB_INPUT_FIELD(EffectParameterPrefix, source_flags, 0xFC);
MB_INPUT_FIELD(EffectParameterPrefix, weight, 0x100);
MB_INPUT_FIELD(EffectParameterPrefix, duration, 0x104);
MB_INPUT_FIELD(EffectParameterPrefix, resource, 0x108);
static_assert(sizeof(EffectParameterPrefix) == 0x10C);
MB_INPUT_FIELD(GlobalParameterControllerPrefix, common, 0x00);
MB_INPUT_FIELD(GlobalParameterControllerPrefix, saved, 0x14C);
MB_INPUT_FIELD(GlobalParameterControllerPrefix, target, 0x15C);
static_assert(sizeof(GlobalParameterControllerPrefix) == 0x16C);
#undef MB_INPUT_FIELD

// A source-level CONTROL-FLOW ANNOTATION, NOT an original return register or
// success status. A Selected_* routine stops at this original continuation;
// it does not execute an omitted arm/tail, claim it is a no-op, or provide the
// live-register/stack machinery needed to resume binary code. Never install
// these extracted blocks in an original vtable or call them as full entries.
struct [[nodiscard]] Continuation { u32 original_pc; };

// The two complete original input entries. f64 carries the incoming f1 FPR;
// using f32 here would add a rounding step absent from the original call ABI.
void MotionBlur_ApplyRamp_800A47B0(RampTaskPrefix* task_r3);
void EffectParameter_UpdateProgress_8042B63C(EffectParameterPrefix* effect_r3,
                                           f64 incoming_f1);

// Fixed-prefix writes run unconditionally on entry; the actual containers
// continue with non-blur work at 3570 / 3760. Original r3 is saved for that tail.
Continuation MotionBlur_DisableFixedPrefix_Selected_800A3528(void* controller_r3);
Continuation MotionBlur_EnableFixedPrefix_Selected_800A3714(void* controller_r3);

// Disjoint pieces of [800A4830,800A492C). Original non-blur work MUST remain
// between these pieces: 48A4..48C4 before submit, 48C8..48F8 before retirement.
// Splitting prevents the last comparison from being hoisted before callbacks.
Continuation MotionBlur_RampAdvance_Selected_800A4830(RampTaskPrefix* task_r3,
                                                    f64 incoming_f1);
Continuation MotionBlur_RampSubmit_Selected_800A48C4(RampTaskPrefix* task_r3);
Continuation MotionBlur_RampRetirement_Selected_800A48F8(RampTaskPrefix* task_r31);

// Constructor suffix starts AFTER the shared/mixed constructor work. Apply
// includes the real common progress call first. Finalize stops BEFORE shared
// base destruction/free. Other resource kinds return their exact omitted-arm
// PC, not fake success. The full containing ranges and predicates are in .cpp.
Continuation EffectGlobalParam_ConstructKind1_Selected_8043A740(
    GlobalParameterControllerPrefix* effect_r30);
Continuation EffectGlobalParam_ApplyKind1_Selected_80439A8C(
    GlobalParameterControllerPrefix* effect_r3, f64 incoming_f1);
Continuation EffectGlobalParam_FinalizeKind1_Selected_8043A4B8(
    GlobalParameterControllerPrefix* effect_r3, s32 delete_flag_r4);

// Interior paths begin just BEFORE 8042659C, not at the enclosing function's
// entry. No self/manager input is invented: these blocks obtain manager three
// separate times. Only execute after each enclosing dispatch/earlier work.
Continuation MotionBlur_DisablePreservingPrefixA_Selected_8020470C();
Continuation MotionBlur_DisablePreservingPrefixB_Selected_802055D4();
Continuation MotionBlur_DisablePreservingPrefixC_Selected_8020630C();
} // namespace input

namespace original {
extern "C" {
// REAL shared effect-side-effect boundary, not a prefix-only callback or no-op.
// 8042659C walks manager entries and calls 80426440 (then indirect effects).
// Only incoming r3 is material here; ignored return is not called success.
void EffectManager_VisitEntries_8042659C(EffectManagerPrefix* manager_r3);
}
} // namespace original

} // namespace motion_blur

#endif // SHADOW_MOTION_BLUR_RECONSTRUCTED_H

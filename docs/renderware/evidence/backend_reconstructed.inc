// APPEND/INCLUDE OUTSIDE namespace motion_blur, AFTER reconstructed/motion_blur.cpp.
// This fragment reopens that namespace and uses the core's defined ppc helpers.
// Example syntax-only TU (stdin):
//   #include "analysis/motion_blur/reconstructed/motion_blur.cpp"
//   #include "analysis/motion_blur/evidence/backend_reconstructed.inc"
// Target: 32-bit big-endian PPC, C++17, freestanding. Not a host implementation.
// main.dol SHA-256 fde4fa6f81a60313b710161c196dc51c2260be62251ee02775d5eee06f9d55af.
// Source factoring is NOT a binary ABI/linker-address binding or runtime proof.
// Only CopyEFB is supplied under an existing original:: declaration. Other
// entries stay in backend; full bodies vs extracted paths are explicitly marked.
// Do NOT install selected paths as a generic RW renderer. SDK adapters MUST bind
// to their stated real addresses, never
// to successful no-op implementations. No task/camera scheduling is changed.

namespace motion_blur {
namespace backend {
using ppc::Fpr;

// Typed prefixes of original persistent storage; NOT newly initialized globals.
struct alignas(4) GXColorBytes { u8 red, green, blue, alpha; };
struct GXTexObjStorage { u32 sdk_words[8]; }; // SDK-owned 0x20-byte object
struct GXTexRegionStorage;                  // SDK-owned; definition unnecessary
struct RasterPluginPrefix {
    u8 sdk_or_unknown_00_0b[0x0C];
    u32 gx_format;                         // +0x0C: 6 = GX_TF_RGBA8
    u32 format_auxiliary;                  // +0x10: untouched 0xFF on RGBA8 create
    u32 alpha_flags;                       // +0x14: low bit controls alpha compare/Z location
    void* allocation;                      // +0x18: unaligned allocator result
    void* image;                           // +0x1C: 32-byte aligned backing image
    void* auxiliary_image;                 // +0x20: initialized zero on selected create path
    u32 unknown_24;
    u32 unknown_28;
    GXTexRegionStorage* texture_region;     // +0x2C: null => global invalidation/load callback
    u16 use_serial;                        // +0x30: create / realize stamp, not a fence wait
    u8 maximum_lod;                        // +0x32: 0 for allocated no-mipmap root
    u8 unknown_33;                         // +0x33: initialized 0xFF
};
struct TexturePrefix {
    RasterPrefix* raster;                  // +0x00: current binding, NOT always root
    u8 unknown_04_4f[0x4C];
    u32 filter_and_addressing;              // +0x50: filter low8; U bits8..11; V bits12..15
};
struct TexturePluginPrefix {
    GXTexObjStorage object;                // +0x00
    u32 cached_sampler_and_flags;          // +0x20: dirty=0x01000000; region=0x02000000
};
struct RenderModePrefix {
    u32 vi_mode;                           // +0x00
    u16 framebuffer_width;                 // +0x04
    u16 efb_height;                        // +0x06 (not the height loaded by Im2D)
    u16 xfb_height;                        // +0x08: Im2D viewport/projection height
    u8 unknown_0a_17[0x0E];
    u8 field_rendering;                    // +0x18: chooses viewport jitter entry
    u8 antialiasing;                       // +0x19: restored copy-filter enable
    u8 sample_pattern[12][2];              // +0x1A..0x31
    u8 vertical_filter[7];                 // +0x32..0x38
};
struct RenderStateCache {
    u32 z_write;                           // 0x805E42D0 +0x00, state 8
    u32 z_test;                            // +0x04, state 6
    u32 gx_z_function;                     // +0x08, 3=LEQUAL or 7=ALWAYS
    u32 untouched_cull_state;              // +0x0C: never reset by this slice
    u32 fog_enabled;                       // +0x10, state 14
    u32 fog_type;                          // +0x14: index into 0x8056FA18
    u32 untouched_fog_color_word;          // +0x18
    GXColorBytes fog_color;                // +0x1C, already RGBA byte order
    u32 fog_end_override;                  // +0x20
    f32 fog_start;                         // +0x24
    f32 fog_end;                           // +0x28
    f32 fog_near;                          // +0x2C
    f32 fog_far;                           // +0x30
    u32 source_blend;                      // +0x34, RW state 10
    u32 destination_blend;                 // +0x38, RW state 11
    u32 z_compare_before_texture;          // +0x3C: cached GXSetZCompLoc argument
    u32 alpha_compare0;                    // +0x40: inherited alpha-test state
    u32 alpha_compare1;                    // +0x44
    u32 alpha_operation;                   // +0x48
    u8 alpha_reference0;                   // +0x4C
    u8 alpha_reference1;                   // +0x4D
    u8 untouched_4e_4f[2];
};
struct CameraFogPrefix {
    CameraPrefix core;
    u8 unknown_64_7f[0x1C];
    f32 near_plane;                        // +0x80
    f32 far_plane;                         // +0x84
    f32 fog_plane;                         // +0x88
};
using RasterAllocateFn = void* (*)(u32 bytes, u32 hint);
struct RuntimeGlobalsPrefix {
    CameraFogPrefix* current_camera;       // word[word[0x805F265C]+0]
    u8 unknown_04_107[0x104];
    RasterAllocateFn allocate;             // +0x108; indirect call, NO hidden this
};
struct GXProjectionVector { f32 type, xx, xw, yy, yw, zz, zw; };
struct GXPositionMatrix { f32 row[3][4]; };
struct RwErrorRecord { u32 plugin; u32 code; };

#define MB_BACKEND_FIELD(type, member, offset) \
    static_assert(__builtin_offsetof(type, member) == (offset), #type "." #member)
MB_BACKEND_FIELD(RasterPluginPrefix, gx_format, 0x0C);
MB_BACKEND_FIELD(RasterPluginPrefix, alpha_flags, 0x14);
MB_BACKEND_FIELD(RasterPluginPrefix, allocation, 0x18);
MB_BACKEND_FIELD(RasterPluginPrefix, image, 0x1C);
MB_BACKEND_FIELD(RasterPluginPrefix, texture_region, 0x2C);
MB_BACKEND_FIELD(RasterPluginPrefix, use_serial, 0x30);
MB_BACKEND_FIELD(RasterPluginPrefix, maximum_lod, 0x32);
MB_BACKEND_FIELD(TexturePrefix, filter_and_addressing, 0x50);
MB_BACKEND_FIELD(TexturePluginPrefix, cached_sampler_and_flags, 0x20);
MB_BACKEND_FIELD(RenderModePrefix, framebuffer_width, 4);
MB_BACKEND_FIELD(RenderModePrefix, xfb_height, 8);
MB_BACKEND_FIELD(RenderModePrefix, field_rendering, 0x18);
MB_BACKEND_FIELD(RenderModePrefix, sample_pattern, 0x1A);
MB_BACKEND_FIELD(RenderModePrefix, vertical_filter, 0x32);
MB_BACKEND_FIELD(RenderStateCache, z_test, 4);
MB_BACKEND_FIELD(RenderStateCache, gx_z_function, 8);
MB_BACKEND_FIELD(RenderStateCache, fog_enabled, 0x10);
MB_BACKEND_FIELD(RenderStateCache, fog_color, 0x1C);
MB_BACKEND_FIELD(RenderStateCache, fog_start, 0x24);
MB_BACKEND_FIELD(RenderStateCache, source_blend, 0x34);
MB_BACKEND_FIELD(RenderStateCache, z_compare_before_texture, 0x3C);
MB_BACKEND_FIELD(RenderStateCache, alpha_reference0, 0x4C);
MB_BACKEND_FIELD(CameraFogPrefix, near_plane, 0x80);
MB_BACKEND_FIELD(CameraFogPrefix, far_plane, 0x84);
MB_BACKEND_FIELD(CameraFogPrefix, fog_plane, 0x88);
MB_BACKEND_FIELD(RuntimeGlobalsPrefix, allocate, 0x108);
static_assert(sizeof(RasterPluginPrefix) == 0x34);
static_assert(sizeof(TexturePluginPrefix) == 0x24);
static_assert(sizeof(GXProjectionVector) == 0x1C && sizeof(GXPositionMatrix) == 0x30);
static_assert(sizeof(RenderStateCache) == 0x50 && sizeof(RwErrorRecord) == 8);
#undef MB_BACKEND_FIELD

// Recovered absolute address accesses. Pointer/offset arithmetic is 32-bit GPR
// modular arithmetic, not a host array-bounds claim. No live value is invented
// for BSS pointer slots or plugin offsets. Reload helpers at each observed use.
template<class T> inline T* at(u32 address) { return reinterpret_cast<T*>(address); }
inline u32 word_at(u32 address) { return ppc::load_word(at<void>(address)); }
template<class T> inline T* pointer_at(u32 address) { return at<T>(word_at(address)); }
template<class T> inline T* extension(void* object, u32 offset_slot) {
    return at<T>(reinterpret_cast<u32>(object) + word_at(offset_slot));
}
inline RasterPluginPrefix* raster_plugin(RasterPrefix* raster) {
    return extension<RasterPluginPrefix>(raster, 0x805F2700u); // r13+0x6200
}
inline TexturePluginPrefix* texture_plugin(TexturePrefix* texture) {
    return extension<TexturePluginPrefix>(texture, 0x805F2718u); // r13+0x6218
}
inline TexturePrefix* current_texture() { return pointer_at<TexturePrefix>(0x805F270Cu); }
inline RenderModePrefix* render_mode() { return pointer_at<RenderModePrefix>(0x805F26F4u); }
inline RuntimeGlobalsPrefix* runtime_globals() {
    return pointer_at<RuntimeGlobalsPrefix>(0x805F265Cu);
}
inline RenderStateCache& states() { return *at<RenderStateCache>(0x805E42D0u); }
inline u32 signed_halfword_bits(s16 value) { return static_cast<u32>(static_cast<s32>(value)); }
inline u32 arithmetic_shift_right_one(u32 word) {
    return (word >> 1) | (word & 0x80000000u); // PPC srawi, no host signed-shift assumption
}
inline Fpr single_at(u32 address) { return ppc::load_single(at<void>(address)); }
inline Fpr double_at(u32 address) { return ppc::load_double(at<void>(address)); }
inline u8& raster_type(RasterPrefix* raster) { return raster->unknown_20; }
inline u8& raster_format_byte(RasterPrefix* raster) { return raster->unknown_22_23[1]; }

namespace raw {
// Evidence snapshots, NOT initializers for original mutable storage.
inline constexpr u32 rw_blend_8056FA28[11] = {0,0,1,2,3,4,5,6,7,2,3};
inline constexpr u32 strip4_word_8056F130 = 0x00000098u;
inline constexpr u32 copy_depth4_8056F544 = 0x804962E4u;
inline constexpr u32 copy_depth8_8056F554 = 0x8049630Cu;
inline constexpr u32 copy_depth16_8056F574 = 0x80496330u;
inline constexpr u32 copy_depth32_8056F5B4 = 0x80496358u;
inline constexpr u32 copy_other_depth = 0x80496380u;
inline constexpr u32 projection_template_8056F13C[7] = {
    0x3F800000,0x3F800000,0xBF800000,0x3F800000,0x3F800000,0xBF800000,0xBF800000
};
inline constexpr u32 matrix_template_8056F158[12] = {
    0x3F800000,0,0,0, 0,0x3F800000,0,0, 0,0,0xBF800000,0
};
inline constexpr u32 im2d_constants_805FC3F8[6] = {
    0,0x3F800000,0x40000000,0xC0000000,0x3F000000,0
};
inline constexpr u64 im2d_unsigned_bias_805FC410 = 0x4330000000000000ULL;
inline constexpr u64 im2d_signed_bias_805FC418 = 0x4330000080000000ULL;
} // namespace raw

// REAL-ADDRESS SDK/shared adapters. Names state the required original entry.
// In mixed-argument adapters Fpr models FPR contents, NOT literal SDK headers;
// integration must preserve the documented rN/fN contract. Nothing here binds a
// linker symbol to a number, allocates GPU resources, or supplies fake success.
namespace sdk {
extern "C" {
void GXSetCopyFilter_80397228(u8 aa_r3, const u8 (*samples_r4)[2],
                            u8 vertical_r5, const u8* filter_r6);
void GXSetTexCopySrc_80396C50(u16 x_r3, u16 y_r4, u16 width_r5, u16 height_r6);
void GXSetTexCopyDst_80396D00(u16 width_r3, u16 height_r4, u32 format_r5, u8 downsample_r6);
void GXCopyTex_803975AC(void* destination_r3, u8 clear_r4);
void GXPixModeSync_80396568();
void GXInvalidateTexRegion_8039868C(GXTexRegionStorage* region_r3);
void GXInvalidateTexAll_803987B8();
u32 RwErrorCode_8047EC78(u32 code_r3, ...);
void RwErrorSet_8047EBD4(const RwErrorRecord* error_r3);
}
} // namespace sdk

} // namespace backend

// COMPLETE wrapper, original [0x80496208,0x80496410), 0x208 bytes.
// Evidence: original/80496208_RwRaster_CopyEFB.asm:5-134; depth jump table
// data_manifest.json:179-218. Blur direct callers: 0x8042E8C0 and 0x8042E968.
// All arithmetic below preserves low-word mul/add/shift behavior, including
// negative signed offsets. Argument 2 is downsample, NEVER the clear flag.
void original::RwRaster_CopyEFB_80496208(RasterPrefix* raster, s32 downsample) {
    using namespace backend;
    RasterPrefix* parent = raster->parent;             // 0x80496238, ONE dereference
    RasterPluginPrefix* plugin = raster_plugin(parent); // 0x80496240..244
    sdk::GXSetCopyFilter_80397228(0, nullptr, 0, nullptr); // BL 0x80496248

    // 0x8049624C..29C: doubling applies to source rectangle only. rlwinm masks
    // each shifted argument to bits 1..15; destination uses parent dimensions.
    const u32 x = signed_halfword_bits(raster->offset_x);
    const u32 y = signed_halfword_bits(raster->offset_y);
    if (downsample != 0) { // 0x80496254..278
        sdk::GXSetTexCopySrc_80396C50(
            static_cast<u16>(x << 1), static_cast<u16>(y << 1),
            static_cast<u16>(raster->width_word << 1),
            static_cast<u16>(raster->height_word << 1)); // BL 0x80496274
    } else { // 0x8049627C..29C
        sdk::GXSetTexCopySrc_80396C50(
            static_cast<u16>(x), static_cast<u16>(y),
            static_cast<u16>(raster->width_word),
            static_cast<u16>(raster->height_word)); // BL 0x8049629C
    }
    sdk::GXSetTexCopyDst_80396D00(
        static_cast<u16>(parent->width_word), static_cast<u16>(parent->height_word),
        plugin->gx_format, static_cast<u8>(downsample)); // BL 0x804962B8

    // 0x804962BC..3A8: address offset in tiled backing storage. This is NOT
    // linear pitch*bytes-per-pixel. Source dimensions can differ from parent.
    const u32 ox = signed_halfword_bits(raster->offset_x);
    const u32 oy = signed_halfword_bits(raster->offset_y);
    u32 image_offset;
    switch (parent->depth_word) {
    case 4: { // table -> 0x804962E4..308; signed srawi only in this arm
        const u32 aligned_width = (parent->width_word + 7u) & ~7u;
        image_offset = arithmetic_shift_right_one((ox << 3) + aligned_width * oy);
        break;
    }
    case 8: { // -> 0x8049630C..32C
        const u32 aligned_width = (parent->width_word + 7u) & ~7u;
        image_offset = (ox << 2) + aligned_width * oy;
        break;
    }
    case 16: { // -> 0x80496330..354
        const u32 aligned_width = (parent->width_word + 3u) & ~3u;
        image_offset = ((ox << 2) + aligned_width * oy) << 1;
        break;
    }
    case 32: { // -> 0x80496358..37C; actual RGBA8 blur/scratch case
        const u32 aligned_width = (parent->width_word + 3u) & ~3u;
        image_offset = ((ox << 2) + aligned_width * oy) << 2;
        break;
    }
    default: { // 0x80496380..3A4: deliberately NO copy/filter restoration/invalidation
        RwErrorRecord error; // actual stack pair: plugin=1, error code from callee
        error.plugin = 1;
        error.code = sdk::RwErrorCode_8047EC78(0x8000000Cu); // BL 0x80496394
        sdk::RwErrorSet_8047EBD4(&error);                    // BL 0x804963A0
        return;
    }
    }
    // 0x804963A8..3BC: LOW BYTE of independently owned global word, unnormalized.
    void* destination = at<void>(reinterpret_cast<u32>(plugin->image) + image_offset);
    const u8 clear = static_cast<u8>(word_at(0x805F2688u));
    sdk::GXCopyTex_803975AC(destination, clear); // BL 0x804963B8
    sdk::GXPixModeSync_80396568();             // BL 0x804963BC; NOT GXDrawDone
    RenderModePrefix* mode = render_mode();
    sdk::GXSetCopyFilter_80397228(mode->antialiasing, mode->sample_pattern,
                                1, mode->vertical_filter); // BL 0x804963D4
    if (plugin->texture_region != nullptr) // 0x804963D8..3E0
        sdk::GXInvalidateTexRegion_8039868C(plugin->texture_region); // BL 0x804963E4
    else
        sdk::GXInvalidateTexAll_803987B8(); // BL 0x804963EC
    // No sampler, pixel format, write-mask, culling or copy-clear value changes.
}

namespace backend {

// NON-ORIGINAL scope assertion. These names intentionally expose extracted
// paths rather than claim generic renderer coverage. An out-of-scope call must
// not silently succeed, return an invented RW failure, or call a fake SDK stub.
// This trap is not attributed to any address in the original DOL.
inline void require_extracted_path(bool in_scope) {
    if (!in_scope)
        __builtin_trap();
}

namespace raw {
// State-case address mapping: direct table indices, NOT SDK enum assumptions.
struct StateCaseMapping { u32 id, getter_block, setter_block; };
inline constexpr StateCaseMapping blur_state_cases[] = {
    {1,  0x804985A8, 0x80498C18},
    {6,  0x804985D0, 0x80498CA4},
    {8,  0x804985BC, 0x80498C38},
    {9,  0x80498590, 0x80498BFC},
    {10, 0x804985E8, 0x80498D38},
    {11, 0x80498600, 0x80498DC0},
    {12, 0x804986B0, 0x80498F58}, // BOTH unsupported; setter initial r6=0
    {14, 0x804984E4, 0x80498990}
};
inline constexpr u32 fog_type_8056FA18[4] = {0,2,4,5};
inline constexpr u32 fog_disable_constants_805FC44C[3] = {
    0x40A00000, 0x41200000, 0x3D4CCCCD // 5.0,10.0,0.05 (exact f32 bits)
};
} // namespace raw

namespace sdk {
extern "C" {
void GXSetZMode_80399DF0(u8 enable_r3, u32 function_r4, u8 update_r5);
void GXSetBlendMode_80399D44(u32 type_r3, u32 source_r4, u32 destination_r5, u32 logic_r6);
void GXSetFog_80399A0C(u32 type_r3, Fpr start_f1, Fpr end_f2,
                     Fpr near_f3, Fpr far_f4, const GXColorBytes* color_r4);
void GXSetAlphaCompare_80399778(u32 compare0_r3, u8 reference0_r4,
                              u32 operation_r5, u32 compare1_r6, u8 reference1_r7);
void GXSetZCompLoc_80399E24(u8 before_texture_r3);
}
} // namespace sdk

// COMPLETE original [0x8049819C,0x804981B8), 0x1C bytes, asm:5-11.
// r3=texture descriptor, r4=raw raster pointer; callers place zero in r5 but the
// original overwrites r5 immediately and ignores that input. This does not bind
// a GX texture yet and does NOT change texture+0x50's inherited sampler word.
s32 RwTexture_SetRaster_8049819C(TexturePrefix* texture, RasterPrefix* raster) {
    texture->raster = raster; // 0x8049819C
    texture_plugin(texture)->cached_sampler_and_flags = 0x01000000u; // 0x804981AC
    return 1; // 0x804981B0
}

// Extracted cases of original [0x804984C4,0x804986B8), 0x1F4 bytes.
// Evidence asm:5-129; getter table 0x8056FA58 (data_manifest.json:709-750).
// Scope is IDs 1,6,8,9,10,11,12,14; ALL input words for these IDs are covered.
// The generic original's other successful/failed state cases are not supplied.
s32 RwRenderState_Get_BlurCases_804984C4(u32 state, u32* output) {
    u32 value;
    switch (state) {
    case 1:  value = reinterpret_cast<u32>(current_texture()->raster); break; // 804985A8..5B8
    case 6:  value = states().z_test; break;                                // 804985D0..5E4
    case 8:  value = states().z_write; break;                               // 804985BC..5CC
    case 9:  value = current_texture()->filter_and_addressing & 0xFFu; break; // 80498590..5A4
    case 10: value = states().source_blend; break;                          // 804985E8..5FC
    case 11: value = states().destination_blend; break;                     // 80498600..614
    case 14: value = states().fog_enabled; break;                           // 804984E4..4F8
    case 12:
        // 0x8056FA88 -> 0x804986B0: NO write, NO output-pointer dereference.
        // In the draw, old_vertex_alpha stays raw stack residue; do not repair it.
        return 0;
    default:
        require_extracted_path(false);
        __builtin_unreachable();
    }
    ppc::store_word(output, value); // each supported arm's stw r0,0(r4)
    return 1;
}

// Extracted cases of original [0x80498954,0x80498F74), 0x620 bytes.
// Evidence asm:5-396; setter table 0x8056FAD4. The addresses for each case are
// recorded above. Returns include exact equality-before-validation behavior;
// bool-valued states normalize only when the original actually changes them.
s32 RwRenderState_Set_BlurCases_80498954(u32 state, u32 value_word) {
    switch (state) {
    case 1: { // 0x80498C18..C34, BL 0x80498C2C -> 0x8049819C
        TexturePrefix* texture = current_texture();
        if (reinterpret_cast<u32>(texture->raster) != value_word)
            (void)RwTexture_SetRaster_8049819C(texture, at<RasterPrefix>(value_word));
        return 1;
    }
    case 6: { // 0x80498CA4..D34: actual GX enable stays 1, compare changes
        RenderStateCache& cache = states();
        if (value_word != 0) {
            if (cache.z_test == 0) {
                sdk::GXSetZMode_80399DF0(1, 3, static_cast<u8>(cache.z_write)); // 80498CD0
                cache.gx_z_function = 3; // 80498CE4, LEQUAL
                cache.z_test = 1;        // 80498CE8
            }
        } else if (cache.z_test != 0) {
            sdk::GXSetZMode_80399DF0(1, 7, static_cast<u8>(cache.z_write)); // 80498D14
            cache.gx_z_function = 7; // 80498D28, ALWAYS (not hardware test disable)
            cache.z_test = 0;        // 80498D2C
        }
        return 1;
    }
    case 8: { // 0x80498C38..CA0
        RenderStateCache& cache = states();
        if (value_word != 0) {
            if (cache.z_write == 0) {
                sdk::GXSetZMode_80399DF0(1, cache.gx_z_function, 1); // 80498C5C
                cache.z_write = 1; // 80498C68
            }
        } else if (cache.z_write != 0) {
            sdk::GXSetZMode_80399DF0(1, cache.gx_z_function, 0); // 80498C8C
            cache.z_write = 0; // 80498C98
        }
        return 1;
    }
    case 9: { // 0x80498BFC..C14: low-byte substitution, not immediate GX emission
        TexturePrefix* texture = current_texture();
        texture->filter_and_addressing =
            (texture->filter_and_addressing & 0xFFFFFF00u) | (value_word & 0xFFu);
        return 1;
    }
    case 10: { // 0x80498D38..DBC; valid unequal values: 1,2,5,6,7,8,9,10
        RenderStateCache& cache = states();
        if (value_word == cache.source_blend) // equality BEFORE validation
            return 1;
        const s32 value = ppc::signed_word(value_word);
        if (!(value == 1 || value == 2 || (value >= 5 && value <= 10)))
            return 0; // 0x80498DAC
        const u32 source = word_at(0x8056FA28u + (value_word << 2));
        const u32 destination = word_at(0x8056FA28u + (cache.destination_blend << 2));
        sdk::GXSetBlendMode_80399D44(1, source, destination, 0); // BL 0x80498DA0
        cache.source_blend = value_word; // 0x80498DA4
        return 1;
    }
    case 11: { // 0x80498DC0..E2C; valid unequal values: 1..8
        RenderStateCache& cache = states();
        if (value_word == cache.destination_blend)
            return 1;
        const s32 value = ppc::signed_word(value_word);
        if (value < 1 || value >= 9)
            return 0; // 0x80498E1C
        const u32 destination = word_at(0x8056FA28u + (value_word << 2));
        const u32 source = word_at(0x8056FA28u + (cache.source_blend << 2));
        sdk::GXSetBlendMode_80399D44(1, source, destination, 0); // BL 0x80498E10
        cache.destination_blend = value_word; // 0x80498E14
        return 1;
    }
    case 12:
        // 0x80498960 r6=0; 0x8056FB04 -> 0x80498F58. No setter body,
        // no evaluation/dereference of value_word, no vertex-alpha cache or GX call.
        return 0;
    case 14: { // 0x80498990..A7C: fog enable/disable AND conditional plane cache
        RenderStateCache& cache = states();
        if (value_word != 0) {
            if (cache.fog_enabled == 0) {
                CameraFogPrefix* camera = runtime_globals()->current_camera; // 804989AC..9B4
                if (camera != nullptr) { // unlike several other backend calls, guarded here
                    if (cache.fog_end_override == 0)
                        ppc::store_single(&cache.fog_end, ppc::load_single(&camera->far_plane)); // 9C8..9CC
                    ppc::store_single(&cache.fog_start, ppc::load_single(&camera->fog_plane)); // 9D4..9DC
                    ppc::store_single(&cache.fog_near, ppc::load_single(&camera->near_plane)); // 9E0..9E4
                    ppc::store_single(&cache.fog_far, ppc::load_single(&camera->far_plane)); // 9E8..9EC
                }
                GXColorBytes color;
                ppc::store_word(&color, ppc::load_word(&cache.fog_color)); // stack+10 at 80498A14
                const u32 type = word_at(0x8056FA18u + (cache.fog_type << 2));
                const Fpr start = ppc::load_single(&cache.fog_start);
                const Fpr end = ppc::load_single(&cache.fog_end);
                const Fpr near_plane = ppc::load_single(&cache.fog_near);
                const Fpr far_plane = ppc::load_single(&cache.fog_far);
                sdk::GXSetFog_80399A0C(type, start, end, near_plane, far_plane, &color); // 80498A28
                cache.fog_enabled = 1; // 80498A30
            }
        } else if (cache.fog_enabled != 0) {
            GXColorBytes color;
            ppc::store_word(&color, ppc::load_word(&cache.fog_color)); // stack+14, 80498A64
            const Fpr ten = single_at(0x805FC450u); // f2/f4, 0x41200000
            sdk::GXSetFog_80399A0C(0, single_at(0x805FC44Cu), ten,
                                 single_at(0x805FC454u), ten, &color); // 80498A6C
            cache.fog_enabled = 0; // 80498A74
        }
        return 1;
    }
    default:
        require_extracted_path(false);
        __builtin_unreachable();
    }
}

// Forward-declared reconstructed path, NOT an external successful stub.
void RwTexture_Realize_RGBA8_NoMip_Path_8049AC88(TexturePrefix* texture, u32 map);

// Original [0x80498854,0x80498910), 0xBC bytes, asm:5-51.
// Complete Flush control logic; its realization callee below is deliberately
// scoped to blur/scratch nonpaletted RGBA8/no-mipmap textures.
void RwTexture_FlushState_RGBA8_Path_80498854() {
    TexturePrefix* texture = current_texture();
    if (texture->raster == nullptr) // 8049886C..874
        return;
    RwTexture_Realize_RGBA8_NoMip_Path_8049AC88(texture, 0); // BL 8049887C
    // Original reloads current texture after realization; it reads THAT raster's
    // plugin directly here, whereas realization addresses raster->parent's plugin.
    RasterPrefix* raster = current_texture()->raster;
    RenderStateCache& cache = states();
    const u32 before_texture = (raster_plugin(raster)->alpha_flags & 1u) ^ 1u; // 8049889C..8A4
    if (cache.z_compare_before_texture == before_texture) // 804988A8..8AC
        return;
    if (before_texture == 1) {
        sdk::GXSetAlphaCompare_80399778(7, 0, 0, 7, 0); // 804988CC, ALWAYS/AND/ALWAYS
    } else {
        sdk::GXSetAlphaCompare_80399778(
            cache.alpha_compare0, cache.alpha_reference0, cache.alpha_operation,
            cache.alpha_compare1, cache.alpha_reference1); // 804988E8: inherited tests
    }
    sdk::GXSetZCompLoc_80399E24(static_cast<u8>(before_texture)); // 804988F0
    cache.z_compare_before_texture = before_texture; // 804988F4
}

namespace raw {
// GXSetTevOp stage0/op0 loads these table words. SDK's alpha insertion mask
// is 0x00FFFFF0: the low four existing swap-selector bits are NOT reset.
inline constexpr u32 tev_modulate_color_80568B00 = 0xC008F8AFu;
inline constexpr u32 tev_modulate_alpha_80568B28 = 0xC108F2F0u;
inline constexpr u32 tev_channel4_80568B60 = 0; // COLOR0A0 -> raster channel0
} // namespace raw

namespace sdk {
extern "C" {
void GXClearVtxDesc_80395758();
void GXSetVtxDesc_8039530C(u32 attribute_r3, u32 type_r4);
void GXSetVtxAttrFmt_80395790(u32 format_r3, u32 attribute_r4, u32 count_r5,
                           u32 type_r6, u8 fractional_bits_r7);
void GXSetNumTevStages_803999E4(u8 count_r3);
void GXSetNumChans_80397C80(u8 count_r3);
void GXSetChanCtrl_80397CBC(u32 channel_r3, u8 lighting_r4, u32 ambient_source_r5,
                         u32 material_source_r6, u32 lights_r7,
                         u32 diffuse_function_r8, u32 attenuation_function_r9);
void GXSetTevOp_803992EC(u32 stage_r3, u32 operation_r4);
void GXSetNumTexGens_80395FC8(u8 count_r3);
void GXSetTexCoordGen2_80395D48(u32 coord_r3, u32 function_r4, u32 source_r5,
                             u32 matrix_r6, u8 normalize_r7, u32 postmatrix_r8);
void GXSetTevOrder_80399848(u32 stage_r3, u32 coord_r4, u32 map_r5, u32 color_r6);
u32 VIGetNextField_80380004();
void GXSetViewportJitter_8039A444(Fpr x_f1, Fpr y_f2, Fpr width_f3, Fpr height_f4,
                               Fpr near_f5, Fpr far_f6, u32 field_r3);
void GXSetViewport_8039A49C(Fpr x_f1, Fpr y_f2, Fpr width_f3, Fpr height_f4,
                         Fpr near_f5, Fpr far_f6);
void GXGetProjectionv_8039A1E4(GXProjectionVector* output_r3);
void GXSetProjectionv_8039A158(const GXProjectionVector* input_r3);
void GXLoadPosMtxImm_8039A22C(const GXPositionMatrix* matrix_r3, u32 index_r4);
void GXSetCurrentMtx_8039A2CC(u32 index_r3);
void GXSetScissor_8039A4E4(u32 left_r3, u32 top_r4, u32 width_r5, u32 height_r6);
void GXSetScissorBoxOffset_8039A55C(u32 x_word_r3, u32 y_word_r4);
void GXBegin_80396934(u32 primitive_r3, u32 format_r4, u16 vertices_r5);
}
} // namespace sdk

// TEXTURED path of original [0x8049148C,0x80491774), 0x2E8 bytes.
// Common setup, textured arm 0x80491548..5B4, both viewport arms, and all matrix
// setup are supplied. Untextured 0x804915B8..5E0 is intentionally NOT supplied.
// Texture realization has the separately stated nonpaletted RGBA8/no-mip scope.
void Im2D_Prepare_Textured_RGBA8_Path_8049148C() {
    // 0x8049149C..534: no guessed fullscreen-state resets added.
    sdk::GXClearVtxDesc_80395758();                        // 8049149C
    sdk::GXSetVtxDesc_8039530C(9, 1);                      // 804914A8, POS direct
    sdk::GXSetVtxAttrFmt_80395790(0, 9, 1, 4, 0);         // 804914C0, XYZ f32
    sdk::GXSetVtxDesc_8039530C(11, 1);                     // 804914CC, CLR0 direct
    sdk::GXSetVtxAttrFmt_80395790(0, 11, 1, 5, 0);        // 804914E4, RGBA u8
    sdk::GXSetNumTevStages_803999E4(1);                    // 804914EC
    sdk::GXSetNumChans_80397C80(1);                       // 804914F4
    sdk::GXSetChanCtrl_80397CBC(4, 0, 0, 1, 0, 0, 2);   // 80491514, vertex material
    sdk::GXSetChanCtrl_80397CBC(5, 0, 0, 0, 0, 0, 2);   // 80491534, register material

    require_extracted_path(current_texture()->raster != nullptr); // scope of 80491544 not-taken
    sdk::GXSetVtxDesc_8039530C(13, 1);                    // 80491550, TEX0 direct
    sdk::GXSetVtxAttrFmt_80395790(0, 13, 1, 4, 0);       // 80491568, ST f32
    sdk::GXSetTevOp_803992EC(0, 0);                      // 80491574, GX_MODULATE
    // Default RGB = textureRGB * vertexRGB; default alpha = textureA * vertexA.
    // The latter is NOT unconditional blurAlpha/255. SDK table insertion keeps
    // inherited TEV swap selectors; this code never forces identity swaps.
    sdk::GXSetNumTexGens_80395FC8(1);                     // 8049157C
    sdk::GXSetTexCoordGen2_80395D48(0, 1, 4, 0x3C, 0, 0x7D); // 80491598
    // Texgen0: 2x4, TEX0 source, identity 0x3C, no normalization, identity postmatrix.
    sdk::GXSetTevOrder_80399848(0, 0, 0, 4);             // 804915AC, coord0/map0/COLOR0A0
    RwTexture_FlushState_RGBA8_Path_80498854();            // 804915B0

    // 0x804915E0..684: whole-frame viewport, not camera-raster dimensions.
    RenderModePrefix* mode = render_mode();
    if (mode->field_rendering != 0) { // 804915F0..63C
        const u32 field = sdk::VIGetNextField_80380004() ^ 1u; // 804915F0/600
        const Fpr zero = single_at(0x805FC3F8u);
        const Fpr bias = double_at(0x805FC410u);
        const Fpr width = ppc::unsigned_word_to_single(mode->framebuffer_width, bias);
        const Fpr height = ppc::unsigned_word_to_single(mode->xfb_height, bias);
        sdk::GXSetViewportJitter_8039A444(zero, zero, width, height,
                                        zero, single_at(0x805FC3FCu), field); // 80491638
    } else { // 80491640..680
        const Fpr zero = single_at(0x805FC3F8u);
        const Fpr bias = double_at(0x805FC410u);
        const Fpr width = ppc::unsigned_word_to_single(mode->framebuffer_width, bias);
        const Fpr height = ppc::unsigned_word_to_single(mode->xfb_height, bias);
        sdk::GXSetViewport_8039A49C(zero, zero, width, height,
                                  zero, single_at(0x805FC3FCu)); // 80491680
    }

    // 0x80491684..6F8: only xx and yy of the persistent template are replaced.
    // Remaining words stay [type=1,xw=-1,yw=1,zz=-1,zw=-1]. They are not fresh
    // synthesized defaults each call. The old GX projection goes to global BSS.
    mode = render_mode();
    CameraFogPrefix* camera = runtime_globals()->current_camera;
    RasterPrefix* raster = camera->core.raster; // 804916BC, no camera/raster null guards
    GXProjectionVector* projection = at<GXProjectionVector>(0x8056F13Cu);
    const Fpr unsigned_bias = double_at(0x805FC410u);
    const Fpr width = ppc::unsigned_word_to_single(mode->framebuffer_width, unsigned_bias);
    ppc::store_single(&projection->xx, ppc::div_single(single_at(0x805FC400u), width)); // 804916CC..6D0, +2/W
    const Fpr height = ppc::unsigned_word_to_single(mode->xfb_height, unsigned_bias);
    ppc::store_single(&projection->yy, ppc::div_single(single_at(0x805FC404u), height)); // 804916E4..6E8, -2/H
    sdk::GXGetProjectionv_8039A1E4(at<GXProjectionVector>(0x805E4218u)); // 804916EC
    sdk::GXSetProjectionv_8039A158(projection);                       // 804916F8

    // 0x804916FC..75C: only matrix translations x/y change. z scale remains -1.
    GXPositionMatrix* matrix = at<GXPositionMatrix>(0x8056F158u);
    const Fpr signed_bias = double_at(0x805FC418u);
    const Fpr half = single_at(0x805FC408u); // raw 0x3F000000, not zero
    const Fpr x = ppc::signed_word_to_single(signed_halfword_bits(raster->offset_x), signed_bias);
    ppc::store_single(&matrix->row[0][3], ppc::add_single(half, x)); // 8049172C..734
    const Fpr y = ppc::signed_word_to_single(signed_halfword_bits(raster->offset_y), signed_bias);
    ppc::store_single(&matrix->row[1][3], ppc::add_single(half, y)); // 80491748..750
    sdk::GXLoadPosMtxImm_8039A22C(matrix, 0); // 80491754
    sdk::GXSetCurrentMtx_8039A2CC(0);         // 8049175C
}

// Non-original factoring of repeated signed conversion/call blocks in Restore.
// No arithmetic is hoisted across the original VIGetNextField call. Doubling is
// INTEGER modulo-2^32 before signed conversion, not floating-point *2 afterward.
inline void restore_camera_viewport(RasterPrefix* raster, bool double_y, bool jitter) {
    u32 field = 0;
    if (jitter)
        field = sdk::VIGetNextField_80380004() ^ 1u; // 804917B8 or 804918D4
    const u32 x_word = signed_halfword_bits(raster->offset_x);
    u32 y_word = signed_halfword_bits(raster->offset_y);
    const u32 width_word = raster->width_word;
    u32 height_word = raster->height_word;
    if (double_y) { // ONLY 80491954..9CC; jitter branch deliberately doesn't double
        y_word <<= 1;
        height_word <<= 1;
    }
    const Fpr bias = double_at(0x805FC418u);
    const Fpr x = ppc::signed_word_to_single(x_word, bias);
    const Fpr y = ppc::signed_word_to_single(y_word, bias);
    const Fpr width = ppc::signed_word_to_single(width_word, bias);
    const Fpr height = ppc::signed_word_to_single(height_word, bias);
    const Fpr zero = single_at(0x805FC3F8u), one = single_at(0x805FC3FCu);
    if (jitter)
        sdk::GXSetViewportJitter_8039A444(x, y, width, height, zero, one, field); // 80491830/94C
    else
        sdk::GXSetViewport_8039A49C(x, y, width, height, zero, one); // 804918A8/9CC
}

// COMPLETE original [0x80491774,0x80491B08), 0x394 bytes, asm:5-233.
// Caller in selected primitive: BL 0x80492438. Globals additionally used here:
// 0x805F2698 (r13+6198), 0x805EEFD8 (+2AD8), 0x805F26F0 (+61F0).
// This reconstructs camera-dependent viewport/scissor logic, not a saved copy
// of every incoming GX state. Camera root case restores ONLY projection.
void Im2D_RestoreProjectionViewport_80491774() {
    RasterPrefix* raster = runtime_globals()->current_camera->core.raster; // 80491784..78C
    if (raster != raster->parent) { // 80491790..798
        if (word_at(0x805F2698u) == 0) { // 8049179C..7A4
            const bool jitter = render_mode()->field_rendering != 0;
            restore_camera_viewport(raster, false, jitter); // 804917A8..8A8
            sdk::GXSetScissor_8039A4E4(
                signed_halfword_bits(raster->offset_x), signed_halfword_bits(raster->offset_y),
                raster->width_word, raster->height_word); // BL 804918BC
        } else { // 804918C4..AE4
            const bool jitter = render_mode()->field_rendering != 0;
            restore_camera_viewport(raster, !jitter, jitter); // 804918D4..9CC
            if (word_at(0x805EEFD8u) != 0) { // 804919D0..9D8
                const u32 y = signed_halfword_bits(raster->offset_y);
                const u32 h = raster->height_word;
                const u32 limit = word_at(0x805F26F0u) + 2u;
                if (ppc::signed_word((y + h) << 1) <= ppc::signed_word(limit)) {
                    sdk::GXSetScissor_8039A4E4(signed_halfword_bits(raster->offset_x), y << 1,
                                              render_mode()->framebuffer_width, h << 1); // 80491A10
                } else if (ppc::signed_word(y << 1) > ppc::signed_word(limit)) {
                    sdk::GXSetScissor_8039A4E4(0, 0, render_mode()->framebuffer_width, limit); // 80491A34
                } else {
                    // r6 stays limit, NOT limit-2*y. Do not invent a rectangle clip formula.
                    sdk::GXSetScissor_8039A4E4(signed_halfword_bits(raster->offset_x), y << 1,
                                              render_mode()->framebuffer_width, limit); // 80491A48
                }
                sdk::GXSetScissorBoxOffset_8039A55C(0, 0); // 80491A54
            } else { // 80491A5C..AE4
                const u32 y = signed_halfword_bits(raster->offset_y);
                const u32 base = word_at(0x805F26F0u);
                const u32 threshold = base - 2u;
                if (ppc::signed_word(y << 1) >= ppc::signed_word(threshold)) {
                    sdk::GXSetScissor_8039A4E4(signed_halfword_bits(raster->offset_x), y << 1,
                                              render_mode()->framebuffer_width,
                                              raster->height_word << 1); // 80491A88
                } else {
                    const u32 h = raster->height_word;
                    const u32 limit = base + 2u;
                    if (ppc::signed_word((y + h) << 1) < ppc::signed_word(limit)) {
                        sdk::GXSetScissor_8039A4E4(0, threshold,
                                                  render_mode()->framebuffer_width, limit); // 80491AB8
                    } else {
                        sdk::GXSetScissor_8039A4E4(signed_halfword_bits(raster->offset_x), threshold,
                                                  render_mode()->framebuffer_width, h << 1); // 80491AD4
                    }
                }
                sdk::GXSetScissorBoxOffset_8039A55C(0, word_at(0x805F26F0u) - 2u); // 80491AE4
            }
        }
    }
    sdk::GXSetProjectionv_8039A158(at<GXProjectionVector>(0x805E4218u)); // 80491AF0
    // No restoration of matrix0, current matrix index, sampler binding, TEV,
    // channels, descriptors, cull, color/alpha write masks, or depth state here.
}

// Source factoring of the exact FIFO write sequence, not a fictional GX draw API.
// Gekko FIFO address: lis r7,0xCC01; stores to -0x8000(r7) => 0xCC008000.
inline void emit_textured_vertex(const Im2DVertex& vertex) {
    const Fpr z = ppc::load_single(&vertex.z); // pair block 80491F14/1F5C
    const Fpr y = ppc::load_single(&vertex.y);
    const Fpr x = ppc::load_single(&vertex.x);
    ppc::store_single(at<void>(0xCC008000u), x);
    ppc::store_single(at<void>(0xCC008000u), y);
    ppc::store_single(at<void>(0xCC008000u), z);
    const u8 alpha = vertex.alpha, blue = vertex.blue, green = vertex.green, red = vertex.red;
    *at<volatile u8>(0xCC008000u) = red;
    *at<volatile u8>(0xCC008000u) = green;
    *at<volatile u8>(0xCC008000u) = blue;
    *at<volatile u8>(0xCC008000u) = alpha;
    const Fpr v = ppc::load_single(&vertex.v);
    const Fpr u = ppc::load_single(&vertex.u);
    ppc::store_single(at<void>(0xCC008000u), u);
    ppc::store_single(at<void>(0xCC008000u), v);
}

// STRICT primitive4/count4/textured path of [0x80491E80,0x8049245C), 0x5DC.
// Scope guard is not an original check. Original blocks retained: 80491EA4..EC0,
// dispatch 80491EC4..EE8, textured guard 80491EE8..F10, pair loop 80491F14..FB0
// with CTR=2/no odd tail, and shared restore/return 80492438..458.
// Original other primitives, count values and untextured emitters are omitted.
s32 Im2D_RenderStrip4_RGBA8_Path_80491E80(u32 primitive, Im2DVertex* vertices, u32 count) {
    require_extracted_path(primitive == 4 && count == 4);
    Im2D_Prepare_Textured_RGBA8_Path_8049148C(); // original BL 80491EA4
    sdk::GXBegin_80396934(word_at(0x8056F130u), 0, 4); // BL 80491EC0, GX 0x98
    require_extracted_path(current_texture()->raster != nullptr); // 80491EF4 not-taken
    for (u32 pair = 0; pair != 2; ++pair) { // CTR=count>>1, precisely 2 for blur
        emit_textured_vertex(vertices[0]); // 80491F14..F58
        emit_textured_vertex(vertices[1]); // 80491F5C..FA4
        vertices += 2;                     // 80491F9C (original pointer stride 0x30)
    }
    // 80491FAC count&1 == 0 -> 80492438. No GXEnd call exists in this body.
    Im2D_RestoreProjectionViewport_80491774(); // BL 80492438
    return 1; // 80492440, API status is not proof of visible GPU output
}

namespace raw {
struct GXFilterPair { u32 minimum, magnification; };
inline constexpr GXFilterPair filter_pairs_8056FC90[7] = {
    {0,0}, {0,0}, {1,1}, {2,0}, {3,1}, {4,0}, {5,1}
};
inline constexpr u32 wrap_modes_8056FCC8[5] = {0,1,2,0,0};
inline constexpr u8 min_filter_bp_encoding_805EED30[6] = {0,4,1,5,2,6};
inline constexpr u32 texture_lod_zero_805FC498 = 0;
inline constexpr u64 texture_lod_bias_805FC4A0 = 0x4330000000000000ULL;
inline constexpr u32 texture_dirty = 0x01000000u;
inline constexpr u32 texture_uses_region = 0x02000000u;
inline constexpr u32 rgba8_gx_format = 6;
// SDK LOD constants: bias clamp [-4,3.99] (upper trigger 4), scale32, LOD0..10.
// The actual SDK adapter still owns quantization and register masks; these
// snapshots are not an alternative implementation or invented runtime defaults.
inline constexpr u32 gx_lod_constants_805FB7D0[7] = {
    0xC0800000,0x40800000,0x407F5C29,0x42000000,0x00000000,0x41200000,0x3D000000
};
} // namespace raw

namespace sdk {
extern "C" {
void GXInitTexObj_80397E34(GXTexObjStorage* object_r3, void* image_r4,
                         u16 width_r5, u16 height_r6, u32 format_r7,
                         u32 wrap_s_r8, u32 wrap_t_r9, u8 mipmapped_r10);
void GXInitTexObjLOD_803980C8(GXTexObjStorage* object_r3, u32 minimum_filter_r4,
                            u32 magnification_filter_r5,
                            Fpr minimum_lod_f1, Fpr maximum_lod_f2, Fpr bias_f3,
                            u8 bias_clamp_r6, u8 edge_lod_r7, u32 anisotropy_r8);
u32 GXGetTexObjMaxAniso_803982AC(const GXTexObjStorage* object_r3);
u8 GXGetTexObjEdgeLOD_80398298(const GXTexObjStorage* object_r3);
u8 GXGetTexObjBiasClamp_8039828C(const GXTexObjStorage* object_r3);
Fpr GXGetTexObjLODBias_8039824C(const GXTexObjStorage* object_r3); // f1 result
void GXLoadTexObj_8039843C(GXTexObjStorage* object_r3, u32 map_r4);
void GXLoadTexObjPreLoaded_803982C0(GXTexObjStorage* object_r3,
                                 GXTexRegionStorage* region_r4, u32 map_r5);
void DCInvalidateRange_80372508(void* base_r3, u32 bytes_r4);
}
} // namespace sdk

// NONPAL/NO-MIP RGBA8, map0 path of [0x8049AC88,0x8049B568), 0x8E0 bytes.
// Caller in this slice: 8049887C (texture != null, r4=0). Original null-texture
// fallback 8049ACB8..ACC8 and all paletted arms are excluded.
// Retained blocks: 8049ACCC..ACF4 -> 8049B1EC; dirty 8049B1F8..B21C ->
// B2C0..B360; clean B364..B3CC -> B470..B510; final B510..B544.
// Scope precondition: supplied raster is nonpal/no-mip and its parent is an
// allocated RGBA8/depth32 root with maxLOD=0, as proved for blur/scratch backing.
// Sampler values themselves are NOT fixed to defaults and are NOT clamped here.
void RwTexture_Realize_RGBA8_NoMip_Path_8049AC88(TexturePrefix* texture, u32 map) {
    require_extracted_path(texture != nullptr && map == 0);
    RasterPrefix* raster = texture->raster;
    require_extracted_path(raster != nullptr && (raster_format_byte(raster) & 0xE0u) == 0);
    TexturePluginPrefix* entry_native = texture_plugin(texture); // r29: 8049ACD0/ACDC
    RasterPluginPrefix* entry_parent = raster_plugin(raster->parent); // r27: ACCC..ACE4
    require_extracted_path(entry_parent->gx_format == 6 && raster->parent->depth_word == 32 &&
                           entry_parent->maximum_lod == 0);
    entry_parent->use_serial = *at<volatile u16>(0x805EEFF0u); // 8049ACE0..ACE8

    const u32 cached = entry_native->cached_sampler_and_flags; // 8049B1EC
    const bool dirty = (cached & 0x01000000u) != 0;             // 8049B1F0/B1F4
    // B364..B374's low16 comparison is reached only when the dirty bit is clear.
    if (dirty || (texture->filter_and_addressing & 0xFFFFu) != (cached & 0xFFFFu)) {
        Fpr lod_bias;
        u8 bias_clamp, edge_lod;
        u32 anisotropy;
        if (dirty) { // original 8049B314..B348 supplies actual fresh-object arguments
            lod_bias = single_at(0x805FC498u); // raw +0
            bias_clamp = 1;
            edge_lod = 1;
            anisotropy = 0;
        } else { // 8049B378..B3A8: preserve these settings on sampler-only changes
            anisotropy = sdk::GXGetTexObjMaxAniso_803982AC(&entry_native->object); // B37C
            edge_lod = sdk::GXGetTexObjEdgeLOD_80398298(&entry_native->object);    // B388
            bias_clamp = sdk::GXGetTexObjBiasClamp_8039828C(&entry_native->object); // B394
            lod_bias = sdk::GXGetTexObjLODBias_8039824C(&entry_native->object);    // B3A0
        }
        // The original reloads these descriptors/extensions in each init arm.
        raster = texture->raster;
        TexturePluginPrefix* native = texture_plugin(texture);
        RasterPluginPrefix* parent = raster_plugin(raster->parent);
        const u32 sampler = texture->filter_and_addressing;
        const u32 wrap_s = word_at(0x8056FCC8u + (((sampler >> 8) & 0xFu) << 2));
        const u32 wrap_t = word_at(0x8056FCC8u + (((sampler >> 12) & 0xFu) << 2));
        // 8049B2C0..B2F8 OR B470..B4A8. Width/height are raster dimensions,
        // image and GX format come from its PARENT extension. No half-texel UV
        // adjustment, image upload, palette allocation, or implicit clamp here.
        sdk::GXInitTexObj_80397E34(&native->object, parent->image,
            static_cast<u16>(raster->width_word), static_cast<u16>(raster->height_word),
            parent->gx_format, wrap_s, wrap_t, 0); // BL B2F8 / B4A8, mipmap bit is 0
        const u32 filter = texture->filter_and_addressing & 0xFFu; // B2FC / B4AC
        const u32 minimum = word_at(0x8056FC90u + (filter << 3));
        const u32 magnification = word_at(0x8056FC94u + (filter << 3));
        const Fpr zero = single_at(0x805FC498u);
        const Fpr maximum_lod = ppc::unsigned_word_to_single(
            parent->maximum_lod, double_at(0x805FC4A0u)); // B344 / B4F4; 0 here
        sdk::GXInitTexObjLOD_803980C8(&native->object, minimum, magnification,
            zero, maximum_lod, lod_bias, bias_clamp, edge_lod, anisotropy); // B348 / B4F8
        // Low16 sampler replaces low16; keep upper flags, then clear only dirty.
        native->cached_sampler_and_flags =
            ((native->cached_sampler_and_flags & 0xFFFF0000u) |
             (texture->filter_and_addressing & 0xFFFFu)) & ~0x01000000u; // B34C..B35C / B4FC..B50C
    }
    // 8049B510..B53C runs even when neither init/LOD call is necessary.
    if ((entry_native->cached_sampler_and_flags & 0x02000000u) == 0)
        sdk::GXLoadTexObj_8039843C(&entry_native->object, map); // 8049B524, real SDK callback use
    else
        sdk::GXLoadTexObjPreLoaded_803982C0(&entry_native->object,
                                           entry_parent->texture_region, map); // 8049B538
    *at<TexturePrefix*>(0x8056FCDCu + (map << 2)) = texture; // 8049B53C..B544, last-loaded table
    // No sampler restoration or conversion to a guessed clamp/linear policy.
}

// Format/create scope below includes the empty type5/default-format SCRATCH
// view, because 804336D8 calls Create(0,0,0,5), NOT 0x505. Its backing at
// 804336F0 and BOTH blur Create calls 8042F010/F024 use 0x505. The scratch
// view initially resolves RGB565, then SubRaster copies the parent's RGBA8
// format/depth/type; it does not allocate a second backing image.
namespace raw {
inline constexpr u32 type5_default_depth0_dispatch_8056F96C = 0x8049751Cu;
inline constexpr u32 rgba8_size_depth32_dispatch_8056F628 = 0x80496548u;
inline constexpr u32 blur_create_flags = 0x00000505u;
inline constexpr u32 scratch_empty_view_flags = 0x00000005u;
inline constexpr u32 backing_allocation_hint = 0x00030411u;
inline constexpr u32 generic_raster_allocation_message_8056F9F0 = 0x8056F9F0u;
} // namespace raw

// Extracted successful format paths of [0x80497458,0x80497A70), 0x618 bytes.
// 0x505: 80497464..4C0 -> 4F0 -> 765C..790 -> 7850/7854 -> 7884..789C -> 7A54.
// Empty (0,0,depth0,flags5): same prefix -> 74F0..7518, table[0]=8049751C,
// type5 branch 7524..7540 -> 7A54. No paletted/other-format/error cases claimed.
s32 RwRaster_ResolveFormat_BlurCreatePaths_80497458(RasterPrefix* raster, u32 flags) {
    require_extracted_path(flags == 0x505u ||
        (flags == 5u && raster->width_word == 0 && raster->height_word == 0 && raster->depth_word == 0));
    RasterPluginPrefix* parent = raster_plugin(raster->parent); // 80497474..480
    raster_type(raster) = static_cast<u8>(flags & 7u);        // 8049747C, type5
    raster->flags_21 = static_cast<u8>(flags & 0xF8u);        // 80497488, raw byte flags
    if (flags == 0x505u) { // selected 0x500 format branch 80497884..789C
        parent->gx_format = 6;   // 8049788C, GX_TF_RGBA8
        parent->alpha_flags = 1; // 80497894
        raster->depth_word = 32; // 80497898
        raster_format_byte(raster) = 0x05; // 80497A54..A58, no mipmap/palette flags
    } else { // selected depth0/type5 default branch 80497524..7540
        parent->gx_format = 4;   // 8049752C, GX_TF_RGB565; empty scratch VIEW ONLY
        parent->alpha_flags = 0; // 80497538
        raster->depth_word = 16; // 8049753C
        raster_format_byte(raster) = 0x02; // format |= 0x200 then >>8, 80497A54..A58
    }
    return 1; // 80497A5C
}

// Selected RGBA8 LEVEL0 path of [0x80496478,0x804965A8), 0x130 bytes.
// Bounded freshly hash-checked disassembly, not present in 81-function manifest.
// 80496484..4D0 (level=0), depth32 dispatch 8056F628 -> 80496548..564,
// final alignment 80496590..594. Other depth/level/error branches are excluded.
u32 RwRaster_Level0Size_RGBA8_Path_80496478(RasterPrefix* raster) {
    require_extracted_path(raster->depth_word == 32);
    RasterPrefix* parent = raster->parent;
    u32 width = parent->width_word, height = parent->height_word;
    if ((raster->unknown_22_23[0] & 6u) == 0) { // 8049648C; level0 sraw is identity
        if (width == 0) width = 1;             // 804964B0..4BC
        if (height == 0) height = 1;           // 804964C0..4CC
    }
    const u32 padded_width = (width + 3u) & ~3u;
    const u32 padded_height = (height + 3u) & ~3u;
    const u32 bytes = (padded_width * padded_height) << 2; // low-word multiply/shift
    return (bytes + 31u) & ~31u; // each level has its own 32-byte alignment
}

// ROOT paths of [0x80497CA8,0x80498058), 0x3B0 bytes, asm:5-240.
// r3 input is unused; r4=raster; r5=flags. Requires wrapper-initialized self
// parent and flags 0x505, or the observed empty scratch (0,0,0,5) case.
// Covers zero-sized no-storage and nonzero RGBA8 single-level allocation,
// including both actual error-report calls on pixel-allocation failure.
// Does NOT implement generic palette/mipmap/camera raster allocation.
s32 RwRaster_BackendCreate_BlurPaths_80497CA8(void* unused_r3, RasterPrefix* raster, u32 flags) {
    (void)unused_r3;
    require_extracted_path(raster->parent == raster &&
        (flags == 0x505u || (flags == 5u && raster->width_word == 0 &&
                            raster->height_word == 0 && raster->depth_word == 0)));
    RasterPluginPrefix* plugin = raster_plugin(raster); // 80497CD0/CD8
    ppc::store_word(&raster->unknown_18_1b, 0);         // 80497CD4, raster stride word
    plugin->gx_format = 0xFF;                         // 80497CE4
    plugin->format_auxiliary = 0xFF;                  // 80497CE8
    plugin->alpha_flags = 0;                          // 80497CEC
    plugin->allocation = nullptr;                    // 80497CF0
    plugin->image = nullptr;                         // 80497CF4
    plugin->auxiliary_image = nullptr;               // 80497CF8
    plugin->unknown_24 = 0;                           // 80497CFC
    plugin->unknown_28 = 0;                           // 80497D00
    plugin->texture_region = nullptr;                // 80497D04
    plugin->use_serial = *at<volatile u16>(0x805F2720u); // 80497D08..D0C
    plugin->maximum_lod = 0xFF;                       // 80497D10
    plugin->unknown_33 = 0xFF;                        // 80497D14
    if (!RwRaster_ResolveFormat_BlurCreatePaths_80497458(raster, flags)) // BL 80497D18
        return 0; // 80497D24 (retained status check, selected resolver succeeds)
    if (raster->width_word == 0 || raster->height_word == 0) { // 80497D2C..D40
        raster->flags_21 = 0x80; // 8049802C: don't allocate, but do NOT zero depth/format
        return 1;               // 80498028
    }
    // Type5, don't-allocate bit clear, mipmap bit clear: 80497D44..DBC.
    // Known one-level loop: 80497DC0..DF0, E60 (postdecrement to level0),
    // E4C..E5C (one call with r4=0), E60..E70 (last counter becomes 0xFFFFFFFF).
    plugin = raster_plugin(raster); // original r31 at 80497DC8
    plugin->maximum_lod = 0;        // 80497DCC, exactly one level
    const u32 image_bytes = RwRaster_Level0Size_RGBA8_Path_80496478(raster); // BL 80497E58
    const u32 allocation_bytes = image_bytes + 31u;
    // REAL INDIRECT allocator boundary: word[word[0x805F265C]+0x108],
    // (r3=allocation_bytes, r4=0x30411), no hidden engine this. Not operator new.
    plugin->allocation = runtime_globals()->allocate(allocation_bytes, 0x30411u); // BCTRL 80497F48
    if (plugin->allocation == nullptr) { // 80497F50..F88 -> FB4/FB8 not-taken
        RwErrorRecord allocation_error;
        allocation_error.plugin = 1;
        allocation_error.code = sdk::RwErrorCode_8047EC78(0x80000013u, allocation_bytes); // 80497F74
        sdk::RwErrorSet_8047EBD4(&allocation_error); // 80497F80
        RwErrorRecord raster_error;
        raster_error.plugin = 1;
        raster_error.code = sdk::RwErrorCode_8047EC78(2, at<const char>(0x8056F9F0u)); // 80497FD4
        sdk::RwErrorSet_8047EBD4(&raster_error); // 80497FE0
        return 0; // 80497FE4; no invented backing or fake success
    }
    // 80497F8C..FAC: allocation padding aligns the IMAGE, not the owning pointer.
    plugin->image = at<void>((reinterpret_cast<u32>(plugin->allocation) + 31u) & ~31u);
    sdk::DCInvalidateRange_80372508(plugin->image, image_bytes); // BL 80497FAC, type5
    // No memset, initial black frame, pre-upload, or GX copy is performed here.
    return 1; // 80497FB0 -> 80498034
}

// COMPLETE original [0x804981B8,0x804981E8), 0x30 bytes, asm:5-16.
// Wrapper 8048AE14 calls engine+0x78 at 8048AE8C with (view,parent,0).
// This callback doesn't attach the parent itself: wrapper 8048AE98..AEA0 later
// assigns view->parent=parent->parent on success. No raster-plugin alpha/LOD
// metadata is copied, and no image storage is allocated by this callback.
s32 RwRaster_BackendSubRaster_804981B8(RasterPrefix* view, RasterPrefix* parent,
                                     u32 unused_r5) {
    (void)unused_r5;
    ppc::store_word(&view->unknown_18_1b, ppc::load_word(&parent->unknown_18_1b)); // 804981B8..1C0
    view->depth_word = parent->depth_word; // 804981C4..1C8
    raster_type(view) = raster_type(parent); // 804981CC..1D0
    raster_format_byte(view) = raster_format_byte(parent); // 804981D4..1D8
    ppc::store_word(&view->unknown_04_0b, 0); // 804981DC, pPixels at +4
    return 1; // 804981E0
}

namespace raw {
// Selected records from 27-record source table 0x8051D400; do not reinterpret
// these as newly installed function pointers. Driver initialization/live table
// replacement remain external. Wrapper uses confirm the noted engine offsets.
struct StandardCallbackRecord { u32 index, address; };
inline constexpr StandardCallbackRecord blur_standard_records[] = {
    {4,  0x80497CA8}, // record 0x8051D448, RasterCreate -> engine+0x58, BCTRL at 8048AF58
    {8,  0x8049819C}, // record 0x8051D458, TextureSetRaster
    {12, 0x804981B8}  // record 0x8051D4B0, SubRaster -> engine+0x78, BCTRL at 8048AE8C
};
inline constexpr u32 raster_plugin_bytes = 0x34; // 80496448, registration BL 80496460
inline constexpr u32 raster_plugin_id = 0x40C;   // 8049644C; result stored 805F2700 at 80496464
} // namespace raw

} // namespace backend
} // namespace motion_blur

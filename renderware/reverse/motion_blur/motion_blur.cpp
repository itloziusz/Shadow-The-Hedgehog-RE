#include "motion_blur.h"

// Complete core [0x8042E254,0x8042F258) and six direct task entries.
// Original address ranges below are half-open; sizes are hexadecimal.
// "Caller" lists verified direct BL sites, not an exhaustive indirect call graph.
// INTEGRATION SCOPE UPDATE: the preserved backend fragment is appended AFTER
// the closed core namespace, then the explicitly selected input reconstructions.
// Shared bodies are still real-address boundaries, never invented stub success.
// original::RwRaster_CopyEFB is now defined; the other original:: declarations
// remain ABI/address-binding obligations. No selected callbacks are installed.
// The original task vtable addresses are data, not vtables for these C++ symbols.
// Static reconstruction only: final syntax/LLVM-IR checks are documented in
// ../evidence/source_integration.md. No PPC machine object, link or runtime proof.
// State12 below is a requested RW concept: the selected backend proves it is
// unsupported (getter leaves stack residue, setter is ineffective). Keep both.

#if defined(__clang__)
#pragma clang fp contract(off)
#endif

namespace motion_blur {

// Reconstruction notation, NOT additional original functions or SDK calls.
// FPRs are represented by double, as on PPC. Each ...s operation below is the
// actual single-rounding PPC instruction; there is no implicit C++ multiply/add
// contraction, double-expression reassociation, or guessed host fenv behavior.
// Loads/stores also avoid C++ evaluation of uninitialized float/word objects.
// These helpers require the real 32-bit PPC target and its current FPSCR. They
// do not initialize/change FPSCR, emulate another target, or establish a runtime
// FPSCR value. Inline-assembly compilation and the external ABI need validation.
namespace ppc {
using Fpr = f64;

inline u32 load_word(const void* address) {
    u32 value;
    __asm__ volatile("lwz %0,0(%1)" : "=r"(value) : "b"(address) : "memory");
    return value;
}
inline void store_word(void* address, u32 value) {
    __asm__ volatile("stw %0,0(%1)" : : "r"(value), "b"(address) : "memory");
}
inline Fpr load_single(const void* address) {
    Fpr value;
    __asm__ volatile("lfs %0,0(%1)" : "=f"(value) : "b"(address) : "memory");
    return value;
}
inline void store_single(void* address, Fpr value) {
    __asm__ volatile("stfs %0,0(%1)" : : "f"(value), "b"(address) : "memory");
}
inline Fpr load_double(const void* address) {
    Fpr value;
    __asm__ volatile("lfd %0,0(%1)" : "=f"(value) : "b"(address) : "memory");
    return value;
}
inline Fpr add_single(Fpr left, Fpr right) {
    Fpr value;
    __asm__ volatile("fadds %0,%1,%2" : "=f"(value) : "f"(left), "f"(right) : "memory");
    return value;
}
inline Fpr sub_single(Fpr left, Fpr right) {
    Fpr value;
    __asm__ volatile("fsubs %0,%1,%2" : "=f"(value) : "f"(left), "f"(right) : "memory");
    return value;
}
inline Fpr mul_single(Fpr left, Fpr right) {
    Fpr value;
    __asm__ volatile("fmuls %0,%1,%2" : "=f"(value) : "f"(left), "f"(right) : "memory");
    return value;
}
inline Fpr div_single(Fpr left, Fpr right) {
    Fpr value;
    __asm__ volatile("fdivs %0,%1,%2" : "=f"(value) : "f"(left), "f"(right) : "memory");
    return value;
}
inline Fpr negate(Fpr input) {
    Fpr value;
    __asm__ volatile("fneg %0,%1" : "=f"(value) : "f"(input) : "memory");
    return value;
}
inline Fpr signed_word_to_single(u32 word, Fpr signed_bias) {
    // xoris low-word,0x8000; stack high word 0x43300000; lfd; fsubs.
    alignas(8) const u64 bits = 0x4330000000000000ULL | (word ^ 0x80000000u);
    return sub_single(load_double(&bits), signed_bias);
}
inline Fpr unsigned_word_to_single(u32 word, Fpr unsigned_bias) {
    // No xoris: even words >= 0x80000000 are UNSIGNED here.
    alignas(8) const u64 bits = 0x4330000000000000ULL | word;
    return sub_single(load_double(&bits), unsigned_bias);
}
inline s32 signed_word(u32 word) {
    // A defined signed interpretation without an out-of-range unsigned->signed cast.
    if (word <= 0x7FFFFFFFu)
        return static_cast<s32>(word);
    return -1 - static_cast<s32>(0xFFFFFFFFu - word);
}
inline s32 sign_extend_low16(u32 word) {
    const s32 low = static_cast<s32>(word & 0xFFFFu);
    return low < 0x8000 ? low : low - 0x10000;
}
} // namespace ppc

namespace {
using ppc::Fpr;

// Non-original source factoring of the actual engine-table accesses. Reload
// word[0x805F265C] for EVERY call, exactly as the core does. No null guards,
// hidden device this, cached table binding, or save/restore policy is added.
EnginePrefix* current_engine() {
    return *reinterpret_cast<EnginePrefix* volatile*>(constants::engine_pointer_805F265C);
}
void get_state(u32 state, u32* output) {
    (void)current_engine()->device.get_state(state, output); // engine+0x24
}
void set_state(u32 state, u32 word) {
    (void)current_engine()->device.set_state(state, word);   // engine+0x20
}
void render_strip(Im2DVertex* vertices) {
    // engine+0x30 -> descriptor 0x8056F50C+0x20 -> 0x80491E80 in static data.
    // 0x80491EAC..0x80491EC0 maps RW 4 through [0x8056F130]=GX 0x98 and
    // calls GXBegin at 0x80396934 with format 0/count 4. Backend body is separate.
    (void)current_engine()->device.render_primitive(constants::rw_triangle_strip,
                                                    vertices, 4);
}
void white_with_current_alpha(Im2DVertex& vertex, const BlurParameters& parameters) {
    vertex.red = 0xFF;
    vertex.green = 0xFF;
    vertex.blue = 0xFF;
    vertex.alpha = parameters.alpha; // a separate reload for each vertex
}
void initialize_depth_and_white(Im2DVertex& vertex) {
    ppc::store_single(&vertex.z, ppc::load_single(&current_engine()->device.screen_near_z));
    vertex.red = 0xFF;
    vertex.green = 0xFF;
    vertex.blue = 0xFF;
    vertex.alpha = 0xFF;
    // Deliberately NO x, y, u, or v initialization.
}
void copy_vertex_storage(Im2DVertex& destination, const Im2DVertex& source) {
    // Source-level factoring of uninterrupted lfs/stfs and lbz/stb transfers,
    // NOT an original call or invented field defaults. Draw-to-full x/y/u/v
    // still read indeterminate allocation bytes through the PPC memory helpers.
    // The first full vertex is explicitly interleaved at 0x8042EC24..0x8042ECB0:
    // do NOT group it here across the original trapping height arithmetic.
    ppc::store_single(&destination.x, ppc::load_single(&source.x));
    ppc::store_single(&destination.y, ppc::load_single(&source.y));
    ppc::store_single(&destination.z, ppc::load_single(&source.z));
    destination.red = source.red;
    destination.green = source.green;
    destination.blue = source.blue;
    destination.alpha = source.alpha;
    ppc::store_single(&destination.u, ppc::load_single(&source.u));
    ppc::store_single(&destination.v, ppc::load_single(&source.v));
}
} // namespace

/*
 * Original [0x8042E254,0x8042E838), size 0x5E4.
 * Evidence: original/8042E254_MotionBlur_Draw.asm:5-381.
 * Caller: 0x8042F184 (manager wrapper 0x8042F16C).
 * Callees: 0x8042B620,0x803A1AFC,0x8048AB7C,0x8048652C,0x80486504;
 *   provider vtable+0x24; engine+0x24/+0x20/+0x30 (getter/setter/primitive).
 * Globals: 0x805E25C8 via getter; RTTI 0x8056BCE4/0x8056BCD0;
 *   0x805F265C; 0x8051C378..0x8051C39F (templates, half/neutral/biases).
 *   Static device table 0x8056F50C; primitive map entry 0x8056F130=0x98.
 * Confidence: high for exported instructions/raw constants and operation graph;
 *   live callback installation, FPSCR and GPU results are not runtime-verified.
 */
void MotionBlur_Draw_8042E254(BlurState* state) {
    // 0x8042E268..0x8042E2C8: validity is intentionally AFTER context/camera calls.
    if (state->parameters.enabled == 0)
        return;
    if (state->parameters.alpha == 0)
        return;
    void* context = original::Context_8042B620(); // BL 0x8042E280
    auto* provider = static_cast<CameraProviderPrefix*>(original::RuntimeCast_803A1AFC(
        context, 0, reinterpret_cast<const void*>(0x8056BCE4u),
        reinterpret_cast<const void*>(0x8056BCD0u), 0)); // BL 0x8042E29C
    if (provider == nullptr)
        return;
    CameraPrefix* camera = provider->vtable->get_camera(provider); // BCTRL 0x8042E2B4
    if (camera == nullptr)
        return;
    if (state->saved_valid == 0)
        return;

    // 0x8042E2CC..0x8042E3A8: ignored getter status, no fabricated defaults.
    // If a getter fails to write, its saved word is original-style stack residue.
    u32 old_source_blend, old_destination_blend, old_vertex_alpha;
    u32 old_z_test, old_z_write, old_fog, old_filter;
    get_state(10, &old_source_blend);       // 0x8042E2E8
    get_state(11, &old_destination_blend);  // 0x8042E308
    get_state(12, &old_vertex_alpha);       // 0x8042E328
    get_state(6, &old_z_test);              // 0x8042E348
    get_state(8, &old_z_write);             // 0x8042E368
    get_state(14, &old_fog);                // 0x8042E388
    get_state(9, &old_filter);              // 0x8042E3A8

    RasterPrefix* raster = camera->raster; // 0x8042E3AC
    s16 offset_x, offset_y;
    (void)original::RwRaster_GetOffset_8048AB7C(raster, &offset_x, &offset_y); // 0x8042E3BC

    // 0x8042E3C0..0x8042E408: wrap the INTEGER sums before any conversion.
    const u32 width_word = raster->width_word;
    const u32 height_word = raster->height_word;
    const u32 x_word = static_cast<u32>(static_cast<s32>(offset_x));
    const u32 y_word = static_cast<u32>(static_cast<s32>(offset_y));
    const u32 right_word = x_word + width_word;    // add at 0x8042E3D8, modulo 2^32
    const u32 bottom_word = y_word + height_word;  // add at 0x8042E3E8, modulo 2^32
    RasterPrefix* parent = raster->parent;        // exactly ONE parent dereference
    const u32 parent_width_word = parent->width_word;
    const u32 parent_height_word = parent->height_word;

    // 0x8042E418..0x8042E4F0: four 0.5-biased UVs. Every conversion/add/division
    // retains its own f32 round point; duplicate denominator conversions remain.
    const Fpr signed_bias = ppc::load_double(&constants::signed_bias_8051C390);
    const Fpr x = ppc::signed_word_to_single(x_word, signed_bias);                  // E42C
    const Fpr half = ppc::load_single(&constants::half_8051C388);                    // E430
    const Fpr unsigned_bias = ppc::load_double(&constants::unsigned_bias_8051C398); // E438
    const Fpr u0_numerator = ppc::add_single(half, x);                              // E440
    const Fpr right = ppc::signed_word_to_single(right_word, signed_bias);          // E44C
    const Fpr u1_numerator = ppc::add_single(half, right);                          // E45C
    const Fpr y = ppc::signed_word_to_single(y_word, signed_bias);                  // E464
    const Fpr bottom = ppc::signed_word_to_single(bottom_word, signed_bias);        // E474
    const Fpr v0_numerator = ppc::add_single(half, y);                              // E480
    const Fpr v1_numerator = ppc::add_single(half, bottom);                         // E48C
    const Fpr parent_width0 = ppc::unsigned_word_to_single(parent_width_word, unsigned_bias); // E494
    const Fpr u0 = ppc::div_single(u0_numerator, parent_width0);                    // E4A0
    const Fpr parent_width1 = ppc::unsigned_word_to_single(parent_width_word, unsigned_bias); // E4B8
    const Fpr parent_height0 = ppc::unsigned_word_to_single(parent_height_word, unsigned_bias); // E4C0
    const Fpr parent_height1 = ppc::unsigned_word_to_single(parent_height_word, unsigned_bias); // E4CC
    const Fpr u1 = ppc::div_single(u1_numerator, parent_width1);                    // E4D0
    const Fpr v0 = ppc::div_single(v0_numerator, parent_height0);                    // E4E0
    const Fpr v1 = ppc::div_single(v1_numerator, parent_height1);                    // E4E8
    // E4BC/E4C8/E4D4/E4D8 first copy zero template words 0x8051C378..387 to
    // stack UV pairs; E4DC/E4E4/E4EC/E4F0 overwrite ALL of them. No extra input.

    Im2DVertex* vertices = state->draw_vertices;
    // 0x8042E4F4..0x8042E528: TL, BL, TR, BR, not perimeter-order GX_QUADS.
    ppc::store_single(&vertices[0].u, u0);
    ppc::store_single(&vertices[0].v, v0);
    const Fpr neutral = ppc::load_single(&constants::neutral_8051C38C); // E50C
    ppc::store_single(&vertices[1].u, u0);
    ppc::store_single(&vertices[1].v, v1);
    ppc::store_single(&vertices[2].u, u1);
    ppc::store_single(&vertices[2].v, v0);
    ppc::store_single(&vertices[3].u, u1);
    ppc::store_single(&vertices[3].v, v1);
    white_with_current_alpha(vertices[0], state->parameters); // E52C..E540
    white_with_current_alpha(vertices[1], state->parameters); // E544..E558
    const Fpr width = ppc::signed_word_to_single(width_word, signed_bias); // E564
    white_with_current_alpha(vertices[2], state->parameters); // E560..E578
    white_with_current_alpha(vertices[3], state->parameters); // E57C..E590

    // 0x8042E598..0x8042E600: ABSOLUTE per-call geometry. No old x/y inputs,
    // += accumulation, size/UV clamp, offset addition to positions, or hidden FMA.
    const Fpr scale = ppc::load_single(&state->parameters.geometry_scale); // E598
    const Fpr height = ppc::signed_word_to_single(height_word, signed_bias); // E59C
    const Fpr delta = ppc::sub_single(scale, neutral);       // E5A4: scale - 1
    const Fpr height_product = ppc::mul_single(delta, height); // E5AC, BEFORE width
    const Fpr width_product = ppc::mul_single(delta, width); // E5B4
    const Fpr height_again = ppc::signed_word_to_single(height_word, signed_bias); // E5BC
    const Fpr dy = ppc::mul_single(half, height_product);     // E5C0
    const Fpr dx = ppc::mul_single(half, width_product);      // E5C4
    const Fpr top = ppc::negate(dy);                         // E5D0: preserves -0
    const Fpr left = ppc::negate(dx);                        // E5D4: preserves -0
    const Fpr width_again = ppc::signed_word_to_single(width_word, signed_bias); // E5D8
    const Fpr expanded_bottom = ppc::add_single(height_again, dy); // E5DC
    ppc::store_single(&vertices[0].x, left);                 // E5E0
    const Fpr expanded_right = ppc::add_single(width_again, dx); // E5E4
    ppc::store_single(&vertices[0].y, top);                  // E5E8
    ppc::store_single(&vertices[1].x, left);                 // E5EC
    ppc::store_single(&vertices[1].y, expanded_bottom);      // E5F0
    ppc::store_single(&vertices[2].x, expanded_right);        // E5F4
    ppc::store_single(&vertices[2].y, top);                  // E5F8
    ppc::store_single(&vertices[3].x, expanded_right);        // E5FC
    ppc::store_single(&vertices[3].y, expanded_bottom);      // E600
    // All four z values stay as constructed; raster offsets are handled later
    // by backend matrix translation at 0x804916FC..0x80491754, not by this core.

    RasterPrefix* texture = state->history_view->parent;    // E604/E608: NO null guard
    if (original::RwCamera_BeginUpdate_8048652C(camera) == nullptr) // E60C/E614
        return; // coordinates/getters above already happened; no end/restore calls

    // 0x8042E618..0x8042E738: GX bridge through the live RenderWare table.
    // RW 5/6 maps to GX 4/5 via 0x8056FA28. Compound hardware state is backend work.
    set_state(1, reinterpret_cast<u32>(texture)); // E634: texture is NOT saved
    set_state(10, 5);                            // E654: source alpha
    set_state(11, 6);                            // E674: inverse source alpha
    set_state(12, 1);                            // E694: vertex alpha
    set_state(6, 0);                             // E6B4: RW Z test
    set_state(8, 0);                             // E6D4: RW Z write
    set_state(14, 0);                            // E6F4: fog
    set_state(9, 1);                             // E714: texture filter
    render_strip(vertices);                     // E738: (4,state+0x14,4)

    // 0x8042E73C..0x8042E820: same seven states, in the same order. DO NOT
    // restore state 1 or append a universal GX/texture/projection reset here.
    set_state(10, ppc::load_word(&old_source_blend));      // E758
    set_state(11, ppc::load_word(&old_destination_blend)); // E778
    set_state(12, ppc::load_word(&old_vertex_alpha));      // E798
    set_state(6, ppc::load_word(&old_z_test));             // E7B8
    set_state(8, ppc::load_word(&old_z_write));            // E7D8
    set_state(14, ppc::load_word(&old_fog));               // E7F8
    set_state(9, ppc::load_word(&old_filter));             // E818
    (void)original::RwCamera_EndUpdate_80486504(camera);   // E820
}

/*
 * Original [0x8042E838,0x8042E9C4), size 0x18C.
 * Evidence: original/8042E838_MotionBlur_SaveScreen.asm:5-103.
 * Caller: 0x8042F158 (manager wrapper 0x8042F140).
 * Callees: 0x8042B620,0x803A1AFC; provider vtable+0x24; 0x80496208 twice;
 *   0x8048652C/0x80486504 twice conditionally; engine+0x20/+0x30.
 * Globals: 0x805E25C8 via getter, RTTI 0x8056BCE4/0x8056BCD0, 0x805F265C;
 *   copy backend independently consumes clear word 0x805F2688, NOT argument 2.
 * Confidence: high for call/guard order and stores; capture completion, initial
 *   backing contents and temporal image age require external/runtime evidence.
 */
void MotionBlur_SaveScreen_8042E838(BlurState* state) {
    state->saved_valid = 0; // 0x8042E850: clear before even enable/alpha gates
    if (state->parameters.enabled == 0)
        return;
    if (state->parameters.alpha == 0)
        return;
    void* context = original::Context_8042B620(); // 0x8042E86C
    auto* provider = static_cast<CameraProviderPrefix*>(original::RuntimeCast_803A1AFC(
        context, 0, reinterpret_cast<const void*>(0x8056BCE4u),
        reinterpret_cast<const void*>(0x8056BCD0u), 0)); // 0x8042E888
    if (provider == nullptr)
        return;
    CameraPrefix* camera = provider->vtable->get_camera(provider); // 0x8042E8A0
    if (camera == nullptr)
        return;

    // 0x8042E8AC..0x8042E8C0: NO owner/scratch null guard. Scratch view and its
    // parent are real raster descriptors, not a fictional renderer/texture handle.
    RasterPrefix* scratch_parent = state->owner->screen_view->parent;
    original::RwRaster_CopyEFB_80496208(scratch_parent, 0); // E8C0: downsample=0
    if (original::RwCamera_BeginUpdate_8048652C(camera) != nullptr) { // E8C8
        // 0x8042E8D4..0x8042E95C: first GX bridge, ONE/ZERO RW blend pair.
        set_state(1, reinterpret_cast<u32>(scratch_parent)); // E8F0
        set_state(10, 2);                                  // E910
        set_state(11, 1);                                  // E930
        render_strip(state->half_size_vertices);            // E954: (4,state+7C,4)
        (void)original::RwCamera_EndUpdate_80486504(camera); // E95C
    }

    original::RwRaster_CopyEFB_80496208(state->history_view, 0); // E968, even if begin failed
    if (original::RwCamera_BeginUpdate_8048652C(camera) != nullptr) { // E970
        // E97C..E9A4: no additional texture bind, blend change, or state restore.
        render_strip(state->full_size_vertices);             // E99C: (4,state+DC,4)
        (void)original::RwCamera_EndUpdate_80486504(camera);  // E9A4
    }
    state->saved_valid = 1; // E9AC: even if either or BOTH optional begin calls failed
}

/*
 * Original [0x8042E9C4,0x8042E9F0), size 0x2C.
 * Evidence: original/8042E9C4_MotionBlur_CopyParameters.asm:5-15.
 * Caller: 0x8042F0A4. Callees: none. Globals: none.
 * Confidence: high, exact member/load/store order; no +2/+3 padding transfer.
 */
BlurParameters* MotionBlur_CopyParameters_8042E9C4(BlurParameters* destination,
                                                  const BlurParameters* source) {
    destination->enabled = source->enabled;
    destination->alpha = source->alpha;
    ppc::store_single(&destination->geometry_scale, ppc::load_single(&source->geometry_scale));
    const u32 auxiliary_08 = ppc::load_word(&source->auxiliary_08);
    const u32 auxiliary_0c = ppc::load_word(&source->auxiliary_0c);
    ppc::store_word(&destination->auxiliary_08, auxiliary_08);
    ppc::store_word(&destination->auxiliary_0c, auxiliary_0c);
    return destination; // original r3 is unchanged
}

/*
 * Original [0x8042E9F0,0x8042EA1C), size 0x2C.
 * Evidence: original/8042E9F0_MotionBlur_AssignParameters.asm:5-15.
 * Caller: 0x8042F100. Callees: none. Globals: none.
 * Confidence: high; scheduled differently from copy entry. No source null guard,
 *   no whole-parameter C++ assignment/memcpy, and no padding or valid-byte copy.
 */
BlurParameters* MotionBlur_AssignParameters_8042E9F0(BlurParameters* destination,
                                                    const BlurParameters* source) {
    const u8 enabled = source->enabled; // E9F0
    const u8 alpha = source->alpha;     // E9F4
    destination->enabled = enabled;   // E9F8
    const Fpr scale = ppc::load_single(&source->geometry_scale); // E9FC
    destination->alpha = alpha;       // EA00
    const u32 auxiliary_08 = ppc::load_word(&source->auxiliary_08); // EA04
    ppc::store_single(&destination->geometry_scale, scale);       // EA08
    const u32 auxiliary_0c = ppc::load_word(&source->auxiliary_0c); // EA0C
    ppc::store_word(&destination->auxiliary_08, auxiliary_08);     // EA10
    ppc::store_word(&destination->auxiliary_0c, auxiliary_0c);     // EA14
    return destination;
}

/*
 * Original [0x8042EA1C,0x8042EA50), size 0x34.
 * Evidence: original/8042EA1C_MotionBlur_AssignParametersIfPresent.asm:5-17.
 * Callers: 0x8042F048,0x8042F12C. Callees: none. Globals: none.
 * Confidence: high; only the SOURCE pointer is null-checked, before any read.
 */
BlurParameters* MotionBlur_AssignParametersIfPresent_8042EA1C(
    BlurParameters* destination, const BlurParameters* source) {
    if (source == nullptr)
        return destination;
    destination->enabled = source->enabled;
    destination->alpha = source->alpha;
    ppc::store_single(&destination->geometry_scale, ppc::load_single(&source->geometry_scale));
    const u32 auxiliary_08 = ppc::load_word(&source->auxiliary_08);
    const u32 auxiliary_0c = ppc::load_word(&source->auxiliary_0c);
    ppc::store_word(&destination->auxiliary_08, auxiliary_08);
    ppc::store_word(&destination->auxiliary_0c, auxiliary_0c);
    return destination;
}

/*
 * Original [0x8042EA50,0x8042F084), size 0x634.
 * Evidence: original/8042EA50_MotionBlur_Construct.asm:5-401.
 * Caller: 0x8042F23C. Callees: 0x8042B620,0x803A1AFC, provider vtable+0x24,
 *   0x8048AEC4 twice,0x8048AE14,0x8042EA1C on supplied-parameter path.
 * Globals: 0x805E25C8 via getter; RTTI 0x8056BCE4/0x8056BCD0; 0x805F265C;
 *   zero templates 0x8051C358..387; half/one/zero/biases 0x8051C388..3A3.
 * Confidence: high for final known fields and failure/call behavior; allocation
 *   bytes remain unknown. The early path is a PARTIAL construction, not failure
 *   returning null. Later +0x74/+0x78 MUST NOT be pre-zeroed before camera lookup.
 */
BlurState* MotionBlur_Construct_8042EA50(BlurState* state, EffectManagerPrefix* owner,
                                       const BlurParameters* parameters) {
    // 0x8042EA6C..0x8042EA80: only these prefix stores precede the lookup.
    state->parameters.enabled = 0;
    const Fpr neutral = ppc::load_single(&constants::neutral_8051C38C);
    state->parameters.alpha = 0;
    ppc::store_single(&state->parameters.geometry_scale, neutral);
    state->owner = owner;
    // +2/+3, +8/+C, all vertices, history_view and saved_valid are untouched so far.
    void* context = original::Context_8042B620(); // 0x8042EA84
    auto* provider = static_cast<CameraProviderPrefix*>(original::RuntimeCast_803A1AFC(
        context, 0, reinterpret_cast<const void*>(0x8056BCE4u),
        reinterpret_cast<const void*>(0x8056BCD0u), 0)); // 0x8042EAA0
    if (provider == nullptr)
        return state; // EAA8..EAB0: do not apply supplied parameters/default alpha 0x80
    CameraPrefix* camera = provider->vtable->get_camera(provider); // 0x8042EAC0
    if (camera == nullptr)
        return state; // EAC8..EAD0: +74/+78 still unresolved allocation bytes

    // 0x8042EAF4..0x8042EB88: four separate runtime device-pointer/near-Z loads.
    initialize_depth_and_white(state->draw_vertices[0]); // EAF4..EB28
    initialize_depth_and_white(state->draw_vertices[1]); // EB2C..EB4C
    initialize_depth_and_white(state->draw_vertices[2]); // EB50..EB6C
    initialize_depth_and_white(state->draw_vertices[3]); // EB70..EB88
    state->history_view = nullptr; // EB8C, only after the camera-success path
    state->saved_valid = 0;        // EB90

    RasterPrefix* parent = camera->raster->parent; // EB94..EB9C: immediate parent
    const u32 width_word = parent->width_word;     // EBA4
    const u32 height_word = parent->height_word;   // EBA8
    const Fpr unsigned_bias = ppc::load_double(&constants::unsigned_bias_8051C398);
    const Fpr signed_bias = ppc::load_double(&constants::signed_bias_8051C390);
    const Fpr half = ppc::load_single(&constants::half_8051C388);

    // Stack rectangle templates 0x8051C358..367 are all ZERO words. EBFC/EC00
    // preserve x/y=0; EC08/EC0C replace width/height with the parent words.
    // This is a local initialized rectangle, NOT a whole-object zero fill.
    RasterRect rectangle;
    rectangle.x = 0;
    rectangle.y = 0;
    rectangle.width = ppc::signed_word(width_word);
    rectangle.height = ppc::signed_word(height_word);

    // 0x8042EBC4..0x8042EC14: parent denominator UNSIGNED, upper numerator SIGNED.
    // Even in the constructor it is not safe to make both conversions signed.
    const Fpr width_unsigned0 = ppc::unsigned_word_to_single(width_word, unsigned_bias); // EBC4
    const Fpr width_signed = ppc::signed_word_to_single(width_word, signed_bias);         // EBCC
    const Fpr right_numerator = ppc::add_single(half, width_signed);                     // EBDC
    const Fpr width_unsigned1 = ppc::unsigned_word_to_single(width_word, unsigned_bias); // EBE4
    const Fpr u0 = ppc::div_single(half, width_unsigned0);                               // EBEC
    const Fpr u1 = ppc::div_single(right_numerator, width_unsigned1);                    // EC04
    // Zero UV templates 0x8051C368..377 are overwritten at EC10/EC14/ECC0/ECCC.

    Im2DVertex* full = state->full_size_vertices;
    // 0x8042EC24..0x8042ECB0: FIRST full-size vertex, interleaved with height UV.
    // Raw draw x/y/u/v are still uninitialized allocation bytes. Preserve each
    // object load/store around the actual FSUBS/FADDS/FDIVS points; a grouped
    // copy would write blue/alpha/u/v before the original EC78/ECA0 divisions.
    // This is source event order, NOT original register/stack/exception ABI.
    const Fpr first_x = ppc::load_single(&state->draw_vertices[0].x); // EC24
    ppc::store_single(&full[0].x, first_x);                           // EC30, B+DC
    const Fpr first_y = ppc::load_single(&state->draw_vertices[0].y); // EC34
    ppc::store_single(&full[0].y, first_y);                           // EC3C, B+E0
    const Fpr first_z = ppc::load_single(&state->draw_vertices[0].z); // EC44
    const Fpr height_signed = ppc::signed_word_to_single(height_word, signed_bias); // EC48
    ppc::store_single(&full[0].z, first_z);                           // EC50, B+E4
    const Fpr bottom_numerator = ppc::add_single(half, height_signed); // EC54
    const u8 first_red = state->draw_vertices[0].red;                 // EC58
    full[0].red = first_red;                                         // EC60, B+E8
    const u8 first_green = state->draw_vertices[0].green;             // EC68
    const Fpr height_unsigned0 = ppc::unsigned_word_to_single(height_word, unsigned_bias); // EC6C
    full[0].green = first_green;                                     // EC74, B+E9
    const Fpr v0 = ppc::div_single(half, height_unsigned0);            // EC78
    const u8 first_blue = state->draw_vertices[0].blue;               // EC7C
    full[0].blue = first_blue;                                       // EC84, B+EA
    const u8 first_alpha = state->draw_vertices[0].alpha;             // EC8C
    const Fpr height_unsigned1 = ppc::unsigned_word_to_single(height_word, unsigned_bias); // EC90
    full[0].alpha = first_alpha;                                     // EC94, B+EB
    const Fpr first_u = ppc::load_single(&state->draw_vertices[0].u); // EC98
    const Fpr v1 = ppc::div_single(bottom_numerator, height_unsigned1); // ECA0
    ppc::store_single(&full[0].u, first_u);                           // ECA4, B+EC
    const Fpr first_v = ppc::load_single(&state->draw_vertices[0].v); // ECA8
    ppc::store_single(&full[0].v, first_v);                           // ECB0, B+F0

    // 0x8042ECB4..0x8042EDA8: remaining draw-to-full copies contain no FP
    // arithmetic. Retain their field order; all transient full x/y/u/v are
    // overwritten at EDAC..EE38, without inventing draw-field initialization.
    copy_vertex_storage(full[1], state->draw_vertices[1]); // ECB4..ED00
    copy_vertex_storage(full[2], state->draw_vertices[2]); // ED04..ED48
    copy_vertex_storage(full[3], state->draw_vertices[3]); // ED4C..EDA8
    const Fpr zero = ppc::load_single(&constants::zero_8051C3A0); // ED64, +0.0

    // 0x8042EDAC..0x8042EE38: full-size TL/BL/TR/BR, full-range biased UVs.
    ppc::store_single(&full[0].x, zero); // EDAC
    ppc::store_single(&full[0].y, zero); // EDB0
    ppc::store_single(&full[0].u, u0);   // EDB4
    ppc::store_single(&full[0].v, v0);   // EDB8
    ppc::store_single(&full[1].x, zero); // EDBC
    ppc::store_single(&full[1].y, ppc::signed_word_to_single(height_word, signed_bias)); // EDD0/EDD4
    ppc::store_single(&full[1].u, u0);   // EDD8
    ppc::store_single(&full[1].v, v1);   // EDDC
    ppc::store_single(&full[2].x, ppc::signed_word_to_single(width_word, signed_bias)); // EDF0/EDF4
    ppc::store_single(&full[2].y, zero); // EDF8
    ppc::store_single(&full[2].u, u1);   // EDFC
    ppc::store_single(&full[2].v, v0);   // EE00
    ppc::store_single(&full[3].x, ppc::signed_word_to_single(width_word, signed_bias)); // EE14/EE18
    ppc::store_single(&full[3].y, ppc::signed_word_to_single(height_word, signed_bias)); // EE2C/EE30
    ppc::store_single(&full[3].u, u1);   // EE34
    ppc::store_single(&full[3].v, v1);   // EE38

    // 0x8042EE3C..0x8042EE58: srawi 1; addze = SIGNED /2, truncating toward 0.
    // C++17 signed division by positive 2 has exactly that integer rule, including
    // negative odd inputs. A bare arithmetic right shift would give wrong sizes.
    rectangle.width = rectangle.width / 2;
    rectangle.height = rectangle.height / 2;
    Im2DVertex* reduced = state->half_size_vertices;
    copy_vertex_storage(reduced[0], full[0]); // EE5C..EEA8
    copy_vertex_storage(reduced[1], full[1]); // EEAC..EEF0
    copy_vertex_storage(reduced[2], full[2]); // EEF4..EF38
    copy_vertex_storage(reduced[3], full[3]); // EF3C..EF80

    // 0x8042EF84..0x8042F008: change only xy. Keep copied full-size UVs/z/RGBA.
    const u32 half_width_word = static_cast<u32>(rectangle.width);
    const u32 half_height_word = static_cast<u32>(rectangle.height);
    ppc::store_single(&reduced[0].x, zero);
    ppc::store_single(&reduced[0].y, zero);
    ppc::store_single(&reduced[1].x, zero);
    ppc::store_single(&reduced[1].y, ppc::signed_word_to_single(half_height_word, signed_bias)); // EFA0/EFA4
    ppc::store_single(&reduced[2].x, ppc::signed_word_to_single(half_width_word, signed_bias)); // EFB8/EFBC
    ppc::store_single(&reduced[2].y, zero);
    ppc::store_single(&reduced[3].x, ppc::signed_word_to_single(half_width_word, signed_bias)); // EFEC/EFF0
    ppc::store_single(&reduced[3].y, ppc::signed_word_to_single(half_height_word, signed_bias)); // F004/F008

    // 0x8042F00C..0x8042F034: distinct VIEW and BACKING allocation, not ping-pong.
    // No allocation guards, cleanup-on-failure, or assignment of SubRaster's
    // return is present in the original. Preserve those failure preconditions.
    state->history_view = original::RwRaster_Create_8048AEC4(0, 0, 0, 0x505); // F00C/F010
    RasterPrefix* backing = original::RwRaster_Create_8048AEC4(
        rectangle.width, rectangle.height, 0, 0x505); // F024
    (void)original::RwRaster_SubRaster_8048AE14(state->history_view, backing, &rectangle); // F034

    if (parameters != nullptr) { // F038/F03C
        (void)MotionBlur_AssignParametersIfPresent_8042EA1C(&state->parameters, parameters); // F048
    } else {
        state->parameters.enabled = 0; // F058
        state->parameters.alpha = 0x80; // F064; different from failed/absent-state defaults
        ppc::store_single(&state->parameters.geometry_scale,
                          ppc::load_single(&constants::neutral_8051C38C)); // F060/F068
        // +8/+C, +2/+3 and +79..7B remain UNTOUCHED, not invented zero defaults.
    }
    return state; // F06C
}

/*
 * Original [0x8042F084,0x8042F0E8), size 0x64.
 * Evidence: original/8042F084_MotionBlur_GetParameters.asm:5-29.
 * Callers: 0x80204720,0x802055E8,0x80206320,0x8043A908.
 * Callee: 0x8042E9C4. Global: neutral float 0x8051C38C.
 * Confidence: high for stores/indeterminate source identification; actual bytes
 *   from original SP+0x10/+0x14 cannot be recovered statically. They are NOT 0.
 * ABI boundary: explicit output r3/manager r4. No return-register fiction.
 */
void MotionBlur_GetParameters_8042F084(BlurParameters* output,
                                     const EffectManagerPrefix* manager) {
    BlurState* state = manager->blur; // F098; manager itself is NOT null-checked
    if (state != nullptr) {
        (void)MotionBlur_CopyParameters_8042E9C4(output, &state->parameters); // F0A4
        return;
    }

    // UNRESOLVED ORIGINAL STACK RESIDUE, deliberately without initializers.
    // The original reserves 0x20 stack bytes and reads unwritten +0x10/+0x14.
    // These raw local storage cells express indeterminacy without a C++ typed
    // uninitialized read; they do NOT promise the original stack addresses/bits.
    // A bit-exact replay needs the two actual captured original stack words.
    RawWord32 unresolved_original_sp_10;
    RawWord32 unresolved_original_sp_14;
    output->enabled = 0; // F0B4
    const Fpr neutral = ppc::load_single(&constants::neutral_8051C38C); // F0B8
    output->alpha = 0;   // F0BC (NOT the successful ctor's 0x80)
    const u32 auxiliary_08 = ppc::load_word(&unresolved_original_sp_10); // F0C0
    ppc::store_single(&output->geometry_scale, neutral);                // F0C4
    const u32 auxiliary_0c = ppc::load_word(&unresolved_original_sp_14); // F0C8
    ppc::store_word(&output->auxiliary_08, auxiliary_08);                // F0CC
    ppc::store_word(&output->auxiliary_0c, auxiliary_0c);                // F0D0
    // No writes to padding +2/+3; no deterministic auxiliary defaults.
}

/*
 * Original [0x8042F0E8,0x8042F114), size 0x2C.
 * Evidence: original/8042F0E8_MotionBlur_SetParameters.asm:5-15.
 * Callers: 0x8020475C,0x80205624,0x8020635C,0x80439FD8,0x8043A544.
 * Callee: 0x8042E9F0. Globals: none. Confidence: high.
 */
void MotionBlur_SetParameters_8042F0E8(EffectManagerPrefix* manager,
                                     const BlurParameters* parameters) {
    BlurState* state = manager->blur; // F0F4
    if (state != nullptr)
        (void)MotionBlur_AssignParameters_8042E9F0(&state->parameters, parameters); // F100
    // A nonnull state with null parameters still dereferences the source.
}

/*
 * Original [0x8042F114,0x8042F140), size 0x2C.
 * Evidence: original/8042F114_MotionBlur_SetParametersIfPresent.asm:5-15.
 * Callers: 0x800A356C,0x800A375C,0x800A481C.
 * Callee: 0x8042EA1C. Globals: none. Confidence: high.
 */
void MotionBlur_SetParametersIfPresent_8042F114(EffectManagerPrefix* manager,
                                              const BlurParameters* parameters) {
    BlurState* state = manager->blur; // F120: manager is not null-checked
    if (state != nullptr)
        (void)MotionBlur_AssignParametersIfPresent_8042EA1C(&state->parameters, parameters); // F12C
}

/*
 * Original [0x8042F140,0x8042F16C), size 0x2C.
 * Evidence: original/8042F140_MotionBlur_ManagerSaveScreen.asm:5-15.
 * Caller: 0x801D2774. Callee: 0x8042E838. Globals: none. Confidence: high.
 */
void MotionBlur_ManagerSaveScreen_8042F140(EffectManagerPrefix* manager) {
    BlurState* state = manager->blur; // F14C
    if (state != nullptr)
        MotionBlur_SaveScreen_8042E838(state); // F158
}

/*
 * Original [0x8042F16C,0x8042F198), size 0x2C.
 * Evidence: original/8042F16C_MotionBlur_ManagerDraw.asm:5-15.
 * Caller: 0x801D280C. Callee: 0x8042E254. Globals: none. Confidence: high.
 */
void MotionBlur_ManagerDraw_8042F16C(EffectManagerPrefix* manager) {
    BlurState* state = manager->blur; // F178
    if (state != nullptr)
        MotionBlur_Draw_8042E254(state); // F184
}

/*
 * Original [0x8042F198,0x8042F20C), size 0x74.
 * Evidence: original/8042F198_MotionBlur_Destroy.asm:5-33.
 * Caller: 0x801D19B8. Callees: 0x8048AC2C (root then view),0x803A1334.
 * Globals: none directly; allocator/raster subsystem are adapter boundaries.
 * Confidence: high for ordering, alias test and null behavior. If constructor
 *   returned before +74 initialization, its residual pointer is NOT repaired here.
 */
void MotionBlur_Destroy_8042F198(EffectManagerPrefix* manager) {
    BlurState* state = manager->blur; // F1AC
    if (state == nullptr)
        return; // F1B4; F1B8 repeats the same CR test on the nonnull path
    RasterPrefix* view = state->history_view; // F1BC
    if (view != nullptr) {
        RasterPrefix* parent = view->parent; // F1C8
        if (parent != view)
            (void)original::RwRaster_Destroy_8048AC2C(parent); // F1D4; NO parent-null test
        // Original reloads the member AFTER the first external destruction.
        (void)original::RwRaster_Destroy_8048AC2C(state->history_view); // F1D8/F1DC
        state->history_view = nullptr; // F1E4
    }
    original::Deallocate_803A1334(state); // F1EC
    manager->blur = nullptr;             // F1F4, only on the original nonnull path
}

/*
 * Original [0x8042F20C,0x8042F258), size 0x4C.
 * Evidence: original/8042F20C_MotionBlur_Create.asm:5-23.
 * Caller: 0x801D1A80. Callees: 0x803A1380,0x8042EA50. Globals: none directly.
 * Confidence: high. There is no existing-state test/free and no whole-object zero.
 *   A partially constructed NONNULL allocation is still installed at manager+40.
 */
void MotionBlur_Create_8042F20C(EffectManagerPrefix* manager,
                              const BlurParameters* parameters) {
    auto* state = static_cast<BlurState*>(original::Allocate_803A1380(0x13C)); // F228
    if (state != nullptr)
        (void)MotionBlur_Construct_8042EA50(state, manager, parameters); // F23C
    manager->blur = state; // F240 uses the allocation pointer, not a new success flag
}

/*
 * Original [0x801D2058,0x801D20D8), size 0x80.
 * Evidence: original/801D2058_DrawBlurTask_Construct.asm:5-36.
 * Caller: 0x801D1AF0. Callees: 0x800461A0,0x8004F014,0x803A1380,0x801D20D8.
 * Globals/data: name 0x804D0F18="DrawBlur"; vtable 0x8053DC78; scheduler via
 *   0x800461A0 (its transitive storage/creation is an external boundary).
 * Confidence: high for fresh body/name/vtable bytes; INTEGRATION UPDATE: full
 *   0x30 allocation size is now freshly verified by D[801D1AD0,801D1AFC), including
 *   li r3,0x30 at 1AE0 and BL 1AE4/1AF0 (not a new manifest export).
 */
DrawBlurTask* DrawBlurTask_Construct_801D2058(DrawBlurTask* task) {
    RenderLevelsPrefix* levels = original::RenderLevels_800461A0(); // 801D206C
    (void)original::Task_Construct_8004F014(&task->base, levels->draw_parent); // 801D207C
    task->base.vtable = reinterpret_cast<const void*>(0x8053DC78u); // 801D208C
    task->base.name = reinterpret_cast<const char*>(0x804D0F18u);    // 801D2098
    task->base.flags = static_cast<u16>(task->base.flags | 0x100u);  // 801D209C..20A4
    auto* child = static_cast<SaveScreenTask*>(original::Allocate_803A1380(0x28)); // 20A8
    if (child != nullptr)
        child = SaveScreenTask_Construct_801D20D8(child); // 20B4: use constructor return
    task->save_task = child; // 20BC; +2C..2F untouched
    return task;            // 20C0
}

/*
 * Original [0x801D20D8,0x801D213C), size 0x64.
 * Evidence: original/801D20D8_SaveScreenTask_Construct.asm:5-29.
 * Caller: 0x801D20B4. Callees: 0x800461A0,0x8004F014.
 * Globals/data: name 0x804D0F24="SaveScreen"; vtable 0x8053DC5C; scheduler
 *   via the shared getter. Confidence: high for fresh instructions and data.
 */
SaveScreenTask* SaveScreenTask_Construct_801D20D8(SaveScreenTask* task) {
    RenderLevelsPrefix* levels = original::RenderLevels_800461A0(); // 801D20EC
    (void)original::Task_Construct_8004F014(&task->base, levels->save_parent); // 801D20FC
    task->base.vtable = reinterpret_cast<const void*>(0x8053DC5Cu); // 801D210C
    task->base.name = reinterpret_cast<const char*>(0x804D0F24u);    // 801D2118
    task->base.flags = static_cast<u16>(task->base.flags | 0x100u);  // 801D211C..2124
    return task;
}

/*
 * Original [0x801D270C,0x801D2764), size 0x58.
 * Evidence: original/801D270C_SaveScreenTask_Destroy.asm:5-26.
 * Callers: no direct BL found; vtable cell 0x8053DC64 contains this entry.
 * Callees: 0x8004EF18 with r4=0;0x803A1334 if extsh(r4)>0.
 * Globals/data: vtable 0x8053DC5C. Confidence: high; shared task behavior external.
 */
SaveScreenTask* SaveScreenTask_Destroy_801D270C(SaveScreenTask* task, s32 delete_flag) {
    if (task != nullptr) { // 801D271C..2724: this null guard IS original
        task->base.vtable = reinterpret_cast<const void*>(0x8053DC5Cu); // 2734
        (void)original::Task_Destroy_8004EF18(&task->base, 0); // 2738
        if (ppc::sign_extend_low16(static_cast<u32>(delete_flag)) > 0) // 273C/2740
            original::Deallocate_803A1334(task); // 2748
    }
    return task; // 274C, even on the delete path; not a live-object guarantee
}

/*
 * Original [0x801D2764,0x801D2788), size 0x24.
 * Evidence: original/801D2764_SaveScreenTask_Exec.asm:5-13.
 * Callers: no direct BL found; vtable cell 0x8053DC68 contains this entry.
 * Callees: 0x800A35D8,0x8042F140. Globals: 0x805EF2D8 via lazy manager getter.
 * Confidence: high for fresh code/vtable; execution/frame ordering not assumed.
 */
void SaveScreenTask_Exec_801D2764(SaveScreenTask* task) {
    (void)task; // incoming this is not read by the original body
    EffectManagerPrefix* manager = original::EffectManager_800A35D8(); // 801D2770
    MotionBlur_ManagerSaveScreen_8042F140(manager);                    // 801D2774
}

/*
 * Original [0x801D2788,0x801D27FC), size 0x74.
 * Evidence: original/801D2788_DrawBlurTask_Destroy.asm:5-33.
 * Callers: no direct BL found; vtable cell 0x8053DC80 contains this entry.
 * Callees: 0x8004EF18 with r4=0;0x803A1334 if extsh(r4)>0.
 * Globals/data: vtable 0x8053DC78. Confidence: high for fresh code/data. Setting
 *   the child's disposal bit does NOT immediately destroy/free it or blur history.
 */
DrawBlurTask* DrawBlurTask_Destroy_801D2788(DrawBlurTask* task, s32 delete_flag) {
    if (task != nullptr) { // 801D2798..27A0
        task->base.vtable = reinterpret_cast<const void*>(0x8053DC78u); // 27AC
        SaveScreenTask* child = task->save_task; // 27B0
        if (child != nullptr)
            child->base.flags = static_cast<u16>(child->base.flags | 1u); // 27BC..27C4
        (void)original::Task_Destroy_8004EF18(&task->base, 0); // 27D0
        if (ppc::sign_extend_low16(static_cast<u32>(delete_flag)) > 0) // 27D4/27D8
            original::Deallocate_803A1334(task); // 27E0
    }
    return task; // 27E4; child pointer/tail are not cleared
}

/*
 * Original [0x801D27FC,0x801D2820), size 0x24.
 * Evidence: original/801D27FC_DrawBlurTask_Exec.asm:5-13.
 * Callers: no direct BL found; vtable cell 0x8053DC84 contains this entry.
 * Callees: 0x800A35D8,0x8042F16C. Globals: 0x805EF2D8 via lazy manager getter.
 * Confidence: high for fresh code/vtable; no renderer or manager null guard added.
 */
void DrawBlurTask_Exec_801D27FC(DrawBlurTask* task) {
    (void)task;
    EffectManagerPrefix* manager = original::EffectManager_800A35D8(); // 801D2808
    MotionBlur_ManagerDraw_8042F16C(manager);                          // 801D280C
}

} // namespace motion_blur

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

// INPUT INTEGRATION. The backend fragment above is appended verbatim; its old
// two-include example describes pre-integration use, NOT an instruction to
// include it a second time. Neither input nor backend selected paths are
// installed as generic callbacks. See ../evidence/source_integration.md.
// Authoritative input conditions: ../evidence/12_closure.md (NOT the stale
// unordered upper-clamp claim in 11_decompiler.md:494). Exact VAs below are
// local main.dol addresses; ranges are half-open. Selected functions explicitly
// return a continuation annotation rather than pretend an omitted arm is done.

namespace motion_blur {
namespace ppc {
// NON-ORIGINAL instruction notation, like the core helpers. FCMPO may set UN;
// "ordered" names the instruction, not a promise that operands are finite.
// mfcr puts CR0.LT/GT/EQ/UN at bits 31/30/29/28. Capture it in the SAME asm so
// compiler-generated compares cannot replace the original floating CR result.
// If an enabled FP exception traps, these expressions do not promise to return.
struct OrderedComparison {
    u32 cr0;
    bool lt() const { return (cr0 & 8u) != 0; }
    bool gt() const { return (cr0 & 4u) != 0; }
    bool eq() const { return (cr0 & 2u) != 0; }
    bool un() const { return (cr0 & 1u) != 0; }
};
inline OrderedComparison compare_ordered(Fpr left, Fpr right) {
    u32 cr;
    __asm__ volatile("fcmpo 0,%1,%2\n\tmfcr %0"
                     : "=r"(cr) : "f"(left), "f"(right) : "cr0", "memory");
    return {(cr >> 28) & 0xFu};
}
inline u32 fctiwz_low_word(Fpr input) {
    // PPC fctiwz -> stfd -> lwz of the second big-endian word, NOT a host
    // float-to-int cast. Invalid/out-of-range and NaN behavior belongs to the
    // actual PPC/FPSCR. No saturation, invented indefinite value, or mask of
    // the floating representation is substituted. Only the low word is used.
    alignas(8) RawWord32 integer_image[2];
    Fpr encoded_integer;
    __asm__ volatile("fctiwz %0,%2\n\tstfd %0,0(%1)"
                     : "=&f"(encoded_integer)
                     : "b"(integer_image), "f"(input) : "memory");
    return load_word(&integer_image[1]);
}
inline Fpr multiply_add_single(Fpr multiplicand, Fpr multiplier, Fpr addend) {
    // One actual FMADDS (fA*fC+fB), with its single-precision rounding; the
    // already-rounded FMULS saved product is supplied separately by the caller.
    Fpr value;
    __asm__ volatile("fmadds %0,%1,%2,%3" : "=f"(value)
                     : "f"(multiplicand), "f"(multiplier), "f"(addend) : "memory");
    return value;
}
} // namespace ppc

namespace input {
using ppc::Fpr;
namespace raw {
// Exact binary representations, read with lfs/lfd, not host decimal guesses.
// Evidence: 12_closure.md:12-24; data_manifest.json:670-706,839-850.
// These are snapshots of read-only literal bytes, NOT new mutable game globals.
inline constexpr u32 ramp_one_805F3BCC = 0x3F800000u;
inline constexpr u32 fixed_scale_805F3BD0 = 0x3F866666u; // 1.0499999523162842
inline constexpr u32 ramp_divisor_805F3BD4 = 0x3F000000u; // 0.5
inline constexpr u32 ramp_zero_805F3BDC = 0x00000000u;
inline constexpr u32 ramp_alpha_805F3BE0 = 0x42800000u; // 64, NOT fixed preset's 128
inline constexpr u32 progress_epsilon_8051BFA4 = 0x38D1B717u;
inline constexpr u32 progress_one_8051BFA8 = 0x3F800000u;
inline constexpr u32 interpolation_snap_8051CB58 = 0x3F7FF972u; // ~0.9999, NOT 1
inline constexpr u32 interpolation_one_8051CB5C = 0x3F800000u;
alignas(8) inline constexpr u64 alpha_unsigned_bias_8051CB60 = 0x4330000000000000ULL;
} // namespace raw

/*
 * SELECTED entry-prefix of original [0x800A3528,0x800A35D8), size 0xB0;
 * represented [0x800A3528,0x800A3570), NOT a complete standalone blur entry.
 * Evidence: original/800A3528_MotionBlur_DisableFixedParameters.asm:5-22.
 * Caller: wrapper BL 0x800A2E28, with r3=ptr32[wrapper+28]. Immediate wrapper
 *   caller 8007C15C chooses fixed disable iff N+20 bit3 is clear; the other arm
 *   instead calls 800A2DF0 -> 800A34C8 to create a new ramp (12_closure.md:48-50).
 * Callees in prefix: 800A35D8 at 3564, 8042F114 at 356C. Global manager cache
 *   805EF2D8 via getter; literals 805F3BCC/3BD0. No self field gates the setter.
 * Confidence: high for prefix stores/call inputs; no inference about later
 *   effects or their side effects. Real continuation 3570..35D8 is NOT supplied.
 */
Continuation MotionBlur_DisableFixedPrefix_Selected_800A3528(void* controller_r3) {
    (void)controller_r3; // original saves it in r31 ONLY for the excluded tail
    BlurParameters parameters; // original SP+10, padding/auxiliary bytes unwritten
    const Fpr neutral = ppc::load_single(&raw::ramp_one_805F3BCC); // 3530
    const Fpr scale = ppc::load_single(&raw::fixed_scale_805F3BD0); // 3540
    parameters.enabled = 0;                      // 354C, initial local default
    parameters.alpha = 0;                        // 3550
    ppc::store_single(&parameters.geometry_scale, neutral); // 3554
    parameters.enabled = 0;                      // 3558, fixed disable preset
    parameters.alpha = 0x80;                     // 355C (NOT retained current alpha)
    ppc::store_single(&parameters.geometry_scale, scale);   // 3560
    EffectManagerPrefix* manager = original::EffectManager_800A35D8(); // 3564
    MotionBlur_SetParametersIfPresent_8042F114(manager, &parameters);  // 356C
    // Aux +8/+C = original unwritten SP+18/+1C, copied as raw words by the core.
    // A local RawWord32 model preserves indeterminacy, NOT original stack bits.
    return {0x800A3570u}; // non-blur handle/effect work and 801D3BD8(1) remain outside
}

/*
 * SELECTED entry-prefix of [0x800A3714,0x800A37A4), size 0x90;
 * represented [0x800A3714,0x800A3760). Evidence: matching original .asm:5-23.
 * Caller: 800A2E4C (wrapper 800A2E3C; +28 controller pointer, no wrapper guard).
 * Immediate enable request 8007BC8C: ((N+20 & 8)!=0 OR 800ABF30(N+24)==5),
 *   s32[N+1C]==1, then (N+20 & 4)!=0; passes ptr32[ptr32[N+4]+23C]. These are
 *   NUMERIC conditions, not asserted gameplay identities (12_closure.md:48-49).
 * Callees 800A35D8/8042F114 at 3754/375C; globals as disable. Confidence high
 *   for unconditional prefix and raw constants. Tail 3760..37A4 is excluded.
 */
Continuation MotionBlur_EnableFixedPrefix_Selected_800A3714(void* controller_r3) {
    (void)controller_r3;
    BlurParameters parameters; // SP+10; +2/+3, +8/+C deliberately NOT initialized
    const Fpr neutral = ppc::load_single(&raw::ramp_one_805F3BCC); // 371C
    const Fpr scale = ppc::load_single(&raw::fixed_scale_805F3BD0); // 372C
    parameters.enabled = 0;                      // 373C
    parameters.alpha = 0;                        // 3740
    ppc::store_single(&parameters.geometry_scale, neutral); // 3744
    parameters.enabled = 1;                      // 3748
    parameters.alpha = 0x80;                     // 374C
    ppc::store_single(&parameters.geometry_scale, scale);   // 3750
    EffectManagerPrefix* manager = original::EffectManager_800A35D8(); // 3754
    MotionBlur_SetParametersIfPresent_8042F114(manager, &parameters);  // 375C
    return {0x800A3760u}; // handle construction/assignment/release NOT faked here
}

/*
 * COMPLETE original [0x800A47B0,0x800A4830), size 0x80.
 * Evidence: original/800A47B0_MotionBlur_ApplyRamp.asm:5-36; 12_closure.md:55-58.
 * Caller: 800A48C4 with r3=T; NO strength passed in incoming f1. The original
 *   reloads f32[T+38] twice. Callees: 800A35D8 at 4814, 8042F114 at 481C.
 * Globals: manager cache 805EF2D8 via getter; literals 805F3BCC/3BD0/3BDC/3BE0.
 * Confidence: high for all inputs/transfers and CR bits; PPC/FPSCR/trap and
 *   unwritten stack bytes remain explicit machine-state dependencies.
 */
void MotionBlur_ApplyRamp_800A47B0(RampTaskPrefix* task_r3) {
    BlurParameters parameters; // original SP+8; auxiliary SP+10/+14 UNWRITTEN
    const Fpr neutral = ppc::load_single(&raw::ramp_one_805F3BCC); // 47B8
    const Fpr zero = ppc::load_single(&raw::ramp_zero_805F3BDC);   // 47C4
    parameters.enabled = 0; // 47C8
    parameters.alpha = 0;   // 47CC
    ppc::store_single(&parameters.geometry_scale, neutral); // 47D0
    const Fpr strength_for_gate = ppc::load_single(&task_r3->strength); // 47D4
    const auto gate = ppc::compare_ordered(strength_for_gate, zero);    // 47D8
    // 47DC cror EQ,LT,EQ; 47E0 bne: UN also chooses enabled=1 absent trapping.
    parameters.enabled = static_cast<u8>(!(gate.lt() || gate.eq()));  // 47EC
    const Fpr multiplier = ppc::load_single(&raw::ramp_alpha_805F3BE0); // 47F0
    const Fpr strength_for_alpha = ppc::load_single(&task_r3->strength); // 47F4
    const Fpr scale = ppc::load_single(&raw::fixed_scale_805F3BD0); // 47F8
    const Fpr scaled_alpha = ppc::mul_single(multiplier, strength_for_alpha); // 47FC
    ppc::store_single(&parameters.geometry_scale, scale); // 4800
    parameters.alpha = static_cast<u8>(ppc::fctiwz_low_word(scaled_alpha)); // 4804..4810
    EffectManagerPrefix* manager = original::EffectManager_800A35D8(); // 4814
    MotionBlur_SetParametersIfPresent_8042F114(manager, &parameters);  // 481C
    // Finite x in [0,1] gives alpha 0..64. There is no saturating byte cast;
    // 0<x<1/64 still enables with alpha=0, and the core's second gate suppresses it.
}

/*
 * SELECTED blur update prefix of [0x800A4830,0x800A492C), size 0xFC;
 * represented decisions/stores [0x800A4844,0x800A48A4). NOT the full task update.
 * Evidence: original/800A4830_MotionBlur_RampTaskUpdate.asm:5-33; 12_closure.md:52-63.
 * Caller: indirect task update, vptr 80522A78, cell 80522A84=800A4830 (+0xC).
 * Inputs: r3=T; actual delta in f1. Globals: 805F3BD4/3BDC/3BCC. No callees
 *   until the later submit block. No camera/velocity input enters this math.
 * Confidence: high for every single-rounding operation and LT/GT-only clamp.
 * Immediate producers of T+34, NOT whole actor reconstruction:
 *   800A34B0..34C8 sets ptr32[C+30]->34=1 if present. 800A34C8..3528 marks the
 *   old task likewise, allocates 0x40, and calls 800A4AB4 for a NEW task. That
 *   constructor writes T+34=0/T+38=0 at 4B08..4B18, NOT current blur alpha, and
 *   sets actor flag 0x15 via 4B24 -> 80076DDC. Actor ref's first word -> T+28.
 *   Both decrease callers 800AF68C/800AF988 use 800A2DCC -> 800A34B0. In
 *   [800AF60C,800AF6A4), actor=ptr32[self+28], flag0x15 is A8 mask 00200000;
 *   require it AND (f32(s32[8007993C()+70C])<=0 OR flag0x22 is CLEAR).
 *   Flag0x22 is AC mask 4 (indexing verified 8006CEA0..CEC4); counter conversion
 *   bias 805F3D20=4330000080000000, threshold 805F3D18=0. Message arm
 *   [800AF924,800AF998) requires u16[message+4]==0x106 and flag0x15; it requests
 *   decrease and ORs 2 into message+7. Delta values/units and scheduling unknown.
 */
Continuation MotionBlur_RampAdvance_Selected_800A4830(RampTaskPrefix* task_r3,
                                                    Fpr incoming_f1) {
    if (task_r3->decreasing != 0) { // 4844..484C
        const Fpr divisor = ppc::load_single(&raw::ramp_divisor_805F3BD4); // 4850
        const Fpr old_strength = ppc::load_single(&task_r3->strength);   // 4854
        const Fpr step = ppc::div_single(incoming_f1, divisor);          // 4858
        const Fpr zero = ppc::load_single(&raw::ramp_zero_805F3BDC);     // 485C
        ppc::store_single(&task_r3->strength, ppc::sub_single(old_strength, step)); // 4860/4864
        const Fpr stored_strength = ppc::load_single(&task_r3->strength); // 4868
        if (ppc::compare_ordered(stored_strength, zero).lt())            // 486C/4870
            ppc::store_single(&task_r3->strength, zero);                // 4874
    } else { // 487C: no low clamp on this direction
        const Fpr divisor = ppc::load_single(&raw::ramp_divisor_805F3BD4); // 487C
        const Fpr old_strength = ppc::load_single(&task_r3->strength);   // 4880
        const Fpr step = ppc::div_single(incoming_f1, divisor);          // 4884
        const Fpr one = ppc::load_single(&raw::ramp_one_805F3BCC);       // 4888
        ppc::store_single(&task_r3->strength, ppc::add_single(old_strength, step)); // 488C/4890
        const Fpr stored_strength = ppc::load_single(&task_r3->strength); // 4894
        if (ppc::compare_ordered(stored_strength, one).gt())             // 4898/489C
            ppc::store_single(&task_r3->strength, one);                 // 48A0
    }
    // 489C raw 40810008 is branch when GT is CLEAR, so UN SKIPS the clamp.
    // UN also skips the lower clamp. Never use !(x<=1) or a host min/max here.
    return {0x800A48A4u}; // required non-blur global writes 48A4..48C4 remain outside
}

// SELECTED call instruction in the SAME [800A4830,800A492C) container, not an
// original entry at 48C4. Preceding 48A4..48C4 multiplies T+38 by another literal
// and writes 8058E760+30/+24; those non-blur writes are NOT silently executed or
// erased by this model. Live r3=T was set at 48B4. Callee/globals: ApplyRamp above.
// Evidence: original/800A4830_MotionBlur_RampTaskUpdate.asm:34-42. Confidence high.
Continuation MotionBlur_RampSubmit_Selected_800A48C4(RampTaskPrefix* task_r3) {
    MotionBlur_ApplyRamp_800A47B0(task_r3); // the actual BL 800A48C4
    return {0x800A48C8u}; // 8007993C and 8016F4B0 side-effect work must precede retirement
}

// SELECTED [800A48F8,800A4918) in that same update. Evidence .asm:55-62.
// Inputs: live r31=T and POST-callback T+38; no delta/old cached strength input.
// Global: 805F3BDC. No callees. Confidence high for ordered retirement flag;
// callback effects and actual task destruction timing are not reconstructed.
Continuation MotionBlur_RampRetirement_Selected_800A48F8(RampTaskPrefix* task_r31) {
    const Fpr current_strength = ppc::load_single(&task_r31->strength); // 48F8
    const Fpr zero = ppc::load_single(&raw::ramp_zero_805F3BDC);         // 48FC
    const auto comparison = ppc::compare_ordered(current_strength, zero); // 4900
    if (comparison.lt() || comparison.eq()) // 4904 cror; 4908 bne skips UN
        task_r31->task.flags = static_cast<u16>(task_r31->task.flags | 1u); // 490C..4914
    return {0x800A4918u}; // original epilogue, not an immediate destroy/free
}

/*
 * COMPLETE original [0x8042B63C,0x8042B6CC), size 0x90.
 * Evidence: original/8042B63C_EffectParameter_UpdateProgress.asm:5-40;
 *   12_closure.md:85-99. Blur-containing caller BL 80439AA0; other parameter
 *   callers share this helper but their unrelated effects are not implemented.
 * Inputs r3=E/f1=delta; E+C age, +18 flags, +100 weight, +104 duration.
 * No callees. Globals 8051BFA4 EPS=38D1B717, 8051BFA8 one=3F800000.
 * Confidence high for flag/branch semantics, including unordered outcomes
 *   assuming comparison execution returns normally. No duration unit guessed.
 */
void EffectParameter_UpdateProgress_8042B63C(EffectParameterPrefix* effect_r3,
                                           Fpr incoming_f1) {
    const u32 flags = ppc::load_word(&effect_r3->execution_flags); // B63C
    if ((flags & 1u) == 0) // B640/B644
        return;
    if ((flags & 4u) != 0) // B648/B64C
        return;
    if ((flags & 2u) != 0) { // B650/B654: freezes age ONLY, not weight recomputation
        const Fpr old_age = ppc::load_single(&effect_r3->age); // B658
        ppc::store_single(&effect_r3->age, ppc::add_single(old_age, incoming_f1)); // B65C/B660
    }
    const Fpr duration = ppc::load_single(&effect_r3->duration);           // B668
    const Fpr epsilon = ppc::load_single(&raw::progress_epsilon_8051BFA4); // B66C
    const Fpr age = ppc::load_single(&effect_r3->age);                     // B670
    const Fpr expiration_age = ppc::add_single(epsilon, duration);        // B674
    if (ppc::compare_ordered(age, expiration_age).gt()) { // B678/B67C: STRICT GT
        const u32 current_flags = ppc::load_word(&effect_r3->execution_flags); // B680
        ppc::store_word(&effect_r3->execution_flags, current_flags & ~1u); // B684/B688
        return; // B68C: old weight is retained, NOT set to 1; no setter here
    }
    if (ppc::compare_ordered(duration, epsilon).gt()) { // B690/B694
        ppc::store_single(&effect_r3->weight, ppc::div_single(age, duration)); // B698/B69C
    } else { // duration <= EPS OR UN => fallback 1, not a division
        ppc::store_single(&effect_r3->weight, ppc::load_single(&raw::progress_one_8051BFA8)); // B6A8/B6AC
    }
    const Fpr stored_weight = ppc::load_single(&effect_r3->weight);       // B6B4
    const Fpr one = ppc::load_single(&raw::progress_one_8051BFA8);        // B6B8
    if (ppc::compare_ordered(stored_weight, one).gt())                    // B6BC/B6C0
        ppc::store_single(&effect_r3->weight, one);                     // B6C4
    // No lower clamp; an unordered quotient survives the upper clamp.
}

namespace detail {
// NON-ORIGINAL source factoring, not another call in the DOL. Actual cast BLs:
// 8043A768,80439AD4,8043A4F8. All use (r3=E+1C pointer,r4=0,r5=8056C094,
// r6=8056C080,r7=0). These descriptors differ from the core camera-provider cast.
inline EffectManagerPrefix* cast_manager(EffectParameterPrefix* effect) {
    void* manager_base = reinterpret_cast<void*>(ppc::load_word(&effect->manager_base));
    return static_cast<EffectManagerPrefix*>(original::RuntimeCast_803A1AFC(
        manager_base, 0, reinterpret_cast<const void*>(0x8056C094u),
        reinterpret_cast<const void*>(0x8056C080u), 0));
}
inline BlurResourcePrefix* resource(EffectParameterPrefix* effect) {
    return reinterpret_cast<BlurResourcePrefix*>(ppc::load_word(&effect->resource));
}
inline s32 resource_kind(EffectParameterPrefix* effect) {
    // No resource-null guard is present in these original dispatches.
    return ppc::signed_word(ppc::load_word(&resource(effect)->kind));
}
} // namespace detail

/*
 * SELECTED constructor suffix, NOT an original standalone entry at A740/A900.
 * Actual container [0x8043A5A4,0x8043AB68), size 0x5C4. Covered common guard/cast/
 * dispatch [8043A740,8043A7A4) and kind1 snapshot/target [8043A900,8043A97C).
 * Evidence: 12_closure.md:77-83; freshly hash-checked D[8043A5A4,8043A7A4) and
 * D[8043A900,8043A97C). D means dol_evidence.py bounded read, NOT a new export.
 * Constructor callers: 80433900 (outer factory type8 allocates 0x1F4) and
 * 801FA8F8 (derived constructor). Type8 != resource kind1; dispatch reads R+0.
 * Earlier shared constructor 8042B734 at A5C0 and base 80426F8C establish
 * E+1C manager-base, E+C=0/E+18=1; active-path D+2C flags -> E+FC,
 * D+30 duration -> E+104, ptr32[D+34] -> E+108. Bit1 advance is NOT established.
 * Earlier blur-only defaults A660..A678 write saved/target enabled=alpha=0,
 * scale=1, leaving their aux words untouched. These and other-kind construction
 * work are upstream preconditions, NOT replayed/zero-filled by this suffix.
 * Callees: real RTTI 803A1AFC at A768, getter 8042F084 at A908. Globals: RTTI
 * 8056C094/8056C080; getter's 8051C38C absent-state literal. Live r30=E.
 * Confidence: high for guards, field offsets and every selected transfer;
 * original stack residue, concrete descriptors and mixed-constructor effects
 * remain outside. A kind other than 1 returns its unreconstructed arm PC.
 */
Continuation EffectGlobalParam_ConstructKind1_Selected_8043A740(
    GlobalParameterControllerPrefix* effect_r30) {
    if ((ppc::load_word(&effect_r30->common.execution_flags) & 1u) == 0) // A740..A748
        return {0x8043AB54u}; // A74C/A750: original r3=E, then epilogue
    // This constructor guard does NOT additionally test inhibit bit2.
    EffectManagerPrefix* manager = detail::cast_manager(&effect_r30->common); // A754..A768
    if (manager == nullptr) // A76C/A770
        return {0x8043AB50u};
    switch (detail::resource_kind(&effect_r30->common)) { // A774..A7A0, signed dispatch
    case 1: break;                         // -> A900: only implemented kind
    case 0: return {0x8043A7A4u};           // unrelated arm, NOT an invented no-op
    case 2: return {0x8043A97Cu};
    case 3: return {0x8043AA58u};
    default: return {0x8043AB50u};          // original default continuation
    }

    // A900 mr r4,r3; A904 r3=SP+1C: manager comes from the actual cast result.
    BlurParameters snapshot;
    MotionBlur_GetParameters_8042F084(&snapshot, manager); // A908
    effect_r30->saved.enabled = snapshot.enabled;         // A90C/A910
    effect_r30->saved.alpha = snapshot.alpha;             // A914/A918
    ppc::store_single(&effect_r30->saved.geometry_scale,
                      ppc::load_single(&snapshot.geometry_scale)); // A91C/A920
    const u32 saved_auxiliary_08 = ppc::load_word(&snapshot.auxiliary_08); // A924
    const u32 saved_auxiliary_0c = ppc::load_word(&snapshot.auxiliary_0c); // A928
    ppc::store_word(&effect_r30->saved.auxiliary_08, saved_auxiliary_08);  // A92C
    ppc::store_word(&effect_r30->saved.auxiliary_0c, saved_auxiliary_0c);  // A930

    const u32 source_flags = ppc::load_word(&effect_r30->common.source_flags); // A934
    BlurResourcePrefix* resource = detail::resource(&effect_r30->common);     // A938: RELOAD
    effect_r30->target.enabled = static_cast<u8>((source_flags & 0x00010000u) != 0); // A93C..A954
    effect_r30->target.alpha = resource->alpha; // A958/A95C: byte at R+8, NOT R+4
    ppc::store_single(&effect_r30->target.geometry_scale,
                      ppc::load_single(&resource->geometry_scale)); // A960/A964, R+C
    ppc::store_single(&effect_r30->target.auxiliary_08,
                      ppc::load_single(&resource->auxiliary_10));   // A968/A96C, R+10
    ppc::store_single(&effect_r30->target.auxiliary_0c,
                      ppc::load_single(&resource->auxiliary_14));   // A970/A974, R+14
    // Last two transfers really ARE lfs/stfs; unlike the snapshot's lwz/stw,
    // their interpretation here is floating-point, even though meaning is opaque.
    return {0x8043AB50u}; // actual branch at A978, not entry at A97C (kind2)
}

/*
 * SELECTED common preamble + blur kind1 of [0x80439A8C,0x8043A4B8), size 0xA2C;
 * NOT the generic Apply callback. Covered preamble [80439A8C,80439B10), selected
 * body [80439EBC,80439FE0), then original common epilogue at 8043A4A4.
 * Evidence: original/80439A8C_EffectGlobalParam_Apply.asm:5-37,273-345;
 * 12_closure.md:85-107. Caller is effect virtual dispatch, not a BL to interior
 * 9EBC; this source does not replace that vtable slot or infer scheduling.
 * Callees: common progress 8042B63C at 9AA0; 803A1AFC at 9AD4;
 * setter 8042F0E8 at 9FD8. Globals RTTI8056C094/8056C080, snap8051CB58,
 * one8051CB5C, unsigned-byte bias8051CB60; progress literals as above.
 * Confidence: high for guard ordering, fused interpolation, byte conversion,
 * saved/target identity and setter inputs. Other resource kinds explicitly
 * return their real arm PCs; no fog/actor/other-effect implementation is claimed.
 */
Continuation EffectGlobalParam_ApplyKind1_Selected_80439A8C(
    GlobalParameterControllerPrefix* effect_r3, Fpr incoming_f1) {
    EffectParameter_UpdateProgress_8042B63C(&effect_r3->common, incoming_f1); // 9AA0 FIRST
    const u32 flags = ppc::load_word(&effect_r3->common.execution_flags); // 9AA4, AFTER update
    if ((flags & 1u) == 0 || (flags & 4u) != 0) // 9AA8..9AB4
        return {0x8043A4A4u}; // expiry skips THIS setter; does not force target
    EffectManagerPrefix* manager = detail::cast_manager(&effect_r3->common); // 9AB8..9AD4
    if (manager == nullptr) // 9AD8/9ADC
        return {0x8043A4A4u};
    switch (detail::resource_kind(&effect_r3->common)) { // 9AE0..9B0C
    case 1: break;
    case 0: return {0x80439B10u};
    case 2: return {0x80439FE0u};
    case 3: return {0x8043A374u};
    default: return {0x8043A4A4u};
    }

    // Copy target to Q at original SP+14, never memset/copy the padding bytes.
    BlurParameters parameters;
    const u8 target_enabled = effect_r3->target.enabled; // 9EBC, live r4 for OR gate
    const Fpr snap = ppc::load_single(&raw::interpolation_snap_8051CB58); // 9EC4
    parameters.enabled = target_enabled;               // 9EC8
    parameters.alpha = effect_r3->target.alpha;         // 9ECC/9ED0
    ppc::store_single(&parameters.geometry_scale,
                      ppc::load_single(&effect_r3->target.geometry_scale)); // 9ED4/9ED8
    const u32 target_auxiliary_08 = ppc::load_word(&effect_r3->target.auxiliary_08); // 9EDC
    const u32 target_auxiliary_0c = ppc::load_word(&effect_r3->target.auxiliary_0c); // 9EE0
    ppc::store_word(&parameters.auxiliary_08, target_auxiliary_08); // 9EE4
    ppc::store_word(&parameters.auxiliary_0c, target_auxiliary_0c); // 9EE8
    const Fpr threshold_weight = ppc::load_single(&effect_r3->common.weight); // 9EEC
    if (ppc::compare_ordered(threshold_weight, snap).lt()) { // 9EF0/9EF4; UN skips interpolation
        // Short-circuit saved.enabled load exactly as 9EF8..9F1C. Disabling an
        // endpoint does NOT replace that endpoint's alpha with zero.
        parameters.enabled = static_cast<u8>(target_enabled != 0 || effect_r3->saved.enabled != 0);
        const u8 target_alpha = parameters.alpha;       // 9F20
        const u8 saved_alpha = effect_r3->saved.alpha;   // 9F24
        if (target_alpha != saved_alpha) { // 9F28/9F2C: equal alpha has NO conversion
            alignas(8) const u64 saved_image_bits = 0x4330000000000000ULL | saved_alpha;
            const Fpr weight = ppc::load_single(&effect_r3->common.weight); // 9F40 reload
            const Fpr one = ppc::load_single(&raw::interpolation_one_8051CB5C); // 9F48
            const Fpr bias = ppc::load_double(&raw::alpha_unsigned_bias_8051CB60); // 9F4C
            const Fpr saved_image = ppc::load_double(&saved_image_bits); // 9F50
            const Fpr complement = ppc::sub_single(one, weight);        // 9F54
            alignas(8) const u64 target_image_bits = 0x4330000000000000ULL | target_alpha;
            const Fpr saved_value = ppc::sub_single(saved_image, bias); // 9F5C, unsigned byte
            const Fpr target_image = ppc::load_double(&target_image_bits); // 9F64
            const Fpr saved_product = ppc::mul_single(saved_value, complement); // 9F68
            const Fpr target_value = ppc::sub_single(target_image, bias); // 9F6C
            const Fpr mixed_alpha = ppc::multiply_add_single(target_value, weight, saved_product); // 9F70
            parameters.alpha = static_cast<u8>(ppc::fctiwz_low_word(mixed_alpha)); // 9F74..9F80
        }

        // Original reloads weight and recomputes (1-t), even after alpha math.
        const Fpr weight = ppc::load_single(&effect_r3->common.weight); // 9F88
        const Fpr one = ppc::load_single(&raw::interpolation_one_8051CB5C); // 9F8C
        const Fpr saved_scale = ppc::load_single(&effect_r3->saved.geometry_scale); // 9F90
        const Fpr complement = ppc::sub_single(one, weight); // 9F94
        const Fpr target_scale = ppc::load_single(&parameters.geometry_scale); // 9F98
        const Fpr target_auxiliary_float_08 = ppc::load_single(&parameters.auxiliary_08); // 9F9C
        const Fpr target_auxiliary_float_0c = ppc::load_single(&parameters.auxiliary_0c); // 9FA0
        const Fpr scale_product = ppc::mul_single(saved_scale, complement); // 9FA4
        ppc::store_single(&parameters.geometry_scale,
                          ppc::multiply_add_single(target_scale, weight, scale_product)); // 9FA8/9FAC
        const Fpr saved_auxiliary_float_08 = ppc::load_single(&effect_r3->saved.auxiliary_08); // 9FB0
        const Fpr auxiliary_product_08 = ppc::mul_single(saved_auxiliary_float_08, complement); // 9FB4
        ppc::store_single(&parameters.auxiliary_08,
            ppc::multiply_add_single(target_auxiliary_float_08, weight, auxiliary_product_08)); // 9FB8/9FBC
        const Fpr saved_auxiliary_float_0c = ppc::load_single(&effect_r3->saved.auxiliary_0c); // 9FC0
        const Fpr auxiliary_product_0c = ppc::mul_single(saved_auxiliary_float_0c, complement); // 9FC4
        ppc::store_single(&parameters.auxiliary_0c,
            ppc::multiply_add_single(target_auxiliary_float_0c, weight, auxiliary_product_0c)); // 9FC8/9FCC
        // Do not rewrite any of these as saved+(target-saved)*t, and do not
        // clamp t or cast a nonfinite/out-of-range FPR with host C++ semantics.
    }
    // >= snap OR UN submits the exact copied target (including its enable byte).
    MotionBlur_SetParameters_8042F0E8(manager, &parameters); // 9FD0..9FD8, r3=M/r4=SP+14
    return {0x8043A4A4u}; // 9FDC: common epilogue, NOT kind2 at 9FE0
}

/*
 * SELECTED finalizer prefix/kind1 of [0x8043A4B8,0x8043A5A4), size 0xEC.
 * Covered null/vptr/cast/dispatch [A4B8,A534), kind1 [A540,A54C).
 * Evidence: original/8043A4B8_EffectGlobalParam_Destroy.asm:5-40;
 * 12_closure.md:108. Called as destruction/virtual lifecycle work, not as a
 * standalone function at A540; no timing relative to expiry is proved.
 * Callees in this slice: 803A1AFC at A4F8, 8042F0E8 at A544. Globals: vptr
 * 8056C0D4 and RTTI 8056C094/8056C080. Inputs r3=E/r4=delete flag saved in r31.
 * Confidence: high for target identity and ABSENCE of active/inhibit/weight
 * checks. Shared 8042B6CC destruction and conditional context-vtable+14 free
 * at A560..A58C are explicit continuations, NOT omitted successful no-ops.
 */
Continuation EffectGlobalParam_FinalizeKind1_Selected_8043A4B8(
    GlobalParameterControllerPrefix* effect_r3, s32 delete_flag_r4) {
    (void)delete_flag_r4; // needed by the original A56C extsh, outside this selected prefix
    if (effect_r3 == nullptr) // A4C8/A4D8
        return {0x8043A58Cu};
    effect_r3->common.original_vtable = reinterpret_cast<const void*>(0x8056C0D4u); // A4DC/A4E4
    EffectManagerPrefix* manager = detail::cast_manager(&effect_r3->common); // A4E0..A4F8
    if (manager == nullptr) // A4FC/A500
        return {0x8043A560u};
    switch (detail::resource_kind(&effect_r3->common)) { // A504..A530
    case 1: break;
    case 0: return {0x8043A534u};
    case 2: return {0x8043A54Cu};
    case 3: return {0x8043A558u};
    default: return {0x8043A560u};
    }
    // No E+18 active/inhibit check, no t check. It writes TARGET E+15C, not the
    // saved E+14C. Expiration alone does not call this or imply it has happened.
    MotionBlur_SetParameters_8042F0E8(manager, &effect_r3->target); // A540/A544
    return {0x8043A560u}; // A548: preserve the real shared destruction/free continuation
}

namespace detail {
// NON-ORIGINAL factoring of three identical inline instruction sequences:
// 80204724..4754,802055EC..561C,80206324..6354. Do not attribute a BL to this
// helper or use whole-object assignment (padding +2/+3 is never transferred).
inline void copy_then_disable(BlurParameters& destination, const BlurParameters& source) {
    const u8 enabled = source.enabled;  // first lbz into r3
    const u8 alpha = source.alpha;      // lbz into r5, before first stb
    destination.enabled = enabled;     // original transient copy, then overwritten below
    const Fpr scale = ppc::load_single(&source.geometry_scale);
    const u32 auxiliary_08 = ppc::load_word(&source.auxiliary_08);
    const u32 auxiliary_0c = ppc::load_word(&source.auxiliary_0c);
    destination.alpha = alpha;
    ppc::store_single(&destination.geometry_scale, scale);
    ppc::store_word(&destination.auxiliary_08, auxiliary_08);
    ppc::store_word(&destination.auxiliary_0c, auxiliary_0c);
    destination.enabled = 0; // ONLY this final field differs from the getter result
}
} // namespace detail

/*
 * SELECTED interior path [0x8020470C,0x80204760); official blur prefix block
 * [80204714,80204760) plus its immediately preceding manager side-effect call.
 * Containing original [0x802046BC,0x80204968), size 0x2AC, NOT an entry at 4714.
 * Evidence: 12_closure.md:65-75; fresh D[802046BC,80204768) verifies inputs/calls.
 * Enter ONLY on incoming self+4==0 and 80044E38()==1, AFTER earlier 8010A14C /
 * 801FAEF4 calls at 4704/4708. Those conditions belong to the containing dispatch,
 * not new checks to perform after callbacks. Later 4760..4968 remains outside.
 * Callees: 800A35D8 THREE times (470C/4714/4754), real 8042659C at 4710,
 * getter8042F084 at 4720, setter8042F0E8 at 475C. Global805EF2D8 via getter;
 * 8042659C's manager-entry walks/80426440/indirect effects are an external boundary.
 * Confidence: high for transfer and call order, NOT prefix preservation across
 * the earlier callback. Preserve alpha/scale/aux AS RETURNED AFTER 8042659C.
 */
Continuation MotionBlur_DisablePreservingPrefixA_Selected_8020470C() {
    original::EffectManager_VisitEntries_8042659C(original::EffectManager_800A35D8()); // 470C/4710
    BlurParameters snapshot; // original getter output SP+30
    EffectManagerPrefix* getter_manager = original::EffectManager_800A35D8(); // 4714, RELOAD
    MotionBlur_GetParameters_8042F084(&snapshot, getter_manager); // 4718..4720 (r3=out,r4=M)
    BlurParameters disabled; // separate SP+40 record, not the getter's own storage
    detail::copy_then_disable(disabled, snapshot); // 4724..4750: raw aux, no padding copy
    EffectManagerPrefix* setter_manager = original::EffectManager_800A35D8(); // 4754, RELOAD again
    MotionBlur_SetParameters_8042F0E8(setter_manager, &disabled); // 4758/475C
    return {0x80204760u};
}

/*
 * SELECTED interior [0x802055D4,0x80205628); blur prefix itself [55DC,5628).
 * Container [0x8020554C,0x80205E3C), size 0x8F0, NOT standalone at 55DC.
 * Evidence: 12_closure.md:65-75; fresh D[8020554C,80205630). Dispatch 5560..5580
 * reads self+1C, indexes 80544E74; ONLY entry0 -> 5584. Require self+1C==0,
 * 80044E38()==1, then store wrapping self+4 -= u32[8058E76C] at 55A0/55A4,
 * reload and require SIGNED self+4<=0 at 55A8..55B0. All 55B4..55D4 secondary
 * calls execute BEFORE this slice; do not recalculate those predicates later.
 * Callees: getter800A35D8 at 55D4/55DC/561C, 8042659C at 55D8, parameter getter
 * 8042F084 at 55E8, setter8042F0E8 at 5624. Globals manager cache plus enclosing
 * dispatch/tick data above. Confidence high for exact selected call/field order;
 * unrelated enclosing state-machine arms and side effects are not reconstructed.
 */
Continuation MotionBlur_DisablePreservingPrefixB_Selected_802055D4() {
    original::EffectManager_VisitEntries_8042659C(original::EffectManager_800A35D8()); // 55D4/55D8
    BlurParameters snapshot; // SP+20, NOT SP+30 as in A/C
    EffectManagerPrefix* getter_manager = original::EffectManager_800A35D8(); // 55DC
    MotionBlur_GetParameters_8042F084(&snapshot, getter_manager); // 55E0..55E8
    BlurParameters disabled; // SP+48
    detail::copy_then_disable(disabled, snapshot); // 55EC..5618
    EffectManagerPrefix* setter_manager = original::EffectManager_800A35D8(); // 561C
    MotionBlur_SetParameters_8042F0E8(setter_manager, &disabled); // 5620/5624
    return {0x80205628u}; // original non-blur continuation, not successful full state update
}

/*
 * SELECTED interior [0x8020630C,0x80206360); blur prefix itself [6314,6360).
 * Container [0x802062AC,0x80206534), size 0x288, NOT standalone at 6314.
 * Evidence: 12_closure.md:65-75; fresh D[802062AC,80206368). Incoming self+4==0
 * and 80044E38()==1 select this arm. Earlier calls 62F4..630C (8010A14C,
 * 801FAEF4,800C9F6C,800CBBC8,8006F87C,80074DDC) remain required outside;
 * those and the later arm tail are not replaced by this selected function.
 * Callees: 800A35D8 at 630C/6314/6354, 8042659C at 6310, 8042F084 at 6320,
 * 8042F0E8 at 635C. Global805EF2D8 via getter; callback transitive state unknown.
 * Confidence high for post-callback prefix-only disable and repeated lookups.
 */
Continuation MotionBlur_DisablePreservingPrefixC_Selected_8020630C() {
    original::EffectManager_VisitEntries_8042659C(original::EffectManager_800A35D8()); // 630C/6310
    BlurParameters snapshot; // SP+30
    EffectManagerPrefix* getter_manager = original::EffectManager_800A35D8(); // 6314
    MotionBlur_GetParameters_8042F084(&snapshot, getter_manager); // 6318..6320
    BlurParameters disabled; // SP+40
    detail::copy_then_disable(disabled, snapshot); // 6324..6350
    EffectManagerPrefix* setter_manager = original::EffectManager_800A35D8(); // 6354
    MotionBlur_SetParameters_8042F0E8(setter_manager, &disabled); // 6358/635C
    return {0x80206360u};
}

// No setter above allocates a blur, changes B+74/B+78, clears pixels, arbitrates
// producers or resets capture history. Original SaveScreen clears B+78 before
// its gates; constructor sets it only on the camera-success path. The input
// functions do not invent a temporal reset, a generic effect/actor fallback,
// or any linkage from these selected definitions into original runtime tables.
} // namespace input
} // namespace motion_blur

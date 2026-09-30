#pragma once

#include <array>
#include <cstdint>

namespace shadow::boot {

// Thirteen consecutive 12-byte entries at the eight PAL static table sites.
// Their C++ source type and call convention remain unresolved. The words are
// kept opaque; the three-word copy graph is proven by the original PPC bodies.
using ConstructorTriple = std::array<std::uint32_t, 3>;
using MotionTableWords = std::array<ConstructorTriple, 13>;

// Projects only the eight global-table entry copies performed by each bounded
// 58-word constructor body. Inputs are live pre-call words, including the
// shared source at 0x80514CB8..C0. This is not a connected boot transition or
// a model of the function's stack and register effects.
void ApplyMotionTableCopies(MotionTableWords& entries,
                            ConstructorTriple shared_source);

}  // namespace shadow::boot

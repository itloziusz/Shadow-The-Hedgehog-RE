#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace shadow::gc {

// PRS decompresser traced from main.dol 0x80043FE0.
// Flag bits are consumed from the low bit of each flag byte (srwi).
// A long-form offset word of 0 ends the stream and is not a copy.
// When exact_size is set, the output must be that many bytes. The directory
// field is the decompressed size (confirmed against every sampled ONE entry).
std::vector<std::uint8_t> prs_decompress(std::span<const std::uint8_t> src,
                                         std::optional<std::size_t> exact_size = std::nullopt);

}  // namespace shadow::gc

#include "shadow/gc/Prs.hpp"

#include "shadow/BinaryReader.hpp"

namespace shadow::gc {
namespace {

struct BitReader {
    std::span<const std::uint8_t> src;
    std::size_t cursor = 1;
    std::uint32_t flags = 0;
    int bits_left = 9;

    explicit BitReader(std::span<const std::uint8_t> input) : src(input) {
        if (src.empty()) {
            throw ParseError("PRS source is empty");
        }
        flags = src[0];
    }

    int next() {
        // 0x80043FF0: addic. bits_left, -1; reload the next flag byte only when it hits 0.
        --bits_left;
        if (bits_left == 0) {
            if (cursor >= src.size()) {
                throw ParseError("PRS flag byte runs past the source");
            }
            flags = src[cursor++];
            bits_left = 8;
        }
        const int bit = static_cast<int>(flags & 1u);
        flags >>= 1;
        return bit;
    }

    std::uint8_t byte() {
        if (cursor >= src.size()) {
            throw ParseError("PRS literal runs past the source");
        }
        return src[cursor++];
    }
};

}  // namespace

std::vector<std::uint8_t> prs_decompress(std::span<const std::uint8_t> src,
                                         std::optional<std::size_t> exact_size) {
    BitReader bits(src);
    std::vector<std::uint8_t> out;
    if (exact_size) {
        out.reserve(*exact_size);
    }

    auto push = [&](std::uint8_t value) {
        if (exact_size && out.size() >= *exact_size) {
            throw ParseError("PRS output exceeds the directory size");
        }
        out.push_back(value);
    };

    for (;;) {
        const int literal_bit = bits.next();
        if (literal_bit == 1) {
            push(bits.byte());
            continue;
        }

        const int long_form = bits.next();
        std::size_t length = 0;
        std::int32_t offset = 0;
        if (long_form == 1) {
            const std::uint8_t low = bits.byte();
            const std::uint8_t high = bits.byte();
            const std::uint32_t pair = static_cast<std::uint32_t>(low) | (static_cast<std::uint32_t>(high) << 8);
            if (pair == 0) {
                if (exact_size && out.size() != *exact_size) {
                    throw ParseError("PRS size does not match the directory");
                }
                return out;
            }
            // 0x80044070: length is the low 3 bits; offset is pair >> 3, then OR -8192.
            const std::uint32_t low_length = low & 7u;
            offset = static_cast<std::int32_t>((pair >> 3) | 0xFFFFE000u);
            if (low_length == 0) {
                length = static_cast<std::size_t>(bits.byte()) + 1u;
            } else {
                length = static_cast<std::size_t>(low_length) + 2u;
            }
        } else {
            length = 0;
            for (int i = 0; i < 2; ++i) {
                length = (length << 1) | static_cast<std::size_t>(bits.next());
            }
            const auto offb = bits.byte();
            length += 2;
            offset = static_cast<std::int32_t>(0xFFFFFF00u | offb);
        }

        if (offset >= 0) {
            throw ParseError("PRS copy offset is not negative");
        }
        const auto distance = static_cast<std::size_t>(-static_cast<std::int64_t>(offset));
        if (distance == 0 || distance > out.size()) {
            throw ParseError("PRS copy looks behind the output");
        }
        std::size_t from = out.size() - distance;
        for (std::size_t n = 0; n < length; ++n) {
            push(out[from++]);
        }
    }
}

}  // namespace shadow::gc

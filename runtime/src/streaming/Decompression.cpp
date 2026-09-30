#include "shadowpc/streaming/Decompression.hpp"
#include <cstdint>
#include <stdexcept>

namespace shadowpc {

static std::vector<std::byte> prs_decode(std::span<const std::byte> input, std::size_t expected) {
    const auto* data=reinterpret_cast<const std::uint8_t*>(input.data());
    const std::size_t size=input.size();
    if (!size) return {};
    std::size_t src=0; std::vector<std::byte> out;
    if (expected) out.reserve(expected);
    int bit_pos=9; std::uint8_t current=data[src++];
    auto bit=[&]() -> int {
        --bit_pos;
        if (bit_pos==0) {
            if (src>=size) throw std::runtime_error("PRS control stream ended");
            current=data[src++]; bit_pos=8;
        }
        int flag=current & 1; current >>= 1; return flag;
    };
    for (;;) {
        if (bit()) {
            if (src>=size) throw std::runtime_error("PRS literal missing");
            out.push_back(std::byte(data[src++]));
            continue;
        }
        int length=0; int offset=0;
        if (bit()) {
            if (src+2>size) throw std::runtime_error("PRS long copy missing");
            const std::uint16_t v=std::uint16_t(data[src]) | (std::uint16_t(data[src+1])<<8); src+=2;
            if (v==0) break;
            length=v & 7; offset=int(v>>3) | ~0x1FFF;
            if (length==0) {
                if (src>=size) throw std::runtime_error("PRS long length missing");
                length=int(data[src++])+1;
            } else length+=2;
        } else {
            length=(bit()<<1)|bit();
            if (src>=size) throw std::runtime_error("PRS short copy missing");
            offset=int(data[src++]) | ~0xFF; length+=2;
        }
        for (int i=0;i<length;++i) {
            const auto pos=static_cast<std::ptrdiff_t>(out.size())+offset;
            if (pos<0 || static_cast<std::size_t>(pos)>=out.size()) throw std::runtime_error("PRS invalid back-reference");
            out.push_back(out[static_cast<std::size_t>(pos)]);
        }
    }
    if (expected && out.size()!=expected) throw std::runtime_error("PRS decompressed-size mismatch");
    return out;
}

DecompressionResult decompress(CompressionCodec codec, std::span<const std::byte> src, std::size_t expected_size) {
    DecompressionResult result;
    try {
        switch (codec) {
        case CompressionCodec::None:
            result.bytes.assign(src.begin(), src.end()); break;
        case CompressionCodec::PRS:
            result.bytes=prs_decode(src, expected_size); break;
        default:
            throw std::runtime_error("codec is declared but not enabled in this build");
        }
        if (expected_size && codec==CompressionCodec::None && result.bytes.size()!=expected_size)
            throw std::runtime_error("uncompressed-size mismatch");
        result.ok=true;
    } catch (const std::exception& e) { result.ok=false; result.error=e.what(); result.bytes.clear(); }
    return result;
}

} // namespace shadowpc

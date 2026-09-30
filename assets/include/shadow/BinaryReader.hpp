#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace shadow {

// Thrown for truncated, overflowing, or structurally inconsistent input.
// Callers do not get a partially described asset after this.
class ParseError : public std::runtime_error {
public:
    explicit ParseError(const std::string& message) : std::runtime_error(message) {}
};

inline std::size_t checked_mul(std::size_t count, std::size_t stride, const char* what) {
    if (count != 0 && stride > std::numeric_limits<std::size_t>::max() / count) {
        throw ParseError(std::string("size overflow in ") + what);
    }
    return count * stride;
}

inline std::size_t checked_add(std::size_t a, std::size_t b, const char* what) {
    if (a > std::numeric_limits<std::size_t>::max() - b) {
        throw ParseError(std::string("offset overflow in ") + what);
    }
    return a + b;
}

// Explicit cursor. Multi-byte fields are read with a chosen endian.
// Nothing here assumes a 32-bit address or a GameCube alignment.
class BinaryReader {
public:
    BinaryReader(std::span<const std::uint8_t> data, std::string what)
        : data_(data), what_(std::move(what)) {}

    std::size_t size() const noexcept { return data_.size(); }
    std::size_t tell() const noexcept { return pos_; }
    const std::string& what() const noexcept { return what_; }
    std::span<const std::uint8_t> bytes() const noexcept { return data_; }

    void seek(std::size_t offset) {
        if (offset > data_.size()) {
            fail("seek past end");
        }
        pos_ = offset;
    }

    void skip(std::size_t count) { seek(checked_add(pos_, count, what_.c_str())); }

    std::span<const std::uint8_t> read(std::size_t count) {
        const std::size_t next = checked_add(pos_, count, what_.c_str());
        if (next > data_.size()) {
            fail("truncated read");
        }
        auto out = data_.subspan(pos_, count);
        pos_ = next;
        return out;
    }

    std::span<const std::uint8_t> slice(std::size_t offset, std::size_t count) const {
        const std::size_t next = checked_add(offset, count, what_.c_str());
        if (next > data_.size()) {
            throw ParseError(what_ + ": slice out of range");
        }
        return data_.subspan(offset, count);
    }

    std::uint8_t u8() { return read(1)[0]; }

    std::uint16_t u16le() {
        auto b = read(2);
        return static_cast<std::uint16_t>(b[0] | (b[1] << 8));
    }
    std::uint16_t u16be() {
        auto b = read(2);
        return static_cast<std::uint16_t>((b[0] << 8) | b[1]);
    }
    std::uint32_t u32le() {
        auto b = read(4);
        return static_cast<std::uint32_t>(b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24));
    }
    std::uint32_t u32be() {
        auto b = read(4);
        return static_cast<std::uint32_t>((b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3]);
    }
    std::int32_t i32le() { return static_cast<std::int32_t>(u32le()); }
    float f32le() { return bit_cast_f32(u32le()); }
    float f32be() { return bit_cast_f32(u32be()); }

    std::uint32_t u32le_at(std::size_t offset) const {
        auto b = slice(offset, 4);
        return static_cast<std::uint32_t>(b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24));
    }
    std::uint16_t u16be_at(std::size_t offset) const {
        auto b = slice(offset, 2);
        return static_cast<std::uint16_t>((b[0] << 8) | b[1]);
    }
    std::uint32_t u32be_at(std::size_t offset) const {
        auto b = slice(offset, 4);
        return static_cast<std::uint32_t>((b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3]);
    }

    // Reads a name stored in a fixed on-disk field. The width is a property of
    // that record, not a cap on any PC-side string that later holds the result.
    std::string field_string(std::size_t field_bytes) {
        auto b = read(field_bytes);
        std::size_t n = 0;
        while (n < b.size() && b[n] != 0) {
            ++n;
        }
        return std::string(reinterpret_cast<const char*>(b.data()), n);
    }

    std::string rest_cstring() {
        std::size_t n = 0;
        while (pos_ + n < data_.size() && data_[pos_ + n] != 0) {
            ++n;
        }
        auto b = read(n);
        if (pos_ < data_.size() && data_[pos_] == 0) {
            skip(1);
        }
        return std::string(reinterpret_cast<const char*>(b.data()), b.size());
    }

    bool exhausted() const noexcept { return pos_ == data_.size(); }

private:
    static float bit_cast_f32(std::uint32_t bits) {
        float value = 0.f;
        static_assert(sizeof(value) == sizeof(bits));
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }

    void fail(const char* message) const {
        throw ParseError(what_ + " at " + std::to_string(pos_) + ": " + message);
    }

    std::span<const std::uint8_t> data_;
    std::string what_;
    std::size_t pos_ = 0;
};

inline std::vector<std::uint8_t> read_binary_file(const std::string& path) {
    // Defined out of line in the .cpp below via a header implementation would
    // pull <fstream> into every TU. The function lives in FileIO.cpp.
    extern std::vector<std::uint8_t> read_binary_file_impl(const std::string& path);
    return read_binary_file_impl(path);
}

}  // namespace shadow

#include "shadow/boot/BootFoundation.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace shadow::boot {
namespace {

constexpr std::uint32_t kEntry = 0x80003154u;
constexpr std::uint32_t kRegisterHelper = 0x800032B0u;
constexpr std::uint32_t kStopBeforeHardware = 0x80003158u;
constexpr std::size_t kDolSize = 0x5816E0u;

struct Section {
    bool text;
    std::uint32_t index;
    std::uint32_t file_offset;
    std::uint32_t address;
    std::uint32_t size;
};

// PROVEN: DOL header and the checked map in MINIMAL_BOOT_FOUNDATION.md.
constexpr std::array<Section, 10> kSections{{
    {true,  0, 0x00000100u, 0x80003100u, 0x00002500u},
    {true,  1, 0x00002600u, 0x80008D40u, 0x004A1F20u},
    {false, 0, 0x004A4520u, 0x80005600u, 0x00001F20u},
    {false, 1, 0x004A6440u, 0x80007520u, 0x00001820u},
    {false, 2, 0x004A7C60u, 0x804AAC60u, 0x00000480u},
    {false, 3, 0x004A80E0u, 0x804AB0E0u, 0x00000020u},
    {false, 4, 0x004A8100u, 0x804AB100u, 0x00072420u},
    {false, 5, 0x0051A520u, 0x8051D520u, 0x000528E0u},
    {false, 6, 0x0056CE00u, 0x805E4500u, 0x0000AB20u},
    {false, 7, 0x00577920u, 0x805F2780u, 0x00009DC0u},
}};

// PROVEN: every word of the register helper, checked against main.dol and
// the independent address/word table in experimental_native_boot. Checking
// only its first/last word would permit a changed middle instruction to pass.
constexpr std::array<std::uint32_t, 36> kRegisterHelperWords{{
    0x38000000u, 0x38600000u, 0x38800000u, 0x38A00000u,
    0x38C00000u, 0x38E00000u, 0x39000000u, 0x39200000u,
    0x39400000u, 0x39600000u, 0x39800000u, 0x39C00000u,
    0x39E00000u, 0x3A000000u, 0x3A200000u, 0x3A400000u,
    0x3A600000u, 0x3A800000u, 0x3AA00000u, 0x3AC00000u,
    0x3AE00000u, 0x3B000000u, 0x3B200000u, 0x3B400000u,
    0x3B600000u, 0x3B800000u, 0x3BA00000u, 0x3BC00000u,
    0x3BE00000u, 0x3C208060u, 0x6021C5F0u, 0x3C40805Fu,
    0x6042A780u, 0x3DA0805Eu, 0x61ADC500u, 0x4E800020u,
}};

std::uint32_t ReadBE32(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 4) {
        throw std::runtime_error("read outside DOL fixture");
    }
    return (std::uint32_t(bytes[offset]) << 24) |
           (std::uint32_t(bytes[offset + 1]) << 16) |
           (std::uint32_t(bytes[offset + 2]) << 8) |
            std::uint32_t(bytes[offset + 3]);
}

}  // namespace

BootImage::BootImage(std::vector<std::uint8_t> dol) : dol_(std::move(dol)) {}

std::uint32_t BootImage::ReadWord(std::uint32_t address) const {
    if ((address & 3u) != 0u) throw std::runtime_error("unaligned guest word");
    for (const Section& s : kSections) {
        if (address >= s.address &&
            std::uint64_t(address) + 4 <= std::uint64_t(s.address) + s.size) {
            return ReadBE32(dol_, s.file_offset + (address - s.address));
        }
    }
    throw std::runtime_error("guest address is not in a known DOL section");
}

BootImage LoadValidatedFixture(std::vector<std::uint8_t> dol) {
    if (dol.size() != kDolSize || ReadBE32(dol, 0xE0) != kEntry ||
        ReadBE32(dol, 0xD8) != 0x8056FE00u ||
        ReadBE32(dol, 0xDC) != 0x0008C7ECu) {
        throw std::runtime_error("DOL size, entry, or BSS header mismatch");
    }
    for (const Section& s : kSections) {
        const std::size_t i = s.index;
        const std::size_t offset_field = (s.text ? 0x00u : 0x1Cu) + i * 4u;
        const std::size_t address_field = (s.text ? 0x48u : 0x64u) + i * 4u;
        const std::size_t size_field = (s.text ? 0x90u : 0xACu) + i * 4u;
        if (ReadBE32(dol, offset_field) != s.file_offset ||
            ReadBE32(dol, address_field) != s.address ||
            ReadBE32(dol, size_field) != s.size ||
            std::uint64_t(s.file_offset) + s.size > dol.size() ||
            s.address < 0x80000000u ||
            std::uint64_t(s.address) + s.size > 0x81800000ull) {
            throw std::runtime_error("DOL section does not match BootManifest");
        }
    }
    // PROVEN for this PAL header: unused text slots 2..6 and data slots
    // 8..10 have zero offset, address and size. An extra nonempty slot must
    // not slip through the known-section view.
    for (std::size_t i = 2; i < 7; ++i) {
        if (ReadBE32(dol, i * 4) != 0 ||
            ReadBE32(dol, 0x48 + i * 4) != 0 ||
            ReadBE32(dol, 0x90 + i * 4) != 0) {
            throw std::runtime_error("unexpected DOL text section");
        }
    }
    for (std::size_t i = 8; i < 11; ++i) {
        if (ReadBE32(dol, 0x1C + i * 4) != 0 ||
            ReadBE32(dol, 0x64 + i * 4) != 0 ||
            ReadBE32(dol, 0xAC + i * 4) != 0) {
            throw std::runtime_error("unexpected DOL data section");
        }
    }
    return BootImage(std::move(dol));
}

StartupState EnterRegisterStartup(const BootImage& image) {
    // 0x80003154 bl 0x800032B0; return target at 0x80003158 is
    // 0x80003400 (hardware/runtime helper), deliberately not entered.
    if (image.ReadWord(kEntry) != 0x4800015Du ||
        image.ReadWord(kStopBeforeHardware) != 0x480002A9u) {
        throw std::runtime_error("entry instruction fingerprint mismatch");
    }
    for (std::size_t i = 0; i < kRegisterHelperWords.size(); ++i) {
        if (image.ReadWord(kRegisterHelper + static_cast<std::uint32_t>(4 * i)) !=
            kRegisterHelperWords[i]) {
            throw std::runtime_error("register helper instruction fingerprint mismatch");
        }
    }

    StartupState state;
    // Direct semantics of all 36 pinned helper instructions. No memory read,
    // FPR/CR or hardware state is claimed at this stop point.
    state.gpr.fill(0);
    state.gpr[1] = 0x8060C5F0u;
    state.gpr[2] = 0x805FA780u;
    state.gpr[13] = 0x805EC500u;
    state.lr = kStopBeforeHardware;
    state.pc = state.lr;
    return state;
}

}  // namespace shadow::boot

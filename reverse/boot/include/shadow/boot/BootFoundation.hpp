#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace shadow::boot {

// PROVEN for PAL GUPP8P: DOL entry 0x80003154 and register helper
// 0x800032B0..0x8000333C. This state stops before 0x80003400.
struct StartupState {
    std::array<std::uint32_t, 32> gpr{};
    std::uint32_t pc = 0;
    std::uint32_t lr = 0;
};

class BootImage {
public:
    std::uint32_t ReadWord(std::uint32_t address) const;

private:
    explicit BootImage(std::vector<std::uint8_t> dol);
    friend BootImage LoadValidatedFixture(std::vector<std::uint8_t> dol);
    std::vector<std::uint8_t> dol_;
};

// Validates the PAL header/section map. The caller must separately verify the
// full DOL SHA-256 when claiming identity with the known PAL input.
BootImage LoadValidatedFixture(std::vector<std::uint8_t> dol);
StartupState EnterRegisterStartup(const BootImage& image);

}  // namespace shadow::boot

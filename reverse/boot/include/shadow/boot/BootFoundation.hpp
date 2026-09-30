#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace shadow::boot {

// PROVEN for PAL GUPP8P: DOL entry 0x80003154 and register helper
// 0x800032B0..0x8000333C. The next translated branch reaches the entry of
// 0x80003400 but does not execute that callee.
struct StartupState {
    std::array<std::uint32_t, 32> gpr{};
    std::uint32_t pc = 0;
    std::uint32_t lr = 0;
};

// The first five instructions of 0x80003400, ending before 0x80371714.
// MSR is supplied by the original pre-entry state; no default is valid.
struct HardwareCallPrefix {
    StartupState cpu;
    std::uint32_t msr = 0;
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
// PROVEN: bl at 0x80003158 sets LR=0x8000315C and PC=0x80003400;
// all GPRs are unchanged. No hardware-helper instruction runs here.
StartupState EnterHardwareCall(const BootImage& image, const StartupState& state);
HardwareCallPrefix EnterPairedSetupCall(const BootImage& image,
                                        const StartupState& state,
                                        std::uint32_t observed_msr);

}  // namespace shadow::boot

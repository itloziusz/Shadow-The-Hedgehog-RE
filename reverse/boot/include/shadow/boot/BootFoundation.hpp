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

struct MemoryWrite32 {
    std::uint32_t address = 0;
    std::uint32_t value = 0;  // written in guest big-endian byte order
};

// Bounded guest stack bytes touched by the paired-setup prologue. A validity
// bit is kept for each byte: an unwritten stack slot is not assumed zero.
// The two reached stores cover [base, base+3] and [base+12, base+15].
struct PairedSetupStackMemory {
    std::uint32_t base = 0;
    std::array<std::uint8_t, 16> bytes{};
    std::array<bool, 16> valid{};

    void StoreBE32(std::uint32_t address, std::uint32_t value);
    std::uint32_t LoadBE32(std::uint32_t address) const;
};

// First four words of the paired setup callee. Stops at the HID2 read leaf,
// before any hardware register is read. Writes are in original program order.
struct PairedSetupStackPrefix {
    HardwareCallPrefix machine;
    std::array<MemoryWrite32, 2> ordered_writes{};
    PairedSetupStackMemory stack_memory;
};

struct SprWriteRequest {
    std::uint32_t spr = 0;
    std::uint32_t value = 0;
};

// The write is an issued Gekko SPR command, not a claim about a later
// hardware readback. Stops before the next HID0-reading call at 0x8037172C.
struct Hid2WriteBoundary {
    PairedSetupStackPrefix prefix;
    SprWriteRequest request;
};

// HID0[ICFI] is a command bit. The issued operand is not a persistent
// readback value; stop before the following sync and later cache consumers.
struct Hid0IcfiBoundary {
    Hid2WriteBoundary prefix;
    SprWriteRequest request;
};

// A prediction conditional on successful completion of the still-unmodeled
// ICFI/sync boundary. This is not a connected boot state. It projects only
// the byte-proven instructions 0x80371734..0x80371764 and their ordinary
// register/stack/GQR write operands; no hardware acceptance is asserted.
struct PostSyncGqrTailPrediction {
    HardwareCallPrefix before_saved_lr_load;
    std::array<SprWriteRequest, 8> ordered_gqr_writes{};
    HardwareCallPrefix after_return;
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
PairedSetupStackPrefix EnterHid2ReadCall(const BootImage& image,
                                         const HardwareCallPrefix& state);
// Reads the two-word HID2 accessor only with an explicit, externally
// measured SPR value. No value is inferred from the DOL or defaulted.
PairedSetupStackPrefix ReturnFromHid2Read(const BootImage& image,
                                          const PairedSetupStackPrefix& state,
                                          std::uint32_t measured_hid2);
Hid2WriteBoundary IssueHid2Write(const BootImage& image,
                                 const PairedSetupStackPrefix& state);
Hid0IcfiBoundary IssueHid0IcfiRequest(const BootImage& image,
                                      const Hid2WriteBoundary& state,
                                      std::uint32_t measured_hid0);
PostSyncGqrTailPrediction PredictPostSyncGqrTail(const BootImage& image,
                                                const Hid0IcfiBoundary& state);

}  // namespace shadow::boot

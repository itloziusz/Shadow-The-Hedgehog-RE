#include "shadow/boot/FprSeedProjection.hpp"

#include <stdexcept>

namespace shadow::boot {
namespace {

std::uint32_t ReadBE32(const std::array<std::uint8_t, 16>& bytes, unsigned offset) {
    return (std::uint32_t(bytes[offset]) << 24) |
           (std::uint32_t(bytes[offset + 1]) << 16) |
           (std::uint32_t(bytes[offset + 2]) << 8) | bytes[offset + 3];
}

std::uint64_t WidenNormalOrZero(std::uint32_t bits) {
    const auto exponent = (bits >> 23) & 0xFFu;
    const auto fraction = bits & 0x7FFFFFu;
    const auto sign = std::uint64_t(bits & 0x80000000u) << 32;
    if (exponent == 0u) {
        if (fraction != 0u) throw std::runtime_error("paired subnormal requires independent validation");
        return sign;
    }
    if (exponent == 0xFFu) throw std::runtime_error("paired nonfinite requires independent validation");
    return sign | (std::uint64_t(exponent + 896u) << 52) |
           (std::uint64_t(fraction) << 29);
}

std::uint32_t WriteAllFpscrFields(std::uint64_t bits) {
    // Gekko mtfsf FM=FF. VX and FEX are summaries, not copied source bits.
    // IBM bit 20 (numeric 0x800) is reserved and reads as zero.
    auto word = static_cast<std::uint32_t>(bits) & ~0x60000800u;
    if ((word & 0x01F80700u) != 0u) word |= 0x20000000u;
    if (((word & 0x20000080u) == 0x20000080u) ||
        ((word & 0x10000040u) == 0x10000040u) ||
        ((word & 0x08000020u) == 0x08000020u) ||
        ((word & 0x04000010u) == 0x04000010u) ||
        ((word & 0x02000008u) == 0x02000008u)) word |= 0x40000000u;
    return word;
}

void CheckWords(const BootImage& image) {
    constexpr std::array<std::uint32_t, 9> prefix{{
        0x7C6000A6u, 0x60632000u, 0x7C600124u, 0x7C78E2A6u,
        0x54631FFFu, 0x4182008Cu, 0x3C60805Fu, 0x38631F38u, 0xE0030000u,
    }};
    for (unsigned i = 0; i < prefix.size(); ++i) {
        if (image.ReadWord(0x80370CDCu + 4u * i) != prefix[i])
            throw std::runtime_error("FPR prefix instruction fingerprint mismatch");
    }
    for (unsigned n = 1; n < 32; ++n) {
        if (image.ReadWord(0x80370D00u + 4u * (n - 1)) != (0x10000090u | (n << 21)) ||
            image.ReadWord(0x80370D80u + 4u * (n - 1)) != (0xFC000090u | (n << 21)))
            throw std::runtime_error("FPR sweep instruction fingerprint mismatch");
    }
    if (image.ReadWord(0x80370D7Cu) != 0xC80D5A30u ||
        image.ReadWord(0x80370DFCu) != 0xFDFE058Eu ||
        image.ReadWord(0x80370E00u) != 0x4E800020u)
        throw std::runtime_error("FPR scalar tail instruction fingerprint mismatch");
}

}  // namespace

FprSeedPrediction PredictFprSeed(const BootImage& image, const FprSeedInputs& inputs) {
    CheckWords(image);
    const auto& entry = inputs.entry;
    if (entry.machine.cpu.pc != 0x80370CDCu)
        throw std::runtime_error("FPR seed entry PC mismatch");
    // POW, PR, EE, FE0/FE1, trace modes and LE are outside this bounded
    // ordinary big-endian path. Do not silently ignore their consequences.
    if ((entry.machine.msr & 0x4CF01u) != 0u)
        throw std::runtime_error("FPR seed has unsupported power/privilege/interrupt/trace/FP/endian mode");
    if ((entry.fpscr & 0x800u) != 0u)
        throw std::runtime_error("incoming FPSCR sets reserved bit 20");
    if ((inputs.live_hid2 & 0xA0000000u) != 0xA0000000u)
        throw std::runtime_error("unvalidated HID2 paired/quantized path");
    if (inputs.live_gqr0 != 0u)
        throw std::runtime_error("unvalidated nonzero GQR0");
    if (inputs.source_address != 0x805F1F30u || entry.machine.cpu.gpr[13] != 0x805EC500u)
        throw std::runtime_error("FPR source provenance/address mismatch");
    const auto scalar = (std::uint64_t(ReadBE32(inputs.source_bytes, 0)) << 32) |
                         ReadBE32(inputs.source_bytes, 4);
    if (((scalar >> 52) & 0x7FFu) == 0x7FFu)
        throw std::runtime_error("scalar nonfinite requires independent validation");
    const FprLanes pair{WidenNormalOrZero(ReadBE32(inputs.source_bytes, 8)),
                        WidenNormalOrZero(ReadBE32(inputs.source_bytes, 12))};

    FprSeedPrediction out{};
    auto state = entry;
    state.machine.msr |= 0x2000u;
    state.machine.cpu.gpr[3] = (inputs.live_hid2 >> 29) & 1u;
    const auto comparison = state.machine.cpu.gpr[3] == 0u ? 0x20000000u : 0x40000000u;
    state.cr = (state.cr & 0x0FFFFFFFu) | comparison | (state.xer & 0x80000000u ? 0x10000000u : 0u);
    state.machine.cpu.pc = 0x80370CF0u;
    out.checkpoints[0] = state;
    state.machine.cpu.gpr[3] = 0x805F1F38u; // original lis/addi constant, not a state default
    state.machine.cpu.pc = 0x80370CFCu;
    out.checkpoints[1] = state;
    state.fpr[0] = pair;
    state.machine.cpu.pc = 0x80370D00u;
    out.checkpoints[2] = state;
    for (unsigned n = 1; n < 32; ++n) state.fpr[n] = pair;
    state.machine.cpu.pc = 0x80370D7Cu;
    out.checkpoints[3] = state;
    state.fpr[0].ps0 = scalar;  // lfd preserves the measured paired second lane
    state.machine.cpu.pc = 0x80370D80u;
    out.checkpoints[4] = state;
    state.fpr[1].ps0 = scalar;
    state.machine.cpu.pc = 0x80370D84u;
    out.checkpoints[5] = state;
    for (unsigned n = 2; n < 32; ++n) state.fpr[n].ps0 = scalar;
    state.machine.cpu.pc = 0x80370DFCu;
    out.checkpoints[6] = state;
    state.fpscr = WriteAllFpscrFields(scalar);
    state.machine.cpu.pc = state.machine.cpu.lr & ~3u;
    out.checkpoints[7] = state;
    return out;
}

}  // namespace shadow::boot

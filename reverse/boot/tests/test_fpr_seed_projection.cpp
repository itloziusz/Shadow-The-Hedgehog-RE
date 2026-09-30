#include "shadow/boot/FprSeedProjection.hpp"

#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace {
void Require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
template<class F> void Reject(F action, const char* message) {
    try { action(); } catch (const std::exception&) { return; }
    throw std::runtime_error(message);
}
void PutWord(std::array<std::uint8_t, 16>& bytes, unsigned offset, std::uint32_t word) {
    for (unsigned i = 0; i < 4; ++i) bytes[offset+i] = static_cast<std::uint8_t>(word >> (24-8*i));
}
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("PAL DOL argument required");
        std::ifstream file(argv[1], std::ios::binary);
        if (!file) throw std::runtime_error("PAL DOL unavailable");
        const std::vector<std::uint8_t> original(std::istreambuf_iterator<char>{file}, {});
        const auto image = shadow::boot::LoadValidatedFixture(original);
        shadow::boot::FprSeedInputs inputs;
        inputs.entry.machine.cpu.pc = 0x80370CDCu;
        inputs.entry.machine.cpu.lr = 0x80003418u;
        inputs.entry.machine.msr = 0x32u;
        inputs.entry.cr = 0x9ABCDEF0u;
        inputs.entry.xer = 0x80000000u;
        inputs.entry.fpscr = 0xDEADB6EFu;
        for (unsigned n = 0; n < 32; ++n) {
            inputs.entry.machine.cpu.gpr[n] = 0x10000u + n;
            inputs.entry.fpr[n] = {0x1111000000000000ull + n, 0x2222000000000000ull + n};
        }
        inputs.entry.machine.cpu.gpr[13] = 0x805EC500u;
        inputs.live_hid2 = 0xE1234567u;
        inputs.live_gqr0 = 0u;
        inputs.source_address = 0x805F1F30u;
        PutWord(inputs.source_bytes, 0, 0x400A0000u); // double 3.25
        PutWord(inputs.source_bytes, 4, 0u);
        PutWord(inputs.source_bytes, 8, 0x3FC00000u); // single 1.5
        PutWord(inputs.source_bytes, 12, 0xC0100000u); // single -2.25
        const auto first = shadow::boot::PredictFprSeed(image, inputs);
        Require(first.checkpoints[0].cr == 0x5ABCDEF0u && first.checkpoints[0].xer == 0x80000000u,
                "CR0 failed to retain XER.SO and untouched fields");
        Require(first.checkpoints[0].machine.cpu.gpr[3] == 1u &&
                first.checkpoints[0].machine.msr == 0x2032u, "MSR/branch data flow differs");
        const auto& loaded = first.checkpoints[2];
        Require(loaded.fpr[0].ps0 == 0x3FF8000000000000ull && loaded.fpr[0].ps1 == 0xC002000000000000ull,
                "paired finite widening differs");
        for (unsigned n = 1; n < 32; ++n)
            Require(loaded.fpr[n].ps0 == inputs.entry.fpr[n].ps0 && loaded.fpr[n].ps1 == inputs.entry.fpr[n].ps1,
                    "pair load changed an untouched register");
        const auto& one_scalar_move = first.checkpoints[5];
        Require(one_scalar_move.fpr[1].ps0 == 0x400A000000000000ull &&
                one_scalar_move.fpr[2].ps0 == 0x3FF8000000000000ull,
                "first scalar move silently completed the whole sweep");
        const auto& returned = first.checkpoints[7];
        for (unsigned n = 0; n < 32; ++n) {
            Require(returned.fpr[n].ps0 == 0x400A000000000000ull &&
                    returned.fpr[n].ps1 == 0xC002000000000000ull, "scalar sweep erased a paired lane");
            if (n != 3) Require(returned.machine.cpu.gpr[n] == inputs.entry.machine.cpu.gpr[n],
                                "FPR routine changed an unrelated GPR");
        }
        Require(returned.fpscr == 0u && returned.machine.cpu.pc == 0x80003418u,
                "FPSCR/return differs");
        auto changed = inputs;
        PutWord(changed.source_bytes, 0, 0x3FF00000u);
        PutWord(changed.source_bytes, 4, 0x40000000u); // raw FEX but no active exception
        Require(shadow::boot::PredictFprSeed(image, changed).checkpoints[7].fpscr == 0u,
                "mtfsf copied raw FEX");
        PutWord(changed.source_bytes, 4, 0x00000800u);
        Require(shadow::boot::PredictFprSeed(image, changed).checkpoints[7].fpscr == 0u,
                "mtfsf wrote reserved bit 20");
        PutWord(changed.source_bytes, 4, 0x01000080u); // invalid flag and VE
        Require(shadow::boot::PredictFprSeed(image, changed).checkpoints[7].fpscr == 0x61000080u,
                "mtfsf failed to derive VX/FEX");
        PutWord(changed.source_bytes, 8, 0x80000000u); // -0
        PutWord(changed.source_bytes, 12, 0u); // +0
        Require(shadow::boot::PredictFprSeed(image, changed).checkpoints[2].fpr[0].ps0 == 0x8000000000000000ull &&
                shadow::boot::PredictFprSeed(image, changed).checkpoints[2].fpr[0].ps1 == 0u,
                "signed zero lost its sign or crossed lanes");
        PutWord(changed.source_bytes, 8, 0x00800000u); // smallest normal
        PutWord(changed.source_bytes, 12, 0xFF7FFFFFu); // largest negative finite
        const auto extremes = shadow::boot::PredictFprSeed(image, changed);
        Require(extremes.checkpoints[2].fpr[0].ps0 == 0x3810000000000000ull &&
                extremes.checkpoints[2].fpr[0].ps1 == 0xC7EFFFFFE0000000ull, "finite exponent boundary differs");
        changed = inputs;
        changed.entry.machine.cpu.lr = 0x8000341Bu;
        Require(shadow::boot::PredictFprSeed(image, changed).checkpoints[7].machine.cpu.lr == 0x8000341Bu &&
                shadow::boot::PredictFprSeed(image, changed).checkpoints[7].machine.cpu.pc == 0x80003418u,
                "blr did not preserve LR while aligning its target");
        for (auto flag : {0x40000u, 0x4000u, 0x8000u, 0x800u, 0x100u, 0x400u, 0x200u, 1u}) {
            changed = inputs; changed.entry.machine.msr |= flag;
            Reject([&] { (void)shadow::boot::PredictFprSeed(image, changed); }, "unsupported MSR path accepted");
        }
        for (auto value : {0u, 0x20000000u, 0x80000000u}) {
            changed = inputs; changed.live_hid2 = value;
            Reject([&] { (void)shadow::boot::PredictFprSeed(image, changed); }, "unsupported HID2 path accepted");
        }
        changed = inputs; changed.live_gqr0 = 1u;
        Reject([&] { (void)shadow::boot::PredictFprSeed(image, changed); }, "nonzero GQR accepted");
        changed = inputs; changed.entry.fpscr |= 0x800u;
        Reject([&] { (void)shadow::boot::PredictFprSeed(image, changed); }, "impossible reserved FPSCR input accepted");
        changed = inputs; changed.source_address += 4;
        Reject([&] { (void)shadow::boot::PredictFprSeed(image, changed); }, "wrong source provenance accepted");
        changed = inputs; changed.entry.machine.cpu.gpr[13] += 4;
        Reject([&] { (void)shadow::boot::PredictFprSeed(image, changed); }, "wrong SDA source accepted");
        changed = inputs; changed.entry.machine.cpu.pc += 4;
        Reject([&] { (void)shadow::boot::PredictFprSeed(image, changed); }, "wrong entry PC accepted");
        for (auto offset : {8u, 12u}) for (auto value : {1u, 0x80000001u, 0x7F800000u, 0x7FC01234u, 0x7F800001u}) {
            changed = inputs; PutWord(changed.source_bytes, offset, value);
            Reject([&] { (void)shadow::boot::PredictFprSeed(image, changed); }, "exceptional paired input accepted");
        }
        changed = inputs; PutWord(changed.source_bytes, 0, 0x7FF00000u);
        Reject([&] { (void)shadow::boot::PredictFprSeed(image, changed); }, "nonfinite double accepted");
        for (unsigned n = 0; n < 74; ++n) {
            auto mutated = original;
            mutated[0x36A59Cu + 4*n + 3] ^= 1;
            Reject([&] { (void)shadow::boot::PredictFprSeed(shadow::boot::LoadValidatedFixture(mutated), inputs); },
                   "changed FPR instruction accepted");
        }
        const auto repeated = shadow::boot::PredictFprSeed(image, inputs);
        Require(repeated.checkpoints[7].fpr[31].ps0 == first.checkpoints[7].fpr[31].ps0 &&
                repeated.checkpoints[7].fpscr == first.checkpoints[7].fpscr, "stale state leaked between calls");
        std::cout << "PAL FPR seed bounded projection and adversarial gates passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
}

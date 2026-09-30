#include "shadow/boot/FprSeedProjection.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>

int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::runtime_error("usage: fpr_projection <main.dol> <explicit-state.txt>");
        std::ifstream dol(argv[1], std::ios::binary), input(argv[2]);
        if (!dol || !input) throw std::runtime_error("cannot open explicit fixture");
        const auto image = shadow::boot::LoadValidatedFixture(
            std::vector<std::uint8_t>(std::istreambuf_iterator<char>{dol}, {}));
        const auto read = [&input]() {
            std::uint64_t word;
            if (!(input >> std::hex >> word)) throw std::runtime_error("incomplete explicit state");
            return word;
        };
        const auto read32 = [&read]() {
            auto word = read();
            if (word > std::numeric_limits<std::uint32_t>::max()) throw std::runtime_error("32-bit field overflow");
            return static_cast<std::uint32_t>(word);
        };
        shadow::boot::FprSeedInputs data;
        auto& entry = data.entry;
        entry.machine.msr = read32(); entry.machine.cpu.pc = read32();
        entry.machine.cpu.lr = read32(); entry.cr = read32(); entry.xer = read32();
        entry.fpscr = read32(); data.live_hid2 = read32(); data.live_gqr0 = read32();
        data.source_address = read32();
        for (auto& r : entry.machine.cpu.gpr) r = read32();
        for (auto& f : entry.fpr) f.ps0 = read();
        for (auto& f : entry.fpr) f.ps1 = read();
        for (unsigned n = 0; n < 4; ++n) {
            const auto word = read32();
            for (unsigned b = 0; b < 4; ++b) data.source_bytes[4*n+b] = static_cast<std::uint8_t>(word >> (24-8*b));
        }
        std::string extra;
        if (input >> extra) throw std::runtime_error("unexpected extra state fields");
        const auto prediction = shadow::boot::PredictFprSeed(image, data);
        std::cout << std::hex << std::setfill('0');
        for (const auto& state : prediction.checkpoints) {
            std::cout << std::setw(8) << state.machine.cpu.pc << ' '
                      << std::setw(8) << state.machine.msr << ' '
                      << std::setw(8) << state.machine.cpu.lr << ' '
                      << std::setw(8) << state.cr << ' ' << std::setw(8) << state.xer << ' '
                      << std::setw(8) << state.fpscr;
            for (auto r : state.machine.cpu.gpr) std::cout << ' ' << std::setw(8) << r;
            for (auto f : state.fpr) std::cout << ' ' << std::setw(16) << f.ps0;
            for (auto f : state.fpr) std::cout << ' ' << std::setw(16) << f.ps1;
            std::cout << '\n';
        }
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FPR projection declined: " << error.what() << '\n';
        return 1;
    }
}

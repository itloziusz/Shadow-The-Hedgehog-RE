#include "shadow/boot/NativeBootPrefix.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>

int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::runtime_error("usage: native_prefix <main.dol> <explicit-entry.txt>");
        std::ifstream dol(argv[1], std::ios::binary), input(argv[2]);
        if (!dol || !input) throw std::runtime_error("cannot open explicit fixture");
        const auto image = shadow::boot::LoadValidatedFixture(
            std::vector<std::uint8_t>(std::istreambuf_iterator<char>{dol}, {}));
        const auto read = [&input]() {
            std::uint64_t word;
            if (!(input >> std::hex >> word)) throw std::runtime_error("incomplete explicit entry state");
            return word;
        };
        const auto read32 = [&read]() {
            auto word = read();
            if (word > std::numeric_limits<std::uint32_t>::max()) throw std::runtime_error("32-bit field overflow");
            return static_cast<std::uint32_t>(word);
        };
        shadow::boot::NativeBootInputs data{};
        data.msr = read32(); data.hid0 = read32(); data.hid2 = read32();
        data.cr = read32(); data.xer = read32(); data.ctr = read32(); data.fpscr = read32();
        for (auto& f : data.fpr) f.ps0 = read();
        for (auto& f : data.fpr) f.ps1 = read();
        for (auto& q : data.gqr) q = read32();
        data.source_address = read32();
        for (unsigned n = 0; n < 4; ++n) {
            const auto word = read32();
            for (unsigned b = 0; b < 4; ++b) data.source_bytes[4*n+b] = static_cast<std::uint8_t>(word >> (24-8*b));
        }
        std::string extra;
        if (input >> extra) throw std::runtime_error("unexpected extra entry fields");
        const auto run = shadow::boot::RunImmutableNativeBootPrefix(image, data);
        std::cout << std::hex << std::setfill('0');
        for (const auto& cp : run.checkpoints) {
            const auto& state = cp.state;
            std::cout << std::setw(8) << state.machine.cpu.pc << ' '
                      << std::setw(8) << state.machine.msr << ' '
                      << std::setw(8) << state.machine.cpu.lr << ' '
                      << std::setw(8) << state.cr << ' ' << std::setw(8) << state.xer << ' '
                      << std::setw(8) << state.fpscr;
            for (auto r : state.machine.cpu.gpr) std::cout << ' ' << std::setw(8) << r;
            for (auto f : state.fpr) std::cout << ' ' << std::setw(16) << f.ps0;
            for (auto f : state.fpr) std::cout << ' ' << std::setw(16) << f.ps1;
            std::cout << ' ' << std::setw(8) << cp.ctr << ' ' << std::setw(8) << cp.hid0
                      << ' ' << std::setw(8) << cp.hid2;
            for (auto q : cp.gqr) std::cout << ' ' << std::setw(8) << q;
            std::cout << ' ';
            for (auto b : cp.paired_stack.bytes) std::cout << std::setw(2) << unsigned(b);
            std::cout << ' ';
            for (auto b : cp.paired_stack.valid) std::cout << (b ? '1' : '0');
            std::cout << '\n';
        }
        for (const auto& write : run.cache_stack_writes) {
            const auto& bytes = run.cache_stack[write.address < run.cache_stack[1].base ? 0u : 1u];
            std::cout << "COMMITTED_STACK " << std::setw(8) << write.address << ' '
                      << std::setw(8) << bytes.LoadBE32(write.address) << '\n';
        }
        std::cout << "STOP pc=0x" << std::setw(8) << run.checkpoints.back().state.machine.cpu.pc
                  << " reason=LIVE_L2CR_UNRESOLVED\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Native prefix declined: " << error.what() << '\n';
        return 1;
    }
}

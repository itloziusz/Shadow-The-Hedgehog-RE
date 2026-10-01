#include "shadow/boot/NativeL2Prefix.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>

namespace {
void Print(const shadow::boot::NativeBootCheckpoint& cp, std::uint32_t l2cr,
           const shadow::boot::L2StackBytes* stack) {
    const auto& s = cp.state;
    std::cout << std::setw(8) << s.machine.cpu.pc << ' ' << std::setw(8) << s.machine.msr
              << ' ' << std::setw(8) << s.machine.cpu.lr << ' ' << std::setw(8) << s.cr
              << ' ' << std::setw(8) << s.xer << ' ' << std::setw(8) << s.fpscr;
    for (auto r : s.machine.cpu.gpr) std::cout << ' ' << std::setw(8) << r;
    for (auto f : s.fpr) std::cout << ' ' << std::setw(16) << f.ps0;
    for (auto f : s.fpr) std::cout << ' ' << std::setw(16) << f.ps1;
    std::cout << ' ' << std::setw(8) << cp.ctr << ' ' << std::setw(8) << cp.hid0
              << ' ' << std::setw(8) << cp.hid2;
    for (auto q : cp.gqr) std::cout << ' ' << std::setw(8) << q;
    std::cout << ' ';
    for (auto b : cp.paired_stack.bytes) std::cout << std::setw(2) << unsigned(b);
    std::cout << ' ';
    for (auto b : cp.paired_stack.valid) std::cout << (b ? '1' : '0');
    std::cout << ' ' << std::setw(8) << l2cr << ' ';
    if (stack) {
        for (auto b : stack->bytes) std::cout << std::setw(2) << unsigned(b);
        std::cout << ' ';
        for (auto b : stack->valid) std::cout << (b ? '1' : '0');
    } else std::cout << "- -";
    std::cout << '\n';
}
}
int main(int argc, char** argv) {
    try {
        if (argc != 3) throw std::runtime_error("usage: native_l2 <main.dol> <explicit-entry-with-l2.txt>");
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
        shadow::boot::NativeL2Inputs data{};
        auto& entry = data.entry;
        entry.msr = read32(); entry.hid0 = read32(); entry.hid2 = read32();
        entry.cr = read32(); entry.xer = read32(); entry.ctr = read32(); entry.fpscr = read32();
        for (auto& f : entry.fpr) f.ps0 = read();
        for (auto& f : entry.fpr) f.ps1 = read();
        for (auto& q : entry.gqr) q = read32();
        entry.source_address = read32();
        for (unsigned n = 0; n < 4; ++n) {
            const auto word = read32();
            for (unsigned b = 0; b < 4; ++b) entry.source_bytes[4*n+b] = static_cast<std::uint8_t>(word >> (24-8*b));
        }
        data.l2cr = read32();
        std::string extra;
        if (input >> extra) throw std::runtime_error("unexpected extra entry fields");
        const auto run = shadow::boot::RunImmutableNativeL2Prefix(image, data);
        std::cout << std::hex << std::setfill('0');
        for (const auto& cp : run.prefix.checkpoints) Print(cp, data.l2cr, nullptr);
        for (const auto& cp : run.checkpoints) Print(cp.native, cp.l2cr, &cp.stack);
        for (const auto& write : run.prefix.cache_stack_writes) {
            const auto& bytes = run.prefix.cache_stack[write.address < run.prefix.cache_stack[1].base ? 0u : 1u];
            std::cout << "COMMITTED_STACK " << std::setw(8) << write.address << ' '
                      << std::setw(8) << bytes.LoadBE32(write.address) << '\n';
        }
        for (const auto& write : run.ordered_stack_writes)
            std::cout << "COMMITTED_L2 " << std::setw(8) << write.address << ' '
                      << std::setw(8) << run.checkpoints.back().stack.LoadBE32(write.address) << '\n';
        for (const auto& e : run.effects)
            std::cout << "L2_EFFECT " << unsigned(e.kind) << ' ' << std::setw(8) << e.pc << ' '
                      << std::setw(8) << e.value << '\n';
        std::cout << "STOP pc=0x80372904 reason=LIVE_HANDLER_SLOT_UNRESOLVED\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Native L2 prefix declined: " << e.what() << '\n';
        return 1;
    }
}

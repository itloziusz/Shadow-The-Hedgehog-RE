#include "shadow/boot/BootFoundation.hpp"

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("usage: shadow_boot_probe <main.dol>");
        std::ifstream file(argv[1], std::ios::binary);
        if (!file) throw std::runtime_error("cannot open main.dol fixture");
        std::vector<std::uint8_t> dol(std::istreambuf_iterator<char>{file}, {});
        const auto image = shadow::boot::LoadValidatedFixture(std::move(dol));
        const auto state = shadow::boot::EnterRegisterStartup(image);
        std::cout << std::hex << std::uppercase << std::setfill('0')
                  << "STOP pc=0x" << std::setw(8) << state.pc
                  << " lr=0x" << std::setw(8) << state.lr
                  << " r1=0x" << std::setw(8) << state.gpr[1]
                  << " r2=0x" << std::setw(8) << state.gpr[2]
                  << " r13=0x" << std::setw(8) << state.gpr[13] << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "boot probe: " << e.what() << '\n';
        return 1;
    }
}

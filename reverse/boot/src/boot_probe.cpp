#include "shadow/boot/BootFoundation.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    try {
        if (argc != 2 && argc != 4) {
            throw std::runtime_error("usage: shadow_boot_probe <main.dol> [--observed-msr <8-hex-digits>]");
        }
        std::uint32_t observed_msr = 0;
        if (argc == 4) {
            const std::string msr_text = argv[3];
            if (std::string(argv[2]) != "--observed-msr" || msr_text.size() != 8 ||
                !std::all_of(msr_text.begin(), msr_text.end(), [](unsigned char c) {
                    return std::isxdigit(c) != 0;
                })) {
                throw std::runtime_error("observed MSR must be eight hexadecimal digits");
            }
            std::size_t parsed = 0;
            observed_msr = static_cast<std::uint32_t>(std::stoul(msr_text, &parsed, 16));
            if (parsed != 8) throw std::runtime_error("invalid observed MSR");
        }
        std::ifstream file(argv[1], std::ios::binary);
        if (!file) throw std::runtime_error("cannot open main.dol fixture");
        std::vector<std::uint8_t> dol(std::istreambuf_iterator<char>{file}, {});
        const auto image = shadow::boot::LoadValidatedFixture(std::move(dol));
        const auto helper = shadow::boot::EnterRegisterStartup(image);
        const auto state = shadow::boot::EnterHardwareCall(image, helper);
        std::cout << std::hex << std::uppercase << std::setfill('0')
                  << "CHECKPOINT pc=0x" << std::setw(8) << helper.pc
                  << " lr=0x" << std::setw(8) << helper.lr << '\n';
        if (argc == 4) {
            const auto hardware = shadow::boot::EnterPairedSetupCall(image, state, observed_msr);
            std::cout << "CHECKPOINT pc=0x" << std::setw(8) << state.pc
                      << " lr=0x" << std::setw(8) << state.lr << '\n'
                      << "INPUT msr=0x" << std::setw(8) << observed_msr
                      << " provenance=CALLER_SUPPLIED" << '\n'
                      << "STOP pc=0x" << std::setw(8) << hardware.cpu.pc
                      << " lr=0x" << std::setw(8) << hardware.cpu.lr
                      << " r0=0x" << std::setw(8) << hardware.cpu.gpr[0]
                      << " r31=0x" << std::setw(8) << hardware.cpu.gpr[31]
                      << " msr=0x" << std::setw(8) << hardware.msr << '\n';
            return 0;
        }
        std::cout << "STOP pc=0x" << std::setw(8) << state.pc
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

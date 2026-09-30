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
        if (argc != 2 && argc != 4 && argc != 6 && argc != 8) {
            throw std::runtime_error("usage: shadow_boot_probe <main.dol> [--observed-msr <8-hex-digits> [--observed-hid2 <8-hex-digits> [--observed-hid0 <8-hex-digits>]]]");
        }
        const auto parse_hex_word = [](const char* word) {
            const std::string value = word;
            if (value.size() != 8 ||
                !std::all_of(value.begin(), value.end(), [](unsigned char c) {
                    return std::isxdigit(c) != 0;
                })) {
                throw std::runtime_error("observed word must be eight hexadecimal digits");
            }
            std::size_t parsed = 0;
            const auto result = static_cast<std::uint32_t>(std::stoul(value, &parsed, 16));
            if (parsed != 8) throw std::runtime_error("invalid observed word");
            return result;
        };
        std::uint32_t observed_msr = 0;
        std::uint32_t observed_hid2 = 0;
        std::uint32_t observed_hid0 = 0;
        if (argc >= 4) {
            if (std::string(argv[2]) != "--observed-msr") {
                throw std::runtime_error("missing --observed-msr");
            }
            observed_msr = parse_hex_word(argv[3]);
        }
        if (argc >= 6) {
            if (std::string(argv[4]) != "--observed-hid2") {
                throw std::runtime_error("missing --observed-hid2");
            }
            observed_hid2 = parse_hex_word(argv[5]);
        }
        if (argc == 8) {
            if (std::string(argv[6]) != "--observed-hid0") {
                throw std::runtime_error("missing --observed-hid0");
            }
            observed_hid0 = parse_hex_word(argv[7]);
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
        if (argc >= 4) {
            const auto hardware = shadow::boot::EnterPairedSetupCall(image, state, observed_msr);
            const auto hid2 = shadow::boot::EnterHid2ReadCall(image, hardware);
            std::cout << "CHECKPOINT pc=0x" << std::setw(8) << state.pc
                      << " lr=0x" << std::setw(8) << state.lr << '\n'
                      << "INPUT msr=0x" << std::setw(8) << observed_msr
                      << " provenance=CALLER_SUPPLIED" << '\n'
                      << "CHECKPOINT pc=0x" << std::setw(8) << hardware.cpu.pc
                      << " lr=0x" << std::setw(8) << hardware.cpu.lr
                      << " r0=0x" << std::setw(8) << hardware.cpu.gpr[0]
                      << " r31=0x" << std::setw(8) << hardware.cpu.gpr[31]
                      << " msr=0x" << std::setw(8) << hardware.msr << '\n';
            for (const auto& write : hid2.ordered_writes) {
                std::cout << "WRITE addr=0x" << std::setw(8) << write.address
                          << " value=0x" << std::setw(8) << write.value
                          << " width=4 endian=BE" << '\n';
            }
            std::cout << (argc >= 6 ? "CHECKPOINT pc=0x" : "STOP pc=0x")
                      << std::setw(8) << hid2.machine.cpu.pc
                      << " lr=0x" << std::setw(8) << hid2.machine.cpu.lr
                      << " r0=0x" << std::setw(8) << hid2.machine.cpu.gpr[0]
                      << " r1=0x" << std::setw(8) << hid2.machine.cpu.gpr[1]
                      << " r31=0x" << std::setw(8) << hid2.machine.cpu.gpr[31]
                      << " msr=0x" << std::setw(8) << hid2.machine.msr << '\n';
            if (argc >= 6) {
                const auto readback = shadow::boot::ReturnFromHid2Read(image, hid2, observed_hid2);
                const auto hid2_write = shadow::boot::IssueHid2Write(image, readback);
                std::cout << "INPUT hid2=0x" << std::setw(8) << observed_hid2
                          << " provenance=CALLER_SUPPLIED" << '\n'
                          << "CHECKPOINT pc=0x" << std::setw(8) << readback.machine.cpu.pc
                          << " lr=0x" << std::setw(8) << readback.machine.cpu.lr
                          << " r0=0x" << std::setw(8) << readback.machine.cpu.gpr[0]
                          << " r1=0x" << std::setw(8) << readback.machine.cpu.gpr[1]
                          << " r3=0x" << std::setw(8) << readback.machine.cpu.gpr[3]
                          << " msr=0x" << std::setw(8) << readback.machine.msr << '\n'
                          << "SPR_WRITE_REQUEST spr=" << std::dec << hid2_write.request.spr
                          << std::hex << " value=0x" << std::setw(8) << hid2_write.request.value
                          << '\n'
                          << (argc == 8 ? "CHECKPOINT pc=0x" : "STOP pc=0x")
                          << std::setw(8) << hid2_write.prefix.machine.cpu.pc
                          << " lr=0x" << std::setw(8) << hid2_write.prefix.machine.cpu.lr
                          << " r0=0x" << std::setw(8) << hid2_write.prefix.machine.cpu.gpr[0]
                          << " r1=0x" << std::setw(8) << hid2_write.prefix.machine.cpu.gpr[1]
                          << " r3=0x" << std::setw(8) << hid2_write.prefix.machine.cpu.gpr[3]
                          << " msr=0x" << std::setw(8) << hid2_write.prefix.machine.msr << '\n';
                if (argc == 8) {
                    const auto icfi = shadow::boot::IssueHid0IcfiRequest(image, hid2_write,
                                                                          observed_hid0);
                    std::cout << "INPUT hid0=0x" << std::setw(8) << observed_hid0
                              << " provenance=CALLER_SUPPLIED" << '\n'
                              << "SPR_WRITE_REQUEST spr=" << std::dec << icfi.request.spr
                              << std::hex << " value=0x" << std::setw(8) << icfi.request.value
                              << '\n'
                              << "STOP pc=0x" << std::setw(8) << icfi.prefix.prefix.machine.cpu.pc
                              << " lr=0x" << std::setw(8) << icfi.prefix.prefix.machine.cpu.lr
                              << " r0=0x" << std::setw(8) << icfi.prefix.prefix.machine.cpu.gpr[0]
                              << " r1=0x" << std::setw(8) << icfi.prefix.prefix.machine.cpu.gpr[1]
                              << " r3=0x" << std::setw(8) << icfi.prefix.prefix.machine.cpu.gpr[3]
                              << " msr=0x" << std::setw(8) << icfi.prefix.prefix.machine.msr << '\n';
                }
            }
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

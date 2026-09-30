#include "shadow/boot/BootFoundation.hpp"

#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Require(bool value, const char* why) {
    if (!value) throw std::runtime_error(why);
}

void MustReject(const std::function<void()>& action, const char* why) {
    try {
        action();
    } catch (const std::exception&) {
        return;
    }
    throw std::runtime_error(why);
}

void ChangeWord(std::vector<std::uint8_t>& bytes, std::size_t file_offset) {
    bytes.at(file_offset + 3) ^= 1u;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        Require(argc == 2, "usage: shadow_boot_tests <main.dol>");
        std::ifstream file(argv[1], std::ios::binary);
        Require(static_cast<bool>(file), "cannot open PAL DOL fixture");
        const std::vector<std::uint8_t> original(std::istreambuf_iterator<char>{file}, {});

        const auto image = shadow::boot::LoadValidatedFixture(original);
        const auto state = shadow::boot::EnterRegisterStartup(image);
        Require(state.pc == 0x80003158u && state.lr == state.pc,
                "entry did not stop before hardware initialization");
        for (std::size_t i = 0; i < state.gpr.size(); ++i) {
            const std::uint32_t expected = i == 1 ? 0x8060C5F0u :
                                           i == 2 ? 0x805FA780u :
                                           i == 13 ? 0x805EC500u : 0u;
            Require(state.gpr[i] == expected, "register helper result differs");
        }
        Require(image.ReadWord(0x805E4500u) == 0x804AB134u,
                "initialized data6 address mapping differs");
        // PROVEN 0x80372900 passes selector 1 to 0x80373378. The store at
        // 0x803733C0 uses 0x80586CB0 + (selector << 2), not 0x80580000.
        Require(image.ReadWord(0x80372900u) == 0x38600001u &&
                image.ReadWord(0x803733A0u) == 0x3C808058u &&
                image.ReadWord(0x803733A4u) == 0x57A513BAu &&
                image.ReadWord(0x803733A8u) == 0x38046CB0u &&
                image.ReadWord(0x803733B0u) == 0x7C802A14u &&
                image.ReadWord(0x803733C0u) == 0x93840000u &&
                0x80586CB0u + (1u << 2) == 0x80586CB4u,
                "handler-table slot evidence differs");
        MustReject([&] { (void)image.ReadWord(0x8056FE00u); },
                   "uninitialized BSS must remain unavailable");
        MustReject([&] { (void)image.ReadWord(0x80003155u); },
                   "unaligned guest word accepted");
        MustReject([&] { (void)image.ReadWord(0x805FC540u); },
                   "section-end word accepted");

        auto mutated = original;
        mutated.resize(0x100);
        MustReject([&] { (void)shadow::boot::LoadValidatedFixture(mutated); },
                   "truncated DOL accepted");

        mutated = original;
        ChangeWord(mutated, 0xE0);  // DOL entry in header.
        MustReject([&] { (void)shadow::boot::LoadValidatedFixture(mutated); },
                   "changed entry header accepted");

        mutated = original;
        ChangeWord(mutated, 0xC4);  // data6 size in DOL header.
        MustReject([&] { (void)shadow::boot::LoadValidatedFixture(mutated); },
                   "changed data6 descriptor accepted");

        mutated = original;
        ChangeWord(mutated, 0x08);  // Absent text2 offset in DOL header.
        MustReject([&] { (void)shadow::boot::LoadValidatedFixture(mutated); },
                   "unexpected text section accepted");

        mutated = original;
        ChangeWord(mutated, 0xCC);  // Absent data8 size in DOL header.
        MustReject([&] { (void)shadow::boot::LoadValidatedFixture(mutated); },
                   "unexpected data section accepted");

        mutated = original;
        ChangeWord(mutated, 0x154);  // 0x80003154 entry bl.
        MustReject([&] { (void)shadow::boot::EnterRegisterStartup(
                   shadow::boot::LoadValidatedFixture(mutated)); },
                   "changed entry instruction accepted");

        mutated = original;
        ChangeWord(mutated, 0x158);  // Next branch must remain the hardware boundary.
        MustReject([&] { (void)shadow::boot::EnterRegisterStartup(
                   shadow::boot::LoadValidatedFixture(mutated)); },
                   "changed stop-boundary instruction accepted");

        // Independently mutate every instruction at 0x800032B0..0x8000333C.
        // This catches a regression to checking only the helper's endpoints.
        for (std::size_t i = 0; i < 36; ++i) {
            mutated = original;
            ChangeWord(mutated, 0x2B0 + 4 * i);
            MustReject([&] { (void)shadow::boot::EnterRegisterStartup(
                       shadow::boot::LoadValidatedFixture(mutated)); },
                       "changed register-helper instruction accepted");
        }

        std::cout << "PAL register-startup positive and negative gates passed\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "boot regression: " << e.what() << '\n';
        return 1;
    }
}

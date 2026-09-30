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

std::uint32_t DirectBranchTarget(std::uint32_t address, std::uint32_t word) {
    Require((word >> 26) == 18u && (word & 3u) == 1u,
            "expected relative branch-and-link encoding");
    std::uint32_t displacement = word & 0x03FFFFFCu;
    if ((displacement & 0x02000000u) != 0) displacement |= 0xFC000000u;
    return address + displacement;
}

std::uint32_t DecodedSpr(std::uint32_t word) {
    return ((word >> 16) & 31u) | (((word >> 11) & 31u) << 5);
}

}  // namespace

int main(int argc, char** argv) {
    try {
        Require(argc == 2, "usage: shadow_boot_tests <main.dol>");
        std::ifstream file(argv[1], std::ios::binary);
        Require(static_cast<bool>(file), "cannot open PAL DOL fixture");
        const std::vector<std::uint8_t> original(std::istreambuf_iterator<char>{file}, {});

        const auto image = shadow::boot::LoadValidatedFixture(original);
        // Decode fields from the raw BE words independently of the native
        // transition code. The branch LI is signed and AA=0, LK=1.
        Require(DirectBranchTarget(0x80003158u, image.ReadWord(0x80003158u)) ==
                    0x80003400u &&
                DirectBranchTarget(0x80003410u, image.ReadWord(0x80003410u)) ==
                    0x80371714u,
                "raw branch fields do not encode the translated targets");
        const std::uint32_t mflr = image.ReadWord(0x8000340Cu);
        Require((mflr >> 26) == 31u && ((mflr >> 1) & 1023u) == 339u &&
                    ((mflr >> 21) & 31u) == 31u && DecodedSpr(mflr) == 8u,
                "raw mflr fields do not read LR into r31");
        const std::uint32_t hid2_read = image.ReadWord(0x80370BA8u);
        const std::uint32_t gqr0_write = image.ReadWord(0x80371738u);
        Require(DecodedSpr(hid2_read) == 920u && DecodedSpr(gqr0_write) == 912u,
                "raw SPR fields do not encode HID2/GQR0");
        const std::uint32_t hid0_command = image.ReadWord(0x803725F8u);
        Require((hid0_command >> 26) == 24u &&
                    ((hid0_command >> 21) & 31u) == 3u &&
                    ((hid0_command >> 16) & 31u) == 3u &&
                    (hid0_command & 0xFFFFu) == 0x0800u,
                "raw HID0 ICFI command fields differ");
        // HID0[ICFI] is a command that self-clears when ICE is enabled.
        // A future hardware-state lowering needs a readback regression using
        // the synthetic observed 0x0011C464, not a persistent OR model.
        const std::uint32_t rotate = image.ReadWord(0x80370CECu);
        Require((rotate >> 26) == 21u && ((rotate >> 11) & 31u) == 3u &&
                    ((rotate >> 6) & 31u) == 31u && ((rotate >> 1) & 31u) == 31u &&
                    (rotate & 1u) == 1u,
                "raw HID2 test fields do not encode rlwinm. rotate/mask");
        constexpr std::uint32_t observed_hid2 = 0xE0000000u;
        Require(((((observed_hid2 << 3) | (observed_hid2 >> 29)) & 1u) == 1u) &&
                    ((observed_hid2 << 3) & 1u) == 0u,
                "HID2 rotate/shift counterexample was lost");
        const auto state = shadow::boot::EnterRegisterStartup(image);
        Require(state.pc == 0x80003158u && state.lr == state.pc,
                "entry did not stop before hardware initialization");
        for (std::size_t i = 0; i < state.gpr.size(); ++i) {
            const std::uint32_t expected = i == 1 ? 0x8060C5F0u :
                                           i == 2 ? 0x805FA780u :
                                           i == 13 ? 0x805EC500u : 0u;
            Require(state.gpr[i] == expected, "register helper result differs");
        }
        const auto call = shadow::boot::EnterHardwareCall(image, state);
        Require(call.pc == 0x80003400u && call.lr == 0x8000315Cu,
                "direct hardware call did not reach the observed callee entry");
        Require(call.gpr == state.gpr, "direct call altered a GPR");
        const auto paired_entry = shadow::boot::EnterPairedSetupCall(image, call, 0x2032u);
        Require(paired_entry.cpu.pc == 0x80371714u &&
                    paired_entry.cpu.lr == 0x80003414u &&
                    paired_entry.cpu.gpr[0] == 0x2032u &&
                    paired_entry.cpu.gpr[31] == 0x8000315Cu &&
                    paired_entry.msr == 0x2032u,
                "synthetic reference hardware-prefix state differs");
        for (std::size_t i = 1; i < 31; ++i) {
            Require(paired_entry.cpu.gpr[i] == call.gpr[i],
                    "hardware prefix changed an unrelated GPR");
        }
        const auto fp_off = shadow::boot::EnterPairedSetupCall(image, call, 0x0032u);
        Require(fp_off.msr == 0x2032u && fp_off.cpu.gpr[0] == 0x2032u,
                "MSR FP bit was not derived from a distinct input");
        auto wrong_entry = state;
        wrong_entry.pc += 4u;
        MustReject([&] { (void)shadow::boot::EnterHardwareCall(image, wrong_entry); },
                   "hardware call accepted an incorrect source PC");
        MustReject([&] { (void)shadow::boot::EnterPairedSetupCall(image, state, 0x2032u); },
                   "hardware prefix accepted an incorrect source PC");
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
        MustReject([&] { (void)shadow::boot::EnterHardwareCall(
                   shadow::boot::LoadValidatedFixture(mutated), state); },
                   "changed hardware call instruction accepted");

        // Every translated hardware-prefix instruction is part of the gate.
        for (std::size_t i = 0; i < 5; ++i) {
            mutated = original;
            ChangeWord(mutated, 0x400 + 4 * i);
            MustReject([&] { (void)shadow::boot::EnterPairedSetupCall(
                       shadow::boot::LoadValidatedFixture(mutated), call, 0x2032u); },
                       "changed hardware-prefix instruction accepted");
        }

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

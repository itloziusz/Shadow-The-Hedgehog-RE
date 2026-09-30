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
        const auto hid2_entry = shadow::boot::EnterHid2ReadCall(image, paired_entry);
        Require(hid2_entry.machine.cpu.pc == 0x80370BA8u &&
                    hid2_entry.machine.cpu.lr == 0x80371724u &&
                    hid2_entry.machine.cpu.gpr[0] == 0x80003414u &&
                    hid2_entry.machine.cpu.gpr[1] == 0x8060C5E8u &&
                    hid2_entry.machine.cpu.gpr[31] == 0x8000315Cu &&
                    hid2_entry.machine.msr == paired_entry.msr,
                "synthetic HID2-read entry register state differs");
        Require(hid2_entry.ordered_writes[0].address == 0x8060C5F4u &&
                    hid2_entry.ordered_writes[0].value == 0x80003414u &&
                    hid2_entry.ordered_writes[1].address == 0x8060C5E8u &&
                    hid2_entry.ordered_writes[1].value == 0x8060C5F0u,
                "paired setup stack stores differ from fresh PPC capture");
        Require(hid2_entry.stack_memory.base == 0x8060C5E8u &&
                    hid2_entry.stack_memory.LoadBE32(0x8060C5E8u) == 0x8060C5F0u &&
                    hid2_entry.stack_memory.LoadBE32(0x8060C5F4u) == 0x80003414u,
                "paired setup stores were not applied as big-endian guest bytes");
        Require(hid2_entry.stack_memory.bytes[0] == 0x80u &&
                    hid2_entry.stack_memory.bytes[1] == 0x60u &&
                    hid2_entry.stack_memory.bytes[2] == 0xC5u &&
                    hid2_entry.stack_memory.bytes[3] == 0xF0u &&
                    hid2_entry.stack_memory.bytes[12] == 0x80u &&
                    hid2_entry.stack_memory.bytes[13] == 0x00u &&
                    hid2_entry.stack_memory.bytes[14] == 0x34u &&
                    hid2_entry.stack_memory.bytes[15] == 0x14u,
                "paired setup guest-byte order differs");
        MustReject([&] { (void)hid2_entry.stack_memory.LoadBE32(0x8060C5ECu); },
                   "unwritten stack bytes were treated as initialized");
        MustReject([&] { (void)hid2_entry.stack_memory.LoadBE32(0x8060C5F0u); },
                   "unwritten aligned stack word was accepted");
        MustReject([&] { (void)hid2_entry.stack_memory.LoadBE32(0x8060C5F2u); },
                   "unaligned stack word was accepted");
        MustReject([&] { (void)hid2_entry.stack_memory.LoadBE32(0x8060C5F8u); },
                   "out-of-window stack read was accepted");
        auto altered_stack = hid2_entry.stack_memory;
        altered_stack.StoreBE32(0x8060C5F4u, 0x12345678u);
        Require(altered_stack.LoadBE32(0x8060C5F4u) == 0x12345678u &&
                    hid2_entry.stack_memory.LoadBE32(0x8060C5F4u) == 0x80003414u,
                "later saved-LR read was replaced by a hard-coded return value");
        MustReject([&] { altered_stack.StoreBE32(0x8060C5F6u, 0u); },
                   "unaligned stack store was accepted");
        MustReject([&] { altered_stack.StoreBE32(0x8060C5F8u, 0u); },
                   "out-of-window stack store was accepted");
        for (std::size_t i = 2; i < 32; ++i) {
            Require(hid2_entry.machine.cpu.gpr[i] == paired_entry.cpu.gpr[i],
                    "stack prefix changed an unrelated GPR");
        }
        const auto hid2_return = shadow::boot::ReturnFromHid2Read(image, hid2_entry,
                                                                   0xE0000000u);
        Require(hid2_return.machine.cpu.pc == 0x80371724u &&
                    hid2_return.machine.cpu.lr == 0x80371724u &&
                    hid2_return.machine.cpu.gpr[3] == 0xE0000000u &&
                    hid2_return.machine.cpu.gpr[0] == 0x80003414u &&
                    hid2_return.machine.cpu.gpr[1] == 0x8060C5E8u,
                "measured HID2 accessor return differs from fresh PPC capture");
        const auto alternate_hid2 = shadow::boot::ReturnFromHid2Read(image, hid2_entry,
                                                                       0x12345678u);
        Require(alternate_hid2.machine.cpu.gpr[3] == 0x12345678u &&
                    alternate_hid2.machine.cpu.pc == hid2_return.machine.cpu.pc,
                "HID2 read was hard-coded to the synthetic reference value");
        const auto hid2_write = shadow::boot::IssueHid2Write(image, hid2_return);
        Require(hid2_write.request.spr == 920u &&
                    hid2_write.request.value == 0xE0000000u &&
                    hid2_write.prefix.machine.cpu.pc == 0x8037172Cu &&
                    hid2_write.prefix.machine.cpu.lr == 0x8037172Cu &&
                    hid2_write.prefix.machine.cpu.gpr[3] == 0xE0000000u &&
                    hid2_write.prefix.machine.cpu.gpr[1] == hid2_return.machine.cpu.gpr[1] &&
                    hid2_write.prefix.ordered_writes[0].address == hid2_return.ordered_writes[0].address &&
                    hid2_write.prefix.ordered_writes[0].value == hid2_return.ordered_writes[0].value &&
                    hid2_write.prefix.ordered_writes[1].address == hid2_return.ordered_writes[1].address &&
                    hid2_write.prefix.ordered_writes[1].value == hid2_return.ordered_writes[1].value,
                "HID2 write boundary differs from the byte-derived PPC transition");
        const auto alternate_write = shadow::boot::IssueHid2Write(image, alternate_hid2);
        Require(alternate_write.request.spr == 920u &&
                    alternate_write.request.value == 0xB2345678u &&
                    alternate_write.prefix.machine.cpu.gpr[3] == 0xB2345678u,
                "HID2 OR instruction lost unforced incoming bits");
        for (std::size_t i = 0; i < 32; ++i) {
            if (i != 3) {
                Require(hid2_write.prefix.machine.cpu.gpr[i] == hid2_return.machine.cpu.gpr[i],
                        "HID2 writer changed an unrelated GPR");
            }
        }
        const auto icfi = shadow::boot::IssueHid0IcfiRequest(image, hid2_write,
                                                               0x0011C464u);
        Require(icfi.request.spr == 1008u &&
                    icfi.request.value == 0x0011CC64u &&
                    icfi.prefix.prefix.machine.cpu.pc == 0x80371730u &&
                    icfi.prefix.prefix.machine.cpu.lr == 0x80371730u &&
                    icfi.prefix.prefix.machine.cpu.gpr[3] == 0x0011CC64u &&
                    icfi.prefix.prefix.machine.cpu.gpr[1] == 0x8060C5E8u &&
                    icfi.prefix.prefix.machine.msr == 0x2032u,
                "HID0 ICFI request differs from the synthetic PPC checkpoint");
        Require(icfi.prefix.prefix.stack_memory.LoadBE32(0x8060C5E8u) ==
                    0x8060C5F0u &&
                    icfi.prefix.prefix.stack_memory.LoadBE32(0x8060C5F4u) ==
                    0x80003414u,
                "applied stack writes were lost before the sync boundary");
        const auto tail = shadow::boot::PredictPostSyncGqrTail(image, icfi);
        Require(tail.before_saved_lr_load.cpu.pc == 0x80371758u &&
                    tail.before_saved_lr_load.cpu.gpr[0] == 0x80003414u &&
                    tail.before_saved_lr_load.cpu.gpr[1] == 0x8060C5E8u &&
                    tail.before_saved_lr_load.cpu.gpr[3] == 0u &&
                    tail.before_saved_lr_load.cpu.lr == 0x80371730u,
                "conditional GQR-tail projection differs from HLE pre-load state");
        for (std::size_t i = 0; i < tail.ordered_gqr_writes.size(); ++i) {
            Require(tail.ordered_gqr_writes[i].spr == 912u + i &&
                        tail.ordered_gqr_writes[i].value == 0u,
                    "conditional GQR-tail projection lost an ordered SPR write");
        }
        Require(tail.after_return.cpu.pc == 0x80003414u &&
                    tail.after_return.cpu.lr == 0x80003414u &&
                    tail.after_return.cpu.gpr[0] == 0x80003414u &&
                    tail.after_return.cpu.gpr[1] == 0x8060C5F0u &&
                    tail.after_return.cpu.gpr[3] == 0u,
                "conditional GQR-tail return differs from observed PPC state");
        for (std::size_t i = 2; i < 32; ++i) {
            if (i != 3) {
                Require(tail.after_return.cpu.gpr[i] == icfi.prefix.prefix.machine.cpu.gpr[i],
                        "conditional GQR tail changed an unrelated GPR");
            }
        }
        auto changed_saved_lr = icfi;
        changed_saved_lr.prefix.prefix.stack_memory.StoreBE32(0x8060C5F4u, 0x8000315Cu);
        const auto changed_return = shadow::boot::PredictPostSyncGqrTail(
            image, changed_saved_lr);
        Require(changed_return.after_return.cpu.pc == 0x8000315Cu &&
                    changed_return.after_return.cpu.gpr[0] == 0x8000315Cu,
                "GQR tail hard-coded the saved return address");
        changed_saved_lr.prefix.prefix.stack_memory.StoreBE32(0x8060C5F4u,
                                                              0x8000315Fu);
        const auto unaligned_lr = shadow::boot::PredictPostSyncGqrTail(
            image, changed_saved_lr);
        Require(unaligned_lr.after_return.cpu.gpr[0] == 0x8000315Fu &&
                    unaligned_lr.after_return.cpu.lr == 0x8000315Fu &&
                    unaligned_lr.after_return.cpu.pc == 0x8000315Cu,
                "blr failed to clear only the next-PC low bits");
        auto changed_r3 = icfi;
        changed_r3.prefix.prefix.machine.cpu.gpr[3] = 0xDEADBEEFu;
        changed_r3.request.value = 0xDEADBEEFu;
        Require(shadow::boot::PredictPostSyncGqrTail(image, changed_r3)
                    .before_saved_lr_load.cpu.gpr[3] == 0u,
                "GQR tail failed to overwrite arbitrary incoming r3");
        auto missing_saved_lr = icfi;
        missing_saved_lr.prefix.prefix.stack_memory.valid[12] = false;
        MustReject([&] { (void)shadow::boot::PredictPostSyncGqrTail(
                   image, missing_saved_lr); },
                   "GQR tail accepted an unwritten saved return byte");
        const auto alternate_icfi = shadow::boot::IssueHid0IcfiRequest(
            image, hid2_write, 0x40000000u);
        Require(alternate_icfi.request.value == 0x40000800u &&
                    alternate_icfi.prefix.prefix.machine.cpu.gpr[3] == 0x40000800u,
                "HID0 ICFI request was hard-coded to HLE state");
        for (std::size_t i = 0; i < 32; ++i) {
            if (i != 3) {
                Require(icfi.prefix.prefix.machine.cpu.gpr[i] == hid2_write.prefix.machine.cpu.gpr[i],
                        "HID0 ICFI leaf changed an unrelated GPR");
            }
        }
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
        MustReject([&] { (void)shadow::boot::EnterPairedSetupCall(image, call, 0x4000u); },
                   "hardware prefix executed supervisor mfmsr from user mode");
        MustReject([&] { (void)shadow::boot::EnterPairedSetupCall(image, call, 0x6032u); },
                   "hardware prefix accepted user mode with FP enabled");
        auto wrong_stack_entry = paired_entry;
        wrong_stack_entry.cpu.gpr[1] -= 8u;
        MustReject([&] { (void)shadow::boot::EnterHid2ReadCall(image, wrong_stack_entry); },
                   "paired stack prefix accepted an incorrect SP");
        MustReject([&] { (void)shadow::boot::ReturnFromHid2Read(image,
                   shadow::boot::PairedSetupStackPrefix{}, 0xE0000000u); },
                   "HID2 accessor accepted an incorrect entry PC");
        auto user_mode_hid2_read = hid2_entry;
        user_mode_hid2_read.machine.msr |= 0x4000u;
        MustReject([&] { (void)shadow::boot::ReturnFromHid2Read(
                   image, user_mode_hid2_read, 0xE0000000u); },
                   "HID2 accessor executed supervisor mfspr from user mode");
        MustReject([&] { (void)shadow::boot::IssueHid2Write(image, hid2_entry); },
                   "HID2 writer accepted an incorrect entry PC");
        auto user_mode_hid2 = hid2_return;
        user_mode_hid2.machine.msr |= 0x4000u;
        MustReject([&] { (void)shadow::boot::IssueHid2Write(image, user_mode_hid2); },
                   "HID2 writer accepted a user-mode SPR access");
        MustReject([&] { (void)shadow::boot::IssueHid0IcfiRequest(image,
                   shadow::boot::Hid2WriteBoundary{}, 0x0011C464u); },
                   "HID0 ICFI accepted an incorrect entry PC");
        auto user_mode_icfi = hid2_write;
        user_mode_icfi.prefix.machine.msr |= 0x4000u;
        MustReject([&] { (void)shadow::boot::IssueHid0IcfiRequest(image,
                   user_mode_icfi, 0x0011C464u); },
                   "HID0 ICFI accepted a user-mode SPR access");
        auto wrong_tail_entry = icfi;
        wrong_tail_entry.prefix.prefix.machine.cpu.pc += 4u;
        MustReject([&] { (void)shadow::boot::PredictPostSyncGqrTail(
                   image, wrong_tail_entry); },
                   "GQR-tail projection accepted an incorrect source PC");
        auto altered_icfi_request = icfi;
        altered_icfi_request.request.value ^= 1u;
        MustReject([&] { (void)shadow::boot::PredictPostSyncGqrTail(
                   image, altered_icfi_request); },
                   "GQR-tail projection accepted a mismatched ICFI operand");
        altered_icfi_request = icfi;
        altered_icfi_request.request.value &= ~0x800u;
        altered_icfi_request.prefix.prefix.machine.cpu.gpr[3] =
            altered_icfi_request.request.value;
        MustReject([&] { (void)shadow::boot::PredictPostSyncGqrTail(
                   image, altered_icfi_request); },
                   "GQR-tail projection accepted a missing ICFI command bit");
        auto user_mode_tail = icfi;
        user_mode_tail.prefix.prefix.machine.msr |= 0x4000u;
        MustReject([&] { (void)shadow::boot::PredictPostSyncGqrTail(
                   image, user_mode_tail); },
                   "GQR-tail projection accepted user-mode GQR writes");
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
        for (std::size_t i = 0; i < 4; ++i) {
            mutated = original;
            ChangeWord(mutated, 0x36AFD4 + 4 * i);
            MustReject([&] { (void)shadow::boot::EnterHid2ReadCall(
                       shadow::boot::LoadValidatedFixture(mutated), paired_entry); },
                       "changed paired stack-prefix instruction accepted");
        }
        for (std::size_t i = 0; i < 2; ++i) {
            mutated = original;
            ChangeWord(mutated, 0x36A468 + 4 * i);
            MustReject([&] { (void)shadow::boot::ReturnFromHid2Read(
                       shadow::boot::LoadValidatedFixture(mutated), hid2_entry,
                       0xE0000000u); },
                       "changed HID2 read accessor word accepted");
        }
        for (const std::size_t offset : {0x36AFE4u, 0x36AFE8u,
                                         0x36A470u, 0x36A474u}) {
            mutated = original;
            ChangeWord(mutated, offset);
            MustReject([&] { (void)shadow::boot::IssueHid2Write(
                       shadow::boot::LoadValidatedFixture(mutated), hid2_return); },
                       "changed HID2 write word accepted");
        }
        for (const std::size_t offset : {0x36AFECu, 0x36BEB4u, 0x36BEB8u,
                                         0x36BEBCu, 0x36BEC0u, 0x36AFF0u}) {
            mutated = original;
            ChangeWord(mutated, offset);
            MustReject([&] { (void)shadow::boot::IssueHid0IcfiRequest(
                       shadow::boot::LoadValidatedFixture(mutated), hid2_write,
                       0x0011C464u); },
                       "changed HID0 ICFI word accepted");
        }
        for (std::size_t i = 0; i < 13; ++i) {
            mutated = original;
            ChangeWord(mutated, 0x36AFF4u + 4u * i);
            MustReject([&] { (void)shadow::boot::PredictPostSyncGqrTail(
                       shadow::boot::LoadValidatedFixture(mutated), icfi); },
                       "changed post-sync GQR-tail word accepted");
        }
        mutated = original;
        ChangeWord(mutated, 0x36AFF0u); // sync predecessor itself.
        MustReject([&] { (void)shadow::boot::PredictPostSyncGqrTail(
                   shadow::boot::LoadValidatedFixture(mutated), icfi); },
                   "changed sync predecessor accepted by tail projection");

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

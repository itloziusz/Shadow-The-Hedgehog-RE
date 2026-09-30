#include "shadow/boot/BootFoundation.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>

namespace shadow::boot {
namespace {

constexpr std::uint32_t kEntry = 0x80003154u;
constexpr std::uint32_t kRegisterHelper = 0x800032B0u;
constexpr std::uint32_t kStopBeforeHardware = 0x80003158u;
constexpr std::size_t kDolSize = 0x5816E0u;

struct Section {
    bool text;
    std::uint32_t index;
    std::uint32_t file_offset;
    std::uint32_t address;
    std::uint32_t size;
};

// PROVEN: DOL header and the checked map in MINIMAL_BOOT_FOUNDATION.md.
constexpr std::array<Section, 10> kSections{{
    {true,  0, 0x00000100u, 0x80003100u, 0x00002500u},
    {true,  1, 0x00002600u, 0x80008D40u, 0x004A1F20u},
    {false, 0, 0x004A4520u, 0x80005600u, 0x00001F20u},
    {false, 1, 0x004A6440u, 0x80007520u, 0x00001820u},
    {false, 2, 0x004A7C60u, 0x804AAC60u, 0x00000480u},
    {false, 3, 0x004A80E0u, 0x804AB0E0u, 0x00000020u},
    {false, 4, 0x004A8100u, 0x804AB100u, 0x00072420u},
    {false, 5, 0x0051A520u, 0x8051D520u, 0x000528E0u},
    {false, 6, 0x0056CE00u, 0x805E4500u, 0x0000AB20u},
    {false, 7, 0x00577920u, 0x805F2780u, 0x00009DC0u},
}};

// PROVEN: every word of the register helper, checked against main.dol and
// the independent address/word table in experimental_native_boot. Checking
// only its first/last word would permit a changed middle instruction to pass.
constexpr std::array<std::uint32_t, 36> kRegisterHelperWords{{
    0x38000000u, 0x38600000u, 0x38800000u, 0x38A00000u,
    0x38C00000u, 0x38E00000u, 0x39000000u, 0x39200000u,
    0x39400000u, 0x39600000u, 0x39800000u, 0x39C00000u,
    0x39E00000u, 0x3A000000u, 0x3A200000u, 0x3A400000u,
    0x3A600000u, 0x3A800000u, 0x3AA00000u, 0x3AC00000u,
    0x3AE00000u, 0x3B000000u, 0x3B200000u, 0x3B400000u,
    0x3B600000u, 0x3B800000u, 0x3BA00000u, 0x3BC00000u,
    0x3BE00000u, 0x3C208060u, 0x6021C5F0u, 0x3C40805Fu,
    0x6042A780u, 0x3DA0805Eu, 0x61ADC500u, 0x4E800020u,
}};

std::uint32_t ReadBE32(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 4) {
        throw std::runtime_error("read outside DOL fixture");
    }
    return (std::uint32_t(bytes[offset]) << 24) |
           (std::uint32_t(bytes[offset + 1]) << 16) |
           (std::uint32_t(bytes[offset + 2]) << 8) |
            std::uint32_t(bytes[offset + 3]);
}

}  // namespace

BootImage::BootImage(std::vector<std::uint8_t> dol) : dol_(std::move(dol)) {}

std::uint32_t BootImage::ReadWord(std::uint32_t address) const {
    if ((address & 3u) != 0u) throw std::runtime_error("unaligned guest word");
    for (const Section& s : kSections) {
        if (address >= s.address &&
            std::uint64_t(address) + 4 <= std::uint64_t(s.address) + s.size) {
            return ReadBE32(dol_, s.file_offset + (address - s.address));
        }
    }
    throw std::runtime_error("guest address is not in a known DOL section");
}

BootImage LoadValidatedFixture(std::vector<std::uint8_t> dol) {
    if (dol.size() != kDolSize || ReadBE32(dol, 0xE0) != kEntry ||
        ReadBE32(dol, 0xD8) != 0x8056FE00u ||
        ReadBE32(dol, 0xDC) != 0x0008C7ECu) {
        throw std::runtime_error("DOL size, entry, or BSS header mismatch");
    }
    for (const Section& s : kSections) {
        const std::size_t i = s.index;
        const std::size_t offset_field = (s.text ? 0x00u : 0x1Cu) + i * 4u;
        const std::size_t address_field = (s.text ? 0x48u : 0x64u) + i * 4u;
        const std::size_t size_field = (s.text ? 0x90u : 0xACu) + i * 4u;
        if (ReadBE32(dol, offset_field) != s.file_offset ||
            ReadBE32(dol, address_field) != s.address ||
            ReadBE32(dol, size_field) != s.size ||
            std::uint64_t(s.file_offset) + s.size > dol.size() ||
            s.address < 0x80000000u ||
            std::uint64_t(s.address) + s.size > 0x81800000ull) {
            throw std::runtime_error("DOL section does not match BootManifest");
        }
    }
    // PROVEN for this PAL header: unused text slots 2..6 and data slots
    // 8..10 have zero offset, address and size. An extra nonempty slot must
    // not slip through the known-section view.
    for (std::size_t i = 2; i < 7; ++i) {
        if (ReadBE32(dol, i * 4) != 0 ||
            ReadBE32(dol, 0x48 + i * 4) != 0 ||
            ReadBE32(dol, 0x90 + i * 4) != 0) {
            throw std::runtime_error("unexpected DOL text section");
        }
    }
    for (std::size_t i = 8; i < 11; ++i) {
        if (ReadBE32(dol, 0x1C + i * 4) != 0 ||
            ReadBE32(dol, 0x64 + i * 4) != 0 ||
            ReadBE32(dol, 0xAC + i * 4) != 0) {
            throw std::runtime_error("unexpected DOL data section");
        }
    }
    return BootImage(std::move(dol));
}

void PairedSetupStackMemory::StoreBE32(std::uint32_t address, std::uint32_t value) {
    if ((address & 3u) != 0u) {
        throw std::runtime_error("unaligned paired setup stack store");
    }
    if (address < base || std::uint64_t(address) + 4 > std::uint64_t(base) + bytes.size()) {
        throw std::runtime_error("paired setup stack store outside modeled window");
    }
    const std::size_t offset = address - base;
    for (std::size_t i = 0; i < 4; ++i) {
        bytes[offset + i] = static_cast<std::uint8_t>(value >> (24u - 8u * i));
        valid[offset + i] = true;
    }
}

std::uint32_t PairedSetupStackMemory::LoadBE32(std::uint32_t address) const {
    if ((address & 3u) != 0u) {
        throw std::runtime_error("unaligned paired setup stack load");
    }
    if (address < base || std::uint64_t(address) + 4 > std::uint64_t(base) + bytes.size()) {
        throw std::runtime_error("paired setup stack load outside modeled window");
    }
    const std::size_t offset = address - base;
    std::uint32_t result = 0;
    for (std::size_t i = 0; i < 4; ++i) {
        if (!valid[offset + i]) {
            throw std::runtime_error("paired setup stack load from unwritten byte");
        }
        result = (result << 8) | bytes[offset + i];
    }
    return result;
}

StartupState EnterRegisterStartup(const BootImage& image) {
    // 0x80003154 bl 0x800032B0; return target at 0x80003158 is
    // 0x80003400 (hardware/runtime helper), deliberately not entered.
    if (image.ReadWord(kEntry) != 0x4800015Du ||
        image.ReadWord(kStopBeforeHardware) != 0x480002A9u) {
        throw std::runtime_error("entry instruction fingerprint mismatch");
    }
    for (std::size_t i = 0; i < kRegisterHelperWords.size(); ++i) {
        if (image.ReadWord(kRegisterHelper + static_cast<std::uint32_t>(4 * i)) !=
            kRegisterHelperWords[i]) {
            throw std::runtime_error("register helper instruction fingerprint mismatch");
        }
    }

    StartupState state;
    // Direct semantics of all 36 pinned helper instructions. No memory read,
    // FPR/CR or hardware state is claimed at this stop point.
    state.gpr.fill(0);
    state.gpr[1] = 0x8060C5F0u;
    state.gpr[2] = 0x805FA780u;
    state.gpr[13] = 0x805EC500u;
    state.lr = kStopBeforeHardware;
    state.pc = state.lr;
    return state;
}

StartupState EnterHardwareCall(const BootImage& image, const StartupState& state) {
    if (state.pc != kStopBeforeHardware ||
        image.ReadWord(kStopBeforeHardware) != 0x480002A9u) {
        throw std::runtime_error("hardware-call entry or instruction mismatch");
    }
    StartupState next = state;
    // 0x80003158: bl 0x80003400. This direct PPC branch updates LR and PC;
    // it does not read memory or change GPRs, CR, XER, CTR, FPRs, or SPRs.
    next.lr = kStopBeforeHardware + 4u;
    next.pc = 0x80003400u;
    return next;
}

HardwareCallPrefix EnterPairedSetupCall(const BootImage& image,
                                        const StartupState& state,
                                        std::uint32_t observed_msr) {
    if (state.pc != 0x80003400u || state.lr != 0x8000315Cu) {
        throw std::runtime_error("hardware helper entry state mismatch");
    }
    // The first instruction is supervisor-only mfmsr. A user-mode entry
    // takes a program exception before any of this prefix can execute.
    if ((observed_msr & 0x4000u) != 0u) {
        throw std::runtime_error("hardware helper mfmsr requires supervisor state");
    }
    constexpr std::array<std::uint32_t, 5> words{{
        0x7C0000A6u, // mfmsr r0
        0x60002000u, // ori r0,r0,0x2000
        0x7C000124u, // mtmsr r0
        0x7FE802A6u, // mflr r31
        0x4836E305u, // bl 0x80371714
    }};
    for (std::size_t i = 0; i < words.size(); ++i) {
        if (image.ReadWord(0x80003400u + static_cast<std::uint32_t>(i * 4)) != words[i]) {
            throw std::runtime_error("hardware helper prefix instruction mismatch");
        }
    }
    HardwareCallPrefix next{state, observed_msr | 0x2000u};
    next.cpu.gpr[0] = next.msr;
    next.cpu.gpr[31] = state.lr;
    next.cpu.lr = 0x80003414u;
    next.cpu.pc = 0x80371714u;
    return next;
}

PairedSetupStackPrefix EnterHid2ReadCall(const BootImage& image,
                                         const HardwareCallPrefix& state) {
    if (state.cpu.pc != 0x80371714u || state.cpu.lr != 0x80003414u ||
        state.cpu.gpr[1] != 0x8060C5F0u) {
        throw std::runtime_error("paired setup stack entry state mismatch");
    }
    constexpr std::array<std::uint32_t, 4> words{{
        0x7C0802A6u, // mflr r0
        0x90010004u, // stw r0,4(r1)
        0x9421FFF8u, // stwu r1,-8(r1)
        0x4BFFF489u, // bl 0x80370BA8
    }};
    for (std::size_t i = 0; i < words.size(); ++i) {
        if (image.ReadWord(0x80371714u + static_cast<std::uint32_t>(i * 4)) != words[i]) {
            throw std::runtime_error("paired setup stack instruction mismatch");
        }
    }
    PairedSetupStackPrefix next{state, {}};
    const std::uint32_t old_sp = state.cpu.gpr[1];
    next.machine.cpu.gpr[0] = state.cpu.lr;
    next.ordered_writes[0] = {old_sp + 4u, next.machine.cpu.gpr[0]};
    next.ordered_writes[1] = {old_sp - 8u, old_sp};
    next.stack_memory.base = old_sp - 8u;
    for (const MemoryWrite32& write : next.ordered_writes) {
        next.stack_memory.StoreBE32(write.address, write.value);
    }
    next.machine.cpu.gpr[1] = old_sp - 8u;
    next.machine.cpu.lr = 0x80371724u;
    next.machine.cpu.pc = 0x80370BA8u;
    return next;
}

PairedSetupStackPrefix ReturnFromHid2Read(const BootImage& image,
                                          const PairedSetupStackPrefix& state,
                                          std::uint32_t measured_hid2) {
    if (state.machine.cpu.pc != 0x80370BA8u ||
        state.machine.cpu.lr != 0x80371724u) {
        throw std::runtime_error("HID2 read accessor entry state mismatch");
    }
    if ((state.machine.msr & 0x4000u) != 0u) {
        throw std::runtime_error("HID2 read requires supervisor state");
    }
    if (image.ReadWord(0x80370BA8u) != 0x7C78E2A6u ||
        image.ReadWord(0x80370BACu) != 0x4E800020u) {
        throw std::runtime_error("HID2 read accessor instruction mismatch");
    }
    PairedSetupStackPrefix next = state;
    next.machine.cpu.gpr[3] = measured_hid2;
    next.machine.cpu.pc = state.machine.cpu.lr;
    return next;
}

Hid2WriteBoundary IssueHid2Write(const BootImage& image,
                                 const PairedSetupStackPrefix& state) {
    if (state.machine.cpu.pc != 0x80371724u ||
        state.machine.cpu.lr != 0x80371724u) {
        throw std::runtime_error("HID2 write entry state mismatch");
    }
    if ((state.machine.msr & 0x4000u) != 0u) {
        throw std::runtime_error("HID2 write requires supervisor state");
    }
    if (image.ReadWord(0x80371724u) != 0x6463A000u ||
        image.ReadWord(0x80371728u) != 0x4BFFF489u ||
        image.ReadWord(0x80370BB0u) != 0x7C78E3A6u ||
        image.ReadWord(0x80370BB4u) != 0x4E800020u) {
        throw std::runtime_error("HID2 write instruction fingerprint mismatch");
    }

    Hid2WriteBoundary next{state, {920u, state.machine.cpu.gpr[3] | 0xA0000000u}};
    // oris r3,r3,0xA000; bl writer; mtspr HID2,r3; blr. The command's
    // observable future readback is hardware-dependent and remains unknown.
    next.prefix.machine.cpu.gpr[3] = next.request.value;
    next.prefix.machine.cpu.lr = 0x8037172Cu;
    next.prefix.machine.cpu.pc = next.prefix.machine.cpu.lr;
    return next;
}

Hid0IcfiBoundary IssueHid0IcfiRequest(const BootImage& image,
                                      const Hid2WriteBoundary& state,
                                      std::uint32_t measured_hid0) {
    const auto& cpu = state.prefix.machine.cpu;
    if (cpu.pc != 0x8037172Cu || cpu.lr != 0x8037172Cu) {
        throw std::runtime_error("HID0 ICFI entry state mismatch");
    }
    if ((state.prefix.machine.msr & 0x4000u) != 0u) {
        throw std::runtime_error("HID0 ICFI requires supervisor state");
    }
    constexpr std::array<std::uint32_t, 6> words{{
        0x48000EC9u, // 0x8037172C: bl 0x803725F4
        0x7C70FAA6u, // 0x803725F4: mfspr r3,HID0
        0x60630800u, // 0x803725F8: ori r3,r3,0x800
        0x7C70FBA6u, // 0x803725FC: mtspr HID0,r3
        0x4E800020u, // 0x80372600: blr
        0x7C0004ACu, // 0x80371730: sync (next unexecuted boundary)
    }};
    constexpr std::array<std::uint32_t, 6> addresses{{
        0x8037172Cu, 0x803725F4u, 0x803725F8u,
        0x803725FCu, 0x80372600u, 0x80371730u,
    }};
    for (std::size_t i = 0; i < words.size(); ++i) {
        if (image.ReadWord(addresses[i]) != words[i]) {
            throw std::runtime_error("HID0 ICFI instruction fingerprint mismatch");
        }
    }

    Hid0IcfiBoundary next{state, {1008u, measured_hid0 | 0x00000800u}};
    next.prefix.prefix.machine.cpu.gpr[3] = next.request.value;
    next.prefix.prefix.machine.cpu.lr = 0x80371730u;
    next.prefix.prefix.machine.cpu.pc = next.prefix.prefix.machine.cpu.lr;
    return next;
}

PostSyncGqrTailPrediction PredictPostSyncGqrTail(const BootImage& image,
                                                const Hid0IcfiBoundary& state) {
    const auto& incoming = state.prefix.prefix.machine;
    if (incoming.cpu.pc != 0x80371730u || incoming.cpu.lr != 0x80371730u) {
        throw std::runtime_error("post-sync GQR tail entry state mismatch");
    }
    if (state.request.spr != 1008u ||
        (state.request.value & 0x800u) == 0u ||
        state.request.value != incoming.cpu.gpr[3] ||
        image.ReadWord(0x80371730u) != 0x7C0004ACu) {
        throw std::runtime_error("post-sync GQR tail has no matching ICFI/sync predecessor");
    }
    if ((incoming.msr & 0x4000u) != 0u) {
        throw std::runtime_error("post-sync GQR writes require supervisor state");
    }
    // The sync itself remains unexecuted. Check every projected word from
    // the original PAL DOL before predicting anything beyond the barrier.
    constexpr std::array<std::uint32_t, 13> words{{
        0x38600000u, // li r3,0
        0x7C70E3A6u, 0x7C71E3A6u, 0x7C72E3A6u, 0x7C73E3A6u,
        0x7C74E3A6u, 0x7C75E3A6u, 0x7C76E3A6u, 0x7C77E3A6u,
        0x8001000Cu, // lwz r0,12(r1)
        0x38210008u, // addi r1,r1,8
        0x7C0803A6u, // mtlr r0
        0x4E800020u, // blr
    }};
    for (std::size_t i = 0; i < words.size(); ++i) {
        if (image.ReadWord(0x80371734u + static_cast<std::uint32_t>(4u * i)) != words[i]) {
            throw std::runtime_error("post-sync GQR tail instruction fingerprint mismatch");
        }
    }

    PostSyncGqrTailPrediction prediction{};
    prediction.before_saved_lr_load = incoming;
    prediction.before_saved_lr_load.cpu.gpr[3] = 0u;
    prediction.before_saved_lr_load.cpu.pc = 0x80371758u;
    for (std::size_t i = 0; i < prediction.ordered_gqr_writes.size(); ++i) {
        prediction.ordered_gqr_writes[i] = {static_cast<std::uint32_t>(912u + i), 0u};
    }

    prediction.after_return = prediction.before_saved_lr_load;
    const auto sp = prediction.after_return.cpu.gpr[1];
    const std::uint64_t saved_lr_address = std::uint64_t(sp) + 12u;
    if (saved_lr_address > 0xFFFFFFFFull) {
        throw std::runtime_error("post-sync saved LR address overflow");
    }
    const auto restored_lr = state.prefix.prefix.stack_memory.LoadBE32(
        static_cast<std::uint32_t>(saved_lr_address));
    prediction.after_return.cpu.gpr[0] = restored_lr;
    prediction.after_return.cpu.gpr[1] = sp + 8u;
    prediction.after_return.cpu.lr = restored_lr;
    // bclr/blr forms NIA from LR[0:29] || 0b00; LR retains the full word.
    prediction.after_return.cpu.pc = restored_lr & ~3u;
    return prediction;
}

}  // namespace shadow::boot

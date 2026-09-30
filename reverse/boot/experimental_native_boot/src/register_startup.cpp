#include "register_startup.h"

namespace {

void Pin(const BootImage& image, const InstructionPin& pin) {
    const GuestWord32 word = image.ReadWord(pin.address);
    if (word != pin.word) {
        throw BootError("instruction fingerprint mismatch");
    }
}

}  // namespace

StartupState EnterRegisterStartup(const BootImage& image) {
    for (const InstructionPin& pin : NativeBootManifest::entry_pins) {
        Pin(image, pin);
    }
    for (const InstructionPin& pin : NativeBootManifest::register_helper_pins) {
        Pin(image, pin);
    }

    StartupState state;
    // Direct semantic translation of the pinned helper. The helper writes these
    // three bases with lis/ori and clears every other GPR. It does not read memory.
    state.gpr.fill(0);
    state.gpr[1] = NativeBootManifest::stack_base;
    state.gpr[2] = NativeBootManifest::sdata2_base;
    state.gpr[13] = NativeBootManifest::sdata_base;
    state.lr = NativeBootManifest::stop_before_hardware;
    state.pc = state.lr;

    if (state.pc != NativeBootManifest::stop_before_hardware ||
        state.gpr[1] != 0x8060C5F0u || state.gpr[2] != 0x805FA780u ||
        state.gpr[13] != 0x805EC500u) {
        throw BootError("register startup did not reach the proven stop state");
    }
    for (std::size_t index = 0; index < state.gpr.size(); ++index) {
        if (index == 1 || index == 2 || index == 13) {
            continue;
        }
        if (state.gpr[index] != 0u) {
            throw BootError("register helper left an unexpected GPR");
        }
    }
    return state;
}

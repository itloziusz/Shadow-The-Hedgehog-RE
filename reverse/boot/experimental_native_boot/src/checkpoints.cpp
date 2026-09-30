#include "checkpoints.h"

#include "boot_image.h"

#include <iomanip>
#include <iostream>
#include <string>

std::string_view CheckpointName(Checkpoint checkpoint) {
    switch (checkpoint) {
    case Checkpoint::BootInputValidated:
        return "BOOT_INPUT_VALIDATED";
    case Checkpoint::DolEntryReached:
        return "DOL_ENTRY_REACHED";
    case Checkpoint::RegisterInitComplete:
        return "REGISTER_INIT_COMPLETE";
    case Checkpoint::HardwareSemanticsComplete:
        return "HARDWARE_SEMANTICS_COMPLETE";
    case Checkpoint::CrtInitComplete:
        return "CRT_INIT_COMPLETE";
    case Checkpoint::OsInitEntered:
        return "OS_INIT_ENTERED";
    case Checkpoint::OsInitComplete:
        return "OS_INIT_COMPLETE";
    case Checkpoint::Constructor0Complete:
        return "CONSTRUCTOR_0_COMPLETE";
    case Checkpoint::Constructor1Complete:
        return "CONSTRUCTOR_1_COMPLETE";
    case Checkpoint::Constructor2Complete:
        return "CONSTRUCTOR_2_COMPLETE";
    case Checkpoint::Constructor3Complete:
        return "CONSTRUCTOR_3_COMPLETE";
    case Checkpoint::Constructor4Complete:
        return "CONSTRUCTOR_4_COMPLETE";
    case Checkpoint::Constructor5Complete:
        return "CONSTRUCTOR_5_COMPLETE";
    case Checkpoint::Constructor6Complete:
        return "CONSTRUCTOR_6_COMPLETE";
    case Checkpoint::Constructor7Complete:
        return "CONSTRUCTOR_7_COMPLETE";
    case Checkpoint::Constructor8Complete:
        return "CONSTRUCTOR_8_COMPLETE";
    case Checkpoint::ConstructorsComplete:
        return "CONSTRUCTORS_COMPLETE";
    case Checkpoint::ApplicationMainReached:
        return "APPLICATION_MAIN_REACHED";
    case Checkpoint::GameInitComplete:
        return "GAME_INIT_COMPLETE";
    case Checkpoint::GameplayGateReached:
        return "GAMEPLAY_GATE_REACHED";
    }
    return "UNKNOWN_CHECKPOINT";
}

void LogReached(Checkpoint checkpoint, const CheckpointFacts& facts) {
    if (checkpoint != Checkpoint::BootInputValidated &&
        checkpoint != Checkpoint::DolEntryReached &&
        checkpoint != Checkpoint::RegisterInitComplete &&
        checkpoint != Checkpoint::HardwareSemanticsComplete &&
        checkpoint != Checkpoint::CrtInitComplete &&
        checkpoint != Checkpoint::OsInitEntered &&
        checkpoint != Checkpoint::OsInitComplete &&
        checkpoint != Checkpoint::Constructor0Complete &&
        checkpoint != Checkpoint::Constructor1Complete &&
        checkpoint != Checkpoint::Constructor2Complete &&
        checkpoint != Checkpoint::Constructor3Complete &&
        checkpoint != Checkpoint::Constructor4Complete &&
        checkpoint != Checkpoint::Constructor5Complete &&
        checkpoint != Checkpoint::Constructor6Complete &&
        checkpoint != Checkpoint::Constructor7Complete &&
        checkpoint != Checkpoint::Constructor8Complete) {
        throw BootError(std::string(CheckpointName(checkpoint)) +
                        " is not a proven checkpoint in this executable");
    }
    std::cout << "CHECKPOINT " << CheckpointName(checkpoint) << '\n'
              << "  guest_pc=0x" << std::hex << std::uppercase << std::setfill('0') << std::setw(8)
              << facts.guest_pc << std::dec << '\n'
              << "  native=" << facts.native_state << '\n'
              << "  guest_globals=" << facts.guest_globals << '\n'
              << "  unresolved=" << facts.unresolved << '\n'
              << "  comparison=" << facts.comparison << std::setfill(' ') << std::dec << '\n';
}

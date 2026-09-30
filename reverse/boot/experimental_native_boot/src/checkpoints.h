#pragma once

#include "guest_types.h"

#include <array>
#include <string_view>

enum class Checkpoint {
    BootInputValidated,
    DolEntryReached,
    RegisterInitComplete,
    // Named so later milestones have a stable vocabulary.
    // LogReached rejects every value below until that milestone is actually proven.
    HardwareSemanticsComplete,
    CrtInitComplete,
    OsInitEntered,
    OsInitComplete,
    Constructor0Complete,
    Constructor1Complete,
    Constructor2Complete,
    Constructor3Complete,
    Constructor4Complete,
    Constructor5Complete,
    Constructor6Complete,
    Constructor7Complete,
    Constructor8Complete,
    ConstructorsComplete,
    ApplicationMainReached,
    GameInitComplete,
    GameplayGateReached,
};

struct CheckpointFacts {
    GuestAddress32 guest_pc = 0;
    std::string_view native_state;
    std::string_view guest_globals;
    std::string_view unresolved;
    std::string_view comparison;
};

std::string_view CheckpointName(Checkpoint checkpoint);
void LogReached(Checkpoint checkpoint, const CheckpointFacts& facts);

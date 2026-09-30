// Names for the StageAction ids (dispatch table 0x8052CC68; classes are original RTTI names).
#include "shadow/gameplay/StageFlow.hpp"

namespace shadow::gameplay {

const char* StageActionName(StageAction a) {
    static const char* const k[] = {"None", "(no action object)", "InitAction", "PlayAction", "EventAction",
        "PauseAction", "GoalAction", "RestartAction", "ContinueAction", "DeadAction", "SaveAction",
        "TryAgainAction", "ShowAction", "FlashbackAction", "Result2PAction", "TimeupAction", "EndAction"};
    const int i = (int)a;
    return (i >= 0 && i <= 16) ? k[i] : "?";
}

}  // namespace shadow::gameplay

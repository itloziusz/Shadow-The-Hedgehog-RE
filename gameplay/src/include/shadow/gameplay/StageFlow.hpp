// Native reconstruction of StageState request/dispatch (Shadow the Hedgehog GC).
// Evidence: MISSION_SYSTEM.md §6.2; StageState_RequestAction 0x801762D0 and
// StageState_DispatchAction 0x80174974 (read by the integration pass). Singleton .bss 0x80577038.
#pragma once
#include <array>
#include <cstdint>

namespace shadow::gameplay {

// Dispatch jump table 0x8052CC68 -> original StageAction::* classes (RTTI names)
enum class StageAction : int32_t {
    None = 0, Idle = 1 /* no action object */, Init = 2, Play = 3, Event = 4, Pause = 5, Goal = 6,
    Restart = 7, Continue = 8, Dead = 9, Save = 10, TryAgain = 11, Show = 12, Flashback = 13,
    Result2P = 14, Timeup = 15, End = 16,
};
const char* StageActionName(StageAction a);

class StageState {
public:
    // 0x801762D0: ignored if disabled[req] (+0x17+req) or, unless force, if locked (+0x14).
    void RequestAction(StageAction req, bool force = false) {
        const int r = (int)req;
        if (r >= 0 && r < (int)disabled.size() && disabled[r]) return;
        if (locked && !force) return;
        request_ = req;                                   // +0x0C
    }
    // 0x80174974: returns true if a new action became current. Creating/destroying the
    // StageAction object is engine-side (fn_80176254 + per-case ctors).
    bool Dispatch() {
        if (request_ == StageAction::None || request_ == current_) return false;
        if ((int)request_ <= 0x10) transient = {0, 0};    // memset(+0x15, 0, 2) in every case
        current_ = request_;                              // +0x08 = +0x0C
        if (!disabled[0]) request_ = StageAction::None;   // literal: tests byte +0x17 (index 0)
        return true;
    }
    StageAction current() const { return current_; }
    StageAction pending() const { return request_; }
    bool locked = false;                                  // +0x14
    std::array<uint8_t, 2> transient{};                   // +0x15..+0x16 (meaning UNKNOWN)
    std::array<uint8_t, 0x11> disabled{};                 // +0x17 + action id
private:
    StageAction current_ = StageAction::None;             // +0x08
    StageAction request_ = StageAction::None;             // +0x0C
};

}  // namespace shadow::gameplay

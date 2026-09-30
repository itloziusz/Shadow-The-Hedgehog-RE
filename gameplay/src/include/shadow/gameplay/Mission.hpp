// Native reconstruction of Shadow the Hedgehog's (GC) mission framework.
// Evidence: gameplay/MISSION_SYSTEM.md, gameplay/MISSION_PARAMETER_MAP.md.
// Class names mirror the original RTTI names (Mission::*); method names are recovered semantics.
#pragma once
#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace shadow::gameplay {

class SetData;

// ---- Stage table (.rodata 0x804C5AE8, 59 x 0x50) ---------------------------------------------
struct StageRoute { int32_t nextIndex; int32_t missionKey; int32_t unknownId; };  // +0x1C+0xC*slot
struct StageTableEntry {
    int32_t     id;          // +0x00
    const char* name;        // +0x04 nukkoro section name ("City1")
    uint32_t    flags;       // +0x08 bit0 boss
    StageRoute  route[3];    // slot 0 Dark, 1 Normal, 2 Hero
    int32_t     hardKey;     // +0x40 hard-mode mission key (slot 1)
};
const std::vector<StageTableEntry>& OriginalStageTable();   // generated
int StageTable_IndexOfId(int stageId);                      // fn_80176FE8

// ---- Mission descriptors (MissionManager +0x30 std::map) ------------------------------------
enum class MissionType : int32_t { Base = 0, Goal = 1, Count = 2, Enemy = 3, Timer = 4,
                                   TimerGoal = 5, TimerCount = 6, Ring = 7 };
struct MissionDesc { MissionType type; int32_t param; int32_t count; int32_t failCount; };
std::map<int, MissionDesc> DefaultMissionDescs();            // fn_8016CB48 / fn_8016C8EC
// TGameDefaultParam::ParseLine (fn_801E030C) subset: MISSIONCOUNT_{D,N,H,HARD} : A B
int ApplyMissionCountFile(const std::string& path, std::map<int, MissionDesc>& descs);

// ---- Enemy census / defeat counters (EnemyManager, .bss 0x80580BD8) --------------------------
enum EnemyTeam : int { TeamGun = 0, TeamEggman = 1, TeamBlackArms = 2 };
struct EnemyManager {
    int total = 0;                         // +0x24
    std::array<int, 3> placed{};           // +0x28
    std::array<int, 3> defeated{};         // +0x34  (EnemyMission source)
    std::array<int, 3> defeatedByPlayer{}; // +0x40
    std::map<uint8_t, int> groupAlive;     // +0x10  link id -> alive count
    std::array<int, 3> cpDefeated{}, cpByPlayer{};  // +0x4C/+0x58 checkpoint copies

    void ScanSetEnemies(SetData& set);     // fn_801A2BA0 (via fn_801A2CFC)
    static int TeamOfSetId(uint16_t id);   // census rule (0x64..0x95)
    // fn_801A250C: flags bit0 death, bit1 revive, bit2 by player
    void OnEnemyLifeEvent(uint8_t linkId, int team, uint32_t flags);
    void CountDefeat(int team, bool byPlayer);                 // fn_801A2930
    void SaveCheckpoint() { cpDefeated = defeated; cpByPlayer = defeatedByPlayer; }     // fn_801A27B0
    void RestoreCheckpoint() { defeated = cpDefeated; defeatedByPlayer = cpByPlayer; }  // fn_801A26B8
};

// ---- Stage action requests (StageState::RequestAction fn_801762D0) --------------------------
enum StageActionRequest : int { ActionGoal = 6, ActionTimeup = 15 };

struct MissionContext {
    EnemyManager* enemies = nullptr;
    int playerRings = 0;             // RingMission source
    float remainingTime = 0.0f;      // .bss 0x8057E800
    std::vector<int> requests;       // StageState requests issued (6 GoalAction, 15 TimeupAction)
};

// ---- Missions -------------------------------------------------------------------------------
class MissionManager;
namespace Mission {

enum class State : int32_t { Running = 0, Cleared = 1, Failed = 2 };   // Mission+0x0C

class Mission {                                    // RTTI Mission::Mission (0x40)
public:
    Mission(MissionManager& mgr, int key, int slot) : mgr_(mgr), key_(key), slot_(slot) {}
    virtual ~Mission() = default;
    virtual MissionType GetType() const { return MissionType::Base; }   // Mission-view slot 1
    virtual void UpdateMission(MissionContext&) {}                     // Mission-view vslot 0xF
    void Update(MissionContext& c) {                                   // fn_8016B5B4
        if (state_ == State::Running) UpdateMission(c);
    }
    void Clear(MissionContext& c);                                     // fn_8016B688 -> fn_8016B43C
    void Fail(MissionContext& c);                                      // fn_8016B654 -> fn_8016B330
    State state() const { return state_; }
    int key() const { return key_; }
    int slot() const { return slot_; }
    bool activated = false;                                            // +0x14
protected:
    MissionManager& mgr_;
private:
    int key_;                                                          // +0x04
    int slot_;                                                         // +0x08 0 Dark 1 Normal 2 Hero
    State state_ = State::Running;                                     // +0x0C
};

class GoalMission : public Mission {                // type 1
public:
    using Mission::Mission;
    MissionType GetType() const override { return MissionType::Goal; }
};

class CountMission : public Mission {               // type 2 (Mission virtual base @0x58)
public:
    CountMission(MissionManager& m, int key, int slot, int required, int failThreshold, int initial)
        : Mission(m, key, slot), required_(required), fail_(failThreshold) { count_[0] = initial; }
    MissionType GetType() const override { return MissionType::Count; }
    void SetCount(MissionContext& c, int value, int idx);   // vf06 0x8016AF74
    void AddCount(MissionContext& c, int delta, int idx);   // vf05 0x8016B00C
    int count(int idx = 0) const { return count_[idx]; }
    int required() const { return required_; }
private:
    void Check(MissionContext& c, int idx);
    int required_;                                   // +0x08
    int fail_;                                       // +0x0C (0 = never fails)
    int count_[2] = {0, 0};                          // +0x18/+0x1C
};

class EnemyMission : public CountMission {          // type 3; team at +0x58
public:
    EnemyMission(MissionManager& m, int key, int slot, int team, int required)
        : CountMission(m, key, slot, required, 0 /*forced*/, 0), team_(team) {}
    MissionType GetType() const override { return MissionType::Enemy; }
    void UpdateMission(MissionContext& c) override;  // vf0B 0x801F3CA4
    int team() const { return team_; }
private:
    int team_;
};

class RingMission : public CountMission {           // type 7
public:
    RingMission(MissionManager& m, int key, int slot, int required)
        : CountMission(m, key, slot, required, 0, 0) {}
    MissionType GetType() const override { return MissionType::Ring; }
    void UpdateMission(MissionContext& c) override { SetCount(c, c.playerRings, 0); }  // 0x8016F0DC
};

class TimerMission : public Mission {               // type 4; time limit at +0x08
public:
    TimerMission(MissionManager& m, int key, int slot, float seconds) : Mission(m, key, slot), limit_(seconds) {}
    MissionType GetType() const override { return MissionType::Timer; }
    void UpdateMission(MissionContext& c) override;  // vf07 0x802C92B4
    float timeLimit() const { return limit_; }
private:
    float limit_;
};

class TimerGoalMission : public TimerMission {      // type 5
public:
    using TimerMission::TimerMission;
    MissionType GetType() const override { return MissionType::TimerGoal; }
};

class TimerCountMission : public CountMission {     // type 6: TimerMission part @0x58
public:
    TimerCountMission(MissionManager& m, int key, int slot, int required, float seconds)
        : CountMission(m, key, slot, required, 0, 0), limit_(seconds) {}
    MissionType GetType() const override { return MissionType::TimerCount; }
    void UpdateMission(MissionContext& c) override;
    float timeLimit() const { return limit_; }
private:
    float limit_;
};

}  // namespace Mission

// ---- MissionManager (.bss 0x80576FBC) -------------------------------------------------------
class MissionManager {
public:
    explicit MissionManager(std::map<int, MissionDesc> descs) : descs_(std::move(descs)) {}
    // fn_8016E4EC: create the three missions of a stage (hard mode: only slot 1, key +0x40)
    void InitStage(int stageIndex, bool hardMode);
    // fn_8016B768 factory
    std::unique_ptr<Mission::Mission> Create(int key, int slot) ;
    void Update(MissionContext& c);                  // fn_8016DD58 (MissionManagerTask::vf01)
    void OnGoalRing(MissionContext& c);              // fn_8016DE18
    void OnMissionFailed(MissionContext& c);         // fn_8016DDBC
    Mission::Mission* GetMission(int slot) { return missions_[slot].get(); }   // fn_8007420C
    int currentSlot = 1;                             // +0x20
    bool anyTimeLimit = false, allTimeLimit = false; // +0x28/+0x29
    float minTimeLimit = 0.0f;                       // +0x2C
    int stageIndex = -1;                             // +0x04
    // GoalAction::DetermineClearedSlot fn_802053B0: slot 1, then last slot whose state == Cleared
    int DetermineClearedSlot() const;
private:
    std::map<int, MissionDesc> descs_;                    // +0x30
    std::array<std::unique_ptr<Mission::Mission>, 3> missions_;  // +0x08 shared_ptr x3
};

}  // namespace shadow::gameplay

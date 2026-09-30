// Native reconstruction of the mission framework; each function names its PPC origin.
#include "shadow/gameplay/Mission.hpp"
#include "shadow/gameplay/SetSystem.hpp"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>

namespace shadow::gameplay {

// ---------------------------------------------------------------------------------------------
int StageTable_IndexOfId(int id) {
    const auto& t = OriginalStageTable();
    for (size_t i = 0; i < t.size(); ++i)
        if (t[i].id == id) return (int)i;
    return -1;
}

// fn_8016CB48 inserts {type, param, count = 1, failCount = 0} for keys 0x00..0x28.
std::map<int, MissionDesc> DefaultMissionDescs() {
    using T = MissionType;
    static const struct { int key; T type; int param; } k[] = {
        {0x00, T::Goal, 0}, {0x01, T::Enemy, 0}, {0x02, T::Enemy, 2}, {0x03, T::Count, 0},
        {0x04, T::Count, 0}, {0x05, T::Enemy, 2}, {0x06, T::Count, 0}, {0x07, T::Count, 0},
        {0x08, T::Count, 0}, {0x09, T::Enemy, 0}, {0x0A, T::Count, 0}, {0x0B, T::Enemy, 0},
        {0x0C, T::Ring, 0}, {0x0D, T::TimerCount, 0}, {0x0E, T::TimerCount, 0}, {0x0F, T::Enemy, 0},
        {0x10, T::Count, 0}, {0x11, T::Count, 0}, {0x12, T::Count, 0}, {0x13, T::Count, 0},
        {0x14, T::Count, 0}, {0x15, T::Enemy, 2}, {0x16, T::Count, 0}, {0x17, T::Count, 0},
        {0x18, T::Enemy, 2}, {0x19, T::Enemy, 0}, {0x1A, T::Count, 0}, {0x1B, T::Count, 0},
        {0x1C, T::TimerGoal, 0}, {0x1D, T::Enemy, 2}, {0x1E, T::Count, 0}, {0x1F, T::Enemy, 0},
        {0x20, T::Count, 0}, {0x21, T::TimerGoal, 0}, {0x22, T::Timer, 0}, {0x23, T::Count, 0},
        {0x24, T::TimerGoal, 0}, {0x25, T::Count, 0}, {0x26, T::Count, 0}, {0x27, T::Count, 0},
        {0x28, T::Timer, 0}};
    std::map<int, MissionDesc> m;
    for (const auto& e : k) m[e.key] = {e.type, e.param, 1, 0};
    return m;
}

// fn_801E01D0: decimal, leading '-', "0x" prefix whose loop only accepts digits 0-9 (quirk kept).
static bool ParseNumber(const char*& p, int& out) {
    while (*p == ' ' || *p == '\t') ++p;
    bool neg = false;
    if (*p == '-') { neg = true; ++p; }
    int base = 10;
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) { base = 16; p += 2; }
    if (*p < '0' || *p > '9') return false;
    int v = 0;
    while (*p >= '0' && *p <= '9') v = v * base + (*p++ - '0');
    out = neg ? -v : v;
    return true;
}

// Subset of TGameDefaultParam::ParseLine (fn_801E030C) + SetMissionCount (fn_801DF470).
int ApplyMissionCountFile(const std::string& path, std::map<int, MissionDesc>& descs) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return -1;
    const auto& table = OriginalStageTable();
    int section = -1;   // +0x2C50 ([ALL] or unknown -> -1)
    int applied = 0;
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        if (line[0] == '[') {
            const size_t e = line.find(']');
            const std::string name = line.substr(1, e == std::string::npos ? std::string::npos : e - 1);
            section = -1;
            for (size_t i = 0; i < table.size(); ++i)
                if (name == table[i].name) { section = (int)i; break; }
            continue;
        }
        struct Cmd { const char* name; int slot; bool hard; };
        static const Cmd cmds[] = {{"MISSIONCOUNT_HARD", 1, true}, {"MISSIONCOUNT_D", 0, false},
                                   {"MISSIONCOUNT_N", 1, false}, {"MISSIONCOUNT_H", 2, false}};
        for (const Cmd& c : cmds) {
            const size_t n = std::strlen(c.name);
            if (line.compare(0, n, c.name) != 0) continue;
            if (!c.hard && line.size() > n && line[n] == '_') continue;   // D/N/H vs HARD
            if (section < 0) break;                                       // needs a stage block
            const size_t colon = line.find(':');
            if (colon == std::string::npos) break;
            const char* p = line.c_str() + colon + 1;
            int a, b;
            if (!ParseNumber(p, a) || !ParseNumber(p, b)) break;         // exactly two numbers
            const int key = c.hard ? table[section].hardKey : table[section].route[c.slot].missionKey;
            auto it = descs.find(key);
            if (it == descs.end()) break;                                 // descriptor must exist
            it->second.count = a;
            it->second.failCount = b;                                     // last section wins
            ++applied;
            break;
        }
    }
    return applied;
}

// ---------------------------------------------------------------------------------------------
// EnemyManager
int EnemyManager::TeamOfSetId(uint16_t id) {             // fn_801A2BA0 split
    return id < 0x78 ? TeamGun : id < 0x8C ? TeamEggman : TeamBlackArms;
}

void EnemyManager::ScanSetEnemies(SetData& set) {       // fn_801A2CFC -> fn_801A2BA0
    total = 0;
    placed = {};
    defeated = {};
    defeatedByPlayer = {};
    groupAlive.clear();
    for (int i = 0; i < set.recordCount(); ++i) {
        SetRecord& r = set.records()[i];
        if (!(r.flags & kSetEnabled) || r.id < 0x64 || r.id >= 0x96) continue;
        int team = TeamOfSetId(r.id);
        int n = 1;
        if (r.id == 0x0064) {                            // GUN_SOLDIER: AppearType (+0x24) == 5 excluded
            int32_t appear = 0;
            if (r.misc && r.miscLen >= 0x28) std::memcpy(&appear, r.misc + 0x24, 4);
            if (appear == 5) continue;
        } else if (r.id == 0x0091) {                     // BK_LARVA: counts param Num (+0x1C)
            int32_t num = 0;
            if (r.misc && r.miscLen >= 0x20) std::memcpy(&num, r.misc + 0x1C, 4);
            n = num;
        }
        placed[team] += n;
        total += n;
        groupAlive[r.link] += n;
    }
}

void EnemyManager::CountDefeat(int team, bool byPlayer) {   // fn_801A2930
    defeated[team] += 1;                                    // any cause counts
    if (byPlayer) defeatedByPlayer[team] += 1;              // + EnemyDeadByPlayerCommand (0x403)
}

void EnemyManager::OnEnemyLifeEvent(uint8_t link, int team, uint32_t flags) {   // fn_801A250C
    auto g = groupAlive.find(link);
    if (g == groupAlive.end()) return;                      // only registered groups
    if (flags & 2) { defeated[team] -= 1; g->second += 1; } // revive
    if (flags & 1) { CountDefeat(team, (flags & 4) != 0); g->second -= 1; }
    // group reaching 0 -> fn_80169460(set, linkId, 1) link event (engine side)
}

// ---------------------------------------------------------------------------------------------
namespace Mission {

void Mission::Clear(MissionContext& c) {                    // fn_8016B688 / fn_8016B43C
    state_ = State::Cleared;
    c.requests.push_back(ActionGoal);                       // fn_801762D0(StageState, 6, 0)
}

void Mission::Fail(MissionContext& c) {                     // fn_8016B654 / fn_8016B330
    if (state_ == State::Cleared) return;
    state_ = State::Failed;
    if (mgr_.currentSlot == slot_) mgr_.currentSlot = 1;    // fn_8016E220(1)
    mgr_.OnMissionFailed(c);                                // fn_8016DDBC
}

void CountMission::Check(MissionContext& c, int idx) {
    if (idx == 0) {
        if (count_[0] >= required_) Clear(c);              // 0x8016B070 / 0x8016AFB8
    } else if (fail_ != 0 && count_[1] >= fail_) {
        Fail(c);
    }
}

void CountMission::SetCount(MissionContext& c, int v, int idx) {   // vf06 0x8016AF74
    if (idx == 1 && fail_ == 0) return;
    count_[idx] = v < 0 ? 0 : v;
    Check(c, idx);
}

void CountMission::AddCount(MissionContext& c, int d, int idx) {   // vf05 0x8016B00C
    if (idx == 1 && fail_ == 0) return;
    count_[idx] += d;
    if (count_[idx] < 0) count_[idx] = 0;
    Check(c, idx);
}

void EnemyMission::UpdateMission(MissionContext& c) {       // vf0B 0x801F3CA4
    if (c.enemies) SetCount(c, c.enemies->defeated[team_], 0);   // fn_801A24FC
}

void TimerMission::UpdateMission(MissionContext& c) {       // vf07 0x802C92B4
    if (std::fabs(c.remainingTime) < 0.0001f) Fail(c);     // fn_8000A950
}

void TimerCountMission::UpdateMission(MissionContext& c) {
    if (std::fabs(c.remainingTime) < 0.0001f) Fail(c);
}

}  // namespace Mission

// ---------------------------------------------------------------------------------------------
std::unique_ptr<Mission::Mission> MissionManager::Create(int key, int slot) {   // fn_8016B768
    auto it = descs_.find(key);
    if (it == descs_.end()) return nullptr;
    const MissionDesc& d = it->second;
    switch (d.type) {
    case MissionType::Goal:      return std::make_unique<Mission::GoalMission>(*this, key, slot);
    case MissionType::Count:     return std::make_unique<Mission::CountMission>(*this, key, slot, d.count, d.failCount, 0);
    case MissionType::Enemy:     return std::make_unique<Mission::EnemyMission>(*this, key, slot, d.param, d.count);
    case MissionType::Timer:     return std::make_unique<Mission::TimerMission>(*this, key, slot, (float)d.count);
    case MissionType::TimerGoal: return std::make_unique<Mission::TimerGoalMission>(*this, key, slot, (float)d.count);
    case MissionType::TimerCount:
        return std::make_unique<Mission::TimerCountMission>(*this, key, slot, d.count, (float)d.failCount);
    case MissionType::Ring:      return std::make_unique<Mission::RingMission>(*this, key, slot, d.count);
    default:                     return std::make_unique<Mission::Mission>(*this, key, slot);
    }
}

void MissionManager::InitStage(int index, bool hardMode) {  // fn_8016E4EC
    stageIndex = index;
    const StageTableEntry& e = OriginalStageTable().at(index);
    for (auto& m : missions_) m.reset();
    if (hardMode) {
        if (e.hardKey >= 0) missions_[1] = Create(e.hardKey, 1);
    } else {
        for (int s = 0; s < 3; ++s)
            if (e.route[s].missionKey >= 0) missions_[s] = Create(e.route[s].missionKey, s);
    }
    // NOTE: +0x28/+0x29/+0x2C semantics per MISSION_SYSTEM.md §3.5 (PROVEN fields); the loop below
    // is a behavioural approximation of fn_8016E4EC 0x8016E640..0x8016E6A8, not an instruction-level port.
    anyTimeLimit = false;
    allTimeLimit = true;
    minTimeLimit = 0.0f;
    bool first = true;
    for (auto& m : missions_) {
        if (!m) continue;
        float t = 0.0f;
        bool timed = true;
        if (auto* tm = dynamic_cast<Mission::TimerMission*>(m.get())) t = tm->timeLimit();
        else if (auto* tc = dynamic_cast<Mission::TimerCountMission*>(m.get())) t = tc->timeLimit();
        else timed = false;
        if (timed) {
            anyTimeLimit = true;
            if (first || t < minTimeLimit) minTimeLimit = t;
            first = false;
        } else {
            allTimeLimit = false;
        }
    }
    currentSlot = 1;
}

void MissionManager::Update(MissionContext& c) {            // fn_8016DD58
    for (auto& m : missions_)
        if (m) m->Update(c);
}

void MissionManager::OnGoalRing(MissionContext& c) {        // fn_8016DE18
    for (auto& m : missions_)                               // TimerGoalMission (type 5) first
        if (m && m->GetType() == MissionType::TimerGoal && m->state() != Mission::State::Failed) {
            m->Clear(c);
            return;
        }
    for (auto& m : missions_)                               // then GoalMission (type 1)
        if (m && m->GetType() == MissionType::Goal) {
            m->Clear(c);
            return;
        }
}

void MissionManager::OnMissionFailed(MissionContext& c) {   // fn_8016DDBC
    for (auto& m : missions_)
        if (m && m->state() != Mission::State::Failed) return;
    c.requests.push_back(ActionTimeup);                     // every slot empty or failed
}

int MissionManager::DetermineClearedSlot() const {          // fn_802053B0 (story path)
    int slot = 1;
    for (int s = 0; s < 3; ++s)
        if (missions_[s] && missions_[s]->state() == Mission::State::Cleared) slot = s;
    return slot;
}

}  // namespace shadow::gameplay

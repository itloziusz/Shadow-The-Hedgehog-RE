// BkLarvaGenerator and BkLarva gameplay recovery from PAL GUPP8P main.dol.
// The stage/world trigger and collision delivery remain explicit engine hooks.
#pragma once
#include "shadow/gameplay/Mission.hpp"
#include "shadow/gameplay/SetSystem.hpp"
#include "shadow/gameplay/Task.hpp"
#include <vector>

namespace shadow::gameplay {

class BkLarvaGenerator;

// RTTI-original class name. Collision handlers: 0x8018A7D0 and 0x8018AA84.
class BkLarva : public Task {
public:
    BkLarva(BkLarvaGenerator& generator, int ordinal);
    void Update(float dt) override; // movement/attack/collision body remains an engine hook
    // Explicit engine collision/command delivery. A second event is ignored.
    void ReportDefeat(bool byPlayer);
    void ReportCommand14Defeat(bool byPlayer);
    int ordinal() const { return ordinal_; }
    int deathState() const { return deathState_; } // original states 4/5; 0 means active hook
private:
    BkLarvaGenerator& generator_;
    int ordinal_;
    bool reported_ = false;
    int deathState_ = 0;
    float deathTimer_ = 0.0f;
};

// The original's state 0/1 depend on stage and world services not recovered here.
// 0x80189BC0..0x80189C84; the host supplies those predicates explicitly.
struct BkLarvaEngine {
    virtual ~BkLarvaEngine() = default;
    virtual bool StageAllowsGenerator(const SetSlot& slot) = 0;
    virtual bool SpawnTriggerReached(const SetSlot& slot) = 0;
    virtual void UpdateLarvaMotion(const SetSlot& slot, int ordinal, float dt) = 0;
    virtual void OnGeneratorDestroyed(const SetSlot& slot) = 0; // checkpoint/state-adapter service
};

// RTTI-original class name. The original is a Task under Enemy layer, not EnemyTemplate.
class BkLarvaGenerator : public Task {
public:
    enum class State { WaitingStage, WaitingTrigger, Spawning, WaitingChildren };
    BkLarvaGenerator(Task* enemyLayer, SetSlot& slot, EnemyManager& enemies, BkLarvaEngine& engine);
    ~BkLarvaGenerator() override;
    void Update(float dt) override;
    BkLarva* FirstAttackable() const;
    int requested() const { return requested_; }
    int spawned() const { return spawned_; }
    int defeated() const { return defeated_; }
    int stateRemaining() const { return stateRemaining_; } // original generator +0x148
    int active() const { return static_cast<int>(active_.size()); }
    bool valid() const { return valid_; }
    State state() const { return state_; }
private:
    friend class BkLarva;
    void OnLarvaDefeated(BkLarva& larva, bool byPlayer);
    void UpdateLarvaMotion(int ordinal, float dt);
    SetSlot& slot_;
    EnemyManager& enemies_;
    BkLarvaEngine& engine_;
    State state_ = State::WaitingStage;
    int requested_ = 0;
    int spawned_ = 0;
    int defeated_ = 0;
    int stateRemaining_ = 0;
    bool valid_ = false;
    std::vector<BkLarva*> active_;
};

}  // namespace shadow::gameplay

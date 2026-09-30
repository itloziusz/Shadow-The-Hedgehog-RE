#include "shadow/gameplay/BkLarva.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>

namespace shadow::gameplay {

BkLarva::BkLarva(BkLarvaGenerator& generator, int ordinal)
    : Task(&generator, "BkLarva"), generator_(generator), ordinal_(ordinal) {}

void BkLarva::Update(float dt) {                     // 0x8018AF18..0x8018B4C8: engine-side movement
    if (deathState_ == 4) {                           // 0x8018B2C8..0x8018B36C
        deathTimer_ = 0.5f;
        --generator_.stateRemaining_;
        deathState_ = 5;
    } else if (deathState_ == 5 && deathTimer_ < 0.0f) { // 0x8018B374..0x8018B3E4
        Kill();
        return;
    }
    generator_.UpdateLarvaMotion(ordinal_, dt);
    if (deathState_ == 5) deathTimer_ -= dt;          // 0x8018B498..0x8018B4A0
}

void BkLarva::ReportDefeat(bool byPlayer) {  // collision path 0x8018A9B8..0x8018A9D8
    if (reported_) return;
    reported_ = true;
    generator_.OnLarvaDefeated(*this, byPlayer);
    deathState_ = 4;                                 // collision handler 0x8018A9D0..0x8018A9D8
}

void BkLarva::ReportCommand14Defeat(bool byPlayer) { // 0x8018AA9C..0x8018AC08
    if (reported_) return;
    reported_ = true;
    generator_.OnLarvaDefeated(*this, byPlayer);
    --generator_.stateRemaining_;                    // 0x8018ABE4..0x8018ABF8
    Kill();                                           // 0x8018ABFC..0x8018AC04
}

BkLarvaGenerator::BkLarvaGenerator(Task* layer, SetSlot& slot, EnemyManager& enemies, BkLarvaEngine& engine)
    : Task(layer, "BkLarvaGenerator"), slot_(slot), enemies_(enemies), engine_(engine) {
    // 0x80189A10..0x80189A1C copies 0x2C bytes. Num is word 7, generator+0x74.
    uint32_t params[11] = {};
    valid_ = SetSlot_GetParams(slot_, params, sizeof params);
    if (!valid_) return; // explicit refusal: no made-up default for a malformed SET record
    std::memcpy(&requested_, &params[7], sizeof requested_);
    stateRemaining_ = requested_;                    // 0x80189A90..0x80189A98
    SetSlot_Attach(slot_, this);                     // SetEnemyBase ctor 0x8018A300..0x8018A30C
    SetSlot_SetFlag8(slot_);                         // 0x8018A3A8..0x8018A3AC
}

BkLarvaGenerator::~BkLarvaGenerator() {               // 0x80189F84..0x80189F98 -> SetSlot_Detach
    if (slot_.object == this) {
        engine_.OnGeneratorDestroyed(slot_);           // 0x80189EE0..0x80189F18 state/checkpoint path
        slot_.object = nullptr;
        slot_.rec->runtimeFlags &= ~(kSetAlive | kSetDespawnRequest);
    }
}

void BkLarvaGenerator::UpdateLarvaMotion(int ordinal, float dt) {
    engine_.UpdateLarvaMotion(slot_, ordinal, dt);
}

void BkLarvaGenerator::Update(float) {              // 0x80189B70..0x80189DE0
    if (!valid_) return;
    switch (state_) {
    case State::WaitingStage:
        if (engine_.StageAllowsGenerator(slot_)) state_ = State::WaitingTrigger;
        break;
    case State::WaitingTrigger:
        if (engine_.SpawnTriggerReached(slot_)) state_ = State::Spawning;
        else break;
        [[fallthrough]]; // original 0x80189C80 falls into the state-2 spawn block
    case State::Spawning: {
        // 0x80189C88..0x80189CAC: postincrement and create one BkLarva per frame.
        const int ordinal = spawned_++;
        active_.push_back(new BkLarva(*this, ordinal));
        if (spawned_ >= requested_) state_ = State::WaitingChildren; // 0x80189CB0..0x80189CC4
        break;
    }
    case State::WaitingChildren:
        // 0x80189DB8..0x80189DE0: count descendants, then request task death.
        // A reported larva remains a Task until deferred scheduler deletion.
        if (!firstChild()) Kill();
        break;
    }
}

BkLarva* BkLarvaGenerator::FirstAttackable() const {
    return active_.empty() ? nullptr : active_.front();
}

void BkLarvaGenerator::OnLarvaDefeated(BkLarva& larva, bool byPlayer) {
    auto it = std::find(active_.begin(), active_.end(), &larva);
    if (it == active_.end()) return;
    active_.erase(it);
    ++defeated_;
    // 0x8018A9BC..0x8018A9CC and 0x8018ABCC..0x8018ABE0:
    // generator+0x28 is the SET slot, r5=2 (Black Arms), r6=1|player_bit4.
    enemies_.OnEnemyLifeEvent(slot_.rec->link, TeamBlackArms, 1u | (byPlayer ? 4u : 0u));
}

}  // namespace shadow::gameplay

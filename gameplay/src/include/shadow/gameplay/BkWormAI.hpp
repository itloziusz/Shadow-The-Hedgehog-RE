// BkWormAI decision states recovered from PAL GUPP8P main.dol.
// Original RTTI class/state names; method names below describe recovered semantics.
// Evidence and deliberate engine boundaries: gameplay/enemy/BK_WORM.md.
#pragma once

#include "shadow/gameplay/EnemyAI.hpp"

namespace shadow::gameplay {

// Display, terrain, random-number and movement operations remain engine services.
// Each method denotes a real DOL call group, not a substitute animation or mover.
struct BkWormBody {
    virtual ~BkWormBody() = default;
    virtual Vec3 Position() const = 0;
    virtual Vec3 TargetPosition() const = 0;          // sensor input, EnemyBaseAI vf02
    virtual Vec3 TargetDirection() const = 0;         // EnemyAITarget+0x0C, target - worm
    virtual bool TurnTowards(const Vec3& direction) = 0; // 0x8019F8F8; bool used by Move
    virtual bool MotionEnded() const = 0;            // display vslot +0x2C
    virtual float Uniform(float lo, float hi) = 0;  // fn_8040FCD0
    virtual void InitialSetup() = 0;                 // display vslot +0x20(2,1)
    virtual void BeginBuriedWait() = 0;              // 0x801835EC engine display/effect group
    virtual void PrepareEmergence(const Vec3&) = 0; // 0x801834B4 effect at current position
    virtual void BeginAppear() = 0;                  // 0x801832FC engine display/rotation group
    virtual void BeginStanding() = 0;                // 0x80183170 display motion selection
    virtual void BeginAttack() = 0;                  // 0x80183074 display motion 3
    virtual void BeginAfterAttack() = 0;             // 0x80182EE8 display motion selection
    virtual Vec3 BeginMove() = 0;                    // 0x80182C30..98: choose/set home, display
    virtual void BeginMoveTravel() = 0;              // 0x801829E4..74 engine display/effect group
    virtual void CompleteMoveTravel() = 0;           // 0x80182AB8..B10 home/position/effect group
};

class BkWormAI final : public EnemyBaseAI {
public:
    BkWormAI(BkWormBody& body, const EnemySensor& sensor, int attackTimes,
             float intervalSec)
        : body(body), sensor(sensor), attackTimes(attackTimes), intervalSec(intervalSec),
          home(sensor.pos) {}

    void EnterInitialState() override; // BkWormAI::vf0C 0x801838E4
    void ReturnToWait() override;       // BkWormAI::vf0D 0x8018388C

    BkWormBody& body;
    const EnemySensor& sensor;
    int attackTimes;                     // SET E+0x290, t1 AttackTimes
    float intervalSec;                   // SET E+0x294, t4 IntervalSec
    Vec3 home;                           // E+0x6C, updated by Move's engine service
    bool buried = true;                  // AI+0x7C, ctor 0x801839A8
    int remainingAttacks = 0;            // AI+0x78

    static EnemyAIState& WaitAppear();
    static EnemyAIState& Appear();
    static EnemyAIState& WaitStanding();
    static EnemyAIState& Attack();
    static EnemyAIState& AfterAttack();
    static EnemyAIState& Move();
};

} // namespace shadow::gameplay

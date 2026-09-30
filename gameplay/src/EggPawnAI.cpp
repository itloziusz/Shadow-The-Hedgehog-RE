#include "shadow/gameplay/EggPawnAI.hpp"

namespace shadow::gameplay {
namespace {
EggPawnAI& EP(EnemyBaseAI& ai) { return static_cast<EggPawnAI&>(ai); }

// Shared EnemyWalkerAI states are retained by RTTI name. The unrecovered
// standing/patrol motion steps are delegated explicitly to the engine body.
struct StandingState : EnemyAIState {
    const char* Name() const override { return "EnemyWalkerAI_WaitAction_Standing"; }
    void Enter(EnemyBaseAI& ai) override {
        auto& e = EP(ai);
        e.subStep = 0;
        e.body.BeginStandingWait(); // 0x8019A60C
        e.body.SetMoveTarget(e.sensor.pos);
    }
    void Update(EnemyBaseAI& ai) override {
        auto& e = EP(ai);
        if (e.body.DetectNearestPlayer(e.sensor)) { e.OnTargetFound(); return; }
        e.body.TickStandingWait(); // 0x8019A300 after detection
    }
};
struct MovingState : EnemyAIState {
    const char* Name() const override { return "EnemyWalkerAI_WaitAction_Moving"; }
    void Enter(EnemyBaseAI& ai) override {
        auto& e = EP(ai);
        e.subStep = 0;
        e.body.BeginMovingWait(); // 0x8019A218, shared patrol remains an engine service
    }
    void Update(EnemyBaseAI& ai) override {
        auto& e = EP(ai);
        if (e.body.DetectNearestPlayer(e.sensor)) { e.OnTargetFound(); return; }
        e.body.TickMovingWait(); // 0x80199E90 after detection
    }
};
struct CautionState : EnemyAIState {
    const char* Name() const override { return "EnemyWalkerAI_Caution"; }
    void Enter(EnemyBaseAI& ai) override {
        auto& e = EP(ai);
        e.body.BeginCaution();
        e.timer = e.timerReset = 1.0f; // 0x80199E68..0x80199E74
    }
    void Update(EnemyBaseAI& ai) override {
        auto& e = EP(ai);
        e.body.FaceTrackedTarget(); // 0x80199D3C..0x80199D58
        if (!e.body.TrackedTargetInSearchArea(e.sensor)) { e.ReturnToWait(); return; }
        if (e.timer < 0.0f) e.StartAttack(); // strict negative, 0x80199DCC..0x80199DF4
    }
};
struct ReturnHomeState : EnemyAIState {
    const char* Name() const override { return "EnemyWalkerAI_ReturnHome"; }
    void Enter(EnemyBaseAI& ai) override {
        auto& e = EP(ai);
        e.body.BeginReturnHome(false); // 0x8019A7D8
        e.body.SetMoveTarget(e.sensor.pos);
    }
    void Update(EnemyBaseAI& ai) override {
        auto& e = EP(ai);
        if (e.body.AdvanceTowardMoveTarget()) e.ReturnToWait(); // 0x8019A77C
    }
};
struct ReturnHomeDashState : EnemyAIState {
    const char* Name() const override { return "EnemyWalkerAI_ReturnHomeDash"; }
    void Enter(EnemyBaseAI& ai) override {
        auto& e = EP(ai);
        e.body.BeginReturnHome(true); // 0x8019A6FC, EggPawnBase::GetDashMotion = 4
        e.body.SetMoveTarget(e.sensor.pos);
    }
    void Update(EnemyBaseAI& ai) override {
        auto& e = EP(ai);
        if (e.body.AdvanceTowardMoveTarget()) e.ReturnToWait(); // 0x8019A6A0
    }
};
struct WeaponAttackState : EnemyAIState {
    const char* Name() const override { return "EggPawnAI_WeaponAttack"; }
    void Enter(EnemyBaseAI& ai) override {
        auto& e = EP(ai);
        e.body.SetMotion(e.AttackMotionId(), true); // 0x8017E1F8
        e.subStep = 0;
    }
    void Update(EnemyBaseAI& ai) override { // 0x8017DF84..0x8017E1C0
        auto& e = EP(ai);
        switch (e.subStep) {
        case 0:
            if (!e.body.MotionEnded()) return;
            e.body.SetMotion(e.AimMotionId(), true);
            e.body.SetMoveTarget(e.body.PickRandomPointNear(30.0f));
            e.timer = e.timerReset = 2.0f;
            e.subStep = 1;
            return;
        case 1:
            if (!e.body.TrackedTargetInSearchArea(e.sensor)) { e.ReturnToWait(); return; }
            if (e.body.AdvanceTowardMoveTarget() || e.timer < 0.0f) {
                e.body.FaceTrackedTarget();
                e.subStep = 2;
            }
            return;
        case 2:
            // The original queries the target position, then restarts the attack motion.
            (void)e.body.TrackedTargetPosition();
            e.body.SetMotion(e.AttackMotionId(), true);
            e.subStep = 0;
            return;
        default: return;
        }
    }
};
void BeginCharge(EggPawnAI& e) { // 0x8017E508 / 0x8017E7F8
    e.body.SetMotion(e.body.HasShield() ? 0xD : 0xC, true);
    e.body.SetMoveTarget(e.body.AttackPoint(-1.0f));
    e.timer = e.timerReset = 3.0f;
    e.subStep = 0;
}
void RestartCharge(EggPawnAI& e) { // 0x8017E43C / 0x8017E730
    e.timer = e.timerReset = 3.0f;
    e.body.SetMoveTarget(e.body.AttackPoint(-1.0f));
    e.body.SetMotion(e.body.HasShield() ? 0xD : 0xC, true);
    e.subStep = 0;
}
struct TukiAttackState : EnemyAIState {
    const char* Name() const override { return "EggPawnAI_TukiAttack"; }
    void Enter(EnemyBaseAI& ai) override { BeginCharge(EP(ai)); }
    void Update(EnemyBaseAI& ai) override { // 0x8017E270..0x8017E4FC
        auto& e = EP(ai);
        switch (e.subStep) {
        case 0:
            if (e.timer < 0.0f || e.body.TrackedTargetDistance() < 15.0f ||
                e.body.AdvanceTowardMoveTarget()) {
                e.body.SetMotion(0xE, true); // lance thrust
                e.subStep = 1;
            }
            return;
        case 1:
            if (e.body.MotionEnded()) {
                e.timer = e.timerReset = 1.0f;
                e.body.SetMotion(e.ChargePauseMotionId(), true);
                e.subStep = 2;
            }
            return;
        case 2:
            if (!e.body.TrackedTargetInSearchArea(e.sensor)) { e.ReturnToWait(); return; }
            if (e.timer < 0.0f) RestartCharge(e);
            return;
        default: return;
        }
    }
};
struct DashAttackState : EnemyAIState {
    const char* Name() const override { return "EggPawnAI_DashAttack"; }
    void Enter(EnemyBaseAI& ai) override { BeginCharge(EP(ai)); }
    void Update(EnemyBaseAI& ai) override { // 0x8017E5E8..0x8017E7F4
        auto& e = EP(ai);
        switch (e.subStep) {
        case 0:
            if (e.timer < 0.0f || e.body.AdvanceTowardMoveTarget()) {
                e.timer = e.timerReset = 1.0f;
                e.body.SetMotion(e.ChargePauseMotionId(), true);
                e.subStep = 1;
            }
            return;
        case 1:
            if (!e.body.TrackedTargetInSearchArea(e.sensor)) { e.ReturnToWait(); return; }
            if (e.timer < 0.0f) RestartCharge(e);
            return;
        default: return;
        }
    }
};
} // namespace

EnemyAIState& EggPawnAI::Standing()       { static StandingState s; return s; }
EnemyAIState& EggPawnAI::Moving()         { static MovingState s; return s; }
EnemyAIState& EggPawnAI::Caution()        { static CautionState s; return s; }
EnemyAIState& EggPawnAI::ReturnHome()     { static ReturnHomeState s; return s; }
EnemyAIState& EggPawnAI::ReturnHomeDash() { static ReturnHomeDashState s; return s; }
EnemyAIState& EggPawnAI::WeaponAttack()   { static WeaponAttackState s; return s; }
EnemyAIState& EggPawnAI::TukiAttack()     { static TukiAttackState s; return s; }
EnemyAIState& EggPawnAI::DashAttack()     { static DashAttackState s; return s; }

void EggPawnAI::EnterInitialState() { // 0x8017ECD0..0x8017EE54
    switch (appearType) {
    case 0: ReturnToWait(); return;
    case 1: case 2: sm.ChangeState(&ReturnHome()); return;
    case 3: sm.ChangeState(&ReturnHomeDash()); return;
    default: sm.ChangeState(&Standing()); return;
    }
}
void EggPawnAI::ReturnToWait() { // 0x8017EB58..0x8017EC04
    sm.ChangeState(waitActMoveType == 0 ? &Standing() : &Moving());
}
void EggPawnAI::OnTargetFound() { // inherited EnemyWalkerAI 0x8019A85C
    sm.ChangeState(&Caution());
}
void EggPawnAI::StartAttack() { // 0x8017E8D4..0x8017E940
    if (weaponType == 0) sm.ChangeState(&DashAttack());
    else if (weaponType == 3) sm.ChangeState(&TukiAttack());
    else sm.ChangeState(&WeaponAttack());
}

int EggPawnAI::AttackMotionId() const { // 0x8017CB50..0x8017CBA0 (vslot +0x280)
    if (weaponType == 1 || weaponType == 2) return body.HasShield() ? 0xB : 0xA;
    if (weaponType == 3) return body.HasShield() ? 0xD : 0xC;
    return 6;
}

int EggPawnAI::AimMotionId() const { // 0x8017CBD8..0x8017CC1C (vslot +0x274)
    if (weaponType == 1 || weaponType == 2) return body.HasShield() ? 3 : 1;
    return body.HasShield() ? 2 : 0;
}

int EggPawnAI::ChargePauseMotionId() const { // 0x8017CC20..0x8017CC64 (param vslot +0x48)
    if (weaponType == 1 || weaponType == 2) return body.HasShield() ? 9 : 7;
    return body.HasShield() ? 8 : 6;
}

} // namespace shadow::gameplay

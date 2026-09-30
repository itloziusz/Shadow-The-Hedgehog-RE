#include "shadow/gameplay/GunRobotAI.hpp"

namespace shadow::gameplay {
namespace {
GunRobotAI& GR(EnemyBaseAI& ai) { return static_cast<GunRobotAI&>(ai); }

struct StandingState final : EnemyAIState {
    const char* Name() const override { return "EnemyWalkerAI_WaitAction_Standing"; }
    void Enter(EnemyBaseAI& ai) override {
        auto& g = GR(ai);
        g.subStep = 0;
        g.body.BeginStandingWait();
        g.body.SetMoveTarget(g.body.SetPosition());
    }
    void Update(EnemyBaseAI& ai) override {
        auto& g = GR(ai);
        if (g.body.DetectNearestPlayer(g.sensor)) { g.OnTargetFound(); return; }
        g.body.TickStandingWait();
    }
};
struct MovingState final : EnemyAIState {
    const char* Name() const override { return "EnemyWalkerAI_WaitAction_Moving"; }
    void Enter(EnemyBaseAI& ai) override {
        auto& g = GR(ai);
        g.subStep = 0;
        g.body.BeginMovingWait();
    }
    void Update(EnemyBaseAI& ai) override {
        auto& g = GR(ai);
        if (g.body.DetectNearestPlayer(g.sensor)) { g.OnTargetFound(); return; }
        g.body.TickMovingWait();
    }
};
struct ReturnHomeState final : EnemyAIState {
    const char* Name() const override { return "EnemyWalkerAI_ReturnHome"; }
    void Enter(EnemyBaseAI& ai) override {
        auto& g = GR(ai);
        g.body.BeginReturnHome();
        g.body.SetMoveTarget(g.body.SetPosition());
    }
    void Update(EnemyBaseAI& ai) override {
        auto& g = GR(ai);
        if (g.body.AdvanceTowardMoveTarget()) g.ReturnToWait();
    }
};
struct BaseCautionState final : EnemyAIState {
    const char* Name() const override { return "EnemyBaseAI_Caution"; }
    void Enter(EnemyBaseAI& ai) override { GR(ai).body.BeginBaseCaution(); }
    void Update(EnemyBaseAI& ai) override { GR(ai).body.UpdateBaseCaution(GR(ai)); }
};
struct WeaponAttackState final : EnemyAIState {
    const char* Name() const override { return "GunRobotAI_WeaponAttack"; }
    void Enter(EnemyBaseAI& ai) override { // 0x80186514
        auto& g = GR(ai);
        g.body.SetMotion(g.AttackMotionId(), true);
        g.subStep = 0;
    }
    void Update(EnemyBaseAI& ai) override { // 0x80186308..0x80186500
        auto& g = GR(ai);
        switch (g.subStep) {
        case 0:
            if (!g.body.MotionEnded()) return;
            g.body.SetMotion(0, true); // GunRobot::vf1E, 0x80195AD0
            g.body.SetMoveTarget(g.body.PickRandomPointNear(g.body.Position(), 30.0f));
            ++g.subStep;
            return;
        case 1: {
            const Vec3 target = g.body.TrackedTargetPosition(); // AI vf02, 0x80186408
            if (!g.body.TrackedTargetInSearchArea(g.sensor, target)) { g.ReturnToWait(); return; }
            if (g.body.AdvanceTowardMoveTarget()) {
                g.body.FaceTrackedTarget();
                ++g.subStep;
            }
            return;
        }
        case 2:
            if (g.body.TurnToTargetYaw()) {
                g.body.SetMotion(g.AttackMotionId(), true);
                g.subStep = 0;
            }
            return;
        default: return;
        }
    }
};
} // namespace

EnemyAIState& GunRobotAI::Standing()     { static StandingState s; return s; }
EnemyAIState& GunRobotAI::Moving()       { static MovingState s; return s; }
EnemyAIState& GunRobotAI::ReturnHome()   { static ReturnHomeState s; return s; }
EnemyAIState& GunRobotAI::BaseCaution()  { static BaseCautionState s; return s; }
EnemyAIState& GunRobotAI::WeaponAttack() { static WeaponAttackState s; return s; }

void GunRobotAI::EnterInitialState() { // 0x8018684C..0x80186990
    if (appearType == 0) ReturnToWait();
    else if (appearType >= 1 && appearType <= 3) sm.ChangeState(&ReturnHome());
    else sm.ChangeState(&Standing());
}
void GunRobotAI::ReturnToWait() { // 0x8018678C..0x80186838
    sm.ChangeState(waitActMoveType == 0 ? &Standing() : &Moving());
}
void GunRobotAI::OnTargetFound() { // 0x801866CC
    sm.ChangeState(&BaseCaution());
}
void GunRobotAI::OnDead() { // 0x80186588..0x801865D0
    body.ApplyDeathEffects(); // original calls display + status; no ChangeState
}
void GunRobotAI::StartAttack() { // 0x801865E8
    sm.ChangeState(&WeaponAttack());
}
int GunRobotAI::AttackMotionId() const { // 0x80195A50, E+0x290
    if (weaponType == 0 || weaponType == 1 || weaponType == 5) return 4;
    if (weaponType >= 2 && weaponType <= 4) return 5;
    return 2;
}

} // namespace shadow::gameplay

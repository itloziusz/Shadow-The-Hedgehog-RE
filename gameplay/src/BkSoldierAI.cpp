// Partial native BK Soldier state recovery from main.dol (PAL GUPP8P).
// See enemy/BK_SOLDIER.md for DOL instruction ranges and remaining engine hooks.
#include "shadow/gameplay/BkSoldierAI.hpp"
#include <cmath>

namespace shadow::gameplay {
namespace {
BkSoldierAI& BK(EnemyBaseAI& ai) { return static_cast<BkSoldierAI&>(ai); }

// Shared SoldierCommonAI standing update: 0x80189308..0x801895DC.
struct StandingState final : EnemyAIState {
    const char* Name() const override { return "WaitAction_Standing"; }
    void Enter(EnemyBaseAI& ai) override { BK(ai).body.BeginStandingWait(); }
    void Update(EnemyBaseAI& ai) override { if (BK(ai).Detects()) ai.OnTargetFound(); }
};

// Shared moving state: sensor test 0x80188BDC, patrol 0x80188B98.
// Patrol is an explicit engine hook because the native shared state is not yet
// extracted into a reusable component for both GUN and BK soldiers.
struct MovingState final : EnemyAIState {
    const char* Name() const override { return "WaitAction_Moving"; }
    void Enter(EnemyBaseAI& ai) override { BK(ai).body.BeginMovingWait(); }
    void Update(EnemyBaseAI& ai) override {
        auto& b = BK(ai);
        if (b.Detects()) { ai.OnTargetFound(); return; }
        b.body.UpdateSharedPatrol();
    }
};

// 0x8018043C..0x801804CC: melee branches before motion-end check.
struct CautionState final : EnemyAIState {
    const char* Name() const override { return "Caution"; }
    void Enter(EnemyBaseAI& ai) override { BK(ai).body.BeginCaution(); }
    void Update(EnemyBaseAI& ai) override {
        auto& b = BK(ai);
        b.body.TurnTowards(b.body.TargetPosition());
        if (b.weaponType == 0 || b.weaponType == 1) {
            b.sm.ChangeState(&BkSoldierAI::ChaseTarget());
        } else if (b.body.CautionMotionEnded()) {
            b.StartCombat();
        }
    }
};

// 0x80180234..0x80180308: turn; sensor loss -> BASE GoHome;
// strict distance <30 -> NearAttack. Root motion is an engine service.
struct ChaseState final : EnemyAIState {
    const char* Name() const override { return "ChaseTarget"; }
    void Enter(EnemyBaseAI& ai) override { BK(ai).body.BeginChase(); }
    void Update(EnemyBaseAI& ai) override {
        auto& b = BK(ai);
        b.body.TurnTowards(b.body.TargetPosition());
        if (!b.Detects()) b.sm.ChangeState(&BkSoldierAI::BaseGoHome());
        else if (b.body.TargetDistance() < 30.0f)
            b.sm.ChangeState(&BkSoldierAI::NearAttack());
    }
};

// 0x8017FF00..0x80180088: sensor loss -> BASE GoHome; on motion end,
// strict distance >30 -> Chase, otherwise start another melee motion.
struct NearState final : EnemyAIState {
    const char* Name() const override { return "NearAttack"; }
    void Enter(EnemyBaseAI& ai) override { BK(ai).body.BeginNearAttack(); }
    void Update(EnemyBaseAI& ai) override {
        auto& b = BK(ai);
        b.body.TurnTowards(b.body.TargetPosition());
        if (!b.Detects()) { b.sm.ChangeState(&BkSoldierAI::BaseGoHome()); return; }
        if (!b.body.MeleeMotionEnded()) return;
        if (b.body.TargetDistance() > 30.0f) b.sm.ChangeState(&BkSoldierAI::ChaseTarget());
        else b.body.BeginNearAttack();
    }
};

// Shared EnemyBaseAI_GoHome 0x8019BF30: only Enemy::vf01 asks whether
// movement has reached home. It contains no re-detection call.
struct BaseHomeState final : EnemyAIState {
    const char* Name() const override { return "BaseGoHome"; }
    void Enter(EnemyBaseAI& ai) override { BK(ai).body.BeginBaseGoHome(); }
    void Update(EnemyBaseAI& ai) override {
        auto& b = BK(ai);
        if (b.body.BaseGoHomeReached()) ai.ReturnToWait();
    }
};

// BK-specific GoHome 0x8017F374: re-detects before checking XZ <10.
struct BkHomeState final : EnemyAIState {
    const char* Name() const override { return "BkGoHome"; }
    void Enter(EnemyBaseAI& ai) override { BK(ai).body.BeginBkGoHome(); }
    void Update(EnemyBaseAI& ai) override {
        auto& b = BK(ai);
        b.body.TurnTowards(b.home);
        if (b.Detects()) { b.sm.ChangeState(&BkSoldierAI::Caution()); return; }
        if (b.HomeDistanceXZ() < 10.0f) ai.ReturnToWait();
    }
};

// 0x8017F258: XZ <10 starts combat without a sensor test.
struct AppearHomeState final : EnemyAIState {
    const char* Name() const override { return "AppearHome"; }
    void Enter(EnemyBaseAI& ai) override { BK(ai).body.BeginAppearHome(); }
    void Update(EnemyBaseAI& ai) override {
        auto& b = BK(ai);
        b.body.TurnTowards(b.home);
        if (b.HomeDistanceXZ() < 10.0f) b.StartCombat();
    }
};

// Attack 0x8017FA80 and AttackVacuum 0x8017F5EC contain engine motion,
// weapon and random timing operations. The lost-target test follows those operations.
// The remaining work stays an explicit body service.
template<bool Vacuum> struct RangedState final : EnemyAIState {
    const char* Name() const override { return Vacuum ? "AttackVacuum" : "Attack"; }
    void Enter(EnemyBaseAI& ai) override { BK(ai).body.BeginRangedAttack(Vacuum); }
    void Update(EnemyBaseAI& ai) override {
        auto& b = BK(ai);
        b.body.UpdateRangedAttack(Vacuum);
        if (!b.Detects()) b.sm.ChangeState(&BkSoldierAI::BkGoHome());
    }
    void Exit(EnemyBaseAI& ai) override { BK(ai).body.EndRangedAttack(Vacuum); }
};
} // namespace

EnemyAIState& BkSoldierAI::Standing() { static StandingState s; return s; }
EnemyAIState& BkSoldierAI::Moving() { static MovingState s; return s; }
EnemyAIState& BkSoldierAI::Caution() { static CautionState s; return s; }
EnemyAIState& BkSoldierAI::ChaseTarget() { static ChaseState s; return s; }
EnemyAIState& BkSoldierAI::NearAttack() { static NearState s; return s; }
EnemyAIState& BkSoldierAI::BaseGoHome() { static BaseHomeState s; return s; }
EnemyAIState& BkSoldierAI::BkGoHome() { static BkHomeState s; return s; }
EnemyAIState& BkSoldierAI::AppearHome() { static AppearHomeState s; return s; }
EnemyAIState& BkSoldierAI::Attack() { static RangedState<false> s; return s; }
EnemyAIState& BkSoldierAI::AttackVacuum() { static RangedState<true> s; return s; }

float BkSoldierAI::HomeDistanceXZ() const {
    const Vec3 p = body.Position();
    const float dx = home.x - p.x, dz = home.z - p.z;
    return std::sqrt(dx * dx + dz * dz);
}

// EnemyBaseAI::vf0C 0x8019D894 calls virtual vf0D.
void BkSoldierAI::EnterInitialState() { ReturnToWait(); }

// 0x80180784..0x801808D0. Values outside [0,5] use Standing.
void BkSoldierAI::ReturnToWait() {
    if (appearType == 0 || appearType < 0 || appearType >= 6) sm.ChangeState(&Standing());
    else if (appearType <= 3) sm.ChangeState(&Moving());
    else if (appearedHome) sm.ChangeState(&Standing());
    else {
        appearedHome = true;             // AI+0x7C at 0x8018087C
        sm.ChangeState(&AppearHome());
    }
}

void BkSoldierAI::OnTargetFound() { sm.ChangeState(&Caution()); } // 0x80180738

// 0x80180574..0x801805C8.
void BkSoldierAI::StartCombat() {
    if (weaponType == 0 || weaponType == 1) sm.ChangeState(&ChaseTarget());
    else if (weaponType == 6) sm.ChangeState(&AttackVacuum());
    else sm.ChangeState(&Attack());
}

} // namespace shadow::gameplay

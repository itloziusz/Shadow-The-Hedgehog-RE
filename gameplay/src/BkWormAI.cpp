// Bounded native BkWormAI decision loop (PAL GUPP8P main.dol).
// DOL ranges and engine-service limits are in enemy/BK_WORM.md.
#include "shadow/gameplay/BkWormAI.hpp"

namespace shadow::gameplay {
namespace {
BkWormAI& Worm(EnemyBaseAI& ai) { return static_cast<BkWormAI&>(ai); }

struct WaitAppearState final : EnemyAIState {
    const char* Name() const override { return "WaitAppear"; }
    void Enter(EnemyBaseAI& ai) override { // 0x801835EC..0x801836A8
        auto& w = Worm(ai);
        w.body.BeginBuriedWait();
        w.buried = true;
        w.timer = w.timerReset = w.intervalSec;
        w.subStep = 0;
    }
    void Update(EnemyBaseAI& ai) override { // 0x8018342C..0x8018353C
        auto& w = Worm(ai);
        if (w.subStep == 0) {
            if (w.sensor.Detect(w.body.TargetPosition()) && w.timer < 0.0f) {
                w.body.PrepareEmergence(w.body.Position());
                w.timer = w.timerReset = 1.0f;
                ++w.subStep;
            }
        } else if (w.subStep == 1 && w.timer < 0.0f) {
            w.sm.ChangeState(&BkWormAI::Appear());
        }
    }
};

struct AppearState final : EnemyAIState {
    const char* Name() const override { return "Appear"; }
    void Enter(EnemyBaseAI& ai) override { // 0x801832FC..0x80183424
        auto& w = Worm(ai);
        w.body.BeginAppear();
        w.buried = false;
    }
    void Update(EnemyBaseAI& ai) override { // 0x801831D0..0x8018324C
        auto& w = Worm(ai);
        w.body.TurnTowards(w.body.TargetDirection());
        if (w.body.MotionEnded()) w.sm.ChangeState(&BkWormAI::WaitStanding());
    }
};

struct WaitStandingState final : EnemyAIState {
    const char* Name() const override { return "WaitStanding"; }
    void Enter(EnemyBaseAI& ai) override { // 0x80183144..0x801831C8
        auto& w = Worm(ai);
        w.timer = w.timerReset = w.body.Uniform(1.0f, 3.0f);
        w.body.BeginStanding();
    }
    void Update(EnemyBaseAI& ai) override { // 0x801830BC..0x80183140
        auto& w = Worm(ai);
        w.body.TurnTowards(w.body.TargetDirection());
        if (w.timer < 0.0f) {
            w.remainingAttacks = w.attackTimes;
            w.sm.ChangeState(&BkWormAI::Attack());
        }
    }
};

struct AttackState final : EnemyAIState {
    const char* Name() const override { return "Attack"; }
    void Enter(EnemyBaseAI& ai) override { Worm(ai).body.BeginAttack(); } // 0x80183074
    void Update(EnemyBaseAI& ai) override { // 0x80182F48..0x80182FC4
        auto& w = Worm(ai);
        w.body.TurnTowards(w.body.TargetDirection());
        if (w.body.MotionEnded()) w.sm.ChangeState(&BkWormAI::AfterAttack());
    }
};

struct AfterAttackState final : EnemyAIState {
    const char* Name() const override { return "AfterAttack"; }
    void Enter(EnemyBaseAI& ai) override { // 0x80182EBC..0x80182F40
        auto& w = Worm(ai);
        w.timer = w.timerReset = w.body.Uniform(0.8f, 1.2f);
        w.body.BeginAfterAttack();
    }
    void Update(EnemyBaseAI& ai) override { // 0x80182CBC..0x80182D60
        auto& w = Worm(ai);
        w.body.TurnTowards(w.body.TargetDirection());
        if (w.timer < 0.0f) {
            --w.remainingAttacks;
            if (w.remainingAttacks < 1) w.sm.ChangeState(&BkWormAI::Move());
            else w.sm.ChangeState(&BkWormAI::Attack());
        }
    }
};

struct MoveState final : EnemyAIState {
    const char* Name() const override { return "Move"; }
    void Exit(EnemyBaseAI& ai) override { Worm(ai).sm.Unlock(); } // 0x8018293C
    void Enter(EnemyBaseAI& ai) override { // 0x80182BF8..0x80182CB4
        auto& w = Worm(ai);
        w.sm.Lock();
        w.timer = w.timerReset = w.body.Uniform(0.8f, 1.2f);
        w.home = w.body.BeginMove();
        w.subStep = 0;
    }
    void Update(EnemyBaseAI& ai) override { // 0x80182964..0x80182B40
        auto& w = Worm(ai);
        if (w.subStep == 0) {
            const Vec3 pos = w.body.Position();
            const Vec3 direction{w.home.x - pos.x, w.home.y - pos.y, w.home.z - pos.z};
            if (w.body.TurnTowards(direction)) {
                w.body.BeginMoveTravel();
                w.buried = true;
                ++w.subStep;
            }
        } else if (w.subStep == 1 && w.body.MotionEnded()) {
            w.body.CompleteMoveTravel();
            w.sm.Unlock();
            w.sm.ChangeState(&BkWormAI::WaitAppear());
        }
    }
};
} // namespace

EnemyAIState& BkWormAI::WaitAppear() { static WaitAppearState s; return s; }
EnemyAIState& BkWormAI::Appear() { static AppearState s; return s; }
EnemyAIState& BkWormAI::WaitStanding() { static WaitStandingState s; return s; }
EnemyAIState& BkWormAI::Attack() { static AttackState s; return s; }
EnemyAIState& BkWormAI::AfterAttack() { static AfterAttackState s; return s; }
EnemyAIState& BkWormAI::Move() { static MoveState s; return s; }

void BkWormAI::EnterInitialState() { // 0x801838E4..0x8018393C
    body.InitialSetup();
    sm.ChangeState(&WaitAppear());
}

void BkWormAI::ReturnToWait() { // 0x8018388C..0x801838E0
    sm.ChangeState(buried ? &WaitAppear() : &WaitStanding());
}

} // namespace shadow::gameplay

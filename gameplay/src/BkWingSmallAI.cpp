#include "shadow/gameplay/BkWingSmallAI.hpp"

namespace shadow::gameplay {
namespace {
BkWingSmallAI& Wing(EnemyBaseAI& ai) { return static_cast<BkWingSmallAI&>(ai); }

// Inherited original RTTI EnemyAIState_FlyerCommonAI_WaitFloating.
struct WaitFloatingState final : EnemyAIState {
    const char* Name() const override { return "WaitFloating"; }
    void Enter(EnemyBaseAI& ai) override { Wing(ai).body.EnterFloatingMotion(); } // 0x80184270
    void Update(EnemyBaseAI& ai) override { // 0x80183F30
        auto& w = Wing(ai);
        if (w.Detects()) {
            w.body.TurnTowards(w.body.TargetPosition()); // 0x80183F88..0x80183FA0
            if (w.HasAttackWindow()) { ai.OnTargetFound(); return; } // 0x80183FA4..0x80183FD8
        } else {
            w.body.FaceSpawnHeading(); // 0x80183FDC..0x80184024
        }
        if (w.floatWidth > 0.1f) w.body.UpdateFloatMotion(ai.dt); // 0x80184028..0x8018423C
    }
};

// Original RTTI EnemyAIState_BkWingSmallAI_MoveOnPath.
struct MoveOnPathState final : EnemyAIState {
    const char* Name() const override { return "MoveOnPath"; }
    void Enter(EnemyBaseAI& ai) override { // 0x80181E68
        auto& w = Wing(ai);
        ai.subStep = 0;
        w.body.EnterPathMotion();
        w.body.SetPathScales(1.0f, 1.0f); // E+0x2F8/+0x2FC
    }
    void Update(EnemyBaseAI& ai) override { // 0x80181D3C
        auto& w = Wing(ai);
        if (ai.subStep == 0 && w.InAttackWindow()) {
            ++ai.subStep;                              // 0x80181D74..0x80181D80
            ai.sm.ChangeState(&BkWingSmallAI::Caution()); // 0x80181D84..0x80181D94
        }
        // The original advances the path even when the state changed above.
        w.body.AdvanceOnPath(ai.dt);                    // 0x80181D98..0x80181DA4
    }
};

// Original RTTI EnemyAIState_BkWingSmallAI_Caution.
struct CautionState final : EnemyAIState {
    const char* Name() const override { return "Caution"; }
    void Enter(EnemyBaseAI& ai) override { Wing(ai).body.EnterCautionMotion(); } // 0x80182518
    void Update(EnemyBaseAI& ai) override { // 0x801823EC
        auto& w = Wing(ai);
        w.body.TurnTowards(w.body.TargetPosition());     // 0x80182400..0x80182418
        if (w.body.MotionFlagA())                         // 0x8018241C..0x80182440
            ai.sm.ChangeState(&BkWingSmallAI::Attack()); // 0x80182444..0x80182454
    }
};

// Original RTTI EnemyAIState_BkWingSmallAI_Attack. Its entry is bounded;
// its movement/transform update remains an explicit engine hook.
struct AttackState final : EnemyAIState {
    const char* Name() const override { return "Attack"; }
    void Enter(EnemyBaseAI& ai) override { // 0x801822F8..0x801823D0
        Wing(ai).body.EnterAttackMotion();
        ai.timer = ai.timerReset = 3.0f;
        ai.subStep = 0;
    }
    void Update(EnemyBaseAI& ai) override { Wing(ai).body.UpdateAttackMotion(ai.dt); }
};
} // namespace

EnemyAIState& BkWingSmallAI::WaitFloating() { static WaitFloatingState s; return s; }
EnemyAIState& BkWingSmallAI::MoveOnPath() { static MoveOnPathState s; return s; }
EnemyAIState& BkWingSmallAI::Caution() { static CautionState s; return s; }
EnemyAIState& BkWingSmallAI::Attack() { static AttackState s; return s; }

void BkWingSmallAI::ConfigurePath(float pathLength, float baseSpeed, float moveSpeedRatio,
                                  float attackStart, float attackEnd) {
    pathSpeed = moveSpeedRatio * baseSpeed;  // 0x80193BBC..0x80193BCC
    windowStart = attackStart * pathLength; // 0x80193C24..0x80193C34
    windowEnd = attackEnd * pathLength;     // 0x80193C38..0x80193C44
    windowDuration = pathSpeed > 0.1f
        ? ((attackEnd - attackStart) * pathLength) / pathSpeed : 1.0f; // 0x80193C48..0x80193C78
    pathConfigured = true;
}

bool BkWingSmallAI::InAttackWindow() const { // 0x80193AD4..0x80193B30
    if (!pathConfigured || !HasAttackWindow()) return false;
    const float d = body.PathDistance();
    return windowStart <= d && d <= windowEnd; // inclusive, both ends
}

void BkWingSmallAI::EnterInitialState() { ReturnToWait(); } // EnemyBaseAI::vf0C
void BkWingSmallAI::ReturnToWait() { // 0x801825D0: E+0x32C chooses path/float
    sm.ChangeState(UsesPath() ? &MoveOnPath() : &WaitFloating());
}
void BkWingSmallAI::OnTargetFound() { sm.ChangeState(&Caution()); } // 0x80182598

void BkWingSmallAI::OnDead() { // FlyerCommonAI::OnDead 0x80184318
    body.PlayDeathEffect();
    body.RequestDelete();
}

} // namespace shadow::gameplay

#include "shadow/gameplay/GunBeetleAI.hpp"

namespace shadow::gameplay {
namespace {
GunBeetleAI& GB(EnemyBaseAI& ai) { return static_cast<GunBeetleAI&>(ai); }

// Original RTTI EnemyAIState_FlyerCommonAI_WaitFloating, Update 0x80183F30.
struct WaitFloatingState final : EnemyAIState {
    const char* Name() const override { return "WaitFloating"; }
    void Enter(EnemyBaseAI& ai) override { GB(ai).body.EnterFloatingMotion(); } // 0x80184270
    void Update(EnemyBaseAI& ai) override {
        auto& g = GB(ai);
        if (g.Detects()) {                                      // 0x80183F7C..0x80183F84
            g.body.AimAt(g.body.TargetPosition());             // 0x80183F88..0x80183FA0
            if (g.HasAttackWindow()) { ai.OnTargetFound(); return; } // 0x80183FA4..0x80183FD8
        } else {
            g.body.FaceSpawnHeading();                         // 0x80183FDC..0x80184024
        }
        if (g.floatWidth > 0.1f) g.body.UpdateFloatMotion(ai.dt); // 0x80184028..0x8018423C
    }
};

// Original RTTI EnemyAIState_FlyerCommonAI_MoveOnPath, Update 0x80183CC4.
struct MoveOnPathState final : EnemyAIState {
    const char* Name() const override { return "MoveOnPath"; }
    void Enter(EnemyBaseAI& ai) override {
        auto& g = GB(ai);
        ai.subStep = 0;                                       // 0x80183EA4..0x80183EA8
        g.onPath = true;                                      // 0x80183F0C..0x80183F14
        g.body.EnterPathMotion();                             // 0x80183EAC..0x80183EF0
    }
    void Update(EnemyBaseAI& ai) override {
        auto& g = GB(ai);
        if (ai.subStep == 0) {
            if (g.InAttackWindow()) {                         // 0x80183CF8..0x80183D04
                ai.subStep = 1;
                g.body.StartFiring();                         // 0x80183D18..0x80183D2C
                ai.timer = ai.timerReset = g.windowDuration; // 0x80183D30..0x80183D44
            }
        } else if (ai.subStep == 1) {
            Vec3 aim = g.body.TargetPosition();               // 0x80183D4C..0x80183D60
            const float fraction = ai.timerReset > 0.01f ? ai.timer / ai.timerReset : 0.0f;
            aim.y += 10.0f * (1.0f - fraction);              // 0x80183D64..0x80183D9C
            g.body.AimAt(aim);                                // 0x80183DA0..0x80183DA8
            if (!g.InAttackWindow()) {                        // 0x80183DAC..0x80183DBC
                ai.subStep = 0;
                g.body.StopFiring();                          // 0x80183DC0..0x80183DE4
            }
        }
        // Window decisions use the distance at frame start; path movement comes last.
        // The path sampler/geometry remains an explicit engine hook.
        g.body.AdvanceOnPath(ai.dt);                          // 0x80183DE8..0x80183DF4
    }
    void Exit(EnemyBaseAI& ai) override { GB(ai).onPath = false; } // 0x80183CB0
};

// Original RTTI EnemyAIState_GunBeetleAI_Caution, Update 0x80184784.
struct CautionState final : EnemyAIState {
    const char* Name() const override { return "Caution"; }
    void Enter(EnemyBaseAI& ai) override { ai.timer = ai.timerReset = 1.0f; } // 0x801848C8
    void Update(EnemyBaseAI& ai) override {
        auto& g = GB(ai);
        Vec3 aim = g.body.TargetPosition();
        aim.y += 10.0f;
        g.body.AimAt(aim);                                    // 0x80184798..0x801847CC
        if (ai.timer < 0.0f) ai.sm.ChangeState(&GunBeetleAI::Attack()); // 0x801847D0..0x80184804
    }
};

// Original RTTI EnemyAIState_GunBeetleAI_Attack, Update 0x80184574.
struct AttackState final : EnemyAIState {
    const char* Name() const override { return "Attack"; }
    void Enter(EnemyBaseAI& ai) override {
        ai.timer = ai.timerReset = 3.0f;                     // 0x80184730..0x8018474C
        ai.subStep = 0;
        GB(ai).body.StartFiring();                            // 0x80184758..0x8018476C
    }
    void Update(EnemyBaseAI& ai) override {
        auto& g = GB(ai);
        if (!g.Detects()) {                                   // 0x80184588..0x801845B8
            ai.sm.ChangeState(&GunBeetleAI::WaitFloating()); // 0x801845BC..0x801845FC
            return;
        }
        Vec3 aim = g.body.TargetPosition();
        aim.y += 10.0f;
        g.body.AimAt(aim);                                    // 0x80184600..0x80184634
        if (ai.timer < 0.0f) ai.sm.ChangeState(&GunBeetleAI::Caution()); // 0x80184638..0x8018466C
    }
    void Exit(EnemyBaseAI& ai) override { GB(ai).body.StopFiring(); } // 0x80184540
};
} // namespace

EnemyAIState& GunBeetleAI::WaitFloating() { static WaitFloatingState s; return s; }
EnemyAIState& GunBeetleAI::MoveOnPath() { static MoveOnPathState s; return s; }
EnemyAIState& GunBeetleAI::Caution() { static CautionState s; return s; }
EnemyAIState& GunBeetleAI::Attack() { static AttackState s; return s; }

void GunBeetleAI::ConfigurePath(float pathLength, float baseSpeed, float moveSpeedRatio,
                                float attackStart, float attackEnd) {
    pathSpeed = moveSpeedRatio * baseSpeed;             // 0x80193BBC..0x80193BCC
    windowStart = attackStart * pathLength;            // 0x80193C24..0x80193C34
    windowEnd = attackEnd * pathLength;                // 0x80193C38..0x80193C44
    windowDuration = pathSpeed > 0.1f
        ? ((attackEnd - attackStart) * pathLength) / pathSpeed : 1.0f; // 0x80193C48..0x80193C78
    pathConfigured = true;
}

// FlyerCommon_IsInAttackWindow 0x80193AD4..0x80193B30: both boundaries inclusive.
bool GunBeetleAI::InAttackWindow() const {
    // The DOL helper tests ActionType 2 and the two inclusive distances; it does
    // not test AppearType. Only MoveOnPath calls it in the recovered AI path.
    if (!pathConfigured || !HasAttackWindow()) return false;
    const float d = body.PathDistance();
    return windowStart <= d && d <= windowEnd;
}

// EnemyBaseAI::Init calls vf0C; GunBeetleAI inherits the base delegation to vf0D.
void GunBeetleAI::EnterInitialState() { ReturnToWait(); }
void GunBeetleAI::ReturnToWait() { sm.ChangeState(UsesPath() ? &MoveOnPath() : &WaitFloating()); }
void GunBeetleAI::OnTargetFound() { sm.ChangeState(&Caution()); } // 0x801848DC

// FlyerCommonAI::OnDead 0x80184318: effects and delete request in the same frame,
// no Dead state or fabricated death motion.
void GunBeetleAI::OnDead() {
    body.PlayDeathEffect();
    body.RequestDelete();
}

} // namespace shadow::gameplay

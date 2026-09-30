// GunBeetleAI / FlyerCommonAI decision reconstruction for PAL GUPP8P.
// Original RTTI state names are retained. Names of methods are recovered semantics.
#pragma once

#include "shadow/gameplay/EnemyAI.hpp"

namespace shadow::gameplay {

// Engine-facing operations. Path sampling, floating motion, display, weapon mount and
// effect implementations are not inferred by this native AI module.
struct GunBeetleBody {
    virtual ~GunBeetleBody() = default;
    virtual Vec3 TargetPosition() const = 0;
    virtual void AimAt(const Vec3& position) = 0;       // AI+0x60, fn_8019F69C
    virtual void FaceSpawnHeading() = 0;               // 0x80183FDC..0x80184024
    virtual void UpdateFloatMotion(float dt) = 0;      // 0x80184028..0x8018423C; engine transform hook
    virtual void EnterFloatingMotion() = 0;            // 0x80184270; display/root-motion engine hook
    virtual void EnterPathMotion() = 0;                // 0x80183E8C; display/root-motion engine hook
    virtual float PathDistance() const = 0;            // E+0x2E4; path geometry lives in engine
    virtual void AdvanceOnPath(float dt) = 0;          // fn_80193814, called last in path Update
    virtual void StartFiring() = 0;                    // flyer base vslot +0x244
    virtual void StopFiring() = 0;                     // flyer base vslot +0x248
    virtual void PlayDeathEffect() = 0;                // EnemyDispCommon vslot +0x4C
    virtual void RequestDelete() = 0;                 // TEnemySetBase vslot +0x10
};

class GunBeetleAI final : public EnemyBaseAI {
public:
    GunBeetleAI(GunBeetleBody& body, const EnemySensor& sensor,
                int appearType, int actionType)
        : body(body), sensor(sensor), appearType(appearType), actionType(actionType) {}

    // GunBeetleBase::UsesPath 0x801849D4; FlyerCommonBase::UpdateActivation
    // 0x80193CAC. The caller applies the resulting visibility/activity to the engine.
    bool UsesPath() const { return appearType == 1; }
    bool ShouldActivate() const { return !UsesPath() || Detects(); }
    bool Detects() const { return sensor.Detect(body.TargetPosition()); }
    bool HasAttackWindow() const { return actionType == 2; } // 0x80193DC0

    // FlyerCommonBase_ReadSetParams 0x80193C24..0x80193C78. The engine supplies
    // pathLength and baseSpeed after loading EnemyPath.one; no path is fabricated here.
    void ConfigurePath(float pathLength, float baseSpeed, float moveSpeedRatio,
                       float attackStart, float attackEnd);
    void ConfigureFloat(float width) { floatWidth = width; } // E+0x2DC, param 10
    bool InAttackWindow() const;                        // fn_80193AD4

    void EnterInitialState() override;                 // EnemyBaseAI::vf0C -> vf0D
    void ReturnToWait() override;                      // 0x80184914
    void OnTargetFound() override;                     // 0x801848DC
    void OnDead() override;                            // FlyerCommonAI::OnDead 0x80184318

    static EnemyAIState& WaitFloating();               // original RTTI state 0x805F056C
    static EnemyAIState& MoveOnPath();                 // original RTTI state 0x805F0578
    static EnemyAIState& Caution();                    // original RTTI state 0x805F0624
    static EnemyAIState& Attack();                     // original RTTI state 0x805F0630

    GunBeetleBody& body;
    const EnemySensor& sensor;
    int appearType = 0;                                // E+0x32C
    int actionType = 0;                                // E+0x330
    bool pathConfigured = false;                       // native guard for missing engine path
    bool onPath = false;                               // E+0x30C
    float pathSpeed = 0.0f;                            // E+0x2E0
    float windowStart = 0.0f, windowEnd = 0.0f;       // E+0x2E8/+0x2EC
    float windowDuration = 1.0f;                       // E+0x2F0
    float floatWidth = 0.0f;                           // E+0x2DC
};

} // namespace shadow::gameplay

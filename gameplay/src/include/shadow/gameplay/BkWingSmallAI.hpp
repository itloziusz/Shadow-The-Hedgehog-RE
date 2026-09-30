// PAL GUPP8P BkWingSmallAI decision slice. RTTI class/state names are original;
// method names are recovered semantics. See enemy/BK_WINGSMALL.md.
#pragma once

#include "shadow/gameplay/EnemyAI.hpp"

namespace shadow::gameplay {

// Engine services required by the recovered AI. These methods do not assert that
// motion, path geometry, transforms, audio or attack displacement are native.
struct BkWingSmallBody {
    virtual ~BkWingSmallBody() = default;
    virtual Vec3 TargetPosition() const = 0;
    virtual void TurnTowards(const Vec3& position) = 0; // 0x80183F9C, 0x80182414
    virtual void FaceSpawnHeading() = 0;                // 0x80183FDC..0x80184024
    virtual void UpdateFloatMotion(float dt) = 0;       // 0x80184028..0x8018423C
    virtual void EnterFloatingMotion() = 0;             // 0x80184270; includes engine display setup
    virtual void EnterPathMotion() = 0;                 // 0x80181E88..0x80181ECC
    virtual void SetPathScales(float a, float b) = 0;  // E+0x2F8/+0x2FC at 0x80181ED0
    virtual float PathDistance() const = 0;            // E+0x2E4, engine path geometry
    virtual void AdvanceOnPath(float dt) = 0;          // 0x80181D98..0x80181DA4
    virtual void EnterCautionMotion() = 0;             // 0x8018252C..0x80182580, SE 0x603F
    virtual bool MotionFlagA() const = 0;              // S+0x3A, 0x8018241C..0x80182440
    virtual void EnterAttackMotion() = 0;              // 0x801822F8..0x801823B4, SE 0x603D
    virtual void UpdateAttackMotion(float dt) = 0;    // UNRECOVERED here: 0x80181EFC..0x801822B8
    virtual void PlayDeathEffect() = 0;                // FlyerCommonAI::OnDead 0x80184318
    virtual void RequestDelete() = 0;                 // FlyerCommonAI::OnDead 0x80184318
};

class BkWingSmallAI final : public EnemyBaseAI {
public:
    BkWingSmallAI(BkWingSmallBody& body, const EnemySensor& sensor,
                  int appearType, int actionType)
        : body(body), sensor(sensor), appearType(appearType), actionType(actionType) {}

    bool UsesPath() const { return appearType == 1; }       // 0x801822E4, E+0x32C
    bool HasAttackWindow() const { return actionType == 1; } // 0x8019162C, E+0x330
    bool Detects() const { return sensor.Detect(body.TargetPosition()); }
    bool ShouldActivate() const { return !UsesPath() || Detects(); } // 0x80193CAC

    // The engine must supply path length and base speed after EnemyPath setup.
    // The arithmetic is FlyerCommonBase_ReadSetParams 0x80193BBC..0x80193C78.
    void ConfigurePath(float pathLength, float baseSpeed, float moveSpeedRatio,
                       float attackStart, float attackEnd);
    void ConfigureFloat(float width) { floatWidth = width; } // E+0x2DC
    bool InAttackWindow() const;                             // 0x80193AD4

    void EnterInitialState() override;                       // base delegates to vf0D
    void ReturnToWait() override;                            // 0x801825D0
    void OnTargetFound() override;                           // 0x80182598
    void OnDead() override;                                  // 0x80184318 inherited

    static EnemyAIState& WaitFloating(); // RTTI EnemyAIState_FlyerCommonAI_WaitFloating
    static EnemyAIState& MoveOnPath();   // RTTI EnemyAIState_BkWingSmallAI_MoveOnPath
    static EnemyAIState& Caution();      // RTTI EnemyAIState_BkWingSmallAI_Caution
    static EnemyAIState& Attack();       // RTTI EnemyAIState_BkWingSmallAI_Attack

    BkWingSmallBody& body;
    const EnemySensor& sensor;
    int appearType = 0;
    int actionType = 0;
    bool pathConfigured = false; // native guard: no invented window without engine path
    float pathSpeed = 0.0f;
    float windowStart = 0.0f, windowEnd = 0.0f, windowDuration = 1.0f;
    float floatWidth = 0.0f;
};

} // namespace shadow::gameplay

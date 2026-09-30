// Native EggPawn / EnemyWalkerAI decision reconstruction, PAL GUPP8P.
// Evidence: enemy/EGG_PAWN.md §5 and the DOL addresses beside each method.
// RTTI class names are original; method names below describe recovered semantics.
#pragma once
#include "shadow/gameplay/EnemyAI.hpp"

namespace shadow::gameplay {

// The scene's motion, target-reference, movement and random-point services are not
// reconstructed here. Implementations must supply them; no AI outcome is invented.
struct EggPawnBody {
    virtual ~EggPawnBody() = default;
    virtual bool DetectNearestPlayer(const EnemySensor&) = 0; // 0x8019B2C0, also binds target
    virtual bool TrackedTargetInSearchArea(const EnemySensor&) const = 0; // 0x801A6C7C
    virtual Vec3 TrackedTargetPosition() const = 0; // EnemyAITarget_GetPosition 0x8019DF7C
    virtual float TrackedTargetDistance() const = 0; // EnemyAITarget_GetDistance 0x8019DEAC
    virtual void FaceTrackedTarget() = 0; // fn_8019B46C
    virtual void BeginStandingWait() = 0; // shared walker Enter 0x8019A60C
    virtual void TickStandingWait() = 0;  // remaining shared walker Update 0x8019A300
    virtual void BeginMovingWait() = 0;   // shared walker Enter 0x8019A218
    virtual void TickMovingWait() = 0;    // remaining patrol Update 0x80199E90
    virtual void BeginCaution() = 0;      // motion from enemy vslot +0x54, 0x80199E0C
    virtual void BeginReturnHome(bool dash) = 0; // walker 0x8019A7D8 / 0x8019A6FC
    virtual void SetMoveTarget(const Vec3&) = 0; // Enemy_SetHomePosition 0x8017E1C4
    virtual bool AdvanceTowardMoveTarget() = 0; // Enemy::MoveTowardHome 0x8019B0C8;
                                                // this call also drives movement/turning
    virtual bool MotionEnded() const = 0;       // display vslot +0x2C
    virtual void SetMotion(int motion, bool restart) = 0; // EnemyStatusCommon::SetMotion 0x8017B9D0
    virtual bool HasShield() const = 0;         // E+0x270, 0x8017CC68
    virtual Vec3 AttackPoint(float parameter) = 0; // EnemyBaseAI::GetAttackPoint 0x8019DA1C;
                                                   // projection geometry unresolved
    virtual Vec3 PickRandomPointNear(float radius) = 0; // 0x8019AE98; RNG/terrain service
};

class EggPawnAI final : public EnemyBaseAI {
public:
    EggPawnAI(EggPawnBody& body, const EnemySensor& sensor, int appearType,
              int weaponType, int waitActMoveType)
        : body(body), sensor(sensor), appearType(appearType), weaponType(weaponType),
          waitActMoveType(waitActMoveType) {}

    void EnterInitialState() override; // 0x8017ECD0
    void ReturnToWait() override;      // 0x8017EB58
    void OnTargetFound() override;     // inherited walker 0x8019A85C
    void StartAttack();                // AI vslot 0x10, 0x8017E8D4
    int AttackMotionId() const;        // EggPawnBase::vf1E 0x8017CB50
    int AimMotionId() const;           // EggPawnBase::vf1B 0x8017CBD8
    int ChargePauseMotionId() const;   // EggPawnBase::vf1A 0x8017CC20

    EggPawnBody& body;
    const EnemySensor& sensor;
    int appearType;       // E+0x298; 0 WAIT_ACT, 1 OFFSET, 2 WARP, 3 DASH
    int weaponType;       // E+0x294; 0 none, 1 autorifle, 2 bazooka, 3 lance
    int waitActMoveType;  // E+0x2A8; 0 standing, nonzero moving

    static EnemyAIState& Standing();       // original RTTI EnemyWalkerAI_WaitAction_Standing
    static EnemyAIState& Moving();         // EnemyWalkerAI_WaitAction_Moving
    static EnemyAIState& Caution();        // EnemyWalkerAI_Caution
    static EnemyAIState& ReturnHome();     // EnemyWalkerAI_ReturnHome
    static EnemyAIState& ReturnHomeDash(); // EnemyWalkerAI_ReturnHomeDash
    static EnemyAIState& WeaponAttack();   // EggPawnAI_WeaponAttack
    static EnemyAIState& TukiAttack();     // EggPawnAI_TukiAttack
    static EnemyAIState& DashAttack();     // EggPawnAI_DashAttack
};

} // namespace shadow::gameplay

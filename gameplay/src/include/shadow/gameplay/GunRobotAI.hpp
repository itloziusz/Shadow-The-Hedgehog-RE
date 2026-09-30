// Bounded GUN Robot AI reconstruction, PAL GUPP8P main.dol.
// DOL evidence and confidence: enemy/GUN_ROBOT.md.
#pragma once
#include "shadow/gameplay/EnemyAI.hpp"

namespace shadow::gameplay {

class GunRobotAI;

// Motion, target acquisition, path movement and the shared base Caution state
// remain engine services. These calls must not silently invent their outcomes.
struct GunRobotBody {
    virtual ~GunRobotBody() = default;
    virtual bool DetectNearestPlayer(const EnemySensor&) = 0; // 0x8019B2C0
    virtual void BeginStandingWait() = 0;        // 0x8019A60C
    virtual void TickStandingWait() = 0;         // 0x8019A300 after detection
    virtual void BeginMovingWait() = 0;          // 0x8019A218
    virtual void TickMovingWait() = 0;           // 0x80199E90 after detection
    virtual void BeginReturnHome() = 0;          // 0x8019A7D8
    virtual bool AdvanceTowardMoveTarget() = 0;  // Enemy::MoveTowardHome 0x8019B0C8
    virtual void SetMoveTarget(const Vec3&) = 0; // Enemy_SetHomePosition call 0x801863E4
    virtual Vec3 SetPosition() const = 0;        // E+0x48
    virtual Vec3 Position() const = 0;           // Enemy_GetPosition
    virtual Vec3 PickRandomPointNear(const Vec3& centre, float radius) = 0; // 0x8019AE98
    virtual void SetMotion(int motion, bool restart) = 0; // 0x8017B9D0
    virtual bool MotionEnded() const = 0;        // display vslot +0x2C
    virtual bool TrackedTargetInSearchArea(const EnemySensor&, const Vec3&) const = 0; // 0x801A6C7C
    virtual Vec3 TrackedTargetPosition() const = 0; // 0x8019DF7C
    virtual void FaceTrackedTarget() = 0;       // 0x8019B46C
    virtual bool TurnToTargetYaw() = 0;         // 0x8019AFB0, turns before result
    virtual void BeginBaseCaution() = 0;        // shared state 0x8019C780
    virtual void UpdateBaseCaution(GunRobotAI&) = 0; // shared state 0x8019C544
    virtual void ApplyDeathEffects() = 0;       // GunRobotAI::vf14 0x80186588
};

class GunRobotAI final : public EnemyBaseAI {
public:
    GunRobotAI(GunRobotBody& body, const EnemySensor& sensor, int appearType,
               int weaponType, int waitActMoveType)
        : body(body), sensor(sensor), appearType(appearType),
          weaponType(weaponType), waitActMoveType(waitActMoveType) {}

    void EnterInitialState() override; // GunRobotAI::vf0C 0x8018684C
    void ReturnToWait() override;      // GunRobotAI::vf0D 0x8018678C
    void OnTargetFound() override;     // GunRobotAI::vf0F 0x801866CC
    void OnDead() override;            // GunRobotAI::vf14 0x80186588 (no Dead state)
    void StartAttack();                // GunRobotAI::vf10 0x801865E8
    int AttackMotionId() const;        // GunRobot::vf21 0x80195A50

    GunRobotBody& body;
    const EnemySensor& sensor;
    int appearType;                    // E+0x28C, SET index 7
    int weaponType;                    // E+0x290, SET index 8
    int waitActMoveType;               // E+0x2A4, SET index 13

    static EnemyAIState& Standing();
    static EnemyAIState& Moving();
    static EnemyAIState& ReturnHome();
    static EnemyAIState& BaseCaution();
    static EnemyAIState& WeaponAttack();
};

} // namespace shadow::gameplay

// BK Soldier AI decisions recovered from PAL GUPP8P main.dol.
// Addresses and confidence are recorded in enemy/BK_SOLDIER.md.
// Engine motion, locomotion and weapon services remain explicit hooks.
#pragma once
#include "shadow/gameplay/EnemyAI.hpp"

namespace shadow::gameplay {

struct BkSoldierBody {
    virtual ~BkSoldierBody() = default;
    virtual Vec3 Position() const = 0;
    virtual Vec3 TargetPosition() const = 0;       // EnemyAITarget_GetPosition
    virtual void TurnTowards(const Vec3&) = 0;     // EnemyAI_TurnTowards
    virtual float TargetDistance() const = 0;      // EnemyAITarget_GetDistance (engine hook)
    virtual bool CautionMotionEnded() const = 0;   // display vslot +0x28
    virtual bool MeleeMotionEnded() const = 0;     // display vslot +0x2C
    virtual void BeginStandingWait() = 0;          // shared SoldierCommonAI state
    virtual void BeginMovingWait() = 0;            // shared SoldierCommonAI state
    virtual void UpdateSharedPatrol() = 0;         // 0x80188B98; shared patrol engine service
    virtual void BeginCaution() = 0;               // 0x801804E0, motion param 0x13
    virtual void BeginChase() = 0;                 // 0x801803C8, motion param 0x11
    virtual void BeginNearAttack() = 0;            // 0x801801A4, variant from global frame bit
    virtual void BeginBaseGoHome() = 0;            // 0x8019BF90
    virtual bool BaseGoHomeReached() const = 0;   // Enemy::vf01, called at 0x8019BF50
    virtual void BeginBkGoHome() = 0;              // 0x8017F50C
    virtual void BeginAppearHome() = 0;            // 0x8017F2EC
    // The ranged state includes motion/root-motion/weapon event operations that remain
    // outside this native AI. Implementations must provide these services explicitly.
    virtual void BeginRangedAttack(bool vacuum) = 0;
    virtual void UpdateRangedAttack(bool vacuum) = 0;
    virtual void EndRangedAttack(bool vacuum) = 0;
};

class BkSoldierAI final : public EnemyBaseAI {
public:
    BkSoldierAI(BkSoldierBody& body, const EnemySensor& sensor, int appearType, int weaponType)
        : body(body), sensor(sensor), appearType(appearType), weaponType(weaponType),
          home(sensor.pos) {}

    void EnterInitialState() override;  // base vf0C 0x8019D894 -> vf0D
    void ReturnToWait() override;       // 0x80180770
    void OnTargetFound() override;      // 0x80180738
    void StartCombat();                  // 0x80180560

    bool Detects() const { return sensor.Detect(body.TargetPosition()); }
    float HomeDistanceXZ() const;

    BkSoldierBody& body;
    const EnemySensor& sensor;
    int appearType;                     // E+0x274
    int weaponType;                     // E+0x278
    Vec3 home;                          // E+0x6C, initial SET position
    bool appearedHome = false;          // AI+0x7C

    static EnemyAIState& Standing();
    static EnemyAIState& Moving();
    static EnemyAIState& Caution();
    static EnemyAIState& ChaseTarget();
    static EnemyAIState& NearAttack();
    static EnemyAIState& BaseGoHome();
    static EnemyAIState& BkGoHome();
    static EnemyAIState& AppearHome();
    static EnemyAIState& Attack();
    static EnemyAIState& AttackVacuum();
};

}  // namespace shadow::gameplay

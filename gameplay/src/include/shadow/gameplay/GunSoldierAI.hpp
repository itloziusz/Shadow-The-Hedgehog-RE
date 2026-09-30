// Native reconstruction of GunSoldier::GunSoldierAI decision logic (Shadow the Hedgehog GC).
// Evidence: enemy/GUN_SOLDIER.md, notes/enemy_gunsoldier_trace.md §2.4-§2.6.
// Only the state machine / transition rules are reconstructed. Animation, turning and locomotion
// are engine services behind GunSoldierBody (the original drives them through Disp/Move components);
// motion ids are passed through as opaque numbers where the trace recorded them.
#pragma once
#include "shadow/gameplay/EnemyAI.hpp"
#include <cstddef>
#include <cstdint>

namespace shadow::gameplay {

// Services the AI needs from the soldier body / world (implemented by the engine layer).
struct GunSoldierBody {
    virtual ~GunSoldierBody() = default;
    virtual Vec3 Position() const = 0;               // E+0x48 region
    virtual Vec3 TargetPosition() const = 0;         // EnemyBaseAI::vf02 0x8019DF7C (nearest player)
    virtual bool TurnTowards(const Vec3& p) = 0;     // fn_8019F8F8: returns |dYaw| < 1 deg
    virtual float YawErrorDeg(const Vec3& p) const = 0;
    virtual void MoveTowards(const Vec3& p) = 0;     // walk request (Move component)
    virtual void Stop() = 0;
    virtual void StartFireMotion() = 0;              // Attack fire motion; the shot itself is emitted by
                                                     // motion event 0x5000 in GunSoldierBase::vf18 0x80197CF4
    // Engine-facing patrol effects. Defaults are explicitly unimplemented engine stubs; a body
    // that simulates moving soldiers must supply them. See GUN_SOLDIER.md patrol section.
    virtual void SetPatrolTarget(const Vec3&) {}       // Enemy_SetHomePosition 0x8017E1C4, E+0x6C
    virtual void SetPatrolMotion(int, bool) {}         // Status.SetMotion 0x8017B9D0
    virtual void SetPatrolRootMotion(bool) {}          // Disp.vf0D 0x801A0EB8
    virtual void SetPatrolAnimationRate(float) {}     // Disp.vf0A 0x801A0F70
    virtual void SetPatrolDisplayFlag22(bool) {}      // Disp.vf14 0x8017B9C8; meaning UNKNOWN
    virtual int PatrolAttackMotion() const { return -1; } // E.vf1F 0x80197A50; dynamic variant hook
    virtual bool MotionFlag3A() const = 0;           // S+0x3A (motion end, used by ToHinshi)
    virtual bool MotionFlag3B() const = 0;           // S+0x3B (motion end, used by Caution/NearAttack)
    virtual bool Damaged() const = 0;                // S+0x8A
    virtual bool FellOut() const = 0;                // Move vf02 (M+0x5F)
    virtual void RequestDespawn() = 0;               // SB.vf04 + SB.vf02 (respawnable)
    virtual void PlaySE(int id) = 0;                 // e.g. 0x600F
    virtual float Random(float lo, float hi) = 0;
};

class GunSoldierAI : public EnemyBaseAI {
public:
    GunSoldierAI(GunSoldierBody& body, const EnemySensor& sensor, int appearType, int weaponType)
        : body(body), sensor(sensor), appearType(appearType), weaponType(weaponType) {}

    // GunSoldierAI overrides (vtable 0x8053A108)
    void EnterInitialState() override;   // vf0C 0x801885CC
    void ReturnToWait() override;        // vf0D 0x801883E0
    void OnTargetFound() override;       // vf0F 0x801882B0 -> Caution
    void OnDead() override;              // vf14 0x801884E8 -> ToHinshi
    void OnRescued() override;           // vf16 0x801882E8 -> Thanks
    void StartCombat();                  // fn_801881A0: weapon 0/1 -> ChaseTarget, 2-6 -> Attack
    void ConfigurePatrol(const std::uint8_t* misc, std::size_t length, float spawnYawRad);
    void UpdatePatrol();                  // 0x80188B98, after detection
    void RegenerateRandomPatrolPoint();  // fn_80198F18

    bool Detects() const { return sensor.Detect(body.TargetPosition()); }
    float DistToTarget() const;          // fn_8019FACC (LIKELY 3D distance)
    float DistXZ(const Vec3& a, const Vec3& b) const;

    GunSoldierBody& body;
    const EnemySensor& sensor;
    int appearType;      // E+0x274: 0 STAND 1 LINEAR_MOVE 2 TRIANGLE_MOVE 3 RANDOM_MOVE 4 OFFSETPOS 5 HINSHI
    int weaponType;      // E+0x278: 0 NONE 1 KNIFE 2 GUN 3 MACHINEGUN 4 RIFLE 5 GRENADE 6 MISSILE (enum strings 0x80539464)
    bool rescued = false;  // E+0x356
    Vec3 home;           // E+0x6C (GoHome target = SET position)
    struct PatrolPoint { Vec3 pos{}; float waitSec = 0, moveSpeedRatio = 0; int waitType = 0; };
    PatrolPoint patrolPoints[4]{};       // E+0x288 + i*0x18; index 3 is RANDOM_MOVE
    int patrolMode = 0;                  // E+0x27C, derived from AppearType at 0x80197938
    int patrolPoint = 0;                 // E+0x280
    float patrolRadius = 0;             // E+0x284
    float patrolSpawnYawRad = 0;        // E+0x64, SET yaw (distinct from live transform yaw)
    Vec3 patrolTarget{};                // E+0x6C in the original object
    bool patrolConfigured = false;      // native guard: no raw SET block supplied
    bool patrolHasShield = false;       // E+0x270, read by WaitType 1 motion 0x80197B40

    // State singletons (getters as traced)
    static EnemyAIState& Standing();     // SoldierCommonAI_WaitAction_Standing obj 0x805F04FC
    static EnemyAIState& Moving();       // SoldierCommonAI_WaitAction_Moving   obj 0x805F0508
    static EnemyAIState& Caution();      // 0x80186D50
    static EnemyAIState& Attack();       // 0x80188204
    static EnemyAIState& ChaseTarget();  // 0x801874BC
    static EnemyAIState& NearAttack();   // 0x801876B4
    static EnemyAIState& GoHome();       // 0x80187204
    static EnemyAIState& AppearHome();   // 0x801880F4
    static EnemyAIState& ToHinshi();     // 0x80188520
    static EnemyAIState& Hinshi();       // 0x80187FC0
    static EnemyAIState& Thanks();       // 0x80188334
};

}  // namespace shadow::gameplay

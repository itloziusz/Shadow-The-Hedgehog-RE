// Native reconstruction of EnemyBaseAI / TEnemyAIState (Shadow the Hedgehog GC).
// Evidence: gameplay/enemy/GUN_SOLDIER.md, notes/enemy_gunsoldier_trace.md §2.
//
// Original design kept: states are stateless shared singletons (function-local statics in the DOL,
// re-bound to their owner before every call); per-enemy data lives in the AI object.
#pragma once
#include "shadow/gameplay/EnemyFramework.hpp"

namespace shadow::gameplay {

class EnemyBaseAI;

// Detection sensor / leash (E+0xA8 in GunSoldier). Built by fn_801A6DB8 from the RAW SET misc
// block (SetSlot_GetParamPtr), tested by fn_801A6C7C (detect) and fn_801A6D00 (leash).
struct EnemySensor {
    Vec3  home;          // +0x00 (x,z = SET pos; y = 0)
    Vec3  pos;           // +0x0C SET position
    float leash = 0, leashSq = 0;          // +0x18/+0x1C MoveRange, MoveRange^2
    Vec3  centreXZ;      // +0x20 (y = 0)
    Vec3  centre;        // +0x2C detection centre
    float width = 0, widthSq = 0, height = 0;  // +0x38/+0x3C/+0x40
    // fn_801A6DB8(sensor, pos, rotRad, params[0..5] = MoveRange, SearchRange, SearchAngle(deg),
    //             SearchWidth, SearchHeight, SearchHeightOffset)
    void Build(const Vec3& p, const Vec3& rotRad, const float* params);
    bool Detect(const Vec3& target) const;     // fn_801A6C7C
    bool InLeash(const Vec3& p) const;         // fn_801A6D00
};

// TEnemyAIState vtable: [1] BindOwner (implicit here), [2] Enter, [3] Update, [4] Exit, [5] Name
struct EnemyAIState {
    virtual ~EnemyAIState() = default;
    virtual void Enter(EnemyBaseAI&) {}
    virtual void Update(EnemyBaseAI&) {}
    virtual void Exit(EnemyBaseAI&) {}
    virtual const char* Name() const { return "none"; }   // 0x8017F190 returns "none"
};

// AI+0x28: {owner, cur @+0x2C, prev @+0x30, u8 locked @+0x34}
struct EnemyStateMachine {
    EnemyBaseAI* owner = nullptr;
    EnemyAIState* cur = nullptr;
    EnemyAIState* prev = nullptr;
    bool locked = false;
    void ChangeState(EnemyAIState* next);   // 0x8019FD00
    void RevertToPrevious();                // 0x8019FC54
    void Lock() { locked = true; }          // 0x8019FDB8
    void Unlock() { locked = false; }       // 0x8019FDAC
};

// Status/move facts the AI pre-update reads (EnemyStatusCommon / EnemyMoveCommon fields).
struct EnemyAIInputs {
    bool dead = false;        // S+0x90 (status slot 0x14 IsDead)
    bool wasDead = false;     // S+0x91
    bool guardHit = false;    // BitTest(S+0x80, 5) shield guard (cleared when consumed)
    bool flinch = false;      // S+0x8C
    bool airborne = false;    // S+0x8E (= !(M+0x38 < 5.0))
    bool moveBlocksFall = false;  // Move vf06 (EnemyMoveWalker: always 0)
};

class EnemyBaseAI : public IEnemyAI {
public:
    EnemyBaseAI();
    // IEnemyAI
    void Init() override;                   // vf00 0x8019EEC4 -> EnterInitialState (slot 0x0C)
    void Update(float dt) override;         // vf01 0x8019EACC

    // Overridable reactions (AI vtable slots; GunSoldier overrides 0x0C/0x0D/0x0F/0x14/0x16)
    virtual void EnterInitialState();       // 0x0C
    virtual void ReturnToWait();            // 0x0D
    virtual void OnTargetFound();           // 0x0F
    virtual void OnGuard();                 // 0x12 -> Guard
    virtual void OnDamaged();               // 0x13 -> WaitFinishDamaged
    virtual void OnDead();                  // 0x14 -> EnemyBaseAI_Dead
    virtual void OnRescued() {}             // 0x16

    EnemyStateMachine sm;                   // +0x28
    EnemyAIInputs in;                       // mirrored status/move facts
    float dt = 0.0f;                        // +0x20
    int   subStep = 0;                      // +0x24 per-state sub-step counter
    float timer = 0.0f, timerReset = 0.0f;  // +0x6C / +0x70
    static bool s_globalIdle;               // byte 0x8057E828 (meaning UNKNOWN; forces Idle)

    // Base states (singletons; getters 0x8019F5E4 None, 0x8019D970 Idle, 0x8019D3D8 Guard,
    // 0x8019D27C WaitFinishDamaged, 0x8019EE18 WaitFinishFalling, 0x8019D120 Dead)
    static EnemyAIState& StateNone();
    static EnemyAIState& StateIdle();
    static EnemyAIState& StateGuard();
    static EnemyAIState& StateWaitFinishDamaged();
    static EnemyAIState& StateWaitFinishFalling();
    static EnemyAIState& StateDead();
    bool motionEnded = false;               // S+0x3A/0x3B motion flags used by Guard/WaitFinishDamaged
private:
    void PreUpdate();                       // vf09 0x8019EB54
};

}  // namespace shadow::gameplay

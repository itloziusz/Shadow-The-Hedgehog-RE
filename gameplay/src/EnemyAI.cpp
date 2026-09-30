// Native reconstruction of the enemy AI state machine. Addresses name the PPC origin.
#include "shadow/gameplay/EnemyAI.hpp"
#include <cmath>

namespace shadow::gameplay {

bool EnemyBaseAI::s_globalIdle = false;

// fn_801A6DB8
void EnemySensor::Build(const Vec3& p, const Vec3& rot, const float* m) {
    pos = p;
    home = {p.x, 0.0f, p.z};
    leash = m[0];
    leashSq = m[0] * m[0];
    const float yaw = rot.y + (180.0f + m[2]) * (3.14159265f / 180.0f);
    // fn_8040E0FC rotate (0,0,SearchRange) about Y: x' = x*cos + z*sin, z' = -x*sin + z*cos
    const float s = std::sin(yaw), c = std::cos(yaw);   // fn_80031568 -> fdlibm sin/cos
    const Vec3 off = {m[1] * s, 0.0f, m[1] * c};
    centre = {p.x + off.x, p.y + off.y + m[5], p.z + off.z};
    centreXZ = {centre.x, 0.0f, centre.z};
    width = m[3];
    widthSq = m[3] * m[3];
    height = m[4];
}

// fn_801A6C7C: |t.y - centre.y| < height && distXZ^2(t, centre) < width^2
bool EnemySensor::Detect(const Vec3& t) const {
    if (!(std::fabs(t.y - centre.y) < height)) return false;
    const float dx = t.x - centreXZ.x, dz = t.z - centreXZ.z;
    return dx * dx + dz * dz < widthSq;
}

// fn_801A6D00: distXZ^2(p, home) < MoveRange^2
bool EnemySensor::InLeash(const Vec3& p) const {
    const float dx = p.x - home.x, dz = p.z - home.z;
    return dx * dx + dz * dz < leashSq;
}

// 0x8019FD00
void EnemyStateMachine::ChangeState(EnemyAIState* next) {
    if (locked) return;
    if (cur) cur->Exit(*owner);            // cur->vf01(owner) (rebind) then vf04
    if (next) {
        prev = cur;
        cur = next;
        next->Enter(*owner);               // next->vf01(owner) then vf02
    }
}

// 0x8019FC54: ChangeState with next = previous state
void EnemyStateMachine::RevertToPrevious() { ChangeState(prev); }

namespace {
struct NoneState : EnemyAIState { const char* Name() const override { return "None"; } };
// NOTE: Idle/WaitFinishFalling revert via RevertToPrevious (PROVEN); the exact trigger conditions
// below (flag cleared / landed) are LIKELY — not yet read instruction by instruction.
struct IdleState : EnemyAIState {                         // Update 0x8019CD6C -> revert
    const char* Name() const override { return "Idle"; }
    void Update(EnemyBaseAI& ai) override { if (!EnemyBaseAI::s_globalIdle) ai.sm.RevertToPrevious(); }
};
struct GuardState : EnemyAIState {                        // Update 0x8019C3D0
    const char* Name() const override { return "Guard"; }
    void Update(EnemyBaseAI& ai) override { if (ai.motionEnded) ai.ReturnToWait(); }
};
struct WaitFinishDamagedState : EnemyAIState {            // Update 0x8019BE18
    const char* Name() const override { return "WaitFinishDamaged"; }
    void Update(EnemyBaseAI& ai) override { if (ai.motionEnded) ai.ReturnToWait(); }
};
struct WaitFinishFallingState : EnemyAIState {            // Update 0x8019BCD8
    const char* Name() const override { return "WaitFinishFalling"; }
    void Update(EnemyBaseAI& ai) override { if (!ai.in.airborne) ai.sm.RevertToPrevious(); }
};
struct DeadState : EnemyAIState {                         // Update 0x8019BB58 -> SB.vf02 (delete)
    const char* Name() const override { return "Dead"; }
};
}  // namespace

EnemyAIState& EnemyBaseAI::StateNone() { static NoneState s; return s; }
EnemyAIState& EnemyBaseAI::StateIdle() { static IdleState s; return s; }
EnemyAIState& EnemyBaseAI::StateGuard() { static GuardState s; return s; }
EnemyAIState& EnemyBaseAI::StateWaitFinishDamaged() { static WaitFinishDamagedState s; return s; }
EnemyAIState& EnemyBaseAI::StateWaitFinishFalling() { static WaitFinishFallingState s; return s; }
EnemyAIState& EnemyBaseAI::StateDead() { static DeadState s; return s; }

EnemyBaseAI::EnemyBaseAI() {
    sm.owner = this;                       // SM::Init 0x8019F528 installs None
    sm.cur = &StateNone();
}

void EnemyBaseAI::Init() { EnterInitialState(); }                 // vf00 0x8019EED4
void EnemyBaseAI::EnterInitialState() { sm.ChangeState(&StateIdle()); }
void EnemyBaseAI::ReturnToWait() { sm.ChangeState(&StateIdle()); }
void EnemyBaseAI::OnTargetFound() {}
void EnemyBaseAI::OnGuard() { sm.ChangeState(&StateGuard()); }                  // 0x8019D328
void EnemyBaseAI::OnDamaged() { sm.ChangeState(&StateWaitFinishDamaged()); }    // 0x8019D1CC
void EnemyBaseAI::OnDead() { sm.ChangeState(&StateDead()); }                    // base: EnemyBaseAI_Dead

// vf01 0x8019EACC
void EnemyBaseAI::Update(float frameDt) {
    dt = frameDt;                                     // ai+0x20
    PreUpdate();                                      // vf09
    if (sm.cur) sm.cur->Update(*this);                // cur->vf01(owner); cur->vf03()
    // vf0A 0x8019EB50 is an empty function
}

// vf09 0x8019EB54 — priority order recovered from the disassembly
void EnemyBaseAI::PreUpdate() {
    timer -= dt;                                      // ai+0x6C -= dt
    if (in.dead) {                                    // 0x8019EB98
        if (!in.wasDead) {
            sm.Unlock();
            OnDead();                                 // slot 0x14 (GunSoldier: ToHinshi)
            in.wasDead = true;                        // native mirror of S+0x91 (set by the status component)
        }
        return;
    }
    if (s_globalIdle) {                               // 0x8019EBF0
        sm.ChangeState(&StateIdle());
    } else if (in.guardHit) {                         // 0x8019ECA8
        in.guardHit = false;
        OnGuard();
    } else if (in.flinch) {                           // 0x8019ED08
        sm.Unlock();
        OnDamaged();
    } else if (!in.moveBlocksFall && in.airborne) {   // 0x8019ED4C
        // re-entry guard: LIKELY (0x8019ED4C..0x8019EE00 not fully read)
        if (sm.cur != &StateWaitFinishFalling()) sm.ChangeState(&StateWaitFinishFalling());
    }
}

}  // namespace shadow::gameplay

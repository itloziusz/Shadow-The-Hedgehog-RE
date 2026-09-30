// Native reconstruction of the GUN soldier AI states. Transition rules follow
// notes/enemy_gunsoldier_trace.md §2.5 (addresses per state). Where the trace only gives the
// rule and not every engine effect (e.g. display root motion), the code says so.
#include "shadow/gameplay/GunSoldierAI.hpp"
#include <cmath>
#include <cstring>

namespace shadow::gameplay {

namespace {
GunSoldierAI& GS(EnemyBaseAI& ai) { return static_cast<GunSoldierAI&>(ai); }

// SoldierCommonAI_WaitAction_Standing (Update 0x80189308): detect at END of update -> Caution
struct StandingState : EnemyAIState {
    const char* Name() const override { return "WaitAction_Standing"; }
    void Enter(EnemyBaseAI& ai) override { GS(ai).body.Stop(); ai.subStep = 0; }   // 0x801895F4
    void Update(EnemyBaseAI& ai) override {
        if (GS(ai).Detects()) ai.OnTargetFound();                                   // 0x801895C0..DC
    }
};

// SoldierCommonAI_WaitAction_Moving (Enter 0x801891C4, Update 0x80188B98).
struct MovingState : EnemyAIState {
    const char* Name() const override { return "WaitAction_Moving"; }
    void Enter(EnemyBaseAI& ai) override {
        auto& g = GS(ai);
        ai.subStep = 0;
        if (!g.patrolConfigured) return; // explicit missing-SET-data stub
        g.body.SetPatrolMotion(0x21, true);              // 0x801891E4..0x80189228
        g.body.SetPatrolRootMotion(false);               // 0x8018922C..0x80189250
        g.patrolPoint = g.patrolMode == 3 ? 3 : (g.patrolMode == 0 ? 0 : 1);
        if (g.patrolPoint == 3) g.RegenerateRandomPatrolPoint();
        g.patrolTarget = g.patrolPoints[g.patrolPoint].pos;
        g.body.SetPatrolTarget(g.patrolTarget);         // 0x801892B8..0x801892EC
    }
    void Update(EnemyBaseAI& ai) override {
        if (GS(ai).Detects()) { ai.OnTargetFound(); return; }                       // 0x80188BDC..F8
        GS(ai).UpdatePatrol();
    }
    void Exit(EnemyBaseAI& ai) override { GS(ai).body.SetPatrolRootMotion(true); } // 0x80188B58
};

// GunSoldierAI_Caution (Update 0x801877D4)
struct CautionState : EnemyAIState {
    const char* Name() const override { return "Caution"; }
    void Update(EnemyBaseAI& ai) override {
        GunSoldierAI& g = GS(ai);
        g.body.TurnTowards(g.body.TargetPosition());
        if (g.weaponType == 0 || g.weaponType == 1) { g.sm.ChangeState(&GunSoldierAI::ChaseTarget()); return; }
        if (g.body.MotionFlag3B()) g.StartCombat();                                 // fn_801881A0
    }
};

// GunSoldierAI_Attack (Enter 0x801872B0, Update 0x80186ED0)
struct AttackState : EnemyAIState {
    const char* Name() const override { return "Attack"; }
    void Enter(EnemyBaseAI& ai) override { ai.subStep = 0; }
    void Update(EnemyBaseAI& ai) override {
        GunSoldierAI& g = GS(ai);
        if (!g.Detects()) { g.sm.ChangeState(&GunSoldierAI::GoHome()); return; }  // 0x801871C8..E4
        const Vec3 t = g.body.TargetPosition();
        if (ai.subStep == 0) {                       // aim
            if (g.body.TurnTowards(t)) {
                g.body.StartFireMotion();            // shot emitted later by motion event 0x5000 (vf18)
                ai.timer = 3.0f;                     // timer 3 s
                ai.subStep = 1;
            }
        } else if (ai.timer < 0.0f || (g.body.MotionFlag3B() && std::fabs(g.body.YawErrorDeg(t)) > 30.0f)) {
            ai.subStep = 0;                          // back to aiming
        }
    }
};

// GunSoldierAI_ChaseTarget (Update 0x801875F8)
struct ChaseState : EnemyAIState {
    const char* Name() const override { return "ChaseTarget"; }
    void Update(EnemyBaseAI& ai) override {
        GunSoldierAI& g = GS(ai);
        if (!g.Detects()) { g.sm.ChangeState(&GunSoldierAI::GoHome()); return; }
        if (g.DistToTarget() < 15.0f) { g.sm.ChangeState(&GunSoldierAI::NearAttack()); return; }  // 0x80187680
        g.body.MoveTowards(g.body.TargetPosition());
    }
};

// GunSoldierAI_NearAttack (Update 0x80187370)
struct NearAttackState : EnemyAIState {
    const char* Name() const override { return "NearAttack"; }
    void Update(EnemyBaseAI& ai) override {
        GunSoldierAI& g = GS(ai);
        if (!g.Detects()) { g.sm.ChangeState(&GunSoldierAI::GoHome()); return; }
        if (g.body.MotionFlag3B() && g.DistToTarget() > 20.0f)                     // 0x80187420
            g.sm.ChangeState(&GunSoldierAI::ChaseTarget());
    }
};

// GunSoldierAI_GoHome (Enter 0x80186DFC: home := SET position; Update 0x80186C64)
struct GoHomeState : EnemyAIState {
    const char* Name() const override { return "GoHome"; }
    void Update(EnemyBaseAI& ai) override {
        GunSoldierAI& g = GS(ai);
        if (g.Detects()) { ai.OnTargetFound(); return; }
        if (g.DistXZ(g.body.Position(), g.home) < 10.0f) { ai.ReturnToWait(); return; }
        g.body.MoveTowards(g.home);
    }
};

// GunSoldierAI_AppearHome (Update 0x80186B48)
struct AppearHomeState : EnemyAIState {
    const char* Name() const override { return "AppearHome"; }
    void Update(EnemyBaseAI& ai) override {
        GunSoldierAI& g = GS(ai);
        if (g.DistXZ(g.body.Position(), g.home) < 10.0f) { g.StartCombat(); return; }
        g.body.MoveTowards(g.home);
    }
};

// GunSoldierAI_ToHinshi (Enter 0x80188078 lock; Update 0x80187EE4; Exit 0x80187EBC unlock)
struct ToHinshiState : EnemyAIState {
    const char* Name() const override { return "ToHinshi"; }
    void Enter(EnemyBaseAI& ai) override { ai.sm.Lock(); }
    void Update(EnemyBaseAI& ai) override {
        GunSoldierAI& g = GS(ai);
        if (g.body.FellOut()) { g.body.RequestDespawn(); return; }                 // SB.vf04 + SB.vf02
        if (g.body.MotionFlag3A()) { ai.sm.Unlock(); ai.sm.ChangeState(&GunSoldierAI::Hinshi()); }
    }
    void Exit(EnemyBaseAI& ai) override { ai.sm.Unlock(); }
};

// GunSoldierAI_Hinshi (injured; Enter 0x80187DB8, Update 0x80187D14, Exit 0x80187C84)
struct HinshiState : EnemyAIState {
    const char* Name() const override { return "Hinshi"; }
    void Enter(EnemyBaseAI& ai) override {
        ai.sm.Lock();                                // leaves only via OnRescued (unlock)
        ai.timer = GS(ai).body.Random(30.0f, 60.0f);
    }
    void Update(EnemyBaseAI& ai) override {
        if (ai.timer < 0.0f) {                       // every expiry: SE 0x600F ("help" voice, LIKELY)
            GS(ai).body.PlaySE(0x600F);
            ai.timer = GS(ai).body.Random(30.0f, 60.0f);
        }
    }
};

// GunSoldierAI_Thanks (Enter 0x80187BCC lock; Update 0x801879A8)
struct ThanksState : EnemyAIState {
    const char* Name() const override { return "Thanks"; }
    void Enter(EnemyBaseAI& ai) override { ai.sm.Lock(); }
    void Update(EnemyBaseAI& ai) override {
        GunSoldierAI& g = GS(ai);
        if (g.body.Damaged()) {                      // 0x80187B88..AC
            ai.sm.Unlock();
            ai.sm.ChangeState(&GunSoldierAI::ChaseTarget());
        }
    }
};
}  // namespace

EnemyAIState& GunSoldierAI::Standing() { static StandingState s; return s; }
EnemyAIState& GunSoldierAI::Moving() { static MovingState s; return s; }
EnemyAIState& GunSoldierAI::Caution() { static CautionState s; return s; }
EnemyAIState& GunSoldierAI::Attack() { static AttackState s; return s; }
EnemyAIState& GunSoldierAI::ChaseTarget() { static ChaseState s; return s; }
EnemyAIState& GunSoldierAI::NearAttack() { static NearAttackState s; return s; }
EnemyAIState& GunSoldierAI::GoHome() { static GoHomeState s; return s; }
EnemyAIState& GunSoldierAI::AppearHome() { static AppearHomeState s; return s; }
EnemyAIState& GunSoldierAI::ToHinshi() { static ToHinshiState s; return s; }
EnemyAIState& GunSoldierAI::Hinshi() { static HinshiState s; return s; }
EnemyAIState& GunSoldierAI::Thanks() { static ThanksState s; return s; }

float GunSoldierAI::DistXZ(const Vec3& a, const Vec3& b) const {
    const float dx = a.x - b.x, dz = a.z - b.z;
    return std::sqrt(dx * dx + dz * dz);
}

float GunSoldierAI::DistToTarget() const {
    const Vec3 a = body.Position(), b = body.TargetPosition();
    const float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

namespace {
float ReadPatrolFloat(const std::uint8_t* p, int index) {
    float value;
    std::memcpy(&value, p + 4 * index, sizeof value);
    return value;
}
int ReadPatrolInt(const std::uint8_t* p, int index) {
    int value;
    std::memcpy(&value, p + 4 * index, sizeof value);
    return value;
}
float PatrolDistance(const Vec3& a, const Vec3& b) {
    const float dx = a.x - b.x, dz = a.z - b.z;
    return std::sqrt(dx * dx + dz * dz);
}
} // namespace

// SoldierCommon_ReadSetParams 0x80198D90. The SET loader has already converted the misc
// block to host-endian 32-bit slots; its size is 23 slots (92 bytes).
void GunSoldierAI::ConfigurePatrol(const std::uint8_t* misc, std::size_t length, float spawnYawRad) {
    patrolConfigured = false;
    if (!misc || length < 92) return;
    home = sensor.pos;
    patrolSpawnYawRad = spawnYawRad;
    patrolMode = appearType >= 1 && appearType <= 3 ? appearType : 0; // 0x80197938
    patrolHasShield = ReadPatrolInt(misc, 7) != 0;
    patrolPoints[0] = {sensor.pos, ReadPatrolFloat(misc, 11), ReadPatrolFloat(misc, 12), ReadPatrolInt(misc, 10)};
    patrolPoints[1] = {{sensor.pos.x + ReadPatrolFloat(misc, 13), sensor.pos.y,
                        sensor.pos.z + ReadPatrolFloat(misc, 14)},
                       ReadPatrolFloat(misc, 16), ReadPatrolFloat(misc, 17), ReadPatrolInt(misc, 15)};
    patrolPoints[2] = {{sensor.pos.x + ReadPatrolFloat(misc, 18), sensor.pos.y,
                        sensor.pos.z + ReadPatrolFloat(misc, 19)},
                       ReadPatrolFloat(misc, 21), ReadPatrolFloat(misc, 22), ReadPatrolInt(misc, 20)};
    patrolPoints[3] = {sensor.pos, patrolPoints[0].waitSec,
                       patrolPoints[0].moveSpeedRatio, patrolPoints[0].waitType};
    // 0x80198EC8..0x80198EE0: Pos1_WaitSec doubles as RANDOM_MOVE radius.
    patrolRadius = std::fmin(ReadPatrolFloat(misc, 16), ReadPatrolFloat(misc, 0));
    patrolConfigured = true;
}

// fn_80198F18: two engine RNG draws, rotate (x,0,0) around Y by angle [0, pi],
// then add SET home. It generates point 3; wait settings are copied from point 0.
void GunSoldierAI::RegenerateRandomPatrolPoint() {
    const float x = body.Random(-patrolRadius, patrolRadius);
    const float angle = body.Random(0.0f, 3.14159265f);
    patrolPoints[3].pos = {sensor.pos.x + x * std::cos(angle), sensor.pos.y,
                           sensor.pos.z - x * std::sin(angle)};
}

// WaitAction_Moving::Update 0x80188C00..0x80189188, after the early sensor test.
void GunSoldierAI::UpdatePatrol() {
    if (!patrolConfigured) return; // explicit missing-SET-data stub, no invented point table
    const float distance = PatrolDistance(patrolTarget, body.Position());
    switch (subStep) {
    case 0: { // face current patrol point, then walk
        if (distance * distance < 100.0f) {             // 0x80188C70..0x80188CA8
            body.SetPatrolDisplayFlag22(false);
        } else {
            // 0x80188CAC..0x80188D28: dot of facing vector with normalized flat target.
            const float dot = std::cos(body.YawErrorDeg(patrolTarget) * (3.14159265f / 180.0f));
            body.SetPatrolAnimationRate(dot < 0.0f ? 0.2f : dot);
        }
        if (!body.TurnTowards(patrolTarget)) return;   // 0x80188D2C..0x80188D40
        subStep = 1;
        body.SetPatrolRootMotion(true);                 // 0x80188D44..0x80188D70
        timer = timerReset = 3.0f + distance / 10.0f;  // 0x80188D74..0x80188DB4; type-0 speed table 0x804CD4D4
        return;
    }
    case 1: { // walk, with a timeout and a 10-unit arrival threshold
        body.TurnTowards(patrolTarget);                // 0x80188DBC..0x80188DC4
        if (timer < 0.0f || distance < 10.0f) {
            subStep = 2;
            body.SetPatrolAnimationRate(0.2f);        // 0x80188E00..0x80188E30
            return;
        }
        // For GUN soldier type 0, the speed table value is exactly 10.0f. The original
        // distance < base-speed animation-rate branch at 0x80188E34..0x80188EFC is
        // unreachable after the strict distance < 10.0f arrival test above.
        return;
    }
    case 2: { // turn back to stored SET/spawn yaw and start the point's wait motion
        body.SetPatrolAnimationRate(0.2f);            // 0x80188F00..0x80188F20
        // fn_80189694 reads E+0x64, the stored SET yaw, whereas live rotation is
        // Enemy_GetRotation(E) -> [E+0x30]+0xC. Its local (0,0,-1) forward vector
        // becomes (-sin(yaw),0,-cos(yaw)); the body hook accepts a world target.
        const Vec3 p = body.Position();
        const Vec3 spawnFacing = {p.x - std::sin(patrolSpawnYawRad), p.y,
                                  p.z - std::cos(patrolSpawnYawRad)};
        if (!body.TurnTowards(spawnFacing)) return;   // 0x80188F24..0x80188F40
        subStep = 3;                                  // 0x80188F2C..0x80188F4C
        const int waitType = patrolPoints[patrolPoint].waitType;
        int motion = -1;
        switch (waitType) {                           // GunSoldierBase::vf0C 0x80197988
        case 0: motion = 6; break;
        case 1: motion = weaponType >= 3 && weaponType <= 6 ? (patrolHasShield ? 2 : 1) : 0; break;
        case 2: motion = 0x22; break;
        case 3: motion = body.PatrolAttackMotion(); break; // dynamic attack variant: explicit engine hook
        default: motion = 0; break;
        }
        if (motion >= 0) body.SetPatrolMotion(motion, true); // 0x80188F50..0x80188F90
        timer = timerReset = patrolPoints[patrolPoint].waitSec; // 0x80188F94..0x80188FBC
        body.SetPatrolRootMotion(true);                // 0x80188FC0..0x80188FDC
        return;
    }
    case 3: // wait, then select next point or restart wait motion on motion flag A
        if (timer < 0.0f) {                            // 0x80188FE4..0x80189014
            if (patrolMode == 1) patrolPoint = patrolPoint == 1 ? 0 : 1;
            else if (patrolMode == 2) patrolPoint = patrolPoint == 1 ? 2 : (patrolPoint == 2 ? 0 : 1);
            body.SetPatrolMotion(0x21, true);         // 0x80189094..0x801890D8
            if (patrolPoint == 3) RegenerateRandomPatrolPoint();
            patrolTarget = patrolPoints[patrolPoint].pos;
            body.SetPatrolTarget(patrolTarget);        // 0x801890DC..0x80189110
            subStep = 0;
        } else if (body.MotionFlag3A()) {              // 0x80189124..0x80189144
            const int waitType = patrolPoints[patrolPoint].waitType;
            int motion = -1;
            switch (waitType) {
            case 0: motion = 6; break;
            case 1: motion = weaponType >= 3 && weaponType <= 6 ? (patrolHasShield ? 2 : 1) : 0; break;
            case 2: motion = 0x22; break;
            case 3: motion = body.PatrolAttackMotion(); break;
            default: motion = 0; break;
            }
            if (motion >= 0) body.SetPatrolMotion(motion, true);
        }
        return;
    default: return;
    }
}

// vf0C 0x801885CC
void GunSoldierAI::EnterInitialState() {
    if (rescued) { sm.ChangeState(&Thanks()); return; }
    switch (appearType) {
    case 0: sm.ChangeState(&Standing()); break;
    case 1: case 2: case 3: sm.ChangeState(&Moving()); break;
    case 4: sm.ChangeState(&AppearHome()); break;
    case 5: sm.ChangeState(&Hinshi()); break;
    default: break;
    }
}

// vf0D 0x801883E0: as vf0C but AppearType 4 -> Standing
void GunSoldierAI::ReturnToWait() {
    if (rescued) { sm.ChangeState(&Thanks()); return; }
    if (appearType == 4) { sm.ChangeState(&Standing()); return; }
    EnterInitialState();
}

void GunSoldierAI::OnTargetFound() { sm.ChangeState(&Caution()); }        // vf0F
void GunSoldierAI::OnDead() { sm.ChangeState(&ToHinshi()); }             // vf14 (no EnemyBaseAI_Dead)
void GunSoldierAI::OnRescued() {                                          // vf16
    rescued = true;
    sm.Unlock();
    sm.ChangeState(&Thanks());
}

// fn_801881A0
void GunSoldierAI::StartCombat() {
    if (weaponType == 0 || weaponType == 1) sm.ChangeState(&ChaseTarget());
    else sm.ChangeState(&Attack());
}

}  // namespace shadow::gameplay

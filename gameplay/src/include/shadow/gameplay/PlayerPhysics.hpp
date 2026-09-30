// Native reconstruction of Shadow's player motion core (Shadow the Hedgehog GC).
// Evidence: PLAYER_GAMEPLAY_RECOVERY.md / notes/player_trace.md §5 (integrator verified against
// fn_80093BBC by the integration pass). Constants carry their original addresses; the drag
// constants live in writable .sdata in the original (tunables).
#pragma once
#include "shadow/gameplay/SetSystem.hpp"   // Vec3

namespace shadow::gameplay {

struct PlayerTuning {
    // Drag k (.sdata, fn_80079854 selects by state)
    float dragSlideOrRoll = 1.0f;     // 0x805E5650 (flags 7 Sliding / 9 SpinDash roll)
    float dragGround      = 20.0f;    // 0x805E564C (flag 2 grounded)
    float dragAirAttack   = 10.0f;    // 0x805E5648 (flag 0x1D)
    float dragAirHoriz    = 3.0f;     // 0x805E5640 (air, component perpendicular to up)
    float dragAirVert     = 1.0f;     // 0x805E5644 (air, component along up)
    // Gravity (fn_800A7AE8)
    float gravityIdle     = -0.0625f; // 0x805F3C78 (flag 1)
    float gravityGround   = -800.0f;  // 0x805F3C7C (flag 2)
    float gravityAir      = -320.0f;  // 0x805F3C80
    // Ground running (fn_80082E0C / Ground::Update 0x80083A38)
    float groundAccelMin  = 1000.0f;  // 0x805F3710
    float groundAccelMax  = 15000.0f; // 0x805F3718 x charTable[id].+4 (Shadow 1.0)
    // Jump (table 0x804B1628, 0xC per character; Shadow row)
    float jumpSpeed       = 100.0f;   // +0 (Jump::Enter 0x80087F48)
    float jumpHoldThrust  = 270.0f;   // +4 (Jump::Update 0x800873D8)
    float jumpHoldTime    = 0.75f;    // 0x805F3834
    float airSteer        = 220.0f;   // 0x805F3824 / Fall 0x805F3698
    // Dashes
    float jumpDashImpulse = 180.0f;   // 0x805F3848
    float jumpDashThrust  = 430.0f;   // 0x805F3850
    float jumpDashTime    = 0.4f;     // 0x805F3840
    float homingThrust    = 1000.0f;  // 0x805F37C0
    float homingTimeout   = 10.0f;    // 0x805F37BC
    float homingRange     = 150.0f;   // sbss 0x805F0CE0 (init 0x80207274)
    float spinDashBase    = 150.0f;   // 0x805F3990
    float spinDashPerSec  = 850.0f;   // 0x805F399C (charge capped at 1 s)
    // Damage
    float knockbackUp     = 100.0f;   // 0x805F35F8
    float knockbackBack   = 100.0f;   // 0x805F35F8 (Super: 700, sbss 0x805EF458)
    float invincibleAfterDamage = 2.0f;   // timer +0x27C (PLAYER doc §7)
};

// fn_80093BBC(pos, vel, accel, k, dt): exact solution of v' = a - k*v over dt.
void IntegrateLinearDrag(Vec3& pos, Vec3& vel, const Vec3& accel, float k, float dt);

// fn_80079854 air case: split velocity into components along/perpendicular to `up` and integrate each
// with its own drag (air vertical 1.0, horizontal 3.0). STRONG (per notes §5.3 table).
void IntegrateAir(Vec3& pos, Vec3& vel, const Vec3& accel, const Vec3& up, const PlayerTuning& t, float dt);

// PROVEN: Jump::Update 0x80087384..0x800873F0. While behavior bit 4 is set and
// the hold timer is >= 0, decrement the timer, then add the per-character
// table's +4 thrust along the saved jump direction to player acceleration.
// This is one recovered part of Jump::Update, not a complete behavior.
void ApplyJumpHold(Vec3& accel, const Vec3& jumpDirection, float& holdTimer,
                   bool holdEnabled, float thrust, float dt);

// PROVEN slices of Fall::Update 0x80080998..0x80080A40 and Jump::Update
// 0x800875B4..0x80087604. The caller supplies the input/action flags read
// by those behaviors; turn and terrain services remain engine hooks.
void ApplyFallAirSteering(Vec3& accel, const Vec3& input, float strength = 220.0f);
float FallInputTurn(bool moveInputFlag, bool parallelMoveFlag, float dt);
void ApplyJumpAirSteering(Vec3& accel, const Vec3& input, bool steeringEnabled,
                          bool carryingObject);

// HomingAttack::Update 0x80085804..0x80085864 after its engine steering query
// has supplied `forward`: add forward*1000 to acceleration and re-aim the
// existing velocity with its speed unchanged. `forward` must be the original
// engine's unit vector; obtaining it is an explicit external service.
void ApplyHomingDrive(Vec3& accel, Vec3& velocity, const Vec3& forward,
                      float thrust = 1000.0f);

// Spin dash launch speed: 150 + 850 * min(charge, 1 s)   (fn_8008F26C 0x8008F354)
inline float SpinDashLaunchSpeed(const PlayerTuning& t, float charge) {
    return t.spinDashBase + t.spinDashPerSec * (charge < 1.0f ? charge : 1.0f);
}

}  // namespace shadow::gameplay

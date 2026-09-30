// Native reconstruction of the player motion integrator (fn_80093BBC) — instruction-level
// structure preserved: k <= 0 uses the constant-acceleration form, k > 0 the exponential form.
#include "shadow/gameplay/PlayerPhysics.hpp"
#include <cmath>

namespace shadow::gameplay {

static Vec3 add(Vec3 a, Vec3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
static Vec3 sub(Vec3 a, Vec3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
static Vec3 mul(Vec3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }   // fn_8040DDF8
static float dot(Vec3 a, Vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

void IntegrateLinearDrag(Vec3& pos, Vec3& vel, const Vec3& a, float k, float dt) {
    if (k <= 0.0f) {                                   // 0x80093BFC (fcmpo k, 0; cror lt|eq)
        // pos += v*dt + 0.5*a*dt*dt ; v += a*dt       (0x80093C08..0x80093C9C)
        pos = add(add(pos, mul(vel, dt)), mul(mul(mul(a, dt), dt), 0.5f));
        vel = add(vel, mul(a, dt));
        return;
    }
    // e^{-k dt} = 1 / pow(e, k*dt)                    (0x80093CA4..0x80093CC8; e = double 0x805F3A68)
    const float decay = 1.0f / (float)std::pow(2.718281828459045, (double)(k * dt));
    const float invK = 1.0f / k;
    const Vec3 dv = mul(sub(mul(vel, k), a), invK);   // v - a/k
    const Vec3 term = mul(mul(dv, invK), 1.0f - decay);   // (v - a/k)(1 - e^{-k dt}) / k
    const Vec3 aOverK = mul(a, invK);
    pos = add(add(pos, mul(aOverK, dt)), term);        // p += (a/k) dt + term
    vel = add(aOverK, mul(dv, decay));                  // v = a/k + (v - a/k) e^{-k dt}
}

void IntegrateAir(Vec3& pos, Vec3& vel, const Vec3& a, const Vec3& up, const PlayerTuning& t, float dt) {
    // Split along up (unit) and perpendicular; integrate each with its own drag.
    const Vec3 vUp = mul(up, dot(vel, up)), aUp = mul(up, dot(a, up));
    Vec3 vH = sub(vel, vUp), aH = sub(a, aUp);
    Vec3 pV{0, 0, 0}, pH{0, 0, 0};
    Vec3 vV = vUp;
    IntegrateLinearDrag(pV, vV, aUp, t.dragAirVert, dt);
    IntegrateLinearDrag(pH, vH, aH, t.dragAirHoriz, dt);
    pos = add(pos, add(pV, pH));
    vel = add(vV, vH);
}

void ApplyJumpHold(Vec3& accel, const Vec3& jumpDirection, float& holdTimer,
                   bool holdEnabled, float thrust, float dt) {
    // Jump::Update 0x80087384..0x800873F0 checks the flag and old timer
    // before subtracting dt; the final nonnegative frame still adds thrust.
    if (!holdEnabled || !(holdTimer >= 0.0f)) return;
    holdTimer -= dt;                                  // 0x800873AC..0x800873B4
    accel = add(accel, mul(jumpDirection, thrust));   // 0x800873D8..0x800873F0
}

void ApplyFallAirSteering(Vec3& accel, const Vec3& input, float strength) {
    accel = add(accel, mul(input, strength));          // 0x80080998..0x800809B8, 0x805F3698
}

float FallInputTurn(bool moveInputFlag, bool parallelMoveFlag, float dt) {
    return moveInputFlag && !parallelMoveFlag ? 15.0f * dt : 0.0f;
    // 0x80080A04..0x80080A40, 0x805F369C; caller executes Player_TurnTowardInput.
}

void ApplyJumpAirSteering(Vec3& accel, const Vec3& input, bool steeringEnabled,
                          bool carryingObject) {
    if (!steeringEnabled) return;                     // behavior bit 1 @0x800875A0
    const float strength = carryingObject ? 110.0f : 220.0f;
    accel = add(accel, mul(input, strength));          // 0x800875B4..0x80087604
}

void ApplyHomingDrive(Vec3& accel, Vec3& velocity, const Vec3& forward, float thrust) {
    accel = add(accel, mul(forward, thrust));          // 0x8008581C..0x8008583C
    const float speed = std::sqrt(dot(velocity, velocity)); // Vec3_Length @0x80085848
    velocity = mul(forward, speed);                    // 0x8008584C..0x80085864
}

}  // namespace shadow::gameplay

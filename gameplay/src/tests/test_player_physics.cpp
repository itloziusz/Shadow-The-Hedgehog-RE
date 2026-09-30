// Properties of the reconstructed player integrator (fn_80093BBC) and derived speeds.
#include "shadow/gameplay/PlayerPhysics.hpp"
#include <cmath>
#include <cstdio>

using namespace shadow::gameplay;
static int g_fail = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); ++g_fail; } } while (0)
static bool near(float a, float b, float eps) { return std::fabs(a - b) <= eps; }

int main() {
    PlayerTuning t;
    // k = 0 branch: exact constant-acceleration kinematics
    Vec3 p{0, 0, 0}, v{0, 0, 0};
    IntegrateLinearDrag(p, v, {0, t.gravityAir, 0}, 0.0f, 1.0f);
    CHECK(near(p.y, -160.0f, 1e-3f) && near(v.y, -320.0f, 1e-3f));

    // closed form: one 1 s step == 1000 steps of 1 ms (k = ground drag)
    Vec3 p1{0, 0, 0}, v1{10, 0, 0}, p2 = p1, v2 = v1;
    const Vec3 a{t.groundAccelMax, 0, 0};
    IntegrateLinearDrag(p1, v1, a, t.dragGround, 1.0f);
    for (int i = 0; i < 1000; ++i) IntegrateLinearDrag(p2, v2, a, t.dragGround, 0.001f);
    CHECK(near(p1.x, p2.x, 0.05f) && near(v1.x, v2.x, 0.05f));

    // terminal ground speed = accelMax / k = 15000 / 20 = 750 u/s (Shadow)
    Vec3 pg{0, 0, 0}, vg{0, 0, 0};
    for (int i = 0; i < 600; ++i) IntegrateLinearDrag(pg, vg, a, t.dragGround, 1.0f / 60);
    std::printf("ground top speed %.2f\n", vg.x);
    CHECK(near(vg.x, 750.0f, 0.01f));

    // falling: vertical air drag 1 -> terminal 320 u/s
    Vec3 pf{0, 0, 0}, vf{0, 0, 0};
    for (int i = 0; i < 60 * 30; ++i)
        IntegrateAir(pf, vf, {0, t.gravityAir, 0}, {0, 1, 0}, t, 1.0f / 60);
    std::printf("fall terminal speed %.2f\n", vf.y);
    CHECK(near(vf.y, -320.0f, 0.05f));

    // horizontal air speed decays with k = 3
    Vec3 ph{0, 0, 0}, vh{300, 0, 0};
    IntegrateAir(ph, vh, {0, 0, 0}, {0, 1, 0}, t, 1.0f);
    CHECK(near(vh.x, 300.0f * std::exp(-3.0f), 0.01f));

    CHECK(near(SpinDashLaunchSpeed(t, 0.5f), 575.0f, 1e-3f));
    CHECK(near(SpinDashLaunchSpeed(t, 2.0f), 1000.0f, 1e-3f));

    // Jump::Update 0x80087384..0x800873F0: timer is tested before decrement,
    // and thrust is acceleration, not a dt-scaled velocity impulse.
    Vec3 jumpAccel{10, 0, -5};
    float hold = 0.01f;
    ApplyJumpHold(jumpAccel, {0, 1, 0}, hold, true, t.jumpHoldThrust, 0.02f);
    CHECK(near(hold, -0.01f, 1e-6f));
    CHECK(near(jumpAccel.x, 10, 1e-6f) && near(jumpAccel.y, 270, 1e-6f) &&
          near(jumpAccel.z, -5, 1e-6f));
    ApplyJumpHold(jumpAccel, {0, 1, 0}, hold, true, t.jumpHoldThrust, 0.02f);
    CHECK(near(jumpAccel.y, 270, 1e-6f));
    hold = 0.0f;
    ApplyJumpHold(jumpAccel, {0, 1, 0}, hold, true, t.jumpHoldThrust, 0.02f);
    CHECK(near(jumpAccel.y, 540, 1e-6f)); // timer zero still grants this frame
    hold = 0.5f;
    ApplyJumpHold(jumpAccel, {0, 1, 0}, hold, false, t.jumpHoldThrust, 0.02f);
    CHECK(near(hold, 0.5f, 1e-6f) && near(jumpAccel.y, 540, 1e-6f));

    // Fall::Update 0x80080998..0x80080A40: force is not multiplied by dt;
    // turn request is suppressed by state flag 0x3C or absent input flag 0x1B.
    Vec3 fallAccel{1, 2, 3};
    ApplyFallAirSteering(fallAccel, {0.5f, 0, -1});
    CHECK(near(fallAccel.x, 111, 1e-5f) && near(fallAccel.y, 2, 1e-5f) &&
          near(fallAccel.z, -217, 1e-5f));
    CHECK(near(FallInputTurn(true, false, 0.02f), 0.3f, 1e-6f));
    CHECK(FallInputTurn(false, false, 0.02f) == 0 && FallInputTurn(true, true, 0.02f) == 0);

    // Jump::Update 0x800875B4..0x80087604: bit-1 gate and carrying half-force.
    Vec3 steerAccel{0, 0, 0};
    ApplyJumpAirSteering(steerAccel, {1, 0, 0}, false, false);
    CHECK(steerAccel.x == 0);
    ApplyJumpAirSteering(steerAccel, {1, 0, 0}, true, true);
    CHECK(near(steerAccel.x, 110, 1e-5f));
    ApplyJumpAirSteering(steerAccel, {1, 0, 0}, true, false);
    CHECK(near(steerAccel.x, 330, 1e-5f));

    // HomingAttack::Update 0x8008581C..0x80085864: re-aim preserves the
    // current speed while acceleration receives the full 1000 force.
    Vec3 homeAccel{2, 3, 4}, homeVel{3, 4, 0};
    ApplyHomingDrive(homeAccel, homeVel, {0, 0, -1});
    CHECK(near(homeAccel.x, 2, 1e-5f) && near(homeAccel.z, -996, 1e-5f));
    CHECK(near(homeVel.x, 0, 1e-5f) && near(homeVel.y, 0, 1e-5f) &&
          near(homeVel.z, -5, 1e-5f));

    std::printf(g_fail ? "FAILED (%d)\n" : "ALL PASS\n", g_fail);
    return g_fail ? 1 : 0;
}

// Checks the reconstructed player behavior transition rules (fn_800A82FC, factory remap).
#include "shadow/gameplay/PlayerBehavior.hpp"
#include <cstdio>
#include <cmath>
#include <string>

using namespace shadow::gameplay;
static int g_fail = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); ++g_fail; } } while (0)
static std::string g_log;

struct Probe : Behavior {
    bool allow;
    Probe(BehaviorId id, bool a) : Behavior(id), allow(a) {}
    bool CanEnter(BehaviorId prev, const InitializeArgs&) override {
        g_log += "can" + std::to_string((int)id()) + "<" + std::to_string((int)prev) + " ";
        return allow;
    }
    void Enter(BehaviorId prev, const InitializeArgs&) override {
        g_log += "enter" + std::to_string((int)id()) + "<" + std::to_string((int)prev) + " ";
    }
    void Leave(BehaviorId next) override { g_log += "leave" + std::to_string((int)id()) + ">" + std::to_string((int)next) + " "; }
};

int main() {
    BehaviorFactory f;
    for (int i = 0; i < 0x23; ++i) {
        const BehaviorId id = (BehaviorId)i;
        const bool allow = id != BehaviorId::Dead;       // make Dead refuse, to test rejection
        f.Register(id, [id, allow] { return std::make_unique<Probe>(id, allow); });
    }
    PlayerExecuter ex(f);
    CHECK(ex.ChangeBehavior(InitializeArgs(BehaviorId::Idle)));
    CHECK(g_log == "can0<37 enter0<37 ");                // prev = 0x25 when nothing current
    g_log.clear();
    CHECK(ex.ChangeBehavior(InitializeArgs(BehaviorId::Jump)));
    CHECK(g_log == "can5<0 leave0>5 enter5<0 ");         // CanEnter, then Leave(old), then Enter(new)
    g_log.clear();
    CHECK(!ex.ChangeBehavior(InitializeArgs(BehaviorId::Dead)));
    CHECK(g_log == "can31<5 " && ex.current()->id() == BehaviorId::Jump);   // rejected: nothing changes
    CHECK(ex.talkTimer == 10.0f);                        // reset even on rejection
    ex.superFlag = true;                                 // Super: Fall -> Super (0x22)
    CHECK(ex.ChangeBehavior(InitializeArgs(BehaviorId::Fall)));
    CHECK(ex.current()->id() == BehaviorId::Super);

    // Ground::Update 0x80083B7C..0x80083DC0: exact boundary/priority checks.
    GroundExitSignals ground;
    ground.grounded = true;
    ground.groundNormal = {0, 1, 0}; ground.up = {0, 1, 0};
    Vec3 vel{150, 0, 0};
    auto gr = StepGroundExit(vel, ground);
    CHECK(!gr.highSpeedFlag && !gr.fastActionEnabled && gr.slideActionEnabled == false);
    CHECK(!gr.transition);
    vel = {200, 0, 0}; gr = StepGroundExit(vel, ground);
    CHECK(gr.highSpeedFlag && gr.slideActionEnabled == false); // strict >200
    vel = {250, 0, 0}; gr = StepGroundExit(vel, ground);
    CHECK(!gr.fastActionEnabled && gr.slideActionEnabled == true); // strict >250
    vel = {251, 0, 0}; gr = StepGroundExit(vel, ground);
    CHECK(gr.fastActionEnabled);
    ground.parallelMove = true;
    gr = StepGroundExit(vel, ground);
    CHECK(!gr.highSpeedFlag);                             // state 0x3B suppressed by 2D path
    ground.parallelMove = false;

    ground.groundNormal = {0.4f, 0, 0};             // dot(up) = 0, below cos 65 degrees
    vel = {249, 0, 0}; gr = StepGroundExit(vel, ground);
    CHECK(gr.transition == BehaviorId::Fall && std::fabs(vel.x - 40.0f) < 1e-4f);
    vel = {251, 0, 0}; gr = StepGroundExit(vel, ground);
    CHECK(!gr.transition && vel.x == 251);          // high speed skips slope exit
    ground.groundNormal = {0, 1, 0};
    vel = {0.1f, 0, 0}; gr = StepGroundExit(vel, ground);
    CHECK(gr.transition == BehaviorId::Idle);
    vel = {0.2f, 0, 0}; gr = StepGroundExit(vel, ground);
    CHECK(!gr.transition);                          // exact 0.2 is not idle

    ground.grounded = false; ground.ledgeFound = false;
    vel = {300, 0, 0}; gr = StepGroundExit(vel, ground);
    CHECK(gr.transition == BehaviorId::Fall && !gr.slideActionEnabled && vel.x == 300);
    ground.ledgeFound = true; gr = StepGroundExit(vel, ground);
    CHECK(gr.transition == BehaviorId::Ottotto && vel.x == 0);

    // Fall::Update 0x80080B88..0x80080C48: sensor must point against velocity
    // and be closer than the engine supplied landing limit.
    FallLandingSignals fall;
    fall.sensorHit = true; fall.sensorNormal = {0, 1, 0};
    fall.sensorDistance = 4; fall.landingDistanceLimit = 5;
    CHECK(StepFallLanding({0, -20, 0}, fall) == BehaviorId::Landing);
    CHECK(!StepFallLanding({0, 20, 0}, fall));
    fall.sensorDistance = 5;
    CHECK(!StepFallLanding({0, -20, 0}, fall));      // strict distance comparison
    fall.grounded = true;
    CHECK(StepFallLanding({0, 20, 0}, fall) == BehaviorId::Landing);

    // Jump::Update 0x80087680..0x8008770C: no landing until the grace
    // counter is zero at the start of a grounded update.
    int grace = 10;
    for (int i = 0; i < 10; ++i) CHECK(!StepJumpLandingGrace(grace, true));
    CHECK(grace == 0 && StepJumpLandingGrace(grace, true));
    CHECK(grace == 0);
    grace = 0;
    CHECK(!StepJumpLandingGrace(grace, false) && grace == 0);

    // HomingAttack::Update 0x80085648..0x80085800: state exits precede
    // timer changes, target loss precedes timeout timer increment.
    float elapsed = 9.9f, steer = 8.0f;
    auto hs = StepHomingAttack(2, true, elapsed, steer, 0.2f);
    CHECK(hs.transition == BehaviorId::HomingJump && hs.homingJumpVariant == 0);
    CHECK(elapsed == 9.9f && steer == 8.0f);
    hs = StepHomingAttack(3, true, elapsed, steer, 0.2f);
    CHECK(hs.transition == BehaviorId::HomingJump && hs.homingJumpVariant == 1);
    hs = StepHomingAttack(1, true, elapsed, steer, 0.2f);
    CHECK(hs.transition == BehaviorId::Fall && elapsed == 9.9f);
    hs = StepHomingAttack(0, false, elapsed, steer, 0.2f);
    CHECK(hs.transition == BehaviorId::Fall && elapsed == 9.9f && std::fabs(steer - 10.0f) < 1e-5f);
    elapsed = 9.8f; steer = 8.0f;
    hs = StepHomingAttack(0, true, elapsed, steer, 0.2f);
    CHECK(!hs.transition && std::fabs(elapsed - 10.0f) < 1e-5f); // strict >10
    hs = StepHomingAttack(0, true, elapsed, steer, 0.01f);
    CHECK(hs.transition == BehaviorId::Fall);
    std::printf(g_fail ? "FAILED (%d)\n" : "ALL PASS\n", g_fail);
    return g_fail ? 1 : 0;
}

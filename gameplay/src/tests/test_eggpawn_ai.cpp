// Decision-path regression for EggPawnAI, PAL GUPP8P DOL 0x8017DF84..0x8017EE54.
#include "shadow/gameplay/EggPawnAI.hpp"
#include <cstdio>
#include <initializer_list>
#include <string>

using namespace shadow::gameplay;
static int failures = 0;
#define CHECK(x) do { if (!(x)) { std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #x); ++failures; } } while (0)

struct Body : EggPawnBody {
    bool detected = false, inArea = true, reached = false, ended = false, shield = false;
    int standingTicks = 0, movingTicks = 0, faces = 0, motion = -1, motions = 0, advances = 0;
    int cautionBegins = 0, homeBegins = 0, dashHomeBegins = 0;
    float distance = 100.0f, pickRadius = -1.0f, attackParam = 0.0f;
    Vec3 moveTarget{}, target{20, 0, 0};
    bool DetectNearestPlayer(const EnemySensor&) override { return detected; }
    bool TrackedTargetInSearchArea(const EnemySensor&) const override { return inArea; }
    Vec3 TrackedTargetPosition() const override { return target; }
    float TrackedTargetDistance() const override { return distance; }
    void FaceTrackedTarget() override { ++faces; }
    void BeginStandingWait() override {}
    void TickStandingWait() override { ++standingTicks; }
    void BeginMovingWait() override {}
    void TickMovingWait() override { ++movingTicks; }
    void BeginCaution() override { ++cautionBegins; }
    void BeginReturnHome(bool dash) override { if (dash) ++dashHomeBegins; else ++homeBegins; }
    void SetMoveTarget(const Vec3& p) override { moveTarget = p; }
    bool AdvanceTowardMoveTarget() override { ++advances; return reached; }
    bool MotionEnded() const override { return ended; }
    void SetMotion(int id, bool) override { motion = id; ++motions; }
    bool HasShield() const override { return shield; }
    Vec3 AttackPoint(float p) override { attackParam = p; return {40, 0, 0}; }
    Vec3 PickRandomPointNear(float r) override { pickRadius = r; return {50, 0, 0}; }
};
static std::string State(const EggPawnAI& ai) { return ai.sm.cur ? ai.sm.cur->Name() : "null"; }
static EnemySensor Sensor() { EnemySensor s; s.pos = {10, 0, 20}; return s; }

static void TestInitialAndWalkerTransitions() {
    auto sensor = Sensor();
    Body standBody;
    EggPawnAI stand(standBody, sensor, 0, 1, 0);
    stand.Init();
    CHECK(State(stand) == "EnemyWalkerAI_WaitAction_Standing");
    CHECK(standBody.moveTarget.x == 10 && standBody.moveTarget.z == 20);
    stand.Update(0.0f);
    CHECK(standBody.standingTicks == 1);
    standBody.detected = true;
    stand.Update(0.0f);
    CHECK(State(stand) == "EnemyWalkerAI_Caution" && standBody.standingTicks == 1);
    CHECK(stand.timer == 1.0f && standBody.cautionBegins == 1);
    stand.Update(1.0f); // strict timer < 0, so zero does not attack
    CHECK(State(stand) == "EnemyWalkerAI_Caution" && stand.timer == 0.0f);
    stand.Update(0.01f);
    CHECK(State(stand) == "EggPawnAI_WeaponAttack" && standBody.motion == 0xA);

    Body movingBody;
    EggPawnAI moving(movingBody, sensor, 0, 2, 3);
    moving.Init();
    CHECK(State(moving) == "EnemyWalkerAI_WaitAction_Moving");
    moving.Update(0.0f);
    CHECK(movingBody.movingTicks == 1);
    moving.OnTargetFound();
    movingBody.inArea = false;
    moving.Update(1.01f); // even with an expired timer, loss of target wins
    CHECK(State(moving) == "EnemyWalkerAI_WaitAction_Moving");

    for (int appear : {1, 2, 3, 4, -1}) {
        Body b;
        EggPawnAI ai(b, sensor, appear, 1, 0);
        ai.Init();
        if (appear == 1 || appear == 2) {
            CHECK(State(ai) == "EnemyWalkerAI_ReturnHome" && b.homeBegins == 1);
        } else if (appear == 3) {
            CHECK(State(ai) == "EnemyWalkerAI_ReturnHomeDash" && b.dashHomeBegins == 1);
        } else {
            CHECK(State(ai) == "EnemyWalkerAI_WaitAction_Standing");
        }
        if (appear >= 1 && appear <= 3) {
            CHECK(b.moveTarget.x == sensor.pos.x && b.moveTarget.z == sensor.pos.z);
            ai.Update(0.0f); // arrival service says no
            CHECK(State(ai).find("ReturnHome") != std::string::npos);
            CHECK(b.advances == 1);
            b.reached = true;
            ai.Update(0.0f);
            CHECK(State(ai) == "EnemyWalkerAI_WaitAction_Standing");
            CHECK(b.advances == 2);
        }
    }
}

static void TestWeaponAttack() {
    auto sensor = Sensor();
    for (int weapon : {1, 2, 4, -1}) { // all except 0 and 3 use the DOL default branch
        Body b;
        EggPawnAI ai(b, sensor, 0, weapon, 0);
        ai.Init(); ai.StartAttack();
        CHECK(State(ai) == "EggPawnAI_WeaponAttack" && b.motion == (weapon == 1 || weapon == 2 ? 0xA : 6));
        ai.Update(0.0f);
        CHECK(ai.subStep == 0);
        b.ended = true;
        ai.Update(0.0f);
        CHECK(ai.subStep == 1 && b.motion == (weapon == 1 || weapon == 2 ? 1 : 0));
        CHECK(b.pickRadius == 30.0f && b.moveTarget.x == 50 && ai.timer == 2.0f);
        b.ended = false;
        ai.Update(2.0f);
        CHECK(ai.subStep == 1 && b.advances == 1); // zero is not expired; move still updates
        ai.Update(0.01f);
        CHECK(ai.subStep == 2 && b.faces == 1);
        ai.Update(0.0f);
        CHECK(ai.subStep == 0 && b.motion == (weapon == 1 || weapon == 2 ? 0xA : 6));
        b.ended = true;
        ai.Update(0.0f);
        b.inArea = false;
        ai.Update(0.0f);
        CHECK(State(ai) == "EnemyWalkerAI_WaitAction_Standing");
    }

    Body shielded;
    shielded.shield = true;
    EggPawnAI rifle(shielded, sensor, 0, 1, 0);
    CHECK(rifle.AttackMotionId() == 0xB && rifle.AimMotionId() == 3);
    shielded.shield = false;
    CHECK(rifle.AttackMotionId() == 0xA && rifle.AimMotionId() == 1);
    EggPawnAI unarmed(shielded, sensor, 0, 0, 0);
    CHECK(unarmed.AttackMotionId() == 6 && unarmed.AimMotionId() == 0);
    CHECK(unarmed.ChargePauseMotionId() == 6 && rifle.ChargePauseMotionId() == 7);
    shielded.shield = true;
    CHECK(unarmed.AttackMotionId() == 6 && unarmed.AimMotionId() == 2);
    CHECK(unarmed.ChargePauseMotionId() == 8 && rifle.ChargePauseMotionId() == 9);
}

static void TestLanceAndDash() {
    auto sensor = Sensor();
    Body lanceBody; lanceBody.shield = true;
    EggPawnAI lance(lanceBody, sensor, 0, 3, 0);
    lance.Init(); lance.StartAttack();
    CHECK(State(lance) == "EggPawnAI_TukiAttack" && lanceBody.motion == 0xD);
    CHECK(lanceBody.attackParam == -1.0f && lanceBody.moveTarget.x == 40 && lance.timer == 3.0f);
    lance.Update(3.0f);
    CHECK(lance.subStep == 0); // strict timer < 0
    lanceBody.distance = 14.9f;
    lance.Update(0.0f);
    CHECK(lance.subStep == 1 && lanceBody.motion == 0xE); // thrust at distance < 15
    lanceBody.ended = true;
    lance.Update(0.0f);
    CHECK(lance.subStep == 2 && lanceBody.motion == 8 && lance.timer == 1.0f);
    lance.Update(1.0f);
    CHECK(lance.subStep == 2);
    lanceBody.shield = false;
    lance.Update(0.01f);
    CHECK(lance.subStep == 0 && lanceBody.motion == 0xC && lance.timer == 3.0f);
    lanceBody.distance = 100.0f; lanceBody.reached = true;
    lance.Update(0.0f);
    CHECK(lance.subStep == 1); // movement arrival is the other thrust trigger
    lanceBody.ended = true; lance.Update(0.0f);
    lanceBody.inArea = false; lance.Update(0.0f);
    CHECK(State(lance) == "EnemyWalkerAI_WaitAction_Standing");

    Body dashBody;
    EggPawnAI dash(dashBody, sensor, 0, 0, 0);
    dash.Init(); dash.StartAttack();
    CHECK(State(dash) == "EggPawnAI_DashAttack" && dashBody.motion == 0xC);
    dashBody.distance = 0.0f;
    dash.Update(0.0f);
    CHECK(dash.subStep == 0); // dash has no lance's <15 distance trigger
    dashBody.reached = true;
    dash.Update(0.0f);
    CHECK(dash.subStep == 1 && dashBody.motion == 6 && dash.timer == 1.0f);
    dashBody.reached = false;
    dash.Update(1.0f);
    CHECK(dash.subStep == 1);
    dashBody.shield = true;
    dash.Update(0.01f);
    CHECK(dash.subStep == 0 && dashBody.motion == 0xD && dash.timer == 3.0f);
    dashBody.reached = true; dash.Update(0.0f);
    dashBody.inArea = false; dash.Update(0.0f);
    CHECK(State(dash) == "EnemyWalkerAI_WaitAction_Standing");
}

static void TestBaseDeathEdge() {
    auto sensor = Sensor();
    Body body;
    EggPawnAI ai(body, sensor, 0, 3, 0);
    ai.Init(); ai.StartAttack();
    ai.in.dead = true;
    ai.Update(0.0f);
    CHECK(State(ai) == "Dead"); // Egg Pawn uses EnemyBaseAI::OnDead 0x8019D070
    body.inArea = false;
    ai.Update(1.0f);
    CHECK(State(ai) == "Dead"); // attack's lost-target path cannot displace death
}

int main() {
    TestInitialAndWalkerTransitions();
    TestWeaponAttack();
    TestLanceAndDash();
    TestBaseDeathEdge();
    std::printf(failures ? "EggPawnAI FAILED (%d)\n" : "EggPawnAI ALL PASS\n", failures);
    return failures ? 1 : 0;
}

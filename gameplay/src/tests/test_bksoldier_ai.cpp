// Focused BK Soldier decision tests using synthetic sensor/body inputs.
// Branches are checked against main.dol 0x80180770, 0x80180560,
// 0x8018043C, 0x80180234, 0x8017FF00, 0x8017F374.
#include "shadow/gameplay/BkSoldierAI.hpp"
#include <cstdio>
#include <string>

using namespace shadow::gameplay;
static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); ++failures; } } while (0)

struct Body final : BkSoldierBody {
    Vec3 position{}, target{500.0f, 0.0f, 0.0f};
    float distance = 100.0f;
    bool cautionDone = false, meleeDone = false, baseReached = false;
    int patrolUpdates = 0, meleeStarts = 0, rangedUpdates = 0;
    int vacuumStarts = 0, normalStarts = 0, rangedEnds = 0;
    Vec3 Position() const override { return position; }
    Vec3 TargetPosition() const override { return target; }
    void TurnTowards(const Vec3&) override {}
    float TargetDistance() const override { return distance; }
    bool CautionMotionEnded() const override { return cautionDone; }
    bool MeleeMotionEnded() const override { return meleeDone; }
    void BeginStandingWait() override {}
    void BeginMovingWait() override {}
    void UpdateSharedPatrol() override { ++patrolUpdates; }
    void BeginCaution() override {}
    void BeginChase() override {}
    void BeginNearAttack() override { ++meleeStarts; }
    void BeginBaseGoHome() override {}
    bool BaseGoHomeReached() const override { return baseReached; }
    void BeginBkGoHome() override {}
    void BeginAppearHome() override {}
    void BeginRangedAttack(bool vacuum) override {
        if (vacuum) ++vacuumStarts; else ++normalStarts;
    }
    void UpdateRangedAttack(bool) override { ++rangedUpdates; }
    void EndRangedAttack(bool) override { ++rangedEnds; }
};

static EnemySensor TestSensor() {
    EnemySensor s;
    s.pos = {0.0f, 0.0f, 0.0f};
    s.centre = s.centreXZ = s.pos;
    s.width = 100.0f;
    s.widthSq = 10000.0f;
    s.height = 100.0f;
    return s;
}

static std::string State(BkSoldierAI& ai) { return ai.sm.cur ? ai.sm.cur->Name() : "null"; }

int main() {
    const EnemySensor sensor = TestSensor();
    for (int appear : {-1, 0, 1, 2, 3, 4, 5, 6}) {
        Body body;
        BkSoldierAI ai(body, sensor, appear, 2);
        ai.Init();
        const char* expected = (appear >= 1 && appear <= 3) ? "WaitAction_Moving"
                             : (appear == 4 || appear == 5) ? "AppearHome"
                             : "WaitAction_Standing";
        CHECK(State(ai) == expected);
        if (appear == 4 || appear == 5) {
            CHECK(ai.appearedHome);
            ai.ReturnToWait();
            CHECK(State(ai) == "WaitAction_Standing");
        }
    }
    for (int weapon : {0, 1, 2, 5, 6, 8}) {
        Body body;
        BkSoldierAI ai(body, sensor, 0, weapon);
        ai.StartCombat();
        CHECK(State(ai) == (weapon <= 1 ? "ChaseTarget"
                         : weapon == 6 ? "AttackVacuum" : "Attack"));
    }
    {
        Body body;
        BkSoldierAI ai(body, sensor, 0, 0);
        ai.Init();
        ai.Update(1.0f / 60.0f);
        CHECK(State(ai) == "WaitAction_Standing");
        body.target = {0.0f, 0.0f, 0.0f};
        ai.Update(1.0f / 60.0f);
        CHECK(State(ai) == "Caution");
        ai.Update(1.0f / 60.0f);
        CHECK(State(ai) == "ChaseTarget"); // melee does not wait for caution motion
        body.distance = 30.0f;
        ai.Update(1.0f / 60.0f);
        CHECK(State(ai) == "ChaseTarget"); // strict <30
        body.distance = 29.9f;
        ai.Update(1.0f / 60.0f);
        CHECK(State(ai) == "NearAttack");
        body.meleeDone = true;
        body.distance = 30.0f;
        ai.Update(1.0f / 60.0f);
        CHECK(State(ai) == "NearAttack" && body.meleeStarts == 2); // strict >30
        body.distance = 30.1f;
        ai.Update(1.0f / 60.0f);
        CHECK(State(ai) == "ChaseTarget");
        body.target = {500.0f, 0.0f, 0.0f};
        ai.Update(1.0f / 60.0f);
        CHECK(State(ai) == "BaseGoHome");
        body.target = {0.0f, 0.0f, 0.0f}; // base GoHome has no sensor path
        ai.Update(1.0f / 60.0f);
        CHECK(State(ai) == "BaseGoHome");
        body.baseReached = true;
        ai.Update(1.0f / 60.0f);
        CHECK(State(ai) == "WaitAction_Standing");
    }
    {
        Body body;
        BkSoldierAI ai(body, sensor, 2, 2);
        ai.Init();
        ai.Update(1.0f / 60.0f);
        CHECK(body.patrolUpdates == 1); // explicit shared patrol engine hook
        body.target = {0.0f, 0.0f, 0.0f};
        ai.Update(1.0f / 60.0f);
        CHECK(State(ai) == "Caution" && body.patrolUpdates == 1);
        ai.Update(1.0f / 60.0f);
        CHECK(State(ai) == "Caution");
        body.cautionDone = true;
        ai.Update(1.0f / 60.0f);
        CHECK(State(ai) == "Attack" && body.normalStarts == 1);
        ai.Update(1.0f / 60.0f);
        CHECK(body.rangedUpdates == 1);
        body.target = {500.0f, 0.0f, 0.0f};
        ai.Update(1.0f / 60.0f);
        CHECK(State(ai) == "BkGoHome" && body.rangedEnds == 1);
        CHECK(body.rangedUpdates == 2); // 0x8017FDC0: engine work precedes lost-target test
        body.target = {0.0f, 0.0f, 0.0f};
        ai.Update(1.0f / 60.0f);
        CHECK(State(ai) == "Caution"); // BK GoHome re-detects
    }
    {
        Body body;
        BkSoldierAI ai(body, sensor, 5, 6);
        ai.Init();
        CHECK(State(ai) == "AppearHome");
        ai.Update(1.0f / 60.0f); // position is already at SET home
        CHECK(State(ai) == "AttackVacuum" && body.vacuumStarts == 1);
        ai.in.dead = true;
        ai.Update(1.0f / 60.0f);
        CHECK(State(ai) == "Dead"); // BK Soldier inherits base death state
    }
    std::printf(failures ? "FAILED (%d)\n" : "ALL PASS\n", failures);
    return failures ? 1 : 0;
}

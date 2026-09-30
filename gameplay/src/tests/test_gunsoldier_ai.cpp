// Drives the reconstructed GunSoldier AI with a real GUN_SOLDIER record from stg0100 and a
// scripted body, checking the state transitions recovered in notes/enemy_gunsoldier_trace.md §2.
#include "shadow/gameplay/GunSoldierAI.hpp"
#include "shadow/gameplay/SetSystem.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

using namespace shadow::gameplay;
static int g_fail = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); ++g_fail; } } while (0)

struct ScriptBody : GunSoldierBody {
    Vec3 pos, target;
    bool flag3A = false, flag3B = false, damaged = false, fell = false;
    int fireCount = 0, seCount = 0, despawns = 0;
    int patrolMotion = -1, patrolMotionCalls = 0, patrolRootCalls = 0;
    bool patrolRoot = false, patrolFlag22 = true, turnReady = true;
    float patrolRate = -1.0f;
    Vec3 patrolTarget{}, lastTurnTarget{};
    Vec3 Position() const override { return pos; }
    Vec3 TargetPosition() const override { return target; }
    bool TurnTowards(const Vec3& p) override { lastTurnTarget = p; return turnReady; }
    float YawErrorDeg(const Vec3&) const override { return 0.0f; }
    void MoveTowards(const Vec3& p) override { pos = p; }          // teleport-walk for the test
    void Stop() override {}
    void StartFireMotion() override { ++fireCount; }
    void SetPatrolTarget(const Vec3& p) override { patrolTarget = p; }
    void SetPatrolMotion(int id, bool) override { patrolMotion = id; ++patrolMotionCalls; }
    void SetPatrolRootMotion(bool enabled) override { patrolRoot = enabled; ++patrolRootCalls; }
    void SetPatrolAnimationRate(float rate) override { patrolRate = rate; }
    void SetPatrolDisplayFlag22(bool enabled) override { patrolFlag22 = enabled; }
    int PatrolAttackMotion() const override { return 0x13; }
    bool MotionFlag3A() const override { return flag3A; }
    bool MotionFlag3B() const override { return flag3B; }
    bool Damaged() const override { return damaged; }
    bool FellOut() const override { return fell; }
    void RequestDespawn() override { ++despawns; }
    void PlaySE(int) override { ++seCount; }
    float Random(float lo, float) override { return lo; }
};

static std::string St(GunSoldierAI& ai) { return ai.sm.cur ? ai.sm.cur->Name() : "null"; }

static void Slot(std::uint8_t* p, int index, float value) { std::memcpy(p + 4 * index, &value, 4); }
static void Slot(std::uint8_t* p, int index, int value) { std::memcpy(p + 4 * index, &value, 4); }

static void TestPatrol() {
    // Synthetic host-endian SET misc block, matching SoldierCommon_ReadSetParams 0x80198D90.
    std::uint8_t misc[92]{};
    Slot(misc, 0, 200.0f);   // MoveRange
    Slot(misc, 7, 1);        // shield
    Slot(misc, 10, 0); Slot(misc, 11, 1.0f); Slot(misc, 12, 0.8f);
    Slot(misc, 13, 100.0f); Slot(misc, 14, 0.0f);
    Slot(misc, 15, 1); Slot(misc, 16, 20.0f); Slot(misc, 17, 0.5f);
    Slot(misc, 18, 0.0f); Slot(misc, 19, 100.0f);
    Slot(misc, 20, 2); Slot(misc, 21, 2.0f); Slot(misc, 22, 0.25f);
    EnemySensor sensor; sensor.pos = {10, 0, 20}; sensor.centre = sensor.pos;
    sensor.centreXZ = sensor.pos; sensor.widthSq = 1; sensor.height = 1;
    ScriptBody body; body.pos = sensor.pos; body.target = {9999, 0, 9999};
    GunSoldierAI linear(body, sensor, 1, 3);
    linear.ConfigurePatrol(nullptr, 0, 0);
    CHECK(!linear.patrolConfigured);
    linear.ConfigurePatrol(misc, sizeof misc, 3.14159265f / 2.0f);
    CHECK(linear.patrolConfigured && linear.patrolMode == 1);
    CHECK(linear.patrolPoints[1].pos.x == 110 && linear.patrolPoints[1].waitSec == 20);
    linear.Init();
    CHECK(linear.patrolPoint == 1 && linear.patrolTarget.x == 110);
    CHECK(body.patrolMotion == 0x21 && !body.patrolRoot);
    body.turnReady = false;
    linear.Update(1.0f / 60);  // face point, but turn incomplete
    CHECK(linear.subStep == 0 && body.patrolRate > 0.99f);
    body.turnReady = true;
    linear.Update(1.0f / 60);  // now start walking
    CHECK(linear.subStep == 1 && body.patrolRoot && linear.timer > 12.0f);
    linear.Update(20.0f);     // traversal timeout
    CHECK(linear.subStep == 2);
    body.turnReady = false;
    linear.Update(1.0f / 60); // step 2 must turn to SET yaw, not current facing or next point
    CHECK(linear.subStep == 2 && std::fabs(body.lastTurnTarget.x - 9.0f) < 0.001f);
    CHECK(std::fabs(body.lastTurnTarget.z - 20.0f) < 0.001f);
    body.turnReady = true;
    linear.Update(1.0f / 60); // wait at point1: WaitType 1 + rifle + shield -> motion 2
    CHECK(linear.subStep == 3 && body.patrolMotion == 2 && linear.timer == 20.0f);
    body.flag3A = true;
    // vf1E 0x80197B40 reads live E+0x270. A broken shield changes the next
    // WaitType-1 motion from 2 to 1; do not cache the initial SET value forever.
    linear.patrolHasShield = false;
    const int oldMotions = body.patrolMotionCalls;
    linear.Update(1.0f / 60); // motion-end repeats wait motion while timer is live
    CHECK(body.patrolMotionCalls == oldMotions + 1 && body.patrolMotion == 1 && linear.patrolPoint == 1);
    body.flag3A = false;
    linear.Update(21.0f);    // LINEAR_MOVE 1 -> 0
    CHECK(linear.patrolPoint == 0 && linear.subStep == 0);
    CHECK(linear.patrolTarget.x == 10 && body.patrolMotion == 0x21);

    ScriptBody triangleBody; triangleBody.pos = sensor.pos; triangleBody.target = body.target;
    GunSoldierAI triangle(triangleBody, sensor, 2, 3);
    triangle.ConfigurePatrol(misc, sizeof misc, 0);
    triangle.Init();
    CHECK(triangle.patrolPoint == 1);
    triangle.subStep = 3; triangle.timer = -1;
    triangle.Update(0); CHECK(triangle.patrolPoint == 2);
    triangle.subStep = 3; triangle.timer = -1;
    triangle.Update(0); CHECK(triangle.patrolPoint == 0);
    triangle.subStep = 3; triangle.timer = -1;
    triangle.Update(0); CHECK(triangle.patrolPoint == 1);

    ScriptBody randomBody; randomBody.pos = sensor.pos; randomBody.target = body.target;
    GunSoldierAI random(randomBody, sensor, 3, 2);
    random.ConfigurePatrol(misc, sizeof misc, 0);
    random.Init();
    CHECK(random.patrolPoint == 3 && random.patrolRadius == 20.0f);
    CHECK(random.patrolTarget.x == -10.0f && random.patrolTarget.z == 20.0f);
    CHECK(random.patrolPoints[3].waitSec == 1.0f); // RANDOM_MOVE uses Pos0 wait settings
    random.subStep = 3; random.timer = -1;
    random.Update(0);
    CHECK(random.patrolPoint == 3 && random.subStep == 0);
}

int main(int argc, char** argv) {
    const std::string root = argc > 1 ? argv[1] : "../../files";
    SetData set;
    CHECK(set.LoadStage(root, 100, false, 5));
    // first GUN_SOLDIER with a ranged weapon and a non-zero sensor width
    SetRecord* rec = nullptr;
    float p[23];
    for (int i = 0; i < set.recordCount(); ++i) {
        SetRecord& r = set.records()[i];
        if (r.id != 0x64 || r.miscLen != 92) continue;
        std::memcpy(p, r.misc, 92);
        int32_t weapon, appear;
        std::memcpy(&weapon, r.misc + 32, 4);
        std::memcpy(&appear, r.misc + 36, 4);
        if (p[3] > 0.0f && weapon >= 2 && appear <= 1) { rec = &r; break; }
    }
    CHECK(rec != nullptr);
    if (!rec) return 1;
    int32_t weapon, appear;
    std::memcpy(&weapon, rec->misc + 32, 4);
    std::memcpy(&appear, rec->misc + 36, 4);
    std::printf("soldier @(%.0f %.0f %.0f) MoveRange %.0f SearchRange %.0f Angle %.0f Width %.0f Height %.0f weapon %d appear %d\n",
                rec->pos.x, rec->pos.y, rec->pos.z, p[0], p[1], p[2], p[3], p[4], weapon, appear);

    EnemySensor sensor;
    const float k = 3.14159265f / 180.0f;
    sensor.Build(rec->pos, {rec->rotDeg.x * k, rec->rotDeg.y * k, rec->rotDeg.z * k}, p);
    ScriptBody body;
    body.pos = rec->pos;
    body.target = {rec->pos.x + 5000, rec->pos.y, rec->pos.z};      // far away
    GunSoldierAI ai(body, sensor, appear, weapon);
    ai.home = rec->pos;
    ai.ConfigurePatrol(rec->misc, rec->miscLen, rec->rotDeg.y * k);
    ai.Init();
    CHECK(St(ai) == (appear == 0 ? "WaitAction_Standing" : "WaitAction_Moving"));

    ai.Update(1 / 60.f);                              // player far: stays waiting
    CHECK(St(ai).rfind("WaitAction", 0) == 0);

    body.target = sensor.centre;                      // player walks into the detection cylinder
    ai.Update(1 / 60.f);
    CHECK(St(ai) == "Caution");
    body.flag3B = true;                               // caution motion ends -> StartCombat
    ai.Update(1 / 60.f);
    CHECK(St(ai) == "Attack");
    body.flag3B = false;
    ai.Update(1 / 60.f);
    CHECK(body.fireCount == 1);                       // aimed -> fire motion, 3 s timer

    body.target = {rec->pos.x + 5000, rec->pos.y, rec->pos.z};      // player leaves
    ai.Update(1 / 60.f);
    CHECK(St(ai) == "GoHome");
    ai.Update(1 / 60.f);                              // body already at home -> ReturnToWait
    CHECK(St(ai).rfind("WaitAction", 0) == 0);

    ai.in.dead = true;                                // HP reached 0: death edge
    ai.Update(1 / 60.f);
    CHECK(St(ai) == "ToHinshi" && ai.sm.locked);
    body.target = sensor.centre;
    ai.Update(1 / 60.f);                              // locked: detection cannot interrupt
    CHECK(St(ai) == "ToHinshi");
    ai.in.dead = false;                               // status revives into injured form (vf14 path)
    body.flag3A = true;
    ai.Update(1 / 60.f);
    CHECK(St(ai) == "Hinshi" && ai.sm.locked);
    for (int i = 0; i < 31 * 60; ++i) ai.Update(1 / 60.f);   // 30 s timer -> "help" SE
    CHECK(body.seCount >= 1);

    ai.OnRescued();                                   // RecoverCommand path
    CHECK(St(ai) == "Thanks");
    body.damaged = true;
    ai.Update(1 / 60.f);
    CHECK(St(ai) == "ChaseTarget");

    TestPatrol();

    std::printf(g_fail ? "FAILED (%d)\n" : "ALL PASS\n", g_fail);
    return g_fail ? 1 : 0;
}

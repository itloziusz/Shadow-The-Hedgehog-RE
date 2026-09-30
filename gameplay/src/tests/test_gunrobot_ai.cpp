// Release-active GUN Robot branch tests. No assert(), which NDEBUG removes.
#include "shadow/gameplay/GunRobotAI.hpp"
#include <cstdio>
#include <string>
#include <vector>

using namespace shadow::gameplay;
static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); ++failures; } } while (0)

struct Body final : GunRobotBody {
    bool detected = false, motionEnded = false, targetInArea = true;
    bool arrived = false, turned = false;
    int standingTicks = 0, movingTicks = 0, cautionTicks = 0, deaths = 0;
    int motion = -1, motionSets = 0;
    Vec3 position{4.0f, 2.0f, 8.0f}, setPos{1.0f, 2.0f, 3.0f};
    Vec3 moveTarget{}, randomCentre{};
    float randomRadius = 0.0f;
    mutable std::vector<std::string> calls;

    bool DetectNearestPlayer(const EnemySensor&) override { calls.emplace_back("detect"); return detected; }
    void BeginStandingWait() override { calls.emplace_back("begin-standing"); }
    void TickStandingWait() override { ++standingTicks; calls.emplace_back("tick-standing"); }
    void BeginMovingWait() override { calls.emplace_back("begin-moving"); }
    void TickMovingWait() override { ++movingTicks; calls.emplace_back("tick-moving"); }
    void BeginReturnHome() override { calls.emplace_back("begin-return"); }
    bool AdvanceTowardMoveTarget() override { calls.emplace_back("advance"); return arrived; }
    void SetMoveTarget(const Vec3& p) override { calls.emplace_back("target"); moveTarget = p; }
    Vec3 SetPosition() const override { return setPos; }
    Vec3 Position() const override { return position; }
    Vec3 PickRandomPointNear(const Vec3& centre, float radius) override {
        calls.emplace_back("random"); randomCentre = centre; randomRadius = radius;
        return {9.0f, 2.0f, 6.0f};
    }
    void SetMotion(int m, bool restart) override {
        calls.emplace_back("motion"); motion = m; ++motionSets;
        CHECK(restart);
    }
    bool MotionEnded() const override { return motionEnded; }
    bool TrackedTargetInSearchArea(const EnemySensor&, const Vec3& target) const override {
        CHECK(target.x == 3.0f && target.y == 0.0f && target.z == 0.0f);
        calls.emplace_back("search"); return targetInArea;
    }
    Vec3 TrackedTargetPosition() const override {
        calls.emplace_back("tracked"); return {3.0f, 0.0f, 0.0f};
    }
    void FaceTrackedTarget() override { calls.emplace_back("face"); }
    bool TurnToTargetYaw() override { calls.emplace_back("turn"); return turned; }
    void BeginBaseCaution() override { calls.emplace_back("begin-caution"); }
    void UpdateBaseCaution(GunRobotAI&) override { ++cautionTicks; }
    void ApplyDeathEffects() override { ++deaths; }
};

static std::string State(const GunRobotAI& ai) {
    return ai.sm.cur ? ai.sm.cur->Name() : "null";
}

int main() {
    const EnemySensor sensor{};
    for (int appear : {-1, 0, 1, 2, 3, 4}) {
        Body b;
        GunRobotAI ai(b, sensor, appear, 0, 0);
        ai.Init();
        CHECK(State(ai) == ((appear >= 1 && appear <= 3)
            ? "EnemyWalkerAI_ReturnHome" : "EnemyWalkerAI_WaitAction_Standing"));
        if (appear >= 1 && appear <= 3) {
            CHECK(b.moveTarget.x == b.setPos.x && b.moveTarget.z == b.setPos.z);
            b.arrived = false;
            ai.Update(0.0f);
            CHECK(State(ai) == "EnemyWalkerAI_ReturnHome");
            b.arrived = true;
            ai.Update(0.0f);
            CHECK(State(ai) == "EnemyWalkerAI_WaitAction_Standing");
        }
    }
    {
        Body b;
        GunRobotAI ai(b, sensor, 0, 0, 3);
        ai.Init();
        CHECK(State(ai) == "EnemyWalkerAI_WaitAction_Moving");
        ai.Update(0.0f);
        CHECK(b.movingTicks == 1);
        b.detected = true;
        ai.Update(0.0f);
        CHECK(State(ai) == "EnemyBaseAI_Caution" && b.movingTicks == 1);
        ai.Update(0.0f);
        CHECK(b.cautionTicks == 1);
        ai.ReturnToWait();
        CHECK(State(ai) == "EnemyWalkerAI_WaitAction_Moving");
    }
    for (int weapon : {-1, 0, 1, 2, 3, 4, 5, 6}) {
        Body b;
        GunRobotAI ai(b, sensor, 0, weapon, 0);
        const int expected = (weapon == 0 || weapon == 1 || weapon == 5) ? 4
                           : (weapon >= 2 && weapon <= 4) ? 5 : 2;
        CHECK(ai.AttackMotionId() == expected);
        ai.StartAttack();
        CHECK(State(ai) == "GunRobotAI_WeaponAttack" && b.motion == expected);
        ai.Update(0.0f);
        CHECK(ai.subStep == 0 && b.motionSets == 1); // motion-end gate
        b.motionEnded = true;
        ai.Update(0.0f);
        CHECK(ai.subStep == 1 && b.motion == 0 && b.motionSets == 2);
        CHECK(b.randomCentre.x == b.position.x && b.randomCentre.z == b.position.z);
        CHECK(b.randomRadius == 30.0f);
        CHECK(b.moveTarget.x == 9.0f && b.moveTarget.z == 6.0f);
        b.arrived = false;
        const auto before = b.calls.size();
        ai.Update(0.0f);
        CHECK(ai.subStep == 1);
        CHECK(b.calls.size() == before + 3);
        CHECK(b.calls[before] == "tracked" && b.calls[before + 1] == "search"
              && b.calls[before + 2] == "advance");
        b.arrived = true;
        ai.Update(0.0f);
        CHECK(ai.subStep == 2 && b.calls.back() == "face");
        b.turned = false;
        ai.Update(0.0f);
        CHECK(ai.subStep == 2 && b.motionSets == 2);
        b.turned = true;
        ai.Update(0.0f);
        CHECK(ai.subStep == 0 && b.motion == expected && b.motionSets == 3);
        ai.subStep = 9; // out-of-range dispatch does nothing in the DOL
        ai.Update(0.0f);
        CHECK(ai.subStep == 9 && b.motionSets == 3);
    }
    {
        Body b;
        GunRobotAI ai(b, sensor, 0, 2, 3);
        ai.StartAttack();
        ai.subStep = 1;
        b.targetInArea = false;
        const auto before = b.calls.size();
        ai.Update(0.0f);
        CHECK(State(ai) == "EnemyWalkerAI_WaitAction_Moving");
        CHECK(b.calls.back() == "begin-moving"); // no movement after lost target
        CHECK(b.calls[before] == "tracked" && b.calls[before + 1] == "search");
    }
    {
        Body b;
        GunRobotAI ai(b, sensor, 0, 2, 0);
        ai.StartAttack();
        ai.in.dead = true;
        ai.Update(0.0f);
        CHECK(b.deaths == 1 && State(ai) == "GunRobotAI_WeaponAttack");
        ai.Update(0.0f);
        CHECK(b.deaths == 1); // death edge is one-shot
    }
    std::printf(failures ? "GUN Robot failures: %d\n" : "GUN Robot tests passed\n", failures);
    return failures ? 1 : 0;
}

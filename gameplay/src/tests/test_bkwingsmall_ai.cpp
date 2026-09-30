// Release-active decision tests. Engine hooks record call order and never supply
// guessed motion, path or attack displacement.
#include "shadow/gameplay/BkWingSmallAI.hpp"
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace shadow::gameplay;
static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); ++failures; } } while (0)

struct Body final : BkWingSmallBody {
    Vec3 target{1000, 0, 0};
    float pathDistance = 0, pathAdvance = 0;
    bool motionFlagA = false;
    int turns = 0, faces = 0, floatUpdates = 0, floatEnters = 0;
    int pathEnters = 0, pathAdvances = 0, cautionEnters = 0, attackEnters = 0;
    int attackUpdates = 0, deathEffects = 0, deletes = 0;
    float pathScaleA = 0, pathScaleB = 0;
    std::vector<std::string> calls;

    Vec3 TargetPosition() const override { return target; }
    void TurnTowards(const Vec3&) override { ++turns; calls.emplace_back("turn"); }
    void FaceSpawnHeading() override { ++faces; calls.emplace_back("face"); }
    void UpdateFloatMotion(float) override { ++floatUpdates; calls.emplace_back("float"); }
    void EnterFloatingMotion() override { ++floatEnters; calls.emplace_back("enter_float"); }
    void EnterPathMotion() override { ++pathEnters; calls.emplace_back("enter_path"); }
    void SetPathScales(float a, float b) override { pathScaleA = a; pathScaleB = b; }
    float PathDistance() const override { return pathDistance; }
    void AdvanceOnPath(float dt) override {
        ++pathAdvances; pathDistance += pathAdvance * dt; calls.emplace_back("advance");
    }
    void EnterCautionMotion() override { ++cautionEnters; calls.emplace_back("enter_caution"); }
    bool MotionFlagA() const override { return motionFlagA; }
    void EnterAttackMotion() override { ++attackEnters; calls.emplace_back("enter_attack"); }
    void UpdateAttackMotion(float) override { ++attackUpdates; calls.emplace_back("attack_hook"); }
    void PlayDeathEffect() override { ++deathEffects; calls.emplace_back("death_effect"); }
    void RequestDelete() override { ++deletes; calls.emplace_back("delete"); }
};

static EnemySensor Sensor() {
    EnemySensor s;
    s.centre = {0, 0, 0}; s.centreXZ = {0, 0, 0};
    s.width = 5; s.widthSq = 25; s.height = 5;
    return s;
}
static bool Near(float a, float b) { return std::fabs(a - b) < 0.0001f; }
static std::string State(const BkWingSmallAI& ai) { return ai.sm.cur ? ai.sm.cur->Name() : "null"; }

static void TestFloating() {
    const auto sensor = Sensor();
    Body b; BkWingSmallAI ai(b, sensor, 0, 1);
    ai.ConfigureFloat(0.1f);
    ai.Init();
    CHECK(State(ai) == "WaitFloating" && b.floatEnters == 1 && ai.ShouldActivate());
    ai.Update(0.1f);
    CHECK(b.faces == 1 && b.floatUpdates == 0); // strict width > 0.1
    ai.ConfigureFloat(2.0f);
    ai.Update(0.1f);
    CHECK(b.faces == 2 && b.floatUpdates == 1);
    b.target = {0, 0, 0};
    ai.Update(0.1f);
    CHECK(State(ai) == "Caution" && b.turns == 1 && b.cautionEnters == 1);
    CHECK(b.floatUpdates == 1); // transition returns before float motion
    ai.Update(0.1f);
    CHECK(State(ai) == "Caution" && b.turns == 2 && b.attackEnters == 0);
    b.motionFlagA = true;
    ai.Update(0.1f);
    CHECK(State(ai) == "Attack" && b.turns == 3 && b.attackEnters == 1);
    CHECK(ai.timer == 3.0f && ai.timerReset == 3.0f && ai.subStep == 0);
    ai.Update(0.25f);
    CHECK(b.attackUpdates == 1 && ai.timer == 2.75f); // update is explicitly hooked

    Body passive; passive.target = {0, 0, 0};
    BkWingSmallAI noAttack(passive, sensor, 0, 0);
    noAttack.ConfigureFloat(2.0f); noAttack.Init(); noAttack.Update(0.1f);
    CHECK(State(noAttack) == "WaitFloating" && passive.turns == 1);
    CHECK(passive.floatUpdates == 1 && passive.cautionEnters == 0);
}

static void TestPathWindow() {
    const auto sensor = Sensor();
    Body b; b.pathDistance = 39; b.pathAdvance = 20;
    BkWingSmallAI ai(b, sensor, 1, 1);
    CHECK(ai.UsesPath() && ai.HasAttackWindow() && !ai.ShouldActivate());
    b.target = {0, 0, 0}; CHECK(ai.ShouldActivate());
    b.target = {1000, 0, 0};
    CHECK(!ai.InAttackWindow()); // no engine path yet, no fabricated window
    ai.ConfigurePath(100, 10, 2, 0.4f, 0.6f);
    CHECK(Near(ai.pathSpeed, 20) && Near(ai.windowStart, 40) && Near(ai.windowEnd, 60));
    CHECK(Near(ai.windowDuration, 1.0f));
    b.pathDistance = ai.windowStart; CHECK(ai.InAttackWindow());
    b.pathDistance = ai.windowEnd; CHECK(ai.InAttackWindow());
    b.pathDistance = ai.windowEnd + 0.01f; CHECK(!ai.InAttackWindow());
    b.pathDistance = 39;
    ai.Init();
    CHECK(State(ai) == "MoveOnPath" && ai.subStep == 0 && b.pathEnters == 1);
    CHECK(b.pathScaleA == 1 && b.pathScaleB == 1);
    ai.Update(0.1f); // window check before path advance
    CHECK(State(ai) == "MoveOnPath" && b.pathDistance == 41 && b.pathAdvances == 1);
    ai.Update(0.1f);
    CHECK(State(ai) == "Caution" && ai.subStep == 1 && b.pathAdvances == 2);
    CHECK(b.calls[b.calls.size()-2] == "enter_caution" && b.calls.back() == "advance");
    b.motionFlagA = true;
    ai.Update(0.1f);
    CHECK(State(ai) == "Attack" && b.attackEnters == 1);

    Body other; other.pathDistance = 50;
    BkWingSmallAI nonattack(other, sensor, 1, 0);
    nonattack.ConfigurePath(100, 10, 1, 0.4f, 0.6f);
    CHECK(!nonattack.InAttackWindow()); // BK-specific ActionType == 1
    nonattack.Init(); nonattack.Update(0.1f);
    CHECK(State(nonattack) == "MoveOnPath" && other.cautionEnters == 0);
    ai.ConfigurePath(100, 0.1f, 1, 0.4f, 0.6f);
    CHECK(ai.windowDuration == 1.0f); // <= 0.1 speed fallback
}

static void TestDead() {
    const auto sensor = Sensor();
    Body b; BkWingSmallAI ai(b, sensor, 0, 0); ai.Init();
    ai.in.dead = true;
    ai.Update(0.1f);
    CHECK(b.deathEffects == 1 && b.deletes == 1);
    bool adjacent = false;
    for (std::size_t i = 1; i < b.calls.size(); ++i)
        adjacent |= b.calls[i-1] == "death_effect" && b.calls[i] == "delete";
    CHECK(adjacent); // PreUpdate delivers death before the current state's Update
    ai.Update(0.1f);
    CHECK(b.deathEffects == 1 && b.deletes == 1); // only on dead edge
}

int main() {
    TestFloating(); TestPathWindow(); TestDead();
    std::printf(failures ? "FAILED (%d)\n" : "ALL PASS\n", failures);
    return failures ? 1 : 0;
}

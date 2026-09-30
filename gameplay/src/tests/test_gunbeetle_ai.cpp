// Decision tests for PAL GUPP8P GunBeetleAI. The body records engine calls;
// it does not substitute a guessed path follower or weapon implementation.
#include "shadow/gameplay/GunBeetleAI.hpp"
#include <cmath>
#include <cstdio>
#include <string>

using namespace shadow::gameplay;
static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); ++failures; } } while (0)

struct Body final : GunBeetleBody {
    Vec3 target{}, lastAim{};
    float pathDistance = 0.0f, pathSpeed = 0.0f;
    int aims = 0, headings = 0, floats = 0, floatEnters = 0, pathEnters = 0;
    int advances = 0, starts = 0, stops = 0, effects = 0, deletes = 0;
    Vec3 TargetPosition() const override { return target; }
    void AimAt(const Vec3& p) override { lastAim = p; ++aims; }
    void FaceSpawnHeading() override { ++headings; }
    void UpdateFloatMotion(float) override { ++floats; }
    void EnterFloatingMotion() override { ++floatEnters; }
    void EnterPathMotion() override { ++pathEnters; }
    float PathDistance() const override { return pathDistance; }
    void AdvanceOnPath(float dt) override { ++advances; pathDistance += pathSpeed * dt; }
    void StartFiring() override { ++starts; }
    void StopFiring() override { ++stops; }
    void PlayDeathEffect() override { ++effects; }
    void RequestDelete() override { ++deletes; }
};

static EnemySensor Sensor() {
    EnemySensor s;
    s.pos = {0, 0, 0}; s.centre = {0, 0, 0}; s.centreXZ = {0, 0, 0};
    s.width = 5; s.widthSq = 25; s.height = 5;
    return s;
}
static std::string State(const GunBeetleAI& ai) { return ai.sm.cur ? ai.sm.cur->Name() : "null"; }

static void TestFloating() {
    const auto sensor = Sensor();
    Body body; body.target = {20, 0, 0};
    GunBeetleAI ai(body, sensor, 0, 2);
    ai.ConfigureFloat(0.1f);
    ai.Init();
    CHECK(State(ai) == "WaitFloating" && body.floatEnters == 1 && ai.ShouldActivate());
    ai.Update(0.1f);
    CHECK(State(ai) == "WaitFloating" && body.headings == 1 && body.floats == 0); // strict > 0.1
    ai.ConfigureFloat(2.0f);
    ai.Update(0.1f);
    CHECK(body.floats == 1 && body.headings == 2);
    body.target = {0, 0, 0};
    ai.Update(0.1f);
    CHECK(State(ai) == "Caution" && ai.timer == 1.0f && body.aims == 1 && body.starts == 0);
    ai.Update(1.0f); // zero is not expired (0x801847E0)
    CHECK(State(ai) == "Caution" && body.starts == 0 && body.lastAim.y == 10.0f);
    ai.Update(0.01f);
    CHECK(State(ai) == "Attack" && ai.timer == 3.0f && body.starts == 1);
    ai.Update(3.0f); // zero is not expired (0x80184644)
    CHECK(State(ai) == "Attack" && body.stops == 0);
    ai.Update(0.01f);
    CHECK(State(ai) == "Caution" && body.stops == 1 && ai.timer == 1.0f);
    ai.Update(1.01f);
    CHECK(State(ai) == "Attack" && body.starts == 2);
    body.target = {20, 0, 0};
    ai.Update(0.01f);
    CHECK(State(ai) == "WaitFloating" && body.stops == 2 && body.floatEnters == 2);
    ai.in.dead = true;
    ai.Update(0.01f);
    CHECK(body.effects == 1 && body.deletes == 1 && State(ai) == "WaitFloating");
    ai.Update(0.01f);
    CHECK(body.effects == 1 && body.deletes == 1); // life event only on dead edge

    Body passiveBody; passiveBody.target = {0, 0, 0};
    GunBeetleAI passive(passiveBody, sensor, 0, 1); // spark-only ActionType
    passive.Init(); passive.Update(0.01f);
    CHECK(State(passive) == "WaitFloating" && passiveBody.starts == 0 && passiveBody.aims == 1);

    // The original window helper itself tests ActionType and distances, not
    // AppearType (0x80193AE8..0x80193B30); AI calls it only in path mode.
    ai.ConfigurePath(100, 10, 1, 0.25f, 0.75f);
    body.pathDistance = 50;
    CHECK(ai.InAttackWindow());
}

static void TestPath() {
    const auto sensor = Sensor();
    Body body; body.target = {1000, 2, 0}; body.pathDistance = 24.0f; body.pathSpeed = 20.0f;
    GunBeetleAI ai(body, sensor, 1, 2);
    CHECK(!ai.ShouldActivate());  // 0x80193CAC: hidden until detected
    body.target = {0, 0, 0};
    CHECK(ai.ShouldActivate());
    body.target = {1000, 2, 0}; // path mode never senses after activation
    CHECK(!ai.InAttackWindow()); // no engine path configured, no invented attack window
    ai.ConfigurePath(100.0f, 10.0f, 2.0f, 0.25f, 0.75f);
    CHECK(ai.pathSpeed == 20.0f && ai.windowStart == 25.0f && ai.windowEnd == 75.0f);
    CHECK(ai.windowDuration == 2.5f);
    body.pathDistance = 25.0f; CHECK(ai.InAttackWindow());
    body.pathDistance = 75.0f; CHECK(ai.InAttackWindow());
    body.pathDistance = 75.001f; CHECK(!ai.InAttackWindow());
    body.pathDistance = 24.0f;
    ai.Init();
    CHECK(State(ai) == "MoveOnPath" && ai.onPath && body.pathEnters == 1);
    ai.Update(0.1f);  // the window check precedes AdvanceOnPath
    CHECK(body.starts == 0 && ai.subStep == 0 && body.pathDistance == 26.0f);
    ai.Update(0.1f);
    CHECK(body.starts == 1 && ai.subStep == 1 && ai.timer == 2.5f && body.pathDistance == 28.0f);
    body.pathDistance = 50.0f;
    ai.Update(0.5f);
    CHECK(body.lastAim.y == 4.0f); // target y=2 + 10*(1-2/2.5)
    CHECK(body.stops == 0);
    body.pathDistance = 76.0f;
    ai.Update(0.1f);
    CHECK(ai.subStep == 0 && body.stops == 1 && body.advances == 4);
    CHECK(body.aims == 2); // leaving the window still aims before stopping
    ai.OnTargetFound();
    CHECK(!ai.onPath && State(ai) == "Caution"); // Exit clears E+0x30C

    Body noAttack; noAttack.pathDistance = 50.0f; noAttack.target = {1000, 0, 0};
    GunBeetleAI spark(noAttack, sensor, 1, 1);
    spark.ConfigurePath(100, 10, 1, 0.25f, 0.75f);
    spark.Init(); spark.Update(0.1f);
    CHECK(State(spark) == "MoveOnPath" && noAttack.starts == 0 && noAttack.advances == 1);
    CHECK(!spark.InAttackWindow()); // ActionType must equal 2

    // Non-positive/near-zero path speed falls back to 1 s at 0x80193C48..0x80193C78.
    spark.ConfigurePath(100, 0.1f, 1, 0.25f, 0.75f);
    CHECK(spark.windowDuration == 1.0f);
}

int main() {
    TestFloating(); TestPath();
    std::printf(failures ? "FAILED (%d)\n" : "ALL PASS\n", failures);
    return failures ? 1 : 0;
}

// Release-active BK Worm decision regression; body methods are labelled engine hooks.
#include "shadow/gameplay/BkWormAI.hpp"
#include <iostream>
#include <string>

using namespace shadow::gameplay;

namespace {
int failures = 0;
void Check(bool okay, const char* expr, int line) {
    if (!okay) { std::cerr << "line " << line << ": " << expr << '\n'; ++failures; }
}
#define CHECK(x) Check(static_cast<bool>(x), #x, __LINE__)

struct Body final : BkWormBody {
    Vec3 position{2, 0, 3}, target{200, 0, 0}, moveHome{20, 0, 5};
    Vec3 lastTurn{};
    bool turnDone = false, motionEnded = false;
    int initial = 0, waitAppear = 0, emergence = 0, appear = 0, standing = 0;
    int attack = 0, after = 0, moving = 0, travel = 0, complete = 0;
    float lastLo = 0, lastHi = 0;
    Vec3 Position() const override { return position; }
    Vec3 TargetPosition() const override { return target; }
    Vec3 TargetDirection() const override {
        return {target.x - position.x, target.y - position.y, target.z - position.z};
    }
    bool TurnTowards(const Vec3& v) override { lastTurn = v; return turnDone; }
    bool MotionEnded() const override { return motionEnded; }
    float Uniform(float lo, float hi) override { lastLo = lo; lastHi = hi; return lo; }
    void InitialSetup() override { ++initial; }
    void BeginBuriedWait() override { ++waitAppear; }
    void PrepareEmergence(const Vec3& p) override {
        CHECK(p.x == position.x && p.z == position.z);
        ++emergence;
    }
    void BeginAppear() override { ++appear; }
    void BeginStanding() override { ++standing; }
    void BeginAttack() override { ++attack; }
    void BeginAfterAttack() override { ++after; }
    Vec3 BeginMove() override { ++moving; return moveHome; }
    void BeginMoveTravel() override { ++travel; }
    void CompleteMoveTravel() override { ++complete; }
};

bool Is(const BkWormAI& ai, const char* state) {
    return ai.sm.cur && std::string(ai.sm.cur->Name()) == state;
}
} // namespace

int main() {
    EnemySensor sensor;
    const float params[6] = {100, 0, 0, 100, 100, 0};
    sensor.Build({0, 0, 0}, {0, 0, 0}, params);
    Body body;
    BkWormAI ai(body, sensor, 2, 2.0f);
    ai.Init();
    CHECK(body.initial == 1 && body.waitAppear == 1);
    CHECK(Is(ai, "WaitAppear") && ai.buried && ai.subStep == 0);
    CHECK(ai.timer == 2.0f);

    // 0x80183490 and 0x801834FC both require timer < 0, not <= 0.
    ai.Update(2.0f);
    CHECK(Is(ai, "WaitAppear") && body.emergence == 0);
    ai.Update(0.01f);
    CHECK(Is(ai, "WaitAppear") && body.emergence == 0); // outside sensor
    body.target = {10, 0, 0};
    ai.Update(0.0f);
    CHECK(Is(ai, "WaitAppear") && ai.subStep == 1 && body.emergence == 1);
    CHECK(ai.timer == 1.0f);
    ai.Update(1.0f);
    CHECK(Is(ai, "WaitAppear"));
    ai.Update(0.01f);
    CHECK(Is(ai, "Appear") && !ai.buried && body.appear == 1);

    ai.Update(0.0f);
    CHECK(Is(ai, "Appear"));
    body.motionEnded = true;
    ai.Update(0.0f);
    CHECK(Is(ai, "WaitStanding") && body.standing == 1);
    CHECK(body.lastLo == 1.0f && body.lastHi == 3.0f && ai.timer == 1.0f);
    ai.Update(1.0f);
    CHECK(Is(ai, "WaitStanding"));
    ai.Update(0.01f);
    CHECK(Is(ai, "Attack") && ai.remainingAttacks == 2 && body.attack == 1);

    body.motionEnded = false;
    ai.Update(0.0f);
    CHECK(Is(ai, "Attack"));
    body.motionEnded = true;
    ai.Update(0.0f);
    CHECK(Is(ai, "AfterAttack") && body.after == 1);
    CHECK(body.lastLo == 0.8f && body.lastHi == 1.2f);
    ai.Update(0.8f);
    CHECK(Is(ai, "AfterAttack") && ai.remainingAttacks == 2);
    ai.Update(0.01f);
    CHECK(Is(ai, "Attack") && ai.remainingAttacks == 1 && body.attack == 2);
    ai.Update(0.0f);
    CHECK(Is(ai, "AfterAttack") && body.after == 2);
    ai.Update(0.81f);
    CHECK(Is(ai, "Move") && ai.remainingAttacks == 0 && ai.sm.locked);
    CHECK(body.moving == 1 && ai.home.x == 20);

    // 0x801829D0 turns toward E+0x6C minus current world position.
    body.turnDone = false;
    ai.Update(0.0f);
    CHECK(Is(ai, "Move") && ai.subStep == 0 && body.travel == 0);
    CHECK(body.lastTurn.x == 18 && body.lastTurn.z == 2);
    body.turnDone = true;
    ai.Update(0.0f);
    CHECK(Is(ai, "Move") && ai.subStep == 1 && ai.buried && body.travel == 1);
    body.motionEnded = false;
    ai.Update(0.0f);
    CHECK(Is(ai, "Move"));
    body.motionEnded = true;
    ai.Update(0.0f);
    CHECK(Is(ai, "WaitAppear") && !ai.sm.locked && body.complete == 1);
    CHECK(ai.timer == 2.0f && body.waitAppear == 2);

    // 0x801838A0 dispatches by AI+0x7C without looking at attack count.
    ai.sm.ChangeState(&BkWormAI::Appear());
    ai.ReturnToWait();
    CHECK(Is(ai, "WaitStanding") && body.standing == 2);
    ai.buried = true;
    ai.ReturnToWait();
    CHECK(Is(ai, "WaitAppear") && body.waitAppear == 3);

    if (failures) return 1;
    std::cout << "BkWormAI native decisions passed\n";
    return 0;
}

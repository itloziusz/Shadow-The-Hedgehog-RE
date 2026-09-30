#include "shadow/gameplay/BkLarva.hpp"
#include <cstdint>
#include <cstring>
#include <cstdio>

using namespace shadow::gameplay;

// CTest builds Release by default; standard assert would disappear under NDEBUG.
static int failures = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); ++failures; } } while (0)

namespace {
struct Gate : BkLarvaEngine {
    bool stage = false, trigger = false;
    bool StageAllowsGenerator(const SetSlot&) override { return stage; }
    bool SpawnTriggerReached(const SetSlot&) override { return trigger; }
    void UpdateLarvaMotion(const SetSlot&, int, float) override {}
    void OnGeneratorDestroyed(const SetSlot&) override {}
};

struct Fixture {
    static constexpr uint32_t kParamBytes = 11u * sizeof(uint32_t);
    TaskManager tasks;
    EnemyManager enemies;
    Gate gate;
    uint32_t params[11] = {};
    static_assert(sizeof params == kParamBytes, "BK_LARVA SET parameters must be 0x2C bytes");
    SetRecord record{};
    SetSlot slot{};
    BkLarvaGenerator* gen = nullptr;
    explicit Fixture(int num, uint32_t len = kParamBytes) {
        params[7] = static_cast<uint32_t>(num);  // Num at SET params +0x1C
        record.id = 0x91;
        record.link = 7;
        record.misc = reinterpret_cast<const uint8_t*>(params);
        record.miscLen = len;
        slot.rec = &record;
        enemies.groupAlive[7] = num;
        gen = new BkLarvaGenerator(tasks.GetLayer(TaskManager::Enemy), slot, enemies, gate);
    }
    ~Fixture() { // tasks must die before their referenced slot/manager/gate members
        Task* layer = tasks.GetLayer(TaskManager::Enemy);
        while (layer->firstChild()) {
            layer->firstChild()->Kill();
            Task::UpdateChildren(layer, 0.0f);
        }
    }
    void Step() { tasks.Run(1.0f / 60.0f); }
};
}

int main() {
    {
        Fixture f(3);
        CHECK(f.gen->valid() && f.gen->requested() == 3);
        CHECK(f.slot.object == f.gen && (f.record.runtimeFlags & kSetFlag8));
        f.Step();                                    // stage gate closed
        CHECK(f.gen->spawned() == 0);
        f.gate.stage = true;
        f.Step();                                    // stage opens, trigger not reached
        CHECK(f.gen->state() == BkLarvaGenerator::State::WaitingTrigger);
        f.Step();
        CHECK(f.gen->spawned() == 0);
        f.gate.trigger = true;
        f.Step();                                    // state 1 falls into first spawn
        CHECK(f.gen->spawned() == 1 && f.gen->active() == 1);
        CHECK(f.gen->FirstAttackable()->ordinal() == 0);
        f.Step();
        CHECK(f.gen->spawned() == 2);
        f.Step();
        CHECK(f.gen->spawned() == 3 && f.gen->state() == BkLarvaGenerator::State::WaitingChildren);
        f.Step();                                    // no fourth larva
        CHECK(f.gen->spawned() == 3);
        BkLarva* first = f.gen->FirstAttackable();
        first->ReportDefeat(true);                    // collision path flags 1|4, Black Arms
        first->ReportDefeat(true);                    // duplicate contact cannot double-count
        CHECK(first->deathState() == 4);
        CHECK(f.enemies.defeated[TeamBlackArms] == 1);
        CHECK(f.enemies.defeatedByPlayer[TeamBlackArms] == 1);
        CHECK(f.enemies.groupAlive[7] == 2);
        f.gen->FirstAttackable()->ReportDefeat(false);
        f.gen->FirstAttackable()->ReportDefeat(true);
        CHECK(f.gen->defeated() == 3 && f.gen->active() == 0);
        CHECK(f.enemies.defeated[TeamBlackArms] == 3);
        CHECK(f.enemies.defeatedByPlayer[TeamBlackArms] == 2);
        CHECK(f.enemies.groupAlive[7] == 0);
        CHECK(f.gen->stateRemaining() == 3);       // collision event precedes state-4 update
        f.Step();                                    // state 4 -> 5, remaining count drops
        CHECK(first->deathState() == 5 && f.gen->stateRemaining() == 0);
        CHECK(f.slot.object != nullptr);            // game retains dying children for ~0.5 s
        for (int i = 0; i < 40; ++i) f.Step();       // timer, child reaping, generator reaping
        CHECK(f.slot.object == nullptr && !(f.record.runtimeFlags & kSetAlive));
    }
    {
        Fixture f(1);
        f.gate.stage = f.gate.trigger = true;
        f.Step();
        f.Step();
        auto* child = f.gen->FirstAttackable();
        CHECK(child);
        child->ReportCommand14Defeat(true);
        CHECK(f.gen->defeated() == 1 && f.gen->stateRemaining() == 0);
        CHECK(child->flags() & Task::kKillRequest); // command path requests deletion immediately
    }
    {
        Fixture f(0);                              // DOL postincrement: zero still creates one
        f.gate.stage = f.gate.trigger = true;
        f.Step();                                    // state 0 -> 1
        f.Step();                                    // state 1 -> 2 -> first child
        CHECK(f.gen->spawned() == 1 && f.gen->active() == 1);
        CHECK(f.gen->state() == BkLarvaGenerator::State::WaitingChildren);
        f.Step();
        CHECK(f.gen->spawned() == 1);
    }
    {
        Fixture f(0x100, 0x28);                    // malformed length; do not invent Num
        CHECK(!f.gen->valid());
        f.gate.stage = f.gate.trigger = true;
        f.Step();
        CHECK(f.gen->spawned() == 0 && f.slot.object == nullptr);
    }
    std::puts(failures ? "BkLarvaGenerator native tests FAILED" : "BkLarvaGenerator native tests passed");
    return failures ? 1 : 0;
}

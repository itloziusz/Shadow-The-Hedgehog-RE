// Checks that the reconstructed enemy lifecycle reproduces the component call order
// recovered from TEnemySetBase::vf07 (0x801A7280) and TEnemySetTask::Update (0x801A6EB8).
#include "shadow/gameplay/EnemyFramework.hpp"
#include <cstdio>
#include <string>

using namespace shadow::gameplay;
static std::string g_log;
static int g_fail = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); ++g_fail; } } while (0)

struct Status : IEnemyStatus {
    bool ready = false, fade = false;
    void Init() override { g_log += "S.init "; }
    void Start() override { g_log += "S.start "; }
    void PreUpdate() override { g_log += "S.pre "; }
    void Update1(float) override { g_log += "S.u1 "; }
    void Update2(float) override { g_log += "S.u2 "; }
    void Update3(float) override { g_log += "S.u3 "; }
    bool IsReady() override { g_log += "S.ready? "; return ready; }
    void PrepareReady() override { g_log += "S.prep "; }
    bool WantsFadeOut() override { return fade; }
};
struct Disp : IEnemyDisp {
    void Init() override { g_log += "D.init "; }
    void Update(float) override { g_log += "D.u "; }
    void PostUpdate(float) override { g_log += "D.post "; }
};
struct Move : IEnemyMove {
    void Init() override { g_log += "M.init "; }
    void Update(float) override { g_log += "M.u "; }
};
struct AI : IEnemyAI {
    void Init() override { g_log += "A.init "; }
    void Update(float) override { g_log += "A.u "; }
};
struct SetB : EnemySetBase {
    using EnemySetBase::EnemySetBase;
    void InitFromSlot(SetSlot*) override { g_log += "Set.init "; }
};

int main() {
    TaskManager tm;
    SetRecord rec{};
    rec.runtimeFlags = kSetEnabled;
    SetSlot slot;
    slot.rec = &rec;
    auto* e = new Enemy(tm.GetLayer(TaskManager::Enemy), &slot);
    auto* st = new Status;
    e->status.reset(st);
    e->disp.reset(new Disp);
    e->move.reset(new Move);
    e->ai.reset(new AI);
    e->setBase.reset(new SetB(*e));
    CHECK(rec.runtimeFlags & kSetAlive);

    tm.Run(1.0f / 60);   // frame 1: states 1 -> 2 -> 3, not ready yet
    CHECK(g_log == "Set.init D.init S.init A.init M.init S.prep S.ready? ");
    CHECK(e->setBase->state() == EnemySetBase::WaitReady);

    g_log.clear();
    st->ready = true;
    tm.Run(1.0f / 60);   // frame 2: becomes ready, starts
    CHECK(g_log == "S.prep S.ready? S.start ");
    CHECK(e->setBase->state() == EnemySetBase::Running);

    g_log.clear();
    tm.Run(1.0f / 60);   // frame 3: running order
    CHECK(g_log == "S.pre S.u1 A.u S.u2 D.u M.u D.post S.u3 ");

    // fade in: alpha rises 2/s, clamps at 1
    for (int i = 0; i < 60; ++i) tm.Run(1.0f / 60);
    CHECK(e->alpha == 1.0f);

    // despawn request (flag 0x4) kills the task; scheduler deletes enemy next traversal
    rec.runtimeFlags |= kSetDespawnRequest;
    tm.Run(1.0f / 60);
    tm.Run(1.0f / 60);
    CHECK(tm.GetLayer(TaskManager::Enemy)->firstChild() == nullptr);
    CHECK(slot.object == nullptr); // destructor mirrors SetSlot_Detach at 0x801A7090
    CHECK((rec.runtimeFlags & (kSetAlive | kSetDespawnRequest)) == 0);

    // A slot that has been reattached elsewhere must not be cleared by the old
    // enemy's destructor (the original skips detach after SetBase detachment).
    SetRecord rec2{};
    SetSlot slot2{};
    slot2.rec = &rec2;
    auto* old = new Enemy(tm.GetLayer(TaskManager::Enemy), &slot2);
    int replacement = 0;
    SetSlot_Attach(slot2, &replacement);
    old->task()->Kill();
    tm.Run(1.0f / 60);
    CHECK(slot2.object == &replacement && (rec2.runtimeFlags & kSetAlive));

    std::printf(g_fail ? "FAILED (%d)\n" : "ALL PASS\n", g_fail);
    return g_fail ? 1 : 0;
}

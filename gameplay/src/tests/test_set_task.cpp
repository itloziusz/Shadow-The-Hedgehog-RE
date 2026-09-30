// Behavioural test of the reconstructed Task scheduler + SET system on real
// stage data (files/stg0100). Checks structural invariants recovered from the
// DOL; it does not claim frame-exact equivalence with the original.
#include "shadow/gameplay/SetSystem.hpp"
#include "shadow/gameplay/Task.hpp"
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

using namespace shadow::gameplay;

static int g_fail = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); ++g_fail; } } while (0)

static std::vector<std::string> g_order;

struct ProbeTask : Task {
    ProbeTask(Task* p, const char* n) : Task(p, n) {}
    void Update(float) override { g_order.push_back(name()); }
};

// GUN_SOLDIER params (23 x 4 bytes) in catalog order — see STAGE_GAMEPLAY_DATA_FORMATS.md
struct GunSoldierParams {
    float moveRange, searchRange, searchAngle, searchWidth, searchHeight, searchHeightOffset, moveSpeedRatio;
    int32_t haveShield, weaponType, appearType, pos0WaitType;
    float pos0WaitSec, pos0MoveSpeedRatio, pos1X, pos1Z;
    int32_t pos1WaitType;
    float pos1WaitSec, pos1MoveSpeedRatio, pos2X, pos2Z;
    int32_t pos2WaitType;
    float pos2WaitSec, pos2MoveSpeedRatio;
};
static_assert(sizeof(GunSoldierParams) == 23 * 4, "param block size");

struct ProbeEnemy : Task {
    SetSlot* slot;
    int frames = 0;
    GunSoldierParams p{};
    bool paramsOk;
    ProbeEnemy(Task* layer, SetSlot& s) : Task(layer, "ProbeGunSoldier"), slot(&s) {
        paramsOk = SetSlot_GetParams(s, &p, sizeof p);
        SetSlot_Attach(s, this);
    }
    void Update(float) override {
        if (++frames == 3) {           // "dies" after 3 frames
            SetSlot_SetFlag8(*slot);
            Kill();
        }
    }
};

int main(int argc, char** argv) {
    const std::string root = argc > 1 ? argv[1] : "../../files";

    // 1. Layer order (PROVEN from tables 0x8053ED6C / 0x8053ED84)
    {
        TaskManager tm;
        for (int i = 0; i < TaskManager::LayerCount; ++i)
            new ProbeTask(tm.GetLayer((TaskManager::Layer)i), TaskManager::LayerName(i));
        tm.Run(1.0f / 60);
        std::string s;
        for (auto& n : g_order) s += n + ",";
        std::printf("update order: %s\n", s.c_str());
        CHECK(s == "System,Audio,Debug,Manager,Controller,Command,CharaColli,Landscape,Gadget,Vehicle,"
                   "Enemy,Player,PostManager,Particle,Editor,Camera,Scene,PostSystem,Render,");  // Scene probe is appended after its 13 sub-layers
    }

    // 2. Deferred kill semantics
    {
        TaskManager tm;
        Task* layer = tm.GetLayer(TaskManager::Enemy);
        auto* a = new ProbeTask(layer, "A");
        new ProbeTask(a, "A.child");
        new ProbeTask(layer, "B");
        a->Kill();
        g_order.clear();
        tm.Run(0.016f);
        std::string s;
        for (auto& n : g_order) s += n + ",";
        CHECK(s == "B,");                          // killed subtree neither updates nor survives
        CHECK(layer->firstChild() && std::strcmp(layer->firstChild()->name(), "B") == 0);
    }

    // 3. SET load for stage 100 (stage index 5), normal mode
    SetData set;
    bool ok = set.LoadStage(root, 100, false, 5);
    CHECK(ok);
    std::printf("stg0100 records: %d\n", set.recordCount());
    for (auto& l : set.log) std::printf("  log: %s\n", l.c_str());
    set.log.clear();
    std::map<int, int> perSrc;
    int gunSoldiers = 0, firstGs = -1;
    for (int i = 0; i < set.recordCount(); ++i) {
        const SetRecord& r = set.records()[i];
        perSrc[r.flags & 0xF000]++;
        if (r.id == 0x0064) { ++gunSoldiers; if (firstGs < 0) firstGs = i; }
    }
    for (auto& kv : perSrc) std::printf("  source 0x%04X: %d\n", kv.first, kv.second);
    CHECK(gunSoldiers == 24);   // tools/setparse.py: 24 in _cmn, 0 in _nrm, 0 in _ds1
    CHECK(firstGs >= 0);

    // 4. Spawn scan with the player standing on the first GUN soldier
    TaskManager tm;
    int created = 0;
    set.RegisterFactory(0x0064, [&](SetSlot& s) -> void* {
        ++created;
        return new ProbeEnemy(tm.GetLayer(TaskManager::Enemy), s);
    });
    Vec3 player = set.records()[firstGs].pos;
    set.Scan(&player, 1);
    std::printf("created GUN soldiers: %d, unimplemented types logged: %zu\n", created, set.log.size());
    CHECK(created >= 1);
    SetSlot& gs = set.slots()[firstGs];
    CHECK(gs.rec->runtimeFlags & kSetAlive);
    auto* probe = static_cast<ProbeEnemy*>(gs.object);
    CHECK(probe && probe->paramsOk);
    if (probe)
        std::printf("GS#%d params: MoveRange %.1f SearchRange %.1f WeaponType %d AppearType %d Pos1 (%.1f,%.1f)\n",
                    firstGs, probe->p.moveRange, probe->p.searchRange, probe->p.weaponType,
                    probe->p.appearType, probe->p.pos1X, probe->p.pos1Z);

    // 5. Second scan must not re-create alive objects (flag 0x2 gate in 0x800CA8D8)
    int before = created;
    set.Scan(&player, 1);
    CHECK(created == before);

    // 6. Run frames: probe dies on frame 3, is deleted by the scheduler
    for (int f = 0; f < 5; ++f) tm.Run(1.0f / 60);
    CHECK(gs.rec->runtimeFlags & kSetFlag8);
    CHECK(tm.GetLayer(TaskManager::Enemy)->firstChild() == nullptr);

    // Normal-enemy death 0x800C9EEC disables and detaches the slot. The
    // scanner must not spawn the same killed record on the following frame.
    set.MarkKilledAndDetach(gs);
    CHECK(gs.object == nullptr);
    CHECK((gs.rec->runtimeFlags & kSetFlag8) != 0);
    CHECK((gs.rec->runtimeFlags & (kSetEnabled | kSetAlive | kSetDespawnRequest)) == 0);
    for (SetSlot* node = set.linkHead(gs.rec->link); node; node = node->next)
        CHECK(node != &gs);
    const int afterKill = created;
    set.Scan(&player, 1);
    CHECK(created == afterKill && gs.object == nullptr);

    // Bit 0x8 itself is not a spawn veto: the scanner preserves it if an
    // external system explicitly sets rearm bit 0x40 (0x800CACA4..0x800CACF4).
    gs.rec->runtimeFlags |= kSetRearm;
    set.Scan(&player, 1);
    CHECK((gs.rec->runtimeFlags & (kSetEnabled | kSetFlag8)) == (kSetEnabled | kSetFlag8));
    CHECK((gs.rec->runtimeFlags & kSetRearm) == 0 && gs.object == nullptr);
    set.Scan(&player, 1);
    CHECK(gs.object != nullptr && created > afterKill);

    std::printf(g_fail ? "FAILED (%d)\n" : "ALL PASS\n", g_fail);
    return g_fail ? 1 : 0;
}

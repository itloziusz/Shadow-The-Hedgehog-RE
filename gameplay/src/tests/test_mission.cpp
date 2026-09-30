// End-to-end check of the reconstructed mission chain on real data:
// stage table (DOL) -> descriptor defaults (fn_8016CB48) + nukkoro2.inf counts ->
// MissionManager::InitStage(stg0100) -> EnemyManager census from the real SET files ->
// enemy deaths -> EnemyMission update -> Clear -> GoalAction request -> next stage.
#include "shadow/gameplay/Mission.hpp"
#include "shadow/gameplay/SetSystem.hpp"
#include "shadow/gameplay/StageFlow.hpp"
#include <cstdio>
#include <string>

using namespace shadow::gameplay;
static int g_fail = 0;
#define CHECK(c) do { if (!(c)) { std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); ++g_fail; } } while (0)

int main(int argc, char** argv) {
    const std::string root = argc > 1 ? argv[1] : "../../files";
    const auto& table = OriginalStageTable();
    CHECK(table.size() == 59);
    const int idx = StageTable_IndexOfId(100);
    CHECK(idx == 5);
    CHECK(std::string(table[idx].name) == "City1");
    CHECK(table[idx].route[0].nextIndex == 6 && table[6].id == 200);   // Dark -> Digital Circuit
    CHECK(table[idx].route[2].nextIndex == 8 && table[8].id == 202);   // Hero -> Lethal Highway

    auto descs = DefaultMissionDescs();
    int applied = ApplyMissionCountFile(root + "/nukkoro2.inf", descs);
    std::printf("nukkoro2.inf MISSIONCOUNT lines applied: %d\n", applied);
    CHECK(applied > 0);
    CHECK(descs[0x01].count == 35);   // [City1] MISSIONCOUNT_D : 35 0
    CHECK(descs[0x02].count == 45);   // [City1] MISSIONCOUNT_H : 45 0

    MissionManager mm(descs);
    mm.InitStage(idx, false);
    auto* dark = dynamic_cast<Mission::EnemyMission*>(mm.GetMission(0));
    auto* normal = mm.GetMission(1);
    auto* hero = dynamic_cast<Mission::EnemyMission*>(mm.GetMission(2));
    CHECK(dark && dark->team() == TeamGun && dark->required() == 35);
    CHECK(normal && normal->GetType() == MissionType::Goal);
    CHECK(hero && hero->team() == TeamBlackArms && hero->required() == 45);

    SetData set;
    CHECK(set.LoadStage(root, 100, false, idx));
    EnemyManager em;
    em.ScanSetEnemies(set);
    std::printf("census: total %d  GUN %d  Eggman %d  BlackArms %d\n", em.total, em.placed[0], em.placed[1], em.placed[2]);
    CHECK(em.placed[TeamGun] >= 35 && em.placed[TeamBlackArms] >= 45);   // requirement <= placed

    MissionContext ctx;
    ctx.enemies = &em;
    // Kill 34 GUN enemies: mission must still be running.
    int killed = 0;
    for (int i = 0; i < set.recordCount() && killed < 34; ++i) {
        const SetRecord& r = set.records()[i];
        if ((r.flags & kSetEnabled) && r.id >= 0x64 && r.id < 0x78) {
            em.OnEnemyLifeEvent(r.link, TeamGun, 1 | 4);
            ++killed;
        }
    }
    mm.Update(ctx);
    CHECK(dark->count() == 34 && dark->state() == Mission::State::Running && ctx.requests.empty());
    // 35th kill -> clear -> GoalAction (6)
    em.CountDefeat(TeamGun, true);
    mm.Update(ctx);
    CHECK(dark->state() == Mission::State::Cleared);
    CHECK(ctx.requests.size() == 1 && ctx.requests[0] == ActionGoal);
    const int slot = mm.DetermineClearedSlot();
    CHECK(slot == 0);
    const int next = table[idx].route[slot].nextIndex;
    std::printf("cleared slot %d -> next stage index %d (stg%04d)\n", slot, next, table[next].id);
    CHECK(table[next].id == 200);
    // the request flows through StageState (0x801762D0 / 0x80174974)
    StageState stage;
    stage.RequestAction(StageAction::Play, true);
    stage.Dispatch();
    stage.locked = true;
    stage.RequestAction((StageAction)ctx.requests[0]);          // locked, not forced -> ignored
    CHECK(!stage.Dispatch() && stage.current() == StageAction::Play);
    stage.locked = false;
    stage.RequestAction((StageAction)ctx.requests[0]);
    CHECK(stage.Dispatch() && stage.current() == StageAction::Goal);
    std::printf("stage action now: %s\n", StageActionName(stage.current()));
    // cleared missions are no longer updated
    mm.Update(ctx);
    CHECK(ctx.requests.size() == 1);

    std::printf(g_fail ? "FAILED (%d)\n" : "ALL PASS\n", g_fail);
    return g_fail ? 1 : 0;
}

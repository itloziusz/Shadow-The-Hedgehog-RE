// stage_sim — runs the reconstructed gameplay modules together on real stage data.
//
//   stage_sim <files-root> [stageId=100]
//
// Wired reconstructed code (each module cites its PPC origin):
//   TaskManager + Task scheduler (19 layers) ........ Task.cpp
//   SET load / range scan / spawn / slot API ......... SetSystem.cpp (+ DOL object catalog)
//   Enemy lifecycle + TEnemySetTask update ........... EnemyFramework.cpp
//   EnemyBaseAI state machine, sensor ................ EnemyAI.cpp
//   GunSoldier AI states ............................. GunSoldierAI.cpp
//   GUN Beetle decision states ....................... GunBeetleAI.cpp
//   Egg Pawn decision states ......................... EggPawnAI.cpp
//   BK Soldier decision states ....................... BkSoldierAI.cpp
//   Stage table, mission descriptors, nukkoro2.inf,
//   MissionManager, EnemyManager census/kill counts .. Mission.cpp (+ DOL stage table)
//   StageState request/dispatch ...................... StageFlow.cpp
//
// HARNESS STUBS (not reconstructed; clearly engine-side or unknown):
//   * The "player" is a scripted attacker that walks to each enemy of the mission team and lands one
//     homing-attack hit every 0.25 s. Damage 2.0 per hit is PROVEN (WEAPONS_AND_TARGETING.md); the
//     movement and the hit timing are scripted.
//   * The spawn viewpoint is the scripted player position (the original uses camera units).
//   * Enemy types without a native implementation get a minimal stub body (EnemyBaseAI default AI,
//     max HP 2.0) so the mission can be exercised; they are counted and reported as stubs.
//   * Motions complete instantly; turning is instant.
#include "shadow/gameplay/EnemyAI.hpp"
#include "shadow/gameplay/BkSoldierAI.hpp"
#include "shadow/gameplay/BkLarva.hpp"
#include "shadow/gameplay/EggPawnAI.hpp"
#include "shadow/gameplay/GunBeetleAI.hpp"
#include "shadow/gameplay/GunSoldierAI.hpp"
#include "shadow/gameplay/Mission.hpp"
#include "shadow/gameplay/SetSystem.hpp"
#include "shadow/gameplay/StageFlow.hpp"
#include "shadow/gameplay/Task.hpp"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

using namespace shadow::gameplay;

namespace {

struct World {
    Vec3 player{};
    EnemyManager* enemies = nullptr;
    SetData* set = nullptr;
    const SetSlot* larvaTargetSlot = nullptr; // scripted attacker activates this generator's engine trigger
    int spawnedNative = 0, spawnedStub = 0, defeated = 0;
    int beetleDuplicateSpawns = 0;
    std::map<const SetSlot*, int> beetleSpawnsBySlot;
    std::map<std::string, int> stubTypes;
};
World g;

// HARNESS HOOK: state-0 stage service (0x80189BC0) and state-1 spatial trigger
// (0x80189C2C..0x80189C60) are supplied by the scene engine in the game.
// The scripted attacker opens only the generator it is currently approaching.
struct SimLarvaEngine : BkLarvaEngine {
    bool StageAllowsGenerator(const SetSlot&) override { return true; }
    bool SpawnTriggerReached(const SetSlot& s) override { return g.larvaTargetSlot == &s; }
    void UpdateLarvaMotion(const SetSlot&, int, float) override {} // HARNESS: no world physics
    void OnGeneratorDestroyed(const SetSlot&) override {}          // HARNESS: no save-state service
};
SimLarvaEngine larvaEngine;

float Dist(const Vec3& a, const Vec3& b) {
    const float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

// ---- component implementations used by the harness ---------------------------------------
struct SimStatus : IEnemyStatus {
    float hp;
    bool reported = false;
    EnemyBaseAI* ai = nullptr;
    SetSlot* slot = nullptr;
    int team = 0;
    explicit SimStatus(float maxHp) : hp(maxHp) {}
    void Init() override {}
    void Start() override {}
    void PreUpdate() override {}
    void Update1(float) override {
        // death edge (EnemyStatusCommon::vf02 0x801A8A38 -> vf23(1) -> EnemyManager 0x801A250C)
        if (hp <= 0.0f && !reported) {
            reported = true;
            g.enemies->OnEnemyLifeEvent(slot->rec->link, team, 1 | 4);
            ++g.defeated;
            // EnemyStatusCommon death edge 0x801A8C40..0x801A8CA0 calls
            // TEnemySetBase::DetachKilled for enemy type != 0. GunSoldier
            // instead enters Hinshi and keeps its attached SET slot.
            if (slot->rec->id != 0x0064) g.set->MarkKilledAndDetach(*slot);
            if (ai) ai->in.dead = true;
        }
    }
    void Update2(float) override {}
    void Update3(float) override {}
    bool IsReady() override { return true; }
    void PrepareReady() override {}
    bool WantsFadeOut() override { return false; }
};
struct SimDisp : IEnemyDisp {
    void Init() override {}
    void Update(float) override {}
    void PostUpdate(float) override {}
};
struct SimMove : IEnemyMove {
    void Init() override {}
    void Update(float) override {}
};

struct SimSoldierBody : GunSoldierBody {
    Vec3 pos;
    int fired = 0;
    Vec3 Position() const override { return pos; }
    Vec3 TargetPosition() const override { return g.player; }
    bool TurnTowards(const Vec3&) override { return true; }
    float YawErrorDeg(const Vec3&) const override { return 0.0f; }
    void MoveTowards(const Vec3& p) override {           // 60 u/s walk (harness)
        const float d = Dist(pos, p);
        if (d < 1.0f) { pos = p; return; }
        const float s = std::fmin(1.0f, 1.0f / d);
        pos = {pos.x + (p.x - pos.x) * s, pos.y + (p.y - pos.y) * s, pos.z + (p.z - pos.z) * s};
    }
    void Stop() override {}
    void StartFireMotion() override { ++fired; }
    bool MotionFlag3A() const override { return true; }
    bool MotionFlag3B() const override { return true; }
    bool Damaged() const override { return false; }
    bool FellOut() const override { return false; }
    void RequestDespawn() override {}
    void PlaySE(int) override {}
    float Random(float lo, float) override { return lo; }
};

struct SimSetBase : EnemySetBase {
    using EnemySetBase::EnemySetBase;
};

// Native GUN soldier: real SET params -> exact sensor (fn_801A6DB8) -> GunSoldierAI.
struct SimGunSoldier : Enemy {
    SimSoldierBody body;
    EnemySensor sensor;
    SimStatus* st;
    SimGunSoldier(Task* layer, SetSlot& s) : Enemy(layer, &s) {
        float p[23] = {};
        int32_t weapon = 0, appear = 0;
        if (SetSlot_GetParams(s, p, sizeof p)) {
            std::memcpy(&weapon, &p[8], 4);
            std::memcpy(&appear, &p[9], 4);
        }
        body.pos = SetSlot_GetPosition(s);
        sensor.Build(body.pos, SetSlot_GetRotationRad(s), p);
        st = new SimStatus(2.0f);                           // MaxHP table 0x804CD678[0]
        status.reset(st);
        disp.reset(new SimDisp);
        move.reset(new SimMove);
        auto* a = new GunSoldierAI(body, sensor, appear, weapon);
        a->home = body.pos;
        a->ConfigurePatrol(s.rec->misc, s.rec->miscLen,
                           s.rec->rotDeg.y * (3.14159265f / 180.0f)); // 0x80189694 uses SET spawn yaw
        ai.reset(a);
        st->ai = a;
        st->slot = &s;
        st->team = TeamGun;
        setBase.reset(new SimSetBase(*this));
    }
};

// Native BK Soldier decision machine. Motion, weapons, and locomotion are
// explicit harness services; its enemy HP and sensor values come from the DOL.
struct SimBkSoldierBody : BkSoldierBody {
    Vec3 pos;
    Vec3 Position() const override { return pos; }
    Vec3 TargetPosition() const override { return g.player; }
    void TurnTowards(const Vec3&) override {}              // HARNESS: instant turn
    float TargetDistance() const override { return Dist(pos, g.player); }
    bool CautionMotionEnded() const override { return true; } // HARNESS: instant motion
    bool MeleeMotionEnded() const override { return true; }   // HARNESS: instant motion
    void BeginStandingWait() override {}
    void BeginMovingWait() override {}
    void UpdateSharedPatrol() override {}                   // HARNESS: no world locomotion
    void BeginCaution() override {}
    void BeginChase() override {}
    void BeginNearAttack() override {}
    void BeginBaseGoHome() override {}
    bool BaseGoHomeReached() const override { return true; } // HARNESS: no world locomotion
    void BeginBkGoHome() override {}
    void BeginAppearHome() override {}
    void BeginRangedAttack(bool) override {}                // HARNESS: no weapon model
    void UpdateRangedAttack(bool) override {}
    void EndRangedAttack(bool) override {}
};

struct SimBkSoldier : Enemy {
    SimBkSoldierBody body;
    EnemySensor sensor;
    SimStatus* st;
    SimBkSoldier(Task* layer, SetSlot& s) : Enemy(layer, &s) {
        float p[24] = {};
        int32_t weapon = 0, appear = 0;
        if (SetSlot_GetParams(s, p, sizeof p)) {
            std::memcpy(&weapon, &p[8], 4);
            std::memcpy(&appear, &p[9], 4);
        }
        body.pos = SetSlot_GetPosition(s);
        sensor.Build(body.pos, SetSlot_GetRotationRad(s), p);
        st = new SimStatus(4.0f);                            // MaxHP table 0x804CD678[5]
        status.reset(st);
        disp.reset(new SimDisp);
        move.reset(new SimMove);
        auto* a = new BkSoldierAI(body, sensor, appear, weapon);
        ai.reset(a);
        st->ai = a;
        st->slot = &s;
        st->team = TeamBlackArms;
        setBase.reset(new SimSetBase(*this));
    }
};

// Native GUN Beetle decisions. The stage harness has no EnemyPath.one geometry,
// mount, animation or flight physics; those remain explicit engine services.
// HARNESS: SimStatus marks every beetle ready; original path-beetle activation
// requires detection at 0x80193CAC and is exposed by GunBeetleAI::ShouldActivate.
struct SimGunBeetleBody : GunBeetleBody {
    Enemy* owner = nullptr;
    Vec3 TargetPosition() const override { return g.player; }
    void AimAt(const Vec3&) override {}                // HARNESS: no aim transform
    void FaceSpawnHeading() override {}                // HARNESS: no turn animation
    void UpdateFloatMotion(float) override {}          // HARNESS: no flight physics
    void EnterFloatingMotion() override {}             // HARNESS: no display model
    void EnterPathMotion() override {}                 // HARNESS: no path model
    float PathDistance() const override { return 0.0f; } // no engine path supplied
    void AdvanceOnPath(float) override {}              // HARNESS: no path geometry
    void StartFiring() override {}                     // HARNESS: no weapon model
    void StopFiring() override {}
    void PlayDeathEffect() override {}                 // HARNESS: no particle model
    void RequestDelete() override { owner->task()->Kill(); }
};

struct SimGunBeetle : Enemy {
    SimGunBeetleBody body;
    EnemySensor sensor;
    SimStatus* st;
    SimGunBeetle(Task* layer, SetSlot& s) : Enemy(layer, &s) {
        float p[18] = {};
        int32_t appear = 0, action = 0, bodyType = 0;
        if (SetSlot_GetParams(s, p, sizeof p)) {
            std::memcpy(&appear, &p[7], 4);
            std::memcpy(&action, &p[8], 4);
            std::memcpy(&bodyType, &p[14], 4);
        }
        body.owner = this;
        sensor.Build(SetSlot_GetPosition(s), SetSlot_GetRotationRad(s), p);
        st = new SimStatus(bodyType == 0 ? 2.0f : 4.0f); // GetMaxHP 0x80194258
        status.reset(st);
        disp.reset(new SimDisp);
        move.reset(new SimMove);
        auto* a = new GunBeetleAI(body, sensor, appear, action);
        a->ConfigureFloat(p[10]);                        // E+0x2DC, 0x80193C84
        // No ConfigurePath: EnemyPath.one geometry is an engine dependency.
        ai.reset(a);
        st->ai = a;
        st->slot = &s;
        st->team = TeamGun;
        setBase.reset(new SimSetBase(*this));
    }
};

// Native Egg Pawn decisions with explicit harness services for walker patrol,
// target acquisition, terrain random-point selection, animation and movement.
struct SimEggPawnBody : EggPawnBody {
    Vec3 pos{}, moveTarget{};
    bool shield = false;
    bool DetectNearestPlayer(const EnemySensor& s) override { return s.Detect(g.player); }
    bool TrackedTargetInSearchArea(const EnemySensor& s) const override { return s.Detect(g.player); }
    Vec3 TrackedTargetPosition() const override { return g.player; }
    float TrackedTargetDistance() const override { return Dist(pos, g.player); }
    void FaceTrackedTarget() override {}                // HARNESS: instant facing
    void BeginStandingWait() override {}                // HARNESS: no walker motion
    void TickStandingWait() override {}
    void BeginMovingWait() override {}
    void TickMovingWait() override {}
    void BeginCaution() override {}
    void BeginReturnHome(bool) override {}
    void SetMoveTarget(const Vec3& p) override { moveTarget = p; }
    bool AdvanceTowardMoveTarget() override { return true; } // HARNESS: no locomotion
    bool MotionEnded() const override { return true; }  // HARNESS: instant motion
    void SetMotion(int, bool) override {}               // HARNESS: no display model
    bool HasShield() const override { return shield; }
    Vec3 AttackPoint(float) override { return g.player; } // HARNESS: no projection engine
    Vec3 PickRandomPointNear(float) override { return pos; } // HARNESS: no terrain/RNG
};

struct SimEggPawn : Enemy {
    SimEggPawnBody body;
    EnemySensor sensor;
    SimStatus* st;
    SimEggPawn(Task* layer, SetSlot& s) : Enemy(layer, &s) {
        float p[27] = {};
        int32_t shield = 0, weapon = 0, appear = 0, waitMove = 0;
        if (SetSlot_GetParams(s, p, sizeof p)) {
            std::memcpy(&shield, &p[7], 4);
            std::memcpy(&weapon, &p[8], 4);
            std::memcpy(&appear, &p[9], 4);
            std::memcpy(&waitMove, &p[13], 4);
        }
        body.pos = SetSlot_GetPosition(s);
        body.shield = shield != 0;
        sensor.Build(body.pos, SetSlot_GetRotationRad(s), p);
        st = new SimStatus(4.0f);                       // MaxHP table 0x804CD678[12]
        status.reset(st);
        disp.reset(new SimDisp);
        move.reset(new SimMove);
        auto* a = new EggPawnAI(body, sensor, appear, weapon, waitMove);
        ai.reset(a);
        st->ai = a;
        st->slot = &s;
        st->team = TeamEggman;
        setBase.reset(new SimSetBase(*this));
    }
};

// Stub for enemy types without a native implementation (harness only).
struct SimStubEnemy : Enemy {
    SimStatus* st;
    SimStubEnemy(Task* layer, SetSlot& s, int team) : Enemy(layer, &s) {
        st = new SimStatus(2.0f);
        status.reset(st);
        disp.reset(new SimDisp);
        move.reset(new SimMove);
        auto* a = new EnemyBaseAI;
        ai.reset(a);
        st->ai = a;
        st->slot = &s;
        st->team = team;
        setBase.reset(new SimSetBase(*this));
    }
};

// Layer "Manager": SET scan (SetManager_Update 0x800CB97C) + mission update (MissionManagerTask::vf01).
struct ManagerTask : Task {
    SetData& set;
    MissionManager& mm;
    MissionContext& ctx;
    StageState& stage;
    ManagerTask(Task* layer, SetData& s, MissionManager& m, MissionContext& c, StageState& st)
        : Task(layer, "SimManager"), set(s), mm(m), ctx(c), stage(st) {}
    void Update(float) override {
        set.Scan(&g.player, 1);
        const size_t before = ctx.requests.size();
        mm.Update(ctx);
        for (size_t i = before; i < ctx.requests.size(); ++i) stage.RequestAction((StageAction)ctx.requests[i]);
        stage.Dispatch();
    }
};

constexpr float kHomingAttackDamage = 2.0f;   // WEAPONS_AND_TARGETING.md (homing attack)

// Layer "Player": scripted attacker (HARNESS STUB).
struct AttackerTask : Task {
    SetData& set;
    int team;
    int targetIdx = -1;
    float hitTimer = 0.0f;
    AttackerTask(Task* layer, SetData& s, int t) : Task(layer, "SimAttacker"), set(s), team(t) {}
    static SimStatus* StatusOf(SetSlot& s) {
        if (!s.object || s.rec->id == 0x0091) return nullptr;
        return static_cast<SimStatus*>(static_cast<Enemy*>(s.object)->status.get());
    }
    void Update(float dt) override {
        // pick the next alive enemy of the mission team
        if (targetIdx >= 0) {
            SetSlot& current = set.slots()[targetIdx];
            if (current.rec->id != 0x0091) {
                SimStatus* st = StatusOf(current);
                if ((st && st->hp <= 0.0f) ||
                    (!st && (current.rec->runtimeFlags & kSetFlag8) &&
                     !(current.rec->runtimeFlags & kSetAlive))) targetIdx = -1;
            }
        }
        if (targetIdx < 0) {
            for (int i = 0; i < set.recordCount(); ++i) {
                SetRecord& r = set.records()[i];
                if (!(r.flags & kSetEnabled) || r.id < 0x64 || r.id >= 0x96) continue;
                if (EnemyManager::TeamOfSetId(r.id) != team) continue;
                SimStatus* st = StatusOf(set.slots()[i]);
                if (st && st->hp <= 0.0f) continue;
                if (visited.count(i)) continue;
                targetIdx = i;
                visited[i] = 1;
                break;
            }
            if (targetIdx < 0) return;
            const Vec3 p = set.records()[targetIdx].pos;
            g.player = {p.x, p.y, p.z + 20.0f};              // walk up to it (teleport, harness)
        }
        SetSlot& target = set.slots()[targetIdx];
        if (target.rec->id == 0x0091) {
            g.larvaTargetSlot = &target;                     // HARNESS trigger, not a recovered world sensor
            auto* generator = static_cast<BkLarvaGenerator*>(target.object);
            if (!generator) return;                          // await SET spawn/engine trigger
            BkLarva* larva = generator->FirstAttackable();
            if (!larva) return;                              // one child spawns per generator update
            hitTimer -= dt;
            if (hitTimer <= 0.0f) {
                larva->ReportDefeat(true);                   // scripted player collision, native life event
                ++g.defeated;
                hitTimer = 0.25f;
                if (generator->defeated() >= generator->requested()) {
                    g.larvaTargetSlot = nullptr;
                    targetIdx = -1;
                }
            }
            return;
        }
        SimStatus* st = StatusOf(target);
        if (!st) {                                           // not spawned yet (next scan spawns it)
            if (++waitFrames > 120) { targetIdx = -1; waitFrames = 0; ++skipped; }   // never spawns: skip
            return;
        }
        waitFrames = 0;
        hitTimer -= dt;
        if (hitTimer <= 0.0f) { st->hp -= kHomingAttackDamage; hitTimer = 0.25f; }
    }
    std::map<int, int> visited;
    int waitFrames = 0;
    int skipped = 0;
};

// Keep task-held references valid during teardown. TaskManager itself was constructed
// before SetData/Mission/EnemyManager and would otherwise delete actors after them.
struct SimTaskCleanup {
    TaskManager& tasks;
    ~SimTaskCleanup() {
        Task::MarkSubtreeKill(tasks.Root());
        while (tasks.Root()->firstChild()) Task::UpdateChildren(tasks.Root(), 0.0f);
    }
};

}  // namespace

int main(int argc, char** argv) {
    const std::string root = argc > 1 ? argv[1] : "../../files";
    const int stageId = argc > 2 ? std::atoi(argv[2]) : 100;
    const int idx = StageTable_IndexOfId(stageId);
    if (idx < 0) { std::printf("unknown stage %d\n", stageId); return 2; }
    const StageTableEntry& st = OriginalStageTable()[idx];
    std::printf("== stage_sim stg%04d \"%s\" (index %d)\n", stageId, st.name, idx);

    TaskManager tm;
    SetData set;
    if (!set.LoadStage(root, stageId, false, idx)) { std::printf("SET load failed\n"); return 2; }
    g.set = &set;
    std::printf("SET records: %d\n", set.recordCount());

    auto descs = DefaultMissionDescs();
    ApplyMissionCountFile(root + "/nukkoro2.inf", descs);
    MissionManager mm(descs);
    mm.InitStage(idx, false);
    EnemyManager em;
    em.ScanSetEnemies(set);
    g.enemies = &em;
    std::printf("enemy census: GUN %d  Eggman %d  BlackArms %d\n", em.placed[0], em.placed[1], em.placed[2]);

    // Choose the mission to play: the first EnemyMission slot (Dark on Westopolis).
    int playSlot = -1, team = TeamGun, required = 0;
    for (int s = 0; s < 3; ++s)
        if (auto* e = dynamic_cast<Mission::EnemyMission*>(mm.GetMission(s))) {
            playSlot = s; team = e->team(); required = e->required(); break;
        }
    static const char* kSlot[3] = {"Dark", "Normal", "Hero"};
    if (playSlot < 0) { std::printf("stage has no EnemyMission; nothing to simulate\n"); return 0; }
    std::printf("playing %s mission: defeat %d of team %d\n", kSlot[playSlot], required, team);

    // Factories: native GunSoldier, GunBeetle, EggPawn, BkSoldier AI, and BkLarvaGenerator;
    // other enemy ids use labelled stubs.
    Task* enemyLayer = tm.GetLayer(TaskManager::Enemy);
    set.RegisterFactory(0x0064, [&](SetSlot& s) -> void* { ++g.spawnedNative; return new SimGunSoldier(enemyLayer, s); });
    set.RegisterFactory(0x0065, [&](SetSlot& s) -> void* {
        ++g.spawnedNative;
        if (++g.beetleSpawnsBySlot[&s] > 1) ++g.beetleDuplicateSpawns;
        return new SimGunBeetle(enemyLayer, s);
    });
    set.RegisterFactory(0x0079, [&](SetSlot& s) -> void* { ++g.spawnedNative; return new SimEggPawn(enemyLayer, s); });
    set.RegisterFactory(0x008D, [&](SetSlot& s) -> void* { ++g.spawnedNative; return new SimBkSoldier(enemyLayer, s); });
    set.RegisterFactory(0x0091, [&](SetSlot& s) -> void* {
        ++g.spawnedNative;
        return new BkLarvaGenerator(enemyLayer, s, em, larvaEngine);
    });
    for (const auto& d : OriginalSetCatalog()) {
        if (d.id <= 0x65 || d.id == 0x0079 || d.id == 0x008D || d.id == 0x0091 || d.id >= 0x96) continue;
        const uint16_t id = d.id;
        const std::string name = d.name ? d.name : "?";
        set.RegisterFactory(id, [&, id, name](SetSlot& s) -> void* {
            ++g.spawnedStub;
            ++g.stubTypes[name];
            return new SimStubEnemy(enemyLayer, s, EnemyManager::TeamOfSetId(id));
        });
    }

    MissionContext ctx;
    ctx.enemies = &em;
    StageState stage;
    SimTaskCleanup taskCleanup{tm};
    stage.RequestAction(StageAction::Play, true);
    stage.Dispatch();
    new ManagerTask(tm.GetLayer(TaskManager::Manager), set, mm, ctx, stage);
    new AttackerTask(tm.GetLayer(TaskManager::Player), set, team);

    const float dt = 1.0f / 60.0f;
    int frame = 0, lastCount = -1;
    auto* mission = dynamic_cast<Mission::CountMission*>(mm.GetMission(playSlot));
    for (; frame < 60 * 60 * 10 && stage.current() != StageAction::Goal; ++frame) {
        tm.Run(dt);
        if (mission && mission->count() != lastCount) {
            lastCount = mission->count();
            if (lastCount % 5 == 0 || lastCount >= required)
                std::printf("  t=%6.2fs  %s mission %d/%d\n", frame * dt, kSlot[playSlot], lastCount, required);
        }
    }
    std::printf("spawned: %d native SET enemies/generators, %d stub enemies", g.spawnedNative, g.spawnedStub);
    for (auto& kv : g.stubTypes) std::printf(" [%s x%d]", kv.first.c_str(), kv.second);
    std::printf("\nGUN Beetle duplicate SET-slot spawns: %d", g.beetleDuplicateSpawns);
    std::printf("\ndefeated: %d   EnemyManager defeated GUN/Egg/BA = %d/%d/%d\n", g.defeated,
                em.defeated[0], em.defeated[1], em.defeated[2]);
    const bool cleared = mission && mission->state() == Mission::State::Cleared;
    std::printf("stage action: %s after %.2fs\n", StageActionName(stage.current()), frame * dt);
    if (!cleared) {
        std::printf("RESULT: mission NOT cleared\n");
        return 1;
    }
    const int slot = mm.DetermineClearedSlot();
    const int next = st.route[slot].nextIndex;
    std::printf("RESULT: %s mission cleared -> next stage index %d (stg%04d)\n", kSlot[slot], next,
                next >= 0 ? OriginalStageTable()[next].id : -1);
    return 0;
}

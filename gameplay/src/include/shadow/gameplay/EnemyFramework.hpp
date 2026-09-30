// Native reconstruction of the standard-enemy framework of Shadow the Hedgehog (GC).
// Evidence: gameplay/GAMEPLAY_UPDATE_PIPELINE.md §6, GAMEPLAY_STRUCTURES.md §4, enemy/GUN_SOLDIER.md.
//
// The original builds each enemy as EnemyTemplate<XBase, XAI> with components joined by
// multiple + virtual inheritance. Natively we keep the same components and the same call order,
// but as plain composition: an Enemy owns one object per component interface.
#pragma once
#include "shadow/gameplay/SetSystem.hpp"
#include "shadow/gameplay/Task.hpp"
#include <memory>

namespace shadow::gameplay {

class Enemy;

// Component interfaces. Method names are recovered semantics; the comment on each gives the
// original vtable byte offset (from the component's vptr) used by the lifecycle code.
struct IEnemyStatus {                  // EnemyStatusCommon (@0x110 in GunSoldier)
    virtual ~IEnemyStatus() = default;
    virtual void Init() = 0;           // +0x08  (state 2)
    virtual void Start() = 0;          // +0x0C  (state 3 -> 4)
    virtual void PreUpdate() = 0;      // +0x10  (state 4, before AI)
    virtual void Update1(float) = 0;   // +0x14  EnemyStatusCommon::vf03 0x801A87D0
    virtual void Update2(float) = 0;   // +0x18  EnemyStatusCommon::vf04 0x801A8768
    virtual void Update3(float) = 0;   // +0x1C  EnemyStatusCommon::vf05 0x801A86CC
    virtual bool IsReady() = 0;        // +0x34  EnemyStatusCommon::vf0B (state 3 gate)
    virtual void PrepareReady() = 0;   // +0x38  (state 3, called before IsReady)
    virtual bool WantsFadeOut() = 0;   // +0x40  EnemyStatusCommon::vf0E (TEnemySetTask fade)
};
struct IEnemyDisp {                    // EnemyDispCommon (@0x1A8, vptr +0xC)
    virtual ~IEnemyDisp() = default;
    virtual void Init() = 0;           // +0x08
    virtual void Update(float) = 0;    // +0x0C  EnemyDispCommon::vf01 0x801A1370
    virtual void PostUpdate(float) = 0;// +0x10
};
struct IEnemyMove {                    // EnemyMoveCommon/Walker/Flyer (@0x1E8)
    virtual ~IEnemyMove() = default;
    virtual void Init() = 0;           // +0x08  EnemyMoveCommon::vf00 0x801A6078
    virtual void Update(float) = 0;    // +0x0C  EnemyMoveCommon::vf01 0x801A5B34
};
struct IEnemyAI {                      // EnemyBaseAI + family AI (@0x360, vptr +0x18)
    virtual ~IEnemyAI() = default;
    virtual void Init() = 0;           // +0x08  EnemyBaseAI::vf00 0x8019EEC4
    virtual void Update(float) = 0;    // +0x0C  EnemyBaseAI::vf01 0x8019EACC
};

// TEnemySetBase — SET binding + lifecycle state machine (@0x250 in GunSoldier).
class EnemySetBase {
public:
    enum State : int { WaitResources = 1, WaitModel = 2, WaitReady = 3, Running = 4 };
    explicit EnemySetBase(Enemy& e) : enemy_(e) {}
    virtual ~EnemySetBase() = default;
    virtual bool ShouldDelete() { return false; }        // +0x08 TEnemySetBase::vf00 0x8017B8EC
    virtual void InitFromSlot(SetSlot* slot);             // +0x20 (e.g. fn_801A7608 path)
    // +0x24 TEnemySetBase::vf07 0x801A7280
    void LifecycleUpdate(float dt);
    int state() const { return state_; }

    // Engine hooks the original resolves through ResourceManager / model loaders:
    virtual bool ResourcesLoaded() { return true; }       // fn_80012408(mgr, res) == 3 && fn_801780AC()
    virtual bool ModelReady() { return true; }            // async handle at +0x0C (fn_80039F04)
private:
    Enemy& enemy_;
    int state_ = WaitResources;                           // +0x08
};

// Enemy = the owning object; TEnemySetTask is its scheduler node.
class Enemy {
public:
    Enemy(Task* layer, SetSlot* slot);
    virtual ~Enemy();
    std::unique_ptr<IEnemyStatus> status;   // +0x04 -> @0x110
    std::unique_ptr<IEnemyDisp>   disp;     // +0x08 -> @0x1A8
    std::unique_ptr<IEnemyMove>   move;     // +0x0C -> @0x1E8
    std::unique_ptr<IEnemyAI>     ai;       // +0x10 -> @0x360
    std::unique_ptr<EnemySetBase> setBase;  // +0x14 -> @0x250
    SetSlot* slot = nullptr;                // TEnemySetTask+0x2C
    float alpha = 0.0f;                     // +0xEC display alpha
    int   liveFrameStamp = 0;               // +0xF8
    static int s_frameCounter;              // bss 0x8057E7F0 (LIKELY)
    virtual float TimeScale(float dt) { return dt; }   // fn_8017C764 (Chaos Control slow-down?) LIKELY
    // fn_800C9D54: fn_80169FC8(recPos, (range*100 + margin)^2) says no viewpoint is near (unless flag 0x800).
    // fn_80169FC8 measures from camera units 0/1 (CameraManager_GetUnitPos) — PROVEN, same as the spawn scan.
    // Needs player positions -> supplied by the native engine layer.
    virtual bool OutOfFadeRange() { return false; }
    Task* task() const { return task_; }
private:
    class SetTask;                          // TEnemySetTask
    Task* task_;
};

}  // namespace shadow::gameplay

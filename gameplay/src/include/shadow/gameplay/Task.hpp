// Native reconstruction of Shadow the Hedgehog's (GC) Task scheduler.
// Behavioural recovery from main.dol — see gameplay/GAMEPLAY_UPDATE_PIPELINE.md.
//
// Original (MWCC) layout, for reference only (native layout differs):
//   +0x00 name  +0x04 u16 flags  +0x08 prev  +0x0C next  +0x10 parent
//   +0x14 firstChild  +0x18 vptr  +0x20 u64 profile ticks
#pragma once
#include <cstdint>

namespace shadow::gameplay {

class Task {
public:
    enum Flags : uint16_t {
        kKillRequest    = 0x0001,  // deferred deletion on next traversal (PROVEN 0x8004ECE8)
        kPendingMask    = 0x000F,  // any of these bits suppresses Update (PROVEN 0x8004ECE0)
        kDestroying     = 0x0020,  // set by ~Task (PROVEN 0x8004EF48)
        kUpdateInPause  = 0x0100,  // runs while the global pause flag is set (PROVEN 0x8004ED6C)
    };

    // Task::Task — 0x8004F014. Appends to the tail of parent's child list.
    explicit Task(Task* parent, const char* name = nullptr);
    // Task::~Task — 0x8004EF18 (vtable slot 0). Unlinks from parent.
    virtual ~Task();
    // vtable slot 1 — default 0x8004EA80 is an empty function.
    virtual void Update(float dt) { (void)dt; }

    void Kill() { flags_ |= kKillRequest; }
    // 0x8004EB04: false if this task or any ancestor has a kill request.
    bool IsAlive() const;
    // 0x8004EBC4
    void SetParent(Task* newParent);

    // 0x8004ECAC — the per-frame traversal (depth first, parent before children,
    // siblings in creation order; killed tasks destroyed here).
    static void UpdateChildren(Task* parent, float dt);
    // 0x8004EECC
    static void MarkSubtreeKill(Task* t);

    // Globals (sbss 0x805EF270..0x805EF27C in the original)
    static int  s_depth;        // 0x805EF270
    static bool s_profile;      // 0x805EF274 (profiling path 0x8004EDF0 when set)
    static int  s_liveCount;    // 0x805EF278
    static bool s_pauseAll;     // 0x805EF27C (u8)

    const char* name() const { return name_; }
    uint16_t flags() const { return flags_; }
    void setFlags(uint16_t f) { flags_ = f; }
    Task* parent() const { return parent_; }
    Task* firstChild() const { return firstChild_; }
    Task* next() const { return next_; }
    uint64_t lastUpdateTicks() const { return profileTicks_; }

private:
    void Unlink();
    void LinkUnder(Task* parent);
    static void UpdateProfiled(Task* t, float dt);  // 0x8004EDF0

    const char* name_;
    uint16_t flags_ = 0;
    Task* prev_ = nullptr;        // first child's prev == last child
    Task* next_ = nullptr;
    Task* parent_ = nullptr;
    Task* firstChild_ = nullptr;
    uint64_t profileTicks_ = 0;
};

// TaskManager — singleton at bss 0x80571C6C in the original (getter 0x8001C4F8).
class TaskManager {
public:
    enum Layer : int {
        System = 0, Audio, Debug, Scene, PostSystem, Render,
        Manager, Controller, Command, CharaColli, Landscape, Gadget, Vehicle,
        Player, Enemy, PostManager, Particle, Editor, Camera, LayerCount
    };
    static TaskManager& Get();              // 0x8001C4F8
    TaskManager();                          // 0x801EB734 (+ 0x801EB53C, 0x801EB4A4)
    ~TaskManager();
    Task* GetLayer(Layer i) const { return layers_[i]; }   // 0x801EB614
    Task* Root() const { return root_; }
    void Run(float dt) { Task::UpdateChildren(root_, dt); } // 0x801EB680
    static const char* LayerName(int i);    // table 0x8053ED20
private:
    Task* root_;
    Task* layers_[LayerCount] = {};
};

}  // namespace shadow::gameplay

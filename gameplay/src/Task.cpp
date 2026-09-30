// Native reconstruction of the Task scheduler. Every function names the PPC
// routine it was recovered from; behaviour follows the disassembly, not guesses.
#include "shadow/gameplay/Task.hpp"
#include <chrono>

namespace shadow::gameplay {

int  Task::s_depth = 0;
bool Task::s_profile = false;
int  Task::s_liveCount = 0;
bool Task::s_pauseAll = false;

static const char kDefaultTaskName[] = "Task";   // *(r13-0x7AC8) in the original

// 0x8004F014 — links as LAST child: head->prev is the tail.
Task::Task(Task* parent, const char* name)
    : name_(name ? name : kDefaultTaskName) {
    LinkUnder(parent);
    ++s_liveCount;   // guarded by fn_80050C40/fn_80050BF8 (interrupt lock) originally
}

void Task::LinkUnder(Task* parent) {
    parent_ = parent;
    next_ = nullptr;
    if (!parent) {
        prev_ = this;
        return;
    }
    Task* first = parent->firstChild_;
    if (first) {
        prev_ = first->prev_;          // old tail
        first->prev_->next_ = this;    // tail.next = this
        first->prev_ = this;           // head.prev = new tail
    } else {
        prev_ = this;
        parent->firstChild_ = this;
    }
}

// Unlink logic shared by ~Task (0x8004EF50..) and SetParent (0x8004EBC4).
void Task::Unlink() {
    Task* p = parent_;
    if (!p) return;
    if (prev_ == this) {                      // only child
        p->firstChild_ = nullptr;
    } else if (p->firstChild_ == this) {      // head
        p->firstChild_ = next_;
        prev_->next_ = nullptr;               // tail.next stays null
        if (next_) next_->prev_ = prev_;      // new head inherits tail pointer
    } else {                                  // middle / tail
        prev_->next_ = next_;
        if (next_) next_->prev_ = prev_;
        else p->firstChild_->prev_ = prev_;   // removed tail: head.prev = new tail
    }
}

// 0x8004EF18. Children are expected to have been destroyed by the scheduler
// (UpdateChildren kills the subtree before deleting a task).
Task::~Task() {
    flags_ |= kDestroying;
    Unlink();
    --s_liveCount;
}

// 0x8004EB04
bool Task::IsAlive() const {
    for (const Task* t = this; t; t = t->parent_)
        if (t->flags_ & kKillRequest) return false;
    return true;
}

// 0x8004EBC4
void Task::SetParent(Task* newParent) {
    Unlink();
    LinkUnder(newParent);
}

// 0x8004EECC
void Task::MarkSubtreeKill(Task* t) {
    for (Task* c = t->firstChild_; c; c = c->next_) {
        c->flags_ |= kKillRequest;
        MarkSubtreeKill(c);
    }
}

// 0x8004EDF0 — same as the plain call but records elapsed ticks at +0x20.
void Task::UpdateProfiled(Task* t, float dt) {
    auto t0 = std::chrono::steady_clock::now();
    t->Update(dt);
    t->profileTicks_ = (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
                           std::chrono::steady_clock::now() - t0).count();
}

// 0x8004ECAC
void Task::UpdateChildren(Task* parent, float dt) {
    ++s_depth;
    Task* t = parent->firstChild_;
    while (parent->firstChild_ && t) {
        uint16_t f = t->flags_;
        if (f & kPendingMask) {
            if ((f & kKillRequest) && !s_pauseAll) {
                t->flags_ = f & ~kKillRequest;
                if (!(t->flags_ & kDestroying)) {
                    Task* victim = t;
                    t = t->next_;
                    while (victim->firstChild_) {        // destroy subtree first
                        MarkSubtreeKill(victim);
                        UpdateChildren(victim, dt);
                    }
                    delete victim;                        // vtable slot 0, r4 = 1
                    continue;
                }
            }
        } else {
            if (!s_pauseAll || (f & kUpdateInPause)) {
                if (s_profile) UpdateProfiled(t, dt);
                else t->Update(dt);                       // vtable slot 1
            }
            UpdateChildren(t, dt);
        }
        t = t->next_;
    }
    --s_depth;
}

// ---------------------------------------------------------------------------
// TaskManager
static const char* const kLayerNames[TaskManager::LayerCount] = {   // 0x8053ED20
    "System", "Audio", "Debug", "Scene", "PostSystem", "Render",
    "Manager", "Controller", "Command", "CharaColli", "Landscape", "Gadget",
    "Vehicle", "Player", "Enemy", "PostManager", "Particle", "Editor", "Camera"};

// 0x8053ED6C (under root) and 0x8053ED84 (under Scene): note Enemy before Player.
static const int kRootLayers[6] = {0, 1, 2, 3, 4, 5};
static const int kSceneLayers[13] = {6, 7, 8, 9, 10, 11, 12, 14, 13, 15, 16, 17, 18};

const char* TaskManager::LayerName(int i) {
    return (i >= 0 && i < LayerCount) ? kLayerNames[i] : "?";
}

TaskManager& TaskManager::Get() {       // 0x8001C4F8 (lazy static)
    static TaskManager s;
    return s;
}

TaskManager::TaskManager() {           // 0x801EB734
    root_ = new Task(nullptr);
    for (int i : kRootLayers) {        // 0x801EB53C
        layers_[i] = new Task(root_, kLayerNames[i]);
        layers_[i]->setFlags(layers_[i]->flags() | Task::kUpdateInPause);
    }
    for (int i : kSceneLayers) {       // 0x801EB4A4 (parent = layer 3 "Scene")
        layers_[i] = new Task(layers_[Scene], kLayerNames[i]);
        layers_[i]->setFlags(layers_[i]->flags() | Task::kUpdateInPause);
    }
}

TaskManager::~TaskManager() {
    // Tear down in scheduler order: kill everything, then flush.
    Task::MarkSubtreeKill(root_);
    while (root_->firstChild()) Task::UpdateChildren(root_, 0.0f);
    delete root_;
}

}  // namespace shadow::gameplay

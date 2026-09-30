// Native reconstruction of TEnemySetTask::Update (0x801A6EB8) and
// TEnemySetBase::LifecycleUpdate (0x801A7280). Call order follows the disassembly.
#include "shadow/gameplay/EnemyFramework.hpp"
#include <algorithm>

namespace shadow::gameplay {

int Enemy::s_frameCounter = 0;

// TEnemySetTask (Task subobject @0x3E0 in GunSoldier; vtable slot 1 = Update)
class Enemy::SetTask : public Task {
public:
    SetTask(Task* layer, Enemy& e) : Task(layer, "TEnemySetTask"), e_(e) {}
    ~SetTask() override { delete &e_; }   // task owns the enemy (deleting dtor path)
    void Update(float dt) override;
private:
    Enemy& e_;
};

Enemy::Enemy(Task* layer, SetSlot* s) : slot(s) {
    task_ = new SetTask(layer, *this);
    if (slot) SetSlot_Attach(*slot, this);   // done by the concrete ctor/init path in the original
}

Enemy::~Enemy() {
    // TEnemySetTask::~TEnemySetTask 0x801A705C..0x801A7098 detaches its slot
    // unless TEnemySetBase already did so. Native `slot->object == this` is the
    // equivalent attached check; SetSlot_Detach 0x800C9FD0 clears bits 0x2/0x4.
    if (slot && slot->object == this) {
        slot->rec->runtimeFlags &= ~(kSetAlive | kSetDespawnRequest);
        slot->object = nullptr;
    }
}

// 0x801A6EB8
void Enemy::SetTask::Update(float dt) {
    Enemy& e = e_;
    const float sdt = e.TimeScale(dt);                         // fn_8017C764
    if (e.setBase->ShouldDelete()) { Kill(); return; }         // Set +0x08
    float a = e.alpha;                                         // +0xEC
    constexpr float kFadeRate = 2.0f;                          // sdata2 0x805F68D8 (0 @0x805F68DC, 1 @0x805F68E0)
    if (e.slot) {                                              // no slot (0x801A6F34): alpha unchanged
        if (SetSlot_ShouldDespawn(*e.slot, false)) { Kill(); return; }   // fn_800C9E4C
        if (e.OutOfFadeRange()) {                              // fn_800C9D54
            a -= kFadeRate * sdt;
            if (a < 0.0f) { Kill(); return; }                  // 0x801A6F70..0x801A6F84
        } else if (e.status->WantsFadeOut()) {                 // Status +0x40
            a = std::max(0.0f, a - kFadeRate * sdt);
        } else {
            a = std::min(1.0f, a + kFadeRate * sdt);
        }
    }
    e.alpha = a;                                               // fn_8019AE90
    e.setBase->LifecycleUpdate(sdt);                           // Set +0x24
}

void EnemySetBase::InitFromSlot(SetSlot*) {}

// 0x801A7280
void EnemySetBase::LifecycleUpdate(float dt) {
    Enemy& e = enemy_;
    switch (state_) {
    case WaitResources:
        if (!ResourcesLoaded()) return;
        // original: slot = vcall +0x2C; builds model request from slot pos (fn_800CA2FC ...)
        state_ = WaitModel;
        [[fallthrough]];
    case WaitModel:
        if (!ModelReady()) return;
        InitFromSlot(e.slot);                 // Set +0x20
        e.disp->Init();                       // Disp +0x08
        e.status->Init();                     // Status +0x08
        e.ai->Init();                         // AI +0x08
        e.move->Init();                       // Move +0x08
        state_ = WaitReady;
        [[fallthrough]];
    case WaitReady:
        e.status->PrepareReady();             // Status +0x38
        if (!e.status->IsReady()) return;     // Status +0x34
        e.liveFrameStamp = Enemy::s_frameCounter;   // enemy+0xF8 = [0x8057E7F0]
        e.status->Start();                    // Status +0x0C
        state_ = Running;
        return;                               // the original returns after state 3 work
    case Running:
        e.status->PreUpdate();                // Status +0x10
        e.status->Update1(dt);                // Status +0x14
        e.ai->Update(dt);                     // AI +0x0C
        e.status->Update2(dt);                // Status +0x18
        e.disp->Update(dt);                   // Disp +0x0C
        e.move->Update(dt);                   // Move +0x0C
        e.disp->PostUpdate(dt);               // Disp +0x10
        e.status->Update3(dt);                // Status +0x1C
        return;
    default:
        return;                               // >= 5: idle
    }
}

}  // namespace shadow::gameplay

// Native reconstruction of the player behavior state machine (Shadow the Hedgehog GC).
// Evidence: PLAYER_GAMEPLAY_RECOVERY.md §4 (vtable slots, fn_800A82FC, factory table 0x80522DE8).
#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include "shadow/gameplay/SetSystem.hpp"

namespace shadow::gameplay {

// Behavior ids = index into factory table 0x80522DE8 (class names are the original RTTI names).
enum class BehaviorId : int32_t {
    Idle = 0x00, Ground, Fall, Landing, Ottotto, Jump, JumpDash, Sliding, HomingAttack, HomingJump,
    Throw, TurnOver, EdgeHang, WallHang, LineHang, SomethingHang, WallJump, Grind, SpinDash, DarkSpin,
    LightDash, PickupObject, PickupWeapon, Launch, Stream, GroundAttack, AirAttack, DriveVehicle,
    ChaosBlast, ChaosControl, Damage, Dead, ExternalControl, ElectricCircuit, Super = 0x22,
    None = 0x25,   // "previous id" reported when there is no current behavior (fn_800A82FC)
};

// Player::Behavior::InitializeArgs (+4 = behavior id); subclasses carry per-behavior payload.
struct InitializeArgs {
    virtual ~InitializeArgs() = default;
    BehaviorId id;
    explicit InitializeArgs(BehaviorId i) : id(i) {}
};

struct PlayerCommandView { uint16_t type = 0; };   // CharCommand +4 type (GAMEPLAY_COMMANDS.md)

// Player::Behavior::Behavior vtable: 0 dtor, 1 CanEnter, 2 Enter, 3 Leave, 4 OnCommand, 5 Update, 6 UNKNOWN
class Behavior {
public:
    explicit Behavior(BehaviorId id) : id_(id) {}
    virtual ~Behavior() = default;
    virtual bool CanEnter(BehaviorId /*prev*/, const InitializeArgs&) { return true; }   // slot 1
    virtual void Enter(BehaviorId /*prev*/, const InitializeArgs&) {}                    // slot 2
    virtual void Leave(BehaviorId /*next*/) {}                                           // slot 3
    virtual bool OnCommand(const PlayerCommandView&) { return false; }                   // slot 4 (default 0x80072548)
    virtual void Update(float /*dt*/) {}                                                 // slot 5
    BehaviorId id() const { return id_; }
private:
    BehaviorId id_;
};

// Player::Behavior::Factory (Shadow: Player::Shadow::ShadowBehavior::vf01 0x800AE8D0)
class BehaviorFactory {
public:
    using Creator = std::function<std::unique_ptr<Behavior>()>;
    void Register(BehaviorId id, Creator c) { creators_[(int)id] = std::move(c); }
    // With state flag 0x25 (Super) ids 0/2/3 (Idle/Fall/Landing) are remapped to 0x22 (Super).
    std::unique_ptr<Behavior> Create(BehaviorId id, bool superFlag) const;
private:
    Creator creators_[0x23];
};

// Player::Executer (player+0x22C) — the part of Executer_Update / ChangeBehavior that is behavior-generic.
class PlayerExecuter {
public:
    explicit PlayerExecuter(const BehaviorFactory& f) : factory_(f) {}
    // fn_800A82FC: returns true if the new behavior was entered.
    bool ChangeBehavior(const InitializeArgs& args);
    // Step 5 of Executer_Update fn_800A8160: current behavior Update.
    void UpdateBehavior(float dt) { if (cur_) cur_->Update(dt); }
    // ShadowExecuter OnCommand order step 3: current behavior OnCommand.
    bool DispatchCommand(const PlayerCommandView& c) { return cur_ && cur_->OnCommand(c); }
    Behavior* current() const { return cur_.get(); }
    bool superFlag = false;            // state flag 0x25
    float talkTimer = 0.0f;            // executer+0x48, reset to 10.0 on every change attempt
private:
    const BehaviorFactory& factory_;
    std::unique_ptr<Behavior> cur_;    // executer+0x30
};

// Recovered, bounded portions of four original behavior Update methods. These
// receive results from engine services (terrain/ledge/target/motion) explicitly;
// callers must not infer those results from these helpers.

// Ground::Update 0x80083B7C..0x80083DC0. The caller runs the original
// locomotion and terrain services before supplying the resulting flags here.
struct GroundExitSignals {
    bool grounded = false;          // Player state flag 2, read at 0x80083C04
    bool ledgeFound = false;        // fn_800971EC, only queried while ungrounded
    bool parallelMove = false;      // Player state flag 0x3C
    Vec3 groundNormal{};            // player+0x1C8
    Vec3 up{};                      // player+0xF4
};
struct GroundExitResult {
    std::optional<BehaviorId> transition;
    bool fastActionEnabled = false;  // action bit 0: speed > 250
    std::optional<bool> slideActionEnabled; // action bit 4 changes only while grounded
    bool highSpeedFlag = false;      // state bit 0x3B: speed > 150 and !parallel
};
GroundExitResult StepGroundExit(Vec3& velocity, const GroundExitSignals& signals);

// Fall::Update 0x80080B88..0x80080C48. sensorHit is the engine ground
// sensor's +0x24 and bit 0 result; sensorDistance is its +0x20.
struct FallLandingSignals {
    bool sensorHit = false;
    Vec3 sensorNormal{};
    float sensorDistance = 0.0f;
    float landingDistanceLimit = 0.0f; // Player query 0x8009354C, supplied by engine
    bool grounded = false;            // player state flag 2
};
std::optional<BehaviorId> StepFallLanding(const Vec3& velocity, const FallLandingSignals& signals);

// Jump::Update 0x80087680..0x8008770C. Landing is tested before the
// frame-count decrement; the counter is then clamped to zero.
bool StepJumpLandingGrace(int& remainingFrames, bool grounded);

// HomingAttack::Update 0x80085648..0x80085800. `targetUsable` is the result
// of the original target handle/bit query at 0x8008572C..0x80085760.
struct HomingStep {
    std::optional<BehaviorId> transition;
    int homingJumpVariant = -1; // 0 from state 2, 1 from state 3
};
HomingStep StepHomingAttack(int state, bool targetUsable, float& elapsedSeconds,
                            float& steerRate, float dt);

}  // namespace shadow::gameplay

// Native reconstruction of Executer_ChangeBehavior (fn_800A82FC) and the behavior factory.
#include "shadow/gameplay/PlayerBehavior.hpp"

namespace shadow::gameplay {

std::unique_ptr<Behavior> BehaviorFactory::Create(BehaviorId id, bool superFlag) const {
    if (superFlag && (id == BehaviorId::Idle || id == BehaviorId::Fall || id == BehaviorId::Landing))
        id = BehaviorId::Super;                                   // 0x800AE8D0 remap
    const int i = (int)id;
    if (i < 0 || i >= 0x23 || !creators_[i]) return nullptr;    // table 0x80522DE8[id] creator
    return creators_[i]();
}

// fn_800A82FC
bool PlayerExecuter::ChangeBehavior(const InitializeArgs& args) {
    const BehaviorId prev = cur_ ? cur_->id() : BehaviorId::None;          // 0x25 when none
    std::unique_ptr<Behavior> nb = factory_.Create(args.id, superFlag);    // factory vslot 1 @0x800A8344
    talkTimer = 10.0f;                                                     // @0x800A8364
    if (nb && nb->CanEnter(prev, args)) {                                  // vslot 1 @0x800A8388
        if (cur_) cur_->Leave(nb->id());                                   // vslot 3 @0x800A83B4
        cur_ = std::move(nb);                                              // fn_800A8420 @0x800A83C0
        cur_->Enter(prev, args);                                           // vslot 2 @0x800A83E4
        return true;
    }
    return false;                                                          // rejected candidate discarded
}

// Ground::Update 0x80083B7C..0x80083DC0. The comparisons are against
// squared speed, exactly as in the original; the slope test follows the
// action-bit updates and only runs while grounded and below 250 u/s.
GroundExitResult StepGroundExit(Vec3& velocity, const GroundExitSignals& s) {
    const float speedSq = velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z;
    GroundExitResult r;
    r.fastActionEnabled = speedSq > 62500.0f;             // 0x805F3790 / 0x80083B8C
    r.highSpeedFlag = !s.parallelMove && speedSq > 22500.0f; // 0x805F3758 / 0x80083BD4
    if (!s.grounded) {                                     // 0x80083C04..0x80083CAC
        if (s.ledgeFound) {
            velocity = {0, 0, 0};                         // 0x80083C40..0x80083C44
            r.transition = BehaviorId::Ottotto;
        } else {
            r.transition = BehaviorId::Fall;
        }
        return r;
    }
    r.slideActionEnabled = speedSq > 40000.0f;            // 200 u/s, 0x80083CB0..0x80083CF4
    const float normalDotUp = s.groundNormal.x * s.up.x +
                              s.groundNormal.y * s.up.y + s.groundNormal.z * s.up.z;
    if (!r.fastActionEnabled && normalDotUp < 0.4226182699203491f) { // 0x805F3794
        velocity = {s.groundNormal.x * 100.0f, s.groundNormal.y * 100.0f,
                    s.groundNormal.z * 100.0f};          // 0x805F3778 / 0x80083D28..0x80083D48
        r.transition = BehaviorId::Fall;
    } else if (speedSq < 0.2f * 0.2f) {                   // 0.2 u/s, 0x805EF4B0
        r.transition = BehaviorId::Idle;
    }
    return r;
}

// Fall::Update 0x80080B88..0x80080C48. The ground sensor is an explicit
// engine input; only its observed gate and final state flag are recovered.
std::optional<BehaviorId> StepFallLanding(const Vec3& velocity, const FallLandingSignals& s) {
    const float approach = velocity.x * s.sensorNormal.x + velocity.y * s.sensorNormal.y +
                           velocity.z * s.sensorNormal.z;
    if (s.sensorHit && approach < 0.0f && s.sensorDistance < s.landingDistanceLimit)
        return BehaviorId::Landing;
    if (s.grounded) return BehaviorId::Landing;
    return std::nullopt;
}

// Jump::Update 0x80087680..0x8008770C. The original checks grounded before
// decrementing and clamps its remaining frame counter at zero.
bool StepJumpLandingGrace(int& remainingFrames, bool grounded) {
    if (grounded && remainingFrames == 0) return true;
    if (remainingFrames > 0) --remainingFrames;
    else remainingFrames = 0;
    return false;
}

// HomingAttack::Update 0x80085648..0x80085800. Target validity is obtained
// from an engine hook by the caller; steering geometry and integration follow
// this slice and are not represented here.
HomingStep StepHomingAttack(int state, bool targetUsable, float& elapsedSeconds,
                            float& steerRate, float dt) {
    if (state == 1) return {BehaviorId::Fall, -1};
    if (state == 2) return {BehaviorId::HomingJump, 0};
    if (state == 3) return {BehaviorId::HomingJump, 1};
    steerRate += 10.0f * dt;                               // +0x28 @0x80085718..0x80085728
    if (!targetUsable) return {BehaviorId::Fall, -1};      // @0x8008572C..0x80085798
    elapsedSeconds += dt;                                 // +0x2C @0x800857A0..0x800857AC
    if (elapsedSeconds > 10.0f) return {BehaviorId::Fall, -1}; // 0x805F37BC
    return {};
}

}  // namespace shadow::gameplay

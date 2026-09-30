#pragma once
#include "crt_semantics.h"
#include <optional>

// Once-entry latch, not a display configuration and not an initialization-
// complete flag. Guest addresses below are producer/validation metadata only.
class NativeDisplayInitGuard {
public:
    struct Decision { GuestWord32 read_value; bool enter_initializer; };
    static constexpr bool NeedsInitialization(GuestWord32 value) { return value == 0; }

    static NativeDisplayInitGuard FromValidatedCrt(const BootImage& image, const CrtSemantics& crt) {
        if (crt.pc != 0x80003170u || crt.zero_ranges != 3 || crt.identity_copies != 10 ||
            crt.copies_executed || crt.guest_image_zeroed)
            throw BootError("display guard has no validated CRT producer");
        const auto fingerprint = [&](GuestAddress32 first, GuestAddress32 last) {
            GuestWord32 h = 2166136261u;
            for (auto pc = first; pc <= last; pc += 4u) h = (h ^ image.ReadWord(pc)) * 16777619u;
            return h;
        };
        if (fingerprint(0x80003340u,0x800033FCu) != 0x556BA682u ||
            fingerprint(0x8000540Cu,0x800054F0u) != 0x4EADA6C8u)
            throw BootError("display guard CRT producer code changed");
        // Derive the byte-fill value from original instructions only AFTER
        // verifying the producer, range and original consumer semantics.
        if (image.ReadWord(0x800055D0u) != 0x805EF020u || image.ReadWord(0x800055D4u) != 0x375Cu ||
            image.ReadWord(0x800033D4u) != 0x38800000u ||
            image.ReadWord(0x8037E850u) != 0x800D5BC8u || image.ReadWord(0x8037E854u) != 0x2C000000u ||
            image.ReadWord(0x8037E858u) != 0x4082045Cu)
            throw BootError("display guard producer/consumer contract changed");
        NativeDisplayInitGuard result;
        result.value_ = (image.ReadWord(0x800033D4u) & 0xFFu) * 0x01010101u;
        return result;
    }

    Decision ConsumeInitialDecision() {
        if (!value_ || consumed_) throw BootError("unproduced or stale display guard decision");
        consumed_ = true;
        return {*value_, NeedsInitialization(*value_)};
    }
    // Deliberately no SetInitialized method: the original store of one occurs
    // AFTER the next unproven input and call. It cannot be moved before them.
private:
    std::optional<GuestWord32> value_;
    bool consumed_ = false;
};

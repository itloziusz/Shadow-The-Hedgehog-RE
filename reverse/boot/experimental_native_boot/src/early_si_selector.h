#pragma once
#include "crt_semantics.h"
#include <array>
#include <optional>

// Only the first, CRT-produced generation is proven. This is NOT a persistent
// substitute for display configuration after its later update paths run.
class EarlySiSelector {
public:
    struct Selection {
        GuestWord32 selector;
        GuestWord32 family;
        GuestAddress32 table_entry;   // validation metadata, never a host pointer
        GuestAddress32 original_target;
    };
    static EarlySiSelector FromValidatedCrt(const BootImage& image, const CrtSemantics& crt) {
        if (crt.pc!=0x80003170u || crt.zero_ranges!=3 || crt.identity_copies!=10 || crt.copies_executed || crt.guest_image_zeroed)
            throw BootError("early selector requires the validated CRT boundary");
        const auto hash=[&](GuestAddress32 first,GuestAddress32 last) {
            GuestWord32 value=2166136261u;
            for(auto pc=first;pc<=last;pc+=4u) value=(value^image.ReadWord(pc))*16777619u;
            return value;
        };
        if (hash(0x80003340u,0x800033FCu)!=0x556BA682u || hash(0x8000540Cu,0x800054F0u)!=0x4EADA6C8u ||
            hash(0x80380138u,0x8038019Cu)!=0xB0F5AE91u || image.ReadWord(0x800033D4u)!=0x38800000u ||
            image.ReadWord(0x800055D0u)!=0x805EF020u || image.ReadWord(0x800055D4u)!=0x375Cu)
            throw BootError("early selector producer/consumer changed");
        EarlySiSelector result;
        result.value_=(image.ReadWord(0x800033D4u)&0xffu)*0x01010101u;
        constexpr std::array<GuestAddress32,8> targets{
            0x80380170u,0x80380178u,0x80380180u,0x80380170u,
            0x80380178u,0x80380180u,0x80380170u,0x80380170u};
        for(std::size_t i=0;i<targets.size();++i) {
            const auto target=image.ReadWord(0x805631ACu+static_cast<GuestWord32>(4*i));
            if(target!=targets[i]) throw BootError("selector mapping target changed");
            result.targets_[i]=target;
            // Normalize the proven li r31,constant / mr r31,r5 cases to data,
            // not a guest-function dispatcher or a captured expected output.
            const auto word=image.ReadWord(target);
            if(word==0x7CBF2B78u) result.families_[i]=static_cast<GuestWord32>(i);
            else if((word&0xffff0000u)==0x3BE00000u) result.families_[i]=word&0xffffu;
            else throw BootError("unproven selector mapping behavior");
        }
        return result;
    }
    GuestWord32 MapSelector(GuestWord32 selector) const {
        if(!value_ || selector>=families_.size()) throw BootError("unproven selector mapping input");
        return families_[selector];
    }
    Selection ConsumeInitialFamily() {
        if(!value_ || consumed_) throw BootError("unproduced/stale early selector");
        const auto selector=*value_;const auto family=MapSelector(selector);
        consumed_=true;
        return {selector,family,0x805631ACu+4u*selector,targets_[selector]};
    }
private:
    std::optional<GuestWord32> value_;
    bool consumed_=false;
    std::array<GuestWord32,8> families_{};
    std::array<GuestAddress32,8> targets_{};
};

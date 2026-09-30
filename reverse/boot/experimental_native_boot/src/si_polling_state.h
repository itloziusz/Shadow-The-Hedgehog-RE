#pragma once
#include "si_sampling_row.h"
#include <array>

struct PollingEffect { bool write; std::uint32_t value; };
struct PollingUpdate {
    std::uint32_t value;
    std::array<PollingEffect,5> effects;
};
// Pure software operation. No publication to a device or callback is implied.
inline PollingUpdate UpdatePollingFields(std::uint32_t previous,SiSamplingFields fields,SamplingClockBand band) {
    const auto retained=previous&0xFC0000FFu;
    const auto next=retained|SiSamplingContribution(fields,band);
    return {next,{{{false,previous},{true,retained},{false,retained},{true,next},{false,next}}}};
}
class InitialSiPollingState {
public:
    void ResetForSampling(const BootImage& image) {
        if(word_)throw BootError("duplicate SI polling reset");
        image.RequirePalFixtureDigest();
        if(image.ReadWord(0x803B6370u)!=0x38000000u || image.ReadWord(0x803B6380u)!=0x90040004u)
            throw BootError("SI polling reset producer changed");
        // Original explicit li r0,0 / stw, not assumed DOL-zero survival.
        word_=0;
    }
    PollingUpdate UpdateInitial(InitialSiSamplingRow& row,StartupSamplingClock& clock) {
        if(!word_ || updated_)throw BootError("missing/stale initial SI polling producer");
        const auto fields=row.Consume();const auto band=clock.ConsumeInitialBand();
        const auto result=UpdatePollingFields(*word_,fields,band);
        word_=result.value;updated_=true;return result;
    }
private:
    std::optional<std::uint32_t> word_;
    bool updated_=false;
};

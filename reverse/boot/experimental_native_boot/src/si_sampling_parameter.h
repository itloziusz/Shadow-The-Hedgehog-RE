#pragma once
#include "early_si_selector.h"
#include "si_sampling_clock.h"

// An immutable initial SI sampling-table parameter, not console RAM/MMIO.
// Only the first family0/index0 lifetime has been proven.
class InitialSiSamplingParameter {
public:
    static InitialSiSamplingParameter FromPalData(const std::vector<std::uint8_t>& dol,
        const EarlySiSelector::Selection& selection, std::uint32_t index);
    std::uint16_t ConsumeBaseInterval() {
        if(!base_ || consumed_) throw BootError("missing/stale initial sampling parameter");
        consumed_=true;return *base_;
    }
private:
    std::optional<std::uint16_t> base_;
    bool consumed_=false;
};

// Pure arithmetic equivalence to original mullw at803B71F0. Calling this in a
// proof does NOT execute that later startup instruction: the intervening byte
// input at803B71EC must be proven first. No polling-control state is written.
inline std::uint32_t ScaleSiSamplingBase(std::uint16_t base,SamplingClockBand band) {
    return std::uint32_t{base}*SamplingMultiplierForBand(band);
}

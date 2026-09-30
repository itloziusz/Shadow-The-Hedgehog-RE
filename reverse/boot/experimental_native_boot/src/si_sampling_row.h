#pragma once
#include "si_sampling_parameter.h"

// First family0/index0 lifetime only. Guest addresses remain provenance metadata.
// Y is the unsigned polling field placed in bits8..15, not a simulated register.
struct SiSamplingFields { std::uint16_t base_x; std::uint8_t polling_y; };
class InitialSiSamplingRow {
public:
    static InitialSiSamplingRow FromPalData(const std::vector<std::uint8_t>& dol,
        const EarlySiSelector::Selection& selection,std::uint32_t index);
    SiSamplingFields Consume() {
        if(!fields_ || consumed_)throw BootError("missing/stale sampling-row provenance");
        consumed_=true;return *fields_;
    }
private:
    std::optional<SiSamplingFields> fields_;
    bool consumed_=false;
};
inline std::uint32_t SiSamplingContribution(SiSamplingFields fields,SamplingClockBand band) {
    return (ScaleSiSamplingBase(fields.base_x,band)<<16)|(std::uint32_t{fields.polling_y}<<8);
}
// This contribution is NOT the final polling word: the helper preserves fields
// from its previous software polling state, whose incoming lifetime is unproved.

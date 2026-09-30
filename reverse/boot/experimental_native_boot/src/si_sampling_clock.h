#pragma once
#include "boot_image.h"
#include <optional>
#include <span>

// Native startup configuration, not a VI/MMIO device. Only the original PAL
// boot-metadata producer and its first SI consumer are currently validated.
enum class SamplingClockBand : std::uint32_t { MHz27=27000000u, MHz54=54000000u };

inline std::uint32_t SamplingMultiplierForBand(SamplingClockBand band) {
    switch(band) {
    case SamplingClockBand::MHz27:
    case SamplingClockBand::MHz54:
        return static_cast<std::uint32_t>(band)/27000000u;
    }
    throw BootError("unproven sampling clock band; no fallback");
}

class StartupSamplingClock {
public:
    static StartupSamplingClock FromPalBootMetadata(std::span<const std::uint8_t> bi2);
    SamplingClockBand ConsumeInitialBand() {
        if(!band_ || consumed_) throw BootError("missing/stale startup sampling clock");
        consumed_=true;
        return *band_;
    }
private:
    std::optional<SamplingClockBand> band_;
    bool consumed_=false;
};

#include "si_sampling_clock.h"
#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>

StartupSamplingClock StartupSamplingClock::FromPalBootMetadata(std::span<const std::uint8_t> bi2) {
    if(bi2.size()!=8192u) throw BootError("missing/incomplete original BI2 metadata");
    // Immutable input asset identity, never a captured register value.
    constexpr std::array<std::uint8_t,32> digest_expected{
        0x8b,0xba,0xdf,0xfc,0x2a,0x7f,0x79,0xed,0xa5,0x98,0x2a,0xa2,0xfe,0x30,0x83,0x19,
        0x69,0xc2,0x70,0x2a,0xa9,0x20,0x6f,0x62,0x1b,0x93,0x0a,0xae,0x0b,0x70,0xaf,0x6b};
    std::array<std::uint8_t,32> digest{};
    const auto status=BCryptHash(BCRYPT_SHA256_ALG_HANDLE,nullptr,0,
        const_cast<PUCHAR>(bi2.data()),static_cast<ULONG>(bi2.size()),
        digest.data(),static_cast<ULONG>(digest.size()));
    if(status<0 || digest!=digest_expected) throw BootError("unbound PAL boot metadata");
    const auto region=(std::uint32_t{bi2[24]}<<24)|(std::uint32_t{bi2[25]}<<16)|
                      (std::uint32_t{bi2[26]}<<8)|std::uint32_t{bi2[27]};
    if(region!=2u) throw BootError("unproven startup region; no default/fallback");
    StartupSamplingClock result;
    // Boot producer semantics: PAL selects the 27 MHz sampling domain. The
    // later display initializer and host elapsed time are not inputs here.
    result.band_=SamplingClockBand::MHz27;
    return result;
}

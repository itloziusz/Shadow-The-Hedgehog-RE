#include "si_sampling_parameter.h"

InitialSiSamplingParameter InitialSiSamplingParameter::FromPalData(
    const std::vector<std::uint8_t>& dol,const EarlySiSelector::Selection& selection,
    std::uint32_t index) {
    auto image=LoadValidatedFixture(dol);image.RequirePalFixtureDigest();
    if(selection.selector!=0 || selection.family!=0 || selection.table_entry!=0x805631ACu ||
       selection.original_target!=0x80380170u || index!=0)
        throw BootError("unproven sampling family/index lifetime");
    constexpr GuestAddress32 address=0x80569A60u; // source metadata, never a host pointer
    std::optional<std::size_t> offset;
    for(const auto& section:NativeBootManifest::sections) {
        if(address>=section.address && std::uint64_t{address}+2<=std::uint64_t{section.address}+section.size) {
            if(offset) throw BootError("ambiguous sampling data producer");
            offset=static_cast<std::size_t>(section.file_offset)+address-section.address;
        }
    }
    if(!offset || *offset+2>dol.size()) throw BootError("missing sampling data producer");
    InitialSiSamplingParameter result;
    result.base_=static_cast<std::uint16_t>((std::uint32_t{dol[*offset]}<<8)|dol[*offset+1]);
    return result;
}

#include "si_sampling_row.h"
InitialSiSamplingRow InitialSiSamplingRow::FromPalData(const std::vector<std::uint8_t>& dol,
    const EarlySiSelector::Selection& selection,std::uint32_t index) {
    // Reuse the proven selector, index, fixture identity and halfword contract.
    auto base=InitialSiSamplingParameter::FromPalData(dol,selection,index);
    constexpr GuestAddress32 field=0x80569A62u;
    std::optional<std::size_t> offset;
    for(const auto& section:NativeBootManifest::sections) {
        if(field>=section.address && std::uint64_t{field}+1<=std::uint64_t{section.address}+section.size) {
            if(offset)throw BootError("ambiguous sampling byte producer");
            offset=static_cast<std::size_t>(section.file_offset)+field-section.address;
        }
    }
    if(!offset || *offset>=dol.size())throw BootError("missing sampling byte producer");
    InitialSiSamplingRow row;
    row.fields_=SiSamplingFields{base.ConsumeBaseInterval(),dol[*offset]};
    return row;
}

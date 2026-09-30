#include "hardware_frontier.h"

#include <array>
#include <iostream>

namespace {

void Pin(const BootImage& image, GuestAddress32 address, GuestWord32 expected) {
    if (image.ReadWord(address) != expected) {
        throw BootError("hardware-frontier fingerprint mismatch");
    }
}

struct CopyEntry {
    GuestWord32 source;
    GuestWord32 destination;
    GuestWord32 size;
};

struct ZeroEntry {
    GuestWord32 address;
    GuestWord32 size;
};

constexpr std::array<CopyEntry, 10> kCopyEntries{{
    {0x80003100u, 0x80003100u, 0x000024E8u}, {0x80005600u, 0x80005600u, 0x00001F08u},
    {0x80007520u, 0x80007520u, 0x00001814u}, {0x80008D40u, 0x80008D40u, 0x004A1F08u},
    {0x804AAC60u, 0x804AAC60u, 0x0000046Cu}, {0x804AB0E0u, 0x804AB0E0u, 0x0000000Cu},
    {0x804AB100u, 0x804AB100u, 0x00072418u}, {0x8051D520u, 0x8051D520u, 0x000528C8u},
    {0x805E4500u, 0x805E4500u, 0x0000AB20u}, {0x805F2780u, 0x805F2780u, 0x00009DB8u},
}};

constexpr std::array<ZeroEntry, 3> kZeroEntries{{
    {0x8056FE00u, 0x00074700u},
    {0x805EF020u, 0x0000375Cu},
    {0x805FC540u, 0x000000ACu},
}};

bool Contains(GuestAddress32 base, GuestWord32 size, GuestAddress32 address) {
    const HostByteCount64 begin = base;
    const HostByteCount64 end = begin + size;
    return HostByteCount64{address} >= begin && HostByteCount64{address} < end;
}

void PinHardwareWords(const BootImage& image) {
    // __init_hardware. Not executed.
    const InstructionPin helper[] = {
        {0x80003400u, 0x7C0000A6u}, {0x80003404u, 0x60002000u}, {0x80003408u, 0x7C000124u},
        {0x8000340Cu, 0x7FE802A6u}, {0x80003410u, 0x4836E305u}, {0x80003414u, 0x4836D8C9u},
        {0x80003418u, 0x4836F421u}, {0x8000341Cu, 0x7FE803A6u}, {0x80003420u, 0x4E800020u},
    };
    for (const InstructionPin& pin : helper) {
        Pin(image, pin.address, pin.word);
    }

    // Instructions after the stop, visible so the order is pinned. Not executed.
    Pin(image, 0x8000315Cu, 0x3800FFFFu);
    Pin(image, 0x8000316Cu, 0x480001D5u);

    // HID2 read/modify/write. The GQR clears are pinned separately.
    Pin(image, 0x80370BA8u, 0x7C78E2A6u);
    Pin(image, 0x80370BB0u, 0x7C78E3A6u);
    Pin(image, 0x80371724u, 0x6463A000u);
    Pin(image, 0x80371734u, 0x38600000u);
    Pin(image, 0x803725F4u, 0x7C70FAA6u);
    Pin(image, 0x803725F8u, 0x60630800u);
    Pin(image, 0x803725FCu, 0x7C70FBA6u);
}

void PinGqrClears(const BootImage& image) {
    const GuestWord32 first = 0x7C70E3A6u;
    for (unsigned index = 0; index < 8u; ++index) {
        Pin(image, 0x80371738u + index * 4u, first + (index << 16));
    }
}

void PinFprHelper(const BootImage& image) {
    const InstructionPin pins[] = {
        {0x80370CE8u, 0x7C78E2A6u}, {0x80370CECu, 0x54631FFFu}, {0x80370CF0u, 0x4182008Cu},
        {0x80370CF4u, 0x3C60805Fu}, {0x80370CF8u, 0x38631F38u}, {0x80370CFCu, 0xE0030000u},
        {0x80370D00u, 0x10200090u}, {0x80370D78u, 0x13E00090u}, {0x80370D7Cu, 0xC80D5A30u},
        {0x80370D80u, 0xFC200090u}, {0x80370DF8u, 0xFFE00090u}, {0x80370DFCu, 0xFDFE058Eu},
        {0x80370E00u, 0x4E800020u},
    };
    for (const InstructionPin& pin : pins) {
        Pin(image, pin.address, pin.word);
    }
}

void PinReportStub(const BootImage& image) {
    // Entire leaf: both branch paths only spill a frame, then drop it and return.
    const InstructionPin pins[] = {
        {0x80370C8Cu, 0x9421FF90u}, {0x80370C90u, 0x40860024u}, {0x80370C94u, 0xD8210028u},
        {0x80370C98u, 0xD8410030u}, {0x80370C9Cu, 0xD8610038u}, {0x80370CA0u, 0xD8810040u},
        {0x80370CA4u, 0xD8A10048u}, {0x80370CA8u, 0xD8C10050u}, {0x80370CACu, 0xD8E10058u},
        {0x80370CB0u, 0xD9010060u}, {0x80370CB4u, 0x90610008u}, {0x80370CB8u, 0x9081000Cu},
        {0x80370CBCu, 0x90A10010u}, {0x80370CC0u, 0x90C10014u}, {0x80370CC4u, 0x90E10018u},
        {0x80370CC8u, 0x9101001Cu}, {0x80370CCCu, 0x91210020u}, {0x80370CD0u, 0x91410024u},
        {0x80370CD4u, 0x38210070u}, {0x80370CD8u, 0x4E800020u},
    };
    for (const InstructionPin& pin : pins) {
        Pin(image, pin.address, pin.word);
    }
}

void PinPartialCacheStore(const BootImage& image) {
    const InstructionPin pins[] = {
        {0x803733A0u, 0x3C808058u}, {0x803733A4u, 0x57A513BAu}, {0x803733A8u, 0x38046CB0u},
        {0x803733ACu, 0x57A6043Eu}, {0x803733B0u, 0x7C802A14u}, {0x803733B4u, 0x83C40000u},
        {0x803733B8u, 0x28060010u}, {0x803733BCu, 0x7C7D1B78u}, {0x803733C0u, 0x93840000u},
        {0x803733C4u, 0x408201A0u}, {0x80372900u, 0x38600001u}, {0x80372904u, 0x48000A75u},
        {0x80373564u, 0x7FA3EB78u},
    };
    for (const InstructionPin& pin : pins) {
        Pin(image, pin.address, pin.word);
    }
}

void PinCrtTables(const BootImage& image) {
    GuestAddress32 cursor = NativeBootManifest::copy_table.raw;
    for (const CopyEntry& entry : kCopyEntries) {
        if (image.ReadWord(cursor) != entry.source ||
            image.ReadWord(cursor + 4u) != entry.destination ||
            image.ReadWord(cursor + 8u) != entry.size || entry.source != entry.destination) {
            throw BootError("CRT copy table does not match the pinned identity entries");
        }
        cursor += 12u;
    }
    if (image.ReadWord(cursor) != 0u || image.ReadWord(cursor + 4u) != 0u ||
        image.ReadWord(cursor + 8u) != 0u) {
        throw BootError("CRT copy table terminator mismatch");
    }

    cursor = NativeBootManifest::zero_table.raw;
    for (const ZeroEntry& entry : kZeroEntries) {
        if (image.ReadWord(cursor) != entry.address || image.ReadWord(cursor + 4u) != entry.size) {
            throw BootError("CRT zero table does not match the pinned ranges");
        }
        cursor += 8u;
    }
    if (image.ReadWord(cursor) != 0u || image.ReadWord(cursor + 4u) != 0u) {
        throw BootError("CRT zero table terminator mismatch");
    }
    if (!Contains(kZeroEntries[1].address, kZeroEntries[1].size, NativeBootManifest::fpr_source) ||
        !Contains(kZeroEntries[1].address, kZeroEntries[1].size, NativeBootManifest::paired_source)) {
        throw BootError("FPR source is not inside the later CRT zero range");
    }
}

void ReportOracle(const PreEntryOracle& oracle, bool applying) {
    if (!oracle.present) {
        std::cout << "  oracle=NO_ORACLE\n"
                  << "  fpr_semantics=UNKNOWN_NOT_APPLIED\n";
        return;
    }
    const bool fpr_zero = oracle.memory_0x805f1f30 == "0000000000000000";
    const bool paired_zero = oracle.memory_0x805f1f38 == "0000000000000000";
    std::cout << "  oracle=" << (applying ? "DOLPHIN_CONSUMED" : "DOLPHIN_OBSERVED_NOT_APPLIED")
              << " pc=" << oracle.pc << " 0x805F1F30=" << oracle.memory_0x805f1f30
              << " 0x805F1F38=" << oracle.memory_0x805f1f38;
    if (!oracle.msr.empty()) {
        std::cout << " msr=" << oracle.msr;
    }
    if (!oracle.hid0.empty()) {
        std::cout << " hid0=" << oracle.hid0;
    }
    if (!oracle.fpscr.empty()) {
        std::cout << " fpscr=" << oracle.fpscr;
    }
    std::cout << '\n';
    if (fpr_zero && paired_zero) {
        std::cout << "  fpr_semantics=OBSERVED_PLUS_ZERO all FPRs and FPSCR are the +0 image;"
                     " PS1 survives lfd/fmr; its observed source is also +0; "
                  << (applying ? "CONSUMED" : "NOT_APPLIED") << '\n';
    } else {
        std::cout << "  fpr_semantics=OBSERVED_NONZERO surviving image is the double at 0x805F1F30; "
                  << (applying ? "CONSUMED" : "NOT_APPLIED") << '\n';
    }
}

}  // namespace

void IsolateHardwareFrontier(const BootImage& image, const PreEntryOracle& oracle, bool applying) {
    PinHardwareWords(image);
    PinGqrClears(image);
    PinFprHelper(image);
    PinReportStub(image);
    PinPartialCacheStore(image);
    PinCrtTables(image);
    image.ExpectUnmapped(NativeBootManifest::fpr_source);
    image.ExpectUnmapped(NativeBootManifest::paired_source);

    std::cout
        << "FRONTIER hardware_init\n"
        << "  blocked_pc=0x80003158 blocked_call=0x80003400\n"
        << "  msr_fp_bit=set_by_ori_0x2000 native_effect=floating_point_available\n"
        << "  hid2=oris_0xA0000000 selects the psq path by setting bit 2; physical HID2 not installed\n"
        << "  gqr0_through_gqr7=cleared native_meaning=float_scale_0; SPR file not installed\n"
        << "  hid0_icfi=ori_0x0800 physical_cache_invalidate; no guest data effect pinned\n"
        << "  consumed=0x805F1F30 via lfd f0,0x5A30(r13); address is outside the fixture\n"
        << "  consumed=0x805F1F38 via psq_l; secondary lane survives the later lfd/fmr block\n"
        << "  report_stub=0x80370C8C discards its stack frame and returns; no guest write pinned\n"
        << "  partial_store=0x803733C0 writes 0x803726D8 to 0x80580000; r3 is the constant 1\n"
        << "  vector_writer=skipped because 1 != 0x10; epilogue has no further guest store\n"
        << "  hid_and_l2_branches=both sides traced; guest stores are stack spills only\n"
        << "  crt_copy_entries=10 identity; crt_zero_ranges=3; tables pinned\n"
        << "  ordering=FPR bytes are read before CRT zero, so a later zero is not an entry value\n";
    ReportOracle(oracle, applying);
    if (applying) {
        std::cout << "  next=native hardware semantics consume the FPR image; no PPC register file\n";
    } else {
        std::cout << "  next=FPR image is unknown without an oracle; M3 stays incomplete\n";
    }
}

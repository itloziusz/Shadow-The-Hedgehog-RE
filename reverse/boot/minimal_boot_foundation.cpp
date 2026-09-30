// PAL GUPP8P boot foothold. Test harness only: the eventual build embeds the
// validated section bytes; this program reads main.dol as a fixture.
#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

constexpr std::uint32_t kEntry = 0x80003154u;
constexpr std::uint32_t kRegisterHelper = 0x800032B0u;
constexpr std::uint32_t kStopBeforeHardware = 0x80003158u;
constexpr std::size_t kDolSize = 0x5816E0u;

struct Section {
    bool text;
    std::uint32_t index;
    std::uint32_t file_offset;
    std::uint32_t address;
    std::uint32_t size;
};

// DOL header + supplied DOL_SECTIONS.csv. These become generated build data.
struct BootManifest {
    static constexpr std::array<Section, 10> sections{{
        {true,  0, 0x00000100u, 0x80003100u, 0x00002500u},
        {true,  1, 0x00002600u, 0x80008D40u, 0x004A1F20u},
        {false, 0, 0x004A4520u, 0x80005600u, 0x00001F20u},
        {false, 1, 0x004A6440u, 0x80007520u, 0x00001820u},
        {false, 2, 0x004A7C60u, 0x804AAC60u, 0x00000480u},
        {false, 3, 0x004A80E0u, 0x804AB0E0u, 0x00000020u},
        {false, 4, 0x004A8100u, 0x804AB100u, 0x00072420u},
        {false, 5, 0x0051A520u, 0x8051D520u, 0x000528E0u},
        {false, 6, 0x0056CE00u, 0x805E4500u, 0x0000AB20u},
        {false, 7, 0x00577920u, 0x805F2780u, 0x00009DC0u},
    }};
};

std::uint32_t ReadBE32(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    if (offset > bytes.size() || bytes.size() - offset < 4) {
        throw std::runtime_error("read outside DOL fixture");
    }
    return (std::uint32_t(bytes[offset]) << 24) |
           (std::uint32_t(bytes[offset + 1]) << 16) |
           (std::uint32_t(bytes[offset + 2]) << 8) |
            std::uint32_t(bytes[offset + 3]);
}

class BootImage {
public:
    explicit BootImage(std::vector<std::uint8_t> dol) : dol_(std::move(dol)) {}

    std::uint32_t ReadWord(std::uint32_t address) const {
        if ((address & 3u) != 0u) throw std::runtime_error("unaligned guest word");
        for (const Section& s : BootManifest::sections) {
            if (address >= s.address &&
                std::uint64_t(address) + 4 <= std::uint64_t(s.address) + s.size) {
                return ReadBE32(dol_, s.file_offset + (address - s.address));
            }
        }
        // Unmapped/low-memory bytes are UNKNOWN, never silently zero.
        throw std::runtime_error("guest address is not in a known DOL section");
    }

private:
    std::vector<std::uint8_t> dol_;
};

BootImage InitializeMinimalBootState(std::vector<std::uint8_t> dol) {
    if (dol.size() != kDolSize || ReadBE32(dol, 0xE0) != kEntry ||
        ReadBE32(dol, 0xD8) != 0x8056FE00u ||
        ReadBE32(dol, 0xDC) != 0x0008C7ECu) {
        throw std::runtime_error("DOL size, entry, or BSS header mismatch");
    }
    for (const Section& s : BootManifest::sections) {
        const std::size_t i = s.index;
        const std::size_t offset_field = (s.text ? 0x00u : 0x1Cu) + i * 4u;
        const std::size_t address_field = (s.text ? 0x48u : 0x64u) + i * 4u;
        const std::size_t size_field = (s.text ? 0x90u : 0xACu) + i * 4u;
        if (ReadBE32(dol, offset_field) != s.file_offset ||
            ReadBE32(dol, address_field) != s.address ||
            ReadBE32(dol, size_field) != s.size ||
            std::uint64_t(s.file_offset) + s.size > dol.size() ||
            s.address < 0x80000000u ||
            std::uint64_t(s.address) + s.size > 0x81800000ull) {
            throw std::runtime_error("DOL section does not match BootManifest");
        }
    }
    return BootImage(std::move(dol));
}

struct StartupState {
    std::array<std::uint32_t, 32> gpr{};
    std::uint32_t pc = kEntry;
    std::uint32_t lr = 0;
};

StartupState EnterTranslatedGameStartup(const BootImage& image) {
    // The only translated entry instruction in this milestone:
    // 0x80003154: bl 0x800032B0; LR becomes 0x80003158.
    // Pin the next instruction too, so the stopping point cannot drift.
    if (image.ReadWord(kEntry) != 0x4800015Du ||
        image.ReadWord(kStopBeforeHardware) != 0x480002A9u ||
        image.ReadWord(kRegisterHelper) != 0x38000000u ||
        image.ReadWord(0x8000333Cu) != 0x4E800020u) {
        throw std::runtime_error("translated startup instruction fingerprint mismatch");
    }

    StartupState state;
    state.lr = kStopBeforeHardware;
    state.pc = kRegisterHelper;
    // Direct translation of 0x800032B0..0x8000333C: li clears all GPRs
    // except r1/r2/r13, then lis/ori establishes these three bases and blr.
    state.gpr[1] = 0x8060C5F0u;
    state.gpr[2] = 0x805FA780u;
    state.gpr[13] = 0x805EC500u;
    state.pc = state.lr;
    if (state.pc != kStopBeforeHardware) throw std::runtime_error("startup return mismatch");
    return state;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("usage: minimal_boot_foundation <main.dol>");
        std::ifstream file(argv[1], std::ios::binary);
        if (!file) throw std::runtime_error("cannot open main.dol fixture");
        std::vector<std::uint8_t> dol(std::istreambuf_iterator<char>{file}, {});
        const BootImage image = InitializeMinimalBootState(std::move(dol));
        const StartupState state = EnterTranslatedGameStartup(image);
        std::cout << std::hex << std::uppercase << std::setfill('0')
                  << "STOP pc=0x" << std::setw(8) << state.pc
                  << " lr=0x" << std::setw(8) << state.lr
                  << " r1=0x" << std::setw(8) << state.gpr[1]
                  << " r2=0x" << std::setw(8) << state.gpr[2]
                  << " r13=0x" << std::setw(8) << state.gpr[13] << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "minimal boot: " << e.what() << '\n';
        return 1;
    }
}

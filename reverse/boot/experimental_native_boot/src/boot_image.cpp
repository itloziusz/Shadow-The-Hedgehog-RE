#include "boot_image.h"

#include <sstream>

#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>

namespace {

GuestWord32 ReadBe32(const std::vector<std::uint8_t>& bytes, HostFileOffset64 offset) {
    if (offset > bytes.size() || bytes.size() - static_cast<std::size_t>(offset) < 4u) {
        throw BootError("read outside DOL fixture");
    }
    const std::size_t at = static_cast<std::size_t>(offset);
    return (GuestWord32{bytes[at]} << 24) | (GuestWord32{bytes[at + 1]} << 16) |
           (GuestWord32{bytes[at + 2]} << 8) | GuestWord32{bytes[at + 3]};
}

std::string Hex32(GuestWord32 value) {
    std::ostringstream out;
    out << std::hex << std::uppercase << value;
    return out.str();
}

}  // namespace

BootImage::BootImage(std::vector<std::uint8_t> dol) : dol_(std::move(dol)) {}

void BootImage::RequirePalFixtureDigest() const {
    if (dol_.size() != NativeBootManifest::dol_size) throw BootError("constructor fixture size mismatch");
    const std::array<std::uint8_t, 32> expected{
        0xfd,0xe4,0xfa,0x6f,0x81,0xa6,0x03,0x13,0xb7,0x10,0x16,0x1c,0x19,0x6d,0xc5,0x1c,
        0x22,0x60,0xbe,0x62,0x25,0x1e,0xe0,0x27,0x75,0xd5,0xee,0xe0,0x6f,0x9d,0x55,0xaf};
    std::array<std::uint8_t, 32> digest{};
    const auto status = BCryptHash(BCRYPT_SHA256_ALG_HANDLE, nullptr, 0,
        const_cast<PUCHAR>(dol_.data()), static_cast<ULONG>(dol_.size()),
        digest.data(), static_cast<ULONG>(digest.size()));
    if (status < 0 || digest != expected) throw BootError("constructor provenance requires the exact PAL SHA-256");
}

GuestWord32 BootImage::ReadWord(GuestAddress32 address) const {
    if ((address & 3u) != 0u) {
        throw BootError("unaligned guest word");
    }
    for (const DolSectionSpec& section : NativeBootManifest::sections) {
        const HostByteCount64 begin = section.address;
        const HostByteCount64 end = begin + section.size;
        if (HostByteCount64{address} >= begin && HostByteCount64{address} + 4u <= end) {
            guest_reads_.push_back(address);
            const HostFileOffset64 file_offset =
                HostFileOffset64{section.file_offset} + (address - section.address);
            return ReadBe32(dol_, file_offset);
        }
    }
    throw BootError("guest address 0x" + Hex32(address) + " is not in a known DOL section");
}

void BootImage::ExpectUnmapped(GuestAddress32 address) const {
    try {
        (void)ReadWord(address);
    } catch (const BootError&) {
        return;
    }
    throw BootError("guest address 0x" + Hex32(address) + " was expected to be unknown");
}

BootImage LoadValidatedFixture(const std::vector<std::uint8_t>& dol) {
    if (HostByteCount64{dol.size()} != NativeBootManifest::dol_size) {
        throw BootError("DOL size does not match the GUPP8P manifest");
    }
    if (ReadBe32(dol, 0xE0) != NativeBootManifest::entry ||
        ReadBe32(dol, 0xD8) != NativeBootManifest::bss_address ||
        ReadBe32(dol, 0xDC) != NativeBootManifest::bss_size) {
        throw BootError("DOL entry or BSS header does not match the GUPP8P manifest");
    }
    for (const DolSectionSpec& section : NativeBootManifest::sections) {
        const std::size_t index = section.index;
        const HostFileOffset64 offset_field = (section.text ? 0x00u : 0x1Cu) + index * 4u;
        const HostFileOffset64 address_field = (section.text ? 0x48u : 0x64u) + index * 4u;
        const HostFileOffset64 size_field = (section.text ? 0x90u : 0xACu) + index * 4u;
        if (ReadBe32(dol, offset_field) != section.file_offset ||
            ReadBe32(dol, address_field) != section.address ||
            ReadBe32(dol, size_field) != section.size) {
            throw BootError("DOL section does not match the GUPP8P manifest");
        }
        if (HostByteCount64{section.file_offset} + section.size > dol.size() ||
            section.address < 0x80000000u ||
            HostByteCount64{section.address} + section.size > 0x81800000ull) {
            throw BootError("DOL section is outside the fixture or the 24 MiB guest window");
        }
    }
    // Header reads above are file reads. The image starts with an empty guest-read log.
    return BootImage(dol);
}

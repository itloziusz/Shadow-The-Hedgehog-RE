#pragma once

#include "guest_types.h"
#include "native_boot_manifest.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

// Fixture-only view of the PAL DOL. Reads succeed only for addresses covered by
// the ten manifest sections. Every other guest address is unknown and throws.
// This is not the permanent program loader.

class BootImage {
public:
    explicit BootImage(std::vector<std::uint8_t> dol);

    GuestWord32 ReadWord(GuestAddress32 address) const;
    void ExpectUnmapped(GuestAddress32 address) const;
    // Constructor provenance is valid only for the exact observed PAL fixture,
    // including all deferred OS code, not just its selected instruction pins.
    void RequirePalFixtureDigest() const;

    const std::vector<GuestAddress32>& guest_reads() const { return guest_reads_; }

private:
    std::vector<std::uint8_t> dol_;
    mutable std::vector<GuestAddress32> guest_reads_;
};

BootImage LoadValidatedFixture(const std::vector<std::uint8_t>& dol);

class BootError : public std::runtime_error {
public:
    explicit BootError(const std::string& message) : std::runtime_error(message) {}
};

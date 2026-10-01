#pragma once

#include "shadow/boot/NativeBi2Prefix.hpp"
#include <string>
#include <vector>

namespace shadow::boot {

struct BootWorkWordGate {
    std::uint32_t address;
    std::uint32_t word;
};

struct BootWorkUnit {
    std::string name;
    // SHA256 of addressBE32 || wordBE32 in this semantic unit's declared order.
    // Runtime gates compare every exact word, rather than trusting this label.
    std::string raw_sha256;
    std::uint64_t repetitions;
    std::uint64_t original_operations;
    std::uint64_t source_operation_work;
};

struct BootElapsedWork {
    NativeBi2Run prefix;
    std::vector<BootWorkUnit> ledger;
    std::uint64_t original_operations = 0;
    std::uint64_t source_operation_work = 0;
    std::uint32_t stop_pc = 0;
    // Work becomes elapsed source time only after separate scheduler, epoch,
    // pending-exception and device/event owners are admitted. Physical Gekko
    // latency is not established by the private interpreter's cost table.
    static constexpr bool connected_clock_admitted = false;
};

// Explicit research basis: PPCTables.cpp from the matching private source,
// SHA256 4814b6c63088c0227eadf2fefa4f32fe2109afde7319b88697b35a7b67dcf771.
// C++ owns finite semantic work; no runtime interpreter/table, host timestamp,
// supplied frontier count, captured iteration list or per-read TB sample.
// Executes the unchanged NativeBi2 path from original inputs and requires its
// stop before 80379628. Unknown inputs and unsupported paths decline.
BootElapsedWork ProduceBootElapsedWorkResearch(const BootImage&, const NativeBi2Inputs&);

// Read-only exact code/data gates for independent mutation coverage. Includes
// all 65 semantic units, CRT descriptors and the unconsumed first TB word.
const std::vector<BootWorkWordGate>& BootElapsedWorkRawGatesResearch();

}  // namespace shadow::boot

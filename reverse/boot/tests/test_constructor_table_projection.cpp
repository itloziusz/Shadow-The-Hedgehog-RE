#include "shadow/boot/ConstructorTableProjection.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>

namespace {

void Require(bool valid, const char* message) {
    if (!valid) throw std::runtime_error(message);
}

void CheckProjection(std::uint32_t salt) {
    shadow::boot::MotionTableWords entries{};
    for (std::size_t slot = 0; slot < entries.size(); ++slot) {
        for (std::size_t word = 0; word < entries[slot].size(); ++word) {
            entries[slot][word] = salt + static_cast<std::uint32_t>(slot * 0x100 + word);
        }
    }
    const auto input = entries;
    const shadow::boot::ConstructorTriple shared{salt ^ 0xA5A50001u,
                                                  salt ^ 0x5A5A0002u,
                                                  salt ^ 0x12340003u};
    shadow::boot::ApplyMotionTableCopies(entries, shared);
    const int source[13] = {0, 1, 0, 1, 4, 4, -1, 7, -1, 7, 10, -1, 10};
    for (std::size_t slot = 0; slot < entries.size(); ++slot) {
        for (std::size_t word = 0; word < entries[slot].size(); ++word) {
            const auto expected = source[slot] < 0 ? shared[word] : input[source[slot]][word];
            Require(entries[slot][word] == expected,
                    "constructor table copy graph diverged from raw DOL mapping");
        }
    }
}

}  // namespace

int main() {
    try {
        CheckProjection(0u);
        CheckProjection(0xDEADBEEFu);
        std::cout << "constructor table projection passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

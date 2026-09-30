#pragma once

#include <cstdint>
#include <vector>

// Negative checks for the closed M3/M4 contracts. Runs before the real startup.
void RunFixtureContractTests(const std::vector<std::uint8_t>& dol);

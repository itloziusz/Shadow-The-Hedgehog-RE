#pragma once

#include <filesystem>
#include <string>
#include <array>
#include <map>
#include "guest_types.h"

// External observation. The boot executable never launches the tool that produced it.
// applied must stay false: seeing a value is not permission to install it.

struct PreEntryOracle {
    bool present = false;
    std::string pc;
    std::string msr;
    std::string hid0;
    std::string fpscr;
    std::string entry_instruction_hex;
    std::string memory_0x805f1f30;
    std::string memory_0x805f1f38;
    // Present together, or not at all. These resolve the post-CRT branches.
    bool has_lowmem = false;
    std::string lowmem_0x80000030;
    std::string lowmem_0x80000034;
    std::string lowmem_0x80000044;
    std::string lowmem_0x800000f4;
    std::string lowmem_0x800030e4;
    std::string lowmem_0x800030e6;
    std::string bi2_bytes;
};

PreEntryOracle ParsePreEntryOracle(const std::string& text);
PreEntryOracle LoadPreEntryOracle(const std::filesystem::path& path);

struct FragmentObservation {
    GuestWord32 id = 0;
    std::array<GuestWord32, 3> slot{};
    GuestWord32 r2 = 0;
    GuestWord32 r13 = 0;
};
struct Constructor0Oracle {
    bool present = false;
    std::array<std::string, 9> startup_inputs;
    // entry, after CRT, before walker, constructor entry. Comparisons only.
    std::array<FragmentObservation, 4> boundaries{};
    FragmentObservation final_state;
    GuestAddress32 final_pc = 0;
    GuestAddress32 final_cursor = 0;
    GuestAddress32 final_next = 0;
    GuestWord32 final_r1 = 0;
    GuestWord32 final_lr = 0;
};
Constructor0Oracle ParseConstructor0Oracle(const std::string& text);
Constructor0Oracle LoadConstructor0Oracle(const std::filesystem::path& path);

struct Constructor1Oracle {
    bool present = false;
    std::array<std::string, 9> startup_inputs;
    // Comparison-only fields. No observed final state feeds native execution.
    std::map<std::string, std::string> fields;
};
Constructor1Oracle ParseConstructor1Oracle(const std::string& text);
Constructor1Oracle LoadConstructor1Oracle(const std::filesystem::path& path);

struct Constructor2Oracle {
    bool present = false;
    std::array<std::string, 9> startup_inputs;
    std::map<std::string, std::string> fields; // comparisons only
};
Constructor2Oracle ParseConstructor2Oracle(const std::string& text);
Constructor2Oracle LoadConstructor2Oracle(const std::filesystem::path& path);

struct Constructor3Oracle {
    bool present = false;
    std::array<std::string,9> startup_inputs;
    std::map<std::string,std::string> fields; // comparisons only
};
Constructor3Oracle ParseConstructor3Oracle(const std::string& text);
Constructor3Oracle LoadConstructor3Oracle(const std::filesystem::path& path);

struct Constructor4Oracle {
    bool present = false;
    std::array<std::string,9> startup_inputs;
    std::map<std::string,std::string> fields; // comparisons only
};
Constructor4Oracle ParseConstructor4Oracle(const std::string& text);
Constructor4Oracle LoadConstructor4Oracle(const std::filesystem::path& path);

struct Constructor5Oracle {
    bool present = false;
    std::array<std::string,9> startup_inputs;
    std::map<std::string,std::string> fields; // comparisons only
};
Constructor5Oracle ParseConstructor5Oracle(const std::string& text);
Constructor5Oracle LoadConstructor5Oracle(const std::filesystem::path& path);

struct Constructor6Oracle {
    bool present = false;
    std::array<std::string,9> startup_inputs;
    std::map<std::string,std::string> fields;
};
Constructor6Oracle ParseConstructor6Oracle(const std::string& text);
Constructor6Oracle LoadConstructor6Oracle(const std::filesystem::path& path);

struct Constructor7Oracle {
    bool present = false;
    std::array<std::string,9> startup_inputs;
    std::map<std::string,std::string> fields;
};
Constructor7Oracle ParseConstructor7Oracle(const std::string& text);
Constructor7Oracle LoadConstructor7Oracle(const std::filesystem::path& path);

struct Constructor8Oracle {
    bool present = false;
    std::array<std::string,9> startup_inputs;
    std::map<std::string,std::string> fields;
};
Constructor8Oracle ParseConstructor8Oracle(const std::string& text);
Constructor8Oracle LoadConstructor8Oracle(const std::filesystem::path& path);

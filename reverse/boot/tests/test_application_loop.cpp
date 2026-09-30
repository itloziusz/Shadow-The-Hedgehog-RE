#include "shadow/boot/ApplicationLoop.hpp"
#include "shadow/boot/BootFoundation.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Require(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}

struct Trace final : shadow::boot::ApplicationLoopHooks {
    std::vector<std::string> calls;
    std::uint32_t exit = 0;
    unsigned services = 0;
    unsigned exit_after_services = 0;
    bool exit_on_frame_event = false;

    std::uint32_t ExitFlag() override {
        calls.push_back("check");
        return exit;
    }
    std::uint32_t DispatchEvent(std::uint32_t event, std::uint32_t argument) override {
        Require(argument == 0, "nonzero event argument");
        calls.push_back("event " + std::to_string(event));
        if (event == 0x12 && exit_on_frame_event) exit = 1;
        return 0; // The recurring loop ignores the dispatch return value.
    }
    void PostEventService() override {
        calls.push_back("service");
        if (++services == exit_after_services) exit = 2; // Any nonzero exit word.
    }
};

void CheckTrace(unsigned frames, std::uint32_t initial_exit,
                std::vector<std::string> expected) {
    Trace trace;
    trace.exit = initial_exit;
    trace.exit_after_services = frames;
    shadow::boot::RunApplicationRecurringPhase(trace);
    Require(trace.calls == expected, "recurring event/exit order differs");
    Require(trace.services == frames, "service count differs");
}

void CheckInstruction(const shadow::boot::BootImage& image,
                      std::uint32_t address, std::uint32_t instruction) {
    Require(image.ReadWord(address) == instruction, "PAL recurring-loop instruction differs");
}

} // namespace

int main(int argc, char** argv) {
    try {
        Require(argc == 2, "usage: shadow_boot_application_loop_tests <main.dol>");
        std::ifstream file(argv[1], std::ios::binary);
        Require(static_cast<bool>(file), "cannot open PAL DOL fixture");
        const std::vector<std::uint8_t> bytes(std::istreambuf_iterator<char>{file}, {});
        const auto image = shadow::boot::LoadValidatedFixture(bytes);

        // Independent exact-word guard for the translation boundary. The CTest
        // wrapper also checks the full PAL DOL SHA-256 before this executable.
        CheckInstruction(image, 0x800511E0u, 0x48000014u); // branch to check
        CheckInstruction(image, 0x800511E4u, 0x38600012u); // event 0x12
        CheckInstruction(image, 0x800511E8u, 0x38800000u); // null argument
        CheckInstruction(image, 0x800511ECu, 0x4800078Du); // dispatch
        CheckInstruction(image, 0x800511F0u, 0x482DC255u); // service
        CheckInstruction(image, 0x800511F4u, 0x801F000Cu); // exit word
        CheckInstruction(image, 0x800511F8u, 0x2C000000u); // compare zero
        CheckInstruction(image, 0x800511FCu, 0x4182FFE8u); // repeat if zero
        CheckInstruction(image, 0x80051200u, 0x3860000Eu); // event 0x0E
        CheckInstruction(image, 0x80051208u, 0x48000771u); // dispatch
        CheckInstruction(image, 0x8005120Cu, 0x38600011u); // event 0x11
        CheckInstruction(image, 0x80051214u, 0x48000765u); // dispatch
        // The game's event 0x12 case is reached through an initialized
        // dispatcher; these words pin its static target and exit setter only.
        CheckInstruction(image, 0x800519A8u, 0x4BFF7511u); // event table dispatcher
        CheckInstruction(image, 0x8051E628u, 0x80049078u); // event 0x12 slot
        CheckInstruction(image, 0x80049078u, 0x48199199u); // Game_FrameStep
        CheckInstruction(image, 0x8004907Cu, 0x5460063Fu); // low-byte test
        CheckInstruction(image, 0x80049080u, 0x40820014u); // keep running
        CheckInstruction(image, 0x80049090u, 0x9003000Cu); // exit word

        CheckTrace(0, 1, {"check", "event 14", "event 17"});
        CheckTrace(1, 0, {"check", "event 18", "service", "check",
                          "event 14", "event 17"});
        CheckTrace(3, 0, {"check", "event 18", "service", "check",
                          "event 18", "service", "check", "event 18",
                          "service", "check", "event 14", "event 17"});
        Trace exit_during_dispatch;
        exit_during_dispatch.exit_on_frame_event = true;
        shadow::boot::RunApplicationRecurringPhase(exit_during_dispatch);
        Require(exit_during_dispatch.calls ==
                    std::vector<std::string>{"check", "event 18", "service",
                                             "check", "event 14", "event 17"},
                "service was skipped after dispatch set the exit word");
        std::cout << "PAL application recurring phase: event order verified\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

#include "startup_clock.h"

#define NOMINMAX
#include <windows.h>

#include <iostream>
#include <stdexcept>

#include <psapi.h>

StartupClock::StartupClock() {
    LARGE_INTEGER frequency{};
    LARGE_INTEGER now{};
    if (!QueryPerformanceFrequency(&frequency) || !QueryPerformanceCounter(&now) ||
        frequency.QuadPart <= 0) {
        throw std::runtime_error("QueryPerformanceCounter is unavailable");
    }
    frequency_ = frequency.QuadPart;
    origin_ = now.QuadPart;
}

void StartupClock::Mark(const char* name) {
    LARGE_INTEGER now{};
    if (!QueryPerformanceCounter(&now)) throw std::runtime_error("QueryPerformanceCounter failed");
    samples_.push_back({name,now.QuadPart});
}

void StartupClock::Report() const {
    long long previous = origin_;
    for (const auto& sample : samples_) {
        const long long delta = sample.ticks - previous;
        const long long since_entry = sample.ticks - origin_;
        const auto nanoseconds = [](long long ticks, long long frequency) {
            return (ticks * 1000000000ll) / frequency;
        };
        std::cout << "TIMING " << sample.name << " delta_ns=" << nanoseconds(delta, frequency_)
                  << " since_entry_ns=" << nanoseconds(since_entry, frequency_) << '\n';
        previous = sample.ticks;
    }
}

void StartupClock::ReportMemory(const char* label) const {
    PROCESS_MEMORY_COUNTERS counters{};
    counters.cb = sizeof(counters);
    if (!GetProcessMemoryInfo(GetCurrentProcess(), &counters, sizeof(counters))) {
        std::cout << "MEMORY " << label << " unavailable\n";
        return;
    }
    std::cout << "MEMORY " << label << " working_set=" << counters.WorkingSetSize
              << " peak_working_set=" << counters.PeakWorkingSetSize
              << " pagefile=" << counters.PagefileUsage
              << " peak_pagefile=" << counters.PeakPagefileUsage << '\n';
}

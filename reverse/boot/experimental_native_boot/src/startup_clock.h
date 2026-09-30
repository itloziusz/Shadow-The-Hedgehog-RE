#pragma once
#include <vector>

// Monotonic Windows performance counter. Durations are printed in nanoseconds.
class StartupClock {
public:
    StartupClock();
    void Mark(const char* name);
    void Report() const;
    void ReportMemory(const char* label) const;

private:
    long long origin_ = 0;
    long long frequency_ = 1;
    struct Sample { const char* name; long long ticks; };
    std::vector<Sample> samples_;
};

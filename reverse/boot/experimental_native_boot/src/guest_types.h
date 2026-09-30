#pragma once

#include <cstdint>

// Guest values keep the original 32-bit program model.
// They are not host pointers and must not be widened just because the process is 64-bit.

using GuestAddress32 = std::uint32_t;
using GuestOffset32 = std::uint32_t;
using GuestWord32 = std::uint32_t;

struct GuestPtr32 {
    GuestAddress32 raw = 0;

    explicit constexpr GuestPtr32(GuestAddress32 address) : raw(address) {}
};

// Host quantities are real 64-bit process state. This boot lab has no guest machine
// behind them: a file size is a host byte count, not a MEM1 offset.
using HostFileOffset64 = std::uint64_t;
using HostByteCount64 = std::uint64_t;
using HostAllocationSize64 = std::uint64_t;

template <typename T>
using HostPtr = T*;

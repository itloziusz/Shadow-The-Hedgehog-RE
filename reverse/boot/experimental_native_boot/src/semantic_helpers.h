#pragma once
#include "constructor_7.h"

// Ordered SetField motif. No guest memory, allocation or callback machinery.
// Contract: caller owns two writable aligned words; r3 identity survives.
std::array<GuestWord32,2> InitializeWordPair(const BootImage& image,
    GuestAddress32 helper, GuestAddress32 object, std::array<GuestWord32,2> inputs,
    GuestAddress32 return_pc, std::vector<Constructor1Event>& events);

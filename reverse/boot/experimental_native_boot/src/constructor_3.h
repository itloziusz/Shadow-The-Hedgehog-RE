#pragma once
#include "constructor_2.h"

struct Constructor3BodyResult {
    NativeVector3 vector;
    std::array<std::uint64_t,2> fpr01; // lfs updates both lanes
    GuestAddress32 destination;
    std::vector<Constructor1Event> events;
};
struct Constructor3Result {
    Constructor2Result preserved; // historical state; body.fpr01 is current
    Constructor3BodyResult body;
    GuestAddress32 pc, cursor, next_constructor;
    GuestWord32 lr;
    unsigned constructors_executed;
};
// Two proven binary32 source words, not oracle final output or a memory image.
Constructor3BodyResult RunConstructor3Body(const BootImage& image,
    std::array<GuestWord32,2> inputs, GuestWord32 sda2, GuestWord32 return_pc, bool fpu_enabled);
Constructor3Result ExecuteFourthConstructor(const BootImage& image, const PreEntryOracle& pre,
    const Constructor2Result& previous, const Constructor2Oracle& prior_evidence,
    const Constructor3Oracle& oracle);

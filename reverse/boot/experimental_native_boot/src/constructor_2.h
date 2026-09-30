#pragma once
#include "constructor_1.h"

// These are value bits, not a guest RAM mapping. Source state is materialized
// only after its CRT origin and preservation projection have been checked.
using NativeVector3 = std::array<GuestWord32, 3>;
struct Constructor2BodyResult {
    std::array<NativeVector3, 2> vectors;
    std::array<std::uint64_t, 2> fpr01; // each lfs fills both PS0 and PS1
    GuestAddress32 source_address, destination_address;
    GuestWord32 r1, return_pc, returned_r3;
    std::vector<Constructor1Event> events;
};
struct Constructor2Result {
    // Earlier semantic effects remain owned and unchanged, including context
    // tokens. fpr01 below supersedes the earlier f0/f1 values, not f2 or f31.
    Constructor1Result preserved;
    NativeVector3 source;
    Constructor2BodyResult body;
    GuestAddress32 pc, cursor, next_constructor;
    GuestWord32 lr;
    unsigned constructors_executed;
};

std::uint64_t WidenConstructor2Single(GuestWord32 bits);
GuestWord32 ConstructorWalkerCompareCr(GuestWord32 cr, GuestWord32 xer, GuestWord32 next);
Constructor2BodyResult RunConstructor2Body(const BootImage& image, const NativeVector3& source,
    GuestWord32 stack, GuestWord32 cursor, GuestWord32 return_pc, bool fpu_enabled);
Constructor2Result ExecuteThirdConstructor(const BootImage& image, const CrtSemantics& crt,
    const PreEntryOracle& pre, const Constructor1Result& previous, const Constructor2Oracle& oracle);

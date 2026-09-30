#pragma once
#include "constructor_4.h"

struct Constructor5BodyResult {
    std::array<std::uint8_t,4> bytes;
    std::array<GuestWord32,4> arguments; // helper leaves r4-r7 unchanged
    GuestAddress32 destination;
    GuestWord32 r1, return_pc;
    std::vector<Constructor1Event> events;
};
struct Constructor5Result {
    Constructor4Result preserved; // includes live destructor registration; never invoked here
    Constructor5BodyResult body;
    GuestAddress32 pc, cursor, next_constructor;
    GuestWord32 lr;
    unsigned constructors_executed;
};
Constructor5BodyResult RunConstructor5Body(const BootImage& image,
    std::array<GuestWord32,4> arguments, GuestWord32 sda, GuestWord32 stack, GuestWord32 return_pc);
Constructor5Result ExecuteSixthConstructor(const BootImage& image, const PreEntryOracle& pre,
    const Constructor4Result& previous, const Constructor4Oracle& prior_evidence,
    const Constructor5Oracle& oracle);

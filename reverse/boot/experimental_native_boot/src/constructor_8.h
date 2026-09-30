#pragma once
#include "semantic_helpers.h"

struct Constructor8BodyResult {
    std::array<std::array<GuestWord32,2>,2> pairs;
    std::array<GuestAddress32,2> destinations;
    GuestWord32 r1,return_pc;
    std::vector<Constructor1Event> events;
};
struct Constructor8Result {
    Constructor7Result preserved;
    Constructor8BodyResult body;
    GuestAddress32 pc,cursor,next_constructor;
    GuestWord32 lr;
    unsigned constructors_executed;
};
Constructor8BodyResult RunConstructor8Body(const BootImage& image,
    std::array<std::array<GuestWord32,2>,2> arguments,GuestWord32 sda,
    GuestWord32 stack,GuestWord32 return_pc);
Constructor8Result ExecuteNinthConstructor(const BootImage& image,const PreEntryOracle& pre,
    const Constructor7Result& previous,const Constructor7Oracle& prior,const Constructor8Oracle& oracle);

#pragma once
#include "constructor_6.h"

struct Constructor7BodyResult {
    std::array<GuestWord32,4> objects;
    std::array<GuestAddress32,4> object_addresses, node_addresses;
    std::array<NativeDestructorRegistration,4> registrations;
    GuestAddress32 head_address, head;
    GuestWord32 r1, return_pc;
    std::vector<Constructor1Event> events;
};
struct Constructor7Result {
    Constructor6Result preserved; // prior list nodes remain owned; body.head is current
    Constructor7BodyResult body;
    GuestAddress32 pc, cursor, next_constructor;
    GuestWord32 lr;
    unsigned constructors_executed;
};
Constructor7BodyResult RunConstructor7Body(const BootImage& image, GuestAddress32 old_head,
    GuestWord32 sda, GuestWord32 stack, GuestWord32 cursor, GuestWord32 return_pc);
Constructor7Result ExecuteEighthConstructor(const BootImage& image, const PreEntryOracle& pre,
    const Constructor6Result& previous, const Constructor6Oracle& prior_evidence,
    const Constructor7Oracle& oracle);

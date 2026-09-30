#pragma once
#include "constructor_3.h"

struct NativeDestructorRegistration {
    GuestAddress32 next, destructor, object; // identities only; no callback invocation
};
struct Constructor4BodyResult {
    std::array<GuestWord32,2> object;
    NativeDestructorRegistration registration;
    GuestAddress32 object_address, node_address, head_address, head;
    GuestWord32 r1, return_pc;
    std::vector<Constructor1Event> events;
};
struct Constructor4Result {
    Constructor3Result preserved; // FPRs and all earlier semantic state unchanged
    Constructor4BodyResult body;
    GuestAddress32 pc, cursor, next_constructor;
    GuestWord32 lr;
    unsigned constructors_executed;
};
Constructor4BodyResult RunConstructor4Body(const BootImage& image, GuestAddress32 old_head,
    GuestWord32 sda, GuestWord32 stack, GuestWord32 return_pc);
Constructor4Result ExecuteFifthConstructor(const BootImage& image, const CrtSemantics& crt,
    const PreEntryOracle& pre, const Constructor3Result& previous,
    const Constructor3Oracle& prior_evidence, const Constructor4Oracle& oracle);

#pragma once
#include "constructor_5.h"

struct Constructor6BodyResult {
    std::vector<std::array<std::uint16_t,2>> elements;
    NativeDestructorRegistration registration;
    GuestAddress32 array_address, element_constructor, cleanup_destructor;
    GuestAddress32 node_address, head_address, head;
    GuestWord32 stride, count, r1, return_pc;
    std::vector<Constructor1Event> events;
};
struct Constructor6Result {
    Constructor5Result preserved; // earlier node remains owned; body.head is the current head
    Constructor6BodyResult body;
    GuestAddress32 pc, cursor, next_constructor;
    GuestWord32 lr;
    unsigned constructors_executed;
};
Constructor6BodyResult RunConstructor6Body(const BootImage& image, GuestAddress32 old_head,
    GuestWord32 sda, GuestWord32 stack, GuestWord32 cursor, GuestWord32 return_pc,
    unsigned count, std::array<GuestWord32,2> element_values);
Constructor6Result ExecuteSeventhConstructor(const BootImage& image, const PreEntryOracle& pre,
    const Constructor5Result& previous, const Constructor5Oracle& prior_evidence,
    const Constructor6Oracle& oracle);

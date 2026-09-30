#pragma once

#include "crt_semantics.h"
#include "runtime_route.h"
#include <string_view>
#include <vector>

// Four semantic words, not a guest address space or host pointer masquerading
// as a PPC address. Descriptor identity remains available to later consumers.
struct FragmentState {
    GuestWord32 registration_id;
    GuestPtr32 descriptor;
    GuestPtr32 sda2;
    GuestWord32 occupied;
};
struct ConstructorEvent {
    std::string_view kind;
    GuestAddress32 pc;
    GuestAddress32 address;
    GuestWord32 value;
};
struct Constructor0Result {
    FragmentState fragment;
    GuestAddress32 pc;
    GuestAddress32 cursor;
    GuestAddress32 next_constructor;
    GuestWord32 r1;
    GuestWord32 lr;
    unsigned constructors_executed;
    std::vector<ConstructorEvent> events;
};

// Actual native translation of the body, also exercised with alternate inputs
// by contract tests. The caller owns order and verified initialization.
void RunConstructor0Body(const BootImage& image, FragmentState& state,
                        GuestWord32 sda2, GuestWord32 stack, GuestWord32 return_pc,
                        std::vector<ConstructorEvent>& events);
Constructor0Result ExecuteFirstConstructor(const BootImage& image, const CrtSemantics& crt,
    const HardwareSemantics& hardware, const RuntimeRoute& route, const PreEntryOracle& pre_entry,
    const Constructor0Oracle& oracle);

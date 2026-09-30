#include "shadow/boot/ConstructorTableProjection.hpp"

namespace shadow::boot {

void ApplyMotionTableCopies(MotionTableWords& entries,
                            ConstructorTriple shared_source) {
    // Raw PAL bodies at constructor indices 135, 141, 143, 145, 147, 149,
    // 151 and 155 copy these complete 12-byte entries in this order. The
    // source and destination addresses are disjoint in all eight bodies.
    entries[2] = entries[0];
    entries[3] = entries[1];
    entries[5] = entries[4];
    entries[6] = shared_source;
    entries[8] = shared_source;
    entries[9] = entries[7];
    entries[11] = shared_source;
    entries[12] = entries[10];
}

}  // namespace shadow::boot

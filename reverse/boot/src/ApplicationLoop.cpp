#include "shadow/boot/ApplicationLoop.hpp"

namespace shadow::boot {

void RunApplicationRecurringPhase(ApplicationLoopHooks& hooks) {
    // 0x800511E0 branches to the exit check before the first event.
    while (hooks.ExitFlag() == 0) {
        (void)hooks.DispatchEvent(0x12u, 0u); // 0x800511E4..0x800511EC
        hooks.PostEventService();              // 0x800511F0
    }
    (void)hooks.DispatchEvent(0x0Eu, 0u); // 0x80051200..0x80051208
    (void)hooks.DispatchEvent(0x11u, 0u); // 0x8005120C..0x80051214
}

} // namespace shadow::boot

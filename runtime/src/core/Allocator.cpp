#include "shadowpc/core/Allocator.hpp"
#include <cstdint>

namespace shadowpc {

void* SystemAllocator::allocate(std::size_t bytes, std::size_t alignment) {
    return ::operator new(bytes, std::align_val_t(alignment));
}
void SystemAllocator::deallocate(void* ptr, std::size_t, std::size_t alignment) {
    ::operator delete(ptr, std::align_val_t(alignment));
}

LinearArena::LinearArena(std::size_t capacity) : memory_(capacity) {}

void* LinearArena::allocate(std::size_t bytes, std::size_t alignment) {
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) return nullptr;
    const std::size_t aligned = (cursor_ + alignment - 1) & ~(alignment - 1);
    if (aligned + bytes > memory_.size()) return nullptr;
    cursor_ = aligned + bytes;
    return memory_.data() + aligned;
}

} // namespace shadowpc

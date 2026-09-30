#include "shadowpc/core/VirtualArena.hpp"
#include <algorithm>
#include <bit>
#include <new>
#include <stdexcept>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace shadowpc {
namespace {
std::size_t align_up(std::size_t v, std::size_t a) { return (v + a - 1) & ~(a - 1); }
}
VirtualArena::VirtualArena(std::size_t reserve_bytes) {
    if (reserve_bytes == 0) throw std::invalid_argument("VirtualArena reserve must be non-zero");
#ifdef _WIN32
    SYSTEM_INFO info{}; GetSystemInfo(&info); page_size_ = info.dwPageSize;
    reserved_ = align_up(reserve_bytes, page_size_);
    base_ = static_cast<std::byte*>(VirtualAlloc(nullptr, reserved_, MEM_RESERVE, PAGE_NOACCESS));
#else
    page_size_ = static_cast<std::size_t>(::sysconf(_SC_PAGESIZE));
    reserved_ = align_up(reserve_bytes, page_size_);
    void* p = ::mmap(nullptr, reserved_, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    base_ = (p == MAP_FAILED) ? nullptr : static_cast<std::byte*>(p);
#endif
    if (!base_) throw std::bad_alloc{};
}
VirtualArena::~VirtualArena() {
    if (!base_) return;
#ifdef _WIN32
    VirtualFree(base_, 0, MEM_RELEASE);
#else
    ::munmap(base_, reserved_);
#endif
}
bool VirtualArena::commit_to(std::size_t required) {
    if (required > reserved_) return false;
    const auto target = align_up(required, page_size_);
    if (target <= committed_) return true;
    const auto delta = target - committed_;
#ifdef _WIN32
    if (!VirtualAlloc(base_ + committed_, delta, MEM_COMMIT, PAGE_READWRITE)) return false;
#else
    if (::mprotect(base_ + committed_, delta, PROT_READ | PROT_WRITE) != 0) return false;
#endif
    committed_ = target;
    return true;
}
void* VirtualArena::allocate(std::size_t bytes, std::size_t alignment) {
    if (bytes == 0) bytes = 1;
    if (!std::has_single_bit(alignment)) throw std::invalid_argument("alignment must be power of two");
    const auto begin = align_up(used_, alignment);
    const auto end = begin + bytes;
    if (end < begin || !commit_to(end)) return nullptr;
    used_ = end;
    return base_ + begin;
}
} // namespace shadowpc

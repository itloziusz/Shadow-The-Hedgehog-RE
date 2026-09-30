#pragma once
#include <cstddef>
#include <cstdint>

namespace shadowpc {

class VirtualArena {
public:
    explicit VirtualArena(std::size_t reserve_bytes);
    ~VirtualArena();
    VirtualArena(const VirtualArena&) = delete;
    VirtualArena& operator=(const VirtualArena&) = delete;

    void* allocate(std::size_t bytes, std::size_t alignment = 16);
    void reset() noexcept { used_ = 0; }
    std::size_t reserved_bytes() const noexcept { return reserved_; }
    std::size_t committed_bytes() const noexcept { return committed_; }
    std::size_t used_bytes() const noexcept { return used_; }

private:
    bool commit_to(std::size_t required);
    std::byte* base_{};
    std::size_t reserved_{};
    std::size_t committed_{};
    std::size_t used_{};
    std::size_t page_size_{};
};

} // namespace shadowpc

#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <new>
#include <optional>
#include <vector>

namespace shadowpc {

class IAllocator {
public:
    virtual ~IAllocator() = default;
    virtual void* allocate(std::size_t bytes, std::size_t alignment) = 0;
    virtual void deallocate(void* ptr, std::size_t bytes, std::size_t alignment) = 0;
};

class SystemAllocator final : public IAllocator {
public:
    void* allocate(std::size_t bytes, std::size_t alignment) override;
    void deallocate(void* ptr, std::size_t bytes, std::size_t alignment) override;
};

class LinearArena final : public IAllocator {
public:
    explicit LinearArena(std::size_t capacity);
    void* allocate(std::size_t bytes, std::size_t alignment) override;
    void deallocate(void*, std::size_t, std::size_t) override {}
    void reset() noexcept { cursor_ = 0; }
    std::size_t used() const noexcept { return cursor_; }
    std::size_t capacity() const noexcept { return memory_.size(); }
private:
    std::vector<std::byte> memory_;
    std::size_t cursor_{};
};

template <class T>
class SlabPool {
public:
    explicit SlabPool(std::size_t capacity) : slots_(capacity), free_() {
        free_.reserve(capacity);
        for (std::size_t i = capacity; i-- > 0;) free_.push_back(i);
    }
    template<class... Args> T* create(Args&&... args) {
        if (free_.empty()) return nullptr;
        const auto idx = free_.back();
        free_.pop_back();
        slots_[idx].emplace(std::forward<Args>(args)...);
        return &*slots_[idx];
    }
    void destroy(T* ptr) {
        if (!ptr) return;
        for (std::size_t i=0;i<slots_.size();++i) {
            if (slots_[i] && &*slots_[i] == ptr) {
                slots_[i].reset();
                free_.push_back(i);
                return;
            }
        }
    }
    std::size_t live_count() const noexcept { return slots_.size() - free_.size(); }
private:
    std::vector<std::optional<T>> slots_;
    std::vector<std::size_t> free_;
};

} // namespace shadowpc

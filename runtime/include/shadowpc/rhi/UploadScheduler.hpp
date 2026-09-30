#pragma once
#include "shadowpc/assets/AssetDecoder.hpp"
#include "shadowpc/rhi/Rhi.hpp"
#include <cstdint>
#include <deque>
#include <optional>
#include <unordered_map>
#include <variant>

namespace shadowpc::rhi {
struct UploadTicket { std::uint64_t value{}; constexpr explicit operator bool() const noexcept { return value!=0; } };
enum class UploadState : std::uint8_t { Queued, Submitted, Ready, Failed };
struct UploadResult { UploadState state{UploadState::Queued}; shadowpc::GpuAsset gpu; };

class UploadScheduler {
public:
    explicit UploadScheduler(IGraphicsBackend& backend):backend_(backend){}
    UploadTicket enqueue(shadowpc::DecodedAsset asset, AssetType type);
    void tick(std::uint32_t submit_budget=8);
    std::optional<UploadResult> query(UploadTicket) const;
    std::optional<UploadResult> collect(UploadTicket);
private:
    struct Job { UploadTicket ticket; AssetType type{}; shadowpc::DecodedAsset asset; UploadResult result; };
    void submit(Job&);
    IGraphicsBackend& backend_;
    std::uint64_t next_{1};
    std::deque<std::uint64_t> queue_;
    std::unordered_map<std::uint64_t,Job> jobs_;
};
} // namespace shadowpc::rhi

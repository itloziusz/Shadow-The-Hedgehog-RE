#pragma once
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace shadowpc {

enum class ResourcePriority : std::uint8_t { Background=0, Low=1, Normal=2, High=3, Critical=4 };
using IoTicket = std::uint64_t;

struct IoRequest {
    std::filesystem::path path;
    std::uint64_t offset{};
    std::uint64_t size{};
    ResourcePriority priority{ResourcePriority::Normal};
};
struct IoResult {
    bool ok{};
    std::vector<std::byte> bytes;
    std::string error;
};

class IAsyncIoScheduler {
public:
    virtual ~IAsyncIoScheduler() = default;
    virtual IoTicket submit(IoRequest request) = 0;
    virtual std::optional<IoResult> try_collect(IoTicket ticket) = 0;
    virtual void cancel(IoTicket ticket) = 0;
};

class ThreadedIoScheduler final : public IAsyncIoScheduler {
public:
    explicit ThreadedIoScheduler(std::size_t workers = 0);
    ~ThreadedIoScheduler() override;
    IoTicket submit(IoRequest request) override;
    std::optional<IoResult> try_collect(IoTicket ticket) override;
    void cancel(IoTicket ticket) override;
    std::size_t worker_count() const noexcept { return workers_.size(); }

private:
    struct Queued {
        IoTicket ticket{};
        IoRequest request;
        std::uint64_t sequence{};
    };
    struct Compare {
        bool operator()(const Queued& a, const Queued& b) const noexcept {
            if (a.request.priority != b.request.priority)
                return static_cast<int>(a.request.priority) < static_cast<int>(b.request.priority);
            return a.sequence > b.sequence;
        }
    };
    void worker_loop(std::stop_token stop);

    std::atomic<IoTicket> next_ticket_{1};
    std::atomic<std::uint64_t> next_sequence_{1};
    std::mutex mutex_;
    std::condition_variable_any cv_;
    std::priority_queue<Queued, std::vector<Queued>, Compare> queue_;
    std::unordered_map<IoTicket, IoResult> completed_;
    std::unordered_set<IoTicket> cancelled_;
    std::vector<std::jthread> workers_;
};

} // namespace shadowpc

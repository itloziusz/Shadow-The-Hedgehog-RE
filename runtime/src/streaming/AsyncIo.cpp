#include "shadowpc/streaming/AsyncIo.hpp"
#include <algorithm>
#include <fstream>

namespace shadowpc {

ThreadedIoScheduler::ThreadedIoScheduler(std::size_t workers) {
    if (workers == 0) workers = std::max<std::size_t>(2, std::thread::hardware_concurrency() / 2);
    workers = std::clamp<std::size_t>(workers, 1, 32);
    workers_.reserve(workers);
    for (std::size_t i=0;i<workers;++i) workers_.emplace_back([this](std::stop_token st){ worker_loop(st); });
}
ThreadedIoScheduler::~ThreadedIoScheduler() {
    for (auto& w : workers_) w.request_stop();
    cv_.notify_all();
}
IoTicket ThreadedIoScheduler::submit(IoRequest request) {
    const auto ticket = next_ticket_.fetch_add(1);
    const auto seq = next_sequence_.fetch_add(1);
    {
        std::lock_guard lock(mutex_);
        queue_.push(Queued{ticket, std::move(request), seq});
    }
    cv_.notify_one();
    return ticket;
}
std::optional<IoResult> ThreadedIoScheduler::try_collect(IoTicket ticket) {
    std::lock_guard lock(mutex_);
    auto it=completed_.find(ticket);
    if (it==completed_.end()) return std::nullopt;
    IoResult result=std::move(it->second); completed_.erase(it); cancelled_.erase(ticket);
    return result;
}
void ThreadedIoScheduler::cancel(IoTicket ticket) {
    std::lock_guard lock(mutex_); cancelled_.insert(ticket);
}
void ThreadedIoScheduler::worker_loop(std::stop_token stop) {
    while (!stop.stop_requested()) {
        Queued work;
        {
            std::unique_lock lock(mutex_);
            cv_.wait(lock, stop, [this]{ return !queue_.empty(); });
            if (stop.stop_requested()) return;
            work=queue_.top(); queue_.pop();
            if (cancelled_.contains(work.ticket)) continue;
        }
        IoResult result;
        try {
            std::ifstream f(work.request.path, std::ios::binary);
            if (!f) throw std::runtime_error("open failed");
            f.seekg(static_cast<std::streamoff>(work.request.offset));
            if (!f) throw std::runtime_error("seek failed");
            result.bytes.resize(static_cast<std::size_t>(work.request.size));
            f.read(reinterpret_cast<char*>(result.bytes.data()), static_cast<std::streamsize>(result.bytes.size()));
            if (static_cast<std::size_t>(f.gcount()) != result.bytes.size()) throw std::runtime_error("short read");
            result.ok=true;
        } catch (const std::exception& e) {
            result.ok=false; result.error=e.what(); result.bytes.clear();
        }
        std::lock_guard lock(mutex_);
        if (!cancelled_.contains(work.ticket)) completed_[work.ticket]=std::move(result);
    }
}

} // namespace shadowpc

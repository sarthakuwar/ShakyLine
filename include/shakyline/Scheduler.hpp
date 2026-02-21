#pragma once

#include <asio.hpp>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace shakyline {

/// Timer scheduler using asio::steady_timer
/// Supports weak_ptr guards for safe cancellation
class Scheduler {
public:
    using TimerId = uint64_t;
    using Callback = std::function<void()>;
    using Clock = std::chrono::steady_clock;

    explicit Scheduler(asio::io_context& io);
    ~Scheduler() = default;

    // Non-copyable, non-movable
    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    /// Schedule a callback after delay
    /// Returns timer ID for cancellation
    TimerId schedule(std::chrono::milliseconds delay, Callback cb);

    /// Schedule with a guard - callback only fires if guard is still valid
    template<typename T>
    TimerId scheduleGuarded(std::chrono::milliseconds delay,
                            std::weak_ptr<T> guard,
                            std::function<void(std::shared_ptr<T>)> cb) {
        return schedule(delay, [guard = std::move(guard), cb = std::move(cb)]() {
            if (auto locked = guard.lock()) {
                cb(std::move(locked));
            }
        });
    }

    /// Cancel a scheduled timer
    /// Returns true if timer was found and cancelled
    bool cancel(TimerId id);

    /// Cancel all timers
    void cancelAll();

    /// Number of active timers
    std::size_t activeCount() const;

private:
    asio::io_context& io_;
    std::mutex mutex_;
    std::unordered_map<TimerId, std::unique_ptr<asio::steady_timer>> timers_;
    std::atomic<TimerId> nextId_{1};
};

} // namespace shakyline

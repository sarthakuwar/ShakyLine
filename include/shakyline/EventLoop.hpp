#pragma once

#include <asio.hpp>
#include <atomic>
#include <thread>

namespace shakyline {

/// Wrapper around asio::io_context for the event loop
class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    // Non-copyable, non-movable
    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;
    EventLoop(EventLoop&&) = delete;
    EventLoop& operator=(EventLoop&&) = delete;

    /// Get the underlying io_context
    asio::io_context& context() noexcept { return io_; }

    /// Run the event loop (blocking)
    void run();

    /// Run in a background thread
    void runInBackground();

    /// Stop the event loop
    void stop();

    /// Wait for background thread to finish
    void join();

    /// Check if running
    bool isRunning() const noexcept { return running_.load(); }

    /// Post work to the event loop
    template<typename Handler>
    void post(Handler&& handler) {
        asio::post(io_, std::forward<Handler>(handler));
    }

    /// Dispatch work (may execute immediately if in io_context thread)
    template<typename Handler>
    void dispatch(Handler&& handler) {
        asio::dispatch(io_, std::forward<Handler>(handler));
    }

private:
    asio::io_context io_;
    std::optional<asio::executor_work_guard<asio::io_context::executor_type>> workGuard_;
    std::atomic<bool> running_{false};
    std::thread backgroundThread_;
};

} // namespace shakyline

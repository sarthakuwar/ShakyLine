#include "shakyline/EventLoop.hpp"

namespace shakyline {

EventLoop::EventLoop()
    : workGuard_(asio::make_work_guard(io_)) {}

EventLoop::~EventLoop() {
    stop();
    join();
}

void EventLoop::run() {
    running_.store(true);
    io_.run();
    running_.store(false);
}

void EventLoop::runInBackground() {
    if (backgroundThread_.joinable()) {
        return;  // Already running
    }
    backgroundThread_ = std::thread([this]() {
        run();
    });
}

void EventLoop::stop() {
    workGuard_.reset();
    io_.stop();
}

void EventLoop::join() {
    if (backgroundThread_.joinable()) {
        backgroundThread_.join();
    }
}

} // namespace shakyline

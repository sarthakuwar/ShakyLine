#include "shakyline/Scheduler.hpp"

namespace shakyline {

Scheduler::Scheduler(asio::io_context& io)
    : io_(io) {}

Scheduler::TimerId Scheduler::schedule(std::chrono::milliseconds delay, Callback cb) {
    TimerId id = nextId_.fetch_add(1);
    
    auto timer = std::make_unique<asio::steady_timer>(io_);
    timer->expires_after(delay);
    
    asio::steady_timer* timerPtr = timer.get();
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        timers_[id] = std::move(timer);
    }
    
    timerPtr->async_wait([this, id, cb = std::move(cb)](const asio::error_code& ec) {
        // Remove timer from map
        {
            std::lock_guard<std::mutex> lock(mutex_);
            timers_.erase(id);
        }
        
        // Only execute if not cancelled
        if (ec != asio::error::operation_aborted) {
            cb();
        }
    });
    
    return id;
}

bool Scheduler::cancel(TimerId id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = timers_.find(id);
    if (it == timers_.end()) {
        return false;
    }
    
    it->second->cancel();
    timers_.erase(it);
    return true;
}

void Scheduler::cancelAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [id, timer] : timers_) {
        timer->cancel();
    }
    timers_.clear();
}

std::size_t Scheduler::activeCount() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(mutex_));
    return timers_.size();
}

} // namespace shakyline

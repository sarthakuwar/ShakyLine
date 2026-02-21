#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <queue>
#include <vector>

namespace shakyline {

/// Delayed packet entry with profile version binding
struct DelayedPacket {
    std::vector<uint8_t> payload;
    std::chrono::steady_clock::time_point releaseTime;
    uint64_t packetSeq;
    uint32_t profileVersion;
    uint8_t direction;  // 0 = client->server, 1 = server->client

    bool operator>(const DelayedPacket& other) const noexcept {
        return releaseTime > other.releaseTime;
    }
};

/// Time-ordered delay queue for fault injection
/// Profile-version-bound: packets keep the profile active at read time
class DelayQueue {
public:
    static constexpr std::size_t MAX_BYTES = 2 * 1024 * 1024;  // 2MB limit

    DelayQueue() = default;

    // Non-copyable, movable
    DelayQueue(const DelayQueue&) = delete;
    DelayQueue& operator=(const DelayQueue&) = delete;
    DelayQueue(DelayQueue&&) noexcept = default;
    DelayQueue& operator=(DelayQueue&&) noexcept = default;

    /// Add a delayed packet
    /// Returns false if queue is full (oldest will be dropped)
    bool push(std::vector<uint8_t> payload,
              std::chrono::steady_clock::time_point releaseTime,
              uint64_t packetSeq,
              uint32_t profileVersion,
              uint8_t direction);

    /// Check if any packets are ready to release
    bool hasReady(std::chrono::steady_clock::time_point now) const noexcept;

    /// Pop the next ready packet (returns empty if none ready)
    std::optional<DelayedPacket> popReady(std::chrono::steady_clock::time_point now);

    /// Get time until next packet release (for scheduling)
    std::optional<std::chrono::steady_clock::time_point> nextReleaseTime() const noexcept;

    /// Current total bytes queued
    std::size_t totalBytes() const noexcept { return totalBytes_; }

    /// Number of packets queued
    std::size_t size() const noexcept { return queue_.size(); }

    /// Is queue empty?
    bool empty() const noexcept { return queue_.empty(); }

    /// Clear all delayed packets
    void clear();

private:
    std::priority_queue<DelayedPacket, 
                        std::vector<DelayedPacket>,
                        std::greater<DelayedPacket>> queue_;
    std::size_t totalBytes_ = 0;

    void dropOldest();
};

} // namespace shakyline

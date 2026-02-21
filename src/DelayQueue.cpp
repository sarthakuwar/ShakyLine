#include "shakyline/DelayQueue.hpp"
#include <optional>

namespace shakyline {

bool DelayQueue::push(std::vector<uint8_t> payload,
                      std::chrono::steady_clock::time_point releaseTime,
                      uint64_t packetSeq,
                      uint32_t profileVersion,
                      uint8_t direction) {
    std::size_t payloadSize = payload.size();
    
    // Drop oldest if exceeding limit
    while (totalBytes_ + payloadSize > MAX_BYTES && !queue_.empty()) {
        dropOldest();
    }

    // Still can't fit (single packet too large)
    if (payloadSize > MAX_BYTES) {
        return false;
    }

    DelayedPacket pkt{
        std::move(payload),
        releaseTime,
        packetSeq,
        profileVersion,
        direction
    };

    totalBytes_ += payloadSize;
    queue_.push(std::move(pkt));
    return true;
}

bool DelayQueue::hasReady(std::chrono::steady_clock::time_point now) const noexcept {
    if (queue_.empty()) return false;
    return queue_.top().releaseTime <= now;
}

std::optional<DelayedPacket> DelayQueue::popReady(std::chrono::steady_clock::time_point now) {
    if (queue_.empty() || queue_.top().releaseTime > now) {
        return std::nullopt;
    }

    // Priority queue doesn't have a non-const top + pop combo
    // We need to copy here (or use a different container)
    DelayedPacket pkt = std::move(const_cast<DelayedPacket&>(queue_.top()));
    queue_.pop();
    totalBytes_ -= pkt.payload.size();
    return pkt;
}

std::optional<std::chrono::steady_clock::time_point> 
DelayQueue::nextReleaseTime() const noexcept {
    if (queue_.empty()) return std::nullopt;
    return queue_.top().releaseTime;
}

void DelayQueue::clear() {
    while (!queue_.empty()) {
        queue_.pop();
    }
    totalBytes_ = 0;
}

void DelayQueue::dropOldest() {
    if (queue_.empty()) return;
    
    // Priority queue orders by release time, so we drop the soonest (top)
    // This is the oldest in terms of when it was supposed to be sent
    totalBytes_ -= queue_.top().payload.size();
    queue_.pop();
}

} // namespace shakyline

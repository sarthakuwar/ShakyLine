#pragma once

#include <atomic>
#include <array>
#include <cstdint>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace shakyline {

/// Thread-local histogram bucket for lock-free accumulation
struct HistogramBucket {
    std::atomic<uint64_t> count{0};
    uint64_t upperBound;
};

/// Lock-free histogram with fixed buckets
class Histogram {
public:
    Histogram(std::string name, std::vector<uint64_t> bucketBounds);

    void observe(uint64_t value);

    std::string renderPrometheus(const std::string& prefix) const;

private:
    std::string name_;
    std::vector<std::pair<uint64_t, std::atomic<uint64_t>>> buckets_;
    std::atomic<uint64_t> sum_{0};
    std::atomic<uint64_t> count_{0};
};

/// Prometheus-compatible metrics registry
class MetricsRegistry {
public:
    MetricsRegistry();

    // Counters
    void incrementActiveSessions() { activeSessions_.fetch_add(1); }
    void decrementActiveSessions() { activeSessions_.fetch_sub(1); }
    void addBytesUpstream(uint64_t bytes) { bytesUpstream_.fetch_add(bytes); }
    void addBytesDownstream(uint64_t bytes) { bytesDownstream_.fetch_add(bytes); }
    void incrementPacketsDropped() { packetsDropped_.fetch_add(1); }
    void incrementPacketsDelayed() { packetsDelayed_.fetch_add(1); }
    void incrementStallEvents() { stallEvents_.fetch_add(1); }
    void incrementHalfCloseEvents() { halfCloseEvents_.fetch_add(1); }
    void incrementConnectFailures() { connectFailures_.fetch_add(1); }

    // Histogram observations
    void observeLatencyInjected(uint64_t ms);
    void observeSessionLifetime(uint64_t seconds);
    void observeBufferOccupancy(uint64_t bytes);

    // Render all metrics in Prometheus text format
    std::string renderPrometheus() const;

private:
    // Counters (atomic for thread safety)
    std::atomic<uint64_t> activeSessions_{0};
    std::atomic<uint64_t> bytesUpstream_{0};
    std::atomic<uint64_t> bytesDownstream_{0};
    std::atomic<uint64_t> packetsDropped_{0};
    std::atomic<uint64_t> packetsDelayed_{0};
    std::atomic<uint64_t> stallEvents_{0};
    std::atomic<uint64_t> halfCloseEvents_{0};
    std::atomic<uint64_t> connectFailures_{0};

    // Histograms
    std::unique_ptr<Histogram> latencyHist_;
    std::unique_ptr<Histogram> lifetimeHist_;
    std::unique_ptr<Histogram> bufferHist_;
};

/// Global metrics instance
MetricsRegistry& globalMetrics();

} // namespace shakyline

#include "shakyline/MetricsRegistry.hpp"
#include <memory>

namespace shakyline {

Histogram::Histogram(std::string name, std::vector<uint64_t> bucketBounds)
    : name_(std::move(name)) {
    for (auto bound : bucketBounds) {
        buckets_.emplace_back(bound, std::atomic<uint64_t>{0});
    }
}

void Histogram::observe(uint64_t value) {
    sum_.fetch_add(value);
    count_.fetch_add(1);
    
    for (auto& [bound, count] : buckets_) {
        if (value <= bound) {
            count.fetch_add(1);
        }
    }
}

std::string Histogram::renderPrometheus(const std::string& prefix) const {
    std::ostringstream oss;
    
    for (const auto& [bound, count] : buckets_) {
        oss << prefix << "_" << name_ << "_bucket{le=\"" << bound << "\"} "
            << count.load() << "\n";
    }
    oss << prefix << "_" << name_ << "_bucket{le=\"+Inf\"} " 
        << count_.load() << "\n";
    oss << prefix << "_" << name_ << "_sum " << sum_.load() << "\n";
    oss << prefix << "_" << name_ << "_count " << count_.load() << "\n";
    
    return oss.str();
}

MetricsRegistry::MetricsRegistry() {
    latencyHist_ = std::make_unique<Histogram>(
        "latency_injected_ms",
        std::vector<uint64_t>{10, 50, 100, 500, 1000, 5000, 30000}
    );
    
    lifetimeHist_ = std::make_unique<Histogram>(
        "session_lifetime_seconds",
        std::vector<uint64_t>{1, 5, 30, 60, 300, 3600}
    );
    
    bufferHist_ = std::make_unique<Histogram>(
        "buffer_occupancy_bytes",
        std::vector<uint64_t>{1024, 8192, 32768, 65536, 262144, 1048576}
    );
}

void MetricsRegistry::observeLatencyInjected(uint64_t ms) {
    latencyHist_->observe(ms);
}

void MetricsRegistry::observeSessionLifetime(uint64_t seconds) {
    lifetimeHist_->observe(seconds);
}

void MetricsRegistry::observeBufferOccupancy(uint64_t bytes) {
    bufferHist_->observe(bytes);
}

std::string MetricsRegistry::renderPrometheus() const {
    std::ostringstream oss;
    
    oss << "# HELP shakyline_active_sessions Current number of active sessions\n";
    oss << "# TYPE shakyline_active_sessions gauge\n";
    oss << "shakyline_active_sessions " << activeSessions_.load() << "\n\n";
    
    oss << "# HELP shakyline_bytes_upstream_total Total bytes forwarded upstream\n";
    oss << "# TYPE shakyline_bytes_upstream_total counter\n";
    oss << "shakyline_bytes_upstream_total " << bytesUpstream_.load() << "\n\n";
    
    oss << "# HELP shakyline_bytes_downstream_total Total bytes forwarded downstream\n";
    oss << "# TYPE shakyline_bytes_downstream_total counter\n";
    oss << "shakyline_bytes_downstream_total " << bytesDownstream_.load() << "\n\n";
    
    oss << "# HELP shakyline_packets_dropped_total Total packets dropped\n";
    oss << "# TYPE shakyline_packets_dropped_total counter\n";
    oss << "shakyline_packets_dropped_total " << packetsDropped_.load() << "\n\n";
    
    oss << "# HELP shakyline_packets_delayed_total Total packets delayed\n";
    oss << "# TYPE shakyline_packets_delayed_total counter\n";
    oss << "shakyline_packets_delayed_total " << packetsDelayed_.load() << "\n\n";
    
    oss << "# HELP shakyline_stall_events_total Total stall events\n";
    oss << "# TYPE shakyline_stall_events_total counter\n";
    oss << "shakyline_stall_events_total " << stallEvents_.load() << "\n\n";
    
    oss << "# HELP shakyline_half_close_events_total Total half-close events\n";
    oss << "# TYPE shakyline_half_close_events_total counter\n";
    oss << "shakyline_half_close_events_total " << halfCloseEvents_.load() << "\n\n";
    
    oss << "# HELP shakyline_connect_failures_total Total upstream connect failures\n";
    oss << "# TYPE shakyline_connect_failures_total counter\n";
    oss << "shakyline_connect_failures_total " << connectFailures_.load() << "\n\n";
    
    oss << "# HELP shakyline Latency injection histogram\n";
    oss << "# TYPE shakyline_latency_injected_ms histogram\n";
    oss << latencyHist_->renderPrometheus("shakyline") << "\n";
    
    oss << "# HELP shakyline Session lifetime histogram\n";
    oss << "# TYPE shakyline_session_lifetime_seconds histogram\n";
    oss << lifetimeHist_->renderPrometheus("shakyline") << "\n";
    
    oss << "# HELP shakyline Buffer occupancy histogram\n";
    oss << "# TYPE shakyline_buffer_occupancy_bytes histogram\n";
    oss << bufferHist_->renderPrometheus("shakyline") << "\n";
    
    return oss.str();
}

MetricsRegistry& globalMetrics() {
    static MetricsRegistry instance;
    return instance;
}

} // namespace shakyline

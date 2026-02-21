#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

namespace shakyline {

/// Directional anomaly profile (one direction of traffic)
struct DirectionalProfile {
    uint32_t latencyMs = 0;
    uint32_t jitterMs = 0;
    uint32_t throttleKbps = 0;
    float dropRate = 0.0f;
    float stallProbability = 0.0f;
    float corruptRate = 0.0f;
    float reorderRate = 0.0f;
    float halfCloseRate = 0.0f;
};

/// Complete anomaly profile with bidirectional settings
struct AnomalyProfile {
    DirectionalProfile clientToServer;
    DirectionalProfile serverToClient;
    uint32_t version = 0;
};

/// Configuration limits
struct ConfigLimits {
    static constexpr uint32_t MAX_LATENCY_MS = 30000;
    static constexpr uint32_t MAX_JITTER_MS = 10000;
    static constexpr uint32_t MAX_THROTTLE_KBPS = 1000000;  // 1 Gbps
    static constexpr float MAX_RATE = 1.0f;
    static constexpr std::size_t MAX_BUFFER_BYTES = 4 * 1024 * 1024;  // 4MB
    static constexpr std::size_t MAX_SESSIONS = 10000;
    static constexpr std::size_t SOFT_LIMIT_PERCENT = 90;
    static constexpr int CONFIG_UPDATE_RATE_LIMIT = 10;  // per second
};

/// Server configuration
struct ServerConfig {
    std::string listenHost = "0.0.0.0";
    uint16_t listenPort = 8080;
    std::string upstreamHost = "127.0.0.1";
    uint16_t upstreamPort = 9000;
    uint16_t controlPort = 9090;
    uint64_t globalSeed = 0;
    
    std::chrono::milliseconds connectTimeout{5000};
    std::chrono::milliseconds idleTimeout{60000};
    std::chrono::milliseconds stallTimeout{30000};
    std::chrono::milliseconds minLingerTimeout{2000};
    std::chrono::milliseconds maxLingerTimeout{120000};
};

/// Thread-safe configuration manager
class ConfigManager {
public:
    ConfigManager() = default;

    /// Get a profile by name (returns default if not found)
    AnomalyProfile getProfile(const std::string& name) const;

    /// Set a profile (returns new version)
    uint32_t setProfile(const std::string& name, AnomalyProfile profile);

    /// Delete a profile
    bool deleteProfile(const std::string& name);

    /// Check rate limit for config updates
    bool checkRateLimit();

    /// Get server config
    const ServerConfig& serverConfig() const { return serverConfig_; }
    ServerConfig& serverConfig() { return serverConfig_; }

    /// Validate a directional profile (clamps values to limits)
    static DirectionalProfile validate(const DirectionalProfile& profile);

private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, AnomalyProfile> profiles_;
    std::atomic<uint32_t> nextVersion_{1};
    
    std::mutex rateMutex_;
    std::chrono::steady_clock::time_point lastUpdate_;
    int updateCount_ = 0;
    
    ServerConfig serverConfig_;
};

} // namespace shakyline

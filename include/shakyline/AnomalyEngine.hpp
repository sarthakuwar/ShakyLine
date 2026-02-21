#pragma once

#include "shakyline/Config.hpp"
#include "shakyline/DeterministicRng.hpp"
#include <chrono>
#include <cstdint>
#include <span>
#include <vector>

namespace shakyline {

/// Direction of traffic flow
enum class Direction : uint8_t {
    ClientToServer = 0,
    ServerToClient = 1
};

/// Anomaly decision result
struct AnomalyDecision {
    enum class Action {
        Forward,        // Send immediately
        Drop,           // Discard packet
        Delay,          // Queue with delay
        Throttle,       // Apply bandwidth limit
        Corrupt,        // Modify data
        Reorder,        // Queue for later
        Stall,          // Stop processing
        HalfClose       // Initiate half-close
    };

    Action action = Action::Forward;
    uint32_t delayMs = 0;
    uint32_t throttleBytesPerSec = 0;
    std::size_t corruptOffset = 0;
    uint8_t corruptMask = 0;
};

/// Stateless anomaly decision engine
/// Uses deterministic RNG for reproducible fault injection
class AnomalyEngine {
public:
    explicit AnomalyEngine(uint64_t globalSeed) : globalSeed_(globalSeed) {}

    /// Make an anomaly decision for a packet
    AnomalyDecision decide(
        std::span<const uint8_t> data,
        Direction direction,
        uint64_t sessionId,
        uint64_t packetSeq,
        const AnomalyProfile& profile
    ) const;

    /// Apply corruption to data (modifies in place)
    static void applyCorruption(
        std::span<uint8_t> data,
        std::size_t offset,
        uint8_t mask
    );

    /// Get the global seed
    uint64_t globalSeed() const noexcept { return globalSeed_; }

private:
    uint64_t globalSeed_;

    const DirectionalProfile& getDirectionalProfile(
        Direction direction,
        const AnomalyProfile& profile
    ) const;
};

} // namespace shakyline

#pragma once

#include <cstdint>

namespace shakyline {

/// SplitMix64 - Stateless deterministic RNG for fault decisions
/// Returns a pseudo-random value based on input seed
/// Same input always produces same output (no mutable state)
class DeterministicRng {
public:
    /// Generate a random uint64 from a seed
    static uint64_t splitmix64(uint64_t seed) noexcept {
        uint64_t z = seed + 0x9e3779b97f4a7c15ULL;
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    }

    /// Hash multiple values into a single seed
    static uint64_t hash(uint64_t globalSeed, uint64_t sessionId, 
                         uint64_t packetSeq, uint8_t direction) noexcept {
        uint64_t h = globalSeed;
        h = splitmix64(h ^ sessionId);
        h = splitmix64(h ^ packetSeq);
        h = splitmix64(h ^ direction);
        return h;
    }

    /// Generate a float in [0.0, 1.0) from components
    static float uniform(uint64_t globalSeed, uint64_t sessionId,
                        uint64_t packetSeq, uint8_t direction) noexcept {
        uint64_t h = hash(globalSeed, sessionId, packetSeq, direction);
        // Use upper 23 bits for float mantissa
        return static_cast<float>(h >> 40) / static_cast<float>(1ULL << 24);
    }

    /// Generate an integer in [0, max) from components
    static uint32_t uniformInt(uint64_t globalSeed, uint64_t sessionId,
                               uint64_t packetSeq, uint8_t direction,
                               uint32_t max) noexcept {
        if (max == 0) return 0;
        uint64_t h = hash(globalSeed, sessionId, packetSeq, direction);
        return static_cast<uint32_t>(h % max);
    }
};

} // namespace shakyline

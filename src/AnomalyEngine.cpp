#include "shakyline/AnomalyEngine.hpp"

namespace shakyline {

AnomalyDecision AnomalyEngine::decide(
    std::span<const uint8_t> data,
    Direction direction,
    uint64_t sessionId,
    uint64_t packetSeq,
    const AnomalyProfile& profile
) const {
    AnomalyDecision decision;
    const auto& dirProfile = getDirectionalProfile(direction, profile);
    uint8_t dir = static_cast<uint8_t>(direction);

    // Use different sub-seeds for each decision type
    // This ensures decisions are independent but deterministic
    
    // Check drop first (highest priority fault)
    if (dirProfile.dropRate > 0.0f) {
        float dropRoll = DeterministicRng::uniform(globalSeed_, sessionId, 
                                                    packetSeq * 7 + 1, dir);
        if (dropRoll < dirProfile.dropRate) {
            decision.action = AnomalyDecision::Action::Drop;
            return decision;
        }
    }

    // Check half-close
    if (dirProfile.halfCloseRate > 0.0f) {
        float hcRoll = DeterministicRng::uniform(globalSeed_, sessionId,
                                                  packetSeq * 7 + 2, dir);
        if (hcRoll < dirProfile.halfCloseRate) {
            decision.action = AnomalyDecision::Action::HalfClose;
            return decision;
        }
    }

    // Check stall
    if (dirProfile.stallProbability > 0.0f) {
        float stallRoll = DeterministicRng::uniform(globalSeed_, sessionId,
                                                     packetSeq * 7 + 3, dir);
        if (stallRoll < dirProfile.stallProbability) {
            decision.action = AnomalyDecision::Action::Stall;
            return decision;
        }
    }

    // Check corruption
    if (dirProfile.corruptRate > 0.0f && !data.empty()) {
        float corruptRoll = DeterministicRng::uniform(globalSeed_, sessionId,
                                                       packetSeq * 7 + 4, dir);
        if (corruptRoll < dirProfile.corruptRate) {
            decision.action = AnomalyDecision::Action::Corrupt;
            decision.corruptOffset = DeterministicRng::uniformInt(
                globalSeed_, sessionId, packetSeq * 7 + 5, dir, 
                static_cast<uint32_t>(data.size())
            );
            decision.corruptMask = static_cast<uint8_t>(
                DeterministicRng::uniformInt(globalSeed_, sessionId, 
                                             packetSeq * 7 + 6, dir, 256)
            );
            // Continue to also apply delay if configured
        }
    }

    // Check delay/jitter
    if (dirProfile.latencyMs > 0 || dirProfile.jitterMs > 0) {
        uint32_t baseLatency = dirProfile.latencyMs;
        if (dirProfile.jitterMs > 0) {
            int32_t jitter = static_cast<int32_t>(
                DeterministicRng::uniformInt(globalSeed_, sessionId,
                                             packetSeq * 7 + 7, dir, 
                                             dirProfile.jitterMs * 2 + 1)
            ) - static_cast<int32_t>(dirProfile.jitterMs);
            baseLatency = static_cast<uint32_t>(std::max(0, 
                static_cast<int32_t>(baseLatency) + jitter));
        }
        
        if (baseLatency > 0) {
            if (decision.action == AnomalyDecision::Action::Forward) {
                decision.action = AnomalyDecision::Action::Delay;
            }
            decision.delayMs = baseLatency;
        }
    }

    // Check throttle
    if (dirProfile.throttleKbps > 0) {
        if (decision.action == AnomalyDecision::Action::Forward) {
            decision.action = AnomalyDecision::Action::Throttle;
        }
        decision.throttleBytesPerSec = dirProfile.throttleKbps * 1024 / 8;
    }

    return decision;
}

void AnomalyEngine::applyCorruption(
    std::span<uint8_t> data,
    std::size_t offset,
    uint8_t mask
) {
    if (offset < data.size()) {
        data[offset] ^= mask;
    }
}

const DirectionalProfile& AnomalyEngine::getDirectionalProfile(
    Direction direction,
    const AnomalyProfile& profile
) const {
    return (direction == Direction::ClientToServer) 
           ? profile.clientToServer 
           : profile.serverToClient;
}

} // namespace shakyline

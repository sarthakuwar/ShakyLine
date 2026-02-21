#include "shakyline/Config.hpp"
#include <algorithm>

namespace shakyline {

AnomalyProfile ConfigManager::getProfile(const std::string& name) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    auto it = profiles_.find(name);
    if (it == profiles_.end()) {
        return AnomalyProfile{};  // Return default profile
    }
    return it->second;
}

uint32_t ConfigManager::setProfile(const std::string& name, AnomalyProfile profile) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    
    profile.clientToServer = validate(profile.clientToServer);
    profile.serverToClient = validate(profile.serverToClient);
    profile.version = nextVersion_.fetch_add(1);
    
    profiles_[name] = profile;
    return profile.version;
}

bool ConfigManager::deleteProfile(const std::string& name) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    return profiles_.erase(name) > 0;
}

bool ConfigManager::checkRateLimit() {
    std::lock_guard<std::mutex> lock(rateMutex_);
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate_);
    
    if (elapsed.count() >= 1) {
        lastUpdate_ = now;
        updateCount_ = 1;
        return true;
    }
    
    if (updateCount_ >= ConfigLimits::CONFIG_UPDATE_RATE_LIMIT) {
        return false;
    }
    
    ++updateCount_;
    return true;
}

DirectionalProfile ConfigManager::validate(const DirectionalProfile& profile) {
    DirectionalProfile validated = profile;
    
    validated.latencyMs = std::min(validated.latencyMs, ConfigLimits::MAX_LATENCY_MS);
    validated.jitterMs = std::min(validated.jitterMs, ConfigLimits::MAX_JITTER_MS);
    validated.throttleKbps = std::min(validated.throttleKbps, ConfigLimits::MAX_THROTTLE_KBPS);
    validated.dropRate = std::clamp(validated.dropRate, 0.0f, ConfigLimits::MAX_RATE);
    validated.stallProbability = std::clamp(validated.stallProbability, 0.0f, ConfigLimits::MAX_RATE);
    validated.corruptRate = std::clamp(validated.corruptRate, 0.0f, ConfigLimits::MAX_RATE);
    validated.reorderRate = std::clamp(validated.reorderRate, 0.0f, ConfigLimits::MAX_RATE);
    validated.halfCloseRate = std::clamp(validated.halfCloseRate, 0.0f, ConfigLimits::MAX_RATE);
    
    return validated;
}

} // namespace shakyline

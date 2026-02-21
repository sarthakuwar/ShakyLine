#include "shakyline/SessionManager.hpp"
#include "shakyline/Logger.hpp"
#include "shakyline/MetricsRegistry.hpp"

namespace shakyline {

SessionManager::Ptr SessionManager::create(
    asio::io_context& io,
    Scheduler& scheduler,
    const AnomalyEngine& engine,
    ConfigManager& config
) {
    return Ptr(new SessionManager(io, scheduler, engine, config));
}

SessionManager::SessionManager(
    asio::io_context& io,
    Scheduler& scheduler,
    const AnomalyEngine& engine,
    ConfigManager& config
) : io_(io)
  , scheduler_(scheduler)
  , engine_(engine)
  , config_(config)
{}

SessionManager::~SessionManager() {
    forceCloseAll();
}

Session::Ptr SessionManager::createSession(Socket clientSocket) {
    if (!tryAdmit()) {
        globalLogger().warn(0, 0, "admission_denied", "", 
                           "count=" + std::to_string(sessionCount()));
        return nullptr;
    }

    uint64_t sessionId = nextSessionId_.fetch_add(1);
    
    auto session = Session::create(
        io_, std::move(clientSocket), weak_from_this(),
        scheduler_, engine_, config_, sessionId
    );

    {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_[sessionId] = session;
    }

    // Post start to session's strand (post-construction activation)
    asio::post(session->strand(), [session, upstream = upstreamEndpoint_]() {
        session->start(upstream);
    });

    return session;
}

void SessionManager::removeSession(uint64_t sessionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(sessionId);
}

Session::Ptr SessionManager::getSession(uint64_t sessionId) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(sessionId);
    if (it == sessions_.end()) return nullptr;
    return it->second;
}

std::size_t SessionManager::sessionCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_.size();
}

void SessionManager::shutdownAll() {
    std::vector<Session::Ptr> toShutdown;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [id, session] : sessions_) {
            toShutdown.push_back(session);
        }
    }

    for (auto& session : toShutdown) {
        asio::post(session->strand(), [session]() {
            session->initiateShutdown();
        });
    }
}

void SessionManager::forceCloseAll() {
    std::vector<Session::Ptr> toClose;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& [id, session] : sessions_) {
            toClose.push_back(session);
        }
    }

    for (auto& session : toClose) {
        asio::post(session->strand(), [session]() {
            session->forceClose();
        });
    }
}

Session::Ptr SessionManager::findOldestIdle() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Session::Ptr oldest = nullptr;
    std::chrono::steady_clock::duration maxIdle{0};

    for (auto& [id, session] : sessions_) {
        auto idle = session->idleTime();
        if (idle > maxIdle) {
            maxIdle = idle;
            oldest = session;
        }
    }

    return oldest;
}

std::vector<uint64_t> SessionManager::getSessionIds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<uint64_t> ids;
    ids.reserve(sessions_.size());
    for (const auto& [id, _] : sessions_) {
        ids.push_back(id);
    }
    return ids;
}

bool SessionManager::canAccept() const {
    return sessionCount() < ConfigLimits::MAX_SESSIONS;
}

bool SessionManager::tryAdmit() {
    std::size_t count = sessionCount();
    std::size_t softLimit = ConfigLimits::MAX_SESSIONS * 
                           ConfigLimits::SOFT_LIMIT_PERCENT / 100;
    
    if (count < softLimit) {
        return true;
    }
    
    if (count >= ConfigLimits::MAX_SESSIONS) {
        // At hard limit - try to shed oldest
        shedOldestIdle();
        return sessionCount() < ConfigLimits::MAX_SESSIONS;
    }
    
    // Between soft and hard limit - probabilistic admission
    float probability = 1.0f - static_cast<float>(count - softLimit) / 
                                (ConfigLimits::MAX_SESSIONS - softLimit);
    
    // Simple random check (not cryptographically secure, but fine for this)
    float roll = static_cast<float>(std::rand()) / RAND_MAX;
    return roll < probability;
}

void SessionManager::shedOldestIdle() {
    auto oldest = findOldestIdle();
    if (oldest) {
        globalLogger().info(oldest->id(), 0, "session_shed", "", "reason=admission");
        asio::post(oldest->strand(), [oldest]() {
            oldest->forceClose();
        });
    }
}

} // namespace shakyline

#pragma once

#include "shakyline/AnomalyEngine.hpp"
#include "shakyline/Config.hpp"
#include "shakyline/EventLoop.hpp"
#include "shakyline/Scheduler.hpp"
#include "shakyline/Session.hpp"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace shakyline {

/// Session ownership and admission control
class SessionManager : public std::enable_shared_from_this<SessionManager> {
public:
    using Ptr = std::shared_ptr<SessionManager>;

    static Ptr create(
        asio::io_context& io,
        Scheduler& scheduler,
        const AnomalyEngine& engine,
        ConfigManager& config
    );

    ~SessionManager();

    // Non-copyable
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

    /// Create and register a new session
    /// Returns nullptr if admission denied
    Session::Ptr createSession(Socket clientSocket);

    /// Remove a session (called by session on close)
    void removeSession(uint64_t sessionId);

    /// Get session by ID
    Session::Ptr getSession(uint64_t sessionId);

    /// Current session count
    std::size_t sessionCount() const;

    /// Initiate graceful shutdown of all sessions
    void shutdownAll();

    /// Force close all sessions
    void forceCloseAll();

    /// Find oldest idle session for shedding
    Session::Ptr findOldestIdle();

    /// Get list of active session IDs
    std::vector<uint64_t> getSessionIds() const;

    /// Check if accepting new connections
    bool canAccept() const;

    /// Get the upstream endpoint
    void setUpstreamEndpoint(const asio::ip::tcp::endpoint& endpoint) {
        upstreamEndpoint_ = endpoint;
    }
    const asio::ip::tcp::endpoint& upstreamEndpoint() const { 
        return upstreamEndpoint_; 
    }

private:
    SessionManager(
        asio::io_context& io,
        Scheduler& scheduler,
        const AnomalyEngine& engine,
        ConfigManager& config
    );

    bool tryAdmit();
    void shedOldestIdle();

    asio::io_context& io_;
    Scheduler& scheduler_;
    const AnomalyEngine& engine_;
    ConfigManager& config_;
    asio::ip::tcp::endpoint upstreamEndpoint_;

    mutable std::mutex mutex_;
    std::unordered_map<uint64_t, Session::Ptr> sessions_;
    std::atomic<uint64_t> nextSessionId_{1};
};

} // namespace shakyline

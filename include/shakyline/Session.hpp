#pragma once

#include "shakyline/AnomalyEngine.hpp"
#include "shakyline/Buffer.hpp"
#include "shakyline/Config.hpp"
#include "shakyline/DelayQueue.hpp"
#include "shakyline/Scheduler.hpp"
#include "shakyline/Socket.hpp"

#include <asio.hpp>
#include <chrono>
#include <cstdint>
#include <memory>

namespace shakyline {

class SessionManager;

/// Session state for 4-way half-close tracking
struct ChannelState {
    bool clientReadOpen = true;
    bool clientWriteOpen = true;
    bool serverReadOpen = true;
    bool serverWriteOpen = true;

    bool isFullyClosed() const noexcept {
        return !clientReadOpen && !clientWriteOpen && 
               !serverReadOpen && !serverWriteOpen;
    }
};

/// Upstream connection state
enum class UpstreamState {
    Connecting,
    Connected,
    Failed
};

/// Session: bidirectional proxy connection with strand serialization
class Session : public std::enable_shared_from_this<Session> {
public:
    using Ptr = std::shared_ptr<Session>;

    /// Factory method - creates session but does NOT start it
    static Ptr create(
        asio::io_context& io,
        Socket clientSocket,
        std::weak_ptr<SessionManager> manager,
        Scheduler& scheduler,
        const AnomalyEngine& engine,
        ConfigManager& config,
        uint64_t sessionId
    );

    ~Session();

    // Non-copyable, non-movable
    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    /// Start the session (must be called via strand post after construction)
    void start(const asio::ip::tcp::endpoint& upstream);

    /// Initiate graceful shutdown
    void initiateShutdown();

    /// Force close with RST
    void forceClose();

    /// Get session ID
    uint64_t id() const noexcept { return sessionId_; }

    /// Get strand for external posts
    asio::strand<asio::io_context::executor_type>& strand() { return strand_; }

    /// Check if session is closed
    bool isClosed() const noexcept { return channels_.isFullyClosed(); }

    /// Get idle time
    std::chrono::steady_clock::duration idleTime() const {
        return std::chrono::steady_clock::now() - lastActivity_;
    }

private:
    Session(
        asio::io_context& io,
        Socket clientSocket,
        std::weak_ptr<SessionManager> manager,
        Scheduler& scheduler,
        const AnomalyEngine& engine,
        ConfigManager& config,
        uint64_t sessionId
    );

    // --- Event handlers ---
    void onConnectComplete(const asio::error_code& ec);
    void onClientRead(const asio::error_code& ec, std::size_t bytesRead);
    void onServerRead(const asio::error_code& ec, std::size_t bytesRead);
    void onClientWrite(const asio::error_code& ec, std::size_t bytesWritten);
    void onServerWrite(const asio::error_code& ec, std::size_t bytesWritten);
    void onDelayExpired();
    void onConnectTimeout();
    void onIdleTimeout();
    void onStallTimeout();

    // --- I/O helpers ---
    void startClientRead();
    void startServerRead();
    void startClientWrite();
    void startServerWrite();
    void processClientData(std::span<uint8_t> data);
    void processServerData(std::span<uint8_t> data);
    void flushDelayQueues();
    void scheduleDelayFlush();

    // --- State management ---
    void closeClientRead();
    void closeClientWrite();
    void closeServerRead();
    void closeServerWrite();
    void checkFullyClosed();
    void resetIdleTimer();
    void recordActivity();

    // --- Adaptive budget ---
    std::size_t calculateBudget() const;

    // Core state
    asio::strand<asio::io_context::executor_type> strand_;
    Socket clientSocket_;
    Socket serverSocket_;
    std::weak_ptr<SessionManager> manager_;
    Scheduler& scheduler_;
    const AnomalyEngine& engine_;
    ConfigManager& config_;
    uint64_t sessionId_;

    // Channel state
    ChannelState channels_;
    UpstreamState upstreamState_ = UpstreamState::Connecting;

    // Buffers (Read → Delay → Write architecture)
    Buffer clientToServerBuf_;
    Buffer serverToClientBuf_;
    DelayQueue clientToServerDelay_;
    DelayQueue serverToClientDelay_;

    // Read buffers for async_read
    std::array<uint8_t, 32768> clientReadBuf_;
    std::array<uint8_t, 32768> serverReadBuf_;

    // Packet sequence counters
    uint64_t clientPktSeq_ = 0;
    uint64_t serverPktSeq_ = 0;

    // Profile snapshot
    AnomalyProfile currentProfile_;
    uint32_t profileVersion_ = 0;

    // Timers
    Scheduler::TimerId connectTimerId_ = 0;
    Scheduler::TimerId idleTimerId_ = 0;
    Scheduler::TimerId stallTimerId_ = 0;
    Scheduler::TimerId delayTimerId_ = 0;

    // Activity tracking
    std::chrono::steady_clock::time_point startTime_;
    std::chrono::steady_clock::time_point lastActivity_;

    // Write-in-progress flags
    bool clientWriteInProgress_ = false;
    bool serverWriteInProgress_ = false;

    // Read paused flags (for backpressure)
    bool clientReadPaused_ = false;
    bool serverReadPaused_ = false;
};

} // namespace shakyline

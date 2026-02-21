#include "shakyline/Session.hpp"
#include "shakyline/Logger.hpp"
#include "shakyline/MetricsRegistry.hpp"
#include "shakyline/SessionManager.hpp"

namespace shakyline {

Session::Ptr Session::create(
    asio::io_context& io,
    Socket clientSocket,
    std::weak_ptr<SessionManager> manager,
    Scheduler& scheduler,
    const AnomalyEngine& engine,
    ConfigManager& config,
    uint64_t sessionId
) {
    // Can't use make_shared with private constructor
    return Ptr(new Session(io, std::move(clientSocket), std::move(manager),
                           scheduler, engine, config, sessionId));
}

Session::Session(
    asio::io_context& io,
    Socket clientSocket,
    std::weak_ptr<SessionManager> manager,
    Scheduler& scheduler,
    const AnomalyEngine& engine,
    ConfigManager& config,
    uint64_t sessionId
) : strand_(asio::make_strand(io))
  , clientSocket_(std::move(clientSocket))
  , serverSocket_(Socket::create(io))
  , manager_(std::move(manager))
  , scheduler_(scheduler)
  , engine_(engine)
  , config_(config)
  , sessionId_(sessionId)
  , startTime_(std::chrono::steady_clock::now())
  , lastActivity_(startTime_)
{
    globalLogger().info(sessionId_, 0, "session_created");
    globalMetrics().incrementActiveSessions();
}

Session::~Session() {
    // Cancel any pending timers
    if (connectTimerId_) scheduler_.cancel(connectTimerId_);
    if (idleTimerId_) scheduler_.cancel(idleTimerId_);
    if (stallTimerId_) scheduler_.cancel(stallTimerId_);
    if (delayTimerId_) scheduler_.cancel(delayTimerId_);

    auto lifetime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - startTime_
    ).count();
    
    globalLogger().info(sessionId_, 0, "session_destroyed", "", 
                        "lifetime_s=" + std::to_string(lifetime));
    globalMetrics().decrementActiveSessions();
    globalMetrics().observeSessionLifetime(lifetime);
}

void Session::start(const asio::ip::tcp::endpoint& upstream) {
    // Fetch current profile
    currentProfile_ = config_.getProfile("default");
    profileVersion_ = currentProfile_.version;

    // Set connect timeout
    connectTimerId_ = scheduler_.scheduleGuarded(
        config_.serverConfig().connectTimeout,
        weak_from_this(),
        [](Session::Ptr self) { self->onConnectTimeout(); }
    );

    upstreamState_ = UpstreamState::Connecting;
    
    globalLogger().debug(sessionId_, 0, "connecting_upstream", "",
                         "host=" + upstream.address().to_string() + 
                         " port=" + std::to_string(upstream.port()));

    // Start upstream connection
    serverSocket_.asyncConnect(upstream,
        asio::bind_executor(strand_, 
            [self = shared_from_this()](const asio::error_code& ec) {
                self->onConnectComplete(ec);
            }
        )
    );
}

void Session::onConnectComplete(const asio::error_code& ec) {
    // Cancel connect timeout
    if (connectTimerId_) {
        scheduler_.cancel(connectTimerId_);
        connectTimerId_ = 0;
    }

    if (ec) {
        globalLogger().warn(sessionId_, 0, "connect_failed", "", 
                           "error=" + ec.message());
        globalMetrics().incrementConnectFailures();
        upstreamState_ = UpstreamState::Failed;
        forceClose();
        return;
    }

    upstreamState_ = UpstreamState::Connected;
    globalLogger().info(sessionId_, 0, "upstream_connected");

    // Configure sockets
    clientSocket_.setNoDelay(true);
    serverSocket_.setNoDelay(true);

    // Start idle timer
    resetIdleTimer();

    // Start reading from both ends
    startClientRead();
    startServerRead();
}

void Session::startClientRead() {
    if (!channels_.clientReadOpen || clientReadPaused_) return;

    clientSocket_.asyncRead(
        asio::buffer(clientReadBuf_),
        asio::bind_executor(strand_,
            [self = shared_from_this()](const asio::error_code& ec, std::size_t n) {
                self->onClientRead(ec, n);
            }
        )
    );
}

void Session::startServerRead() {
    if (!channels_.serverReadOpen || serverReadPaused_) return;

    serverSocket_.asyncRead(
        asio::buffer(serverReadBuf_),
        asio::bind_executor(strand_,
            [self = shared_from_this()](const asio::error_code& ec, std::size_t n) {
                self->onServerRead(ec, n);
            }
        )
    );
}

void Session::onClientRead(const asio::error_code& ec, std::size_t bytesRead) {
    if (ec) {
        if (ec == asio::error::eof || ec == asio::error::connection_reset) {
            globalLogger().debug(sessionId_, clientPktSeq_, "client_eof", "upstream");
            closeClientRead();
        } else if (ec != asio::error::operation_aborted) {
            globalLogger().warn(sessionId_, clientPktSeq_, "client_read_error", "upstream",
                               "error=" + ec.message());
            forceClose();
        }
        return;
    }

    recordActivity();
    ++clientPktSeq_;

    std::span<uint8_t> data(clientReadBuf_.data(), bytesRead);
    processClientData(data);

    // Check backpressure
    if (clientToServerBuf_.shouldPauseReading()) {
        clientReadPaused_ = true;
    } else {
        startClientRead();
    }
}

void Session::onServerRead(const asio::error_code& ec, std::size_t bytesRead) {
    if (ec) {
        if (ec == asio::error::eof || ec == asio::error::connection_reset) {
            globalLogger().debug(sessionId_, serverPktSeq_, "server_eof", "downstream");
            closeServerRead();
        } else if (ec != asio::error::operation_aborted) {
            globalLogger().warn(sessionId_, serverPktSeq_, "server_read_error", "downstream",
                               "error=" + ec.message());
            forceClose();
        }
        return;
    }

    recordActivity();
    ++serverPktSeq_;

    std::span<uint8_t> data(serverReadBuf_.data(), bytesRead);
    processServerData(data);

    // Check backpressure
    if (serverToClientBuf_.shouldPauseReading()) {
        serverReadPaused_ = true;
    } else {
        startServerRead();
    }
}

void Session::processClientData(std::span<uint8_t> data) {
    // Make anomaly decision
    auto decision = engine_.decide(data, Direction::ClientToServer,
                                    sessionId_, clientPktSeq_, currentProfile_);

    switch (decision.action) {
        case AnomalyDecision::Action::Drop:
            globalLogger().info(sessionId_, clientPktSeq_, "drop", "upstream",
                               "bytes=" + std::to_string(data.size()));
            globalMetrics().incrementPacketsDropped();
            return;

        case AnomalyDecision::Action::HalfClose:
            globalLogger().info(sessionId_, clientPktSeq_, "half_close", "upstream");
            globalMetrics().incrementHalfCloseEvents();
            closeServerWrite();
            return;

        case AnomalyDecision::Action::Stall:
            globalLogger().info(sessionId_, clientPktSeq_, "stall", "upstream");
            globalMetrics().incrementStallEvents();
            clientReadPaused_ = true;
            return;

        case AnomalyDecision::Action::Corrupt:
            AnomalyEngine::applyCorruption(data, decision.corruptOffset, decision.corruptMask);
            globalLogger().debug(sessionId_, clientPktSeq_, "corrupt", "upstream");
            break;

        default:
            break;
    }

    // Handle delay
    if (decision.delayMs > 0) {
        globalLogger().debug(sessionId_, clientPktSeq_, "delay", "upstream",
                            "ms=" + std::to_string(decision.delayMs));
        globalMetrics().incrementPacketsDelayed();
        globalMetrics().observeLatencyInjected(decision.delayMs);

        auto releaseTime = std::chrono::steady_clock::now() + 
                          std::chrono::milliseconds(decision.delayMs);
        
        std::vector<uint8_t> payload(data.begin(), data.end());
        clientToServerDelay_.push(std::move(payload), releaseTime,
                                  clientPktSeq_, profileVersion_, 0);
        
        // Schedule timer if needed
        if (delayTimerId_ == 0) {
            scheduleDelayFlush();
        }
    } else {
        // Forward immediately
        clientToServerBuf_.append(data.data(), data.size());
        globalMetrics().addBytesUpstream(data.size());
        startServerWrite();
    }
}

void Session::processServerData(std::span<uint8_t> data) {
    // Make anomaly decision
    auto decision = engine_.decide(data, Direction::ServerToClient,
                                    sessionId_, serverPktSeq_, currentProfile_);

    switch (decision.action) {
        case AnomalyDecision::Action::Drop:
            globalLogger().info(sessionId_, serverPktSeq_, "drop", "downstream",
                               "bytes=" + std::to_string(data.size()));
            globalMetrics().incrementPacketsDropped();
            return;

        case AnomalyDecision::Action::HalfClose:
            globalLogger().info(sessionId_, serverPktSeq_, "half_close", "downstream");
            globalMetrics().incrementHalfCloseEvents();
            closeClientWrite();
            return;

        case AnomalyDecision::Action::Stall:
            globalLogger().info(sessionId_, serverPktSeq_, "stall", "downstream");
            globalMetrics().incrementStallEvents();
            serverReadPaused_ = true;
            return;

        case AnomalyDecision::Action::Corrupt:
            AnomalyEngine::applyCorruption(data, decision.corruptOffset, decision.corruptMask);
            globalLogger().debug(sessionId_, serverPktSeq_, "corrupt", "downstream");
            break;

        default:
            break;
    }

    // Handle delay
    if (decision.delayMs > 0) {
        globalLogger().debug(sessionId_, serverPktSeq_, "delay", "downstream",
                            "ms=" + std::to_string(decision.delayMs));
        globalMetrics().incrementPacketsDelayed();
        globalMetrics().observeLatencyInjected(decision.delayMs);

        auto releaseTime = std::chrono::steady_clock::now() + 
                          std::chrono::milliseconds(decision.delayMs);
        
        std::vector<uint8_t> payload(data.begin(), data.end());
        serverToClientDelay_.push(std::move(payload), releaseTime,
                                  serverPktSeq_, profileVersion_, 1);
        
        if (delayTimerId_ == 0) {
            scheduleDelayFlush();
        }
    } else {
        // Forward immediately
        serverToClientBuf_.append(data.data(), data.size());
        globalMetrics().addBytesDownstream(data.size());
        startClientWrite();
    }
}

void Session::scheduleDelayFlush() {
    auto now = std::chrono::steady_clock::now();
    auto nextC2S = clientToServerDelay_.nextReleaseTime();
    auto nextS2C = serverToClientDelay_.nextReleaseTime();

    std::optional<std::chrono::steady_clock::time_point> next;
    if (nextC2S && nextS2C) {
        next = std::min(*nextC2S, *nextS2C);
    } else if (nextC2S) {
        next = nextC2S;
    } else if (nextS2C) {
        next = nextS2C;
    } else {
        return;
    }

    auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(*next - now);
    if (delay.count() < 0) delay = std::chrono::milliseconds(0);

    delayTimerId_ = scheduler_.scheduleGuarded(
        delay, weak_from_this(),
        [](Session::Ptr self) { self->onDelayExpired(); }
    );
}

void Session::onDelayExpired() {
    delayTimerId_ = 0;
    flushDelayQueues();
    scheduleDelayFlush();
}

void Session::flushDelayQueues() {
    auto now = std::chrono::steady_clock::now();

    // Flush client-to-server delays
    while (auto pkt = clientToServerDelay_.popReady(now)) {
        clientToServerBuf_.append(pkt->payload.data(), pkt->payload.size());
        globalMetrics().addBytesUpstream(pkt->payload.size());
    }
    if (clientToServerBuf_.readable() > 0 && !serverWriteInProgress_) {
        startServerWrite();
    }

    // Flush server-to-client delays
    while (auto pkt = serverToClientDelay_.popReady(now)) {
        serverToClientBuf_.append(pkt->payload.data(), pkt->payload.size());
        globalMetrics().addBytesDownstream(pkt->payload.size());
    }
    if (serverToClientBuf_.readable() > 0 && !clientWriteInProgress_) {
        startClientWrite();
    }
}

void Session::startClientWrite() {
    if (!channels_.clientWriteOpen || clientWriteInProgress_) return;
    if (serverToClientBuf_.empty()) return;

    clientWriteInProgress_ = true;
    clientSocket_.asyncWrite(
        serverToClientBuf_.dataToSend(),
        asio::bind_executor(strand_,
            [self = shared_from_this()](const asio::error_code& ec, std::size_t n) {
                self->onClientWrite(ec, n);
            }
        )
    );
}

void Session::startServerWrite() {
    if (!channels_.serverWriteOpen || serverWriteInProgress_) return;
    if (clientToServerBuf_.empty()) return;

    serverWriteInProgress_ = true;
    serverSocket_.asyncWrite(
        clientToServerBuf_.dataToSend(),
        asio::bind_executor(strand_,
            [self = shared_from_this()](const asio::error_code& ec, std::size_t n) {
                self->onServerWrite(ec, n);
            }
        )
    );
}

void Session::onClientWrite(const asio::error_code& ec, std::size_t bytesWritten) {
    clientWriteInProgress_ = false;

    if (ec) {
        if (ec != asio::error::operation_aborted) {
            globalLogger().warn(sessionId_, 0, "client_write_error", "downstream",
                               "error=" + ec.message());
        }
        closeClientWrite();
        return;
    }

    recordActivity();
    serverToClientBuf_.consume(bytesWritten);
    globalMetrics().observeBufferOccupancy(serverToClientBuf_.readable());

    // Resume server reads if backpressure cleared
    if (serverReadPaused_ && serverToClientBuf_.shouldResumeReading()) {
        serverReadPaused_ = false;
        startServerRead();
    }

    // Continue writing if more data
    if (serverToClientBuf_.readable() > 0) {
        startClientWrite();
    } else if (!channels_.serverReadOpen) {
        // Server finished sending, close client write
        closeClientWrite();
    }
}

void Session::onServerWrite(const asio::error_code& ec, std::size_t bytesWritten) {
    serverWriteInProgress_ = false;

    if (ec) {
        if (ec != asio::error::operation_aborted) {
            globalLogger().warn(sessionId_, 0, "server_write_error", "upstream",
                               "error=" + ec.message());
        }
        closeServerWrite();
        return;
    }

    recordActivity();
    clientToServerBuf_.consume(bytesWritten);
    globalMetrics().observeBufferOccupancy(clientToServerBuf_.readable());

    // Resume client reads if backpressure cleared
    if (clientReadPaused_ && clientToServerBuf_.shouldResumeReading()) {
        clientReadPaused_ = false;
        startClientRead();
    }

    // Continue writing if more data
    if (clientToServerBuf_.readable() > 0) {
        startServerWrite();
    } else if (!channels_.clientReadOpen) {
        // Client finished sending, close server write
        closeServerWrite();
    }
}

void Session::closeClientRead() {
    if (!channels_.clientReadOpen) return;
    channels_.clientReadOpen = false;
    
    std::error_code ec;
    clientSocket_.shutdownRead(ec);
    
    checkFullyClosed();
}

void Session::closeClientWrite() {
    if (!channels_.clientWriteOpen) return;
    channels_.clientWriteOpen = false;
    
    std::error_code ec;
    clientSocket_.shutdownWrite(ec);
    
    checkFullyClosed();
}

void Session::closeServerRead() {
    if (!channels_.serverReadOpen) return;
    channels_.serverReadOpen = false;
    
    std::error_code ec;
    serverSocket_.shutdownRead(ec);
    
    checkFullyClosed();
}

void Session::closeServerWrite() {
    if (!channels_.serverWriteOpen) return;
    channels_.serverWriteOpen = false;
    
    std::error_code ec;
    serverSocket_.shutdownWrite(ec);
    
    checkFullyClosed();
}

void Session::checkFullyClosed() {
    if (!channels_.isFullyClosed()) return;

    globalLogger().debug(sessionId_, 0, "fully_closed");
    
    // Remove from manager
    if (auto mgr = manager_.lock()) {
        mgr->removeSession(sessionId_);
    }
}

void Session::initiateShutdown() {
    globalLogger().info(sessionId_, 0, "shutdown_initiated");
    
    // Stop reading
    channels_.clientReadOpen = false;
    channels_.serverReadOpen = false;
    clientSocket_.cancelPending();
    serverSocket_.cancelPending();
    
    // Flush remaining data with dynamic linger
    auto pendingBytes = clientToServerBuf_.readable() + serverToClientBuf_.readable() +
                       clientToServerDelay_.totalBytes() + serverToClientDelay_.totalBytes();
    
    // Simple linger - just close after flushing what we can
    checkFullyClosed();
}

void Session::forceClose() {
    globalLogger().info(sessionId_, 0, "force_close");
    
    channels_.clientReadOpen = false;
    channels_.clientWriteOpen = false;
    channels_.serverReadOpen = false;
    channels_.serverWriteOpen = false;
    
    clientSocket_.forceReset();
    serverSocket_.forceReset();
    
    if (auto mgr = manager_.lock()) {
        mgr->removeSession(sessionId_);
    }
}

void Session::resetIdleTimer() {
    if (idleTimerId_) {
        scheduler_.cancel(idleTimerId_);
    }
    
    idleTimerId_ = scheduler_.scheduleGuarded(
        config_.serverConfig().idleTimeout,
        weak_from_this(),
        [](Session::Ptr self) { self->onIdleTimeout(); }
    );
}

void Session::onConnectTimeout() {
    connectTimerId_ = 0;
    globalLogger().warn(sessionId_, 0, "connect_timeout");
    globalMetrics().incrementConnectFailures();
    forceClose();
}

void Session::onIdleTimeout() {
    idleTimerId_ = 0;
    globalLogger().info(sessionId_, 0, "idle_timeout");
    initiateShutdown();
}

void Session::onStallTimeout() {
    stallTimerId_ = 0;
    globalLogger().warn(sessionId_, 0, "stall_timeout");
    forceClose();
}

void Session::recordActivity() {
    lastActivity_ = std::chrono::steady_clock::now();
    resetIdleTimer();
}

std::size_t Session::calculateBudget() const {
    if (auto mgr = manager_.lock()) {
        float pressure = static_cast<float>(mgr->sessionCount()) / 
                        ConfigLimits::MAX_SESSIONS;
        std::size_t budget = static_cast<std::size_t>(
            16384.0f / std::max(1.0f, pressure * 4.0f)
        );
        return std::clamp(budget, std::size_t(4096), std::size_t(65536));
    }
    return 16384;
}

} // namespace shakyline

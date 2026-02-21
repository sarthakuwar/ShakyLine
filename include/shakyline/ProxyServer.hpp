#pragma once

#include "shakyline/SessionManager.hpp"
#include "shakyline/Socket.hpp"

#include <asio.hpp>
#include <memory>
#include <atomic>

namespace shakyline {

/// TCP proxy server with acceptor and graceful shutdown
class ProxyServer {
public:
    ProxyServer(
        asio::io_context& io,
        SessionManager::Ptr sessionManager,
        const ServerConfig& config
    );

    ~ProxyServer();

    // Non-copyable
    ProxyServer(const ProxyServer&) = delete;
    ProxyServer& operator=(const ProxyServer&) = delete;

    /// Start accepting connections
    void start();

    /// Stop accepting (graceful)
    void stop();

    /// Check if running
    bool isRunning() const noexcept { return running_.load(); }

    /// Get listen port
    uint16_t listenPort() const { return config_.listenPort; }

private:
    void doAccept();
    void onAccept(const asio::error_code& ec, asio::ip::tcp::socket socket);

    asio::io_context& io_;
    asio::ip::tcp::acceptor acceptor_;
    SessionManager::Ptr sessionManager_;
    ServerConfig config_;
    std::atomic<bool> running_{false};
};

} // namespace shakyline

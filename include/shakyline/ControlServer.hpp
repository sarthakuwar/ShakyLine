#pragma once

#include "shakyline/Config.hpp"
#include "shakyline/MetricsRegistry.hpp"
#include "shakyline/SessionManager.hpp"

#include <asio.hpp>
#include <atomic>
#include <memory>
#include <thread>

namespace shakyline {

/// Minimal HTTP control server for runtime configuration
class ControlServer {
public:
    ControlServer(
        ConfigManager& config,
        SessionManager::Ptr sessionManager,
        uint16_t port
    );

    ~ControlServer();

    // Non-copyable
    ControlServer(const ControlServer&) = delete;
    ControlServer& operator=(const ControlServer&) = delete;

    /// Start the control server (runs in separate thread)
    void start();

    /// Stop the control server
    void stop();

    /// Check if running
    bool isRunning() const noexcept { return running_.load(); }

private:
    void run();
    void doAccept();
    void handleConnection(asio::ip::tcp::socket socket);
    
    std::string handleRequest(const std::string& method, 
                              const std::string& path,
                              const std::string& body);
    
    std::string handleGetHealth();
    std::string handleGetMetrics();
    std::string handleGetSessions();
    std::string handlePostProfile(const std::string& name, const std::string& body);
    std::string handleDeleteProfile(const std::string& name);

    std::string makeResponse(int status, const std::string& contentType, 
                             const std::string& body);
    std::string parseJson(const std::string& json, const std::string& key);

    ConfigManager& config_;
    SessionManager::Ptr sessionManager_;
    uint16_t port_;

    asio::io_context io_;
    std::unique_ptr<asio::ip::tcp::acceptor> acceptor_;
    std::atomic<bool> running_{false};
    std::thread thread_;
};

} // namespace shakyline

#include "shakyline/ProxyServer.hpp"
#include "shakyline/Logger.hpp"

namespace shakyline {

ProxyServer::ProxyServer(
    asio::io_context& io,
    SessionManager::Ptr sessionManager,
    const ServerConfig& config
) : io_(io)
  , acceptor_(io)
  , sessionManager_(std::move(sessionManager))
  , config_(config)
{}

ProxyServer::~ProxyServer() {
    stop();
}

void ProxyServer::start() {
    if (running_.load()) return;

    asio::ip::tcp::endpoint endpoint(
        asio::ip::make_address(config_.listenHost),
        config_.listenPort
    );

    acceptor_.open(endpoint.protocol());
    acceptor_.set_option(asio::socket_base::reuse_address(true));
    acceptor_.bind(endpoint);
    acceptor_.listen(asio::socket_base::max_listen_connections);

    // Set upstream endpoint in session manager
    asio::ip::tcp::endpoint upstream(
        asio::ip::make_address(config_.upstreamHost),
        config_.upstreamPort
    );
    sessionManager_->setUpstreamEndpoint(upstream);

    running_.store(true);
    
    globalLogger().info(0, 0, "server_started", "",
                        "listen=" + config_.listenHost + ":" + 
                        std::to_string(config_.listenPort) +
                        " upstream=" + config_.upstreamHost + ":" +
                        std::to_string(config_.upstreamPort));

    doAccept();
}

void ProxyServer::stop() {
    if (!running_.load()) return;
    
    running_.store(false);
    
    asio::error_code ec;
    acceptor_.cancel(ec);
    acceptor_.close(ec);
    
    globalLogger().info(0, 0, "server_stopped");
}

void ProxyServer::doAccept() {
    if (!running_.load()) return;

    acceptor_.async_accept(
        [this](const asio::error_code& ec, asio::ip::tcp::socket socket) {
            onAccept(ec, std::move(socket));
        }
    );
}

void ProxyServer::onAccept(const asio::error_code& ec, asio::ip::tcp::socket socket) {
    if (ec) {
        if (ec != asio::error::operation_aborted) {
            globalLogger().warn(0, 0, "accept_error", "", "error=" + ec.message());
        }
        return;
    }

    if (!running_.load()) return;

    // Log connection
    std::string remoteAddr = "unknown";
    try {
        auto remote = socket.remote_endpoint();
        remoteAddr = remote.address().to_string() + ":" + 
                    std::to_string(remote.port());
    } catch (...) {}

    globalLogger().debug(0, 0, "connection_accepted", "", "from=" + remoteAddr);

    // Create session
    Socket clientSocket(std::move(socket));
    auto session = sessionManager_->createSession(std::move(clientSocket));
    
    if (!session) {
        globalLogger().warn(0, 0, "session_rejected", "", "from=" + remoteAddr);
    }

    // Accept next connection
    doAccept();
}

} // namespace shakyline

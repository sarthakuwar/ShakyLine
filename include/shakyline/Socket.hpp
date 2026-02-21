#pragma once

#include <asio.hpp>
#include <cstdint>
#include <memory>
#include <optional>
#include <system_error>

namespace shakyline {

/// RAII wrapper for TCP socket with half-close and cancellation support
class Socket {
public:
    using tcp = asio::ip::tcp;

    /// Move-only semantics
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    Socket(Socket&&) noexcept = default;
    Socket& operator=(Socket&&) noexcept = default;

    /// Construct from existing socket
    explicit Socket(tcp::socket socket) noexcept;

    /// Create a new socket on the given io_context
    static Socket create(asio::io_context& io);

    /// Destructor - automatically closes socket
    ~Socket();

    /// Check if socket is open
    bool isOpen() const noexcept;

    /// Get native handle for low-level operations
    tcp::socket::native_handle_type native() const noexcept;

    /// Get underlying asio socket (for async operations)
    tcp::socket& raw() noexcept { return socket_; }
    const tcp::socket& raw() const noexcept { return socket_; }

    /// Set TCP_NODELAY
    void setNoDelay(bool enable);

    /// Set non-blocking mode
    void setNonBlocking(bool enable);

    /// Get bytes available to read without blocking
    std::size_t bytesAvailable() const;

    // --- Half-close operations ---

    /// Shutdown write side (send FIN)
    void shutdownWrite(std::error_code& ec) noexcept;

    /// Shutdown read side
    void shutdownRead(std::error_code& ec) noexcept;

    /// Force RST without graceful close
    void forceReset() noexcept;

    /// Cancel all pending async operations
    void cancelPending() noexcept;

    /// Close socket
    void close() noexcept;

    // --- Async operations ---

    /// Start async connect
    template<typename Handler>
    void asyncConnect(const tcp::endpoint& endpoint, Handler&& handler) {
        socket_.async_connect(endpoint, std::forward<Handler>(handler));
    }

    /// Start async read into buffer
    template<typename MutableBuffer, typename Handler>
    void asyncRead(MutableBuffer&& buffer, Handler&& handler) {
        socket_.async_read_some(std::forward<MutableBuffer>(buffer), 
                                std::forward<Handler>(handler));
    }

    /// Start async write from buffer
    template<typename ConstBuffer, typename Handler>
    void asyncWrite(ConstBuffer&& buffer, Handler&& handler) {
        asio::async_write(socket_, std::forward<ConstBuffer>(buffer),
                          std::forward<Handler>(handler));
    }

    /// Get remote endpoint
    std::optional<tcp::endpoint> remoteEndpoint() const noexcept;

    /// Get local endpoint
    std::optional<tcp::endpoint> localEndpoint() const noexcept;

private:
    tcp::socket socket_;
};

} // namespace shakyline

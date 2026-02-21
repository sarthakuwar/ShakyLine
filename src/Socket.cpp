#include "shakyline/Socket.hpp"

namespace shakyline {

Socket::Socket(tcp::socket socket) noexcept 
    : socket_(std::move(socket)) {}

Socket Socket::create(asio::io_context& io) {
    return Socket(tcp::socket(io));
}

Socket::~Socket() {
    close();
}

bool Socket::isOpen() const noexcept {
    return socket_.is_open();
}

Socket::tcp::socket::native_handle_type Socket::native() const noexcept {
    return socket_.native_handle();
}

void Socket::setNoDelay(bool enable) {
    socket_.set_option(tcp::no_delay(enable));
}

void Socket::setNonBlocking(bool enable) {
    socket_.non_blocking(enable);
}

std::size_t Socket::bytesAvailable() const {
    asio::socket_base::bytes_readable cmd(true);
    // Need non-const socket for io_control
    const_cast<tcp::socket&>(socket_).io_control(cmd);
    return cmd.get();
}

void Socket::shutdownWrite(std::error_code& ec) noexcept {
    socket_.shutdown(tcp::socket::shutdown_send, ec);
}

void Socket::shutdownRead(std::error_code& ec) noexcept {
    socket_.shutdown(tcp::socket::shutdown_receive, ec);
}

void Socket::forceReset() noexcept {
    if (!socket_.is_open()) return;
    
    std::error_code ec;
    // Set SO_LINGER with timeout 0 to force RST
    socket_.set_option(asio::socket_base::linger(true, 0), ec);
    socket_.close(ec);
}

void Socket::cancelPending() noexcept {
    std::error_code ec;
    socket_.cancel(ec);
}

void Socket::close() noexcept {
    if (!socket_.is_open()) return;
    
    std::error_code ec;
    socket_.shutdown(tcp::socket::shutdown_both, ec);
    socket_.close(ec);
}

std::optional<Socket::tcp::endpoint> Socket::remoteEndpoint() const noexcept {
    std::error_code ec;
    auto ep = socket_.remote_endpoint(ec);
    if (ec) return std::nullopt;
    return ep;
}

std::optional<Socket::tcp::endpoint> Socket::localEndpoint() const noexcept {
    std::error_code ec;
    auto ep = socket_.local_endpoint(ec);
    if (ec) return std::nullopt;
    return ep;
}

} // namespace shakyline

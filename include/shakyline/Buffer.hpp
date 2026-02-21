#pragma once

#include <asio.hpp>
#include <cstdint>
#include <cstring>
#include <vector>
#include <algorithm>

namespace shakyline {

/// Ring buffer with high/low watermarks for flow control
class Buffer {
public:
    static constexpr std::size_t DEFAULT_CAPACITY = 64 * 1024;  // 64KB
    static constexpr std::size_t HIGH_WATERMARK = 48 * 1024;    // 48KB
    static constexpr std::size_t LOW_WATERMARK = 16 * 1024;     // 16KB

    explicit Buffer(std::size_t capacity = DEFAULT_CAPACITY);

    // Non-copyable, movable
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&&) noexcept = default;
    Buffer& operator=(Buffer&&) noexcept = default;

    /// Current readable bytes
    std::size_t readable() const noexcept { return size_; }

    /// Available space for writing
    std::size_t writable() const noexcept { return capacity_ - size_; }

    /// Total capacity
    std::size_t capacity() const noexcept { return capacity_; }

    /// Is buffer empty?
    bool empty() const noexcept { return size_ == 0; }

    /// Is buffer full?
    bool full() const noexcept { return size_ >= capacity_; }

    // --- Flow control ---

    /// Should pause reading from source?
    bool shouldPauseReading() const noexcept { return size_ >= HIGH_WATERMARK; }

    /// Should resume reading from source?
    bool shouldResumeReading() const noexcept { return size_ <= LOW_WATERMARK; }

    // --- Data operations ---

    /// Append data to buffer
    /// Returns bytes actually written (may be less if buffer full)
    std::size_t append(const uint8_t* data, std::size_t len);

    /// Consume data from buffer front
    /// Returns bytes actually consumed
    std::size_t consume(std::size_t len);

    /// Peek at front data without consuming
    /// Returns number of contiguous bytes available
    std::size_t peek(const uint8_t** outData) const noexcept;

    /// Get asio mutable buffer for async_read
    asio::mutable_buffer prepareWrite(std::size_t maxBytes);

    /// Commit bytes written via prepareWrite
    void commitWrite(std::size_t bytesWritten);

    /// Get asio const buffer for async_write
    asio::const_buffer dataToSend() const;

    /// Clear all data
    void clear() noexcept;

private:
    std::vector<uint8_t> data_;
    std::size_t capacity_;
    std::size_t readPos_ = 0;
    std::size_t writePos_ = 0;
    std::size_t size_ = 0;

    void compact();
};

} // namespace shakyline

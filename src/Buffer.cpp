#include "shakyline/Buffer.hpp"

namespace shakyline {

Buffer::Buffer(std::size_t capacity)
    : data_(capacity)
    , capacity_(capacity) {}

std::size_t Buffer::append(const uint8_t* data, std::size_t len) {
    if (len == 0) return 0;
    
    // Compact if needed to make space
    if (writePos_ + len > capacity_ && readPos_ > 0) {
        compact();
    }

    std::size_t toWrite = std::min(len, capacity_ - writePos_);
    if (toWrite == 0) return 0;

    std::memcpy(data_.data() + writePos_, data, toWrite);
    writePos_ += toWrite;
    size_ += toWrite;
    return toWrite;
}

std::size_t Buffer::consume(std::size_t len) {
    std::size_t toConsume = std::min(len, size_);
    readPos_ += toConsume;
    size_ -= toConsume;
    
    // Reset positions if empty
    if (size_ == 0) {
        readPos_ = 0;
        writePos_ = 0;
    }
    
    return toConsume;
}

std::size_t Buffer::peek(const uint8_t** outData) const noexcept {
    if (size_ == 0) {
        *outData = nullptr;
        return 0;
    }
    *outData = data_.data() + readPos_;
    return size_;
}

asio::mutable_buffer Buffer::prepareWrite(std::size_t maxBytes) {
    // Compact if needed
    if (writePos_ + maxBytes > capacity_ && readPos_ > 0) {
        compact();
    }
    
    std::size_t available = capacity_ - writePos_;
    std::size_t toReserve = std::min(maxBytes, available);
    return asio::mutable_buffer(data_.data() + writePos_, toReserve);
}

void Buffer::commitWrite(std::size_t bytesWritten) {
    writePos_ += bytesWritten;
    size_ += bytesWritten;
}

asio::const_buffer Buffer::dataToSend() const {
    return asio::const_buffer(data_.data() + readPos_, size_);
}

void Buffer::clear() noexcept {
    readPos_ = 0;
    writePos_ = 0;
    size_ = 0;
}

void Buffer::compact() {
    if (readPos_ == 0) return;
    
    if (size_ > 0) {
        std::memmove(data_.data(), data_.data() + readPos_, size_);
    }
    writePos_ = size_;
    readPos_ = 0;
}

} // namespace shakyline

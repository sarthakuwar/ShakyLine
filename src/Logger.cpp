#include "shakyline/Logger.hpp"

namespace shakyline {

Logger::Logger() {
    output_ = [](const std::string& msg) {
        std::cerr << msg << std::endl;
    };
}

Logger::~Logger() = default;

void Logger::log(LogLevel level, uint64_t sessionId, uint64_t packetSeq,
                 std::string_view event, std::string_view direction,
                 std::string_view details) {
    if (!enabled_) return;

    LogEntry entry{
        std::chrono::steady_clock::now(),
        level,
        sessionId,
        packetSeq,
        std::string(event),
        std::string(direction),
        std::string(details)
    };

    std::lock_guard<std::mutex> lock(mutex_);

    // Always add DEBUG to black box (ring buffer)
    if (level == LogLevel::Debug) {
        if (blackBox_.size() >= BLACK_BOX_SIZE) {
            blackBox_.pop_front();
        }
        blackBox_.push_back(entry);
    }

    // Add to live queue (with pressure-based dropping)
    if (liveQueue_.size() >= LIVE_QUEUE_MAX) {
        pruneQueue();
    }

    if (liveQueue_.size() < LIVE_QUEUE_MAX) {
        liveQueue_.push_back(entry);
        writeEntry(entry);
    }
}

std::string Logger::format(const LogEntry& entry) {
    std::ostringstream oss;
    
    // Level prefix
    switch (entry.level) {
        case LogLevel::Debug: oss << "[DEBUG] "; break;
        case LogLevel::Info:  oss << "[INFO]  "; break;
        case LogLevel::Warn:  oss << "[WARN]  "; break;
        case LogLevel::Error: oss << "[ERROR] "; break;
    }

    // Session and packet info
    oss << "sid=" << entry.sessionId;
    if (entry.packetSeq > 0) {
        oss << " pkt=" << entry.packetSeq;
    }
    
    // Direction
    if (!entry.direction.empty()) {
        oss << " dir=" << entry.direction;
    }

    // Event
    oss << " event=" << entry.event;

    // Additional details
    if (!entry.details.empty()) {
        oss << " " << entry.details;
    }

    return oss.str();
}

void Logger::dumpBlackBox() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::cerr << "\n=== BLACK BOX DUMP (" << blackBox_.size() << " entries) ===" << std::endl;
    for (const auto& entry : blackBox_) {
        std::cerr << format(entry) << std::endl;
    }
    std::cerr << "=== END BLACK BOX ===" << std::endl;
}

void Logger::writeEntry(const LogEntry& entry) {
    if (output_) {
        output_(format(entry));
    }
}

void Logger::pruneQueue() {
    // Drop DEBUG entries first, then INFO
    auto it = liveQueue_.begin();
    while (it != liveQueue_.end() && liveQueue_.size() >= LIVE_QUEUE_MAX) {
        if (it->level == LogLevel::Debug) {
            it = liveQueue_.erase(it);
        } else {
            ++it;
        }
    }

    // If still full, drop INFO
    it = liveQueue_.begin();
    while (it != liveQueue_.end() && liveQueue_.size() >= LIVE_QUEUE_MAX) {
        if (it->level == LogLevel::Info) {
            it = liveQueue_.erase(it);
        } else {
            ++it;
        }
    }
}

Logger& globalLogger() {
    static Logger instance;
    return instance;
}

} // namespace shakyline

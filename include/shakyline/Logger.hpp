#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <deque>
#include <mutex>
#include <string>
#include <string_view>
#include <functional>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace shakyline {

enum class LogLevel {
    Debug,
    Info,
    Warn,
    Error
};

/// Structured log entry
struct LogEntry {
    std::chrono::steady_clock::time_point timestamp;
    LogLevel level;
    uint64_t sessionId;
    uint64_t packetSeq;
    std::string event;
    std::string direction;
    std::string details;
};

/// Two-tier logger with black box buffer
/// - Live queue (50K entries, drops DEBUG first under pressure)
/// - Black box ring (5K DEBUG entries, preserved for post-mortem)
class Logger {
public:
    static constexpr std::size_t LIVE_QUEUE_MAX = 50000;
    static constexpr std::size_t BLACK_BOX_SIZE = 5000;

    Logger();
    ~Logger();

    // Non-copyable
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    /// Log a structured message
    void log(LogLevel level, uint64_t sessionId, uint64_t packetSeq,
             std::string_view event, std::string_view direction = "",
             std::string_view details = "");

    /// Convenience methods
    void debug(uint64_t sid, uint64_t pkt, std::string_view event,
               std::string_view dir = "", std::string_view details = "") {
        log(LogLevel::Debug, sid, pkt, event, dir, details);
    }

    void info(uint64_t sid, uint64_t pkt, std::string_view event,
              std::string_view dir = "", std::string_view details = "") {
        log(LogLevel::Info, sid, pkt, event, dir, details);
    }

    void warn(uint64_t sid, uint64_t pkt, std::string_view event,
              std::string_view dir = "", std::string_view details = "") {
        log(LogLevel::Warn, sid, pkt, event, dir, details);
    }

    void error(uint64_t sid, uint64_t pkt, std::string_view event,
               std::string_view dir = "", std::string_view details = "") {
        log(LogLevel::Error, sid, pkt, event, dir, details);
    }

    /// Format a log entry to string
    static std::string format(const LogEntry& entry);

    /// Dump black box to stderr (for shutdown/crash)
    void dumpBlackBox();

    /// Set log output callback (default: stderr)
    void setOutput(std::function<void(const std::string&)> output) {
        output_ = std::move(output);
    }

    /// Enable/disable logging
    void setEnabled(bool enabled) { enabled_ = enabled; }

private:
    std::mutex mutex_;
    std::deque<LogEntry> liveQueue_;
    std::deque<LogEntry> blackBox_;  // Ring buffer for DEBUG
    std::function<void(const std::string&)> output_;
    bool enabled_ = true;

    void writeEntry(const LogEntry& entry);
    void pruneQueue();
};

/// Global logger instance
Logger& globalLogger();

} // namespace shakyline

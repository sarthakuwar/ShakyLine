#include "shakyline/ControlServer.hpp"
#include "shakyline/Logger.hpp"

#include <sstream>
#include <regex>

namespace shakyline {

ControlServer::ControlServer(
    ConfigManager& config,
    SessionManager::Ptr sessionManager,
    uint16_t port
) : config_(config)
  , sessionManager_(std::move(sessionManager))
  , port_(port)
{}

ControlServer::~ControlServer() {
    stop();
}

void ControlServer::start() {
    if (running_.load()) return;
    
    running_.store(true);
    thread_ = std::thread([this]() { run(); });
    
    globalLogger().info(0, 0, "control_server_started", "",
                        "port=" + std::to_string(port_));
}

void ControlServer::stop() {
    if (!running_.load()) return;
    
    running_.store(false);
    io_.stop();
    
    if (thread_.joinable()) {
        thread_.join();
    }
    
    globalLogger().info(0, 0, "control_server_stopped");
}

void ControlServer::run() {
    try {
        asio::ip::tcp::endpoint endpoint(asio::ip::tcp::v4(), port_);
        acceptor_ = std::make_unique<asio::ip::tcp::acceptor>(io_, endpoint);
        acceptor_->set_option(asio::socket_base::reuse_address(true));
        
        doAccept();
        io_.run();
    } catch (const std::exception& e) {
        globalLogger().error(0, 0, "control_server_error", "", 
                            "error=" + std::string(e.what()));
    }
}

void ControlServer::doAccept() {
    if (!running_.load()) return;

    acceptor_->async_accept([this](const asio::error_code& ec, 
                                    asio::ip::tcp::socket socket) {
        if (!ec && running_.load()) {
            handleConnection(std::move(socket));
        }
        if (running_.load()) {
            doAccept();
        }
    });
}

void ControlServer::handleConnection(asio::ip::tcp::socket socket) {
    try {
        // Read request (simple blocking read - control API is low traffic)
        asio::streambuf buf;
        asio::read_until(socket, buf, "\r\n\r\n");
        
        std::istream stream(&buf);
        std::string method, path, version;
        stream >> method >> path >> version;
        
        // Read headers
        std::string line;
        std::size_t contentLength = 0;
        while (std::getline(stream, line) && line != "\r" && !line.empty()) {
            if (line.find("Content-Length:") == 0) {
                contentLength = std::stoul(line.substr(16));
            }
        }
        
        // Read body if present
        std::string body;
        if (contentLength > 0) {
            // Check if body is already in buffer
            std::size_t already = buf.size();
            if (already < contentLength) {
                asio::read(socket, buf, 
                          asio::transfer_exactly(contentLength - already));
            }
            std::istream bodyStream(&buf);
            body.resize(contentLength);
            bodyStream.read(body.data(), contentLength);
        }
        
        // Handle request
        std::string response = handleRequest(method, path, body);
        
        // Send response
        asio::write(socket, asio::buffer(response));
        
    } catch (const std::exception& e) {
        globalLogger().debug(0, 0, "control_connection_error", "",
                            "error=" + std::string(e.what()));
    }
}

std::string ControlServer::handleRequest(const std::string& method,
                                          const std::string& path,
                                          const std::string& body) {
    // Rate limit check
    if ((method == "POST" || method == "DELETE") && !config_.checkRateLimit()) {
        return makeResponse(429, "text/plain", "Rate limit exceeded");
    }

    // Route requests
    if (path == "/health" && method == "GET") {
        return handleGetHealth();
    }
    
    if (path == "/metrics" && method == "GET") {
        return handleGetMetrics();
    }
    
    if (path == "/sessions" && method == "GET") {
        return handleGetSessions();
    }
    
    // Profile routes: /profiles/{name}
    std::regex profileRegex("/profiles/([^/]+)");
    std::smatch match;
    if (std::regex_match(path, match, profileRegex)) {
        std::string name = match[1];
        if (method == "POST") {
            return handlePostProfile(name, body);
        } else if (method == "DELETE") {
            return handleDeleteProfile(name);
        }
    }
    
    return makeResponse(404, "text/plain", "Not Found");
}

std::string ControlServer::handleGetHealth() {
    bool healthy = sessionManager_ != nullptr;
    if (healthy) {
        return makeResponse(200, "application/json", "{\"status\":\"ok\"}");
    } else {
        return makeResponse(500, "application/json", "{\"status\":\"error\"}");
    }
}

std::string ControlServer::handleGetMetrics() {
    std::string metrics = globalMetrics().renderPrometheus();
    return makeResponse(200, "text/plain; version=0.0.4", metrics);
}

std::string ControlServer::handleGetSessions() {
    auto ids = sessionManager_->getSessionIds();
    
    std::ostringstream oss;
    oss << "{\"sessions\":[";
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (i > 0) oss << ",";
        oss << ids[i];
    }
    oss << "],\"count\":" << ids.size() << "}";
    
    return makeResponse(200, "application/json", oss.str());
}

std::string ControlServer::handlePostProfile(const std::string& name, 
                                              const std::string& body) {
    try {
        AnomalyProfile profile;
        
        // Simple JSON parsing (minimal, no external deps)
        auto parseUint = [&](const std::string& key) -> uint32_t {
            std::string val = parseJson(body, key);
            return val.empty() ? 0 : std::stoul(val);
        };
        auto parseFloat = [&](const std::string& key) -> float {
            std::string val = parseJson(body, key);
            return val.empty() ? 0.0f : std::stof(val);
        };

        // Client to server
        profile.clientToServer.latencyMs = parseUint("c2s_latency_ms");
        profile.clientToServer.jitterMs = parseUint("c2s_jitter_ms");
        profile.clientToServer.throttleKbps = parseUint("c2s_throttle_kbps");
        profile.clientToServer.dropRate = parseFloat("c2s_drop_rate");
        profile.clientToServer.stallProbability = parseFloat("c2s_stall_prob");

        // Server to client
        profile.serverToClient.latencyMs = parseUint("s2c_latency_ms");
        profile.serverToClient.jitterMs = parseUint("s2c_jitter_ms");
        profile.serverToClient.throttleKbps = parseUint("s2c_throttle_kbps");
        profile.serverToClient.dropRate = parseFloat("s2c_drop_rate");
        profile.serverToClient.stallProbability = parseFloat("s2c_stall_prob");

        // Also try simple top-level keys for convenience
        if (profile.clientToServer.latencyMs == 0) {
            profile.clientToServer.latencyMs = parseUint("latency_ms");
            profile.serverToClient.latencyMs = parseUint("latency_ms");
        }
        if (profile.clientToServer.dropRate == 0) {
            profile.clientToServer.dropRate = parseFloat("drop_rate");
            profile.serverToClient.dropRate = parseFloat("drop_rate");
        }

        uint32_t version = config_.setProfile(name, profile);
        
        globalLogger().info(0, 0, "profile_updated", "",
                           "name=" + name + " version=" + std::to_string(version));
        
        return makeResponse(200, "application/json", 
                           "{\"version\":" + std::to_string(version) + "}");
                           
    } catch (const std::exception& e) {
        return makeResponse(400, "application/json",
                           "{\"error\":\"" + std::string(e.what()) + "\"}");
    }
}

std::string ControlServer::handleDeleteProfile(const std::string& name) {
    bool deleted = config_.deleteProfile(name);
    if (deleted) {
        globalLogger().info(0, 0, "profile_deleted", "", "name=" + name);
        return makeResponse(200, "application/json", "{\"deleted\":true}");
    } else {
        return makeResponse(404, "application/json", "{\"error\":\"not found\"}");
    }
}

std::string ControlServer::makeResponse(int status, const std::string& contentType,
                                         const std::string& body) {
    std::ostringstream oss;
    oss << "HTTP/1.1 " << status << " ";
    switch (status) {
        case 200: oss << "OK"; break;
        case 400: oss << "Bad Request"; break;
        case 404: oss << "Not Found"; break;
        case 429: oss << "Too Many Requests"; break;
        case 500: oss << "Internal Server Error"; break;
        default: oss << "Unknown"; break;
    }
    oss << "\r\n";
    oss << "Content-Type: " << contentType << "\r\n";
    oss << "Content-Length: " << body.size() << "\r\n";
    oss << "Connection: close\r\n";
    oss << "\r\n";
    oss << body;
    return oss.str();
}

std::string ControlServer::parseJson(const std::string& json, const std::string& key) {
    // Very simple JSON value extraction (no nested objects)
    std::string pattern = "\"" + key + "\"\\s*:\\s*([0-9.]+|\"[^\"]*\")";
    std::regex re(pattern);
    std::smatch match;
    if (std::regex_search(json, match, re)) {
        std::string val = match[1];
        // Remove quotes if string
        if (!val.empty() && val[0] == '"') {
            return val.substr(1, val.size() - 2);
        }
        return val;
    }
    return "";
}

} // namespace shakyline

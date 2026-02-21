#include "shakyline/AnomalyEngine.hpp"
#include "shakyline/Config.hpp"
#include "shakyline/ControlServer.hpp"
#include "shakyline/EventLoop.hpp"
#include "shakyline/Logger.hpp"
#include "shakyline/MetricsRegistry.hpp"
#include "shakyline/ProxyServer.hpp"
#include "shakyline/Scheduler.hpp"
#include "shakyline/SessionManager.hpp"

#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace shakyline;

namespace {
    std::atomic<bool> g_shutdown{false};
    
    void signalHandler(int signal) {
        if (signal == SIGINT || signal == SIGTERM) {
            g_shutdown.store(true);
        }
    }

    void printUsage(const char* prog) {
        std::cout << "ShakyLine v1.0 - Programmable Network Fault Injection Proxy\n\n"
                  << "Usage: " << prog << " [OPTIONS]\n\n"
                  << "Options:\n"
                  << "  --listen HOST:PORT     Listen address (default: 0.0.0.0:8080)\n"
                  << "  --upstream HOST:PORT   Upstream target (default: 127.0.0.1:9000)\n"
                  << "  --control PORT         Control API port (default: 9090)\n"
                  << "  --seed NUMBER          Global RNG seed (default: random)\n"
                  << "  --help                 Show this help\n\n"
                  << "Control API:\n"
                  << "  POST /profiles/{name}  Update anomaly profile\n"
                  << "  DELETE /profiles/{name} Delete profile\n"
                  << "  GET /sessions          List active sessions\n"
                  << "  GET /metrics           Prometheus metrics\n"
                  << "  GET /health            Health check\n\n"
                  << "Example:\n"
                  << "  " << prog << " --listen 0.0.0.0:8080 --upstream api.example.com:443\n";
    }

    bool parseHostPort(const std::string& arg, std::string& host, uint16_t& port) {
        auto pos = arg.find(':');
        if (pos == std::string::npos) {
            port = std::stoi(arg);
            return true;
        }
        host = arg.substr(0, pos);
        port = std::stoi(arg.substr(pos + 1));
        return true;
    }
}

int main(int argc, char* argv[]) {
    ServerConfig config;
    
    // Parse command line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help" || arg == "-h") {
            printUsage(argv[0]);
            return 0;
        }
        else if (arg == "--listen" && i + 1 < argc) {
            parseHostPort(argv[++i], config.listenHost, config.listenPort);
        }
        else if (arg == "--upstream" && i + 1 < argc) {
            parseHostPort(argv[++i], config.upstreamHost, config.upstreamPort);
        }
        else if (arg == "--control" && i + 1 < argc) {
            config.controlPort = std::stoi(argv[++i]);
        }
        else if (arg == "--seed" && i + 1 < argc) {
            config.globalSeed = std::stoull(argv[++i]);
        }
        else {
            std::cerr << "Unknown option: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    // Generate random seed if not specified
    if (config.globalSeed == 0) {
        config.globalSeed = std::random_device{}();
    }

    std::cout << "╔═══════════════════════════════════════════════════════╗\n"
              << "║       ShakyLine - Fault Injection Proxy v1.0          ║\n"
              << "╚═══════════════════════════════════════════════════════╝\n\n";

    std::cout << "Configuration:\n"
              << "  Listen:   " << config.listenHost << ":" << config.listenPort << "\n"
              << "  Upstream: " << config.upstreamHost << ":" << config.upstreamPort << "\n"
              << "  Control:  http://localhost:" << config.controlPort << "\n"
              << "  Seed:     " << config.globalSeed << "\n\n";

    // Setup signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    try {
        // Create components
        EventLoop eventLoop;
        Scheduler scheduler(eventLoop.context());
        ConfigManager configManager;
        configManager.serverConfig() = config;
        
        AnomalyEngine anomalyEngine(config.globalSeed);
        
        auto sessionManager = SessionManager::create(
            eventLoop.context(), scheduler, anomalyEngine, configManager
        );
        
        ProxyServer proxyServer(eventLoop.context(), sessionManager, config);
        ControlServer controlServer(configManager, sessionManager, config.controlPort);

        // Start servers
        proxyServer.start();
        controlServer.start();

        std::cout << "Proxy started. Press Ctrl+C to stop.\n\n";
        std::cout << "Example commands:\n"
                  << "  curl http://localhost:" << config.controlPort << "/health\n"
                  << "  curl http://localhost:" << config.controlPort << "/metrics\n"
                  << "  curl -X POST http://localhost:" << config.controlPort 
                  << "/profiles/default -d '{\"latency_ms\":100}'\n\n";

        // Run event loop
        eventLoop.runInBackground();

        // Wait for shutdown signal
        while (!g_shutdown.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        std::cout << "\nShutting down...\n";

        // Graceful shutdown sequence
        proxyServer.stop();
        controlServer.stop();
        sessionManager->shutdownAll();
        
        // Wait for drain (simple timeout)
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        sessionManager->forceCloseAll();
        eventLoop.stop();
        eventLoop.join();

        // Dump black box log
        globalLogger().dumpBlackBox();

        std::cout << "Shutdown complete.\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
}

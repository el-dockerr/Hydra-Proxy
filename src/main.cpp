#include <iostream>
#include <csignal>
#include "config.h"
#include "proxy_server.h"

// Global server pointer for signal handler
hydra::ProxyServer* g_server = nullptr;

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nShutting down server..." << std::endl;
        if (g_server) {
            g_server->stop();
        }
    }
}

int main(int argc, char* argv[]) {
    try {
        std::cout << "==================================" << std::endl;
        std::cout << "     HYDRA High-Speed Proxy      " << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << std::endl;
        
        // Load configuration
        std::string config_file = "config.json";
        if (argc > 1) {
            config_file = argv[1];
        }
        
        hydra::Config config;
        if (!config.load(config_file)) {
            std::cerr << "Failed to load configuration from " << config_file << std::endl;
            return 1;
        }
        
        std::cout << std::endl;
        
        // Create the proxy server
        hydra::ProxyServer server(config);
        g_server = &server;
        
        // Set up signal handler for graceful shutdown
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);
        
        std::cout << std::endl;
        std::cout << "Press Ctrl+C to stop the server" << std::endl;
        std::cout << "==================================" << std::endl;
        std::cout << std::endl;
        
        // Run the server (blocks until stopped)
        server.run();
        
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}


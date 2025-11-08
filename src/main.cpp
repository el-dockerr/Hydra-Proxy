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
        std::cout << "888                  888           " << std::endl;     
        std::cout << "888                  888                " << std::endl;
        std::cout << "888                  888                " << std::endl;
        std::cout << "88888b. 888  888 .d88888888d888 8888b.  " << std::endl;
        std::cout << "888 \"88b888  888d88\" 888888P\"      \"88b " << std::endl;
        std::cout << "888  888888  888888  888888    .d888888 " << std::endl;
        std::cout << "888  888Y88b 888Y88b 888888    888  888 " << std::endl;
        std::cout << "888  888 \"Y88888 \"Y88888888    \"Y888888 " << std::endl;
        std::cout << "             888                        " << std::endl;
        std::cout << "        Y8b d88P                        " << std::endl;
        std::cout << "         \"Y88P\"                        " << std::endl;
        std::cout << "================================================" << std::endl;
        std::cout << "              Version 0.0.3           " << std::endl;
        std::cout << "================================================" << std::endl;
        std::cout << "      https://github.com/el-dockerr   " << std::endl;
        std::cout << "      written by Swen 'El Dockerr' Kalski" << std::endl;
        std::cout << "================================================" << std::endl;
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


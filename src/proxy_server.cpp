#include "proxy_server.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <string>

namespace hydra {

// SocketUtils implementation
bool SocketUtils::initialize() {
#ifdef _WIN32
    WSADATA wsa_data;
    return WSAStartup(MAKEWORD(2, 2), &wsa_data) == 0;
#else
    return true;
#endif
}

void SocketUtils::cleanup() {
#ifdef _WIN32
    WSACleanup();
#endif
}

void SocketUtils::close_socket(socket_t sock) {
    if (sock != INVALID_SOCKET) {
#ifdef _WIN32
        closesocket(sock);
#else
        close(sock);
#endif
    }
}

bool SocketUtils::set_non_blocking(socket_t sock) {
#ifdef _WIN32
    u_long mode = 1;
    return ioctlsocket(sock, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags == -1) return false;
    return fcntl(sock, F_SETFL, flags | O_NONBLOCK) != -1;
#endif
}

bool SocketUtils::set_no_delay(socket_t sock) {
    int flag = 1;
    return setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, 
                     (char*)&flag, sizeof(flag)) == 0;
}

bool SocketUtils::set_reuse_addr(socket_t sock) {
    int flag = 1;
    return setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, 
                     (char*)&flag, sizeof(flag)) == 0;
}

// ProxySession implementation
ProxySession::ProxySession(socket_t socket, 
                           const std::vector<Target>& targets,
                           size_t buffer_size)
    : socket_(socket)
    , targets_(targets)
    , buffer_(buffer_size)
    , buffer_size_(buffer_size) {
}

void ProxySession::start() {
    // This will be called in a worker thread
    handle_client();
}

void ProxySession::handle_client() {
    while (true) {
#ifdef _WIN32
        int bytes_read = recv(socket_, buffer_.data(), (int)buffer_size_, 0);
#else
        ssize_t bytes_read = recv(socket_, buffer_.data(), buffer_size_, 0);
#endif
        
        if (bytes_read > 0) {
            // Broadcast the data to all targets
            broadcast_to_targets(buffer_, bytes_read);
            
            // Extract the body from the request (everything after \r\n\r\n)
            const char* body_start = nullptr;
            size_t body_length = 0;
            
            // Look for the end of HTTP headers
            for (size_t i = 0; i < static_cast<size_t>(bytes_read) - 3; i++) {
                if (buffer_[i] == '\r' && buffer_[i+1] == '\n' && 
                    buffer_[i+2] == '\r' && buffer_[i+3] == '\n') {
                    body_start = buffer_.data() + i + 4;
                    body_length = bytes_read - (i + 4);
                    break;
                }
            }
            
            // Build HTTP response with the body
            std::string http_response = 
                "HTTP/1.1 200 OK\r\n"
                "Content-Length: " + std::to_string(body_length) + "\r\n"
                "Connection: keep-alive\r\n"
                "\r\n";
            
            // Send HTTP headers
            size_t total_sent = 0;
            size_t response_len = http_response.length();
            
            while (total_sent < response_len) {
#ifdef _WIN32
                int sent = send(socket_, http_response.c_str() + total_sent, (int)(response_len - total_sent), 0);
#else
                ssize_t sent = send(socket_, http_response.c_str() + total_sent, response_len - total_sent, 0);
#endif
                if (sent == SOCKET_ERROR) {
#ifdef _WIN32
                    std::cerr << "Failed to send HTTP response headers to client: " << WSAGetLastError() << std::endl;
#else
                    std::cerr << "Failed to send HTTP response headers to client: " << strerror(errno) << std::endl;
#endif
                    break;
                }
                total_sent += sent;
            }
            
            // Send the body if there is one
            if (body_length > 0 && body_start != nullptr) {
                total_sent = 0;
                while (total_sent < body_length) {
#ifdef _WIN32
                    int sent = send(socket_, body_start + total_sent, (int)(body_length - total_sent), 0);
#else
                    ssize_t sent = send(socket_, body_start + total_sent, body_length - total_sent, 0);
#endif
                    if (sent == SOCKET_ERROR) {
#ifdef _WIN32
                        std::cerr << "Failed to send HTTP response body to client: " << WSAGetLastError() << std::endl;
#else
                        std::cerr << "Failed to send HTTP response body to client: " << strerror(errno) << std::endl;
#endif
                        break;
                    }
                    total_sent += sent;
                }
            }
        } else if (bytes_read == 0) {
            // Connection closed
            break;
        } else {
#ifdef _WIN32
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                std::cerr << "Read error: " << error << std::endl;
                break;
            }
#else
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                std::cerr << "Read error: " << strerror(errno) << std::endl;
                break;
            }
#endif
        }
    }
    
    SocketUtils::close_socket(socket_);
}

void ProxySession::broadcast_to_targets(const std::vector<char>& data, size_t length) {
    // For maximum speed, spawn threads for each target simultaneously
    std::vector<std::thread> send_threads;
    send_threads.reserve(targets_.size());
    
    for (const auto& target : targets_) {
        // Create a copy of the data for this thread
        auto data_copy = std::make_shared<std::vector<char>>(data.begin(), data.begin() + length);
        
        send_threads.emplace_back([target, data_copy]() {
            // Resolve the target hostname
            struct addrinfo hints, *result = nullptr;
            std::memset(&hints, 0, sizeof(hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            
            std::string port_str = std::to_string(target.port);
            if (getaddrinfo(target.host.c_str(), port_str.c_str(), &hints, &result) != 0) {
                std::cerr << "Failed to resolve " << target.host << std::endl;
                return;
            }
            
            // Create socket
            socket_t target_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
            if (target_socket == INVALID_SOCKET) {
                std::cerr << "Failed to create socket for " << target.host << ":" << target.port << std::endl;
                freeaddrinfo(result);
                return;
            }
            
            // Set TCP_NODELAY for low latency
            SocketUtils::set_no_delay(target_socket);
            
            // Connect to target
            if (connect(target_socket, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
#ifdef _WIN32
                std::cerr << "Connect error to " << target.host << ":" << target.port 
                         << " - Error: " << WSAGetLastError() << std::endl;
#else
                std::cerr << "Connect error to " << target.host << ":" << target.port 
                         << " - " << strerror(errno) << std::endl;
#endif
                SocketUtils::close_socket(target_socket);
                freeaddrinfo(result);
                return;
            }
            
            freeaddrinfo(result);
            
            // Send data
            size_t total_sent = 0;
            while (total_sent < data_copy->size()) {
#ifdef _WIN32
                int sent = send(target_socket, 
                               data_copy->data() + total_sent, 
                               (int)(data_copy->size() - total_sent), 
                               0);
#else
                ssize_t sent = send(target_socket, 
                                   data_copy->data() + total_sent, 
                                   data_copy->size() - total_sent, 
                                   0);
#endif
                if (sent == SOCKET_ERROR) {
#ifdef _WIN32
                    std::cerr << "Write error to " << target.host << ":" << target.port 
                             << " - Error: " << WSAGetLastError() << std::endl;
#else
                    std::cerr << "Write error to " << target.host << ":" << target.port 
                             << " - " << strerror(errno) << std::endl;
#endif
                    break;
                }
                total_sent += sent;
            }
            
            SocketUtils::close_socket(target_socket);
        });
    }
    
    // Wait for all sends to complete
    for (auto& thread : send_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

// ProxyServer implementation
ProxyServer::ProxyServer(const Config& config)
    : listen_socket_(INVALID_SOCKET)
    , config_(config)
    , running_(false) {
    
    SocketUtils::initialize();
    
    // Create listening socket
    listen_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket_ == INVALID_SOCKET) {
        throw std::runtime_error("Failed to create listening socket");
    }
    
    // Set socket options
    SocketUtils::set_reuse_addr(listen_socket_);
    
    // Bind to port
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(config_.get_listen_port());
    
    if (bind(listen_socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        SocketUtils::close_socket(listen_socket_);
        throw std::runtime_error("Failed to bind to port " + std::to_string(config_.get_listen_port()));
    }
    
    // Listen for connections
    if (listen(listen_socket_, SOMAXCONN) == SOCKET_ERROR) {
        SocketUtils::close_socket(listen_socket_);
        throw std::runtime_error("Failed to listen on socket");
    }
    
    std::cout << "Hydra proxy server listening on port " 
              << config_.get_listen_port() << std::endl;
    std::cout << "Broadcasting to " << config_.get_targets().size() 
              << " targets" << std::endl;
}

ProxyServer::~ProxyServer() {
    stop();
    SocketUtils::close_socket(listen_socket_);
    SocketUtils::cleanup();
}

void ProxyServer::run() {
    running_ = true;
    
    // Create worker threads
    unsigned int thread_count = std::thread::hardware_concurrency();
    if (thread_count == 0) thread_count = 4;
    
    std::cout << "Running with " << thread_count << " worker threads" << std::endl;
    
    for (unsigned int i = 0; i < thread_count; ++i) {
        worker_threads_.emplace_back(&ProxyServer::worker_thread, this);
    }
    
    // Accept connections in main thread
    accept_connections();
}

void ProxyServer::stop() {
    running_ = false;
    queue_cv_.notify_all();
    
    for (auto& thread : worker_threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void ProxyServer::accept_connections() {
    while (running_) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        socket_t client_socket = accept(listen_socket_, 
                                       (struct sockaddr*)&client_addr, 
                                       &client_len);
        
        if (client_socket == INVALID_SOCKET) {
            if (!running_) break;
#ifdef _WIN32
            std::cerr << "Accept error: " << WSAGetLastError() << std::endl;
#else
            std::cerr << "Accept error: " << strerror(errno) << std::endl;
#endif
            continue;
        }
        
        // Set TCP_NODELAY for low latency
        SocketUtils::set_no_delay(client_socket);
        
        // Create a new session and add to queue
        auto session = std::make_shared<ProxySession>(
            client_socket,
            config_.get_targets(),
            config_.get_buffer_size()
        );
        
        {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            session_queue_.push(session);
        }
        queue_cv_.notify_one();
    }
}

void ProxyServer::worker_thread() {
    while (running_) {
        std::shared_ptr<ProxySession> session;
        
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            queue_cv_.wait(lock, [this] { return !session_queue_.empty() || !running_; });
            
            if (!running_ && session_queue_.empty()) break;
            
            if (!session_queue_.empty()) {
                session = session_queue_.front();
                session_queue_.pop();
            }
        }
        
        if (session) {
            session->start();
        }
    }
}

} // namespace hydra


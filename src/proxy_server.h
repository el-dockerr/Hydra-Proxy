#ifndef HYDRA_PROXY_SERVER_H
#define HYDRA_PROXY_SERVER_H

#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <queue>
#include <condition_variable>
#include "config.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif
typedef SOCKET socket_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
typedef int socket_t;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

namespace hydra {

class SocketUtils {
public:
    static bool initialize();
    static void cleanup();
    static void close_socket(socket_t sock);
    static bool set_non_blocking(socket_t sock);
    static bool set_no_delay(socket_t sock);
    static bool set_reuse_addr(socket_t sock);
};

class ProxySession : public std::enable_shared_from_this<ProxySession> {
public:
    ProxySession(socket_t socket, 
                 const std::vector<Target>& targets,
                 size_t buffer_size);
    
    void start();
    socket_t get_socket() const { return socket_; }

private:
    void handle_client();
    void broadcast_to_targets(const std::vector<char>& data, size_t length);
    
    socket_t socket_;
    const std::vector<Target>& targets_;
    std::vector<char> buffer_;
    size_t buffer_size_;
};

class ProxyServer {
public:
    ProxyServer(const Config& config);
    ~ProxyServer();
    
    void run();
    void stop();

private:
    void accept_connections();
    void worker_thread();
    
    socket_t listen_socket_;
    const Config& config_;
    std::atomic<bool> running_;
    std::vector<std::thread> worker_threads_;
    std::queue<std::shared_ptr<ProxySession>> session_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
};

} // namespace hydra

#endif // HYDRA_PROXY_SERVER_H


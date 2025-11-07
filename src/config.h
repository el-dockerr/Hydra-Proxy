#ifndef HYDRA_CONFIG_H
#define HYDRA_CONFIG_H

#include <string>
#include <vector>
#include <cstdint>

namespace hydra {

struct Target {
    std::string host;
    uint16_t port;
};

class Config {
public:
    Config();
    bool load(const std::string& filename);
    
    uint16_t get_listen_port() const { return listen_port_; }
    size_t get_buffer_size() const { return buffer_size_; }
    const std::vector<Target>& get_targets() const { return targets_; }

private:
    uint16_t listen_port_;
    size_t buffer_size_;
    std::vector<Target> targets_;
};

} // namespace hydra

#endif // HYDRA_CONFIG_H


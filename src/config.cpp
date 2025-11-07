#include "config.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace hydra {

Config::Config() : listen_port_(8080), buffer_size_(65536) {}

// Simple JSON parser for our specific format
bool Config::load(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << filename << std::endl;
        return false;
    }
    
    std::string line;
    std::string content;
    while (std::getline(file, line)) {
        content += line;
    }
    
    // Parse listen_port
    size_t pos = content.find("\"listen_port\"");
    if (pos != std::string::npos) {
        pos = content.find(':', pos);
        if (pos != std::string::npos) {
            std::string num_str;
            pos++;
            while (pos < content.length() && (std::isdigit(content[pos]) || std::isspace(content[pos]))) {
                if (std::isdigit(content[pos])) {
                    num_str += content[pos];
                }
                pos++;
            }
            if (!num_str.empty()) {
                listen_port_ = static_cast<uint16_t>(std::stoi(num_str));
            }
        }
    }
    
    // Parse buffer_size
    pos = content.find("\"buffer_size\"");
    if (pos != std::string::npos) {
        pos = content.find(':', pos);
        if (pos != std::string::npos) {
            std::string num_str;
            pos++;
            while (pos < content.length() && (std::isdigit(content[pos]) || std::isspace(content[pos]))) {
                if (std::isdigit(content[pos])) {
                    num_str += content[pos];
                }
                pos++;
            }
            if (!num_str.empty()) {
                buffer_size_ = std::stoul(num_str);
            }
        }
    }
    
    // Parse targets array
    pos = content.find("\"targets\"");
    if (pos != std::string::npos) {
        pos = content.find('[', pos);
        if (pos != std::string::npos) {
            size_t end_pos = content.find(']', pos);
            if (end_pos != std::string::npos) {
                std::string targets_str = content.substr(pos + 1, end_pos - pos - 1);
                
                // Parse each target object
                size_t obj_start = 0;
                while ((obj_start = targets_str.find('{', obj_start)) != std::string::npos) {
                    size_t obj_end = targets_str.find('}', obj_start);
                    if (obj_end == std::string::npos) break;
                    
                    std::string obj = targets_str.substr(obj_start, obj_end - obj_start + 1);
                    Target target;
                    
                    // Parse host
                    size_t host_pos = obj.find("\"host\"");
                    if (host_pos != std::string::npos) {
                        size_t start = obj.find('\"', host_pos + 6);
                        if (start != std::string::npos) {
                            start++;
                            size_t end = obj.find('\"', start);
                            if (end != std::string::npos) {
                                target.host = obj.substr(start, end - start);
                            }
                        }
                    }
                    
                    // Parse port
                    size_t port_pos = obj.find("\"port\"");
                    if (port_pos != std::string::npos) {
                        port_pos = obj.find(':', port_pos);
                        if (port_pos != std::string::npos) {
                            std::string num_str;
                            port_pos++;
                            while (port_pos < obj.length() && (std::isdigit(obj[port_pos]) || std::isspace(obj[port_pos]))) {
                                if (std::isdigit(obj[port_pos])) {
                                    num_str += obj[port_pos];
                                }
                                port_pos++;
                            }
                            if (!num_str.empty()) {
                                target.port = static_cast<uint16_t>(std::stoi(num_str));
                            }
                        }
                    }
                    
                    if (!target.host.empty() && target.port > 0) {
                        targets_.push_back(target);
                    }
                    
                    obj_start = obj_end + 1;
                }
            }
        }
    }
    
    std::cout << "Configuration loaded:" << std::endl;
    std::cout << "  Listen port: " << listen_port_ << std::endl;
    std::cout << "  Buffer size: " << buffer_size_ << std::endl;
    std::cout << "  Targets: " << targets_.size() << std::endl;
    for (const auto& target : targets_) {
        std::cout << "    - " << target.host << ":" << target.port << std::endl;
    }
    
    return !targets_.empty();
}

} // namespace hydra


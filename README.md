# Hydra - High-Speed HTTP Proxy

Hydra is a blazingly fast C++ proxy server designed to receive HTTP requests and simultaneously forward them to multiple configured servers with minimal latency.

## Features

- ⚡ **Ultra-High Performance**: Optimized for speed with async I/O and zero-delay forwarding
- 🔀 **Simultaneous Broadcasting**: Forwards requests to all configured servers at the exact same time
- 🎯 **Zero Processing Overhead**: Minimal processing between receiving and forwarding
- 🔧 **Easy Configuration**: Simple JSON configuration for ports and targets
- 🧵 **Multi-threaded**: Uses all available CPU cores for maximum throughput
- 🚀 **TCP_NODELAY**: Nagle's algorithm disabled for lowest possible latency

## Performance Optimizations

- Native socket APIs (Winsock2 on Windows, POSIX sockets on Linux/Mac)
- Multi-threaded worker pool using all CPU cores
- Compiler optimizations (O3, LTO, native CPU instructions)
- Large configurable buffers (default 64KB)
- TCP_NODELAY socket option for immediate packet transmission
- No request parsing or processing - raw byte forwarding

## Requirements

- C++17 compatible compiler (MSVC 2019+, GCC 7+, Clang 5+)
- CMake 3.15 or higher
- No external dependencies - uses only standard library and platform socket APIs

## Building on Windows

### Using Bodge
```bash
bodge --platform=XXX
```

Suported Platforms:
* windows_x64
* linux_x64
* apple_x64


### Using PowerShell Build Script

```powershell
.\build.ps1
```

### Manual Build with CMake

```powershell
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## Building on Linux/Mac

### Using Shell Build Script

```bash
chmod +x build.sh
./build.sh
```

### Manual Build with CMake

```bash
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## Configuration

Edit `config.json` to configure the proxy:

```json
{
  "listen_port": 8080,
  "buffer_size": 65536,
  "targets": [
    {
      "host": "127.0.0.1",
      "port": 9001
    },
    {
      "host": "192.168.1.100",
      "port": 8080
    },
    {
      "host": "server3.example.com",
      "port": 80
    }
  ]
}
```

### Configuration Options

- **listen_port**: Port where Hydra listens for incoming connections (default: 8080)
- **buffer_size**: Size of read buffer in bytes (default: 65536 = 64KB)
- **targets**: Array of target servers to forward requests to
  - **host**: IP address or hostname
  - **port**: Port number

## Usage

### Running the Server

```bash
# Run with default config.json
./hydra

# Run with custom config file
./hydra path/to/custom-config.json
```

### Example: Testing with curl

```bash
# Send a request to Hydra (port 8080)
curl -X POST http://localhost:8080/api/test \
  -H "Content-Type: application/json" \
  -d '{"message": "Hello Hydra!"}'
```

This request will be simultaneously forwarded to all configured target servers.

### Example: Setting up Test Targets

You can use simple HTTP servers for testing:

**Python test server:**
```bash
# Terminal 1 - Target server on port 9001
python -m http.server 9001

# Terminal 2 - Target server on port 9002
python -m http.server 9002

# Terminal 3 - Target server on port 9003
python -m http.server 9003

# Terminal 4 - Run Hydra
./hydra
```

## Architecture

### How It Works

1. **Accept Connection**: Hydra accepts incoming TCP connections on the configured port
2. **Read Data**: Reads incoming data into a high-performance buffer
3. **Broadcast**: Immediately spawns threads to forward data to ALL targets simultaneously
4. **No Waiting**: Each target gets its own thread - no serialization or waiting
5. **Continue**: Continues reading more data while forwarding is in progress

### Threading Model

- Main thread handles accept loop
- Worker threads (one per CPU core) handle client connections
- Each broadcast spawns a thread per target for parallel forwarding
- Efficient thread pool and work queue design for maximum throughput

### Network Flow

```
Client Request
     ↓
[Hydra Proxy]
     ↓
   ┌─┴─┬─────┬─────┐
   ↓   ↓     ↓     ↓
Server1 Server2 Server3 ... ServerN
(simultaneous forwarding)
```

## Performance Tips

1. **Buffer Size**: Increase `buffer_size` for larger requests (e.g., file uploads)
2. **Network**: Use low-latency network connections between Hydra and targets
3. **Hardware**: More CPU cores = better concurrent connection handling
4. **OS Limits**: Increase file descriptor limits for handling many connections:
   ```bash
   # Linux
   ulimit -n 65536
   ```

## Troubleshooting

### Connection Refused to Targets

- Ensure target servers are running and accessible
- Check firewall rules
- Verify IP addresses and ports in config.json

### High Memory Usage

- Reduce `buffer_size` if handling many simultaneous connections
- Monitor the number of concurrent client connections

### Slow Performance

- Ensure Release build (not Debug)
- Check network latency to target servers
- Verify compiler optimizations are enabled

## License

MIT License - Feel free to use in commercial projects

## Contributing

Contributions welcome! Areas for improvement:
- Connection pooling to targets
- Health checks for target servers
- Metrics and monitoring
- Response handling (currently fire-and-forget)


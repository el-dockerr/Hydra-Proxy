cmake_minimum_required(VERSION 3.15)
project(Hydra VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler optimizations for maximum speed
if(MSVC)
    add_compile_options(/O2 /Oi /Ot /GL /arch:AVX2)
    add_link_options(/LTCG)
else()
    add_compile_options(-O3 -march=native -flto)
    add_link_options(-flto)
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/config.cpp
    src/proxy_server.cpp
)

set(HEADERS
    src/config.h
    src/proxy_server.h
)

# Main executable
add_executable(hydra ${SOURCES} ${HEADERS})

target_include_directories(hydra PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Enable threading
find_package(Threads REQUIRED)
target_link_libraries(hydra PRIVATE Threads::Threads)

# Windows specific settings
if(WIN32)
    target_link_libraries(hydra PRIVATE ws2_32)
    target_compile_definitions(hydra PRIVATE _WIN32_WINNT=0x0601)
endif()

# Installation
install(TARGETS hydra DESTINATION bin)
install(FILES config.json DESTINATION bin)


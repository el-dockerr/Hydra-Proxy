#!/bin/bash
# Hydra Build Script for Linux/Mac

set -e

echo "=================================="
echo "   Hydra Build Script"
echo "=================================="
echo ""

# Create build directory
if [ -d "build" ]; then
    echo "Cleaning existing build directory..."
    rm -rf build
fi

echo "Creating build directory..."
mkdir build
cd build

# Configure with CMake
echo ""
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
echo ""
echo "Building..."
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Copy config file
echo ""
echo "Copying configuration file..."
cp ../config.json ./config.json

cd ..

echo ""
echo "=================================="
echo "   Build Successful!"
echo "=================================="
echo ""
echo "Executable location: build/hydra"
echo ""
echo "To run Hydra:"
echo "  cd build"
echo "  ./hydra"
echo ""


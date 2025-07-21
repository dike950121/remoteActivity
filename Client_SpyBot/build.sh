#!/bin/bash

echo "========================================"
echo "  Building Remote Activity Spy Bot"
echo "========================================"
echo

# Check if CMake is available
if ! command -v cmake &> /dev/null; then
    echo "ERROR: CMake is not installed"
    echo "Please install CMake:"
    echo "  Ubuntu/Debian: sudo apt-get install cmake"
    echo "  CentOS/RHEL: sudo yum install cmake"
    echo "  Arch: sudo pacman -S cmake"
    exit 1
fi

# Check if g++ is available
if ! command -v g++ &> /dev/null; then
    echo "ERROR: g++ is not installed"
    echo "Please install build-essential:"
    echo "  Ubuntu/Debian: sudo apt-get install build-essential"
    echo "  CentOS/RHEL: sudo yum groupinstall 'Development Tools'"
    exit 1
fi

# Create build directory
mkdir -p build
cd build

echo "Configuring project with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release
if [ $? -ne 0 ]; then
    echo "ERROR: CMake configuration failed"
    exit 1
fi

echo "Building project..."
make -j$(nproc)
if [ $? -ne 0 ]; then
    echo "ERROR: Build failed"
    exit 1
fi

echo
echo "========================================"
echo "  Build completed successfully!"
echo "  Executable: build/bin/SpyBot"
echo "  Config file: build/bin/config.json"
echo "========================================"
echo

# Copy config file if it doesn't exist in bin directory
if [ ! -f "bin/config.json" ] && [ -f "../config.json" ]; then
    cp "../config.json" "bin/"
    echo "Config file copied to build directory."
fi

echo "Run './bin/SpyBot' to start the spy bot client." 
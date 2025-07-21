#!/bin/bash

echo "========================================"
echo "  Building Remote Activity Spy Bot"
echo "========================================"
echo

# Check if make is available
if ! command -v make &> /dev/null; then
    echo "ERROR: make is not installed"
    echo "Please install make:"
    echo "  Ubuntu/Debian: sudo apt-get install make"
    echo "  CentOS/RHEL: sudo yum install make"
    echo "  Arch: sudo pacman -S make"
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

mkdir -p bin

echo "Building project..."
make -j$(nproc)
if [ $? -ne 0 ]; then
    echo "ERROR: Build failed"
    exit 1
fi

echo
echo "========================================"
echo "  Build completed successfully!"
echo "  Executable: bin/SpyBot"
echo "  Config file: bin/config.json"
echo "========================================"
echo

# Copy config file if it doesn't exist in bin directory
if [ ! -f "bin/config.json" ] && [ -f "config.json" ]; then
    cp "config.json" "bin/"
    echo "Config file copied to bin directory."
fi

echo "Run './bin/SpyBot' to start the spy bot client." 
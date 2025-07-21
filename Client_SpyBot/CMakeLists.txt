cmake_minimum_required(VERSION 3.10)
project(SpyBot VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(include)

# Find required packages
find_package(Threads REQUIRED)

# Add executable
add_executable(SpyBot
    src/main.cpp
    src/NetworkClient.cpp
    src/SystemInfo.cpp
    src/DataCollector.cpp
    src/ConfigManager.cpp
)

# Link libraries
target_link_libraries(SpyBot 
    Threads::Threads
)

# Windows specific libraries
if(WIN32)
    target_link_libraries(SpyBot ws2_32 wsock32 iphlpapi psapi)
endif()

# Linux specific libraries  
if(UNIX AND NOT APPLE)
    target_link_libraries(SpyBot pthread)
endif()

# Set output directory
set_target_properties(SpyBot PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Copy config file to build directory
configure_file(${CMAKE_SOURCE_DIR}/config.json ${CMAKE_BINARY_DIR}/bin/config.json COPYONLY) 
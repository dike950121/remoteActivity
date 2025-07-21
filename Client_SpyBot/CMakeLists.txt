cmake_minimum_required(VERSION 3.10)
project(SpyBot VERSION 1.0.0)

# Explicitly set MSVC compiler
if(MSVC)
    # Use the latest C++ standard supported by MSVC
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    # Add compiler-specific flags for better security and warnings
    add_compile_options(
        /W4           # Enable high-level warnings
        /WX           # Treat warnings as errors
        /utf-8        # Ensure UTF-8 source and execution character sets
        /permissive-  # Strict standard conformance
        /Zc:preprocessor  # Use standard-conforming preprocessor
    )

    # Additional security-focused compiler flags
    add_compile_definitions(
        _CRT_SECURE_NO_WARNINGS  # Disable deprecation warnings for standard C functions
        _SILENCE_CXX17_RESULT_OF_DEPRECATION_WARNING  # Silence specific deprecation warnings
    )
endif()

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
cmake_minimum_required(VERSION 3.10)
project(RemoteClient C)

set(CMAKE_C_STANDARD 99)

# Add include directory
include_directories(include)

# Set source files
set(SOURCE_FILES
    src/main.c
    src/network.c
    src/logger.c
)

# Create executable
add_executable(RemoteClient ${SOURCE_FILES})

# Link Windows Socket library
target_link_libraries(RemoteClient wsock32 ws2_32)

# Ensure logs directory is created during build
add_custom_command(TARGET RemoteClient POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/logs
    COMMENT "Creating logs directory"
)
cmake_minimum_required(VERSION 3.10)
project(RemoteClient C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Add source files
add_executable(remote_client 
    src/main.c
    src/network.c
)

# Include directories
target_include_directories(remote_client PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Windows-specific libraries
if(WIN32)
    target_link_libraries(remote_client PRIVATE 
        ws2_32  # Windows Sockets
    )
endif()
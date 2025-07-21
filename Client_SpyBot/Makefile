# Makefile for SpyBot project

# Compiler to use
CXX = g++

# Compiler flags
CXXFLAGS = -std=c++17 -Wall -Iinclude

# Source files
SOURCES = src/main.cpp src/ConfigManager.cpp src/DataCollector.cpp src/NetworkClient.cpp src/SystemInfo.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# OS-specific settings
ifeq ($(OS),Windows_NT)
    EXECUTABLE = bin/SpyBot.exe
    LDFLAGS = -lws2_32 -lwsock32 -liphlpapi -lpsapi
else
    EXECUTABLE = bin/SpyBot
    LDFLAGS = -lpthread
endif

# Default target
all: $(EXECUTABLE)

# Link objects to create executable
$(EXECUTABLE): $(OBJECTS)
	@mkdir -p bin
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $(EXECUTABLE)

# Compile source to object
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up build artifacts
clean:
	rm -f $(OBJECTS) $(EXECUTABLE) 
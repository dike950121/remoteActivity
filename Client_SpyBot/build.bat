@echo off
echo ========================================
echo  Building Remote Activity Spy Bot with MinGW
echo ========================================
echo.

REM Check if g++ is available
g++ --version >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ERROR: g++ is not installed or not in PATH
    echo Please install MinGW-w64 and add it to your PATH.
    pause
    exit /b 1
)

REM Create build directory
if not exist "build" mkdir build

echo Compiling project with g++...
g++ -std=c++17 -Iinclude -D_WIN32_WINNT=0x0600 -o build/SpyBot.exe src/main.cpp src/NetworkClient.cpp src/SystemInfo.cpp src/DataCollector.cpp src/ConfigManager.cpp -lws2_32 -lwsock32 -liphlpapi -lpsapi -static-libgcc -static-libstdc++ -lwinmm
if %ERRORLEVEL% neq 0 (
    echo ERROR: Compilation failed
    pause
    exit /b 1
)

echo.
echo ========================================
echo  Build completed successfully!
echo  Executable: build\SpyBot.exe
echo ========================================
echo.

REM Copy config file if it doesn't exist in build directory
if not exist "build\config.json" (
    if exist "config.json" (
        copy "config.json" "build\" >nul
        echo Config file copied to build directory.
    )
)

echo "Running SpyBot..."
start "" "build\SpyBot.exe"

pause 
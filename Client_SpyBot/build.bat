@echo off
echo ========================================
echo  Building Remote Activity Spy Bot
echo ========================================
echo.

REM Check if CMake is available
cmake --version >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake is not installed or not in PATH
    echo Please install CMake from https://cmake.org/download/
    pause
    exit /b 1
)

REM Create build directory
if not exist "build" mkdir build
cd build

echo Configuring project with CMake...
cmake .. -G "Visual Studio 17 2022" -A x64
if %ERRORLEVEL% neq 0 (
    echo ERROR: CMake configuration failed
    pause
    exit /b 1
)

echo Building project...
cmake --build . --config Release
if %ERRORLEVEL% neq 0 (
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo.
echo ========================================
echo  Build completed successfully!
echo  Executable: build\bin\Release\SpyBot.exe
echo  Config file: build\bin\config.json
echo ========================================
echo.

REM Copy config file if it doesn't exist in bin directory
if not exist "bin\config.json" (
    if exist "..\config.json" (
        copy "..\config.json" "bin\" >nul
        echo Config file copied to build directory.
    )
)

pause 
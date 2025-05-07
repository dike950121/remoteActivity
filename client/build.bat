@echo off
setlocal enabledelayedexpansion

echo [%DATE% %TIME%] Build process started
echo [%DATE% %TIME%] Checking GCC installation...

:: Check if gcc (MinGW) is installed
gcc --version > gcc_version.txt 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [%DATE% %TIME%] ERROR: GCC ^(MinGW^) is not installed or not in PATH
    echo [%DATE% %TIME%] Please install MinGW and add it to your system PATH
    type gcc_version.txt
    del gcc_version.txt
    pause
    exit /b 1
)
type gcc_version.txt
del gcc_version.txt

:: Create build directory if it doesn't exist
echo [%DATE% %TIME%] Setting up build directory...
if not exist build (
    mkdir build
    if !ERRORLEVEL! NEQ 0 (
        echo [%DATE% %TIME%] ERROR: Failed to create build directory
        pause
        exit /b 1
    )
    echo [%DATE% %TIME%] Created build directory
) else (
    echo [%DATE% %TIME%] Build directory already exists
)

:: Clean previous build
echo [%DATE% %TIME%] Cleaning previous build...
if exist build\remote_client.exe (
    del /f /q build\remote_client.exe
    if !ERRORLEVEL! NEQ 0 (
        echo [%DATE% %TIME%] ERROR: Failed to remove previous executable
        pause
        exit /b 1
    )
    echo [%DATE% %TIME%] Removed previous executable
)

:: Set source and include paths with proper Windows backslashes
set "SOURCES=src\main.c src\network.c"
set "INCLUDE_DIR=include"
set "OUTPUT=build\remote_client.exe"

:: Compile the project with detailed output
echo [%DATE% %TIME%] Compiling the project with MinGW...
echo [%DATE% %TIME%] Command: gcc -D_WIN32_WINNT=0x0600 %SOURCES% -I%INCLUDE_DIR% -o %OUTPUT% -Wall -Wextra -lws2_32

gcc -D_WIN32_WINNT=0x0600 %SOURCES% -I%INCLUDE_DIR% -o %OUTPUT% -Wall -Wextra -lws2_32 > build_log.txt 2>&1

if %ERRORLEVEL% NEQ 0 (
    echo [%DATE% %TIME%] ERROR: Compilation failed with error code %ERRORLEVEL%
    echo [%DATE% %TIME%] Build log:
    echo ----------------------------------------
    type build_log.txt
    echo ----------------------------------------
    del build_log.txt
    pause
    exit /b 1
)

del build_log.txt
echo [%DATE% %TIME%] Compilation completed successfully

:: Verify the executable
echo [%DATE% %TIME%] Verifying executable...
if not exist %OUTPUT% (
    echo [%DATE% %TIME%] ERROR: Compilation completed but executable not found
    pause
    exit /b 1
)

:: Show executable details
echo [%DATE% %TIME%] Executable information:
dir %OUTPUT%

:: Run the client
echo.
echo [%DATE% %TIME%] Build successful! Starting the client...
echo.
echo [%DATE% %TIME%] Running %OUTPUT%
echo ----------------------------------------
"%OUTPUT%"
if !ERRORLEVEL! NEQ 0 (
    echo [%DATE% %TIME%] ERROR: Client execution failed with code !ERRORLEVEL!
    pause
    exit /b !ERRORLEVEL!
)
echo ----------------------------------------
echo [%DATE% %TIME%] Client execution completed

pause
@echo off
echo ========================================
echo  Building Remote Activity Spy Bot
echo ========================================
echo.

REM Check if g++ is available
g++ --version >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ERROR: g++ (MinGW) is not installed or not in PATH
    echo Please install MinGW-w64 from https://www.mingw-w64.org/downloads/
    pause
    exit /b 1
)

if not exist "bin" mkdir bin

echo Building project with MinGW...
mingw32-make
if %ERRORLEVEL% neq 0 (
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo.
echo ========================================
echo  Build completed successfully!
echo  Executable: bin\SpyBot.exe
echo  Config file: bin\config.json
echo ========================================
echo.

if not exist "bin\config.json" (
    if exist "config.json" (
        copy "config.json" "bin\" >nul
        echo Config file copied to bin directory.
    )
)

pause 
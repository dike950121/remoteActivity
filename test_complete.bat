@echo off
echo Building WPF Server...
cd server
dotnet build
if %errorlevel% neq 0 (
    echo ERROR: Failed to build WPF server
    pause
    exit /b 1
)

echo Building C++ Bot Client...
cd ..\bot
call build.bat

echo.
echo To test:
echo 1. cd server && dotnet run
echo 2. Click "Start Server" in the UI
echo 3. cd bot && simple_bot.exe
echo.
pause 
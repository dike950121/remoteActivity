@echo off
echo ========================================
echo Persistent Bot Test
echo ========================================
echo.

echo Building components...
cd server
dotnet build
cd ..\bot
call build.bat

echo.
echo Instructions:
echo 1. Start server: cd server && dotnet run
echo 2. Click "Start Server" in the UI
echo 3. Run bot: cd bot && simple_bot.exe
echo.
echo The bot will:
echo - Connect to server automatically
echo - Send messages every 10 seconds
echo - Auto-reconnect if disconnected
echo - Continue running until Ctrl+C
echo.
pause 
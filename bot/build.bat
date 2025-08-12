@echo off
echo Building Modular TCP Bot Client...

where g++ >nul 2>nul
if %errorlevel% equ 0 (
    g++ main.cpp system_info.cpp network_client.cpp bot_controller.cpp -o modular_bot.exe -lws2_32 -lwininet
    if %errorlevel% equ 0 (
        echo Build successful! Created modular_bot.exe
    ) else (
        echo Build failed!
    )
    goto :end
)

where cl >nul 2>nul
if %errorlevel% equ 0 (
    cl main.cpp system_info.cpp network_client.cpp bot_controller.cpp /EHsc /Fe:modular_bot.exe
    if %errorlevel% equ 0 (
        echo Build successful! Created modular_bot.exe
    ) else (
        echo Build failed!
    )
    goto :end
)

echo ERROR: No C++ compiler found! Install MinGW or Visual Studio.

:end
pause 
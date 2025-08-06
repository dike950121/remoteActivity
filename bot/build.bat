@echo off
echo Building Simple TCP Bot Client...

where g++ >nul 2>nul
if %errorlevel% equ 0 (
    g++ simple_bot.cpp -o simple_bot.exe -lws2_32
    if %errorlevel% equ 0 (
        echo Build successful! Created simple_bot.exe
    ) else (
        echo Build failed!
    )
    goto :end
)

where cl >nul 2>nul
if %errorlevel% equ 0 (
    cl simple_bot.cpp /EHsc /Fe:simple_bot.exe
    if %errorlevel% equ 0 (
        echo Build successful! Created simple_bot.exe
    ) else (
        echo Build failed!
    )
    goto :end
)

echo ERROR: No C++ compiler found! Install MinGW or Visual Studio.

:end
pause 
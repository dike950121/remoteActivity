@echo off
REM Build and run main.c using MinGW (gcc)
setlocal
set MINGW_PATH=C:\MinGW\bin
set PATH=%MINGW_PATH%;%PATH%
if not exist "%MINGW_PATH%\gcc.exe" (
    echo MinGW GCC not found in %MINGW_PATH%.
    echo Please install MinGW and ensure gcc.exe is in C:\MinGW\bin.
    pause
    exit /b 1
)
echo Compiling main.c...
gcc main.c -o bot.exe -lws2_32 -lgdi32
if errorlevel 1 (
    echo Compilation failed.
    pause
    exit /b 1
)
echo Running bot.exe...
bot.exe
endlocal
pause
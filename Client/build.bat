@echo off
REM build.bat - Build script for the client payload
REM Uses MinGW g++ compiler located at C:\MinGW\bin

echo Building Client Payload...
echo.

REM Set compiler path and flags
set COMPILER=C:\MinGW\bin\g++.exe
set CFLAGS=-std=c++11 -Wall -Wextra -O2
set INCLUDES=-Iinclude
set SOURCES=src\*.cpp
set OUTPUT=bin\client.exe

REM Check if MinGW is available
if not exist "%COMPILER%" (
    echo ERROR: MinGW compiler not found at %COMPILER%
    echo Please install MinGW and ensure it's in your PATH or update the COMPILER path
    pause
    exit /b 1
)

REM Create bin directory if it doesn't exist
if not exist "bin" mkdir bin

REM Compile the project
echo Compiling source files...
%COMPILER% %CFLAGS% %INCLUDES% %SOURCES% -o %OUTPUT%

REM Check if compilation was successful
if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build successful! Executable created at: %OUTPUT%
    echo.
    echo To run the client:
    echo   %OUTPUT%
) else (
    echo.
    echo Build failed! Please check the error messages above.
)

pause 
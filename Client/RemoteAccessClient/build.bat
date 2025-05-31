@echo off
echo Building RemoteAccessClient with Visual Studio 2022 MSVC...

:: Set up Visual Studio environment
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

:: Create output directory
if not exist "bin" mkdir bin

:: Compile with MSVC (cl.exe)
cl /std:c++17 /O2 /EHsc /DWIN32_LEAN_AND_MEAN /DNOMINMAX /D_WIN32_WINNT=0x0601 ^
    /I"include" ^
    src\main.cpp ^
    src\network\NetworkManager.cpp ^
    src\common\Logger.cpp ^
    src\common\Protocol.cpp ^
    src\system\SystemInfo.cpp ^
    /Fe:bin\RemoteAccessClient.exe ^
    /link ws2_32.lib wininet.lib advapi32.lib kernel32.lib user32.lib shell32.lib ^
    ole32.lib oleaut32.lib uuid.lib crypt32.lib pdh.lib psapi.lib iphlpapi.lib ^
    netapi32.lib wtsapi32.lib userenv.lib version.lib

if %ERRORLEVEL% EQU 0 (
    echo Build successful! Executable created at bin\RemoteAccessClient.exe
) else (
    echo Build failed with error code %ERRORLEVEL%
)

pause
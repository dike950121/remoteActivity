@echo off
echo ========================================
echo  Building Remote Activity Server
echo ========================================
echo.

REM Check if .NET 8 SDK is available
dotnet --version >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo ERROR: .NET SDK is not installed
    echo Please install .NET 8 SDK from https://dotnet.microsoft.com/download
    pause
    exit /b 1
)

cd RemoteActivityServer

echo Restoring NuGet packages...
dotnet restore --source "https://api.nuget.org/v3/index.json" --property:Http_Proxy=http://upjwcjet:96l8gn49xuw6@31.58.19.50:6322
if %ERRORLEVEL% neq 0 (
    echo ERROR: Package restore failed
    pause
    exit /b 1
)

echo Building project...
dotnet build --configuration Release --no-restore
if %ERRORLEVEL% neq 0 (
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo.
echo ========================================
echo  Build completed successfully!
echo  Run: dotnet run --project RemoteActivityServer
echo  Or publish: dotnet publish -c Release -o publish
echo ========================================
echo.

pause 
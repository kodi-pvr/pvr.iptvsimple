@echo off
setlocal enabledelayedexpansion

:: Variables
set "app_id=pvr.iptvsimple"
set "GENERATOR=Visual Studio 17 2022"
set "CONFIGURATION=Release"

:: Get user architecture selection
echo Select architecture:
echo 1. Win32
echo 2. Win64
echo 3. Win32-UWP
echo 4. Win64-UWP
@REM echo 5. ARM32-UWP
set /p choice="Please make your choice (1-5): "
if "%choice%"=="1" (
    set "ARCHITECTURE=Win32"
    set "WINSTORE="
) else if "%choice%"=="2" (
    set "ARCHITECTURE=x64"
    set "WINSTORE="
) else if "%choice%"=="3" (
    set "ARCHITECTURE=Win32"
    set "WINSTORE=-DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=10.0.17763.0"
) else if "%choice%"=="4" (
    set "ARCHITECTURE=x64"
    set "WINSTORE=-DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=10.0.17763.0"
)
@REM  else if "%choice%"=="5" (
@REM     set "ARCHITECTURE=ARM"
@REM     set "WINSTORE=-DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=10.0.17763.0"
@REM ) 
else (
    echo Invalid choice
    exit /b 1
)

:: Clone Kodi
cd ..
if not exist kodi (
    git clone --branch master --depth=1 https://github.com/xbmc/xbmc.git kodi
)
cd %app_id%

:: Create build folder
if not exist build mkdir build
cd build

:: Create definition folder
if not exist "definition\%app_id%" mkdir "definition\%app_id%"
echo %app_id% . . > "definition\%app_id%\%app_id%.txt"

:: Create symbolic link (requires administrator privileges)
mklink /J "%app_id%" ".."

:: CMake configuration
cmake -T host=x64 -G "%GENERATOR%" -A %ARCHITECTURE% %WINSTORE% ^
    -DADDONS_TO_BUILD=%app_id% ^
    -DCMAKE_BUILD_TYPE=%CONFIGURATION% ^
    -DADDONS_DEFINITION_DIR="%CD%\definition" ^
    -DADDON_SRC_PREFIX=../.. ^
    -DCMAKE_INSTALL_PREFIX=../../kodi/addons ^
    -DPACKAGE_ZIP=1 ^
    ../../kodi/cmake/addons

:: Build the project
cmake --build . --config %CONFIGURATION% --target %app_id%

pause 

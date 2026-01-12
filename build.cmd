@echo off
setlocal EnableDelayedExpansion

:: =============================================================================
:: Links Build Script for Windows
:: Usage:
::   build.cmd [command]
::
:: Commands:
::   (none)    - Default Debug build
::   debug     - Debug build
::   release   - Release build
::   clean     - Clean build directory
::   test      - Run tests
::   help      - Show this help
:: =============================================================================

set "PROJECT_ROOT=%~dp0"
set "VCPKG_DIR=%PROJECT_ROOT%extern\vcpkg"
set "BUILD_DIR=%PROJECT_ROOT%build"

:: Setup Visual Studio environment if not already set
if not defined VSINSTALLDIR (
    echo [*] Setting up Visual Studio environment...
    set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
    if exist "!VSWHERE!" (
        for /f "usebackq tokens=*" %%i in (`"!VSWHERE!" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VS_PATH=%%i"
        if defined VS_PATH (
            call "!VS_PATH!\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
            echo [+] Visual Studio environment ready
        ) else (
            echo [!] Visual Studio with C++ tools not found
            exit /b 1
        )
    ) else (
        echo [!] vswhere.exe not found, please run from Developer Command Prompt
        exit /b 1
    )
)

:: Parse command
set "COMMAND=%~1"
if "%COMMAND%"=="" set "COMMAND=debug"

:: Handle commands
if /i "%COMMAND%"=="help" goto :show_help
if /i "%COMMAND%"=="clean" goto :clean
if /i "%COMMAND%"=="test" goto :run_tests
if /i "%COMMAND%"=="debug" goto :build_debug
if /i "%COMMAND%"=="release" goto :build_release

echo Unknown command: %COMMAND%
goto :show_help

:: -----------------------------------------------------------------------------
:show_help
:: -----------------------------------------------------------------------------
echo.
echo Links Build Script
echo ==================
echo.
echo Usage: build.cmd [command]
echo.
echo Commands:
echo   debug     - Build Debug version (default)
echo   release   - Build Release version
echo   clean     - Clean build directory
echo   test      - Run tests
echo   help      - Show this help
echo.
goto :eof

:: -----------------------------------------------------------------------------
:check_vcpkg
:: -----------------------------------------------------------------------------
echo [*] Checking vcpkg...

if not exist "%VCPKG_DIR%" (
    echo [*] vcpkg not found, cloning...
    git clone https://github.com/microsoft/vcpkg.git "%VCPKG_DIR%"
    if errorlevel 1 (
        echo [!] Failed to clone vcpkg
        exit /b 1
    )
)

if not exist "%VCPKG_DIR%\vcpkg.exe" (
    echo [*] Bootstrapping vcpkg...
    call "%VCPKG_DIR%\bootstrap-vcpkg.bat" -disableMetrics
    if errorlevel 1 (
        echo [!] Failed to bootstrap vcpkg
        exit /b 1
    )
)

echo [+] vcpkg is ready
goto :eof

:: -----------------------------------------------------------------------------
:build_debug
:: -----------------------------------------------------------------------------
call :check_vcpkg
if errorlevel 1 exit /b 1

echo.
echo [*] Configuring Debug build...
cmake --preset debug
if errorlevel 1 (
    echo [!] CMake configuration failed
    exit /b 1
)

echo.
echo [*] Building Debug...
cmake --build --preset debug
if errorlevel 1 (
    echo [!] Build failed
    exit /b 1
)

echo.
echo [+] Debug build completed successfully
goto :eof

:: -----------------------------------------------------------------------------
:build_release
:: -----------------------------------------------------------------------------
call :check_vcpkg
if errorlevel 1 exit /b 1

echo.
echo [*] Configuring Release build...
cmake --preset release
if errorlevel 1 (
    echo [!] CMake configuration failed
    exit /b 1
)

echo.
echo [*] Building Release...
cmake --build --preset release
if errorlevel 1 (
    echo [!] Build failed
    exit /b 1
)

echo.
echo [+] Release build completed successfully
goto :eof

:: -----------------------------------------------------------------------------
:clean
:: -----------------------------------------------------------------------------
echo [*] Cleaning build directory...
if exist "%BUILD_DIR%" (
    rmdir /s /q "%BUILD_DIR%"
    echo [+] Build directory cleaned
) else (
    echo [*] Build directory does not exist
)
goto :eof

:: -----------------------------------------------------------------------------
:run_tests
:: -----------------------------------------------------------------------------
if not exist "%BUILD_DIR%" (
    echo [!] Build directory not found. Please build first.
    exit /b 1
)

echo [*] Running tests...
ctest --preset default
if errorlevel 1 (
    echo [!] Some tests failed
    exit /b 1
)

echo [+] All tests passed
goto :eof

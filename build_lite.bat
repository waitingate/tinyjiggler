@echo off
setlocal
cd /d "%~dp0"

rem ---- Where is your g++? -------------------------------------------------
rem Portable GCC (WinLibs, w64devkit...): the folder that contains g++.exe.
rem If g++ is already on your PATH, this line does no harm.
rem Other folder? Edit it here, or run:  set GCC_DIR=D:\your\gcc\bin
if not defined GCC_DIR set "GCC_DIR=C:\mingw64\bin"
rem ------------------------------------------------------------------------

if exist "%GCC_DIR%\g++.exe" set "PATH=%GCC_DIR%;%PATH%"
g++ --version >nul 2>&1
if errorlevel 1 (
    echo g++ not found. Edit GCC_DIR at the top of %~nx0
    goto :fail
)
rem The program starts at Start. A 32-bit g++ names it _Start, and the linker
rem then only warns and builds a program that starts in the wrong place.
g++ -dumpmachine | findstr /b /l "x86_64- aarch64-" >nul
if errorlevel 1 (
    echo This g++ does not build 64-bit programs. Use a 64-bit MinGW-w64 g++.
    goto :fail
)

g++ -std=c++17 -Os -s -nostdlib -nostartfiles -e Start -Wl,--subsystem,windows -Wl,--gc-sections ^
    -fno-exceptions -fno-rtti -fno-asynchronous-unwind-tables -fno-stack-protector ^
    -o TinyJigglerLite.exe tinyjiggler_lite.cpp -lkernel32 -luser32
if errorlevel 1 goto :fail

echo Built TinyJigglerLite.exe
pause
exit /b 0

:fail
echo Build failed.
pause
exit /b 1

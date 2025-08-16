@echo off
setlocal enabledelayedexpansion
echo Checking pre-requisites... 

:: Check if CMake is installed
cmake --version > nul 2>&1
if %errorlevel% NEQ 0 (
    echo Cannot find path to cmake. Is CMake installed? Exiting...
    exit /b -1
) else (
    echo    CMake      - Ready.
)

echo Dotnet version:
dotnet --version
echo    Dotnet      - Ready.

echo Python version:
py --version
echo    Python      - Ready.

git submodule update --init

mkdir _build
cd _build

echo Creating venv...
py -m venv venv
echo venv created

echo Generating project...
cmake -G "Visual Studio 17 2022" -A x64 ..
echo Done.
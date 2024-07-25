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

echo Python version:
py --version
echo    Python      - Ready.

echo Pip version:
pip --version
echo    Pip      - Ready.

echo Git version:
git --version
echo    Git      - Ready.

echo Dotnet version:
dotnet --version
echo    Dotnet      - Ready.

mkdir _build

echo Copiyng external resources...

xcopy resources\imgui.ini _build\apps\EraEditor\ /Q
xcopy resources\imgui.ini _build\apps\EraRuntime\ /Q
xcopy resources\imgui.ini _build\ /Q

xcopy /s /i resources\bin\Debug _build\Debug /Q

xcopy /s /i resources\bin\Release _build\Release /Q

cd _build
echo Creating venv...
py -m venv venv
echo venv created

echo Building project...
cmake -DCMAKE_BUILD_TYPE=Release ..
echo Done.
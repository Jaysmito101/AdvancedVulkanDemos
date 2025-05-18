@echo off
cls
@REM check cmake is installed
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo CMake is not installed. Please install CMake and add it to your PATH.
    exit /b 1
)
@REM check if build/CMakeCache.txt exists, if not run the cmake -S . -B build
if not exist build\CMakeCache.txt (
    echo CMakeCache.txt not found. Running cmake to generate build files...
    cmake -S . -B build
    if %errorlevel% neq 0 (
        echo CMake configuration failed.
        exit /b 1
    )
) else (
    echo CMakeCache.txt found. Skipping cmake configuration.
)
echo Generating assets...
python tools/assets.py
echo Building the project...
@REM if a if there is --release in the command line, run cmake --build build --config Release
if "%1"=="--release" (
    cmake --build build --config Release
) else (
    cmake --build build
)
if %errorlevel% neq 0 (
    echo Build failed.
    exit /b 1
)
@REM run the program (debug or release)
echo Running the program...
if "%1"=="--release" (
    .\build\avd\Release\avd.exe
) else (
    .\build\avd\Debug\avd.exe
)
@REM check if the program ran successfully
if %errorlevel% neq 0 (
    echo Program failed to run.
    exit /b 1
)
echo Program ran successfully.

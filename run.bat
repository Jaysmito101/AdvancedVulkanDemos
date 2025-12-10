@echo off
cls
@REM check cmake is installed
where cmake >nul 2>nul
if %errorlevel% neq 0 (
    echo CMake is not installed. Please install CMake and add it to your PATH.
    exit /b 1
)

@REM Parse command line arguments to determine build type and toolchain
set BUILD_TYPE=Debug
set TOOLCHAIN=vs
set BUILD_DIR=build.vs

:parse_args
if "%1"=="" goto end_parse
if "%1"=="--release" (
    set BUILD_TYPE=Release
    shift
    goto parse_args
)
if "%1"=="--mingw" (
    set TOOLCHAIN=mingw
    set BUILD_DIR=build
    shift
    goto parse_args
)
if "%1"=="--vs" (
    set TOOLCHAIN=vs
    set BUILD_DIR=build.vs
    shift
    goto parse_args
)
if "%1"=="--ninja" (
    set TOOLCHAIN=ninja
    set BUILD_DIR=build.ninja
    shift
    goto parse_args
)
shift
goto parse_args
:end_parse

echo Using %TOOLCHAIN% toolchain with %BUILD_TYPE% configuration
echo Build directory: %BUILD_DIR%

@REM check if build directory and CMakeCache.txt exists, if not run cmake
if not exist %BUILD_DIR%\CMakeCache.txt (
    echo CMakeCache.txt not found in %BUILD_DIR%. Running cmake to generate build files...
    if "%TOOLCHAIN%"=="mingw" (
        cmake -S . -B %BUILD_DIR% -G "MinGW Makefiles"
    ) else if "%TOOLCHAIN%"=="ninja" (
        cmake -S . -B %BUILD_DIR% -G "Ninja"
    ) else (
        cmake -S . -B %BUILD_DIR%
    )
    if %errorlevel% neq 0 (
        echo CMake configuration failed.
        exit /b 1
    )
) else (
    echo CMakeCache.txt found in %BUILD_DIR%. Skipping cmake configuration.
)

echo Generating assets...
python tools/assets.py

echo Building the project...
if "%BUILD_TYPE%"=="Release" (
    cmake --build %BUILD_DIR% --config Release
) else (
    cmake --build %BUILD_DIR%
)
if %errorlevel% neq 0 (
    echo Build failed.
    exit /b 1
)

@REM run the program (debug or release)
echo Running the program...
if "%TOOLCHAIN%"=="mingw" OR "%TOOLCHAIN%"=="ninja" (
    if "%BUILD_TYPE%"=="Release" (
        .\%BUILD_DIR%\avd\avd.exe
    ) else (
        .\%BUILD_DIR%\avd\avd.exe
    )
) else (
    if "%BUILD_TYPE%"=="Release" (
        .\%BUILD_DIR%\avd\Release\avd.exe
    ) else (
        .\%BUILD_DIR%\avd\Debug\avd.exe
    )
)

@REM check if the program ran successfully
if %errorlevel% neq 0 (
    echo Program failed to run.
    exit /b 1
)
echo Program ran successfully.

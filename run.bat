@echo off
if /i "%~1"=="--help" goto show_help
if /i "%~1"=="-h" goto show_help
if /i "%~1"=="/?" goto show_help
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
set CMAKE_EXTRA_ARGS=
set BUILD_EXTRA_ARGS=

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
if /i "%1"=="--help" goto show_help
if /i "%1"=="-h" goto show_help
if /i "%1"=="/?" goto show_help
if "%1"=="--cmake-opts" (
    shift
    if "%1"=="" (
        echo Missing value for --cmake-opts.
        echo Use --help to see usage.
        exit /b 1
    )
    goto append_cmake_args
)
if "%1"=="--" (
    shift
    goto append_cmake_args
)
if "%1"=="--build-opts" (
    shift
    if "%1"=="" (
        echo Missing value for --build-opts.
        echo Use --help to see usage.
        exit /b 1
    )
    goto append_build_args
)
echo Unknown argument: %1
echo Use --help to see usage.
exit /b 1

:append_cmake_args
if "%1"=="" goto end_parse
if "%1"=="--build-opts" (
    shift
    goto append_build_args
)
set "CURRENT_CMAKE_ARG=%~1"
if /i "%CURRENT_CMAKE_ARG:~0,2%"=="-D" (
    echo(%CURRENT_CMAKE_ARG%| findstr /c:"=" >nul
    if errorlevel 1 (
        if not "%~2"=="" (
            if not "%~2:~0,1%"=="-" (
                set "CURRENT_CMAKE_ARG=%CURRENT_CMAKE_ARG%=%~2"
                shift
            )
        )
    )
)
if "%CMAKE_EXTRA_ARGS%"=="" (
    set "CMAKE_EXTRA_ARGS=%CURRENT_CMAKE_ARG%"
) else (
    set "CMAKE_EXTRA_ARGS=%CMAKE_EXTRA_ARGS% %CURRENT_CMAKE_ARG%"
)
shift
goto append_cmake_args

:append_build_args
if "%1"=="" goto end_parse
if "%BUILD_EXTRA_ARGS%"=="" (
    set BUILD_EXTRA_ARGS=%1
) else (
    set BUILD_EXTRA_ARGS=%BUILD_EXTRA_ARGS% %1
)
shift
goto append_build_args

:end_parse

cls


echo Using %TOOLCHAIN% toolchain with %BUILD_TYPE% configuration
echo Build directory: %BUILD_DIR%
if not "%CMAKE_EXTRA_ARGS%"=="" echo CMake options: %CMAKE_EXTRA_ARGS%
if not "%BUILD_EXTRA_ARGS%"=="" echo Build options: %BUILD_EXTRA_ARGS%

echo Generating assets...
python tools/assets.py

@REM Check if assets changed and invalidate cache
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
set "ASSET_HASH_FILE=avd_assets\generated\.assetshash"
set "BUILD_ASSET_HASH_FILE=%BUILD_DIR%\.assetshash"
set "INVALIDATE_CACHE=0"

rem Persist last CMake options and invalidate cache if they change
set "CMAKE_OPTS_FILE=%BUILD_DIR%\.cmake_opts"
set "LAST_CMAKE_OPTS="
if exist "%CMAKE_OPTS_FILE%" (
    for /f "usebackq delims=" %%A in ("%CMAKE_OPTS_FILE%") do set "LAST_CMAKE_OPTS=%%A"
)
if not "%CMAKE_EXTRA_ARGS%"=="%LAST_CMAKE_OPTS%" (
    if not "%CMAKE_EXTRA_ARGS%"=="" (
        echo CMake options changed to: %CMAKE_EXTRA_ARGS%. Invalidating CMake cache...
    ) else (
        echo CMake options cleared. Invalidating CMake cache...
    )
    set "INVALIDATE_CACHE=1"
)
rem Always write cmake opts file to track state
if "%CMAKE_EXTRA_ARGS%"=="" (
    type nul > "%CMAKE_OPTS_FILE%"
) else (
    >"%CMAKE_OPTS_FILE%" echo %CMAKE_EXTRA_ARGS%
)

if exist "%ASSET_HASH_FILE%" (
    if exist "%BUILD_ASSET_HASH_FILE%" (
        fc /b "%ASSET_HASH_FILE%" "%BUILD_ASSET_HASH_FILE%" >nul
        if errorlevel 1 (
            echo Assets have changed. Invalidating CMake cache...
            set "INVALIDATE_CACHE=1"
        )
    ) else (
        echo Asset hash missing in build directory. Invalidating CMake cache...
        set "INVALIDATE_CACHE=1"
    )
)

if "%INVALIDATE_CACHE%"=="1" (
    if exist "%BUILD_DIR%\CMakeCache.txt" del "%BUILD_DIR%\CMakeCache.txt"
    copy /y "%ASSET_HASH_FILE%" "%BUILD_ASSET_HASH_FILE%" >nul
)

@REM check if build directory and CMakeCache.txt exists, if not run cmake
if not exist %BUILD_DIR%\CMakeCache.txt (
    echo CMakeCache.txt not found in %BUILD_DIR%. Running cmake to generate build files...
    if "%TOOLCHAIN%"=="mingw" (
        cmake -S . -B %BUILD_DIR% -G "MinGW Makefiles" %CMAKE_EXTRA_ARGS%
    ) else if "%TOOLCHAIN%"=="ninja" (
        cmake -S . -B %BUILD_DIR% -G "Ninja" %CMAKE_EXTRA_ARGS%
    ) else (
        cmake -S . -B %BUILD_DIR% %CMAKE_EXTRA_ARGS%
    )
    if %errorlevel% neq 0 (
        echo CMake configuration failed.
        exit /b 1
    )
) else (
    echo CMakeCache.txt found in %BUILD_DIR%. Skipping cmake configuration.
)
rem If `compile_commands.json` exists in the build directory, copy it to ./build
if exist "%BUILD_DIR%\compile_commands.json" (
    if not exist build (
        mkdir build
    )
    copy /Y "%BUILD_DIR%\compile_commands.json" "build\compile_commands.json" >nul 2>nul
    if %errorlevel% equ 0 (
        echo Copied compile_commands.json to build\compile_commands.json
    ) else (
        echo Failed to copy compile_commands.json
    )
)

echo Building the project...
if "%BUILD_TYPE%"=="Release" (
    cmake --build %BUILD_DIR% --config Release %BUILD_EXTRA_ARGS%
) else (
    cmake --build %BUILD_DIR% %BUILD_EXTRA_ARGS%
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

:show_help
echo Usage: run.bat [options]
echo.
echo Options:
echo   --help, -h, /?        Show this help and exit
echo   --release             Build in Release (default: Debug)
echo   --vs                  Use Visual Studio generator (default)
echo   --ninja               Use Ninja generator
echo   --mingw               Use MinGW Makefiles generator
echo   --cmake-opts ...      Pass options to CMake configure step
echo   --build-opts ...      Pass options to CMake build step
echo   -- ...                Pass remaining args as CMake configure options
echo.
echo Examples:
echo   run.bat --ninja -- -DAVD_ENABLE_VULKAN_VIDEO=ON
echo   run.bat --ninja --build-opts --target avd -j 8
echo   run.bat --ninja -- -DAVD_ENABLE_VULKAN_VIDEO=ON --build-opts --target avd -j 8
exit /b 0

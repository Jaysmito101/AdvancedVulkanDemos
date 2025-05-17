@echo off
setlocal enabledelayedexpansion

REM Set the base directory
set "BASE_DIR=avd"

REM Check if clang-format is available
where clang-format >nul 2>&1
if errorlevel 1 (
    echo Error: clang-format is not installed or not in PATH.
    exit /b 1
)

REM Recursively find and format all .c and .h files
for /R "%BASE_DIR%" %%f in (*.c *.h) do (
    echo Formatting %%f
    clang-format -i "%%f"
)

echo Done formatting all .c and .h files in %BASE_DIR%.

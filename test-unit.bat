@echo off
REM Unit test runner for Windows - similar to 'make test-unit'

echo Running unit tests on Windows...

REM Update test discovery (similar to Makefile) - Windows PowerShell version
echo Updating test discovery...
powershell -Command "Get-ChildItem tests\unit\*.c | ForEach-Object { Get-Content $_.FullName | Where-Object { $_ -match 'SENTRY_TEST\(([^)]+)\)' } | ForEach-Object { $_ -replace 'SENTRY_TEST\(([^)]+)\).*', 'XX($1)' } } | Where-Object { $_ -notmatch 'define' } | Sort-Object | Get-Unique | Out-File tests\unit\tests.inc -Encoding ASCII"

REM Create unit-build directory and configure
if not exist unit-build mkdir unit-build
cd unit-build

echo Configuring CMake for unit tests...
cmake -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=%CD% -DSENTRY_BACKEND=none ..

if %errorlevel% neq 0 (
    echo CMake configuration failed!
    exit /b 1
)

echo Building unit tests...
cmake --build . --target sentry_test_unit --parallel

if %errorlevel% neq 0 (
    echo Build failed!
    exit /b 1
)

echo Running unit tests...
REM Find and run the executable in the correct location
for /f "delims=" %%i in ('dir /s /b sentry_test_unit.exe') do (
    echo Found test executable: %%i
    "%%i"
    goto :done
)

echo ERROR: Could not find sentry_test_unit.exe
exit /b 1

:done
cd ..
echo Unit tests completed.
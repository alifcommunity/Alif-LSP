@echo off
echo Building and running Alif LSP tests...
echo.

REM Compile the test executable
g++ -std=c++17 -I../src/third-party -I../src/include ^
    test_completion.cpp ^
    ../src/Completion.cpp ^
    ../src/DocManager.cpp ^
    -o test_completion.exe

if %ERRORLEVEL% NEQ 0 (
    echo ❌ Compilation failed!
    pause
    exit /b 1
)

echo ✅ Compilation successful!
echo.

REM Run the tests
echo Running tests...
echo.
test_completion.exe

REM Cleanup
if exist test_completion.exe del test_completion.exe

echo.
echo Tests completed.
pause

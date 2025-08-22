@echo off
echo Building and running Alif LSP Context-Aware tests with Visual Studio compiler...
echo.

REM Try to find Visual Studio compiler
set "VS_PATH="
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VS_PATH=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"
) else if exist "C:\Program Files\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" (
    set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat"
)

if "%VS_PATH%"=="" (
    echo ❌ Visual Studio not found! Please install Visual Studio with C++ support.
    pause
    exit /b 1
)

REM Setup Visual Studio environment
call "%VS_PATH%" x64

REM Compile the test executable
cl /std:c++17 /EHsc /I../src/third-party /I../src/include ^
    test_context.cpp ^
    ../src/Completion.cpp ^
    ../src/Context.cpp ^
    ../src/DocManager.cpp ^
    /Fe:test_context.exe

if %ERRORLEVEL% NEQ 0 (
    echo ❌ Compilation failed!
    pause
    exit /b 1
)

echo ✅ Compilation successful!
echo.

REM Run the tests
echo Running context tests...
echo.
test_context.exe

REM Cleanup
if exist test_context.exe del test_context.exe
if exist test_context.obj del test_context.obj
if exist Completion.obj del Completion.obj
if exist Context.obj del Context.obj
if exist DocManager.obj del DocManager.obj

echo.
echo Context tests completed.
pause

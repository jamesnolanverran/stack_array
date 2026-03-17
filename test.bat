@echo off
setlocal EnableExtensions

set "SCRIPT_NAME=%~nx0"
set "SRC=stack_array.c tests\test_stack_array.c tests\main.c"
set "OUTDIR=build"
set "OUTBIN=%OUTDIR%\stack_array_tests.exe"
set "TMPDIR=%OUTDIR%\tmp"

if /I "%~1"=="-h" goto :usage
if /I "%~1"=="--help" goto :usage
if /I "%~1"=="/?" goto :usage

if not exist "%OUTDIR%" mkdir "%OUTDIR%"
if not exist "%TMPDIR%" mkdir "%TMPDIR%"

set "TEMP=%CD%\%TMPDIR%"
set "TMP=%CD%\%TMPDIR%"

set "CC=%~1"
if not defined CC set "CC=%CC%"
if not defined CC call :detect_compiler
if not defined CC (
    echo [test] error: no supported compiler found on PATH
    echo [test] looked for: cl, clang-cl, clang, gcc
    exit /b 1
)

call :normalize_compiler "%CC%"
if errorlevel 1 exit /b 1

if /I "%CC_KIND%"=="msvc" (
    call :build_msvc
) else if /I "%CC_KIND%"=="gnu" (
    call :build_gnu
) else (
    echo [test] error: internal compiler kind failure
    exit /b 1
)
if errorlevel 1 exit /b %ERRORLEVEL%

echo [test] Running tests
"%OUTBIN%"
exit /b %ERRORLEVEL%

:detect_compiler
where cl >nul 2>nul
if not errorlevel 1 (
    set "CC=cl"
    exit /b 0
)

where clang-cl >nul 2>nul
if not errorlevel 1 (
    set "CC=clang-cl"
    exit /b 0
)

where clang >nul 2>nul
if not errorlevel 1 (
    set "CC=clang"
    exit /b 0
)

where gcc >nul 2>nul
if not errorlevel 1 (
    set "CC=gcc"
    exit /b 0
)

exit /b 0

:normalize_compiler
set "CC=%~1"
set "CC_KIND="

if /I "%CC%"=="cl" (
    set "CC_KIND=msvc"
    exit /b 0
)
if /I "%CC%"=="clang-cl" (
    set "CC_KIND=msvc"
    exit /b 0
)
if /I "%CC%"=="clang" (
    set "CC_KIND=gnu"
    exit /b 0
)
if /I "%CC%"=="gcc" (
    set "CC_KIND=gnu"
    exit /b 0
)

echo [test] error: unsupported compiler "%CC%"
echo [test] supported values: cl, clang-cl, clang, gcc
exit /b 1

:build_msvc
echo [test] Using compiler: %CC% ^(MSVC-style^)
"%CC%" /nologo /std:c99 /W4 /I. /Fo"%OUTDIR%\\" %SRC% /Fe:"%OUTBIN%"
exit /b %ERRORLEVEL%

:build_gnu
echo [test] Using compiler: %CC% ^(GCC-style^)
"%CC%" -std=c99 -Wall -Wextra -pedantic -I. %SRC% -o "%OUTBIN%"
exit /b %ERRORLEVEL%

:usage
echo Usage:
echo   %SCRIPT_NAME% [cl^|clang-cl^|clang^|gcc]
echo.
echo Examples:
echo   %SCRIPT_NAME% cl (Developer Command Prompt)
echo   %SCRIPT_NAME% clang-cl (Developer Command Prompt)
echo   %SCRIPT_NAME% clang
echo   %SCRIPT_NAME% gcc
echo.
echo If no compiler is given, this script tries: cl, clang-cl, clang, gcc.
exit /b 0

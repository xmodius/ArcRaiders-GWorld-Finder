@echo off
setlocal enabledelayedexpansion

cls
echo ==========================================
echo   Arc Raiders GWorld Scanner - Build
echo ==========================================
echo.

REM Check for Visual Studio environment
where cl.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Visual Studio Developer Command Prompt not detected
    echo.
    echo Please run this script from:
    echo "Developer Command Prompt for VS 2022"
    echo.
    pause
    exit /b 1
)

REM Check if source file exists
if not exist "GWorldScanner.cpp" (
    echo [ERROR] GWorldScanner.cpp not found!
    pause
    exit /b 1
)

echo [1/3] Compiling GWorldScanner.cpp...
cl.exe /nologo /EHsc /std:c++17 /O2 /W4 /Fe:GWorldScanner.exe GWorldScanner.cpp /link /OUT:GWorldScanner.exe >build.log 2>&1

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Compilation failed!
    echo.
    type build.log
    echo.
    pause
    exit /b 1
)

echo [SUCCESS] Compilation complete
echo.

REM Clean up intermediate files
echo [2/3] Cleaning up...
del *.obj 2>nul
del build.log 2>nul

echo [SUCCESS] Build artifacts cleaned
echo.

echo [3/3] Verifying output...
if exist "GWorldScanner.exe" (
    echo [SUCCESS] GWorldScanner.exe created successfully
    echo.
    echo File size: 
    for %%A in (GWorldScanner.exe) do echo   %%~zA bytes
) else (
    echo [ERROR] GWorldScanner.exe not found after build!
    pause
    exit /b 1
)

echo.
echo ==========================================
echo Build complete!
echo.
echo Next steps:
echo   1. Run setup.bat to download MemProcFS
echo   2. Run GWorldScanner.exe
echo ==========================================
echo.
pause

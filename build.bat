@echo off
setlocal enabledelayedexpansion

cls
echo ==========================================
echo   Arc Raiders GWorld Scanner - Build
echo ==========================================
echo.

REM Force x64 environment if not already set
if not "%VSCMD_ARG_TGT_ARCH%"=="x64" (
    echo [INFO] Detecting Visual Studio installation...
    
    REM Try to find and run vcvars64.bat
    set "VCVARS="
    
    REM Check common VS 2022 locations
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" (
        set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    )
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" (
        set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
    )
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat" (
        set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    )
    
    if defined VCVARS (
        echo [INFO] Initializing x64 build environment...
        call "!VCVARS!" >nul 2>&1
    ) else (
        echo [WARNING] Could not auto-detect VS installation
        echo [INFO] Attempting to use current environment...
    )
)

REM Verify we have cl.exe
where cl.exe >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Visual Studio C++ compiler not found
    echo.
    echo Please run this script from:
    echo "x64 Native Tools Command Prompt for VS 2022"
    echo.
    echo Or install Visual Studio 2022 with C++ tools
    echo.
    pause
    exit /b 1
)

REM Check architecture
cl.exe 2>&1 | findstr /C:"x64" >nul
if %ERRORLEVEL% NEQ 0 (
    echo [WARNING] Compiler may not be configured for x64
    echo [INFO] Forcing x64 target...
)

REM Check if source file exists
if not exist "GWorldScanner.cpp" (
    echo [ERROR] GWorldScanner.cpp not found!
    pause
    exit /b 1
)

echo [1/3] Compiling GWorldScanner.cpp for x64...
echo.

REM Compile with explicit x64 target
cl.exe /nologo /EHsc /std:c++17 /O2 /W3 /D_AMD64_ /DWIN64 /D_WIN64 ^
    GWorldScanner.cpp ^
    /Fe:GWorldScanner.exe ^
    /link /MACHINE:X64 /OUT:GWorldScanner.exe >build.log 2>&1

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
    
    REM Check if it's actually x64
    dumpbin /headers GWorldScanner.exe 2>nul | findstr /C:"machine (x64)" >nul
    if %ERRORLEVEL% EQU 0 (
        echo [OK] Verified: x64 (64-bit) executable
    ) else (
        echo [WARNING] Could not verify architecture
    )
    
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
echo   1. Ensure DMA hardware is connected
echo   2. Arc Raiders running in active match
echo   3. Run: GWorldScanner.exe
echo ==========================================
echo.
pause

@echo off
setlocal enabledelayedexpansion

cls
echo ==========================================
echo   MemProcFS Setup for GWorld Scanner
echo ==========================================
echo.

REM Check if already set up
if exist "vmm.dll" if exist "leechcore.dll" (
    echo [INFO] MemProcFS files already present
    echo.
    choice /C YN /M "Re-download MemProcFS files"
    if errorlevel 2 goto :verify
    echo.
)

echo [1/3] Downloading MemProcFS (latest Windows x64)...
echo         Size: ~5.35 MB
echo         This may take 30-60 seconds...
echo.

REM Use the -latest.zip which is a stable link
curl -# -L -o memprocfs.zip "https://github.com/ufrisk/MemProcFS/releases/download/v5.16/MemProcFS_files_and_binaries-win_x64-latest.zip"

if not exist "memprocfs.zip" (
    echo.
    echo [ERROR] Download failed!
    echo.
    echo Please check:
    echo   1. Internet connection
    echo   2. GitHub is accessible
    echo   3. Firewall/antivirus settings
    echo.
    echo Manual download:
    echo   URL: https://github.com/ufrisk/MemProcFS/releases/latest
    echo   File: MemProcFS_files_and_binaries-win_x64-latest.zip
    echo   Extract to: %CD%
    echo.
    pause
    exit /b 1
)

REM Verify download size (should be around 5MB)
for %%A in (memprocfs.zip) do set size=%%~zA
if %size% LSS 1000000 (
    echo.
    echo [ERROR] Downloaded file too small (%size% bytes)
    echo         Expected: ~5.35 MB (5,600,000 bytes)
    echo         Likely a redirect or error page
    echo.
    del memprocfs.zip
    echo Manual download required:
    echo   1. Visit: https://github.com/ufrisk/MemProcFS/releases/latest
    echo   2. Download: MemProcFS_files_and_binaries-win_x64-latest.zip
    echo   3. Extract to: %CD%
    echo.
    pause
    exit /b 1
)

echo [SUCCESS] Downloaded successfully (%size% bytes)
echo.

echo [2/3] Extracting MemProcFS files...
powershell -NoProfile -ExecutionPolicy Bypass -Command "& {Expand-Archive -Path 'memprocfs.zip' -DestinationPath '.' -Force}" 2>nul

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Extraction failed!
    echo.
    echo Trying alternative extraction method...
    powershell -Command "Add-Type -AssemblyName System.IO.Compression.FileSystem; [System.IO.Compression.ZipFile]::ExtractToDirectory('memprocfs.zip', '.')"
)

REM Clean up
del memprocfs.zip 2>nul

echo [SUCCESS] Extraction complete
echo.

:verify
echo [3/3] Verifying installation...
set MISSING=0
set WARNINGS=0

if not exist "vmm.dll" (
    echo [ERROR] vmm.dll not found
    set MISSING=1
) else (
    for %%A in (vmm.dll) do set vmmsize=%%~zA
    echo [OK] vmm.dll found (!vmmsize! bytes)
)

if not exist "leechcore.dll" (
    echo [ERROR] leechcore.dll not found
    set MISSING=1
) else (
    echo [OK] leechcore.dll found
)

if not exist "FTD3XX.dll" (
    echo [WARNING] FTD3XX.dll not found ^(may be optional^)
    set WARNINGS=1
) else (
    echo [OK] FTD3XX.dll found
)

echo.

if %MISSING%==1 (
    echo [ERROR] Critical files missing!
    echo.
    echo Manual setup required:
    echo   1. Visit: https://github.com/ufrisk/MemProcFS/releases/latest
    echo   2. Download: MemProcFS_files_and_binaries-win_x64-latest.zip
    echo   3. Extract ALL files to: %CD%
    echo   4. Verify vmm.dll and leechcore.dll are present
    echo.
    pause
    exit /b 1
)

echo ==========================================
echo [SUCCESS] Setup complete!
echo.
if %WARNINGS%==1 (
    echo Note: Some optional files missing
    echo       Scanner should still work
    echo.
)
echo Ready to run:
echo   GWorldScanner.exe
echo.
echo Requirements before scanning:
echo   1. Arc Raiders running on Gaming PC
echo   2. In an ACTIVE MATCH ^(not menu^)
echo   3. Wait 15-20 seconds after spawning
echo ==========================================
echo.
pause

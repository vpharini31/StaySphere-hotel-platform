@echo off
setlocal EnableDelayedExpansion
title Hotel Management System — Setup (Windows)

echo.
echo ╔══════════════════════════════════════════════════════╗
echo ║       Hotel MS — Windows Setup Script               ║
echo ╚══════════════════════════════════════════════════════╝
echo.

:: ── Check prerequisites ──────────────────────────────────────────────────────
echo [1/5] Checking prerequisites...

where cmake >nul 2>&1
if %ERRORLEVEL% NEq 0 (
    echo  ERROR: CMake not found.
    echo  Install from: https://cmake.org/download/
    echo  Add to PATH during installation.
    pause & exit /b 1
)

where git >nul 2>&1
if %ERRORLEVEL% NEq 0 (
    echo  ERROR: Git not found.
    echo  Install from: https://git-scm.com/download/win
    pause & exit /b 1
)

echo  CMake and Git found. OK

:: ── Download SQLite3 amalgamation ─────────────────────────────────────────────
echo.
echo [2/5] Downloading SQLite3 amalgamation...

if not exist "backend\lib\sqlite3.h" (
    powershell -Command "Invoke-WebRequest -Uri 'https://www.sqlite.org/2024/sqlite-amalgamation-3450100.zip' -OutFile 'sqlite_amal.zip'"
    powershell -Command "Expand-Archive -Path 'sqlite_amal.zip' -DestinationPath 'sqlite_tmp' -Force"
    copy sqlite_tmp\sqlite-amalgamation-3450100\sqlite3.c backend\lib\sqlite3.c
    copy sqlite_tmp\sqlite-amalgamation-3450100\sqlite3.h backend\lib\sqlite3.h
    rmdir /s /q sqlite_tmp
    del sqlite_amal.zip
    echo  SQLite3 downloaded OK
) else (
    echo  SQLite3 already present, skipping.
)

:: ── Create data directories ───────────────────────────────────────────────────
echo.
echo [3/5] Creating data directories...
if not exist "bin"              mkdir bin
if not exist "data\backups"     mkdir data\backups
if not exist "receipts"         mkdir receipts
echo  Directories created OK

:: ── Configure with CMake ──────────────────────────────────────────────────────
echo.
echo [4/5] Configuring CMake (Release build)...
echo Stopping any running server to prevent locked-file errors...
taskkill /F /IM hotel_server.exe >nul 2>&1

:: Force delete old executable to guarantee we run the new one
if exist "bin\hotel_server.exe" (
    del /F /Q "bin\hotel_server.exe"
    if exist "bin\hotel_server.exe" (
        echo ERROR: hotel_server.exe is locked by another program. Close it and try again!
        pause & exit /b 1
    )
)

if not exist "build" mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% NEQ 0 (
    echo  ERROR: CMake configuration failed.
    cd ..
    pause & exit /b 1
)
cd ..
echo  CMake configured OK

:: ── Build ─────────────────────────────────────────────────────────────────────
echo.
echo [5/5] Building project (this may take 2-5 minutes first time)...
cd build
cmake --build . --config Release --parallel
if %ERRORLEVEL% NEQ 0 (
    echo  ERROR: Build failed. Check output above.
    cd ..
    pause & exit /b 1
)
cd ..

:: Fallback copy in case CMake outputs to the build directory instead of bin
if exist "build\Release\hotel_server.exe" (
    copy /Y "build\Release\hotel_server.exe" "bin\hotel_server.exe" >nul
)

echo.
echo ╔══════════════════════════════════════════════════════╗
echo ║   Build SUCCESS!                                     ║
echo ║   Run:  bin\hotel_server.exe                         ║
echo ║   Open: http://localhost:8080                        ║
echo ╚══════════════════════════════════════════════════════╝
pause

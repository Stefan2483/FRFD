@echo off
echo ========================================
echo FRFD PlatformIO Build Fix Script
echo ========================================
echo.

echo [1/6] Cleaning build directory...
pio run -e lilygo-t-dongle-s3 -t clean
echo Done.
echo.

echo [2/6] Cleaning PlatformIO cache...
pio system prune -f
echo Done.
echo.

echo [3/6] Uninstalling existing ESP32 platform...
pio platform uninstall espressif32
echo Done.
echo.

echo [4/6] Installing ESP32 platform v6.4.0...
pio platform install espressif32@6.4.0
echo Done.
echo.

echo [5/6] Installing intelhex to PlatformIO Python...
%USERPROFILE%\.platformio\python3\python.exe -m pip install intelhex --upgrade
echo Done.
echo.

echo [6/6] Building firmware...
pio run -e lilygo-t-dongle-s3
echo Done.
echo.

if %errorlevel% equ 0 (
    echo ========================================
    echo SUCCESS! Build completed successfully.
    echo ========================================
    echo.
    echo Firmware location: .pio\build\lilygo-t-dongle-s3\firmware.bin
    echo.
) else (
    echo ========================================
    echo ERROR: Build failed. Please check the output above.
    echo ========================================
    echo.
    echo If the error persists, try:
    echo 1. Close and reopen VSCode
    echo 2. Run this script again
    echo 3. Or manually run: pio run -e lilygo-t-dongle-s3 -v
    echo.
)

pause

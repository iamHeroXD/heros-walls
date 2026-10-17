@echo off
:: Hero's Walls 1-Click DNS Deactivation Script for Windows
:: Must be run as Administrator

net session >nul 2>&1
if %errorLevel% neq 0 (
    echo ========================================================
    echo   [ERROR] Please right-click this file and choose:
    echo           "Run as administrator"
    echo ========================================================
    pause
    exit /b 1
)

echo ========================================================
echo       HERO'S WALLS - RESTORING DEFAULT DNS
echo ========================================================
echo.
echo Resetting Windows Wi-Fi DNS to Automatic (DHCP)...
netsh interface ip set dns name="Wi-Fi" dhcp
echo Flushing DNS cache...
ipconfig /flushdns

echo.
echo ========================================================
echo   [SUCCESS] Wi-Fi DNS restored to Automatic (Default).
echo ========================================================
echo.
pause

@echo off
:: Hero's Walls 1-Click DNS Activation Script for Windows
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
echo       HERO'S WALLS - 1-CLICK DNS AD BLOCKER SETUP
echo ========================================================
echo.
echo Detecting Hero's Walls IP on local network...

:: Test mDNS first
for /f "tokens=2 delims=[]" %%i in ('ping -n 1 -4 heroswalls.local 2^>nul ^| findstr "Pinging"') do set HERO_IP=%%i

if "%HERO_IP%"=="" (
    set HERO_IP=10.183.73.44
)

echo Found Hero's Walls at IP: %HERO_IP%
echo Pointing Windows Wi-Fi DNS to Hero's Walls...
netsh interface ip set dns name="Wi-Fi" static %HERO_IP%
netsh interface ip add dns name="Wi-Fi" %HERO_IP% index=2

echo Flushing Windows DNS cache...
ipconfig /flushdns

echo.
echo ========================================================
echo   [SUCCESS] Hero's Walls is now ACTIVE on this PC!
echo   Ads and tracking domains are blocked across all browsers.
echo.
echo   Open Dashboard: http://heroswalls.local
echo   To disable anytime, run: deactivate-heros-walls.bat
echo ========================================================
echo.
pause

@echo off
REM POSEIDON flash + serial monitor in one shot.
REM Usage:
REM   scripts\flash_and_monitor.bat          (cardputer env, default)
REM   scripts\flash_and_monitor.bat launcher (cardputer-launcher env)
REM
REM Ctrl+] inside the monitor to exit cleanly.
REM Ctrl+T then Ctrl+H inside monitor for help menu.

setlocal
cd /d %~dp0..

set ENV=cardputer
if "%~1"=="launcher" set ENV=cardputer-launcher

set PYTHONIOENCODING=utf-8
set PYTHONUTF8=1

set PIO=C:\Users\D\.platformio\penv\Scripts\pio.exe

echo === BUILD + FLASH (%ENV%) ===
"%PIO%" run -e %ENV% -t upload --upload-port COM9
if errorlevel 1 (
    echo FLASH FAILED — not starting monitor
    exit /b 1
)

echo.
echo === SERIAL MONITOR (Ctrl+] to exit) ===
"%PIO%" device monitor --port COM9 --baud 115200 --filter esp32_exception_decoder

endlocal

@echo off
REM POSEIDON serial monitor only (no flash).
REM Use this for re-attaching after testing or after a manual flash.
REM Ctrl+] to exit.

setlocal
cd /d %~dp0..

set PIO=C:\Users\D\.platformio\penv\Scripts\pio.exe

"%PIO%" device monitor --port COM9 --baud 115200 --filter esp32_exception_decoder

endlocal

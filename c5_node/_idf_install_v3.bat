@echo off
set MSYSTEM=
set MSYS=
set MINGW_PREFIX=
set MINGW_CHOST=
set MINGW_PACKAGE_PREFIX=
set MSYSTEM_PREFIX=
cd /d C:\Espressif\frameworks\esp-idf-v5.5.4
set IDF_TOOLS_PATH=C:\Espressif
call "C:\Espressif\frameworks\esp-idf-v5.5.4\install.bat" esp32c5 > C:\Users\D\poseidon\c5_node\_idf_install.log 2>&1
echo EXIT=%ERRORLEVEL% >> C:\Users\D\poseidon\c5_node\_idf_install.log

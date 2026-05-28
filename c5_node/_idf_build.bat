@echo off
set MSYSTEM=
set MSYS=
set MINGW_PREFIX=
set MINGW_CHOST=
set MINGW_PACKAGE_PREFIX=
set MSYSTEM_PREFIX=
set IDF_PATH=C:\Espressif\frameworks\esp-idf-v5.5.4
set IDF_TOOLS_PATH=C:\Espressif
set PATH=C:\Espressif\tools\idf-git\2.44.0\cmd;%PATH%
call %IDF_PATH%\export.bat
if errorlevel 1 (
    echo EXPORT FAILED
    exit /b %errorlevel%
)
cd /d C:\Users\D\poseidon\c5_node
python.exe %IDF_PATH%\tools\idf.py %*

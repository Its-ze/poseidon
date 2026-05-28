@echo off
set MSYSTEM=
set IDF_PATH=C:\Espressif\frameworks\esp-idf-v5.5.4
set IDF_TOOLS_PATH=C:\Espressif
call %IDF_PATH%\export.bat
if errorlevel 1 exit /b %errorlevel%
cd /d C:\Users\D\poseidon\c5_node\build
python.exe -m esptool --chip esp32c5 merge_bin -o trident-factory.bin --flash_mode dio --flash_freq 80m --flash_size 4MB 0x2000 bootloader/bootloader.bin 0x8000 partition_table/partition-table.bin 0x10000 poseidon_c5_node.bin

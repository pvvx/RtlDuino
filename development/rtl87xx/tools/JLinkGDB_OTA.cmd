rem @echo off
if %1x==x goto no_dir
if not exist %1\ota.bin goto error_x
PATH=%1;%2;%PATH%
if exist %2\ota.bin del %2\ota.bin
copy /b %1\ota.bin %2\ota.bin
cd "%2"
start JLinkGDBServer.exe -device Cortex-M3 -if SWD -ir -endian little -speed 3500 
%3arm-none-eabi-gdb.exe -x gdb_ota.jlink
taskkill /F /IM JLinkGDBServer.exe
goto end
:no_dir
PATH=D:\MCU\SEGGER\JLink_V612i;%LOCALAPPDATA%\Arduino15\packages\realtek\tools\ameba_tools\arm-none-eabi-gcc\4.8.3-2014q1\bin;%PATH% 
start JLinkGDBServer.exe -device Cortex-M3 -if SWD -ir -endian little -speed 3500 
arm-none-eabi-gdb.exe -x gdb_ota.jlink
taskkill /F /IM JLinkGDBServer.exe
goto end
:error_x
echo Error: Not found ota.bin!
:end
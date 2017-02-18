@echo off
if %1x==x goto no_dir
if not exist %1\ota.bin goto error_1
if not exist %2\run_ram.bin goto error_2
PATH=%1;%2;%PATH%
cd "%2"
copy /b %1\ota.bin ota.bin
echo r0>RunRAM.JLinkScript
echo r1>>RunRAM.JLinkScript
echo h>>RunRAM.JLinkScript
echo r>>RunRAM.JLinkScript
echo loadbin "run_ram.bin" 0x10000bc8>>RunRAM.JLinkScript
echo loadbin "ota.bin" 0x10005FF0>>RunRAM.JLinkScript
echo r>>RunRAM.JLinkScript
echo w4 0x40000210,0x20111117>>RunRAM.JLinkScript
echo g>>RunRAM.JLinkScript
echo q>>RunRAM.JLinkScript
rem PATH=D:\MCU\SEGGER\JLink_V612i;%PATH%;
JLink.exe -Device CORTEX-M3 -If SWD -Speed 3500 RunRAM.JLinkScript
goto end_x
:no_dir
rem PATH=D:\MCU\SEGGER\JLink_V612i;%PATH%;
echo r0>RunRAM.JLinkScript
echo r1>>RunRAM.JLinkScript
echo h>>RunRAM.JLinkScript
echo h>RunRAM.JLinkScript
echo r>>RunRAM.JLinkScript
echo loadbin "run_ram.bin" 0x10000bc8>>RunRAM.JLinkScript
echo loadbin "ota.bin" 0x10005FF0>>RunRAM.JLinkScript
echo r>>RunRAM.JLinkScript
echo w4 0x40000210,0x20011117>>RunRAM.JLinkScript
echo g>>RunRAM.JLinkScript
echo q>>RunRAM.JLinkScript
JLink.exe -Device CORTEX-M3 -If SWD -Speed 3500 RunRAM.JLinkScript
goto end_x
:error_1
echo Error: Not found ota.bin!
goto end_x
:error_2
echo Error: Not found run_ram.bin!
:end_x
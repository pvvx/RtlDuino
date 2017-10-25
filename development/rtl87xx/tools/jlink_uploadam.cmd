@echo off
if %1x==x goto no_dir
if not exist %1\ram_2.bin goto error_1
if not exist %1\sdram.bin goto error_1
if not exist %2\run_ram.bin goto error_2
PATH=%1;%2;%PATH%
cd "%2"
copy /b %1\ram_2.bin ram_2.bin
copy /b %1\sdram.bin sdram.bin
echo r0>RunRAM.JLinkScript
echo r1>>RunRAM.JLinkScript
echo h>>RunRAM.JLinkScript
echo r>>RunRAM.JLinkScript
echo loadbin "run_ram.bin" 0x10000bc8>>RunRAM.JLinkScript
echo loadbin "ram_2.bin" 0x10006000>>RunRAM.JLinkScript
echo r>>RunRAM.JLinkScript
echo w4 0x40000210,0x20011113>>RunRAM.JLinkScript
echo w4 0x1FFF0000,0x12345678>>RunRAM.JLinkScript
echo g>>RunRAM.JLinkScript
echo sleep 1000>>RunRAM.JLinkScript
echo h>>RunRAM.JLinkScript
echo loadbin "sdram.bin" 0x30000000>>RunRAM.JLinkScript
echo w4 0x1FFF0000,0x1>>RunRAM.JLinkScript
echo g>>RunRAM.JLinkScript>>RunRAM.JLinkScript
echo q>>RunRAM.JLinkScript>>RunRAM.JLinkScript
rem PATH=D:\MCU\SEGGER\JLink_V612i;%PATH%;
JLink.exe -Device CORTEX-M3 -If SWD -Speed 1500 RunRAM.JLinkScript
goto end_x
:no_dir
rem PATH=D:\MCU\SEGGER\JLink_V612i;%PATH%;
echo r0>RunRAM.JLinkScript
echo r1>>RunRAM.JLinkScript
echo h>>RunRAM.JLinkScript
echo h>RunRAM.JLinkScript
echo r>>RunRAM.JLinkScript
echo loadbin "run_ram.bin" 0x10000bc8>>RunRAM.JLinkScript
echo loadbin "ram_2.bin" 0x10006000>>RunRAM.JLinkScript
echo r>>RunRAM.JLinkScript
echo w4 0x40000210,0x20011113>>RunRAM.JLinkScript
echo w4 0x1FFF0000,0x12345678>>RunRAM.JLinkScript
echo g>>RunRAM.JLinkScript
echo sleep 1000>>RunRAM.JLinkScript
echo h>>RunRAM.JLinkScript
echo loadbin "sdram.bin" 0x30000000>>RunRAM.JLinkScript
echo w4 0x1FFF0000,0x1>>RunRAM.JLinkScript
echo g>>RunRAM.JLinkScript>>RunRAM.JLinkScript
echo q>>RunRAM.JLinkScript>>RunRAM.JLinkScript
JLink.exe -Device CORTEX-M3 -If SWD -Speed 1500 RunRAM.JLinkScript
goto end_x
:error_1
echo Error: Not found ota.bin!
goto end_x
:error_2
echo Error: Not found run_ram.bin!
:end_x
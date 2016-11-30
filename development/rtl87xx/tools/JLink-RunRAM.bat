@echo off
if %1x==x goto no_dir
echo h>%1\RunRAM.JLinkScript
echo r>>%1\RunRAM.JLinkScript
echo loadbin %1\bsp\image\ram_1.r.bin 0x10000bc8>>%1\RunRAM.JLinkScript
echo loadbin %1\ram_2.bin 0x10006000>>%1\RunRAM.JLinkScript
echo r>>%1\RunRAM.JLinkScript
echo w4 0x40000210,0x20200113>>%1\RunRAM.JLinkScript
echo g>>%1\RunRAM.JLinkScript
echo q>>%1\RunRAM.JLinkScript
rem PATH=D:\MCU\SEGGER\JLink_V610a;%PATH%;
%2\JLink.exe -Device CORTEX-M3 -If SWD -Speed 4000 %1\RunRAM.JLinkScript >null
goto end_x
:no_dir
rem PATH=D:\MCU\SEGGER\JLink_V610a;%PATH%;
start JLink.exe -Device CORTEX-M3 -If SWD -Speed 4000 RunRAM.JLinkScript
:end_x
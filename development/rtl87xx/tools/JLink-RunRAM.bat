@echo off
if %1x==x goto no_dir
echo h>%1\RunRAM.JLinkScript
echo r>>%1\RunRAM.JLinkScript
echo loadbin %1\bsp\image\ram_1.r.bin 0x10000bc8>>%1\RunRAM.JLinkScript
echo loadbin %1\ram_2.bin 0x10006000>>%1\RunRAM.JLinkScript
echo r>>%1\RunRAM.JLinkScript
echo w4 0x40000210,0x20011117>>%1\RunRAM.JLinkScript
echo g>>%1\RunRAM.JLinkScript
echo q>>%1\RunRAM.JLinkScript
rem PATH=D:\MCU\SEGGER\JLink_V610a;%PATH%;
%2\JLink.exe -Device CORTEX-M3 -If SWD -Speed 3500 %1\RunRAM.JLinkScript >null
goto end_x
:no_dir
rem PATH=D:\MCU\SEGGER\JLink_V610a;%PATH%;
echo h>RunRAM.JLinkScript
echo r>>RunRAM.JLinkScript
echo loadbin %LOCALAPPDATA%\Arduino15\packages\realtek\tools\ameba_tools\1.0.8\bsp\image\ram_1.r.bin 0x10000bc8>>RunRAM.JLinkScript
echo loadbin %LOCALAPPDATA%\Arduino15\packages\realtek\tools\ameba_tools\1.0.8\ram_2.bin 0x10006000>>RunRAM.JLinkScript
echo r>>RunRAM.JLinkScript
echo w4 0x40000210,0x20011117>>RunRAM.JLinkScript
echo g>>RunRAM.JLinkScript
echo q>>RunRAM.JLinkScript
start JLink.exe -Device CORTEX-M3 -If SWD -Speed 3500 RunRAM.JLinkScript
:end_x
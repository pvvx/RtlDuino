@echo off
rem PATH=D:\MCU\SEGGER\JLink_V612i;%PATH%
JLink.exe -Device CORTEX-M3 -If SWD -Speed 10000 RTL_FFlash.JLinkScript

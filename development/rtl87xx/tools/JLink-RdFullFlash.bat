@echo off
PATH=D:\MCU\SEGGER\JLink_V610a;%PATH%
JLink.exe -Device CORTEX-M3 -If SWD -Speed 10000 RTL_FFlash.JLinkScript

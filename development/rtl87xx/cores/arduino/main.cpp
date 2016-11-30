/*
  main.cpp - Main loop for Arduino sketches
  Copyright (c) 2005-2013 Arduino Team.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#define ARDUINO_MAIN
#include "Arduino.h"
#include "cmsis_os.h"

#ifdef __cplusplus
extern "C" {
#endif
extern int tcm_heap_freeSpace(void);
extern void console_init(void);

osThreadId main_tid = 0;

// Weak empty variant initialization function.
// May be redefined by variant files.
void initVariant() __attribute__((weak));
void initVariant() { }
/*
void set_cpu_clk166mhz(void)
{
	HalCpuClkConfig(0); // 0 - 166666666 Hz, 1 - 83333333 Hz, 2 - 41666666 Hz, 3 - 20833333 Hz, 4 - 10416666 Hz, 5 - 4000000 Hz
	HAL_LOG_UART_ADAPTER pUartAdapter;
	pUartAdapter.BaudRate = RUART_BAUD_RATE_38400;
	HalLogUartSetBaudRate(&pUartAdapter);
	SystemCoreClockUpdate();
	En32KCalibration();
}
*/
/*
 * \brief handle sketch
 */
void main_task( void const *arg )
{
//    delay(1);

    setup();

/*    
	 ConfigDebugErr  = -1;
	 ConfigDebugInfo = ~_DBG_SPI_FLASH_;
	 ConfigDebugWarn = -1;
	 CfgSysDebugErr = -1;
	 CfgSysDebugInfo = -1;
	 CfgSysDebugWarn = -1;
*/
    for (;;)
    {
        loop();
        if (serialEventRun) serialEventRun();
        osThreadYield();
    }
}

/*
 * \brief Main entry point of Arduino application
 */
int main( void )
{
//	set_cpu_clk166mhz();
	
    init();

    initVariant();

#if 0 // pvvx: add start info
    vPortFree(pvPortMalloc(4)); // Init RAM heap
	sys_info();
#endif 

    osThreadDef(main_task, osPriorityRealtime, 1, 4096*4); 
    main_tid = osThreadCreate(osThread (main_task), NULL);

    osKernelStart();

    while(1);

    return 0;
}

#ifdef __cplusplus
}
#endif


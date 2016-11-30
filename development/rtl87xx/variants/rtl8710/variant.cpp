/*
  Copyright (c) 2011 Arduino.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "variant.h"
#include "gpio_api.h"


#ifdef __cplusplus
extern "C" {
#endif

#include "PinNames.h"

void __libc_init_array(void);

/*
 * Pins descriptions
 */
PinDescription g_APinDescription[TOTAL_GPIO_PIN_NUM]=
{
  {PA_0, NOT_INITIAL, PIO_GPIO | PIO_GPIO_IRQ          , NOT_INITIAL}, //D0  : CD_D2, UART2_RX, SPI_MISO, GPIO_INT
  {PA_1, NOT_INITIAL, PIO_GPIO | PIO_GPIO_IRQ          , NOT_INITIAL}, //D1  : CD_D3, UART2_CTS, SPI1_MOSI, GPIO_INT
  {PA_2, NOT_INITIAL, PIO_GPIO                         , NOT_INITIAL}, //D2  : CD_CMD, UART2_RTS, SPI1_CLK
  {PA_3, NOT_INITIAL, PIO_GPIO                         , NOT_INITIAL}, //D3  : CD_CLK     
  {PA_4, NOT_INITIAL, PIO_GPIO                         , NOT_INITIAL}, //D4  : CD_D0, UART2_TX, SPI1_CS, LED1
  {PA_5, NOT_INITIAL, PIO_GPIO                         , NOT_INITIAL}, //D5  : CD_D1, D_SBY0
  {PB_0, NOT_INITIAL, PIO_GPIO                         , NOT_INITIAL}, //D6  : CON_TX, ETE0, LED0
  {PB_1, NOT_INITIAL, PIO_GPIO                         , NOT_INITIAL}, //D7  : CON_RX, WL_LED0, ETE1, D_SLP0
  {PB_2, NOT_INITIAL, PIO_GPIO                         , NOT_INITIAL}, //D8  : I2C3_SCL, ETE2
  {PB_3, NOT_INITIAL, PIO_GPIO | PIO_GPIO_IRQ          , NOT_INITIAL}, //D9  : I2C3_SDA, ETE3, GPIO_INT
  {PC_0, NOT_INITIAL, PIO_GPIO                | PIO_PWM, NOT_INITIAL}, //D10 : PWM0, SPI0_CS0, UART0_RX, I2S1_WS, PCM1_SYNC, ETE0
  {PC_1, NOT_INITIAL, PIO_GPIO | PIO_GPIO_IRQ | PIO_PWM, NOT_INITIAL}, //D11 : PWM1, SPI0_CLK, UART0_CTS, I2S1_CLK, PCM1_CLK, ETE1, GPIO_INT 
  {PC_2, NOT_INITIAL, PIO_GPIO                | PIO_PWM, NOT_INITIAL}, //D12 : PWM2, SPI0_MOSI, UART0_RTS, I2S1_SD_TX, PCM1_OUT, ETE2
  {PC_3, NOT_INITIAL, PIO_GPIO | PIO_GPIO_IRQ | PIO_PWM, NOT_INITIAL}, //D13 : PWM3, SPI0_MISO, UART0_TX, I2S1_MCK, PCM1_IN, ETE3, GPIO_INT 
  {PC_4, NOT_INITIAL, PIO_GPIO | PIO_GPIO_IRQ          , NOT_INITIAL}, //D14 : I2C1_SDA, SPI0_CS1, I2S1_SD_RX, GPIO_INT
  {PC_5, NOT_INITIAL, PIO_GPIO | PIO_GPIO_IRQ          , NOT_INITIAL}, //D15 : I2C1_SCL, SPI0_CS2, GPIO_INT
  {PE_0, NOT_INITIAL, PIO_GPIO                | PIO_PWM, NOT_INITIAL}, //D16 : PWM0, JTAG_TRST, UART0_TX, I2C2_SCL, SPI0_CS0, PCM0_SYNC
  {PE_1, NOT_INITIAL, PIO_GPIO | PIO_GPIO_IRQ | PIO_PWM, NOT_INITIAL}, //D17 : PWM1, JTAG_TDI, UART0_RTS, SPI0_CLK, GPIO_INT
  {PE_2, NOT_INITIAL, PIO_GPIO | PIO_GPIO_IRQ | PIO_PWM, NOT_INITIAL}, //D18 : PWM2, JTAG_TDO, UART0_CTS, SPI0_MOSI, GPIO_INT
  {PE_3, NOT_INITIAL, PIO_GPIO | PIO_GPIO_IRQ | PIO_PWM, NOT_INITIAL}, //D19 : PWM3, JTAG_TMS, UART0_RX, SPI0_MISO, GPIO_INT, D_SBY3
  {PE_4, NOT_INITIAL, PIO_GPIO | PIO_GPIO_IRQ          , NOT_INITIAL}, //D20 : JTAG_CLK, SPI0_CS1
};

void *gpio_pin_struct[TOTAL_GPIO_PIN_NUM] = {NULL};
void *gpio_irq_handler_list[TOTAL_GPIO_PIN_NUM] = {NULL};

/** The heap API in OS layer */
extern int vPortAddHeapRegion(uint8_t *addr, size_t size);

// it should be the last symbol in SRAM in link result 
extern void *__HeapLimit;

// it should be the last symbol in SDRAM in link result
extern void *__sdram_bss_end__;

//extern int HalPinCtrlRtl8195A(int  Function, int PinLocation, int Operation);

#ifdef __cplusplus
} // extern C
#endif

void serialEvent() __attribute__((weak));
bool Serial_available() __attribute__((weak));

// ----------------------------------------------------------------------------

void serialEventRun(void)
{
    if (Serial_available && serialEvent && Serial_available()) serialEvent();
}


void init( void )
{
    // Initialize C library
    __libc_init_array();
}

// ----------------------------------------------------------------------------

void wait_for_debug() {
    while (((CoreDebug->DHCSR) & CoreDebug_DHCSR_C_DEBUGEN_Msk) == 0) {
        asm("nop");
    }
    delay(1000);
}


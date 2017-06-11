#include "us_ticker_api.h"

extern "C" {
void UserPreInit(void)
{
   Init_CPU_CLK_UART(1,38400); // 83.3 MHz
   // 0 - 166666666 Hz, 1 - 83333333 Hz, 2 - 41666666 Hz, 3 - 20833333 Hz, 4 - 10416666 Hz, 5 - 4000000 Hz
   // 6 - 200000000 Hz, 7 - 10000000 Hz, 8 - 50000000 Hz, 9 - 25000000 Hz, 10 - 12500000 Hz, 11 - 4000000 Hz
}  
} // extern "C"

void setup() {
  sys_info();
  us_ticker_init();
}

void loop() {
  uint32_t t = us_ticker_read();
  delay(1000);
  t = us_ticker_read() - t;
  printf("Delta(us) = %u\n", t);
}

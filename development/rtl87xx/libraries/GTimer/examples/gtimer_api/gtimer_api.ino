extern "C" {
  #include "timer_api.h"
}
 
extern "C" {
void UserPreInit(void)
{
   Init_CPU_CLK_UART(1,38400); // 83.3 MHz
   // 0 - 166666666 Hz, 1 - 83333333 Hz, 2 - 41666666 Hz, 3 - 20833333 Hz, 4 - 10416666 Hz, 5 - 4000000 Hz
   // 6 - 200000000 Hz, 7 - 10000000 Hz, 8 - 50000000 Hz, 9 - 25000000 Hz, 10 - 12500000 Hz, 11 - 4000000 Hz
}
} // extern "C"
 
gtimer_t timer0, timer1;
 
 
int counter = 0;
void timer0handler(uint32_t data) {
  printf("TIMER%u IntCounter: %u\n", data-TIMER0, counter++);
}
 
void setup() {
  sys_info();
  gtimer_init(&timer0, TIMER0);
  gtimer_init(&timer1, TIMER1);
  gtimer_start(&timer0);
  gtimer_start_periodical(&timer1, 10000000, (void *)timer0handler, TIMER1);
  delay(10);
}
 
void loop() {
  uint32_t t = gtimer_read_tick(&timer0);
  delay(1000);
  t -= gtimer_read_tick(&timer0);
  printf("Delta(tick) = %u, Delta(ms) = %u\n", t, t*1000/32768);
}
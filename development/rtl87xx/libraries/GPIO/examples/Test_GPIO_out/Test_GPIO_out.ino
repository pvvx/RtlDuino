/*
 * Demonstrates use ALL i/o RTL00
 **/
extern uint16_t GPIOState[]; // pins A..K
extern "C" void HalPinCtrlRtl8195A(int,int,int);

void setup() {
    printf("========================\n");
    for (int i = 0; i <= 10; i++) 
      printf("Port %c state: 0x%04x\n", i+'A', GPIOState[i]);
    printf("========================\n");
    printf("\nTest %d i/o...\n", TOTAL_GPIO_PIN_NUM);
    delay(100);
    HalPinCtrlRtl8195A(216,0,0); // JTAG Off
    HalPinCtrlRtl8195A(220,0,0); // LOG_UART Off
}

void loop() {
    for(int i = 0; i < TOTAL_GPIO_PIN_NUM; i++) {
      digitalWrite(i, HIGH);
      digitalWrite(i, LOW);
    }
}

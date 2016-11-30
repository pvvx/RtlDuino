/*
 * Demonstrates use 21 i/o RTL00
 **/
extern "C" void HalPinCtrlRtl8195A(int,int,int);

void setup() {
    printf("\nTest 21 i/o...\n");
    HalPinCtrlRtl8195A(216,0,0); // JTAG Off
    HalPinCtrlRtl8195A(220,0,0); // LOG_UART Off
    for(int i = 0; i < TOTAL_GPIO_PIN_NUM; i++) pinMode(i, OUTPUT);
}

void loop() {
    for(int i = 0; i < TOTAL_GPIO_PIN_NUM; i++) {
      digitalWrite(i, HIGH);
      digitalWrite(i, LOW);
    }
}

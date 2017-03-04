/*
 Software serial test

 Receives from serial RX, and then sends to serial TX
 data bits: 7, parity: even, 1 stop bit, with RTS/CTS

 The circuit: (BOARD RTL8710 UART0 (RX:PC_0, TX:PC_3))
 * RX is digital pin 8 (connect to TX of other device)
 * TX is digital pin 11 (connect to RX of other device)
 * RTS is digital pin 10 (connect to CTS of other device)
 * CTS is digital pin 9 (connect to RTS of other device)

 */
#include <SoftwareSerial.h>

#if defined(BOARD_RTL8710)
SoftwareSerial mySerial(8, 11); // UART0 (RX:PC_0, TX:PC_3)
/* 
UART2 (RX:PA_0, TX:PA_4)
SoftwareSerial mySerial(0, 4);   

UART0 (RX:PE_3, TX:PE_0):
extern "C" void HalPinCtrlRtl8195A(int,int,int);
HalPinCtrlRtl8195A(192,0,0); // JTAG Off
SoftwareSerial mySerial(17, 14); 
*/
#else
SoftwareSerial mySerial(0, 1); // RX, TX 
#endif

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(57600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }


  Serial.println("Goodnight moon!");

  // set the data rate for the SoftwareSerial port
  mySerial.begin(38400, 7, PARITY_EVEN, 1, FLOW_CONTROL_RTSCTS, 6, 3);
  mySerial.println("Hello, world?");
}

void loop() { // run over and over
  if (mySerial.available()) {
    mySerial.write(mySerial.read());
  }
}

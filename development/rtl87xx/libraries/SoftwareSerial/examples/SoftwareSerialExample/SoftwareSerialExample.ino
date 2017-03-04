/*
 Software serial test

 Receives from serial RX, and then sends to serial TX

 The circuit: (BOARD RTL8710)
 * RX is digital pin 8 (connect to TX of other device)
 * TX is digital pin 11 (connect to RX of other device)

 */
#include <SoftwareSerial.h>

#if defined(BOARD_RTL8710)
SoftwareSerial mySerial(8, 11); // UART0(RX:PC_0, TX:PC_3)
/* or UART0(RX:PE_3, TX:PE_0):
extern "C" void HalPinCtrlRtl8195A(int,int,int);
HalPinCtrlRtl8195A(192,0,0); // JTAG Off
SoftwareSerial mySerial(17, 14); 
*/
SoftwareSerial mySerial2(0, 4); // RX,TX - UART2(RX:PA_0, TX:PA_4)
//SoftwareSerial mySerial1(18, ?); // RX,TX - UART2(RX:PE_4, TX:PE_7)
#else
SoftwareSerial mySerial(0, 1); // RX, TX 
#endif

void setup() {
  // Open serial communications and wait for port to open:
  delay(10); // wait console start out soo
  Serial.begin(38400);
  Serial.println();
  Serial.println("Goodnight moon!");

  // set the data rate for the SoftwareSerial port
  mySerial.begin(38400);
  mySerial.println("UART0: Hello, world?");
#if defined(BOARD_RTL8710)  
  mySerial2.begin(38400);
  mySerial2.println("UART2: Hello, world?");
#endif  
}

void loop() { // run over and over
  if (mySerial.available()) {
    mySerial.write(mySerial.read());
  }
#if defined(BOARD_RTL8710)  
  if (mySerial2.available()) {
    mySerial2.write(mySerial2.read());
  }
#endif  
}


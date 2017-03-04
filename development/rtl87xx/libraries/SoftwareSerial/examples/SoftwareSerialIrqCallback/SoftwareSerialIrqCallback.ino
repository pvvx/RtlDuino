/*
 Software serial test: using irq callback and semaphore

 Set callback function "mySerialCalback" to software serial. Whenever there is data comes in,
 "mySerialCallback" is invoked. In this sketch, it do nothing s until a end of line.
 And then it send a semphore.
 The loop() use a non-busy loop to wait semaphore.

 To test this sketch, you need type something on software serial and then press Enter.

 The circuit: (BOARD RTL8710)
 * RX is digital pin 8 (connect to TX of other device)
 * TX is digital pin 11 (connect to RX of other device)

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

uint32_t semaID;

/* The callback is hook at UART IRQ handler and please don't do heavy task here. */
void mySerialCallback(char c)
{
  /*  The parameter c is only for peeking. The actuall data is
   *  still in the buffer of SoftwareSerial.
   */
  if (c == '\r' || c == '\n') {
    os_semaphore_release(semaID);
  }
}

void setup() {
  // use 1 count for binary semaphore
  semaID = os_semaphore_create(1);

  // There is a token in the semaphore, clear it.
  os_semaphore_wait(semaID, 0xFFFFFFFF);

  // set the data rate for the SoftwareSerial port
  mySerial.begin(38400);
  mySerial.setAvailableCallback(mySerialCallback);
}

void loop() { // run over and over
  // wait semaphore for 5s timeout
  if (os_semaphore_wait(semaID, 5 * 1000)) {
    // we got data before timeout
    while(mySerial.available()) {
      mySerial.print((char)mySerial.read());
    }
    mySerial.println();
  } else {
    mySerial.println("No data comes in.");
  }
}

/*
  This sketch shows how to update Ameba image via Arduino IDE.

  (1) Download this sketch via usb at first time
  (2) Reset Ameba. It will connect to network, enable mDNS, and wait for client connected.
  (3) Modify this sketch. (Ex. the version number)
  (4) Select network port in "Tools" -> "Port" -> "Arduino at 192.168.1.238 (Ameba RTL8195A)". Then upload.
  (5) After download finish, Ameba would reboot. You can see the changes has been updated.

*/

#include <WiFi.h>
#include <OTA.h>

#include <myAP.h>

#ifndef _MYAPCFG_H_
char ssid[] = "yourNetwork";     //  your network SSID (name)
char pass[] = "secretPassword";  // your network password
#endif

#define MY_VERSION_NUMBER 1
#define OTA_PORT 5000

#if defined(BOARD_RTL8710)
#define RECOVER_PIN 9	// PC_1, if "0" (connect gnd) -> boot OTA
#elif defined(BOARD_RTL8711AM)
#define RECOVER_PIN 10	// PC_1, if "0" (connect gnd) -> boot OTA
#else
#define RECOVER_PIN 18 // PE_5?
#endif

void setup() {
  printf("This is version %d\r\n\r\n", MY_VERSION_NUMBER);

  printf("Try to connect to %s\r\n", ssid);
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    printf("Failed. Wait 0.1 s and retry...\r\n");
    delay(100);
  }
  printf("\r\nConnected...\r\n");

  // These setting only needed at first time download from usb. And it doesn't needed at next OTA.
#if MY_VERSION_NUMBER == 1
  // This set the flash address that store the OTA image. Skip this setting would use default setting which is DEFAULT_OTA_ADDRESS
  OTA.setOtaAddress(DEFAULT_OTA_ADDRESS);

  // This set the recover pin. Boot device with pull up this pin (Eq. connect pin to 3.3V) would make device boot from version 1
  OTA.setRecoverPin(RECOVER_PIN);
#endif

  // Broadcast mDNS service at OTA_PORT that makes Arduino IDE find Ameba device
  OTA.beginArduinoMdnsService("MyModule", OTA_PORT);

  // Listen at OTA_PORT and wait for client (Eq. Arduino IDE). Client would send OTA image and make a update.
  if (OTA.beginLocal(OTA_PORT) < 0) {
    printf("OTA failed!!\r\n");
  }
}

void loop() {
  delay(1000);
}
/*
  This sketch shows how to enable OTA service on RtlDuino 
  and proceed your task at the same time.  
*/

#include <WiFi.h>
#include <OTA.h>

char ssid[] = "RTLDUINO_OTA"; //Set the AP's SSID
char pass[] = "0123456789";   //Set the AP's password
char channel[] = "1";         //Set the AP's channel

#define MY_VERSION_NUMBER 1
#define OTA_PORT 5000

#if defined(BOARD_RTL8710)
#define RECOVER_PIN 9 // PC_1, if "0" (connect gnd) -> boot OTA
#elif defined(BOARD_RTL8711AM)
#define RECOVER_PIN 10 // PC_1, if "0" (connect gnd) -> boot OTA
#else
#define RECOVER_PIN 18 // PE_5?
#endif

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(38400);
  while (!Serial) ; // wait for serial port to connect. Needed for native USB port only
  Serial.println();
  Serial.println("nRtlDuino OTA loader version 0.0.1");
  Serial.println();
  // Start AP:
  WiFi.status();
  while (WiFi.apbegin(ssid, pass, channel) != WL_CONNECTED) {
    delay(100);
  }
  printCurrentNet();
  os_thread_create(ota_thread, NULL, OS_PRIORITY_REALTIME, 2048);
}

void loop() {
  delay(1000);
}

void ota_thread(const void *argument) {
  // These setting only needed at first time download from usb. And it doesn't needed at next OTA.
  // This set the flash address that store the OTA image. Skip this setting would use default setting which is DEFAULT_OTA_ADDRESS
  OTA.setOtaAddress(DEFAULT_OTA_ADDRESS);

  // This set the recover pin. Boot device with pull up this pin (Eq. connect pin to 3.3V) would make device boot from version 1
  OTA.setRecoverPin(RECOVER_PIN);

  // Broadcast mDNS service at OTA_PORT that makes Arduino IDE find Ameba device
  OTA.beginArduinoMdnsService("MyModule", OTA_PORT);

  // Listen at OTA_PORT and wait for client (Eq. Arduino IDE). Client would send OTA image and make a update.
  while ( OTA.beginLocal(OTA_PORT) < 0 ) {
    printCurrentNet();
    Serial.println();
    Serial.println("Retry OTA after 10s\r\n");
    Serial.println();
    delay(10000);
  }

  // This line is not expected to be executed because if OTA success it would reboot device immediatedly.
  os_thread_terminate( os_thread_get_id() );
}

void printCurrentNet() {
  // print the SSID of the AP:
  Serial.println();
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("PASSWORD: ");
  Serial.println(pass);
  // print the MAC address of AP:
  byte bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("BSSID: ");
  Serial.print(bssid[0], HEX);
  Serial.print(":");
  Serial.print(bssid[1], HEX);
  Serial.print(":");
  Serial.print(bssid[2], HEX);
  Serial.print(":");
  Serial.print(bssid[3], HEX);
  Serial.print(":");
  Serial.print(bssid[4], HEX);
  Serial.print(":");
  Serial.println(bssid[5], HEX);

  // print the encryption type:
  Serial.print("Encryption Type: ");
  Serial.println(WiFi.encryptionType(), HEX);
  Serial.println();

  // print your WiFi shield's IP address:
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // print your subnet mask:
  Serial.print("NetMask: ");
  Serial.println(WiFi.subnetMask());

  // print your gateway address:
  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());
  Serial.println();
}



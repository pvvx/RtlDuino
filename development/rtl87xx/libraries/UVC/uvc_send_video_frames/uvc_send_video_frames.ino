/*
   This sketch shows how to take raw capture from UVC and send out to remote server

   You need to fill in remote server's address and port.
   Use windows/*.exe tool to capture the raw file.
*/

#include <WiFi.h>
#include <UVC.h>
#include <myAP.h>

#include "uvc/uvc_drv.h"


//char ssid[] = "yourNetwork";     // your network SSID (name)
//char pass[] = "password";        // your network password
int status = WL_IDLE_STATUS;     // the Wifi radio's status

unsigned char *vfrmbuf = NULL;

WiFiClient client;
char serverIP[] = "192.168.1.2"; // The remote server IP to receive jpeg file
int  serverPort = 5007;           // the remote server port
int chunkSize = 1460;             // If MTU=1500, then data payload = 1500 - IPv4 header(20) - TCP header(20) = 1460

extern "C" void sys_reset(void);

void reboot()
{
//  delay(100);
//  WiFi.off();
  delay(100);
  sys_reset();
}

void setup() {
// Select speed SDRAM
//  *((volatile uint32_t*)(0x40000040)) = (*((volatile uint32_t*)(0x40000040)) & (~0xF00000)) | 0x300000;
//  *((volatile uint32_t*)(0x40000300)) = 0x060031;
  sys_info();
  printf("tSDRAM: %02x-%02x\n", 
    (*((volatile uint32_t*)(0x40000040))>>20)&0xFF,
    (*((volatile uint32_t*)(0x40000300))>>16)&0xFF);
    
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) delay(100);
  printWifiStatus();
  int size = SetCameraParam(VFRMT_YUYV, 176, 144, 5, 0);
//  int size = SetCameraParam(VFRMT_YUYV, 160, 120, 15, 0);
  printf("Video Frame size: %d bytes\n", size);
  vfrmbuf = (unsigned char *)malloc(size);
  if (!vfrmbuf) {
    printf("No alloc buf!");
    reboot();
  }
  printf("Wait Start Camera ...");
  if (!StartCamera()) {
    printf("\nCamera none!\n");
    reboot();
  }
  printf(" ok\n");
}

void loop() {
 
  if (client.connect(serverIP, serverPort)) {
    printf("Get and send 100 frames ...");
    int tt = millis();
    for (int i = 0; i < 100; i++) {
      int len =  GetCameraFrame(vfrmbuf);
      if (len) {
        client.write(vfrmbuf, len);
      } else {
        printf("\nFail to get video frame!\n");
        client.stop();
        reboot();
      }
    }
    printf(" ok (%d) ms\n", millis() - tt);
    client.stop();
  } else {
    printf("\nFail to connect!\n");
  }
  delay(10000);
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  Serial.print(mac[0], HEX);
  Serial.print(":");
  Serial.print(mac[1], HEX);
  Serial.print(":");
  Serial.print(mac[2], HEX);
  Serial.print(":");
  Serial.print(mac[3], HEX);
  Serial.print(":");
  Serial.print(mac[4], HEX);
  Serial.print(":");
  Serial.println(mac[5], HEX);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}


#include <WiFi.h>
#include "SdFatFs.h"
#include "ftps.h"
#include <myAP.h>

//char ssid[] = "HOMEAP"; //  your network SSID (name)
//char pass[] = "password";    // your network password (use for WPA, or use as key for WEP)
//int keyIndex = 0;            // your network key Index number (needed only for WEP)

extern "C" {
void UserPreInit(void)
{
   Init_CPU_CLK_UART(0,38400); // 166 MHz
   // 0 - 166666666 Hz, 1 - 83333333 Hz, 2 - 41666666 Hz, 3 - 20833333 Hz, 4 - 10416666 Hz, 5 - 4000000 Hz
   // 6 - 200000000 Hz, 7 - 10000000 Hz, 8 - 50000000 Hz, 9 - 25000000 Hz, 10 - 12500000 Hz, 11 - 4000000 Hz
}  
} // extern "C"

SdFatFs fs;

int InitSD(void)
{
  int ret = 0;
  char * buf = new char[512];
//  debug_on();
  fs.begin();
  if (fs.getCSD((unsigned char *)buf) == 0) {
    printf("\nSD CSD: ");
    for (int i = 0; i < 16; i++)
      printf("%02x", buf[i]);
    printf("\n");
    buf[0] = 0;
    uint32_t svn;
  if (fs.getLabel(fs.getRootPath(), buf, &svn) == 0)
    printf("Disk Label: '%s', Serial Number: %p\n", buf, svn);
    ret = 1;
  }
  delete[] buf;
//  sys_info();
  return ret;
}

void setup() {
  // attempt to connect to Wifi network:
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    // wait 0.1 seconds for connection:
    delay(100);
  }
  printf("\nConnected to wifi\n");

  if (InitSD()) {
    os_thread_create(ftp_server, NULL, OS_PRIORITY_REALTIME, 1024);
  }
  else printf("No SD!\n");
}

void loop() {
  delay(1000);
}




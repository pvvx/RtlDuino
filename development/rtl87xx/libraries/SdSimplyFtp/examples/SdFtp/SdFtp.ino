
#include <WiFi.h>
#include "SdFatFs.h"
#include "ftps.h"
#include <myAP.h>

//char ssid[] = "yourNetwork"; //  your network SSID (name)
//char pass[] = "password";    // your network password (use for WPA, or use as key for WEP)
//int keyIndex = 0;            // your network key Index number (needed only for WEP)

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




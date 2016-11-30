/*
* RTL00(RTL8710AF) MP3 MP3 stereo player
* 2xPWM 96bits Out GC_2 and PE_2
* Used lib_mp3i2s.a
* https://github.com/pvvx/RTL00MP3
* https://esp8266.ru/forum/threads/rtl00-mp3-player.1697/
* 2016/10, RTL8710: kissste, pvvx
*/
#include <WiFi.h>
#include "RTL00MP3.h"
#include <myAP.h>

#ifndef _MYAPCFG_H_
char ssid[] = "yourNetwork"; //  your network SSID (name)
char pass[] = "password";    // your network password (use for WPA, or use as key for WEP)
#endif //_MYAPCFG_H_

void setup() {
  sys_info();	
  WiFi.begin(ssid, pass);
}

void loop() {
  if(WiFi.status() == WL_CONNECTED) {
    if(RTLmp3.status() == 0) {
      printf("\r\n");
      RTLmp3.begin("icecast.omroep.nl/3fm-sb-mp3", 80);
    }
  }
  else RTLmp3.end();
  delay(50);
}

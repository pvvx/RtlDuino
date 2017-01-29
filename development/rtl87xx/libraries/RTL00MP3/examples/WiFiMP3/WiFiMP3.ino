/*
* RTL00(RTL8710AF) MP3 MP3 stereo player
* Output: 2xPWM 96bits pins GC_2 and PE_2
* MP3 Bitrate: 48..96kbps !
* https://github.com/pvvx/RTL00MP3
* https://esp8266.ru/forum/threads/rtl00-mp3-player.1697/
* 2016/10, RTL8710: kissste, pvvx
* Warning: On run JTag Off! Reset module for program (connect nRST to CH_EN).
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

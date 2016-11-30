/*
* RTL00(RTL8710AF) MP3 MP3 stereo player
* 2xPWM 96bits Out GC_2 and PE_2
* Used lib_mp3i2s.a
* https://github.com/pvvx/RTL00MP3
* https://esp8266.ru/forum/threads/rtl00-mp3-player.1697/
* 2016/10, RTL8710: kissste, pvvx
*/
#ifndef _RTL00MP3_H_
#define _RTL00MP3_H_

class RTLmp3Class {

public:
    RTLmp3Class();
    void begin(char *url, int port);
	int status();
	void end();
//private:
};

extern RTLmp3Class RTLmp3;

#endif // _RTL00MP3_H_

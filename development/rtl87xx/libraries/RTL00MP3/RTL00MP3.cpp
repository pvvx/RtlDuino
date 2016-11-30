/*
* RTL00(RTL8710AF) MP3 MP3 stereo player
* 2xPWM 96bits Out GC_2 and PE_2
* Used lib_mp3i2s.a
* https://github.com/pvvx/RTL00MP3
* https://esp8266.ru/forum/threads/rtl00-mp3-player.1697/
* 2016/10, RTL8710: kissste, pvvx
*/
#if defined(BOARD_RTL8710) || defined(BOARD_RTL8711AM)
#else
#error "Only RTL8710 and RTL8711AM!"
#endif

#include "arduino.h"
#include "RTL00MP3.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void rtl_mp3_close(void);
extern void rtl_mp3_start(char *url, int port);
extern volatile char tskmad_enable, tskreader_enable;

#ifdef __cplusplus
}
#endif

RTLmp3Class::RTLmp3Class() {
}

void RTLmp3Class::end() {
	rtl_mp3_close();
}

int RTLmp3Class::status() {
	return tskreader_enable;
}

void RTLmp3Class::begin(char *url, int port) {
	rtl_mp3_start(url, port);
}

RTLmp3Class RTLmp3 = RTLmp3Class();


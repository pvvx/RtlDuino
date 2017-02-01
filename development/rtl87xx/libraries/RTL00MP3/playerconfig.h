#ifndef _PLAYER_CONFIG_H_
#define _PLAYER_CONFIG_H_

/*
Define the access point name and its password here.
*/
//#define AP_NAME	"HOME_AP"
//#define AP_PASS "0123456789"

/* Define stream URL here. For example, the URL to the MP3 stream of a certain Dutch radio station
is http://icecast.omroep.nl/3fm-sb-mp3 . This translates of a server name of "icecast.omroep.nl"
and a path of "/3fm-sb-mp3". The port usually is 80 (the standard HTTP port) */
#if 0
#define PLAY_SERVER "icecast.omroep.nl/3fm-alternative-mp3" // "/3fm-sb-mp3" // "/3fm-serioustalent-mp3" // "/funx-amsterdamfb-bb-mp3" //
#define PLAY_PORT 80
#endif
#if 1
#define PLAY_SERVER "icecast.omroep.nl/3fm-sb-mp3"  // "/funx-amsterdamfb-bb-mp3" //
#define PLAY_PORT 80
#endif
#if 0
#define PLAY_SERVER "icecast.omroep.nl/3fm-serioustalent-mp3" // "/funx-amsterdamfb-bb-mp3"
#define PLAY_PORT 80
#endif
/*
Here's a DI.fm stream
*/
#if 0
#define PLAY_SERVER "pub7.di.fm/di_classiceurodance"
#define PLAY_PORT 80
#endif

/* You can use something like this to connect to a local mpd server which has a configured 
mp3 output: */
#if 0
#define PLAY_SERVER "192.168.33.128/"
#define PLAY_PORT 8000
#endif

/* You can also play a non-streaming mp3 file that's hosted somewhere. WARNING: If you do this,
make sure to comment out the ADD_DEL_SAMPLES define below, or you'll get too fast a playback 
rate! */
#if 0
#define PLAY_SERVER "meuk.spritesserver.nl/Ii.Romanzeandante.mp3"
#define PLAY_PORT 80
#endif


/*Playing a real-time MP3 stream has the added complication of clock differences: if the sample
clock of the server is a bit faster than our sample clock, it will send out mp3 data faster
than we process it and our buffer will fill up. Conversely, if the server clock is slower, we'll
eat up samples quicker than the server provides them and we end up with an empty buffer.
To fix this, the mp3 logic can insert/delete some samples to modify the speed of playback.
If our buffers are filling up too fast (presumably due to a quick sample clock on the other side)
we will increase our playout speed; if our buffers empty too quickly, we will decrease it a bit.
Unfortunately, adding or deleting samples isn't very good for the audio quality. If you
want better quality, turn this off and/or feel free to implement a better algorithm.
WARNING: Don't use this define if you play non-stream files. It will presume the sample clock
on the server side is waaay too fast and will default to playing back the stream too fast.*/
#define ADD_DEL_SAMPLES


/*While connecting an I2S codec to the I2S port of the ESP is obviously the best way to get nice
16-bit sounds out of the ESP, it is possible to run this code without the codec. For
this to work, instead of outputting a 2x16bit PCM sample the DAC can decode, we use the I2S
port as a makeshift 6.5-bit PWM generator. To do this, we map every mp3 sound sample to a
value that has an amount of 1's set that's linearily related to the sound samples value and
then output that value on the I2S port. The net result is that the average analog value on the 
I2S data pin corresponds to the value of the MP3 sample we're trying to output. Needless to
say, a hacked 6.5-bit PWM output is going to sound a lot worse than a real I2S codec.*/
#define PWM_HACK96BIT

/*
 * Oversamples x2 low ratio stream (>=48k). Only PWM_HACK.
 */
#define OVERSAMPLES

typedef struct _USR_server_setings
{
	u16 port;
	u8 url[128];
} mp3_server_setings;

extern mp3_server_setings mp3_serv;

#endif

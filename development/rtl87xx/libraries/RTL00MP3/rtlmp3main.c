/******************************************************************************
 *
 * FileName: user_main.c
 * RTL8710: (c) kissste, pvvx
 *
 *******************************************************************************/
#include "rtl_common.h"
#include "rtl8195a.h"
#include "hal_log_uart.h"

#include "FreeRTOS.h"
#include "task.h"
//#include "diag.h"
#include "osdep_service.h"
#include "device_lock.h"
#include "semphr.h"
#include "queue.h"

#include <wifi/wifi_conf.h>
#include <wifi/wifi_util.h>

#include "lwip/sockets.h"
#include "lwip/err.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "dhcp/dhcps.h"

#include "mad.h"
#include "stream.h"
#include "frame.h"
#include "synth.h"
#include "i2s_freertos.h"
#include "spiram_fifo.h"
#include "playerconfig.h"
#include "main.h"

#define DEBUG_MAIN_LEVEL 2

//Priorities of the reader and the decoder thread. Higher = higher prio. (ESP8266!)
//RTL87xx Higher = lower prio ?
//#define PRIO_READER (configMAX_PRIORITIES - 2) // (tskIDLE_PRIORITY + PRIORITIE_OFFSET)
//#define PRIO_MAD (PRIO_READER - 1) // PRIO_READER + n; (TCPIP_THREAD_PRIO = (configMAX_PRIORITIES - 2))
#define PRIO_MAD (tskIDLE_PRIORITY + 3 + PRIORITIE_OFFSET)
#define PRIO_READER (PRIO_MAD + 3) // max 11 ?


#define mMIN(a, b)  ((a < b)? a : b)

//The mp3 read buffer size. 2106 bytes should be enough for up to 48KHz mp3s according to the sox sources. Used by libmad.
#define READBUFSZ (2106)
#define MAX_FIFO_SIZE (16*1024) // min 4*1024 (CPU CLK 166), min 8*1024 (CPU CLK 83MHz), absolute work min = 3*READBUFSZ
//#define MIN_FIFO_HEAP (8*1024)
#define SOCK_READ_BUF (256)

unsigned char *readBuf;
char oversampling = 1;
volatile char tskmad_enable, tskreader_enable;
static long bufUnderrunCt;

#define sampToOut i2sPushPWMSamples

#ifdef ADD_DEL_SAMPLES // correct smpr
static char sampCntAdd;
static char sampDelCnt;
static int sampCnt;
#endif

mp3_server_setings mp3_serv = {0,{0}}; //{ PLAY_PORT, { PLAY_SERVER }};

// Called by the NXP modifications of libmad. It passes us (for the mono synth)
// 32 16-bit samples.
void render_sample_block(short *short_sample_buff, int no_samples) {
	int i;
	for (i = 0; i < no_samples; i++) {
		int x = oversampling;
#ifdef ADD_DEL_SAMPLES // correct smpr
		if(++sampCnt > 150) {
			sampCnt = 0;
			if (sampDelCnt < 0) {
				//...and don't output an i2s sample
				sampDelCnt--;
				x = 0;
			}
			else if (sampDelCnt > 0)  {
				//..and output 2 samples instead of one.
				sampDelCnt++;
				x++;
			}
		}
#endif
		while(x--) sampToOut((short_sample_buff[i] << 16) | (u16)short_sample_buff[i+no_samples]);
	}
}

//Called by the NXP modifications of libmad. Sets the needed output sample rate.
static int oldRate = 0;
void set_dac_sample_rate(int rate, int chls) {
	if (rate == oldRate) return;
	oldRate = rate;
#if DEBUG_MAIN_LEVEL > 0
	DBG_8195A("MAD: Rate %d, channels %d\n", rate, chls);
#endif
	oversampling = i2sSetRate(-1, rate);
}

static enum mad_flow input(struct mad_stream *stream) {
	int n, i;
	int rem; //, fifoLen;
	//Shift remaining contents of buf to the front
	rem = stream->bufend - stream->next_frame;
	memmove(readBuf, stream->next_frame, rem);

	while (rem < READBUFSZ) {
		n = (READBUFSZ - rem); // Calculate amount of bytes we need to fill buffer.
		i = RamFifoFill();
		if (i < n)	n = i; 		// If the fifo can give us less, only take that amount
		if (n == 0) {		// Can't take anything?
			// Wait until there is enough data in the buffer. This only happens when the data feed
			// rate is too low, and shouldn't normally be needed!
//			DBG_8195A("Buf uflow, need %d bytes.\n", sizeof(readBuf)-rem);
			bufUnderrunCt++;
			// We both silence the output as well as wait a while by pushing silent samples into the i2s system.
			// This waits for about 200mS
//#if DEBUG_MAIN_LEVEL > 1
//			DBG_8195A("FIFO: Buffer Underrun\n");
//#endif
			for (n = 0; n < 441*2; n++)	sampToOut(0);
		} else {
			//Read some bytes from the FIFO to re-fill the buffer.
			RamFifoRead(&readBuf[rem], n);
			rem += n;
		}
#ifdef ADD_DEL_SAMPLES
		if(i < READBUFSZ) {
			sampCntAdd = 10; // add samples
		}
		else if(RamFifoLen() - i < SOCK_READ_BUF) { // fifo free < SOCK_READ_BUF

			sampCntAdd = -1; // del samples
		}
		else {
				sampCntAdd++; // add samples
		}
		sampDelCnt += sampCntAdd;
#endif
	}
	//Okay, let MAD decode the buffer.
	mad_stream_buffer(stream, readBuf, READBUFSZ);
	return MAD_FLOW_CONTINUE;
}

//Routine to print out an error
static enum mad_flow error(void *data, struct mad_stream *stream,
		struct mad_frame *frame) {
#if DEBUG_MAIN_LEVEL > 0
	DBG_8195A("MAD: Dec err 0x%04x (%s)\n", stream->error,
			mad_stream_errorstr(stream));
#endif
	return MAD_FLOW_CONTINUE;
}

void tskreader(void *pvParameters);

//This is the main mp3 decoding task. It will grab data from the input buffer FIFO in the SPI ram and
//output it to the I2S port.
void tskmad(void *pvParameters) {
	//Initialize I2S
	if (i2sInit(-1, I2S_DMA_PAGE_WAIT_MS_MIN * I2S_DMA_PAGE_SIZE_MS_96K, WORD_LEN)) { // min 2 ms x I2S_DMA_PAGE_SIZE buffers
		//Allocate structs needed for mp3 decoding
		char * mad_bufs = pvPortMalloc(
				sizeof(struct mad_stream) + sizeof(struct mad_frame)
						+ sizeof(struct mad_synth) + READBUFSZ);
		if (mad_bufs == NULL) {
#if DEBUG_MAIN_LEVEL > 0
			DBG_8195A("MAD: Alloc failed\n");
#endif
			goto exit;
		}
		rtl_memset(mad_bufs, 0,
				sizeof(struct mad_stream) + sizeof(struct mad_frame)
						+ sizeof(struct mad_synth));
#if DEBUG_MAIN_LEVEL > 0
		DBG_8195A("MAD: Alloc %d bytes at %p\n",
				sizeof(struct mad_stream) + sizeof(struct mad_frame) + sizeof(struct mad_synth) + READBUFSZ,
				mad_bufs);
#endif
		struct mad_stream *stream = mad_bufs;
		struct mad_frame *frame = &mad_bufs[sizeof(struct mad_stream)];
		struct mad_synth *synth = &mad_bufs[sizeof(struct mad_stream)
				+ sizeof(struct mad_frame)];
		readBuf = &mad_bufs[sizeof(struct mad_stream) + sizeof(struct mad_frame)
				+ sizeof(struct mad_synth)];

		bufUnderrunCt = 0;
		oldRate = 0;
		oversampling = 1;
#ifdef ADD_DEL_SAMPLES
		sampCntAdd = 0;
		sampCnt = 0;
		sampDelCnt = 0;
#endif
		//Initialize mp3 parts
#if DEBUG_MAIN_LEVEL > 0
		DBG_8195A("MAD: Decoder start.\n");
#endif
		mad_stream_init(stream);
		mad_frame_init(frame);
		mad_synth_init(synth);
		while (tskmad_enable == 1) {
			input(stream); //calls mad_stream_buffer internally
			while (tskmad_enable == 1) {
#if DEBUG_MAIN_LEVEL > 3
				DBG_8195A("MAD: Frame decode.\n");
#endif
				int r = mad_frame_decode(frame, stream);
				if (r == -1) {
#if DEBUG_MAIN_LEVEL > 2
					 DBG_8195A("MAD: Frame error.\n");
#endif
					if (!MAD_RECOVERABLE(stream->error)) {
						//We're most likely out of buffer and need to call input() again
						break;
					}
					error(NULL, stream, frame);
					continue;
				}
#if DEBUG_MAIN_LEVEL > 3
				DBG_8195A("MAD: Frame synth.\n");
#endif
				mad_synth_frame(synth, frame);
			}
		};
		mad_synth_finish(synth);
		mad_frame_finish(frame);
		mad_stream_finish(stream);
		vTaskDelay(10);
		vPortFree(mad_bufs);
	}
#if DEBUG_MAIN_LEVEL > 0
	DBG_8195A("MAD: Closed.\n");
#endif
exit:
	i2sClose(-1);
	tskreader_enable = 0;
	tskmad_enable = -1;
	vTaskDelete(NULL);
}

int getIpForHost(const char *host, struct sockaddr_in *ip) {
	struct hostent *he;
	struct in_addr **addr_list;
	he = gethostbyname(host);
	if (he == NULL) return 0;
	addr_list = (struct in_addr **) he->h_addr_list;
	if (addr_list[0] == NULL) return 0;
	ip->sin_family = AF_INET;
	memcpy(&ip->sin_addr, addr_list[0], sizeof(ip->sin_addr));
	return 1;
}

//Open a connection to a webserver and request an URL. Yes, this possibly is one of the worst ways to do this,
//but RAM is at a premium here, and this works for most of the cases.
int openConn(const char *streamHost, const char *streamPath, int streamPort) {
	int n = 5;
	while (tskreader_enable == 1) {
		struct sockaddr_in remote_ip;
		bzero(&remote_ip, sizeof(struct sockaddr_in));
		if (!getIpForHost(streamHost, &remote_ip)) {
			vTaskDelay(1000 / portTICK_RATE_MS);
			if(n--)	continue;
#if DEBUG_MAIN_LEVEL > 0
			DBG_8195A("MP3: Not get IP server <%s>!\n", streamHost);
#endif
			return -1;
		}
		int sock = socket(PF_INET, SOCK_STREAM, 0);
		if (sock == -1) {
#if DEBUG_MAIN_LEVEL > 0
			DBG_8195A("MP3: Not open socket!\n");
#endif
//			tskreader_enable = 0;
			return -1;
		}

		remote_ip.sin_port = htons(streamPort);
#if DEBUG_MAIN_LEVEL > 0
		DBG_8195A("MP3: Connecting to server %s...\n",
				ipaddr_ntoa((const ip_addr_t* )&remote_ip.sin_addr.s_addr));
#endif
		if (connect(sock, (struct sockaddr * )(&remote_ip),
				sizeof(struct sockaddr)) != 00) {
			close(sock);
#if DEBUG_MAIN_LEVEL > 0
			DBG_8195A("MP3: Connect error!\n");
#endif
//			vTaskDelay(1000 / portTICK_RATE_MS);
//			continue;
			return -1;
		}
		//Cobble together HTTP request
		write(sock, "GET ", 4);
		write(sock, streamPath, strlen(streamPath));
		write(sock, " HTTP/1.0\r\nHost: ", 17);
		write(sock, streamHost, strlen(streamHost));
		write(sock, "\r\n\r\n", 4);
		//We ignore the headers that the server sends back... it's pretty dirty in general to do that,
		//but it works here because the MP3 decoder skips it because it isn't valid MP3 data.
		return sock;
	}
	return -1;
}


int http_head_read(unsigned char *buf, int len, int ff) {
	int flg_head = 0;
	int n, ret = 0;
	if ((n = read(ff, buf, len)) <= 0)	return 0;
	if(n > 11 && *((u32 *)buf) == 0x50545448) { // "HTTP" // HTTP/1.0 200 OK
		int x;
		for(x = 3; x < n && buf[x] != ' '; x++);
		while(x < n && buf[x] == ' ') x++;
		if(x < n) ret = atoi(&buf[x]);
		int cnt = 0;
		x = 0;
		while(ret) {
			int z = 0;
			while (x < n) {
				if (cnt++ > 16384)	return 600; // Header Too Large
				if (buf[x++] == ((flg_head & 1) ? 0x0a : 0x0d)) {
					if ((flg_head & 3) == 1) {
#if DEBUG_MAIN_LEVEL > 0
						buf[x-1] = 0;
						DBG_8195A("%s\n", &buf[z]);
#endif
						z = x;
					}
					if (flg_head >= 3) {
						if (n - x > 0) RamFifoWrite(&buf[x], n - x);
#if DEBUG_MAIN_LEVEL > 1
						DBG_8195A("MP3: Skip HTTP head in %d bytes\n\n", cnt);
#endif
						return ret;
					}
					flg_head++;
				}
				else flg_head = 0;
			}
			x = 0;
			while(z < n) buf[x++] = buf[z++];
			if ((n = read(ff, &buf[x], len - x)) <= 0) return 601; // content ??
			n += x;
		};
	}
	else RamFifoWrite(buf, n);
	return ret;
}

// Reader task. This will try to read data from a TCP socket into the SPI fifo buffer.
void tskreader(void *pvParameters) {
	char wbuf[SOCK_READ_BUF];
	int n;
	if (RamFifoInit(MAX_FIFO_SIZE)) {
#if I2S_DEBUG_LEVEL > 1
		unsigned int t = xTaskGetTickCount();
#endif
		while (tskreader_enable == 1) {
			n = strlen(mp3_serv.url);
			int i;
			u8 * uri = NULL;
			for(i = 0; i < n; i++) {
				wbuf[i] = mp3_serv.url[i];
				if(wbuf[i] == '/') {
					wbuf[i] = 0;
					uri = &mp3_serv.url[i];
					break;
				}
			}
			if(uri == NULL) {
#if DEBUG_MAIN_LEVEL > 0
				DBG_8195A("MP3: Error url <%s>!\n", mp3_serv.url);
#endif
				tskreader_enable = 0;
				break;
			}
			int fd = openConn(wbuf, uri, mp3_serv.port);
			if(fd < 0) {
				tskreader_enable = 0;
				break;
			}
			if ((n = http_head_read(wbuf, sizeof(wbuf), fd)) != 200) {
#if DEBUG_MAIN_LEVEL > 0
				DBG_8195A("MP3: HTTP error %d\n", n);
#endif
				tskreader_enable = 0;
				break;
			}
			else do {
				n = read(fd, wbuf, sizeof(wbuf));
				// DBG_8195A("Socket read %d bytes\n", n);
				if (n > 0)	RamFifoWrite(wbuf, n);
				if ((tskmad_enable != 1) && (RamFifoFree() < RamFifoLen() / 2)) {
#if DEBUG_MAIN_LEVEL > 0
					DBG_8195A("FIFO: Start Buffer fill %d\n", RamFifoFill());
#endif
					// Buffer is filled. Start up the MAD task. Yes, the 2100 words of stack is a fairly large amount but MAD seems to need it.
					tskmad_enable = 1;
					if (xTaskCreate(tskmad, "tskmad", 2100, NULL, PRIO_MAD,	NULL) != pdPASS) {
#if DEBUG_MAIN_LEVEL > 0
						DBG_8195A("MP3: Error creating MAD task! Out of memory?\n");
#endif
						tskmad_enable = 0;
						tskreader_enable = 0;
						break;
					}
				}
#if I2S_DEBUG_LEVEL > 1
				if (xTaskGetTickCount() - t > 3000) {
					t = xTaskGetTickCount();
					DBG_8195A("MP3: Buffer fill %d, DMA underrun ct %d, buff underrun ct %d\n", RamFifoFill(), (int )i2sGetUnderrunCnt(), bufUnderrunCt);
				}
#endif
			} while (n > 0 && (tskreader_enable == 1));
			if(fd >= 0) {
#if DEBUG_MAIN_LEVEL > 1
				if(n == 0) {
					u32 err;
					socklen_t slen = sizeof(err);
					if(!lwip_getsockopt(fd, SOL_SOCKET, SO_ERROR, &err, &slen)) {
						DBG_8195A("MP3: Socket error %d\n", err);
					}
				}
#endif
				close(fd);
			}
		}
#if DEBUG_MAIN_LEVEL > 0
		DBG_8195A("MP3: Connection closed.\n");
#endif
	}
	if(tskmad_enable == 1) {
		tskmad_enable = 0;
		while (tskmad_enable == 0) vTaskDelay(2);
	}
	RamFifoClose();
#if DEBUG_MAIN_LEVEL > 2
	DBG_8195A("\nMP3: Task reader closed.\n");
#endif
	tskreader_enable = -1;
	vTaskDelete(NULL);
}

//We need this to tell the OS we're running at a higher clock frequency.
//sk//extern void os_update_cpu_frequency(int mhz);

void rtl_mp3_close(void) {
	if (tskreader_enable == 1) {
		tskreader_enable = 0;
		while(tskreader_enable == 0) vTaskDelay(2);
		tskreader_enable = 0;
	}
}

void rtl_mp3_start(char *url, int port) {

#if DEBUG_MAIN_LEVEL > 1
	ConfigDebugErr  = -1;
	ConfigDebugInfo = -1;
	ConfigDebugWarn = -1;
#elif DEBUG_MAIN_LEVEL > 0
	ConfigDebugErr |= _DBG_MISC_;
#endif
	
	rtl_mp3_close();
	int i = strlen(url);
	if(port != 0 && i > 2) {
		if(i > sizeof(mp3_serv.url) -1) {
#if DEBUG_MAIN_LEVEL > 0
			DBG_8195A("MP3: url to long!\n");
			return;
		}
#endif
		mp3_serv.port = port;
		strcpy(mp3_serv.url, url);
#if DEBUG_MAIN_LEVEL > 0
		DBG_8195A("MP3: Connect url: %s:%d\n", mp3_serv.url, mp3_serv.port);
#endif
		//Fire up the reader task. The reader task will fire up the MP3 decoder as soon
		//as it has read enough MP3 data.
		tskreader_enable = 1;
		if (xTaskCreate(tskreader, "tskreader", 300, NULL, PRIO_READER,	NULL) != pdPASS) {
#if DEBUG_MAIN_LEVEL > 0
			DBG_8195A("\n\r%s xTaskCreate(tskreader) failed", __FUNCTION__);
#endif
			tskreader_enable = 0;
		}
	}
#if DEBUG_MAIN_LEVEL > 0
	else {
		DBG_8195A("MP3: No set url!\n");
	}
#endif
}


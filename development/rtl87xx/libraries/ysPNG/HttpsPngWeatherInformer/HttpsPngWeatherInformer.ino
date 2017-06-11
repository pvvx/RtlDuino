#include <WiFi.h>
#include <Raw_ST7735.h> // Hardware-specific library
#include <SPI.h>
#include "yspng.h"

#include <myAP.h> // там прописаны pass и ssid и #define password pass для быстрой замены в разных примерах... 
//char ssid[] = "yourNetwork"; //  your network SSID (name)
//char pass[] = "secretPassword";    // your network password (use for WPA, or use as key for WEP)

WiFiSSLClient client;

char server[] = "info.weather.yandex.net"; // https://yandex.ru/pogoda/2/informer
unsigned short sity_tab[] = {2, 10883, 969, 213, 0};
char sity_idx;


// These pins will also work for the 1.8" TFT shield
#define TFT_CS    8 // 10  // PC_0 //10
#define TFT_RST   5 // 2   // PA_5  //9  // you can also connect this to the Arduino reset
// in which case, set this #define pin to 0!
#define TFT_DC    4 // 5   // PA_4 //8
#define TFT_SCLK  9 // 13  // PC_1 // set these to be whatever pins you like!
#define TFT_MOSI  10 // 11  // PC_2 // set these to be whatever pins you like!
//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
Raw_ST7735 tft = Raw_ST7735(TFT_CS, TFT_DC, TFT_RST);
YsPngBinaryMemoryStream pngms(12 * 1024, true);

extern "C" {
  extern int  ssl_max_frag_len;
  void UserPreInit(void)
  {
    //if (HalGetCpuClk() < 100000000)
//    Init_CPU_CLK_UART(1, 38400);
    // 0 - 166,666,666 Hz, 1 - 83,333,333 Hz, 2 - 41,666,666 Hz, 3 - 20,833,333 Hz, 4 - 10,416,666 Hz, 5 - 4,000,000 Hz
    // 6 - 200,000,000 Hz, 7 - 100,000,000 Hz, 8 - 50,000,000 Hz, 9 - 25,000,000 Hz, 10 - 12,500,000 Hz, 11 - 4,000,000 Hz
    /* if ssl_handshake returned -0x7200 -> SSL_MAX_CONTENT_LEN to sufficient size
      (minimum value is 512, maximum value is 16384, default value 8192 !) */
    ssl_max_frag_len = 10240; // 512..16384, www.google.ru > 10240, github.com > 3500.
    /* ssl used heap ~ alloc ssl_max_frag_len * 2.5 (!) */
  }
} // extern "C"

class ST7735PngDecoder : public YsRawPngDecoder {
  public:
    void fnDraw(int x, int y, unsigned int rgba);
    int fnPreDraw (int wid, int hei, int bitDepth, int colorType);
};

void ST7735PngDecoder::fnDraw(int x, int y, unsigned int rgba) {
  // преобразование RGBA в формат данного дисплея
#if 0
  tft.drawPixel(x + 3, y + 3, tft.Color565((uint8_t)rgba, (uint8_t)(rgba >> 8), (uint8_t)(rgba >> 16)));
#else
  unsigned short c = ((rgba & 0xF8) << 8) | ((rgba & 0xFC00) >> 5) | ((rgba & 0xFFFFFF) >> 19);
  uint8_t hi = c >> 8, lo = c;
  tft.writedata(hi);
  tft.writedata(lo);
#endif
}

int ST7735PngDecoder::fnPreDraw(int wid, int hei, int bitDepth, int colorType) {
  uint16_t x = tft.width(), y = tft.height();
  printf("PNG: %dx%d, %d, %d\r\n", wid, hei, bitDepth, colorType);
  // проверка - влезет или нет в дисплей?
  if (wid > x || hei > y) {
    printf("PNG: not fit!\r\n");
    return YSERR;
  }
  // Старт вывода в дисплей (задание окна)
  x = (x - wid) >> 1;
  y = (y - hei) >> 1;
  tft.setAddrWindow(x, y, x + wid - 1, y + hei - 1);
  return YSOK;
}

ST7735PngDecoder png;

void setup() {
  SPI.setDefaultFrequency(20000000);
  // Use this initializer if you're using a 1.8" TFT 128x160
  tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
  // tft.fillScreen(ST7735_BLACK);
  // wdt_enable(30000);
}

void WiFiClose(void)
{  
  WiFi.disconnect();
  delay(200);
  WiFi.off();
  delay(200);
  sys_info();
}

void chow_error(int i)
{
  while (i--) {
    delay(256);
    tft.invertDisplay(true);
    delay(256);
    tft.invertDisplay(false);
  }
}

void loop() {
  //  sys_info();
  // wdt_reset();
  // Connect to WPA/WPA2 network.
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) {
    delay(100); // wait 0.1 seconds for connection
  }
  //  printf("\r\nConnected to wifi\r\n");
  if (client.connect(server, 443)) {
    //    printf("connected to server\r\n");
    // Make a HTTP request:
    char * buf = new char [512];
    sprintf(buf, "GET /%u/3_white.ru.png HTTP/1.1\r\n\Host: %s\r\n\Connection: close\r\n", sity_tab[sity_idx++], server);
    if (!sity_tab[sity_idx]) sity_idx = 0;
    client.println(buf);
    delete [] buf;
    while (client.connected()) {
      // if there are incoming bytes available
      // from the server, read them and print them:
      while (client.available()) {
        if (client.read() == 13 && client.read() == 10
            && client.read() == 13 && client.read() == 10) {
          pngms.ResetStream();
          while (client.available() && pngms.Write((unsigned char)client.read()));
          client.flush();
//          delay(100);
          client.stop();
          printf("\r\nContent len %d\r\n", pngms.offset_wr);
          WiFiClose();
          if (pngms.offset_wr > 1024) png.Decode(pngms);
          delay(3000);
          return;
        }
      }
    }
    client.stop();
    chow_error(2);
  } else {
//    printf("connected to server failed");
    chow_error(3);
  }
  delay(3000);
}



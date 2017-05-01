/*
 * Test WebSocketClient + SSL/TSL
 * RTL8710AF pvvx 12/12/2016
 *
 */

#include <WiFi.h>
#include <WebSocketClient.h>
#include "myAP.h"

#define USE_SSL_WEBSOCKED 1 

#ifndef _MYAPCFG_H_
char ssid[] = "yourNetwork"; //  your network SSID (name)
char pass[] = "password";    // your network password (use for WPA, or use as key for WEP)
#endif

char * ws_state[4] = { "CLOSING", "CLOSED", "CONNECTING", "OPEN" };

#if USE_SSL_WEBSOCKED
extern "C" {
 extern int  ssl_max_frag_len;
  void UserPreInit(void)
  {
    /* if ssl_handshake returned -0x7200 -> SSL_MAX_CONTENT_LEN to sufficient size
      (minimum value is 512, maximum value is 16384, default value 8192 !) */
    ssl_max_frag_len = 10240; // 512..16384, www.google.ru > 10240, github.com > 3500.
    /* ssl used heap ~ alloc ssl_max_frag_len * 2.5 (!) */
  }
} // extern "C"
#endif    

void setup() {
  WiFi.begin(ssid, pass);
}

#if USE_SSL_WEBSOCKED
char host[] = "wss://echo.websocket.org";
#define host_port 443
#else
char host[] = "ws://echo.websocket.org";
#define host_port 80
#endif
char path[] = "?encoding=text";
char origin[] = "http://websocket.org";

char txtmsg[] = "Rock it with HTML5 WebSocket";
uint8_t binmsg[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

volatile char rx_msg;
char tx_code;

void ws_message(wsclient_context * ws_client, int msg_size) {
  if(msg_size) {
    switch(tx_code) {
      case 1: // TEXT_FRAME
        printf("ws: rx txtmsg[%d]: '%s'\n", msg_size, ws_client->receivedData);
        break;
      case 2: // BINARY_FRAME
        printf("ws: rx binmsg[%d]:\n");
        hexdump(ws_client->receivedData, msg_size);
        break;
    }
  }
  rx_msg = 1;
}

int wait_msg(WebSocketClient *ws, uint32_t timeout_ms) {
      uint32_t time = millis();
      while(!rx_msg) {
        ws->poll(1);
        if(ws->getReadyState() == CLOSED) return -2;
        if(millis() - time > timeout_ms) return -1;
        delay(1);
      }
      return 0;
}

void loop() {
  printf("\n");
  if (WiFi.status() == WL_CONNECTED) {
    printf("\nConnect %s ...\n", host);
    WebSocketClient *ws = new WebSocketClient(host, host_port, path, origin);
#if USE_SSL_WEBSOCKED
    ws->ssl_func_on();
#endif    
    ws->dispatch(ws_message);
    if(ws->connect() >= 0 ) {
      printf("ws: connected, state = %s\n", ws_state[ws->getReadyState()]);
      printf("ws: send text msg\n");
      rx_msg = 0;
      tx_code = 1;
      uint32_t time = millis();
      ws->send(txtmsg, sizeof(txtmsg), 1);
      if(wait_msg(ws, 5000) == 0) {
        time = millis() - time;
        printf("ping %d ms\n", time);
      
        printf("ws: send binary\n");
        time = millis();
        rx_msg = 0;
        tx_code = 2;
        ws->sendBinary(binmsg, sizeof(binmsg), 1);
        if(wait_msg(ws, 5000) == 0) {
          time = millis() - time;
          printf("ping %d ms\n", time);
        }
        else printf("Timeout!\n");
      }
      else printf("Timeout!\n");
    }
    ws->close();
    delete ws;
  }
  sys_info();
  delay(30000);
}


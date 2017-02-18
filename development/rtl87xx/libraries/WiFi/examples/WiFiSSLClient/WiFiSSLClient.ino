#include <WiFi.h>
#include <myAP.h>

//#if defined(BOARD_RTL8710)
extern "C" {
  void UserPreInit(void)
  {
    //if (HalGetCpuClk() < 100000000) 
        Init_CPU_CLK_UART(7, 38400);
    // 0 - 166,666,666 Hz, 1 - 83,333,333 Hz, 2 - 41,666,666 Hz, 3 - 20,833,333 Hz, 4 - 10,416,666 Hz, 5 - 4,000,000 Hz
    // 6 - 200,000,000 Hz, 7 - 100,000,000 Hz, 8 - 50,000,000 Hz, 9 - 25,000,000 Hz, 10 - 12,500,000 Hz, 11 - 4,000,000 Hz
    /* if ssl_handshake returned -0x7200 -> SSL_MAX_CONTENT_LEN to sufficient size
      (minimum value is 512, maximum value is 16384, default value 8192 !) */
    ssl_max_frag_len = 10240; // 512..16384, www.google.ru > 10240, github.com > 3500.
    /* ssl used heap ~ alloc ssl_max_frag_len * 2.5 (!) */
  }
} // extern "C"
//#endif // BOARD_RTL8710

#ifndef _MYAPCFG_H_
char ssid[] = "yourNetwork"; //  your network SSID (name)
char pass[] = "password";    // your network password (use for WPA, or use as key for WEP)
#endif //_MYAPCFG_H_
int keyIndex = 0;            // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;

char server[] = "yandex.ru";    // name address for Google (using DNS)

WiFiSSLClient client;

void setup() {
  //Initialize serial and wait for port to open:
  delay(10);
  Serial.begin(38400);
  while (!Serial) ; // wait for serial port to connect. Needed for native USB port only
  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 0.5 seconds for connection:
    delay(500);
  }
  Serial.println("\r\nConnected to wifi");
  printWifiStatus();

  sys_info();
  debug_on();

  Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  if (client.connect(server, 443)) { //client.connect(server, 443, test_ca_cert, test_client_cert, test_client_key)
    Serial.println("connected to server");
    // Make a HTTP request:
    client.println("GET / HTTP/1.0");
    client.print("Host: "); client.println(server); //    client.println("Host: www.google.com");
    client.println("Connection: close");
    client.println();
    sys_info();
  }
  else
    Serial.println("connected to server failed");
}

void loop() {
  // if there are incoming bytes available
  // from the server, read them and print them:
  while (client.available()) {
    char c = client.read();
    Serial.write(c);
  }

  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting from server.");
    sys_info();
    client.stop();

    // do nothing forevermore:
    while (true);
  }
}


void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  Serial.print(mac[0], HEX);
  Serial.print(":");
  Serial.print(mac[1], HEX);
  Serial.print(":");
  Serial.print(mac[2], HEX);
  Serial.print(":");
  Serial.print(mac[3], HEX);
  Serial.print(":");
  Serial.print(mac[4], HEX);
  Serial.print(":");
  Serial.println(mac[5], HEX);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

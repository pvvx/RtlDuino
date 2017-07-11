#include <WiFi.h>
#include <NfcTag.h>
#include <netbios.h>
#include "myAP.h"

#define LED_pin 4

#ifndef _MYAPCFG_H_
char ssid[] = "yourNetwork";      // your network SSID (name)
char pass[] = "Password";   // your network password
#endif
char myIpString[24];

WiFiServer server(80);
IPAddress ip;

void setup() {
  pinMode(LED_pin, OUTPUT);
  //Initialize serial and wait for port to open:
  Serial.begin(38400);
  while (!Serial); // wait for serial port to connect. Needed for native USB port only
  // attempt to connect to Wifi network:
  while (WiFi.begin(ssid, pass) != WL_CONNECTED) delay(100);
  printf("\nStart server\n");
  server.begin();
  ip = WiFi.localIP();
  sprintf(myIpString, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  NfcTag.appendRtdUri(myIpString, RTD_URI_HTTP);
  NfcTag.begin();
  Serial.println("\nNFC is OK!");
}

void loop() {
  WiFiClient client = server.available();

  Serial.println(client);
  if (client) {
    Serial.println("Start client");
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        if (c == '\n') {
          if (currentLine.length() == 0) {

            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
            client.println("<html>");
            client.print("<table align='center' border='5'>");
            client.print("<tr>");
            client.print("<td colspan='2' align='center'>Control Led</td>");
            client.print("</tr>");
            client.print("<td><input type='button' value='ON' onclick=self.location.href=\"/on\" style='width:90px;height:30px;font-size:20px;'></td>");
            client.print("<td><input type='button' value='OFF' onclick=self.location.href=\"/off\" style='width:90px;height:30px;font-size:20px;'></td>");
            client.print("</table>");
            client.println();
            client.println("</html>");
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
        if (currentLine.endsWith("GET /on")) {
          digitalWrite(LED_pin, LOW);
        } else  if (currentLine.endsWith("GET /off")) {
          digitalWrite(LED_pin, HIGH);
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("client disonnected");
  }
}



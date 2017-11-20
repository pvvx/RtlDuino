#include <PadiWiFi.h>
#include <PadimDNS.h>
#include <ArduinoOTA.h>
#include <PadiWebServer.h>

const char* ssid = "........";
const char* password = "........";

PadiWebServer server(80);

const char* www_username = "admin";
const char* www_password = "Padi";

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if(WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Connect Failed! Rebooting...");
    delay(1000);
    ESP.restart();
  }
  ArduinoOTA.begin();

  server.on("/", [](){
    if(!server.authenticate(www_username, www_password))
      return server.requestAuthentication();
    server.send(200, "text/plain", "Login OK");
  });
  server.begin();

  Serial.print("Open http://");
  Serial.print(WiFi.localIP());
  Serial.println("/ in your browser to see it working");
}

void loop() {
  ArduinoOTA.handle();
  server.handleClient();
}

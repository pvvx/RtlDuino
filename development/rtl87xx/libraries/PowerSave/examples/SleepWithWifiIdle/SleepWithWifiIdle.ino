/*
   Demonstrates use of sleep api of power management with wlan connected

   This sketch enable sleep mode after wifi connected.
   You can ping Ameba's IP while Ameba is under sleep mode and Ameba will
   echo back. If you ping with high frequency and network traffic becomes high,
   then Ameba will keep awake until traffic becomes low.
*/

#include <WiFi.h>
#include <PowerManagement.h>

#include <myAP.h>
//char ssid[] = "yourNetwork";     // your network SSID (name)
//char pass[] = "secretPassword";  // your network password
int status = WL_IDLE_STATUS;     // the Wifi radio's status

extern "C" void pmu_get_wakelock_hold_stats(char *pcWriteBuffer);
extern "C" void pmu_enable_wakelock_stats(unsigned char enable);

void setup() {

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    // wait 0.1 seconds for connection:
    delay(100);
  }

  // you're connected now, so print out the data:
  Serial.print("You're connected to the network");
  printCurrentNet();
  printWifiData();
  /* Set Station LPS DTIM(4) */
  WiFi.SetDTIM(4);

  /*  If you need any peripheral while sleep, remove below line.
      But it makes Ameba save less power (around 5.5 mA). */
  PowerManagement.setPllReserved(false);

  /* Make Ameba automatically suspend and resume while there is no on-going task. */
  /*
     PMU_OS -  bit0,
     PMU_WLAN_DEVICE - bit1,
     PMU_LOGUART_DEVICE - bit2,
     PMU_SDIO_DEVICE - bit3
  */
  PowerManagement.sleep(5);
  /* Enable Log:
    SSID: HOME_AP
    BSSID: 90:9:EB:C5:12:34
    signal strength (RSSI):-27
    Encryption Type:4

    wakelock_id     holdtime
    0               43883662
    1               7364455
    time passed: 43883662 ms, system sleep 35930238 ms
  */
  /* (sleep < 2.5 mA, run task < 62 mA) -> 82% sleep: ~13.3 mA  */
  pmu_enable_wakelock_stats(1);
}

void loop() {
  char buf[256];
  // check the network connection once every 10 seconds:
  delay(10000);
  printCurrentNet();
  pmu_get_wakelock_hold_stats(buf);
  Serial.println(buf);
}

void printWifiData() {
  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.println(ip);

  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  Serial.print(mac[5], HEX);
  Serial.print(":");
  Serial.print(mac[4], HEX);
  Serial.print(":");
  Serial.print(mac[3], HEX);
  Serial.print(":");
  Serial.print(mac[2], HEX);
  Serial.print(":");
  Serial.print(mac[1], HEX);
  Serial.print(":");
  Serial.println(mac[0], HEX);

}

void printCurrentNet() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the MAC address of the router you're attached to:
  byte bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("BSSID: ");
  Serial.print(bssid[5], HEX);
  Serial.print(":");
  Serial.print(bssid[4], HEX);
  Serial.print(":");
  Serial.print(bssid[3], HEX);
  Serial.print(":");
  Serial.print(bssid[2], HEX);
  Serial.print(":");
  Serial.print(bssid[1], HEX);
  Serial.print(":");
  Serial.println(bssid[0], HEX);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);

  // print the encryption type:
  byte encryption = WiFi.encryptionType();
  Serial.print("Encryption Type:");
  Serial.println(encryption, HEX);
  Serial.println();
}

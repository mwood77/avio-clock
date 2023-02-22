#include <Arduino.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>

WiFiManager wm;
HTTPClient http;
WiFiClient client;

int ntp_offset = 0;
boolean wifiConnected;
boolean ntpSynced = false;
String timeURL = "http://worldtimeapi.org/api/ip";

// NTP Client intializers
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", ntp_offset, 86400); // 86400 seconds = 24 hours

/**
 * Calls external time API & updates ESP32's onboard real time clock
 */
void getTimeForInitialization()
{
  http.begin(client, timeURL);
  int httpCode = http.GET();

  if (httpCode > 0)
  {
    DynamicJsonDocument doc(2048);
    deserializeJson(doc, http.getStream());
    const int offset = doc["raw_offset"];
    ntp_offset = offset;

    timeClient.begin();
    timeClient.setTimeOffset(ntp_offset);
    timeClient.update();

    ntpSynced = !ntpSynced;
  }

  http.end();
}

void mdnsInitialize()
{
  if (!MDNS.begin("avio"))
  {
    Serial.println("[STATUS] - Failed to start mDNS");
  }

  MDNS.addService("_avio", "_tcp", 80);
  Serial.println("[STATUS] - mDNS started");
}

/**
 * Callback triggered from WifiManager when successfully connected to new WiFi network
 */
void saveWifiCallback()
{
  Serial.println("[STATUS] - commencing reset");
  // slow blink to confirm connection success
  ESP.restart();
  delay(2000);
}

void setup()
{
  Serial.begin(9600);
  WiFi.mode(WIFI_STA);

  wifiConnected = false;
  ntpSynced = false;

  // onboard LED
  pinMode(2, OUTPUT);

  // WiFiManager config
  wm.setDarkMode(true);
  wm.setConnectTimeout(20);
  wm.setHostname("avio-clock");
  wm.setConfigPortalTimeout(3600);
  wm.setConfigPortalBlocking(false);
  wm.setSaveConfigCallback(saveWifiCallback);

  if (wm.autoConnect("avio-clock setup"))
  {
    // cancel setup blink LED
    wifiConnected = !wifiConnected;
    Serial.println("[STATUS] - connected to saved network");

    getTimeForInitialization();
  }
  else
  {
    Serial.println("[STATUS] - WiFi Config Portal running");
  };
}

void loop()
{

  if (!wifiConnected)
  {
    digitalWrite(2, LOW);
    delay(250);
    digitalWrite(2, HIGH);
    delay(250);
  }
  else
  {
    // turn off LED
    digitalWrite(2, HIGH);
  }

  if (ntpSynced)
  {
    Serial.println(timeClient.getFormattedTime());
  }

  wm.process();
  delay(1000);
}
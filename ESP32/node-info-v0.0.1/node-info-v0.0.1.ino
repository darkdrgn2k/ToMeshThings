#include "WiFi.h"
#include <U8x8lib.h>
#include <iostream>
#include <HTTPClient.h>
#include "ArduinoJson.h"
#include <Time.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// OLED config
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);

// WiFi config
#define STA_SSID "tomesh-node"
#define STA_PASS "password"

// NTP config
// timezone in seconds
#define NTP_OFFSET -18000
// sync NTP every miliseconds
#define NTP_INTERVAL 3600 * 1000
// NTP server
#define NTP_ADDRESS  "10.0.0.1"

// YGG HTTP vars
String httpYHost = "10.0.0.1";
String httpYPath = "/cgi-bin/peers-yggdrasil";

// put values into yurl
String yurl=("http://" + httpYHost + httpYPath);
static volatile bool wifi_connected = false;

// defines
String mac = "";

// NTP function
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

void setup(){
    Serial.begin(115200);
    pinMode(25, OUTPUT);
    WiFi.disconnect(true);
    WiFi.onEvent(WiFiEvent);
    WiFi.mode(WIFI_MODE_STA);
    WiFi.begin(STA_SSID, STA_PASS);
    mac=WiFi.macAddress();
    timeClient.begin();
    u8x8.begin();
    u8x8.setFont(u8x8_font_chroma48medium8_r);
    u8x8.setContrast(0);
    u8x8.clear();
    u8x8.setCursor(0, 0);
    u8x8.print("Starting...");
    u8x8.setCursor(0, 2);
    u8x8.print("MAC:");
    u8x8.setCursor(0, 3);
    u8x8.print(mac);
}

void loop(){
    if(wifi_connected){
        // sync NTP
        timeClient.update();
        // start wifi connected loop
        wifiConnectedLoop();
    }
    while(Serial.available()) Serial.write(Serial.read());
}

void wifiOnConnect(){
    Serial.println("WiFi MAC:");
    Serial.println(mac);
    Serial.println("STA Connected");
    Serial.print("STA IPv4: ");
    Serial.println(WiFi.localIP());
    Serial.print("STA IPv6: ");
    Serial.println(WiFi.localIPv6());
    u8x8.clear();
    u8x8.setCursor(0, 0);
    u8x8.print("WiFi Connected.");
    u8x8.setCursor(0, 1);
    u8x8.print("IP is:");
    u8x8.setCursor(0, 2);
    u8x8.print(WiFi.localIP());
    delay(500);
}

void wifiOnDisconnect(){
    Serial.println("STA Disconnected");
    u8x8.clear();
    u8x8.setCursor(0, 0);
    u8x8.print("WiFi");
    u8x8.setCursor(0, 1);
    u8x8.print("Connection Fail");
    delay(1000);
    WiFi.begin(STA_SSID, STA_PASS);
}

void WiFiEvent(WiFiEvent_t event){
    switch(event) {
        case SYSTEM_EVENT_STA_CONNECTED:
            //enable sta ipv6 here
            WiFi.enableIpV6();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            wifiOnConnect();
            wifi_connected = true;
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            wifi_connected = false;
            wifiOnDisconnect();
            break;
        default:
            break;
    }
}

void wifiConnectedLoop(){
    // turn LED on to indicate loop is running
    digitalWrite(25, HIGH);
    unsigned long epoch = timeClient.getEpochTime();
    Serial.print("");
    // format time to readable format from epoch
    time_t utcCalc = epoch ;
    unsigned long cyear = year(utcCalc );
    unsigned long cmonth = month(utcCalc );
    unsigned long cday = day(utcCalc );
    unsigned long chour = hour(utcCalc );
    unsigned long cminute = minute(utcCalc );
    unsigned long csecond = second(utcCalc );
    Serial.printf("Current Time: %04lu-%02lu-%02lu %02lu:%02lu:%02lu\n", cyear,cmonth,cday,chour,cminute,csecond);
    // get data from node over HTTP
    // object of class HTTPClient
    Serial.println("Starting HTTPClient http");
    HTTPClient http;
    Serial.println("http.begin: " + yurl);
    http.begin(yurl);
    Serial.println("http.GET");
    int httpCode = http.GET();

    //Check the returning code                                                                  
    if (httpCode > 0) {
      // Get the request response payload
      String ydata = http.getString();
      // Parsing
      // output RAW data
      Serial.println("RAW DATA");
      Serial.println(ydata);
      StaticJsonDocument<5000> doc;
      DeserializationError error = deserializeJson(doc, ydata);
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());
        return;
      }


      JsonObject obj=doc["peers"].as<JsonObject>();
      for (JsonObject::iterator it=obj.begin(); it!=obj.end(); ++it) {
        Serial.print("Peer: ");
        Serial.print(it->key().c_str());
        Serial.print(" Endpoint ");
        const char* endpoint = doc["peers"][it->key()]["endpoint"];
        Serial.print(endpoint);
        Serial.print(" Uptime ");
        const int uptime = doc["peers"][it->key()]["uptime"]; // uptime
        Serial.println(uptime);
      }

 /*     const size_t bufferSize = JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(8) + 370;
      DynamicJsonBuffer jsonBuffer(bufferSize);
      JsonObject& root = jsonBuffer.parseObject(ydata);
      // Parameters
      const char* peer = root["peers"]; // peer
      const char* endpoint = root["endpoint"]; // endpoint
      const char* uptime = root["uptime"]; // uptime
      // Output to serial monitor
      Serial.print("Peer:");
      Serial.println(peer);
      Serial.print("Endpoint:");
      Serial.println(endpoint);
      Serial.print("Uptime:"); 
      Serial.println(uptime);
   */   
    }
    //Close connection
    http.end();
    
    // output stuff to screen
    //u8x8.clear();
    u8x8.clearLine(0);
    u8x8.setCursor(0, 0);
    u8x8.print("");
    u8x8.clearLine(1);
    u8x8.setCursor(0, 1);
    u8x8.print("");
    u8x8.clearLine(2);
    u8x8.setCursor(0, 2);
    u8x8.print("");
    u8x8.clearLine(3);
    u8x8.setCursor(0, 3);
    u8x8.print("");
    u8x8.clearLine(4);
    u8x8.setCursor(0, 4);
    u8x8.print("");
    u8x8.clearLine(5);
    u8x8.setCursor(0, 5);
    u8x8.print("");
    u8x8.clearLine(6);
    u8x8.setCursor(0, 6);
    u8x8.print("");
    u8x8.clearLine(7);
    u8x8.setCursor(0, 7);
    u8x8.print("");

    // turn LED off to indicate end of the loop
    digitalWrite(25, LOW);

    // set how long to wait before updating again in ms
    delay(5000);
}

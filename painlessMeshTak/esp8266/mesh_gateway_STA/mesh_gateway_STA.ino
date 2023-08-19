#include <ESP8266WiFi.h>
#include <WiFiUDP.h>
#include "user_interface.h"
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

#define RX_PIN 4 // D2 pin

const char* ssid = "yourRoutersSSID"; // Change this to your desired SSID
const char* password = "yourRoutersAP";  // Change this to your desired password, leave blank for open wifi access point

SoftwareSerial mySerial(RX_PIN); //Software Serial to recieve data from painlessMesh
WiFiUDP udp;
IPAddress multicastIP(239, 2, 3, 1);    // Multicast address
unsigned int multicastPort = 6969;      // Multicast port
long randNumber; //used to generate a unique messageId inside of GeoChat CoT

void setup() {
  Serial.begin(115200); //for serial monitor
  mySerial.begin(19200); //for data to be sent from mesh esp to access point esp
  delay(100);

  // Create WiFi access point...
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  wifi_set_sleep_type(NONE_SLEEP_T);
  
  udp.beginMulticast(WiFi.localIP(), multicastIP, multicastPort);
}

void loop() {
  while (mySerial.available() > 1) {
  String receivedData = mySerial.readStringUntil('\n');
  Serial.println(receivedData);

  // Parse the received JSON payload...
  DynamicJsonDocument jsonDoc(1024); // Choose an appropriate size for your JSON document
  DeserializationError error = deserializeJson(jsonDoc, receivedData);
  
  // Check if parsing was successful...
  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    } else {
      // Successfully parsed JSON, now you can access the data...
      String cotType = jsonDoc["type"]; //this determines what CoT type to broadcast to TAK (geochat message or map icon)
      String cotCallsign = jsonDoc["callsign"]; //this is also used as the CoT uid
      String cotRemarks = jsonDoc["remarks"]; //this is used as the text message received over GeoChat
      float cotLat = jsonDoc["lat"];
      float cotLon = jsonDoc["lon"];

      char latStr[16];
      dtostrf(cotLat, 8, 6, latStr);
      
      char lonStr[16];
      dtostrf(cotLon, 8, 6, lonStr);

      char cotTypeCharArray[64];
      cotType.toCharArray(cotTypeCharArray, sizeof(cotTypeCharArray));

      char cotCallsignCharArray[64];
      cotCallsign.toCharArray(cotCallsignCharArray, sizeof(cotCallsignCharArray));

      char cotRemarksCharArray[512];
      cotRemarks.toCharArray(cotRemarksCharArray, sizeof(cotRemarksCharArray));

      // Print received data...
      //Serial.println(cotTypeCharArray);
      //Serial.println(cotCallsignCharArray);
      //Serial.println(cotRemarksCharArray);
      //Serial.println(latStr);
      //Serial.println(lonStr);

      // Marker CoT (map icon)...
      if (cotType == "user") {
        Serial.println("sending user cot");
        udp.beginPacketMulticast(multicastIP, multicastPort, WiFi.localIP());
        udp.write("<?xml version='1.0' encoding='UTF-8' standalone='yes'?>");
        udp.write("<event version='2.0' uid='");
        udp.write(cotCallsignCharArray);
        udp.write("' type='b-m-p-s-m' how='m-g' time='2023-08-09T12:34:56Z' start='2023-08-09T12:34:56Z' stale='2023-09-08T12:39:56Z'><point lat='");
        udp.write(latStr);
        udp.write("' lon='");
        udp.write(lonStr);
        udp.write("' hae='0.0' ce='9999999.0' le='9999999.0'/><detail><status readiness='true'/><archive/><color argb='-256'/><usericon iconsetpath='COT_MAPPING_SPOTMAP/b-m-p-s-m/-256'/></detail></event>");
        udp.endPacket();
        Serial.println("finished sending user packet");
       }
        
       //Sensor CoT (geochat)...
       else if (cotType == "sensor") {
         randNumber = random(1000000000, 9999999999);
         char ranNumChar[16];
         ltoa(randNumber,ranNumChar,10);
         Serial.println("sending sensor cot");
         udp.beginPacketMulticast(multicastIP, multicastPort, WiFi.localIP());
         udp.write("<?xml version='1.0' encoding='UTF-8' standalone='yes'?>");
         udp.write("<event version='2.0' uid='");
         udp.write(cotCallsignCharArray);
         udp.write("' type='b-t-f' how='h-g-i-g-o' time='2023-08-16T04:13:20Z' start='2023-08-16T04:13:20Z' stale='2023-09-16T04:13:20Z'><point lat='0.0' lon='0.0' hae='0.0' ce='9999999.0' le='9999999.0'/><detail><__chat parent='RootContactGroup' groupOwner='false' messageId='");
         udp.write(ranNumChar);
         udp.write("' chatroom='All Chat Rooms' id='All Chat Rooms'><chatgrp uid0='All Chat Rooms' uid1='All Chat Rooms' id='All Chat Rooms'/></__chat><remarks source='");
         udp.write(cotCallsignCharArray);
         udp.write("' to='All Chat Rooms' time='2023-08-16T04:13:20Z'>");
         udp.write(cotRemarksCharArray);
         udp.write("</remarks></detail></event>");
         udp.endPacket();
         Serial.println("finished sending sensor packet");
        } else {
         Serial.println("couldnt read CoT type or data");
        } 
      }
   }
}

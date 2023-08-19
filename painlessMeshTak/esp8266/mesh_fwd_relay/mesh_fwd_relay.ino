#include "painlessMesh.h"
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

#define   MESH_PREFIX     "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

long previousTime = 0;
long interval = 60000; // 60 seconds
const int output1 = 13; // D7 pin
int output1state = LOW;
unsigned long currentTime = millis();

Scheduler userScheduler;
painlessMesh  mesh;

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  //Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
    Serial.println(msg);
    
 // Parse the received JSON payload...
    DynamicJsonDocument jsonDoc(1024); // Choose an appropriate size for your JSON document
    DeserializationError error = deserializeJson(jsonDoc, msg);

    // Check if parsing was successful...
    if (error) {
      Serial.print("JSON parsing failed: ");
      Serial.println(error.c_str());
      } else {
      // Successfully parsed JSON, now you can access the data...
      String cotType = jsonDoc["type"]; //this determines what CoT type to broadcast to TAK (geochat message or map icon)
      String cotCallsign = jsonDoc["callsign"]; //this is also used as the CoT uid
      //String cotRemarks = jsonDoc["remarks"]; //this is used as the text message received over GeoChat
      //float cotLat = jsonDoc["lat"];
      //float cotLon = jsonDoc["lon"];

      // Print received data...
      Serial.println(cotCallsign);
      if (cotCallsign == "pir-1") { //can choose which sensor callsign actuates the relay
        if (output1state == LOW) {
          output1state = HIGH;
          digitalWrite(output1, output1state);
          //jsonDoc["lat"] = 0;
          //jsonDoc["lon"] = 0;
          jsonDoc["type"] = "sensor"; //sensor,user,other
          jsonDoc["callsign"] = "relay-1";
          jsonDoc["remarks"] = "relay-1 triggered!!!"; //this prints message to GeoChat
      
          String jsonString;
          serializeJson(jsonDoc, jsonString);
          Serial.println(jsonString); //send over serial port
          mesh.sendBroadcast(jsonString); //send over mesh
          } else if (output1state == HIGH) {
            previousTime = currentTime;
          }
      }
   }
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  //Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
    //Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

void setup() {
  Serial.begin(115200);

  pinMode(output1, OUTPUT);
  digitalWrite(output1, output1state);

  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
}

void loop() {
  mesh.update();
  currentTime = millis();
  if (currentTime - previousTime > interval) {
    previousTime = currentTime;
    output1state = LOW;
    digitalWrite(output1, output1state);
  }
}

#include "painlessMesh.h"
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

#define   MESH_PREFIX     "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

long previousTime = 0;
long interval = 10000;
const int input1 = 13; // D7 pin
int input1State;

Scheduler userScheduler;
painlessMesh  mesh;

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  //Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  Serial.println(msg);
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
  pinMode(input1, INPUT);
  input1State = digitalRead(input1);

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
  
  input1State = digitalRead(input1);
  if (input1State == HIGH) {
  unsigned long currentTime = millis();
    if(currentTime - previousTime > interval) {
      previousTime = currentTime;
      DynamicJsonDocument jsonDoc(1024);
      //jsonDoc["lat"] = 0;
      //jsonDoc["lon"] = 0;
      jsonDoc["type"] = "sensor"; //sensor,user,other
      jsonDoc["callsign"] = "pir-1";
      jsonDoc["remarks"] = "pir-1 triggered!!!";
      
      String jsonString;
      serializeJson(jsonDoc, jsonString);
      Serial.println(jsonString); //send over serial port
      mesh.sendBroadcast(jsonString); //send over mesh
    }
  }
}

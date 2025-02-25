#define   M2_DEBUG            1
#include <Arduino.h>
#include <painlessMesh.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <M2Utils.h>

#define   STATION_SSID     "KeriStudio"
#define   STATION_PASSWORD "androidAP"

#define HOSTNAME "M2_Bridge"
#define BROKER_IP 192, 168, 1, 56
#define SUBSCRIBE_TOPIC "toBridge/#"

// todo: remove this in prod
// #define BROKER_IP 192, 168, 1, 11 //change this back to 56
// #define MESH_PREFIX "M2INCD"
// #define MESH_CHANNEL 8

bool readySent = false;

// Prototypes
void meshReceivedCB( const uint32_t &from, const String &msg );
void mqttReceivedCB(char* topic, byte* payload, unsigned int length);

IPAddress getlocalIP();

IPAddress myIP(0,0,0,0);
IPAddress mqttBroker(BROKER_IP);

painlessMesh  mesh;
WiFiClient wifiClient;
PubSubClient mqttClient(mqttBroker, 1883, mqttReceivedCB, wifiClient);

void setup() {
  CHECK_M2UTILS_VER(5);
  // enableDebug();
  Serial.begin(SERIAL_SPEED);

  mesh.setDebugMsgTypes( ERROR | CONNECTION );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, MESH_CHANNEL );
  mesh.setRoot(true);
  mesh.setContainsRoot(true);

  // callbacks
  mesh.onReceive(&meshReceivedCB);
  mesh.onNewConnection(&newConnectionCB);
  mesh.onDroppedConnection(&droppedConnectionCB);
  mesh.onChangedConnections(&status);

  mesh.stationManual(STATION_SSID, STATION_PASSWORD);
  mesh.setHostname(HOSTNAME);

  Serial.printf("Hi, I'm the master @ %u", mesh.getNodeId());
}

void newConnectionCB(uint32_t nodeId) {
  char buf[12];
  sprintf(buf, "%u", nodeId);
  mqttClient.publish("fromBridge/newConn", buf);
}

void droppedConnectionCB(uint32_t nodeId) {
  char buf[12];
  sprintf(buf, "%u", nodeId);
  mqttClient.publish("fromBridge/droppedConn", buf);
}

void reconnect() {
    debugPrintln("Attempting MQTT connection...");
    // Attempt to connect
    if (mqttClient.connect(HOSTNAME)) {
      if(!readySent){
        mqttClient.publish("fromBridge/init", "m2bridge is ready");
        readySent = true;
      }
      mqttClient.subscribe(SUBSCRIBE_TOPIC);
      debugPrintln("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      meshDelay(5000);
    }
}


void loop() {
  mesh.update();
  mqttClient.loop();

  if(myIP != getlocalIP()){
    myIP = getlocalIP();
    debugPrintln("My IP is " + myIP.toString());
    reconnect();
  } else {
    if(!mqttClient.connected()) reconnect();
  }
}

void meshReceivedCB( const uint32_t &from, const String &msg ) {
  // we have the msg in the buffer now
  Serial.printf("bridge: Received from %u msg=%s\n", from, msg.c_str());
  int colon_i = msg.indexOf(':');
  String msg_topic = msg.substring(0, colon_i);
  String topic = "fromNode/" + String(from) + "/" + msg_topic;
  // TODO: harden this publish
  if (mqttClient.publish(
        topic.c_str(),
        msg.substring(colon_i+1).c_str())) {
  } else {
    Serial.println("Failed to send message!" + msg);
    return;
    // failed to publish, leave it up to the client to retry
  }
  // ack the whole thing though
  if (!msg.startsWith("data:")){
    uint32_t fr = from;
    String ms = "ack:" + msg;
    mesh.sendSingle(fr, ms); // echo back the message in ack
  }
}

void status() {
  /*
  Prints status of the mesh to fromBridge/status topic
  */
  // get topology
  String msg = "=====STATUS=====\n";
  msg += "MasterId:";
  msg += String(mesh.getNodeId());
  msg += "\nMeshSize:";
  msg += String(mesh.getNodeList().size());
  msg += "\nTopology:";
  msg += mesh.subConnectionJson();

  // print & publish
  mqttClient.publish("fromBridge/meshSize", String(mesh.getNodeList().size()).c_str());
  Serial.println(msg);
  Serial.println("=====");
}

void mqttReceivedCB(char* topic, uint8_t* payload, unsigned int length) {
  if(!mqttClient.connected()){
    debugPrintln("NO MQTT");
    return;
  }
  char* cleanPayload = (char*)malloc(length+1);
  payload[length] = '\0';
  memcpy(cleanPayload, payload, length+1);
  String msg = String(cleanPayload);
  free(cleanPayload);
  //
  String targetStr = String(topic).substring(String(SUBSCRIBE_TOPIC).length() - 1);
  if(targetStr.equals("status")) {
    status();
    return;
  }
  uint32_t target = strtoul(targetStr.c_str(), NULL, 10);
  if(mesh.isConnected(target))
  {
    mesh.sendSingle(target, msg);
  }
  else
  {
    String msg = "Client not connected! ID:" + String(target);
    mqttClient.publish("fromBridge/error", msg.c_str());
  }
}

void meshDelay(uint32_t mill) {
  if (mill < 500) {
    delay(mill); return;
  }
  for(uint32_t i = 0; i < (int)(mill/500); i++) {
    delay(500);
    mesh.update();
  }
  delay(mill%500);
}

IPAddress getlocalIP() {
  return IPAddress(mesh.getStationIP());
}

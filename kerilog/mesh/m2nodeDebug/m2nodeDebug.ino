#include <painlessMesh.h>
#include <M2Utils.h>
#include <spi_flash.h>
// Prototypes
void sendMessage();
void receivedCallback(uint32_t from, String & msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);
void delayReceivedCallback(uint32_t from, int32_t delay);

painlessMesh  mesh;

SimpleList<uint32_t> nodes;

void setup() {
  Serial.begin(SERIAL_SPEED);

  // mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes(ERROR | MESH_STATUS | CONNECTION);  // set before init() so that you can see startup messages
  // mesh.setDebugMsgTypes(ERROR | DEBUG | CONNECTION);  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_STA, MESH_CHANNEL );
  mesh.setRoot(false); //not the root
  mesh.setContainsRoot(true); // but should have ro
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);

  Serial.println("=============INIT MESH==============");
  Serial.printf("Hi, I'm the node @ %u\n flash_size %u", mesh.getNodeId(), fs_size());
  Serial.println("=============INIT MESH==============");

}

uint32_t fs_size() { // returns the flash chip's size, in BYTES
  uint32_t id = spi_flash_get_id();
  uint8_t mfgr_id = id & 0xff;
  uint8_t type_id = (id >> 8) & 0xff; // not relevant for size calculation
  uint8_t size_id = (id >> 16) & 0xff; // lucky for us, WinBond ID's their chips as a form that lets us calculate the size
  if(mfgr_id != 0xEF) // 0xEF is WinBond; that's all we care about (for now)
    return id;
  return 1 << size_id;
}

void loop() {
  mesh.update();
  while(Serial.available()) {
    String input = Serial.readString();
    input.trim();
    if(input.equals("status")){
      changedConnectionCallback();
    } else if (input.equals("reset")) {
      ESP.restart();
    } else {
      Serial.println("Invalid Command");
      Serial.println(input);
    }
  }
}


void receivedCallback(uint32_t from, String & msg) {
  Serial.printf("--> startHere: Received from %u msg=%s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
  // Reset blink task
  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("--> Changed connections %s\n", mesh.subConnectionJson().c_str());
  nodes = mesh.getNodeList();

  Serial.printf("--> Num nodes: %d\n", nodes.size());
  Serial.printf("--> Connection list:");

  SimpleList<uint32_t>::iterator node = nodes.begin();
  while (node != nodes.end()) {
    Serial.printf(" %u", *node);
    node++;
  }
  Serial.println();
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("--> Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void delayReceivedCallback(uint32_t from, int32_t delay) {
  Serial.printf("--> Delay to node %u is %d us\n", from, delay);
}

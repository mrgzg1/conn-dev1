//************************************************************
//This acts as a buffer between nano that collects data and between the server
//************************************************************
#include "painlessMesh.h"
#include <LinkedList.h>
#include <M2Utils.h>

#define   RETRY_AFTER     (10 * 1000) // ms
#define   MAX_TRIES       16 // theoretical max 256
#define   RESTART_EVERY   (3 * 3600 * 1000) // hours * sec * ms if we are not connected to the master

painlessMesh  mesh;
bool master_set = false;
uint32_t master_id = 0;
// //todo:remove this in PROD
// #define MESH_PREFIX "M2INCD"
// #define MESH_CHANNEL 8

struct message {
  String msg;
  long last_sent;
  uint8_t num_tries;
};
LinkedList<message> unack_msg_list = LinkedList<message>();

void setup() {
  CHECK_M2UTILS_VER(5);
  // enableDebug();

  Serial.begin(SERIAL_SPEED);
  // mesh init
  mesh.setDebugMsgTypes( ERROR );  // set before init() so that you can see startup messages
  // init(String ssid, String password, uint16_t port = 5555, WiFiMode_t connectMode = WIFI_AP_STA, uint8_t channel = 1, uint8_t hidden = 0, uint8_t maxconn = MAX_CONN);
  mesh.init( MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, MESH_CHANNEL, 0, 3 );
  mesh.setRoot(false); //not the root
  mesh.setContainsRoot(true); // but should have ro
  mesh.onReceive(&receivedCallback);

  // let the bridge know that node is ready
  String hello = "status:boot_m2node with heap->" + String(ESP.getFreeHeap()) + "\n";
  sendMessage(hello);

  // just for debug
  Serial.printf("Hi, I'm the node @ %u\n", mesh.getNodeId());
  // manual watchdog
  ESP.wdtDisable();
}

void setMaster(uint32_t from) {
  // check master and set her appropriately
  if(!master_set) { master_id = from; master_set = true;}
}

void receivedCallback( const uint32_t from, const String msg ) {
  if(msg.equals("[reset]")){
    reset();
  }
  else if(msg.startsWith("ack:")){
    setMaster(from);
    // ack the message
    ackMessage(msg.substring(4));
    debugPrint("Received ack, not sending it to slave:");
    debugPrintln(msg.substring(4));
  }
  else if(msg.equals("[status]")) {
    // status and relay the status
    String status = "status:node is happy with heap->"+String(ESP.getFreeHeap())+"\n";
    sendMessage(status);
    Serial.println(msg);
  }
}

void reset() {
  // reset logic
  Serial.println("RESTARTING!!");
  sendMessage("status:restart:node\n");
  ESP.restart();
}

void loop() {
  // feed the watchdog
  ESP.wdtFeed();
  mesh.update();
  retryMessages();
  // check if we need to restart, ie we haven't heard back from master
//  if((millis()) > RESTART_EVERY) {
//    ESP.restart();
//  }
}

/*
This message is to be sent to the server
*/
void sendMessage(String msg) {
  // update message
  debugPrintln("Broadcasting Message:" + msg);
  meshSend(msg);
  addUnackMessage(msg);
}

void resendMessage(String msg) {
  debugPrintln("REBroadcasting Message:" + msg);
  meshSend(msg);
}

void meshSend(String msg) {
  // handles if it should be sent as broadcast or single
  if(!master_set){
    debugPrintln("Sending Broadcast:" + msg );
    mesh.sendBroadcast(msg);
  } else {
    debugPrintln("Sending Single:" + msg );
    if(!mesh.sendSingle(master_id, msg)){
        debugPrintln("Master cannot be reached, unsetting him");
        master_set = false;
    }
  }
}

bool addUnackMessage(String msg) {
  message m;
  m.msg = msg;
  m.last_sent = millis();
  m.num_tries = 0; // this is done after the first time
  return unack_msg_list.add(m);
}

bool ackMessage(String msg) {
  if(unack_msg_list.size() == 0) return true;
  for(int i = 0; i < unack_msg_list.size(); i++) {
    message m = unack_msg_list.get(i);
    if (msg.equals(m.msg)) {
      unack_msg_list.remove(i);
      return true;
    }
  }
  debugPrintln("NOT found to ACK:" + msg);
  return false;
}

void retryMessages() {
  if(unack_msg_list.size() > 0) {
    for(int i = 0; i < unack_msg_list.size(); i++) {
      message m = unack_msg_list.get(i);
      if(m.num_tries == MAX_TRIES) {
        debugPrintln("MAX TRIES REACHED. Removing msg:" + m.msg);
        // max tries reached
        unack_msg_list.remove(i);
        i--; // so that we don't ignore an element
      }
      else if((millis() - m.last_sent) > RETRY_AFTER) {
        resendMessage(m.msg);
        m.last_sent = millis();
        m.num_tries++;
        unack_msg_list.set(i, m);
      }
      mesh.update();
    }
  }
}

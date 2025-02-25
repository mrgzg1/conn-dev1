#ifndef MESH_H
#define MESH_H

#include "painlessMesh.h"
#include <M2Utils.h>
#include <LinkedList.h>


painlessMesh  mesh;
bool master_set = false;
uint32_t master_id = 0;

long last_fs_try = 0;
long last_ack_received = 0;

// //todo:remove this in PROD
// #define MESH_PREFIX "M2INCD"
// #define MESH_CHANNEL 8

struct message {
  String msg;
  long last_sent;
  uint8_t num_tries;
};
LinkedList<message> unack_msg_list = LinkedList<message>();

void initMesh() {
  // mesh init
  mesh.setDebugMsgTypes( 0 );  // set before init() so that you can see startup messages
  // init(String ssid, String password, uint16_t port = 5555, WiFiMode_t connectMode = WIFI_AP_STA, uint8_t channel = 1, uint8_t hidden = 0, uint8_t maxconn = MAX_CONN);
  mesh.init( MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, MESH_CHANNEL, 0, 3 );
  mesh.setRoot(false); //not the root
  mesh.setContainsRoot(true); // but should have ro

}

long lastBroadcast = 0;
#define BROADCAST_EVERY 1000 // no more than this

void meshSend(String msg) {
  // handles if it should be sent as broadcast or single
  if(!master_set){
    debugPrintln("Sending Broadcast:" + msg );
    if((millis() - lastBroadcast) >  BROADCAST_EVERY) {
      mesh.sendBroadcast(msg);
      lastBroadcast = millis(); // set the time for when we sent the last broadcast
    } else {
      debugPrint("Ignoring broadcast:");
      debugPrintln(msg);
    }
  } else {
    debugPrintln("Sending Single:" + msg );
    if(!mesh.sendSingle(master_id, msg)){
        debugPrintln("Master cannot be reached, unsetting him");
        master_set = false;
    }
  }
}

bool isMaster(uint32_t id) {
  if(master_set && id == master_id) return true;
  return false;
}

void setMaster(uint32_t from) {
  // check master and set her appropriately
  if(!master_set) { master_id = from; master_set = true;}
}

void unsetMaster() {
  master_set = false;
}



void resendMessage(String msg) {
  debugPrint("RETRY MSG::");
  meshSend(msg);
}

bool addUnackMessage(String msg) {
  message m;
  m.msg = msg;
  m.last_sent = millis();
  m.num_tries = 0; // this is done after the first time
  return unack_msg_list.add(m);
}

bool ackMessage(String msg) {
  last_ack_received = millis();
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
/*
This message is to be sent to the server
*/
void sendMessage(String msg) {
  // update message
  debugPrint("SEND MSG::");
  meshSend(msg);
  addUnackMessage(msg);
}



#endif

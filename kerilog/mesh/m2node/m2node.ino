//************************************************************
//This acts as a buffer between nano that collects data and between the server
//************************************************************
#include "mesh.h"
#include <M2Utils.h>
#include "FlashSys.h"

#define   RETRY_AFTER     (10 * 1000) // ms
#define   RETRY_FS_AFTER  (7 * 1000) //ms
#define   FS_RETRY_NUM    16 // how many data points to retry, try to push this
#define   MAX_TRIES       16 // theoretical max 256
#define   NANO_RST_PIN    4
#define   NANO_RST_DELAY  200
#define   ACK_RESTART_TIMEOUT   (15 * 60 * 1000) // min * sec * ms if we are not connected to the master
#define   RESTART_EVERY   (3*3600*1000)  // regardless of the sit, restart the device every 3 hours
#define   TIME_TIMEOUT    5*1000 // 10s
#define   NANO_HEARTBEAT  60*1000 // 60s
#define   DEFRAG_TIMEOUT  12*3600*1000 // 12 hours
volatile long startTime = 0;
volatile long lastBeat = 0;
volatile long lastDefrag = 0;

void setup() {
  CHECK_M2UTILS_VER(5);
  // enableDebug();

  // get the pin ready for Slave Arduino Reset
  pinMode(NANO_RST_PIN, OUTPUT);
  digitalWrite(NANO_RST_PIN, HIGH);
  Serial.begin(SERIAL_SPEED);
  resetNano();
  delay(200); // let nano settle in

  // get time
  Serial.println("[time]");
  //now we wait for sometime to get RTC
  long cmdStartTime = millis();
  while(1) {
    if((millis() - cmdStartTime) > TIME_TIMEOUT || startTime != 0) break;
    while(Serial.available()) {
      String t = Serial.readString();
      debugPrint("GOT:");
      debugPrintln(t);
      if(t.startsWith("time:")) {
        processTimeMsg(t);
      }
    }
    delay(10);
  }

  //setup file SYSTEM
  setupFS();
  // print the configs
  printConfigs();

  initMesh();
  mesh.onReceive(&receivedCallback);


  // let the bridge know that node is ready
  String hello = "status:esp_boot with heap->" + String(ESP.getFreeHeap()) + \
                  " t->"+String(getTime())+"\n";
  sendMessage(hello);

  // just for debug
  Serial.printf("Hi, I'm the node @ %u\n", mesh.getNodeId());
  // manual watchdog
  ESP.wdtDisable();

  // if we failed to get time, send out that message
  if(startTime == 0) {
    sendMessage("error: failed to get time");
  }
}

void processHeartbeat(String msg) {
  debugPrint("Got Heartbeat:");
  debugPrintln(msg);
  lastBeat = millis();
  if(startTime == 0) {
    Serial.println("[time]");
  }
}

void processTimeMsg(String t) {
  debugPrint("Parsing:");
  debugPrintln(String(((t.substring(5)).toInt())));
  startTime = ((t.substring(5)).toInt()) + (millis()/1000);
  debugPrintln("Starttime:" + String(startTime));
}

void receivedCallback( const uint32_t from, const String msg ) {
  // if starts with ack:<msg>
  if(msg.startsWith("ack:")){
    setMaster(from);
    // ack the message
    ackMessage(msg.substring(4));
    debugPrint("Received ack, not sending it to slave:");
    debugPrintln(msg.substring(4));
  }
  // special ack for data points
  else if (msg.startsWith("ackd:")) {
    last_ack_received = millis(); // we know we are connected to the master
    setMaster(from);
    debugPrint("Received ackD, not sending it to slave:");
    debugPrintln(msg.substring(5));
    int startpos = String("ackd:").length();
    String d = msg.substring(startpos);
    ackDataline(d);
  }
  else if(msg.equals("[reset]")){
    reset();
  }
  else if(msg.equals("[defrag]")){
    defragFile();
  }
  else if(msg.equals("[clearconfig]")) {
    if(clearConfig()) {
        sendMessage("info:config cleared");
    } else {
        sendMessage("error:failed to clear config");
    }
  }
  else if(msg.equals("[status]")) {
    // status and relay the status
    String status = "status:heap->"+String(ESP.getFreeHeap())+\
                    " uptime_s->"+String(millis()/1000)+\
                    " time->"+String(getTime())+"\n";
    sendMessage(status);
    Serial.println(msg);
  }
  else if(msg.equals("[printfile]")) {
    printFile();
  }
  else if(msg.equals("[printconfig]")) {
    printConfigs();
  }
  else if(msg.equals("[fsinfo]")) {
    // status and relay the status
    printFSInfo();
  }
  else if(msg.startsWith("[enable:")) {
    debugPrintln("Got enable cmd->" + msg);
    // status and relay the status
    enableSensor(msg);
  }
  else if(msg.equals("[formatfs]")) {
    // status and relay the status
    formatFS();
  }
  else if(msg.startsWith("[") || isMaster(from)){
    debugPrint("Recived command. Sending this msg to slave:");
    Serial.println(msg);
  } else {
    // ignore
    debugPrint("Rejected Command:");
    debugPrintln(msg);
  }
}

long getTime() {
  long curTime = (millis()/1000) + startTime;
  debugPrintln("Current time:" + String(curTime) + "Starttime:" + String(startTime));
  return curTime;
}

void enableSensor(String cmd) {
  // first strip it down to bares
  cmd = cmd.substring(String("[enable:").length(), cmd.length()-1);
  // now store this in the cache
  if(!writeConfigLine(cmd)) {
    debugPrintln("Config rejected");
  }
}

void resetNano() {
  digitalWrite(NANO_RST_PIN, LOW);
  delay(NANO_RST_DELAY);
  digitalWrite(NANO_RST_PIN, HIGH);
}

void reset() {
  // reset logic
  Serial.println("RESTARTING!!");
  sendMessage("status:restart:node\n");
  resetNano(); // reset nano first
  ESP.restart();
}

void loop() {
  // feed the watchdog
  ESP.wdtFeed();
  mesh.update();
  String msg = "";
  while(Serial.available()) {
    // send everything over to the mesh
    String lmsg = Serial.readString();
    for(int i = 0; i < lmsg.length(); i++) {
      if(lmsg.charAt(i) == '\n'){
        procSerialMsg(msg);
        msg = "";
      } else {
        msg += lmsg[i];
      }
      mesh.update();
    }
    if (msg.length() > 0) {
      procSerialMsg(msg);
      msg = "";
    }
    mesh.update();
  }
  retryMessages();
  // check if we need to restart, ie we haven't heard back from master
  long now = millis();
  if((now - last_ack_received) > ACK_RESTART_TIMEOUT) {
    Serial.println("------RESETTING MYSELF!------");
    reset();
  }
  if((now - lastBeat) > NANO_HEARTBEAT) {
    Serial.println("------RESETTING NANO!------");
    sendMessage("error:nano heart failure");
    lastBeat = now + NANO_HEARTBEAT; // give it some time to come back
    resetNano();
  }
  if((now - lastDefrag) > DEFRAG_TIMEOUT) {
    sendMessage("info:defrag");
    lastDefrag = now;
    defragFile();
  }
}

void procSerialMsg(String msg) {
  msg.trim(); // trim the crap
  if (msg.startsWith("@")) {
    sendDataMessage(msg.substring(1));
  } else {
    if(msg.startsWith("init:ready")) {
      printConfigs(); // print old config
      String s_msg = msg + ";" + String(getTime());
      meshSend(s_msg); // don't cache it
    } else if(msg.startsWith("time:")) {
      processTimeMsg(msg);
    } else if(msg.startsWith("<hb>")){
      processHeartbeat(msg);
    } else {
      // just relay it to the master
      sendMessage(msg);
    }
  }
}

void sendDataMessage(String msg) {
  resendDataMessage(msg);
  addFSUnackMessage(msg);
}

void resendDataMessage(String msg) {
  meshSend("data:" + msg);
}


bool addFSUnackMessage(String msg) {
  return writeDataline(msg);
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
  retryFSMessages(); // retry file system messages
}

void retryFSMessages() {
  /*
    Retry un-ack'd messages
    tried every
  */
  if ((millis()-last_fs_try) > RETRY_FS_AFTER) {
    last_fs_try = millis();
    String unacklines;
    if(!master_set) {
      unacklines = readUnacklines(1);
    } else {
      unacklines = readUnacklines(FS_RETRY_NUM);
    }
    if (unacklines == "") return; // if nada return
    String singleMsg = "";
    for (int i = 0; i < unacklines.length(); i++) {
      if(unacklines.charAt(i) == BOUNDARY_MARKER) {
        resendDataMessage(singleMsg);
        singleMsg = "";
      } else {
        singleMsg += unacklines[i];
      }
      mesh.update();
    }
    if(singleMsg.length() > 0) {
      resendDataMessage(singleMsg);
      singleMsg = "";
    }
  }
}

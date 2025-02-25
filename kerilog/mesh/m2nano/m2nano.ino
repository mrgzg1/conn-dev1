#include "sensors.h"
#include "rtc.h"
#include <M2Utils.h>
#define M2DEBUG 1

#define CHECK_READY_EVERY 10000 // every 10s

Proxies proxies = Proxies();
// PTs pts = PTs();
Battery bat = Battery();
DynamicCommandParser dcp('[', ']', ':');
bool ready = false;
long last_ready_sent = 0;

/*
List of valid commands:
[enable:<sensor_type>:<sensor_number>:<interval>]
[disable:<sensor_type>:<sensor_number>]
[status]
*/

void setup() {
  CHECK_M2UTILS_VER(5);
  // enableDebug();
  Serial.begin(SERIAL_SPEED);
  dcp.addParser("enable", enable_cmd, 2);
  dcp.addParser("disable", disable_cmd, 2);
  dcp.addParser("config", config_cmd, 0);
  dcp.addParser("settime", time_cmd, 6);
  dcp.addParser("status", status_cmd, 0);
  dcp.addParser("time", get_time_cmd, 0);
  Serial.println("init:ready");
}

int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void get_time_cmd(char **values, int valueCount) {
  Serial.println("time:" + String(getTime()));
}


void status_cmd(char **values, int valueCount) {
  /*
    Prints the status of the device
  */
  Serial.println("status:ram->" + String(freeRam())+" uptime_s->" + String(millis()/1000));
  printTime();
  // pts.printStatus();
  proxies.printStatus();
  bat.printStatus();
}

void config_cmd(char **values, int valueCount) {
  // we are getting config
  ready = true;
}

void time_cmd(char **values, int valueCount) {
  int year = hardenedAtoI(values[1]);
  int month = hardenedAtoI(values[2]);
  int day = hardenedAtoI(values[3]);
  int hour = hardenedAtoI(values[4]);
  int min = hardenedAtoI(values[5]);
  int sec = hardenedAtoI(values[6]);
  if( year == MAGIC_VALUE ||
      month == MAGIC_VALUE ||
      day == MAGIC_VALUE ||
      hour == MAGIC_VALUE ||
      min == MAGIC_VALUE ||
      sec == MAGIC_VALUE) {
        Serial.println("error: invalid time cmd");
        return;
      }
  Serial.println("Setting time!");
  setRTCTime(year, month, day, hour, min, sec);
}
void enable_cmd(char **values, int valueCount) {
  Sensors *s = getAppropriateSensorClass(values[1]);
  if(s == NULL) return;
  debugPrintln("ENABLE: " + String(values[1]) + "|" + String(values[2]));
  s->enable(values[2]);
}

void disable_cmd(char **values, int valueCount) {
  debugPrint("DISABLE:");
  Sensors *s = getAppropriateSensorClass(values[1]);
  if(s == NULL) return;
  int index = hardenedAtoI(values[2]);
  debugPrintln(String(index));
  s->disable(index);
}

Sensors *getAppropriateSensorClass(char *type) {
  if(strcmp("proxy", type) == 0){
    return &proxies;
  // } else if (strcmp("pt", type) == 0) {
  //   return &pts;
  } else if (strcmp("bat", type) == 0) {
    return &bat;
  } else {
    Serial.print("error:invalid sensor type <");
    Serial.print(type);
    Serial.println(">");
    return NULL;
  }
}

void ensureReady() {
  // we check if the config has been sent to us, else ping head again
  if(!ready) {
    if((millis() - last_ready_sent) > CHECK_READY_EVERY) {
      Serial.println("init:ready");
      last_ready_sent = millis();
    }
  }
}

long lastBeat = 0;
#define BEAT_EVERY 30*1000 // 30s
void loop() {
  while(Serial.available()) { dcp.appendChar(Serial.read());  }
  // process each sensors
  proxies.process();
  // pts.process();
  bat.process();
  ensureReady();
  if((millis() - lastBeat) > BEAT_EVERY) {
    Serial.println("<hb>");
    lastBeat = millis();
  }
  // delay(100);
}

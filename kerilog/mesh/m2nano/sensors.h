#ifndef SENSOR_H
#define SENSOR_H

#define MAX_SENSORS 8
#define MIN_INTERVAL 10 //10 sec
#define PROCESS_WARNING_TIME 1000
#include "rtc.h"
#include <M2Utils.h>


//abstract class Sensor
struct sensor_info {
  unsigned int id;
  uint8_t fail_count; // used for exp backoff
};

class Sensors {
  public:
    Sensors() {
      for(int i = 0 ; i < MAX_SENSORS-1; i++) {
        sensor_list[i] = NULL;
      }
      type_interval = 0;
    };

    void disableAll() {
      for(int i = 0 ; i < MAX_SENSORS-1; i++) {
        disable(i);
      }
    };

    void enable(String enableStr) {
      /*
      Format <1>;<2>;<3>;
      1-> a,b,c
      a-> hw_id
      b-> interval
      c-> sensor id
      */
      String tmp = "";
      debugPrintln("PROCESSING");
      debugPrintln(enableStr);
      for (int i = 0; i < enableStr.length(); i++) {
        char c = enableStr[i];
        if(c == ';'){
          processEnableStr(tmp);
          tmp=""; //clean up at the end
        } else {
          tmp+= String(c);
        }
      }
      if(tmp.length() > 0) {
        processEnableStr(tmp);
      }
    };

    void processEnableStr(String tmp) {
      debugPrintln("GOT");
      debugPrintln(tmp);

      int hw_id, interval, id;
      char tmpArr[3];
      int a = tmp.indexOf(',');
      int b = tmp.lastIndexOf(',');

      tmp.substring(0,a).toCharArray(tmpArr, 3);
      hw_id = hardenedAtoI(tmpArr);
      tmpArr[0] = '\0';

      tmp.substring(a+1,b).toCharArray(tmpArr, 3);
      interval = hardenedAtoI(tmpArr);
      tmpArr[0] = '\0';

      tmp.substring(b+1, tmp.length()).toCharArray(tmpArr, 3);
      id = hardenedAtoI(tmpArr);

      debugPrintln(String(hw_id) + "::" + String(interval) + "::" + String(id));
      if(hw_id == MAGIC_VALUE || interval == MAGIC_VALUE || id == MAGIC_VALUE) {
        Serial.println("error:enabling->" +tmp);
      } else {
        enable_individual(hw_id, interval, id);
      }
    };

    void enable_individual(int index, int interval, int id) {
      if(isEnabled(index)) return; // return if we are already enabled
      if(interval < MIN_INTERVAL) {
        Serial.println("error:enabling " + String(id) + ". too short read interval");
        return;
      }
      if (index < 0 || index > MAX_SENSORS-1 || !this->enable_s(index, interval)) {
        Serial.println("error:enabling " + String(id));
      } else {
        sensor_info *sensor = malloc(sizeof(sensor_info));
        // set the global interval to be the shortest of them all
        if(type_interval == 0) type_interval = (int) interval;
        else if(type_interval > (int) interval) type_interval = (int) interval;
        sensor->fail_count = 0;
        sensor->id = id;
        sensor_list[index] = sensor;
        Serial.println("ack:enable:" + getSensorId(index) + ":" + String(interval));
      }
    };

    void disable(int index) {
      if(index < 0 || index > MAX_SENSORS-1 || !this->disable_s(index)){
        Serial.println("error:disable" + getSensorId(index));
      } else {
        if(index > MAX_SENSORS-1 || sensor_list[index]==NULL) { return false;}
        Serial.println("ack:disable:" + getSensorId(index));
        free(sensor_list[index]);
        sensor_list[index] = NULL;
      }
    };

    bool isEnabled(int index) {
      if(sensor_list[index] != NULL) {
        return true;
      }
      return false;
    }

    String getSensorId(int index) {
      return String(sensor_list[index]->id);
    }

    void process() {
      long start_time = millis();
      if((start_time - last_sent) > (type_interval*1000)) {
          // compiled all enabled sensors data
          String data_str = "";
          for(int i = 0; i < MAX_SENSORS; i++) {
            sensor_info *sensor = sensor_list[i];
            if(sensor == NULL) { continue; }
            data_str += String(getSensorId(i)) + "," + read(i) + ";";
          }
          // @v2:timestamp:id1,data1;id2,data2;
          if(data_str.length() > 0)
            Serial.println("@v2:" + String(getTime()) + ":" + data_str);
          last_sent = start_time;
      }
      int proc_time = millis() - start_time;
      if(proc_time > PROCESS_WARNING_TIME) {
        Serial.println("warning:process took too long " + String(proc_time));
      }
    }
    void printStatus() {
      Serial.print("status:" + getSensorType() + ":");
      for(int i = 0; i < MAX_SENSORS; i++) {
        if(isEnabled(i)) {
          Serial.print(String(i)+","+String(type_interval)+","+getSensorId(i));
          if (i < MAX_SENSORS-1) {
            Serial.print(";");
          }
        }

      }
      Serial.println();

    }
    // TO BE IMPLEMENTED IN SUBCLASS
    // RETURN: unique sensor type string
    virtual String getSensorType() = 0;
    // Print sensor value to the line else print error
    virtual String read(int index) = 0;
    // enable routine for the given sensor
    virtual bool enable_s(int index, int interval) = 0;
    // disable routine for the given sensor
    virtual bool disable_s(int index) = 0;
  private:
    sensor_info *sensor_list[MAX_SENSORS];
    uint32_t type_interval;
    unsigned long last_sent;
};

#define MAX_PROXIES 6
#define EDGE RISING
#include "PinChangeInterrupt.h"
// static counters
uint32_t counts[MAX_PROXIES];
void count0() {counts[0] += 1;};
void count1() {counts[1] += 1;};
void count2() {counts[2] += 1;};
void count3() {counts[3] += 1;};
void count4() {counts[4] += 1;};
void count5() {counts[5] += 1;};


class Proxies : public Sensors {
  public:
    String getSensorType() {
      return "proxy";
    }
    String read(int index) {
      String ret = String(counts[index]);
      counts[index] = 0; // reset it
      return ret;
    };
    bool enable_s(int index, int interval) {
      if (index > MAX_PROXIES-1 || index < 0) {
        return false;
      }
     pinMode(pins[index], INPUT_PULLUP);
      switch(index) {
        case 0:
          attachInterrupt(digitalPinToInterrupt(pins[index]), count0, EDGE);
          break;
        case 1:
          attachInterrupt(digitalPinToInterrupt(pins[index]), count1, EDGE);
          break;
        case 2:
          attachPCINT(digitalPinToPCINT(pins[index]), count2, EDGE);
          break;
        case 3:
          attachPCINT(digitalPinToPCINT(pins[index]), count3, EDGE);
          break;
        case 4:
          attachPCINT(digitalPinToPCINT(pins[index]), count4, EDGE);
          break;
        case 5:
          attachPCINT(digitalPinToPCINT(pins[index]), count5, EDGE);
          break;
      }
      counts[index] = 0; // init counts
      return true;
    };
    bool disable_s(int index) {
      if (index > MAX_PROXIES-1 || index < 0) {
        return false;
      }
      if(index < 2) {
        detachInterrupt(digitalPinToInterrupt(pins[index]));
      } else {
        detachPCINT(digitalPinToPCINT(pins[index]));
      }
      counts[index] = 0;
      return true;
    };
  private:
    int pins[MAX_PROXIES] = {PD2, PD3, PD4, PD5, PD6, PD7};
};

#define MAX_PTS 6
#define READ_SAMPLES 1000
#define PT_M 0.0165
#define PT_N 0
class PTs : public Sensors {
  public:
    String getSensorType() {
      return "pt";
    }
    String read(int index) {
      float adc_value = 0;
      float adc_volt = 0;
      for(int p=0;p<READ_SAMPLES;p++){
        adc_value = adc_value + analogRead(pins[index]);
      }
      adc_volt = (((adc_value/1000) * 5.07)/1023);
      return String((adc_volt - PT_N)/PT_M);
    };
    bool enable_s(int index, int interval) {
      if(index < 0 || index > MAX_PTS-1) return false;
      return true;
    };
    bool disable_s(int index) {
      if(index < 0 || index > MAX_PTS-1) return false;
      return true;
    };
  private:
    int pins[MAX_PTS] = {A0, A1, A2, A3, A6, A7};
};

#define BAT_PIN 12
class Battery : public Sensors {
  public:
    String getSensorType() {
      return "bat";
    }
    String read(int index) {
      if (digitalRead(BAT_PIN))
        return "1";
      else
        return "0";
    };
    bool enable_s(int index, int interval) {
      if(index != 0) return false;
      pinMode(BAT_PIN, INPUT);
      return true;
    };
    bool disable_s(int index) {
      if(index != 0) return false;
      return true;
    };
};

#endif

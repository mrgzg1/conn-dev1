#ifndef M2RTC_H
#define M2RTC_H
/* RTC LOGIC*/
#include "RTClib.h"

bool isRTCSetup = false;
bool rtcFailed = false;

RTC_DS3231 rtc;

void setupRTC() {
  if(!isRTCSetup) {
     if(! rtc.begin()) {
       Serial.println("error:RTC failed to init");
       rtcFailed = true;
       isRTCSetup = false;
       return;
     }
     isRTCSetup = true;
     rtcFailed = false;
  }
}

long getTime() {
  if(rtcFailed) return -1;
  setupRTC();
  return rtc.now().unixtime();
}

bool setRTCTime(int year, int month, int day, int hour, int min, int sec) {
  if(rtcFailed) return false;
  setupRTC();
  rtc.adjust(DateTime(year, month, day, hour, min, sec));
}

void printTime() {
  setupRTC();
  DateTime now = rtc.now();
  Serial.print("time:");
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print("-");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
}

#endif

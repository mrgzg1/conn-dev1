// Very basic Spiffs example, writing 10 strings to SPIFFS filesystem, and then read them back
// For SPIFFS doc see : https://github.com/esp8266/Arduino/blob/master/doc/filesystem.md
// Compiled in Arduino 1.6.7. Runs OK on Wemos D1 ESP8266 board.

#include "FS.h"
#include "FlashSys.h"

void setup() {
  Serial.begin(74880);
  Serial.println("HELLO! FILE SYSTEM TEST");
  Serial.println("BEGINNING");
  setupFS();
  // Next lines have to be done ONLY ONCE!!!!!When SPIFFS is formatted ONCE you can comment these lines out!!
}

void loop() {
  while (Serial.available()){
    String input = Serial.readString();
    input.trim();
    Serial.print("INPUT:");
    Serial.println(input);
    // write line to the file
    if (input.startsWith("write:")) {
      int startpos = String("write:").length();
      input = input.substring(startpos);
      writeDataline(input);
    // print info
    } else if (input.equals("info")) {
      printFSInfo();
    // read a line
  } else if (input.equals("defrag")) {
      defragFile();
    // read a line
    }else if (input.equals("readline")){
      Serial.println("===========");
      Serial.println(readUnacklines(3));
      Serial.println("===========");
    // ack the line
    } else if(input.startsWith("ackline:")){
      int startpos = String("ackline:").length();
      input = input.substring(startpos);
      Serial.println(ackDataline(input));
    // print the whole file
    } else if(input.equals("printfile")) {
        printFile();
    } else if (input.equals("format")) {
      formatFS();
    } else {
      Serial.println("INVALID");
    }
  }
}

#ifndef FLASH_SYS
#define FLASH_SYS

#include "FS.h"
#include "mesh.h"

#include <M2Utils.h>
#define FILENAME "/data.txt"
#define ACK_MARKER '$' // the line has been ack'd
#define BOUNDARY_MARKER '@'
#define FILLER '^'
#define ERROR_MAGIC "$@$"
#define NEWLINE true
#define LINE_SIZE 32

volatile uint32_t pos;

int getLineCount(File f);
String readLine(int lineNum, File f);
String readRawLine(int lineNum, File f);
String trimLine(String line);
bool defragFile();
uint32_t minUnackLines = 0;

void setupFS() {
  String realSize = String(ESP.getFlashChipRealSize());
  String ideSize = String(ESP.getFlashChipSize());
  if (!realSize.equals(ideSize)) {
    sendMessage("warn:warning wrong flash settings. Real->"+ realSize + " ide->"+ ideSize);
  }
  SPIFFS.begin();

  // check if we need to defrag
  FSInfo fsi;
  SPIFFS.info(fsi);
  // defrag anyways
  defragFile();

  sendMessage("info:flash begun");
}

void formatFS() {
  if(SPIFFS.format()) {
    sendMessage("info:format success!");
  } else {
    sendMessage("error: format fail!");
  }
}

void printFSInfo() {
  String realSize = String(ESP.getFlashChipRealSize());
  String ideSize = String(ESP.getFlashChipSize());
  sendMessage("info:RealSize="+realSize+" | IDE Size=" + ideSize+ "");
  size_t fileSize = 0;
  if(SPIFFS.exists(FILENAME)){
    File f = SPIFFS.open(FILENAME, "r");
    fileSize = f.size();
  } else {
    sendMessage("error:datafile doesn't exist");
  }

  FSInfo fsi;
  SPIFFS.info(fsi);
  String info = "info:file=" + String(fileSize) + \
                "|tot=" + String(fsi.totalBytes) + \
                "|used=" + String(fsi.usedBytes);

  sendMessage(info);
}

String prePostFixLine(String str) {
  str.trim();
  // TODO: make sure it doesn't contain any of the markers
  if (!str.startsWith(String(BOUNDARY_MARKER))) {
    // add start marker
    str = BOUNDARY_MARKER + str;
  }
  if (str.length() > 31) {
    // we have the start marker by now
    return ERROR_MAGIC;
  }
  // now pad the end
  for(int i = str.length(); i < 31; i++) {
    str += FILLER;
  }
  if (NEWLINE) str+="\n";
  else str+= FILLER;
  return str;
}

bool writeDataline(String str) {
  // prep the data to write
  str = prePostFixLine(str);
  if(str.equals(ERROR_MAGIC)) return false;

  // in case we are writing first time
  File f = (File)NULL;
  if(!SPIFFS.exists(FILENAME)){
    // first time writing
    sendMessage("warn:first time writing");
    f = SPIFFS.open(FILENAME, "w");
  } else {
    f = SPIFFS.open(FILENAME, "a");
  }
  if (!f) {
      sendMessage("error:file open failed for appending/writing");
      return false;
  }
  // seek from the beginning
  f.print(str);
  f.flush();
  f.close();
  minUnackLines++; // increment this count
  return true;
}

String readUnacklines(int max) {
  // reads the latest line from the end of the file
  // in case we are writing first time
  if(!SPIFFS.exists(FILENAME)){
    return "";
  }
  File f = SPIFFS.open(FILENAME, "r");
  if (!f) {
      Serial.println("file open failed for reading");
      return "";
  }
  //go through each line from the bottom and go up
  int lineCount = getLineCount(f);
  int found=0;
  String ret = "";
  for(int i = lineCount; i > 0; i--) {
    // bottom to top
    String line = readRawLine(i, f);
    if(line.startsWith(String(BOUNDARY_MARKER))){
      found++;
      ret += trimLine(line);
      ret += String(BOUNDARY_MARKER);
    }
    if(found >= max) break;
  }
  f.close();
  return ret;
}

String readRawLine(int lineNum, File f) {
  if (lineNum * 32 > f.size()) {
    return ERROR_MAGIC;
  }
  f.seek((lineNum - 1) * 32);
  String line = "";
  for(int i = 0; i < LINE_SIZE; i++) {
    line += (char)f.read();
  }
  return line;
}

String trimLine(String line) {
  // also massage it clean
  int lineEnd = line.indexOf(FILLER); // first appearance of filler
  if(lineEnd == -1) {
    lineEnd = line.indexOf('\n');
    if(lineEnd == -1) {
      lineEnd = line.length();
    }
  }
  // we have the index for filler, so read till then
  line = line.substring(1, lineEnd);
  return line;
}

String readLine(int lineNum, File f) {
  return trimLine(readRawLine(lineNum, f));
}

int getLineCount(File f){
  return (int) (f.size() / 32);
}

bool ackDataline(String targetLine) {
  // marks the line as done and moves the seek position if at the EoF
  File f = (File)NULL;
  if(!SPIFFS.exists(FILENAME)){
    return false;
  } else {
    f = SPIFFS.open(FILENAME, "r+");
  }

  int lines = getLineCount(f); // num lines

  for(int i = lines; i >0; i--) {
    String line = readRawLine(i, f);
    if (line.equals(ERROR_MAGIC) || line.startsWith(String(ACK_MARKER))) {
      continue;
    }
    line = trimLine(line);
    // we have the index for filler, so read till then
    if(line.equals(targetLine)) {
      // ack the line
      f.seek((i-1)*32, SeekSet);
      f.write(ACK_MARKER);
      f.flush();
      f.close();
      minUnackLines--;
      return true;
    }
  }
  f.close();
  return false;
}

bool defragFile() {
  /* moves all the unacked msgs to the beginning of the file and sets the seek position to it
  Creates a new file and then fills it with only unack data,
  deletes the old file & renames the new one back to old file name
  */
  File oldFile = SPIFFS.open(FILENAME, "r");
  if (!oldFile) {
      sendMessage("error: defrag file open failed");
      return false;
  }
  String newFilePath = "/tmp.txt";
  File newFile = SPIFFS.open(newFilePath, "w");
  if(!newFile) {
    sendMessage("error:file open failed for defrag");
    oldFile.close();
    return false;
  }
  for(int i = 1; i <= getLineCount(oldFile); i++) {
    String line = readRawLine(i, oldFile);
    if(!line.startsWith(String(ACK_MARKER))) {
      // write the line to new file
      newFile.print(line);
    }
  }
  // remove the old file
  oldFile.close();
  SPIFFS.remove(FILENAME);
  SPIFFS.rename(newFilePath, FILENAME);
  sendMessage("info:defrag success!");
  return true;
}

void printFile() {
  String contents="";
  if(SPIFFS.exists(FILENAME)) {
    File file = SPIFFS.open(FILENAME, "r");
    if (!file) {
        sendMessage("error:file open failed for printing");
        return;
    }
    while (file.available()) {
      contents+=char(file.read());
    }
    file.close();
  } else {
    sendMessage("warn:no file for printing");
  }
  Serial.print(contents);
}

//----------- CONFIG FILE
#define CONFIG_FILE "/config.txt"
bool writeConfigLine(String cmd) {
  // check if file exists
  File f = (File)NULL;
  if(!SPIFFS.exists(CONFIG_FILE)){
    // first time writing
    f = SPIFFS.open(CONFIG_FILE, "w");
    f.println(cmd);
    f.flush();
    f.close();
    // also print it out
    Serial.println("[enable:"+cmd+"]");
    return true;
  }
  f = SPIFFS.open(CONFIG_FILE, "r");
  // first read the whole file and see if we find a matching line
  String newFile = "";
  String contents = "";
  while (f.available()) {
    char c = char(f.read());
    if(c == '\n') {
      // check if the config is same as what are looking at
      if (contents.equals(cmd)) {
          // same config, so just exit. nothing to change
          f.close();
          return false;
      } else {
        String newType = cmd.substring(0,cmd.indexOf(":"));
        String fileType = contents.substring(0, contents.indexOf(":"));
        if(!newType.equals(fileType)) {
          // append the old config to new file
          newFile += (contents + "\n");
        }
      }
      contents = "";
    } else {
      contents+=c;
    }
  }
  newFile += (cmd + "\n");
  // if we are here that means we need to print the config & re-write the file
  f.close();
  SPIFFS.remove(CONFIG_FILE);
  f = SPIFFS.open(CONFIG_FILE, "w"); f.print(newFile); f.flush(); f.close();
  Serial.println("[enable:"+cmd+"]");
}

bool clearConfig() {
  return SPIFFS.remove(CONFIG_FILE);
}

bool printConfigs() {
  // prints all the configs in the file
  // just print it all
  String contents="";
  if(SPIFFS.exists(CONFIG_FILE)) {
    File file = SPIFFS.open(CONFIG_FILE, "r");
    if (!file) {
        sendMessage("warn:file open failed for config printing");
        return false;
    }
    while (file.available()) {
      char c = char(file.read());
      if(c == '\n') {
        // prep the command and send it out
        String cmd = "[enable:" + contents + "]";
        Serial.println(cmd);
        //delay(100);
        contents = "";
      } else {
        contents+=c;
      }
    }
    file.close();
    return true;
  } else {
    sendMessage("error:no file for config printing");
    return false;
  }
}

#endif

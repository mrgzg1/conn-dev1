#include "M2Utils.h"

void DynamicCommandParser::addParser(char *cmd, ParserFunction function, uint8_t sub_commands)
{
  mParserLookupSize++;
  mParserLookup = (ParserFunctionLookup*)realloc(mParserLookup, (mParserLookupSize) * sizeof(ParserFunctionLookup));
  mParserLookup[mParserLookupSize-1].command = cmd;
  mParserLookup[mParserLookupSize-1].function = function;
  mParserLookup[mParserLookupSize-1].sub_commands = sub_commands;
}

void DynamicCommandParser::append(char *str)
{
  for(size_t i = 0; i < strlen(str); i++)
  {
    appendChar(str[i]);
  }
}

void DynamicCommandParser::appendChar(char c)
{
  size_t bufferLength = strlen(buffer);

  if(c == mStart)
  {
    // print rouge command if any
    if(rougeCommand) {
      rougeCommand = false;
      Serial.print("rouge:");
      Serial.println(buffer);
    }
    mInCommand = true;
    buffer[0] = 0;
    return;
  }
  else if(c == mEnd)
  {
    parseBuffer();
    buffer[0] = '\0';
    mInCommand = false;
  }
  else if(mInCommand)
  {
    buffer[bufferLength] = c;
    buffer[bufferLength+1] = '\0';
  }
  // else {
  //   rougeCommand = true;
  //   if(c == '\n' || c == '\r' && bufferLength > 0) {
  //     if(bufferLength == 1 && (buffer[0] == '\n' || buffer[0] == '\r')) return;
  //     Serial.print("rouge:");
  //     Serial.println(buffer);
  //     buffer[0] = '\0';
  //     return;
  //   }
  //   buffer[bufferLength] = c;
  //   buffer[bufferLength+1] = '\0';
  // }
}

void DynamicCommandParser::parseBuffer()
{
  // Split buffer
  int partCount = getBufferPartCount();
  char **parts = (char**)malloc(partCount * sizeof(char*));

  parts[0] = buffer;
  int currentPart = 0;

  for(int i = 0; buffer[i] != 0; i++)
  {
    if(buffer[i] == mDelimiter)
    {
      buffer[i] = 0;
      currentPart++;
      parts[currentPart] = &buffer[i+1];
    }
  }

  for(size_t i = 0; i < mParserLookupSize; i++)
  {
    if(strcmp(mParserLookup[i].command, parts[0]) == 0)
    {
      if(mParserLookup[i].sub_commands != partCount-1) {
        Serial.println("error:invalid_num_args. Expected <" +
                String(mParserLookup[i].sub_commands) + "> Got <" +
                String(partCount -1) + "> for cmd" +
                String(mParserLookup[i].command));
        break;
      }
      mParserLookup[i].function(parts, partCount);
      break;
    }
  }

  free(parts);
}

int DynamicCommandParser::getBufferPartCount()
{
  int count = 1;
  for(size_t i = 0; i < strlen(buffer); i++)
  {
    if(buffer[i] == mDelimiter)
      count++;
  }
  return count;
}

long hardenedAtoI(char *value) {
  long val = atoi(value);
  char itoa_num[8];
  if(strcmp(value, itoa(val, itoa_num, 10)) == 0) {
    return val;
  } else {
    Serial.println("error:parsing string to int");
    return MAGIC_VALUE;
  }
}

void enableDebug() {
  m2_debug = true;
}
void disableDebug() {
  m2_debug = false;
}

void debugPrintln(String str) {
  #ifdef M2DEBUG
  if(m2_debug)
  Serial.println(str);
  #endif
}

void debugPrint(String str) {
  #ifdef M2DEBUG
  if(m2_debug)
  Serial.print(str);
  #endif
}

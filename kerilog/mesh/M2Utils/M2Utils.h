// https://github.com/mdjarv/DynamicCommandParser
#ifndef M2UTILS_H
#define M2UTILS_H

#define   M2UTILS_VER     5
#define   MESH_PREFIX     "M2INC"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555
#define   SERIAL_SPEED    74880
#define   MESH_CHANNEL    11

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <Arduino.h>

#define BUFFER_SIZE 64

typedef void (* ParserFunction)(char **values, int valueCount);

typedef struct
{
  char *command;
  ParserFunction function;
  uint8_t sub_commands;
} ParserFunctionLookup;

class DynamicCommandParser
{
public:
  DynamicCommandParser(char start, char end, char delim)
  {
    mInCommand = false;
    rougeCommand = false;
    mStart = start;
    mEnd = end;
    mDelimiter = delim;

    mParserLookup = NULL;
    mParserLookupSize = 0;
    buffer[0] = '\0';
  }

  ~DynamicCommandParser()
  {
    free(mParserLookup);
  }

  void addParser(char *cmd, ParserFunction function, uint8_t sub_commands);
  void append(char *str);
  void appendChar(char c);

private:
  bool mInCommand;
  bool rougeCommand;
  char buffer[BUFFER_SIZE];
  char mStart;
  char mEnd;
  char mDelimiter;

  size_t mParserLookupSize;
  ParserFunctionLookup *mParserLookup;

  void parseBuffer();
  int getBufferPartCount();
};

#define MAGIC_VALUE -234890
static bool m2_debug = false;
long hardenedAtoI(char *value);
void debugPrintln(String str);
void debugPrint(String str);
void enableDebug();
void disableDebug();
#define CHECK_M2UTILS_VER(VER) static_assert(VER == M2UTILS_VER, "invalid version of m2 being used");

#endif

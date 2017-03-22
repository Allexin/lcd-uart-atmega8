#ifndef CUSTOM_UTF8_H
#define CUSTOM_UTF8_H

#include <Arduino.h>
#include <U8x8lib.h>

struct sFontGlyphs{
  const uint8_t* data;
  int startCode;
  int endCode;
  uint8_t zeroBytes;
};

struct sMapRange{
  int startCode;
  int endCode;
  int mapTo;
};

void setRange(uint8_t id, const uint8_t* data, int startCode, int endCode, int charWidth);
void setMap(uint8_t id, int startCode, int endCode, int mapTo);
uint8_t drawTextLine(u8x8_t* u8x8, int line, const uint8_t* text, int scroll, uint8_t strLength, bool x2);


#endif


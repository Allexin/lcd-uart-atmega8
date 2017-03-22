#include "custom_utf8.h"
#include <U8x8lib.h>

const uint8_t RANGES_COUNT = 3;
const uint8_t MAPS_COUNT = 2;
sFontGlyphs ranges[RANGES_COUNT];
sMapRange mapRanges[MAPS_COUNT];

void setMap(uint8_t id, int startCode, int endCode, int mapTo)
{
  mapRanges[id].startCode = startCode;
  mapRanges[id].endCode = endCode;
  mapRanges[id].mapTo = mapTo;
}

int mapCode(int code){
  for (uint8_t i = 0; i<MAPS_COUNT; i++){
    if (code>=mapRanges[i].startCode && code<=mapRanges[i].endCode){
      code = code-mapRanges[i].startCode+mapRanges[i].mapTo;
    }
  }
  return code;
}

void setRange(uint8_t id, const uint8_t* data, int startCode, int endCode, int charWidth){
  ranges[id].data = data;
  ranges[id].startCode = startCode;
  ranges[id].endCode = endCode;
  ranges[id].zeroBytes = 8-charWidth;
};

void reset_glyph(uint8_t *bufOut){
  memset(&bufOut[1],0xff,6);
}

bool read_glyph(uint8_t *bufOut, int code){
  uint8_t range = -1;
  for (uint8_t i = 0; i<RANGES_COUNT; i++)
    if (ranges[i].startCode<=code && ranges[i].endCode>=code){
      range = i;
      break;
    }
  if (range==-1){
    reset_glyph(bufOut);
    return false;
  }

  int offset = (code-ranges[range].startCode)*(8-ranges[range].zeroBytes);
  for (uint8_t i = 0; i<ranges[range].zeroBytes; i++)
    bufOut[i] = 0;
  for (uint8_t i = ranges[range].zeroBytes; i<8; i++){
    bufOut[i] = pgm_read_byte(ranges[range].data+offset+i-ranges[range].zeroBytes);
  }
  return true;  
}

int getNextChar(const uint8_t* text, uint8_t strLen, uint8_t& index){
  if (index==strLen)
    return -1;

  int code = text[index];
  index++;
  if (code<=0x7f)//ASCII
    return (code);
  if (code>=0xc0 && code<=0xdf && index<strLen){//UTF-8 2 bytes
    uint8_t c2 = text[index];
    index++;
    c2 = c2 & 0x3f; //remove first 2 bits
    code = code & 0x1f; //remove 3 first bits
    code <<=6; //shift first 5 bits
    code = code | c2; //add second 6 bits
    return (code);
  }
  /* //UNCOMMENT THIS TO ADD 3-bytes chars
  if (code>=0xe0 && code<=0xef && index<strLen-1){//UTF-8 3 bytes
    int c2 = text[index];
    index++;
    uint8_t c3 = text[index];
    index++;

    c3 = c3 & 0x3f; //remove first 2 bits
    c2 = c2 & 0x3f; //remove first 2 bits
    c2 <<=6; //shift XXXX CCCCCC XXXXXX
    code = code & 0xf; //remove 4 first bits
    code <<=12; //shift CCCC XXXXXX XXXXXX
    code = code | c2 | c3; //compose
    return (code);
  }
*/
  //skip unsupported sequense
  while (code>=0xbf && index<strLen)
    index++;  
  return 0;
}

bool getBit(uint8_t b, uint8_t bit_index){
  return ((b >> bit_index) & 1) != 0;
}

uint8_t getByte(bool b0,bool b1,bool b2,bool b3,bool b4,bool b5,bool b6,bool b7){
  uint8_t b = 0;
  if (b0)
    b |= 1 << 0;
  if (b1)
    b |= 1 << 1;
  if (b2)
    b |= 1 << 2;
  if (b3)
    b |= 1 << 3;
  if (b4)
    b |= 1 << 4;
  if (b5)
    b |= 1 << 5;
  if (b6)
    b |= 1 << 6;
  if (b7)
    b |= 1 << 7;
  return b;
  //return b0?128:0 + b1?64:0 + b2?32:0 + b3?16:0 + b4?8:0 + b5?4:0 + b6?2:0 + b7?1:0;
}

void upscale_buf(uint8_t *src, uint8_t *dest, uint8_t bitShift)
{
  for (uint8_t i = 0;i <4; i++){
    bool b0 = getBit(src[i],bitShift);
    bool b1 = getBit(src[i],bitShift+1);
    bool b2 = getBit(src[i],bitShift+2);
    bool b3 = getBit(src[i],bitShift+3);
    uint8_t b = getByte(b0,b0, b1,b1, b2,b2, b3,b3);
    dest[i*2+0] = b;
    dest[i*2+1] = b;
  }
}

void u8x8_Draw2x2Tile(u8x8_t *u8x8, uint8_t x, uint8_t y, uint8_t* buf)
{
  uint8_t s_buf[8];
  upscale_buf(buf, s_buf, 0);
  u8x8_DrawTile(u8x8, x*2, y, 1, s_buf);

  upscale_buf(&buf[4], s_buf, 0);
  u8x8_DrawTile(u8x8, x*2+1, y, 1, s_buf);

  upscale_buf(buf, s_buf, 4);
  u8x8_DrawTile(u8x8, x*2, y+1, 1, s_buf);

  upscale_buf(&buf[4], s_buf, 4);
  u8x8_DrawTile(u8x8, x*2+1, y+1, 1, s_buf);
}

uint8_t drawTextLine(u8x8_t* u8x8, int line, const uint8_t* text, int scroll, uint8_t strLength, bool x2)
{
  uint8_t index = 0;  
  int cols = u8x8_GetCols(u8x8);

  uint8_t buf[8];

  uint8_t realLength = 0;
  int code = 0;
  while (scroll<cols){
    code = mapCode(getNextChar(text, strLength, index));
    if (scroll>=0){
      if (code>0)
        read_glyph(buf,code);
      else
        memset(buf,0,8);
      if (x2){
        if (scroll*2>-1 && scroll*2<cols)
          u8x8_Draw2x2Tile(u8x8,scroll,line, buf);
      }
      else  
        u8x8_DrawTile(u8x8,scroll,line, 1, buf);
    }
    scroll++;
    if (code>=0)
      realLength++;
  }

  while (code>=0){
    code = getNextChar(text, strLength, index);
    realLength++;
  }

  return realLength;
}


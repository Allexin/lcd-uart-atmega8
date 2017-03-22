/*
 * LCD-UART SHIELD - simple LCD screen with control over uart(9600,1 stop, 8 data, no parity)
 * available two types of strings: scrolling text and static text
 * scrolling text line limited by MAX_SCROLL_LINE_LENGTH(140 by default)
 * static text line limited by MAX_STATIC_LINE_LENGTH(32 by default)
 * text must be in UTF-8 encoding
 * 
 * AVAILABLE COMMANDS:
 * CLEAR SCREEN: 0x10
 * SET SCROLL SPEED: 0x11 <step time in millis / 10>
 * DRAW STATIC LINE: 0x12 <line 0..7> <string> 0x00
 * DRAW SCROLL 1 LINE: 0x13 <line 0..7> <string> 0x00
 * DRAW SCROLL 2 LINE: 0x14 <line 0..7> <string> 0x00
 */


#include <Arduino.h>
#include <U8x8lib.h>
#include "cyr5x8.h"
#include "custom_utf8.h"

u8x8_t u8x8;

const uint8_t MAX_SCROLL_LINE_LENGTH = 140;
const uint8_t MAX_STATIC_LINE_LENGTH = 32;

struct sScrollLine{
  uint8_t data[MAX_SCROLL_LINE_LENGTH];
  uint8_t len;
  uint8_t line;
  int8_t scroll;
};

sScrollLine scrollLines[2];

enum elineFromUARTState{
  NONE,
  READ_SCROLL_LINE,
  READ_STATIC_LINE,
  READ_STATIC_LINEx2
};

uint8_t dataFromUART[MAX_STATIC_LINE_LENGTH];
uint8_t lenFromUART;
uint8_t lineFromUARTState = NONE;
uint8_t scrollLine = 0xff;

enum eUARTState{
  WAIT_START,
  WAIT_LINE,  
  WAIT_SIZE,
  WAIT_END,
  WAIT_SCROLL_SPEED
};
uint8_t uartState = WAIT_START;


const uint8_t START_CLEAR_SCREEN = 0x10;
const uint8_t START_SCROLL_SPEED = 0x11;
const uint8_t START_SET_LINE     = 0x12;
const uint8_t START_SET_SCROLL1  = 0x13;
const uint8_t START_SET_SCROLL2  = 0x14;

uint8_t LOOP_TIME = 25;


void addCharToStaticLine(uint8_t c){
  dataFromUART[lenFromUART] = c;
  lenFromUART++;
}

void processByte(uint8_t data){
    switch(uartState){
      case WAIT_START:{
        switch (data){
          case START_SCROLL_SPEED:
            uartState = WAIT_SCROLL_SPEED;
          break;  
          case START_CLEAR_SCREEN:
            u8x8_ClearDisplay(&u8x8);
            scrollLines[0].len = 0;
            scrollLines[1].len = 0;
          break;
          case START_SET_SCROLL1:
          case START_SET_SCROLL2:
          case START_SET_LINE:
            scrollLine = data - START_SET_SCROLL1;
            uartState = WAIT_LINE;        
            lineFromUARTState = data==START_SET_LINE?READ_STATIC_LINE:READ_SCROLL_LINE;
          case 'P':{
            if (lenFromUART==0)
              addCharToStaticLine(data);
          }break;
          case 'A':{
            if (lenFromUART==1)
              addCharToStaticLine(data);
          }break;
          case 'N':{
            if (lenFromUART==2)
              addCharToStaticLine(data);
          }break;
          case 'I':{
            if (lenFromUART==3)
              addCharToStaticLine(data);
          }break;
          case 'C':{
            if (lenFromUART==4)
              addCharToStaticLine(data);
          }break;
          case ':':{
            if (lenFromUART==5){
              lineFromUARTState = READ_SCROLL_LINE;
              scrollLine = 0;
              scrollLines[scrollLine].len = 0;
              scrollLines[scrollLine].line = 0;
              uartState = WAIT_END;
            }
          }break;
          
              
        }
      };
      break;
      case WAIT_SCROLL_SPEED:{
        LOOP_TIME = data;
        uartState = WAIT_START;
      };
      break;
      case WAIT_LINE:{
        if (lineFromUARTState==READ_STATIC_LINE){          
          lenFromUART = 0;
          scrollLine = data;
          uartState = WAIT_SIZE;
        }
        else{          
          scrollLines[scrollLine].len = 0;
          scrollLines[scrollLine].line = data;
          uartState = WAIT_END;
        }
      };
      break;
      case WAIT_SIZE:{
        if (data!=0)
          lineFromUARTState = READ_STATIC_LINEx2;
        uartState = WAIT_END;  
      };
      break;
      case WAIT_END:{
        if (lineFromUARTState==READ_STATIC_LINE || lineFromUARTState==READ_STATIC_LINEx2){
          if (data==0x00){
            if (lenFromUART>0)
              drawTextLine(&u8x8, scrollLine, dataFromUART, 0, lenFromUART,lineFromUARTState==READ_STATIC_LINEx2);
            lenFromUART = 0;  
            uartState = WAIT_START;
          }
          else{
            if (lenFromUART<MAX_STATIC_LINE_LENGTH && data>=0x20){
              addCharToStaticLine(data);
            }
          }
        }
        else{
          if (data==0x00){
            lineFromUARTState = NONE;
            uartState = WAIT_START;
          }
          else{
            if (scrollLines[scrollLine].len<MAX_SCROLL_LINE_LENGTH && data>=0x20){
              scrollLines[scrollLine].data[scrollLines[scrollLine].len] = data;
              scrollLines[scrollLine].len++;
            }
          }
        }
      };
      break;
    }
}
void setup(void)
{
  //u8x8.initDisplay();
  //u8x8.setPowerSave(0);
  u8x8_Setup(&u8x8, u8x8_d_ssd1306_128x64_noname, u8x8_cad_ssd13xx_i2c, u8x8_byte_arduino_hw_i2c, u8x8_gpio_and_delay_arduino);
  u8x8_SetPin_HW_I2C(&u8x8, U8X8_PIN_NONE, U8X8_PIN_NONE, U8X8_PIN_NONE);
  u8x8_InitDisplay(&u8x8);
  u8x8_ClearDisplay(&u8x8);
  u8x8_SetPowerSave(&u8x8, 0);

  setRange(0,seq_32_96,32,96,5);
  setMap(0,97,122,65);
  setRange(1,seq_123_126,123,126,5);
  setRange(2,seq_1040_1071,1040,1071,5);  
  setMap(1,1072,1103,1040);

  scrollLines[0].len = 0;
  scrollLines[1].len = 0;

  Serial.begin(19200);
}

unsigned long lastUpdate = 0;
short int loopTime = 0;
void loop(void)
{
  while (Serial.available()){
    int b = Serial.read();
    if (b>-1)
      processByte(b);
  }
    
  unsigned long t = millis();
  int dt = t- lastUpdate;
  lastUpdate = t;
  loopTime += dt;

  if (loopTime>=LOOP_TIME*10){ 
    loopTime = 0;
    for (uint8_t i = 0; i<2; i++){
      if (scrollLines[i].len>0){        
        int strLen = drawTextLine(&u8x8, scrollLines[i].line, scrollLines[i].data, scrollLines[i].scroll, scrollLines[i].len,false);
        scrollLines[i].scroll--;
        if (scrollLines[i].scroll<-strLen)
          scrollLines[i].scroll=u8x8_GetCols(&u8x8)-1;
      }
    }
  }  
}

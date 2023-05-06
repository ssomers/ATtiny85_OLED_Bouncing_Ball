
#include "SSD1306_minimal.h"
#include "TinyWireM.h"

/*
    display 128x64
    ball size 4x4
   
    128 / 4 => 32 cols
     64 / 4 => 16 rows
    
*/

unsigned char const ColCount = 32;
unsigned char const RowCount = 16;

SSD1306_Mini oled(0x3c);

// there are different wall types
unsigned char const wall[5][4] = { 
  0x0, 0x0, 0x0, 0x0,
  0xf, 0xf, 0xf, 0xf,
  0xf, 0x9, 0x9, 0xf,
  0x9, 0x9, 0x9, 0x9,
  0x9, 0x6, 0x6, 0x9,
};

// the ball shape
unsigned char const ball[4] = { 0x6, 0x9, 0x9, 0x6 };

static unsigned char snakeRow = 10;
static unsigned char snakeCol = 7;  
static char snakeRowDir = +1;
static char snakeColDir = -1;

// this is the room shape
const static uint8_t room[] PROGMEM = {
  1,1,1,1,1,1,1,1 , 1,1,1,1,1,1,1,1 , 1,1,1,1,1,1,1,1 , 1,1,1,1,1,1,1,1,
  1,0,0,0,0,0,0,0 , 0,0,0,0,0,0,0,0 , 0,0,0,0,0,0,0,0 , 0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0 , 0,0,0,0,0,0,0,0 , 0,0,0,0,0,0,0,0 , 0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0 , 0,0,0,0,0,0,0,0 , 0,0,0,0,0,0,0,0 , 0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0 , 0,0,0,0,0,0,0,0 , 0,0,2,0,0,0,0,0 , 0,0,0,0,0,0,0,1,
  1,0,0,0,2,0,0,0 , 0,0,0,0,0,0,0,0 , 0,0,2,0,0,0,0,2 , 2,2,2,0,0,0,0,1,
  1,0,0,0,2,0,0,0 , 0,0,2,2,2,0,0,0 , 0,0,2,0,0,0,0,0 , 0,0,0,0,0,0,0,1,
  1,0,0,0,2,0,0,0 , 0,0,0,0,2,0,0,0 , 0,0,2,0,0,0,0,0 , 0,0,0,0,0,0,0,1,
  1,0,0,0,2,0,0,0 , 0,0,0,0,2,2,2,0 , 0,0,0,0,0,0,0,0 , 0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0 , 0,0,0,0,0,0,0,0 , 0,0,0,0,0,0,0,0 , 0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0 , 0,0,0,0,0,0,0,0 , 0,0,0,0,0,0,0,0 , 0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0 , 0,2,2,2,2,0,0,0 , 0,2,0,0,0,0,0,2 , 2,2,2,2,0,0,0,1,
  1,0,0,0,0,0,0,0 , 0,0,0,0,0,0,0,0 , 0,2,0,0,0,0,0,0 , 0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0 , 0,0,0,0,0,0,0,0 , 0,0,0,0,0,0,0,0 , 0,0,0,0,0,0,0,1,
  1,0,0,0,0,0,0,0 , 0,0,0,0,0,0,0,0 , 0,0,0,0,0,0,0,0 , 0,0,0,0,0,0,0,1,
  1,1,1,1,1,1,1,1 , 1,1,1,1,1,1,1,1 , 1,1,1,1,1,1,1,1 , 1,1,1,1,1,1,1,1,
};


unsigned char getRoom(unsigned char row, unsigned char col) {
  return pgm_read_byte(&room[row*ColCount + col]);
}

bool getSnake(unsigned char row, unsigned char col) {
  return row == snakeRow && col == snakeCol;
}

  
void displayRoom() {
  oled.startScreen();

  for (char r = 0; r < RowCount; r += 2) {
    for (char c = 0; c < ColCount; c += 1){
      uint8_t upperRow= getRoom(r, c);
      uint8_t lowerRow= getRoom(r+1, c);
     
      bool upperSnake= getSnake(r, c);
      bool lowerSnake= getSnake(r+1, c);

      uint8_t data[4] = { 0, 0, 0, 0 };
      
      // room
      if (upperRow){
        data[0]|= wall[upperRow][0] << 0;
        data[1]|= wall[upperRow][1] << 0;
        data[2]|= wall[upperRow][2] << 0;
        data[3]|= wall[upperRow][3] << 0;
      }
      if (lowerRow){
        data[0]|= wall[lowerRow][0] << 4;
        data[1]|= wall[lowerRow][1] << 4;
        data[2]|= wall[lowerRow][2] << 4;
        data[3]|= wall[lowerRow][3] << 4;
      }
      
      // snake
      if (upperSnake){
        data[0]|= ball[0] << 0;
        data[1]|= ball[1] << 0;
        data[2]|= ball[2] << 0;
        data[3]|= ball[3] << 0;
      }
      if (lowerSnake){
        data[0]|= ball[0] << 4;
        data[1]|= ball[1] << 4;
        data[2]|= ball[2] << 4;
        data[3]|= ball[3] << 4;
      }
      
      oled.sendData(data);
    }
  }  
}

void move(){
  char bHitRoom;
  char hitType;
  
  do {  
    bHitRoom= 0;
    // test row
    hitType = getRoom(snakeRow + snakeRowDir, snakeCol);
    if (hitType != 0){
      snakeRowDir = -snakeRowDir;
      bHitRoom = hitType;
    }
    // test col
    hitType = getRoom(snakeRow, snakeCol + snakeColDir);
    if (hitType != 0){
      snakeColDir = -snakeColDir;
      bHitRoom = hitType;
    }
  } while (bHitRoom);
  
  snakeRow = snakeRow + snakeRowDir;
  snakeCol = snakeCol + snakeColDir;
}


void setup() {
  oled.init();
  oled.clear();
}
 
void loop() {
  displayRoom();
  move();
  delay(60); 
}

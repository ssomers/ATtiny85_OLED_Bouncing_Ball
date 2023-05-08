
#include "SSD1306_minimal.h"
#include "TinyWireM.h"

/*
    display 128x64
    ball size 4x4

    128 / 4 => 32 cols
     64 / 4 => 16 rows

*/

uint8_t const ColCount = 32;
uint8_t const RowCount = 16;

typedef SSD1306_Mini<0x3C> OLED;

// there are different wall types
uint8_t const wall[5][4] = {
  0x0, 0x0, 0x0, 0x0,
  0xf, 0xf, 0xf, 0xf,
  0xf, 0x9, 0x9, 0xf,
  0x9, 0x9, 0x9, 0x9,
  0x9, 0x6, 0x6, 0x9,
};

// the ball shape
uint8_t const ball[4] = { 0x6, 0x9, 0x9, 0x6 };

static uint8_t snakeRow = 10;
static uint8_t snakeCol = 7;
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


uint8_t getRoom(uint8_t row, uint8_t col) {
  return pgm_read_byte(&room[row*ColCount + col]);
}

bool getSnake(uint8_t row, uint8_t col) {
  return row == snakeRow && col == snakeCol;
}


void displayRoom() {
  OLED::startScreenUpload();
  uint8_t buf[2 + 4] = { OLED::prefix_to_send(), OLED::prefix_to_send_data() };

  for (char r = 0; r < RowCount; r += 2) {
    for (char c = 0; c < ColCount; c += 1){
      uint8_t upperRow = getRoom(r, c);
      uint8_t lowerRow = getRoom(r+1, c);

      bool upperSnake = getSnake(r, c);
      bool lowerSnake = getSnake(r+1, c);

      buf[2+0] = 0;
      buf[2+1] = 0;
      buf[2+2] = 0;
      buf[2+3] = 0;

      // room
      if (upperRow) {
        buf[2+0] |= wall[upperRow][0] << 0;
        buf[2+1] |= wall[upperRow][1] << 0;
        buf[2+2] |= wall[upperRow][2] << 0;
        buf[2+3] |= wall[upperRow][3] << 0;
      }
      if (lowerRow) {
        buf[2+0] |= wall[lowerRow][0] << 4;
        buf[2+1] |= wall[lowerRow][1] << 4;
        buf[2+2] |= wall[lowerRow][2] << 4;
        buf[2+3] |= wall[lowerRow][3] << 4;
      }

      // snake
      if (upperSnake) {
        buf[2+0] |= ball[0] << 0;
        buf[2+1] |= ball[1] << 0;
        buf[2+2] |= ball[2] << 0;
        buf[2+3] |= ball[3] << 0;
      }
      if (lowerSnake) {
        buf[2+0] |= ball[0] << 4;
        buf[2+1] |= ball[1] << 4;
        buf[2+2] |= ball[2] << 4;
        buf[2+3] |= ball[3] << 4;
      }

      OLED::send(buf, sizeof buf);
    }
  }
}

void move(){
  uint8_t rowHitType;
  uint8_t colHitType;
  do {
    rowHitType = getRoom(snakeRow + snakeRowDir, snakeCol);
    colHitType = getRoom(snakeRow, snakeCol + snakeColDir);
    if (rowHitType) {
      snakeRowDir = -snakeRowDir;
    }
    if (colHitType) {
      snakeColDir = -snakeColDir;
    }
  } while (rowHitType || colHitType);

  snakeRow = snakeRow + snakeRowDir;
  snakeCol = snakeCol + snakeColDir;
}


void setup() {
  OLED::init();
  OLED::clear();
}

void loop() {
  displayRoom();
  move();
  delay(50);
}

#include <inttypes.h>
#include "SBC-OLED01.h"

/*
    display 128x64
    block size 4x4

    128 / 4 => 32 cols
     64 / 4 => 16 rows

*/

uint8_t const ColCount = 32;
uint8_t const RowCount = 16;

typedef SBS_OLED01<0x3C> OLED;

// the ball shape
static uint8_t const ball[4] = { 0x6, 0x9, 0x9, 0x6 };

static uint8_t ballRow = 10;
static uint8_t ballCol = 7;
static int8_t ballRowDir = +1;
static int8_t ballColDir = -1;

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


static uint8_t getRoom(uint8_t row, uint8_t col) {
  return pgm_read_byte(&room[row*ColCount + col]);
}

static bool isBall(uint8_t row, uint8_t col) {
  return row == ballRow && col == ballCol;
}

static void reportError(USI_TWI_ErrorLevel el) {
  if (el) {
    digitalWrite(LED_BUILTIN, LOW);
    for (int f = 0; f < el; ++f) {
      delay(150);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(300);
      digitalWrite(LED_BUILTIN, LOW);
    }
    delay(300);
  }
}

static void displayRoom() {
  uint8_t buf[2 + 4] = { OLED::prefix_to_send(), OLED::prefix_to_send_data() };
  for (int8_t r = 0; r < RowCount; r += 2) {
    for (int8_t c = 0; c < ColCount; c += 1) {
      uint8_t upperRoom = getRoom(r, c);
      uint8_t lowerRoom = getRoom(r+1, c);
      bool upperBall = isBall(r, c);
      bool lowerBall = isBall(r+1, c);

      buf[2+0] = 0;
      buf[2+1] = 0;
      buf[2+2] = 0;
      buf[2+3] = 0;

      // room
      if (upperRoom) {
        buf[2+0] |= 0xF << 0;
        buf[2+1] |= 0x8 << 0;
        buf[2+2] |= 0x4 << 0;
        buf[2+3] |= 0x2 << 0;
      }
      if (lowerRoom) {
        buf[2+0] |= 0xF << 4;
        buf[2+1] |= 0x1 << 4;
        buf[2+2] |= 0x4 << 4;
        buf[2+3] |= 0x1 << 4;
      }

      // ball
      if (upperBall) {
        buf[2+0] |= ball[0] << 0;
        buf[2+1] |= ball[1] << 0;
        buf[2+2] |= ball[2] << 0;
        buf[2+3] |= ball[3] << 0;
      }
      if (lowerBall) {
        buf[2+0] |= ball[0] << 4;
        buf[2+1] |= ball[1] << 4;
        buf[2+2] |= ball[2] << 4;
        buf[2+3] |= ball[3] << 4;
      }

      reportError(OLED::send(buf, sizeof buf));
    }
  }
}

static void move() {
  uint8_t rowHitType;
  uint8_t colHitType;
  do {
    rowHitType = getRoom(ballRow + ballRowDir, ballCol);
    colHitType = getRoom(ballRow, ballCol + ballColDir);
    if (rowHitType) {
      ballRowDir = -ballRowDir;
    }
    if (colHitType) {
      ballColDir = -ballColDir;
    }
  } while (rowHitType || colHitType);

  ballRow = ballRow + ballRowDir;
  ballCol = ballCol + ballColDir;
}


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  reportError(OLED::init());
  reportError(OLED::clear());
  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  displayRoom();
  move();
  delay(50);
}

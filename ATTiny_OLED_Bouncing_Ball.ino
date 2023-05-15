#include <inttypes.h>
#include "SBC-OLED01.h"

typedef SBS_OLED01<0x3C> OLED;

uint8_t constexpr ColCount = OLED::WIDTH / 4; // 4x4 pixels per element
uint8_t constexpr RowCount = OLED::HEIGHT / 4; // 4x4 pixels per element

// the ball shape
static uint8_t constexpr ball[4] = { 0x6, 0x9, 0x9, 0x6 };

static uint8_t ballRow = 10;
static uint8_t ballCol = 7;
static int8_t ballRowDir = +1;
static int8_t ballColDir = -1;

// this is the room shape
const static constexpr uint8_t room[] PROGMEM = {
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


static uint8_t getWall(uint8_t row, uint8_t col) {
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
  constexpr uint8_t COLUMNS_PER_BLOCK = 32;
  uint8_t buf[2 + 4 * COLUMNS_PER_BLOCK] = { OLED::first_byte_to_send(), OLED::PAYLOAD_DATA };

  // Every row is 4 pixels high, display's page is 8 pixels high, so 2 rows at a time.
  for (int8_t r = 0; r < RowCount; r += 2) {
    for (int8_t block = 0; block < ColCount / COLUMNS_PER_BLOCK; block += 1) {
      for (int8_t delta = 0; delta < COLUMNS_PER_BLOCK; delta += 1) {
        uint8_t const c = block * COLUMNS_PER_BLOCK + delta;
        uint8_t const upperWall = getWall(r, c);
        uint8_t const lowerWall = getWall(r+1, c);
        bool const upperBall = isBall(r, c);
        bool const lowerBall = isBall(r+1, c);
  
        uint8_t* subbuf = &buf[2 + delta * 4]; // 2 fixed bytes and 4 per iteration
        subbuf[0] = 0;
        subbuf[1] = 0;
        subbuf[2] = 0;
        subbuf[3] = 0;
  
        // room
        if (upperWall) {
          subbuf[0] |= 0xF << 0;
          subbuf[1] |= 0xF << 0;
          subbuf[2] |= 0xF << 0;
          subbuf[3] |= 0xF << 0;
        }
        if (lowerWall) {
          subbuf[0] |= 0xF << 4;
          subbuf[1] |= 0xF << 4;
          subbuf[2] |= 0xF << 4;
          subbuf[3] |= 0xF << 4;
        }
  
        // ball
        if (upperBall) {
          subbuf[0] |= ball[0] << 0;
          subbuf[1] |= ball[1] << 0;
          subbuf[2] |= ball[2] << 0;
          subbuf[3] |= ball[3] << 0;
        }
        if (lowerBall) {
          subbuf[0] |= ball[0] << 4;
          subbuf[1] |= ball[1] << 4;
          subbuf[2] |= ball[2] << 4;
          subbuf[3] |= ball[3] << 4;
        }
      }
      reportError(OLED::send(buf, sizeof buf));
    }
  }
}

static void move() {
  uint8_t rowHitType;
  uint8_t colHitType;
  do {
    rowHitType = getWall(ballRow + ballRowDir, ballCol);
    colHitType = getWall(ballRow, ballCol + ballColDir);
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
  digitalWrite(LED_BUILTIN, LOW);
  displayRoom();
  reportError(OLED::set_enabled());
}

void loop() {
  delay(100);
  move();
  displayRoom();
}

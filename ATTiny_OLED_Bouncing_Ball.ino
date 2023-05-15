#include <inttypes.h>
#include "SBC-OLED01.h"

typedef SBS_OLED01<0x3C> OLED;

// Terminology:
// - "row" & "colum" apply to the coarse grid defining the room
// - "COM" & "SEG" refer to the SSD1306's output, row and column of actual pixels
static uint8_t constexpr SEGS_PER_COLUMN = 4;
static uint8_t constexpr COMS_PER_ROW = 4;
static uint8_t constexpr COLUMNS = OLED::WIDTH / SEGS_PER_COLUMN;
static uint8_t constexpr ROWS = OLED::HEIGHT / COMS_PER_ROW;
static uint8_t constexpr BYTES_PER_SEG = OLED::HEIGHT / 8; // 1 bpp, all bits used

// the ball shape
static uint8_t constexpr ball[SEGS_PER_COLUMN] = { 0x6, 0x9, 0x9, 0x6 }; // COM output per SEG

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
  return pgm_read_byte(&room[row * COLUMNS + col]);
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
  constexpr uint8_t COLUMNS_PER_BLOCK = 4;
  uint8_t buf[2 + BYTES_PER_SEG * SEGS_PER_COLUMN * COLUMNS_PER_BLOCK] =
    { OLED::first_byte_to_send(), OLED::PAYLOAD_DATA };

  for (int8_t block = 0; block < COLUMNS / COLUMNS_PER_BLOCK; block += 1) {
    for (int8_t delta = 0; delta < COLUMNS_PER_BLOCK; delta += 1) {
      uint8_t const c = block * COLUMNS_PER_BLOCK + delta;
      for (int8_t r = 0; r < ROWS; r += 8 / COMS_PER_ROW) {
        uint8_t const upperWall = getWall(r, c);
        uint8_t const lowerWall = getWall(r+1, c);
        bool const upperBall = isBall(r, c);
        bool const lowerBall = isBall(r+1, c);
  
        uint8_t* subbuf = &buf[2 + delta * BYTES_PER_SEG * SEGS_PER_COLUMN + r / 2]; // 2 fixed bytes
        subbuf[0 * BYTES_PER_SEG] = 0;
        subbuf[1 * BYTES_PER_SEG] = 0;
        subbuf[2 * BYTES_PER_SEG] = 0;
        subbuf[3 * BYTES_PER_SEG] = 0;
  
        // room
        if (upperWall) {
          subbuf[0 * BYTES_PER_SEG] |= 0xF << 0;
          subbuf[1 * BYTES_PER_SEG] |= 0xF << 0;
          subbuf[2 * BYTES_PER_SEG] |= 0xF << 0;
          subbuf[3 * BYTES_PER_SEG] |= 0xF << 0;
        }
        if (lowerWall) {
          subbuf[0 * BYTES_PER_SEG] |= 0xF << COMS_PER_ROW;
          subbuf[1 * BYTES_PER_SEG] |= 0xF << COMS_PER_ROW;
          subbuf[2 * BYTES_PER_SEG] |= 0xF << COMS_PER_ROW;
          subbuf[3 * BYTES_PER_SEG] |= 0xF << COMS_PER_ROW;
        }
  
        // ball
        if (upperBall) {
          subbuf[0 * BYTES_PER_SEG] |= ball[0] << 0;
          subbuf[1 * BYTES_PER_SEG] |= ball[1] << 0;
          subbuf[2 * BYTES_PER_SEG] |= ball[2] << 0;
          subbuf[3 * BYTES_PER_SEG] |= ball[3] << 0;
        }
        if (lowerBall) {
          subbuf[0 * BYTES_PER_SEG] |= ball[0] << COMS_PER_ROW;
          subbuf[1 * BYTES_PER_SEG] |= ball[1] << COMS_PER_ROW;
          subbuf[2 * BYTES_PER_SEG] |= ball[2] << COMS_PER_ROW;
          subbuf[3 * BYTES_PER_SEG] |= ball[3] << COMS_PER_ROW;
        }
      }
    }
    reportError(OLED::send(buf, sizeof buf));
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

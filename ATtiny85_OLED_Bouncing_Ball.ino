#include <inttypes.h>
#include "SBC-OLED01.h"

typedef SBS_OLED01<0x3C> OLED;

// Terminology:
// - "row" & "col" apply to the coarse grid defining the room
// - "Y" and "X" refer to the rows and columns of actual pixels
static uint8_t constexpr X_PER_COL = 4;
static uint8_t constexpr Y_PER_ROW = 4;
static uint8_t constexpr BYTES_PER_X = OLED::BYTES_PER_SEG;
static uint8_t constexpr COLS = OLED::WIDTH / X_PER_COL;
static uint8_t constexpr ROWS = OLED::HEIGHT / Y_PER_ROW;

static uint8_t ballY = 10 * Y_PER_ROW;
static uint8_t ballX =  7 * X_PER_COL;
static int8_t ballYDir = +1;
static int8_t ballXDir = -2;

static uint8_t constexpr ball_shape[X_PER_COL] = { 0x6, 0xF, 0xF, 0x6 }; // pixel columns per X

static uint8_t const room_shape[ROWS * COLS] PROGMEM = {
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

static uint8_t leftshift(uint8_t value, int8_t positions) {
  return positions >= 0 ? (value << positions) : (value >> -positions);
}

static uint8_t getWall(uint8_t row, uint8_t col) {
  return pgm_read_byte(&room_shape[row * COLS + col]);
}

static void flashN(uint8_t number) {
  while (number >= 5) {
    number -= 5;
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(700);
    digitalWrite(LED_BUILTIN, LOW);
  }
  while (number >= 1) {
    number -= 1;
    delay(100);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(300);
    digitalWrite(LED_BUILTIN, LOW);
  }
}

static void reportError(OLED::Status status) {
  if (status.errorlevel) {
    flashN(status.errorlevel);
    delay(300);
    flashN(status.tracker);
    delay(900);
  }
}

static void displayRoom() {
  OLED::SSD1306 m(40);
  m.send(OLED::PAYLOAD_DATA);
  for (int8_t c = 0; c < COLS; c += 1) {
    uint8_t buf[BYTES_PER_X * X_PER_COL];
    for (int8_t r = 0; r < ROWS; r += 8 / Y_PER_ROW) {
      uint8_t* subbuf = buf + r / 2;
      subbuf[0 * BYTES_PER_X] = 0;
      subbuf[1 * BYTES_PER_X] = 0;
      subbuf[2 * BYTES_PER_X] = 0;
      subbuf[3 * BYTES_PER_X] = 0;

      // room
      if (getWall(r, c)) {
        subbuf[0 * BYTES_PER_X] |= 0xF << 0;
        subbuf[1 * BYTES_PER_X] |= 0xF << 0;
        subbuf[2 * BYTES_PER_X] |= 0xF << 0;
        subbuf[3 * BYTES_PER_X] |= 0xF << 0;
      }
      if (getWall(r + 1, c)) {
        subbuf[0 * BYTES_PER_X] |= 0xF << Y_PER_ROW;
        subbuf[1 * BYTES_PER_X] |= 0xF << Y_PER_ROW;
        subbuf[2 * BYTES_PER_X] |= 0xF << Y_PER_ROW;
        subbuf[3 * BYTES_PER_X] |= 0xF << Y_PER_ROW;
      }

      // ball
      int8_t const ballXoffset = int8_t(ballX) - int8_t(c * X_PER_COL);
      int8_t const ballYoffset = int8_t(ballY) - int8_t(r * Y_PER_ROW);
      if (ballXoffset < X_PER_COL && -Y_PER_ROW < ballYoffset && ballYoffset < Y_PER_ROW * 2) {
        uint8_t const i0 = ballXoffset < 0 ? -ballXoffset : 0;
        uint8_t const iN = X_PER_COL - (ballXoffset < 0 ? 0 : ballXoffset);
        for (uint8_t i = i0; i < iN; ++i) {
          subbuf[(i + ballXoffset) * BYTES_PER_X] |= leftshift(ball_shape[i], ballYoffset);
        }
      }
    }
    for (unsigned char i = 0; i < sizeof buf; ++i) {
      m.send(buf[i]);
    }
  }
  reportError(m.stop());
}

static void move() {
  for (uint8_t twice = 0; twice < 2; ++twice) {
    uint8_t const edgeY = ballY + ballYDir + (ballYDir < 0 ? 0 : Y_PER_ROW - 1);
    uint8_t const edgeX = ballX + ballXDir + (ballXDir < 0 ? 0 : X_PER_COL - 1);
    bool const yHit = getWall(edgeY / Y_PER_ROW, ballX / X_PER_COL) != 0;
    bool const xHit = getWall(ballY / Y_PER_ROW, edgeX / X_PER_COL) != 0;
    if (yHit) {
      ballYDir = -ballYDir;
    }
    if (xHit) {
      ballXDir = -ballXDir;
    }
    if (!yHit && !xHit) {
      ballY += ballYDir;
      ballX += ballXDir;
      return;
    }
  }
  reportError(OLED::Status { 11, 0 }); // trapped between walls
}


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  auto init_err = OLED::init();
  digitalWrite(LED_BUILTIN, LOW);
  reportError(init_err);
  reportError(OLED::set_enabled());
  delay(500);
  OLED::clear();
  delay(1000);
  displayRoom();
}

void loop() {
  delay(30);
  move();
  displayRoom();
}

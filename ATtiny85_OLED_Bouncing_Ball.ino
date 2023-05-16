#include <inttypes.h>
#include "SBC-OLED01.h"

typedef SBS_OLED01<0x3C> OLED;

// Terminology:
// - "row" & "col" apply to the coarse grid defining the room
// - "Y" or "COM" refer to the SSD1306's output pins towards rows of actual pixels
// - "X" or "SEG" refer to the SSD1306's output pins towards columns of actual pixels
static uint8_t constexpr X_PER_COL = 4;
static uint8_t constexpr Y_PER_ROW = 4;
static uint8_t constexpr BYTES_PER_X = OLED::BYTES_PER_SEG;
static uint8_t constexpr COLS = OLED::WIDTH / X_PER_COL;
static uint8_t constexpr ROWS = OLED::HEIGHT / Y_PER_ROW;

static uint8_t ballY = 10 * Y_PER_ROW;
static uint8_t ballX =  7 * X_PER_COL;
static int8_t ballYDir = +1;
static int8_t ballXDir = -2;

static uint8_t constexpr ball_shape[X_PER_COL] = { 0x6, 0xF, 0xF, 0x6 }; // COM output per SEG

const static constexpr uint8_t room_shape[ROWS * COLS] PROGMEM = {
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

static int getBallOffsetX(uint8_t col) {
  return ballX - col * X_PER_COL;
}

static int getBallOffsetY(uint8_t row) {
  return ballY - row * Y_PER_ROW;
}

static void reportError(uint8_t el) {
  if (el) {
    digitalWrite(LED_BUILTIN, LOW);
    for (uint8_t f = 0; f < el; ++f) {
      delay(150);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(300);
      digitalWrite(LED_BUILTIN, LOW);
    }
    delay(300);
  }
}

static void displayRoom() {
  auto m = OLED::feed();
  for (int8_t c = 0; c < COLS; c += 1) {
    uint8_t buf[BYTES_PER_X * X_PER_COL];
    for (int8_t r = 0; r < ROWS; r += 8 / Y_PER_ROW) {
      uint8_t const upperWall = getWall(r, c);
      uint8_t const lowerWall = getWall(r + 1, c);

      uint8_t* subbuf = buf + r / 2;
      subbuf[0 * BYTES_PER_X] = 0;
      subbuf[1 * BYTES_PER_X] = 0;
      subbuf[2 * BYTES_PER_X] = 0;
      subbuf[3 * BYTES_PER_X] = 0;

      // room
      if (upperWall) {
        subbuf[0 * BYTES_PER_X] |= 0xF << 0;
        subbuf[1 * BYTES_PER_X] |= 0xF << 0;
        subbuf[2 * BYTES_PER_X] |= 0xF << 0;
        subbuf[3 * BYTES_PER_X] |= 0xF << 0;
      }
      if (lowerWall) {
        subbuf[0 * BYTES_PER_X] |= 0xF << Y_PER_ROW;
        subbuf[1 * BYTES_PER_X] |= 0xF << Y_PER_ROW;
        subbuf[2 * BYTES_PER_X] |= 0xF << Y_PER_ROW;
        subbuf[3 * BYTES_PER_X] |= 0xF << Y_PER_ROW;
      }

      // ball
      int const xoffsetBall = getBallOffsetX(c);
      int const yoffsetBall = getBallOffsetY(r);
      if (-X_PER_COL < xoffsetBall && xoffsetBall < X_PER_COL &&
          -Y_PER_ROW < yoffsetBall && yoffsetBall < Y_PER_ROW * 2) {
        uint8_t const i0 = max(0, -xoffsetBall);
        uint8_t const iN = X_PER_COL + min(0, -xoffsetBall);
        for (int i = i0; i < iN; ++i) {
          subbuf[(i + xoffsetBall) * BYTES_PER_X] |= leftshift(ball_shape[i], yoffsetBall);
        }
      }
    }

    m.transmit(buf, sizeof buf);
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
  reportError(11); // trapped between walls
}


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  reportError(OLED::init());
  digitalWrite(LED_BUILTIN, LOW);
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

#include <inttypes.h>
#include "OLED.h"

static byte constexpr OLED_ADDRESS = 0x3C;

// Terminology:
// - "row" & "col" apply to the coarse grid defining the room
// - "Y" and "X" refer to the rows and columns of actual pixels
static uint8_t constexpr X_PER_COL = 4;
static uint8_t constexpr Y_PER_ROW = 4;
static uint8_t constexpr COLS = OLED::WIDTH / X_PER_COL;
static uint8_t constexpr ROWS = OLED::HEIGHT / Y_PER_ROW;
static uint8_t constexpr BYTES_PER_X = OLED::BYTES_PER_SEG;
static uint8_t constexpr ROWS_PER_BYTE = 8 / Y_PER_ROW;

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

static void reportError(I2C::Status status) {
  if (status.errorlevel) {
    delay(300);
    flashN(status.errorlevel);
    delay(600);
    flashN(status.location);
    delay(900);
  }
}

static I2C::Status displayRoom() {
  auto chat = OLED::Chat(OLED_ADDRESS, 20).start_data();
  for (uint8_t c = 0; c < COLS; c += 1) {
    uint8_t buf[BYTES_PER_X * X_PER_COL];
    for (uint8_t rp = 0; rp < ROWS / ROWS_PER_BYTE; rp += 1) {
      uint8_t const r = rp * ROWS_PER_BYTE;

      uint8_t* const subbuf = &buf[rp];
      for (uint8_t i = 0; i < X_PER_COL; ++i) {
        subbuf[i * BYTES_PER_X] = 0;
      }

      // room
      for (uint8_t s = 0; s < ROWS_PER_BYTE; ++s) {
        if (getWall(r + s, c)) {
          for (uint8_t i = 0; i < X_PER_COL; ++i) {
            subbuf[i * BYTES_PER_X] |= 0xF << (s * Y_PER_ROW);
          }
        }
      }

      // ball
      int8_t const ballXoffset = int8_t(ballX) - int8_t(c * X_PER_COL);
      int8_t const ballYoffset = int8_t(ballY) - int8_t(r * Y_PER_ROW);
      if (ballXoffset < X_PER_COL && -Y_PER_ROW < ballYoffset && ballYoffset < Y_PER_ROW * 2) {
        uint8_t const i0 =         0 + (ballXoffset < 0 ? -ballXoffset : 0);
        uint8_t const iN = X_PER_COL - (ballXoffset < 0 ? 0 : ballXoffset);
        for (uint8_t i = i0; i < iN; ++i) {
          subbuf[(i + ballXoffset) * BYTES_PER_X] |= leftshift(ball_shape[i], ballYoffset);
        }
      }
    }
    for (uint8_t i = 0; i < sizeof buf; ++i) {
      chat.send(buf[i]);
    }
  }
  return chat.stop();
}

static void move() {
  for (uint8_t twice = 0; twice < 2; ++twice) {
    // We consider the ball to be a square for collision detection.
    // Add size of the ball in pixels when moving in a positive direction,
    // because that's the edge of the square possibly hitting a wall.
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
  reportError(I2C::Status { 11, 0 }); // trapped between walls
}


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  USI_TWI_Master_Initialise();
  auto err = OLED::Chat(OLED_ADDRESS, 0)
             .init()
             .set_addressing_mode(OLED::VerticalAddressing)
             .set_column_address()
             .set_page_address()
             .set_enabled()
             .stop();
  if (!err.errorlevel) {
    err = displayRoom();
  }
  digitalWrite(LED_BUILTIN, LOW);
  reportError(err);
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  move();
  digitalWrite(LED_BUILTIN, LOW);
  reportError(displayRoom());
}

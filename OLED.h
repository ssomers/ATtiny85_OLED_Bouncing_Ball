#pragma once
#include "I2C.h"
#include <Arduino.h>

namespace OLED {

static constexpr uint8_t HEIGHT = 64;                 // number of pixels high
static constexpr uint8_t WIDTH = 128;                 // number of pixels wide
static constexpr uint16_t BYTES = HEIGHT * WIDTH / 8; // number of bytes covering display
static constexpr uint8_t BYTES_PER_SEG = HEIGHT / 8;  // number of bytes per displayed column of pixels
static constexpr uint8_t BYTES_PER_PAGE = WIDTH;      // number of bytes per page (block of 8 displayed rows)

static constexpr byte PAYLOAD_COMMAND = 0x80; // prefix to command or its option(s)
static constexpr byte PAYLOAD_DATA = 0x40;    // prefix to data (bitmap upload)
static constexpr byte PAYLOAD_LASTCOM = 0x00; // prefix to command or its option(s) after which a stop will follow;
//                                               if further data is sent anyway, the device may ignore part of it.

enum Addressing {
  HorizontalAddressing = 0b00,
  VerticalAddressing = 0b01,
  PageAddressing = 0b10
};

// Full conversation with SSD 1306.
template <typename Device>
class Chat : public I2C::Chat<Device> {
    using super = I2C::Chat<Device>;
  public:
    explicit Chat(uint8_t start_location) : I2C::Chat<Device>(start_location) {}

    Chat& init() {
      super::send(PAYLOAD_COMMAND).send(0x8D); // Set charge pump (powering the OLED grid)…
      super::send(PAYLOAD_COMMAND).send(0x14); // …enabled.
      return *this;
    }

    Chat& set_enabled(bool enabled = true) {
      super::send(PAYLOAD_COMMAND).send(byte{0xAE} | byte{enabled});
      return *this;
    }

    Chat& set_contrast(uint8_t fraction) {
      super::send(PAYLOAD_COMMAND).send(0x81);
      super::send(PAYLOAD_COMMAND).send(fraction);
      return *this;
    }

    Chat& set_addressing_mode(Addressing mode) {
      super::send(PAYLOAD_COMMAND).send(0x20);
      super::send(PAYLOAD_COMMAND).send(mode);
      return *this;
    }

    Chat& set_column_address(uint8_t start = 0, uint8_t end = WIDTH - 1) {
      super::send(PAYLOAD_COMMAND).send(0x21);
      super::send(PAYLOAD_COMMAND).send(start);
      super::send(PAYLOAD_COMMAND).send(end);
      return *this;
    }

    Chat& set_page_address(uint8_t start = 0, uint8_t end = 7) {
      super::send(PAYLOAD_COMMAND).send(0x22);
      super::send(PAYLOAD_COMMAND).send(start);
      super::send(PAYLOAD_COMMAND).send(end);
      return *this;
    }

    Chat& set_page_start_address(uint8_t pageN) {
      set_addressing_mode(PageAddressing);
      super::send(PAYLOAD_COMMAND).send(byte{0xB0} | pageN);
      return *this;
    }

    // You can only send the data and stop this chat after this.
    I2C::Chat<Device>& start_data() {
      return super::send(PAYLOAD_DATA);
    }
};

// Data conversation with SSD 1306 addressing two consecutive pages.
template <typename Device>
class QuarterChat : public I2C::Chat<Device> {
    using super = I2C::Chat<Device>;
  public:
    explicit QuarterChat(uint8_t quarter, uint8_t xBegin = 0, uint8_t xEnd = OLED::WIDTH - 1)
      : I2C::Chat<Device> (
          OLED::Chat<Device>(20)
          .set_page_address(quarter * 2, quarter * 2 + 1)
          .set_column_address(xBegin, xEnd)
          .start_data()
        ) {
    }

    // Send one column.
    QuarterChat& send(byte b1, byte b2) {
      super::send(b1);
      super::send(b2);
      return *this;
    }

    // Send empty columns.
    QuarterChat& sendSpacing(uint8_t width) {
      for (uint8_t i = 0; i < width; ++i) {
        send(0, 0);
      }
      return *this;
    }
};

}

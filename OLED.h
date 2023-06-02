#pragma once
#include "I2C.h"
#include <Arduino.h>

class OLED {
  public:
    static constexpr uint8_t HEIGHT = 64;                 // number of pixels high
    static constexpr uint8_t WIDTH = 128;                 // number of pixels wide
    static constexpr uint16_t BYTES = HEIGHT * WIDTH / 8; // number of bytes covering display
    static constexpr uint8_t BYTES_PER_SEG = HEIGHT / 8;  // number of bytes per displayed column of pixels
    static constexpr uint8_t BYTES_PER_PAGE = WIDTH;      // number of bytes per page (block of 8 displayed rows)

    static constexpr byte PAYLOAD_COMMAND = 0x80;      // prefix to command or its option(s)
    static constexpr byte PAYLOAD_DATA = 0x40;         // prefix to data (bitmap upload)
    static constexpr byte PAYLOAD_LASTCOM = 0x00;      // prefix to command after which a stop will follow

    enum Addressing {
      HorizontalAddressing = 0b00, VerticalAddressing = 0b01, PageAddressing = 0b10
    };

    // Full conversation with SSD 1306.
    class Chat : public I2C::Chat {
      public:
        Chat(byte address, uint8_t start_location) : I2C::Chat{address, start_location} {}

        Chat& init() {
          send(PAYLOAD_COMMAND).send(0x8D); // Set charge pump (powering the OLED grid)…
          send(PAYLOAD_COMMAND).send(0x14); // …enabled.
          return *this;
        }

        Chat& set_enabled(bool enabled = true) {
          send(PAYLOAD_COMMAND).send(byte{0xAE} | byte{enabled});
          return *this;
        }

        Chat& set_contrast(uint8_t fraction) {
          send(PAYLOAD_COMMAND).send(0x81);
          send(PAYLOAD_COMMAND).send(fraction);
          return *this;
        }

        Chat& set_addressing_mode(Addressing mode) {
          send(PAYLOAD_COMMAND).send(0x20);
          send(PAYLOAD_COMMAND).send(mode);
          return *this;
        }

        Chat& set_column_address(uint8_t start = 0, uint8_t end = WIDTH - 1) {
          send(PAYLOAD_COMMAND).send(0x21);
          send(PAYLOAD_COMMAND).send(start);
          send(PAYLOAD_COMMAND).send(end);
          return *this;
        }

        Chat& set_page_address(uint8_t start = 0, uint8_t end = 7) {
          send(PAYLOAD_COMMAND).send(0x22);
          send(PAYLOAD_COMMAND).send(start);
          send(PAYLOAD_COMMAND).send(end);
          return *this;
        }

        Chat& set_page_start_address(uint8_t pageN) {
          set_addressing_mode(PageAddressing);
          send(PAYLOAD_COMMAND).send(byte{0xB0} | pageN);
          return *this;
        }

        // You can only send the data and stop this chat after this.
        I2C::Chat& start_data() {
          return send(PAYLOAD_DATA);
        }
    };

    // Data conversation with SSD 1306 addressing two consecutive pages (i.e. one of four rows).
    class QuarterChat : public I2C::Chat {
      public:
        QuarterChat(uint8_t address, uint8_t quarter, uint8_t xBegin = 0, uint8_t xEnd = OLED::WIDTH - 1)
          : I2C::Chat{OLED::Chat(address, 20)
                      .set_page_address(quarter * 2, quarter * 2 + 1)
                      .set_column_address(xBegin, xEnd)
                      .start_data()}
        {}

        // Send one column.
        QuarterChat& send(byte b1, byte b2) {
          I2C::Chat::send(b1);
          I2C::Chat::send(b2);
          return *this;
        }

        // Send N empty columns.
        QuarterChat& sendSpacing(uint16_t width) {
          I2C::Chat::sendN(width * 2, 0);
          return *this;
        }
    };
};

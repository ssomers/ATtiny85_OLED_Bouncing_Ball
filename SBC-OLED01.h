#pragma once
#include "USI_TWI_Master.h"

class SBS_OLED01 {
  public:
    static constexpr uint8_t HEIGHT = 64;                 // number of pixels high
    static constexpr uint8_t WIDTH = 128;                 // number of pixels wide
    static constexpr uint16_t BYTES = HEIGHT * WIDTH / 8; // number of bytes covering display
    static constexpr uint8_t BYTES_PER_SEG = HEIGHT / 8;  // number of bytes per displayed column of pixels
    static constexpr uint8_t BYTES_PER_PAGE = WIDTH;      // number of bytes per page (block of 8 displayed rows)

    enum Addressing {
      HorizontalAddressing = 0b00, VerticalAddressing = 0b01, PageAddressing = 0b10
    };

    // Two numbers describing either success (if errorlevel is zero) or an error.
    struct Status {
      uint8_t errorlevel;
      uint8_t location;
    };

    class BareChat {
      private:
        USI_TWI_ErrorLevel err;
        uint8_t location;

      protected:
        static constexpr byte PAYLOAD_COMMAND = 0x80;      // prefix to command or its option(s)
        static constexpr byte PAYLOAD_DATA = 0x40;         // prefix to data (bitmap upload)
        static constexpr byte PAYLOAD_LASTCOM = 0x00;      // prefix to command after which a stop will follow

        BareChat(byte address, uint8_t start_location) :
          err(USI_TWI_Master_Start()),
          location(start_location) {
          if (!err) {
            ++location;
            byte prefix = USI_TWI_Prefix(USI_TWI_SEND, address);
            err = USI_TWI_Master_Transmit(prefix, true);
          }
        }

      public:
        // Check we're still on speaking terms.
        operator bool() const {
          return err == USI_TWI_OK;
        }

        // Send one byte.
        BareChat& send(byte msg) {
          if (!err) {
            ++location;
            err = USI_TWI_Master_Transmit(msg, false);
          }
          return *this;
        }

        // Send the same byte many times.
        BareChat& sendN(uint16_t count, byte msg) {
          for (uint16_t _ = 0; _ < count; ++_) {
            send(msg);
          }
          return *this;
        }

        Status stop() {
          if (!err) {
            err = USI_TWI_Master_Stop();
          }
          return Status { err, location };
        }
    };

    class Chat : public BareChat {
      public:
        Chat(byte address, uint8_t start_location) : BareChat(address, start_location) {}

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
        BareChat& start_data() {
          return send(PAYLOAD_DATA);
        }
    };
};

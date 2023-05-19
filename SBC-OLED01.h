#include "USI_TWI_Master.h"

template <uint8_t ADDRESS>
class SBS_OLED01 {
  public:
    // Two numbers describing either success (if errorlevel is zero) or an error.
    struct Status {
      uint8_t errorlevel;
      uint8_t location;
    };

    class Chat {
      private:
        USI_TWI_ErrorLevel err;
        uint8_t location = 0;

      public:
        explicit Chat(uint8_t location) :
          err(USI_TWI_Master_Start()),
          location(location) {
          if (!err) {
            ++location;
            uint8_t prefix = USI_TWI_Prefix(USI_TWI_SEND, ADDRESS);
            err = USI_TWI_Master_Transmit(prefix, true);
          }
        }

        // Check we're still on speaking terms.
        operator bool() const {
          return err == USI_TWI_OK;
        }

        // Send one byte.
        Chat& send(uint8_t msg) {
          if (!err) {
            ++location;
            err = USI_TWI_Master_Transmit(msg, false);
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

  public:
    static constexpr uint8_t HEIGHT = 64;                 // number of pixels high
    static constexpr uint8_t WIDTH = 128;                 // number of pixels wide
    static constexpr uint8_t BYTES_PER_SEG = HEIGHT / 8;  // number of bytes per displayed column of pixels
    static constexpr uint8_t PAYLOAD_COMMAND = 0x80;      // prefix to command or its option(s)
    static constexpr uint8_t PAYLOAD_DATA = 0x40;         // prefix to data (bitmap upload)
    static constexpr uint8_t PAYLOAD_COMMAND_DATA = 0x00; // prefix to command to be followed by data

    static Status init() {
      USI_TWI_Master_Initialise();
      return Chat(0)
             .send(PAYLOAD_COMMAND).send(0x20) // Set memory addressing mode…
             .send(PAYLOAD_COMMAND).send(0x01) // …vertical.
             .send(PAYLOAD_COMMAND).send(0x8D) // Set charge pump (powering the OLED grid)…
             .send(PAYLOAD_COMMAND).send(0x14) // …enabled.
             .stop();
    }

    static Status set_enabled(bool truly = true) {
      return Chat(10)
             .send(PAYLOAD_COMMAND).send(uint8_t(0xAE | truly))
             .stop();
    }

    static Status set_contrast(uint8_t fraction) {
      return Chat(15)
             .send(PAYLOAD_COMMAND, 0x81)
             .send(PAYLOAD_COMMAND, fraction)
             .stop();
    }

    static Status clear() {
      Chat chat(20);
      chat.send(PAYLOAD_DATA);
      for (auto i = 0; i < BYTES_PER_SEG * WIDTH; ++i) {
        chat.send(0);
      }
      return chat.stop();
    }
};

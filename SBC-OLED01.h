#include "USI_TWI_Master.h"

template <uint8_t ADDRESS>
class SBS_OLED01 {
  public:
    // Two numbers describing either success (if errorlevel is zero) or an error.
    struct Status {
      uint8_t errorlevel;
      uint8_t tracker;
    };

    class SSD1306 {
      private:
        USI_TWI_ErrorLevel err;
        uint8_t tracker = 0;

      public:
        explicit SSD1306(uint8_t tracker) :
          err(USI_TWI_Master_Start()),
          tracker(tracker) {
          if (!err) {
            ++tracker;
            uint8_t prefix = USI_TWI_Prefix(USI_TWI_SEND, ADDRESS);
            err = USI_TWI_Master_Transmit(prefix, true);
          }
        }

        // Check we're still operational.
        operator bool() const {
          return err == USI_TWI_OK;
        }

        // Send one byte.
        SSD1306& send(uint8_t msg) {
          if (!err) {
            ++tracker;
            err = USI_TWI_Master_Transmit(msg, false);
          }
          return *this;
        }

        Status stop() {
          if (!err) {
            err = USI_TWI_Master_Stop();
          }
          return Status { err, tracker };
        }
    };

  public:
    static constexpr uint8_t HEIGHT = 64;                 // number of pixels high
    static constexpr uint8_t WIDTH = 128;                 // number of pixels wide
    static constexpr uint8_t BYTES_PER_SEG = HEIGHT / 8;  // number of bytes per displayed column of pixels
    static constexpr uint8_t PAYLOAD_COMMAND_CONT = 0x80; // prefix to command to be continued by option or another command
    static constexpr uint8_t PAYLOAD_COMMAND_LAST = 0x00; // prefix to last option or command
    static constexpr uint8_t PAYLOAD_DATA = 0x40;         // prefix to bitmap upload

    static Status init() {
      USI_TWI_Master_Initialise();
      return SSD1306(0)
             .send(PAYLOAD_COMMAND_CONT).send(0x20) // Set memory addressing mode…
             .send(PAYLOAD_COMMAND_CONT).send(0x01) // …vertical.
             .send(PAYLOAD_COMMAND_CONT).send(0x8D) // Set charge pump (powering the OLED grid)…
             .send(PAYLOAD_COMMAND_LAST).send(0x14) // …enabled.
             .stop();
    }

    static Status set_enabled(bool truly = true) {
      return SSD1306(10)
             .send(PAYLOAD_COMMAND_LAST).send(uint8_t(0xAE | truly))
             .stop();
    }

    static Status set_contrast(uint8_t fraction) {
      return SSD1306(15)
             .send(PAYLOAD_COMMAND_CONT, 0x81)
             .send(PAYLOAD_COMMAND_LAST, fraction)
             .stop();
    }

    static Status clear() {
      SSD1306 m(20);
      m.send(PAYLOAD_DATA);
      for (auto i = 0; i < BYTES_PER_SEG * WIDTH; ++i) {
        m.send(0);
      }
      return m.stop();
    }
};

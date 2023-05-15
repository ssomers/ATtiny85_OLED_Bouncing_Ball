#include "USI_TWI_Master.h"

template <uint8_t ADDRESS>
class SBS_OLED01 {
  public:
    static constexpr int HEIGHT = 64; // number of pixels high
    static constexpr int WIDTH = 128; // number of pixels wide
    static constexpr int BYTES_PER_SEG = HEIGHT / 8; // 1 bpp, all bits used
    static constexpr uint8_t PAYLOAD_COMMAND_CONT = 0x80; // prefix to command to be continued by option or another command
    static constexpr uint8_t PAYLOAD_COMMAND_LAST = 0x00; // prefix to last option or command
    static constexpr uint8_t PAYLOAD_DATA = 0x40;         // anything 0b01xxxxxx

    static USI_TWI_Master master() {
      return USI_TWI_Master(ADDRESS);
    }

    static USI_TWI_ErrorLevel init() {
      USI_TWI_Master_Initialise();
      
      return master()
      .transmit(PAYLOAD_COMMAND_CONT, 0x20) // Set memory addressing mode…
      .transmit(PAYLOAD_COMMAND_CONT, 0x01) // …vertical.
      .transmit(PAYLOAD_COMMAND_CONT, 0x8D) // Set charge pump (output power)…
      .transmit(PAYLOAD_COMMAND_LAST, 0x14) // …enabled.
      .stop();
    }

    static USI_TWI_ErrorLevel set_enabled(bool truly = true) {
      return master()
      .transmit(PAYLOAD_COMMAND_LAST, uint8_t(0xAEu | truly))
      .stop();
    }

    static USI_TWI_ErrorLevel set_contrast(uint8_t fraction) {
      return master()
      .transmit(PAYLOAD_COMMAND_CONT, 0x81)
      .transmit(PAYLOAD_COMMAND_LAST, fraction)
      .stop();
    }

    static USI_TWI_ErrorLevel clear() {
      auto m = master();
      m.transmit(PAYLOAD_DATA);
      for (auto i = 0; i < BYTES_PER_SEG * WIDTH; ++i) {
        m.transmit(0);
      }
      return m.stop();
    }
};

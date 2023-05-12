#include "USI_TWI_Master.h"

template <uint8_t ADDRESS>
class SBS_OLED01 {
  public:
    static constexpr uint8_t PAYLOAD_COMMAND_CONT = 0x80; // prefix to command to be continued by option or another command
    static constexpr uint8_t PAYLOAD_COMMAND_LAST = 0x00; // prefix to last option or command
    static constexpr uint8_t PAYLOAD_DATA = 0x40;         // anything 0b01xxxxxx
    
    static USI_TWI_ErrorLevel init() {
      USI_TWI_Master_Initialise();
      uint8_t buf[] = {
        first_byte_to_send(),
        PAYLOAD_COMMAND_CONT, 0x20, // Set memory addressing mode…
        PAYLOAD_COMMAND_CONT, 0x00, // …horizontal.
        PAYLOAD_COMMAND_CONT, 0x8D, // Set charge pump (output power)…
        PAYLOAD_COMMAND_LAST, 0x14, // …enabled.
      };
      return USI_TWI_Master_Transmit(buf, sizeof buf);
    }

    static USI_TWI_ErrorLevel set_enabled(bool truly = true) {
      uint8_t buf[] = {
        first_byte_to_send(),
        PAYLOAD_COMMAND_LAST, 0xAE | truly
      };
      return USI_TWI_Master_Transmit(buf, sizeof buf);
    }

    static USI_TWI_ErrorLevel set_contrast(uint8_t fraction) {
      uint8_t buf[] = {
        first_byte_to_send(),
        PAYLOAD_COMMAND_CONT, 0x81,
        PAYLOAD_COMMAND_LAST, fraction
      };
      return USI_TWI_Master_Transmit(buf, sizeof buf);
    }

    // First byte of a buffer to be sent.
    static uint8_t first_byte_to_send() {
      return USI_TWI_Prefix(USI_TWI_SEND, ADDRESS);
    }
    
    // Sends off buffer and returns 0 on success or error code.
    static USI_TWI_ErrorLevel send(uint8_t buf[], uint8_t len) {
      return USI_TWI_Master_Transmit(buf, len);
    }

    static USI_TWI_ErrorLevel clear() {
      uint8_t buf[2 + 64] = { first_byte_to_send(), PAYLOAD_DATA };
      for (auto i = 0; i < 128*64/8/64; ++i) {
        auto el = USI_TWI_Master_Transmit(buf, sizeof buf);
        if (el) return el;
      }
      return USI_TWI_OK;
    }

    /*
    static void cursorTo(uint8_t row, uint8_t col) {
      sendCommand(0x00 | (0x0F & col) );  // low col = 0
      sendCommand(0x10 | (0x0F & (col>>4)) );  // hi col = 0
      //  sendCommand(0x40 | (0x0F & row) ); // line #0
      sendCommand(0xb0 | (0x03 & row) );  // hi col = 0
    }
    */
};

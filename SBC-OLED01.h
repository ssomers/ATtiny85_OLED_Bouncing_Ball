#include "USI_TWI_Master.h"

template <uint8_t ADDRESS>
class SBS_OLED01 {
  public:
    static constexpr int HEIGHT = 64; // number of pixels high
    static constexpr int WIDTH = 128; // number of pixels wide
    static constexpr uint8_t PAYLOAD_COMMAND_CONT = 0x80; // prefix to command to be continued by option or another command
    static constexpr uint8_t PAYLOAD_COMMAND_LAST = 0x00; // prefix to last option or command
    static constexpr uint8_t PAYLOAD_DATA = 0x40;         // anything 0b01xxxxxx
    
    static USI_TWI_ErrorLevel init() {
      USI_TWI_Master_Initialise();
      uint8_t buf[] = {
        first_byte_to_send(),
        PAYLOAD_COMMAND_CONT, 0x20, // Set memory addressing mode…
        PAYLOAD_COMMAND_CONT, 0x01, // …vertical.
        PAYLOAD_COMMAND_CONT, 0x8D, // Set charge pump (output power)…
        PAYLOAD_COMMAND_LAST, 0x14, // …enabled.
      };
      return USI_TWI_Master_Transmit(buf, sizeof buf);
    }

    static USI_TWI_ErrorLevel set_enabled(bool truly = true) {
      uint8_t buf[] = {
        first_byte_to_send(),
        PAYLOAD_COMMAND_LAST, uint8_t(0xAEu | truly)
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
      constexpr uint8_t PAYLOAD = 64;
      uint8_t buf[2 + PAYLOAD] = { first_byte_to_send(), PAYLOAD_DATA }; // payload gets zero-initialized
      for (auto i = 0; i < HEIGHT*WIDTH/8/PAYLOAD; ++i) {
        auto el = USI_TWI_Master_Transmit(buf, sizeof buf);
        if (el) return el;
      }
      return USI_TWI_OK;
    }
};

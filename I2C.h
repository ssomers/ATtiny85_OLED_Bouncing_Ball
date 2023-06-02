#pragma once
#include "USI_TWI_Master.h"
#include <Arduino.h>

namespace I2C {

// Two numbers describing either success (if errorlevel is zero) or an error.
struct Status {
  uint8_t errorlevel;
  uint8_t location;
};

// Data conversation with an I2C device.
class Chat {
  private:
    USI_TWI_ErrorLevel err;
    uint8_t location;

  public:
    Chat(byte address, uint8_t start_location) :
      err{USI_TWI_Master_Start_Sending(address)},
      location{start_location} {
    }

    // Check we're still on speaking terms.
    operator bool() const {
      return err == USI_TWI_OK;
    }

    // Send one byte.
    Chat& send(byte msg) {
      if (!err) {
        ++location;
        err = USI_TWI_Master_Send(msg);
      }
      return *this;
    }

    // Send the same byte many times.
    Chat& sendN(uint16_t count, byte msg) {
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

}

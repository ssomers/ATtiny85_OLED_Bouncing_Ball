#pragma once
#include "USI_TWI_Master.h"
#include <Arduino.h>

namespace I2C {

// Two numbers describing either success (if errorlevel is zero),
// or a communication error and at which step it occurred.
struct Status {
  uint8_t errorlevel;
  uint8_t location;
};

// Data conversation with an I2C device.
template <typename Device>
class Chat {
  private:
    USI_TWI_ErrorLevel err;
    uint8_t location;

  public:
    // start_location is merely the initial value of a counter for error reporting.
    explicit Chat(uint8_t start_location) :
      err{USI_TWI_Master_Start_Sending<Device>()},
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
        err = USI_TWI_Master_Send<Device>(msg);
      }
      return *this;
    }

    // Send the same byte many times.
    template <typename I>
    Chat& sendN(I count, byte msg) {
      for (I i = 0; i < count; ++i) {
        send(msg);
      }
      return *this;
    }

    Status stop() {
      if (!err) {
        err = USI_TWI_Master_Stop<Device>();
      }
      return Status { err, location };
    }
};

}

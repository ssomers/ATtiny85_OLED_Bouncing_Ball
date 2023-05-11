/*
  TinyWireM.h - a wrapper(+) class for TWI/I2C Master library for the ATtiny on Arduino
  1/21/2011 BroHogan -  brohoganx10 at gmail dot com

  Thanks to 'jkl' for the gcc version of Atmel's USI_TWI_Master code
  http://www.cs.cmu.edu/~dst/ARTSI/Create/PC%20Comm/
  I added Atmel's original Device dependant defines section back into USI_TWI_Master.h


 NOTE! - It's very important to use pullups on the SDA & SCL lines! More so than with the Wire lib.

 USAGE
  Put in setup():
	 USI_TWI_Master_Initialise()            // initialize I2C lib

  To Send:
	 TinyWireM.initFor(address)             // prepare to send to a slave (7 bit address - same as Wire)
	 TinyWireM.queueNext(data)              // buffer up bytes to send - can be called multiple times and/or with multiple bytes
	 rc = TinyWireM.send()                  // actually send the bytes in the buffer
	                                        // returns 0 on sucess or see USI_TWI_Master.h for error codes
  To Receive:
	 rc = TinyWireM.requestFrom(address, numBytes)   // reads 'numBytes' from slave address
	                                        // returns 0 on success or see USI_TWI_Master.h for error codes
	 someByte = TinyWireM.receive()         // returns the next byte in the received buffer - called multiple times
	 rc = TinyWireM.available()             // returns the number of unread bytes in the received buffer

	This library is free software; you can redistribute it and/or modify it under the
  terms of the GNU General Public License as published by the Free Software
  Foundation; either version 2.1 of the License, or any later version.
  This program is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef TinyWireM_h
#define TinyWireM_h

#include "USI_TWI_Master.h"
#include <inttypes.h>

enum TwiDirection { USI_SEND = 0, USI_RCVE = 1 };

class TinyWireM {
  public:
    static uint8_t prefix(uint8_t address, TwiDirection direction) {
      return (address << TWI_ADR_BITS) | direction;
    }

    // First byte of buffer to be sent.
    static uint8_t prefix_to_send(uint8_t address) {
      return prefix(address, USI_SEND);
    }

    // First byte of buffer to be received.
    static uint8_t prefix_to_receive(uint8_t address) {
      return prefix(address, USI_RCVE);
    }

    // Sends off or receives buffer, depending on how the first byte.
    // Returns 0 on success or error code.
    static uint8_t transmit(uint8_t buf[], uint8_t len) {
      return USI_TWI_Start_Read_Write(buf, len);
    }
};

#endif

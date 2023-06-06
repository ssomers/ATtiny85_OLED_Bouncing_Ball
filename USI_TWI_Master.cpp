/*****************************************************************************
  Based on https://github.com/adafruit/TinyWireM
****************************************************************************/
#include "USI_TWI_Master.h"
#include <avr/interrupt.h>
#include <avr/io.h>

/*!
 * @brief USI TWI single master initialization function
 */
void USI_TWI_Master_Initialise() {
  PORT_USI |= (1 << PIN_USI_SDA); // Enable pullup on SDA.
  PORT_USI |= (1 << PIN_USI_SCL); // Enable pullup on SCL.

  DDR_USI |= (1 << PIN_USI_SCL); // Enable SCL as output.
  DDR_USI |= (1 << PIN_USI_SDA); // Enable SDA as output.

  USIDR = 0xFF; // Preload dataregister with "released level" data.
  USICR = (0 << USISIE) | (0 << USIOIE) | // Disable Interrupts.
          (1 << USIWM1) | (0 << USIWM0) | // Set USI in Two-wire mode.
          (1 << USICS1) | (0 << USICS0) |
          (1 << USICLK) | // Software stobe as counter clock source
          (0 << USITC);
  USISR = (1 << USISIF) | (1 << USIOIF) | (1 << USIPF) |
          (1 << USIDC) |    // Clear flags,
          (0x0 << USICNT0); // and reset counter.
}

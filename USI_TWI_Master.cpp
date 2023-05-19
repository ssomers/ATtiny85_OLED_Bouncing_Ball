/*****************************************************************************
  Based on https://github.com/adafruit/TinyWireM
****************************************************************************/
#include "USI_TWI_Master.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <math.h>

static unsigned constexpr nanoseconds_to_cycles(float ns) {
  return ceil(ns * F_CPU / 1e9) - 1;
  // - 1 because whatever we did before or do next takes at least 1 cycle to have effect
}

static unsigned constexpr tHSTART = nanoseconds_to_cycles(600);
static unsigned constexpr tSSTOP = nanoseconds_to_cycles(600);
static unsigned constexpr tIDLE = nanoseconds_to_cycles(1300);

static void delay_cycles(unsigned cycles) {
  __builtin_avr_delay_cycles(cycles);
}

static unsigned char USI_TWI_Master_Transfer(unsigned char);

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

/*!
 * @brief USI Transmit function.
 *
 * Success or error code is returned. Error codes are defined in
 * USI_TWI_Master.h
 */
USI_TWI_ErrorLevel USI_TWI_Master_Transmit(unsigned char const msg, bool const isAddress) {
  unsigned char constexpr tempUSISR_8bit =
      (1 << USISIF) | (1 << USIOIF) | (1 << USIPF) |
      (1 << USIDC) |    // Prepare register value to: Clear flags, and
      (0x0 << USICNT0); // set USI to shift 8 bits i.e. count 16 clock edges.
  unsigned char constexpr tempUSISR_1bit =
      (1 << USISIF) | (1 << USIOIF) | (1 << USIPF) |
      (1 << USIDC) |    // Prepare register value to: Clear flags, and
      (0xE << USICNT0); // set USI to shift 1 bit i.e. count 2 clock edges.

  bool const sif = (USISR & (1 << USISIF)) != 0;
  if (isAddress != sif) {
    if (isAddress) {
      return USI_TWI_ME_START_CON;
    } else {
      return USI_TWI_UE_START_CON;
    }
  }
  if (USISR & (1 << USIPF)) {
    if (isAddress) {
      // Oh pooh…
    } else {
      return USI_TWI_UE_STOP_CON;
    }
  }
  if (USISR & (1 << USIDC))
    return USI_TWI_UE_DATA_COL;

  /* Write a byte */
  PORT_USI &= ~(1 << PIN_USI_SCL);         // Pull SCL LOW.
  USIDR = msg;                             // Setup data.
  USI_TWI_Master_Transfer(tempUSISR_8bit); // Send 8 bits on bus.

  /* Clock and verify (N)ACK from slave */
  DDR_USI &= ~(1 << PIN_USI_SDA); // Enable SDA as input.
  if (USI_TWI_Master_Transfer(tempUSISR_1bit) & (1 << USI_TWI_NACK_BIT)) {
    if (isAddress)
      return USI_TWI_NO_ACK_ON_ADDRESS;
    else
      return USI_TWI_NO_ACK_ON_DATA;
  }
  return USI_TWI_OK;
}

/*!
 * @brief Core function for shifting data in and out from the USI.
 * Data to be sent has to be placed into the USIDR prior to calling
 * this function. Data read, will be return'ed from the function.
 * @param temp Temporary value to set the USISR
 * @return Returns the value read from the device
 */
unsigned char USI_TWI_Master_Transfer(unsigned char temp) {
  USISR = temp;                          // Set USISR according to temp.
                                         // Prepare clocking.
  temp = (0 << USISIE) | (0 << USIOIE) | // Interrupts disabled
         (1 << USIWM1) | (0 << USIWM0) | // Set USI in Two-wire mode.
         (1 << USICS1) | (0 << USICS0) |
         (1 << USICLK) | // Software clock strobe as source.
         (1 << USITC);   // Toggle Clock Port.
  do {
    USICR = temp; // Generate positve SCL edge.
    while (!(PIN_USI & (1 << PIN_USI_SCL)))
      ; // Wait for SCL to go high.
    USICR = temp;                     // Generate negative SCL edge.
    while ((PIN_USI & (1 << PIN_USI_SCL)))
      ; // Wait for SCL to go low.
  } while (!(USISR & (1 << USIOIF))); // Check for transfer complete.

  temp = USIDR;                  // Read out data.
  USIDR = 0xFF;                  // Release SDA.
  DDR_USI |= (1 << PIN_USI_SDA); // Enable SDA as output.

  return temp; // Return the data from the USIDR
}
/*!
 * @brief Function for generating a TWI Start Condition.
 * @return Returns USI_TWI_OK if the signal can be verified, otherwise returns error code.
 */
USI_TWI_ErrorLevel USI_TWI_Master_Start() {
  /* Release SCL to ensure that (repeated) Start can be performed */
  PORT_USI |= (1 << PIN_USI_SCL); // Release SCL.
  while (!(PORT_USI & (1 << PIN_USI_SCL)))
    ; // Verify that SCL becomes high.

  /* Generate Start Condition */
  PORT_USI &= ~(1 << PIN_USI_SDA); // Force SDA LOW.
  delay_cycles(tHSTART);
  PORT_USI &= ~(1 << PIN_USI_SCL); // Pull SCL LOW.
  PORT_USI |= (1 << PIN_USI_SDA);  // Release SDA.

  if (!(USISR & (1 << USISIF))) {
    return USI_TWI_MISSING_START_CON;
  }

  return USI_TWI_OK;
}
/*!
 * @brief Function for generating a TWI Stop Condition. Used to release
 * the TWI bus.
 * @return Returns USI_TWI_OK if it was successful, otherwise returns error code.
 */
USI_TWI_ErrorLevel USI_TWI_Master_Stop() {
  PORT_USI &= ~(1 << PIN_USI_SDA); // Pull SDA low.
  PORT_USI |= (1 << PIN_USI_SCL);  // Release SCL.
  while (!(PIN_USI & (1 << PIN_USI_SCL)))
    ; // Wait for SCL to go high.
  delay_cycles(tSSTOP);
  PORT_USI |= (1 << PIN_USI_SDA); // Release SDA.
  while ((PIN_USI & (1 << PIN_USI_SDA)))
    ; // Wait for SDA to go low.
  delay_cycles(tIDLE);

  if (!(USISR & (1 << USIPF))) {
    return USI_TWI_MISSING_STOP_CON;
  }

  return USI_TWI_OK;
}

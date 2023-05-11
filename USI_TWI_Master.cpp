/*!
 * @file USI_TWI_Master.cpp
 */
/*****************************************************************************
* Based on https://github.com/adafruit/TinyWireM
****************************************************************************/
#include "USI_TWI_Master.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

static unsigned char USI_TWI_Master_Transfer(unsigned char);
static USI_TWI_ErrorLevel USI_TWI_Master_Start();
static USI_TWI_ErrorLevel USI_TWI_Master_Stop();

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
 * @brief USI Transmit and receive function. LSB of first byte in buffer
 * indicates if a read or write cycles is performed. If set a read
 * operation is performed.
 *
 * Function generates (Repeated) Start Condition, sends address and
 * R/W, Reads/Writes Data, and verifies/sends ACK.
 *
 * This function also handles Random Read function if the memReadMode
 * bit is set. In that case, the function will:
 * The address in memory will be the second
 * byte and is written *without* sending a STOP.
 * Then the Read bit is set (lsb of first byte), the byte count is
 * adjusted (if needed), and the function function starts over by sending
 * the slave address again and reading the data.
 *
 * Success or error code is returned. Error codes are defined in
 * USI_TWI_Master.h
 * @param msg Pointer to the location of the msg buffer
 * @param msgSize How much data to send from the buffer
 */
USI_TWI_ErrorLevel USI_TWI_Master_Transmit(unsigned char * const in_msg,
                                           unsigned char const in_msgSize) {
  unsigned char const tempUSISR_8bit =
      (1 << USISIF) | (1 << USIOIF) | (1 << USIPF) |
      (1 << USIDC) |    // Prepare register value to: Clear flags, and
      (0x0 << USICNT0); // set USI to shift 8 bits i.e. count 16 clock edges.
  unsigned char const tempUSISR_1bit =
      (1 << USISIF) | (1 << USIOIF) | (1 << USIPF) |
      (1 << USIDC) |    // Prepare register value to: Clear flags, and
      (0xE << USICNT0); // set USI to shift 1 bit i.e. count 2 clock edges.

#ifdef NOISE_TESTING                                // Test if any unexpected conditions have arrived prior to this execution.
  if (USISR & (1<<USISIF))
    return USI_TWI_UE_START_CON;
  if (USISR & (1<<USIPF))
    return USI_TWI_UE_STOP_CON;
  if (USISR & (1<<USIDC))
    return USI_TWI_UE_DATA_COL;
#endif

  unsigned char *msg = in_msg;
  unsigned char msgSize = in_msgSize;
  unsigned char addressMode         = true;   // Always true for195 first byte
  unsigned char masterWriteDataMode = !(*msg & (1 << USI_TWI_READ_BIT));

  USI_TWI_ErrorLevel el = USI_TWI_Master_Start();
  if (el) return el;

  /*Write address and Read/Write data */
  do {
    /* If masterWrite cycle (or inital address tranmission)*/
    if (addressMode || masterWriteDataMode) {
      /* Write a byte */
      PORT_USI &= ~(1 << PIN_USI_SCL);         // Pull SCL LOW.
      USIDR = *(msg++);                        // Setup data.
      USI_TWI_Master_Transfer(tempUSISR_8bit); // Send 8 bits on bus.

      /* Clock and verify (N)ACK from slave */
      DDR_USI &= ~(1 << PIN_USI_SDA); // Enable SDA as input.
      if (USI_TWI_Master_Transfer(tempUSISR_1bit) & (1 << USI_TWI_NACK_BIT)) {
        if (addressMode)
          return USI_TWI_NO_ACK_ON_ADDRESS;
        else
          return USI_TWI_NO_ACK_ON_DATA;
      }

      addressMode = false;            // Only perform address transmission once.
    }
    /* Else masterRead cycle*/
    else {
      /* Read a data byte */
      DDR_USI &= ~(1 << PIN_USI_SDA); // Enable SDA as input.
      *(msg++) = USI_TWI_Master_Transfer(tempUSISR_8bit);

      /* Prepare to generate ACK (or NACK in case of End Of Transmission) */
      if (msgSize == 1) // If transmission of last byte was performed.
      {
        USIDR = 0xFF; // Load NACK to confirm End Of Transmission.
      } else {
        USIDR = 0x00; // Load ACK. Set data register bit 7 (output for SDA) low.
      }
      USI_TWI_Master_Transfer(tempUSISR_1bit); // Generate ACK/NACK.
    }
  } while (--msgSize); // Until all data sent/received.
  return USI_TWI_Master_Stop();
}

/*!
 * @brief Core function for shifting data in and out from the USI.
 * Data to be sent has to be placed into the USIDR prior to calling
 * this function. Data read, will be return'ed from the function.
 * @param temp Temperature to set the USISR
 * @return Returns the temp read from the device
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
    //_delay_us(T2_TWI);
    USICR = temp; // Generate positve SCL edge.
    while (!(PIN_USI & (1 << PIN_USI_SCL)))
      ; // Wait for SCL to go high.
    //_delay_us(T4_TWI);
    USICR = temp;                     // Generate negative SCL edge.
  } while (!(USISR & (1 << USIOIF))); // Check for transfer complete.

  //_delay_us(T2_TWI);
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
  //_delay_us(T2_TWI);

  /* Generate Start Condition */
  PORT_USI &= ~(1 << PIN_USI_SDA); // Force SDA LOW.
  //_delay_us(T4_TWI);
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
  //_delay_us(T4_TWI);
  PORT_USI |= (1 << PIN_USI_SDA); // Release SDA.
  _delay_us(T2_TWI);

  if (!(USISR & (1 << USIPF))) {
    return USI_TWI_MISSING_STOP_CON;
  }

  return USI_TWI_OK;
}

/*****************************************************************************
  Based on https://github.com/adafruit/TinyWireM.
  Templatized part of USI_TWI_Master.cpp
****************************************************************************/
#include <avr/interrupt.h>
#include <avr/io.h>

template <typename Device>
static unsigned char USI_TWI_Master_Transfer(unsigned char);

template <int N>
static unsigned char constexpr prepUSISR() {
  return (1 << USISIF) | (1 << USIOIF) | (1 << USIPF) |
         (1 << USIDC) |             // Prepare register value to: Clear flags, and
         ((16 - 2 * N) << USICNT0); // set USI to shift N bits i.e. count 2 * N clock edges.
}
static unsigned char constexpr tempUSISR_1bit = prepUSISR<1>();
static unsigned char constexpr tempUSISR_8bit = prepUSISR<8>();

template <typename Device>
USI_TWI_ErrorLevel USI_TWI_Master_Receive(unsigned char* buf, unsigned char len) {
  auto err = USI_TWI_Master_Start<Device>();
  if (err) return err;
  err = USI_TWI_Master_Transmit<Device>(USI_TWI_Prefix(USI_TWI_RCVE, Device::ADDRESS), true);
  if (err) return err;

  while (len > 0) {
    --len;

    /* Read a data byte */
    DDR_USI &= ~(1 << PIN_USI_SDA); // Enable SDA as input.
    auto received = USI_TWI_Master_Transfer<Device>(tempUSISR_8bit);
    *(buf++) = received;

    /* Prepare to generate ACK (or NACK in case of End Of Transmission) */
    if (len == 0) // If transmission of last byte was performed.
    {
      USIDR = 0xFF; // Load NACK to confirm End Of Transmission.
    } else {
      USIDR = 0x00; // Load ACK. Set data register bit 7 (output for SDA) low.
    }
    USI_TWI_Master_Transfer<Device>(tempUSISR_1bit); // Generate ACK/NACK.
  }
  return USI_TWI_Master_Stop<Device>();
}

/*!
 * @brief USI Transmit function.
 *
 * @param isAddress Whether this is the first byte to be transmitted after the start.
 * @return Returns the value read from the device
 * Success or error code is returned. Error codes are defined in
 * USI_TWI_Master.h
 */
template <typename Device>
USI_TWI_ErrorLevel USI_TWI_Master_Transmit(unsigned char const msg, bool const isAddress) {
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
  USI_TWI_Master_Transfer<Device>(tempUSISR_8bit); // Send 8 bits on bus.

  /* Clock and verify (N)ACK from slave */
  DDR_USI &= ~(1 << PIN_USI_SDA); // Enable SDA as input.
  auto received = USI_TWI_Master_Transfer<Device>(tempUSISR_1bit);
  if (received & (1 << USI_TWI_NACK_BIT)) {
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
 * @param temp Temporary value for the USISR
 * @return Returns the value read from the device
 */
template <typename Device>
unsigned char USI_TWI_Master_Transfer(unsigned char temp) {
  USISR = temp;                          // Set USISR according to temp.
                                         // Prepare clocking.
  temp = (0 << USISIE) | (0 << USIOIE) | // Interrupts disabled
         (1 << USIWM1) | (0 << USIWM0) | // Set USI in Two-wire mode.
         (1 << USICS1) | (0 << USICS0) |
         (1 << USICLK) | // Software clock strobe as source.
         (1 << USITC);   // Toggle Clock Port.
  do {
    Device::tPRE_SCL_HIGH.wait();
    USICR = temp; // Generate positve SCL edge.
    while (!(PIN_USI & (1 << PIN_USI_SCL)))
      ; // Wait for SCL to go high.
    Device::tPOST_SCL_HIGH.wait();
    USICR = temp;                     // Generate negative SCL edge.
  } while (!(USISR & (1 << USIOIF))); // Check for transfer complete.

  Device::tPOST_TRANSFER.wait();
  temp = USIDR;                  // Read out data.
  USIDR = 0xFF;                  // Release SDA.
  DDR_USI |= (1 << PIN_USI_SDA); // Enable SDA as output.

  return temp; // Return the data from the USIDR
}

/*!
 * @brief Function for generating a TWI Start Condition.
 * @return Returns USI_TWI_OK if the signal can be verified, otherwise returns error code.
 */
template <typename Device>
USI_TWI_ErrorLevel USI_TWI_Master_Start() {
  /* Release SCL to ensure that (repeated) Start can be performed */
  PORT_USI |= (1 << PIN_USI_SCL); // Release SCL.
  unsigned char counter = 0;
  while (!(PORT_USI & (1 << PIN_USI_SCL))) {
    ; // Verify that SCL becomes high.
    if (++counter == 0) {
      return USI_TWI_NO_SCL_HI;
    }
  }

  /* Generate Start Condition */
  PORT_USI &= ~(1 << PIN_USI_SDA); // Force SDA LOW.
  Device::tHSTART.wait();
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
template <typename Device>
USI_TWI_ErrorLevel USI_TWI_Master_Stop() {
  PORT_USI &= ~(1 << PIN_USI_SDA); // Pull SDA low.
  PORT_USI |= (1 << PIN_USI_SCL);  // Release SCL.
  unsigned char counter = 0;
  while (!(PIN_USI & (1 << PIN_USI_SCL))) {
    ; // Wait for SCL to go high.
    if (++counter == 0) {
      return USI_TWI_NO_SCL_HI;
    }
  }
  Device::tSSTOP.wait();
  PORT_USI |= (1 << PIN_USI_SDA); // Release SDA.
  Device::tIDLE.wait();

  if (!(USISR & (1 << USIPF))) {
    return USI_TWI_MISSING_STOP_CON;
  }

  return USI_TWI_OK;
}

/*****************************************************************************
*
*
* File              USI_TWI_Master.c compiled with gcc
* Date              Friday, 10/31/08
* Updated by        jkl
*

* AppNote           : AVR310 - Using the USI module as a TWI Master
*
*		Extensively modified to provide complete I2C driver.
*
*Notes:
*		- T4_TWI and T2_TWI delays are modified to work with 1MHz default clock
*			and now use hard code values. They would need to change
*			for other clock rates. Refer to the Apps Note.
*
*	12/17/08	Added USI_TWI_Start_Memory_Read Routine		-jkl
*		Note msg buffer will have slave adrs ( with write bit set) and memory adrs;
*			length should be these two bytes plus the number of bytes to read.
****************************************************************************/
#include <avr/interrupt.h>
//#define F_CPU 1000000UL	      // Sets up the default speed for delay.h
//#define F_CPU 8000000UL	      // Sets up the default speed for delay.h

//#include <util/delay.h>
#include "USI_TWI_Master.h"

// For use with delayMicroseconds()
//#define T2_TWI    5     // >4,7us
//#define T4_TWI    4     // >4,0us

static unsigned char USI_TWI_Master_Transfer(unsigned char);
static unsigned char USI_TWI_Master_Stop();
static unsigned char USI_TWI_Master_Start();

/*---------------------------------------------------------------
 USI TWI single master initialization function
---------------------------------------------------------------*/
void USI_TWI_Master_Initialise() {
  PORT_USI |= (1<<PIN_USI_SDA);           // Enable pullup on SDA, to set high as released state.
  PORT_USI |= (1<<PIN_USI_SCL);           // Enable pullup on SCL, to set high as released state.

  DDR_USI  |= (1<<PIN_USI_SCL);           // Enable SCL as output.
// pinMode(PIN_USI_SCL, OUTPUT);
  DDR_USI  |= (1<<PIN_USI_SDA);           // Enable SDA as output.
// pinMode(PIN_USI_SDA, OUTPUT);
  USIDR    =  0xFF;                       // Preload dataregister with "released level" data.
  USICR    =  (0<<USISIE)|(0<<USIOIE)|                            // Disable Interrupts.
              (1<<USIWM1)|(0<<USIWM0)|                            // Set USI in Two-wire mode.
              (1<<USICS1)|(0<<USICS0)|(1<<USICLK)|                // Software stobe as counter clock source
              (0<<USITC);
  USISR   =   (1<<USISIF)|(1<<USIOIF)|(1<<USIPF)|(1<<USIDC)|      // Clear flags,
              (0x0<<USICNT0);                                     // and reset counter.
}

/*---------------------------------------------------------------
 USI Normal Read / Write Function
 Transmit and receive function. LSB of first byte in buffer
 indicates if a read or write cycles is performed. If set a read
 operation is performed.

 Function generates (Repeated) Start Condition, sends address and
 R/W, Reads/Writes Data, and verifies/sends ACK.

 Success or error code is returned. Error codes are defined in
 USI_TWI_Master.h
---------------------------------------------------------------*/
unsigned char USI_TWI_Start_Read_Write(unsigned char * const in_msg, unsigned char const in_msgSize) {
  unsigned char const tempUSISR_8bit = (1<<USISIF)|(1<<USIOIF)|(1<<USIPF)|(1<<USIDC)|      // Prepare register value to: Clear flags, and
                                 (0x0<<USICNT0);                                     // set USI to shift 8 bits i.e. count 16 clock edges.
  unsigned char const tempUSISR_1bit = (1<<USISIF)|(1<<USIOIF)|(1<<USIPF)|(1<<USIDC)|      // Prepare register value to: Clear flags, and
                                 (0xE<<USICNT0); 									// set USI to shift 1 bit i.e. count 2 clock edges.
#ifdef PARAM_VERIFICATION
  if (in_msg > (unsigned char*)RAMEND)                 // Test if address is outside SRAM space
    return USI_TWI_DATA_OUT_OF_BOUND;
  if (in_msgSize <= 1)                                 // Test if the transmission buffer is empty
    return USI_TWI_NO_DATA;
#endif

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
  unsigned char addressMode         = true;   // Always true for first byte
  unsigned char masterWriteDataMode = !(*msg & (1<<TWI_READ_BIT)); // The LSB in the address byte determines if is a masterRead or masterWrite operation.

  unsigned char rc = USI_TWI_Master_Start();
  if (rc)
    return rc;

/*Write address and Read/Write data */
  do {
    /* If masterWrite cycle (or inital address tranmission)*/
    if (addressMode || masterWriteDataMode) {
      /* Write a byte */
      PORT_USI &= ~(1<<PIN_USI_SCL);                // Pull SCL LOW.
      USIDR     = *(msg++);                         // Setup data.
      USI_TWI_Master_Transfer(tempUSISR_8bit);      // Send 8 bits on bus.

      /* Clock and verify (N)ACK from slave */
      DDR_USI  &= ~(1<<PIN_USI_SDA);                // Enable SDA as input.
//pinMode(PIN_USI_SDA, INPUT_PULLUP);
      if (USI_TWI_Master_Transfer(tempUSISR_1bit) & (1<<TWI_NACK_BIT)) {
        if (addressMode)
          return USI_TWI_NO_ACK_ON_ADDRESS;
        else
          return USI_TWI_NO_ACK_ON_DATA;
      }

      addressMode = false;            // Only perform address transmission once.
    } else { /* masterRead cycle*/
      /* Read a data byte */
      DDR_USI   &= ~(1<<PIN_USI_SDA);               // Enable SDA as input.
//pinMode(PIN_USI_SDA, INPUT_PULLUP);
      *(msg++)  = USI_TWI_Master_Transfer(tempUSISR_8bit);

      /* Prepare to generate ACK (or NACK in case of End Of Transmission) */
      if (msgSize == 1)                             // If transmission of last byte was performed.
        USIDR = 0xFF;                               // Load NACK to confirm End Of Transmission.
      else
        USIDR = 0x00;                               // Load ACK. Set data register bit 7 (output for SDA) low.
      USI_TWI_Master_Transfer(tempUSISR_1bit);      // Generate ACK/NACK.
    }
  } while (--msgSize);                              // Until all data sent/received.
  return USI_TWI_Master_Stop();
}

/*---------------------------------------------------------------
 Core function for shifting data in and out from the USI.
 Data to be sent has to be placed into the USIDR prior to calling
 this function. Data read, will be return'ed from the function.
---------------------------------------------------------------*/
unsigned char USI_TWI_Master_Transfer(unsigned char temp) {
  USISR = temp;                                     // Set USISR according to temp.
                                                    // Prepare clocking.
  temp  =  (0<<USISIE)|(0<<USIOIE)|                 // Interrupts disabled
           (1<<USIWM1)|(0<<USIWM0)|                 // Set USI in Two-wire mode.
           (1<<USICS1)|(0<<USICS0)|(1<<USICLK)|     // Software clock strobe as source.
           (1<<USITC);                              // Toggle Clock Port.
  do {
    //delayMicroseconds(T2_TWI);
    USICR = temp;                           // Generate positve SCL edge.
    while (!(PIN_USI & (1<<PIN_USI_SCL)));  // Wait for SCL to go high.
    //delayMicroseconds(T4_TWI);
    USICR = temp;                           // Generate negative SCL edge.
  } while(!(USISR & (1<<USIOIF)));          // Check for transfer complete.

  //delayMicroseconds(T2_TWI);
  temp  = USIDR;                           // Read out data.
  USIDR = 0xFF;                            // Release SDA.
  DDR_USI |= (1<<PIN_USI_SDA);             // Enable SDA as output.
//pinMode(PIN_USI_SDA, OUTPUT);
  return temp;                             // Return the data from the USIDR
}
/*---------------------------------------------------------------
 Function for generating a TWI Start Condition.
---------------------------------------------------------------*/
unsigned char USI_TWI_Master_Start() {
/* Release SCL to ensure that (repeated) Start can be performed */
  PORT_USI |= (1<<PIN_USI_SCL);                     // Release SCL.
  while( !(PORT_USI & (1<<PIN_USI_SCL)) );          // Verify that SCL becomes high.
  //delayMicroseconds(T2_TWI);

/* Generate Start Condition */
  PORT_USI &= ~(1<<PIN_USI_SDA);                    // Force SDA LOW.
  //delayMicroseconds(T4_TWI);
  PORT_USI &= ~(1<<PIN_USI_SCL);                    // Pull SCL LOW.
  PORT_USI |= (1<<PIN_USI_SDA);                     // Release SDA.

  if (!(USISR & (1<<USISIF)))
    return USI_TWI_MISSING_START_CON;
  return 0;
}
/*---------------------------------------------------------------
 Function for generating a TWI Stop Condition. Used to release
 the TWI bus.
---------------------------------------------------------------*/
unsigned char USI_TWI_Master_Stop() {
  PORT_USI &= ~(1<<PIN_USI_SDA);           // Pull SDA low.
  PORT_USI |= (1<<PIN_USI_SCL);            // Release SCL.
  while( !(PIN_USI & (1<<PIN_USI_SCL)) );  // Wait for SCL to go high.
  //delayMicroseconds(T4_TWI);
  PORT_USI |= (1<<PIN_USI_SDA);            // Release SDA.
  //delayMicroseconds(T2_TWI);

  if (!(USISR & (1<<USIPF)))
    return USI_TWI_MISSING_STOP_CON;
  else 
    return 0;
}

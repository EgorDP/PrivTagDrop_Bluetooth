#include <sam.h>
#include "i2c.h"

/* NOTE
 *  - Accelerometer is BA253
 */

void endCondition(int dir){ // Stop condition for reads only
    if(dir == 0){ // Write stop condition
       SERCOM3 -> I2CM.CTRLB.bit.CMD = 0x3;                    // Issue a stop condition to terminate 
       while(SERCOM3 -> I2CM.SYNCBUSY.bit.SYSOP == 1);         // Wait for SYNCBUSY to synchronize the stop condition
    }else if(dir == 1){ // Read stop condition
      SERCOM3 -> I2CM.CTRLB.bit.ACKACT = 1;                   // Send a NACK
      SERCOM3 -> I2CM.CTRLB.bit.CMD = 0x3;                    // Issue a stop condition to terminate 
      while(SERCOM3 -> I2CM.SYNCBUSY.bit.SYSOP == 1);         // Wait for SYNCBUSY to synchronize the stop condition
    }else if(dir == 2){ // Repeated start condition
      SERCOM3 -> I2CM.CTRLB.bit.CMD = 0x1;     
      while(SERCOM3 -> I2CM.SYNCBUSY.bit.SYSOP == 1);  
    }
}

void i2c_init(){
  /**
   * Multiplexing Pins
   * These two pins from Processor must be multiplexed to the accelerometer:
   * PA22 corresponds to AD4/SDA on accelerometer, will select SERCOM3.0
   * PA23 corresponds to AD5/SCL on accelerometer, will select SERCOM3.1
   */
  PORT -> Group[0].PINCFG[22].bit.PMUXEN = 0x1;                       // Enable peripheral functions for PA22, pin corresponds to 22th PINCFG group
  PORT -> Group[0].PINCFG[23].bit.PMUXEN = 0x1;                       // Enable peripheral functions for PA23, pin corresponds to 23rd PINCFG groupx`

  PORT -> Group[0].PMUX[11].bit.PMUXE = PORT_PMUX_PMUXE_C_Val;    // Function C selects SERCOM3 pad 0 for PA22, pin corresponds to 11th PMUX group even bit
  PORT -> Group[0].PMUX[11].bit.PMUXO = PORT_PMUX_PMUXE_C_Val;    // Function C selects SERCOM3 pad 1 for PA23, pin corresponds to 11th PMUX group odd bit  

  /* Setup the generic clock 1 generator */ 
  /*
  SYSCTRL -> OSC8M.bit.PRESC  = 0;                                // No prescaler on clock source OSC8M
  SYSCTRL -> OSC8M.bit.ENABLE = 1;                                // Enable clock source
  GCLK -> GENDIV.bit.ID     = 0x01;                               // Configure clock generator #1
  GCLK -> GENDIV.bit.DIV    = 0X0;                                // No prescaler clock generator #1
  GCLK -> GENCTRL.bit.SRC   = GCLK_GENCTRL_SRC_OSC8M_Val;         // Connect clock source to clock generator #1
  GCLK -> GENCTRL.bit.GENEN = 1;                                  // Enable clock generator #1
  //GLOCK3 = 8mhz
  */
  /* Initialize SERCOM3 with generic clock 1 generator */
  GCLK -> CLKCTRL.bit.ID    = GCLK_CLKCTRL_ID_SERCOM3_CORE_Val;   // Select SERCOM3 channel
  GCLK -> CLKCTRL.bit.GEN   = GCLK_CLKCTRL_GEN_GCLK3_Val;         // Select clock generator #3  
  GCLK -> CLKCTRL.bit.CLKEN = 1;                                  // Clock enable

  /* Configure synchronous bus clock */
  PM -> APBCSEL.bit.APBCDIV   = 0;                                // Select bit 0, corresponds to no prescaler (division by 1)
  PM -> APBCMASK.bit.SERCOM3_ = 0x1;                              // Enable the SERCOM3 interface
  
  /* Initialization procedures for I^2C */
  SERCOM3 -> I2CM.CTRLA.bit.MODE     = 0x5;                        // Select I^2C master mode by writing 0x5 
  SERCOM3 -> I2CM.CTRLA.bit.INACTOUT = 0x0;                        // Disable the inactive bus-timeout 
  

/**
 * Configuring BAUD rate
 *   1. Frequency of generick clock 1 is: 8Mhz
 *   2. BAUD is set to 0xE (14) in the BAUD.bit.BAUD register, this was because the value 290,000 Hz for 
 *      the frequency of BAUD was initially selected, but this required a 13.79 BAUD value, so it was ceiled      
 *      instead to 14 which gives a slightly lower but acceptble frequency of BAUD
 *  Frequency of BAUD is calculated as (frequency of clock)/(2 * BAUD + 1)
 *  Frequency of BAUD = 8,000,000 / 2(25 + 1) = 153,846
 *  
 */
  SERCOM3 -> I2CM.BAUD.bit.BAUD = 0x19;                       // Set the BAUD
  //SERCOM3 -> I2CM.CTRLA.bit.SCLSM  = 0x0;                    // Do not enable clock stretching          
  SERCOM3 -> I2CM.CTRLB.bit.SMEN   = 0x1;                    // Enable smart mode  
   
  SERCOM3 -> I2CM.CTRLA.bit.ENABLE = 0x1;                     // Enable I2C peripheral 
  while(SERCOM3 -> I2CM.SYNCBUSY.bit.ENABLE == 1);            // Wait for SYNCBUSY to synchronize the enable 

  SERCOM3 -> I2CM.INTENSET.bit.MB = 1;                   // Master on bus interrupt enable so that it can set intflags
  SERCOM3 -> I2CM.INTENSET.bit.SB = 1;                   // Slave on bus interrupt enable so that it can set intflags   

  SERCOM3 -> I2CM.STATUS.bit.BUSSTATE = 0b01;                  // Force bus state idle (0b01) to transition from the initial bus state unknown
  while(SERCOM3 -> I2CM.SYNCBUSY.bit.SYSOP == 1);              // Wait for SYNCBUSY to synchronize the busstate 
}






/*
 * Send data over I2C 
 * @param address Slave address to transmit to
 * @param dir  Single bit indicates write + read (1) or write (0) to slave
 * @param data Data to transmit
 * @param len  Length of data to transmit in bits 
 */
uint8_t i2c_transaction(uint8_t address, uint8_t dir, uint8_t* data, uint8_t len){

int writeCount = 0; // Itterator amount of bytes written
int readCount = 0;  // Itterator amount of bytes read
int amountWrite = 2; 
if(dir == 1){
  amountWrite = 1;
}

/* Transaction initilization */
SERCOM3 -> I2CM.ADDR.bit.ADDR = (0x19 << 1) | 0; // Set slave address and direction bit (read or write)
while(SERCOM3 -> I2CM.SYNCBUSY.bit.SYSOP == 1);
// ADDR.bit.ADDR generates a start condition internally 
 
while( SERCOM3 -> I2CM.STATUS.bit.RXNACK != 0);       // Wait for acknowledgement of address receival
while(!(SERCOM3 -> I2CM.INTFLAG.bit.MB) ); // Wait for writing flag to be set s
 
 
  
while(writeCount < amountWrite){   // Continue writing bytes while current byte is less than the length
    // DATA.bit.DATA also sets the MB flag
    SERCOM3 -> I2CM.DATA.bit.DATA = *data;   // Write 1 byte
    while(SERCOM3 -> I2CM.SYNCBUSY.bit.SYSOP == 1);
    writeCount++;         // Add 1 byte to current length 
    data++;          // Move address over to the next byte  
    while(SERCOM3 -> I2CM.STATUS.bit.RXNACK != 0); // Check ACK for each data transmission
}
 

if(dir == 0){
    endCondition(0); // If only writing, end transaction
    return writeCount;         
}else{               // Otherwise send a repeated start condition
    SERCOM3 -> I2CM.CTRLB.bit.CMD = 0x1;     
    while(SERCOM3 -> I2CM.SYNCBUSY.bit.SYSOP == 1);  
}



SERCOM3 -> I2CM.ADDR.bit.ADDR = ((address << 1) |  1); // Set slave address and direction bit (read or write)
while(SERCOM3 -> I2CM.SYNCBUSY.bit.SYSOP == 1); // Sync it


  
while( SERCOM3 -> I2CM.STATUS.bit.RXNACK != 0);       // Wait for acknowledgement of address receival



while(!(SERCOM3 -> I2CM.INTFLAG.bit.SB)); // Wait for flag to be set if writing
    while(readCount < len){
        
        *data = SERCOM3 -> I2CM.DATA.bit.DATA; 
        
        while(SERCOM3 -> I2CM.SYNCBUSY.bit.SYSOP == 1);
        //if(*data == 250){ // Test for chip ID if necessary
            //delay(500);
            // printf("Chip ID is: %d \n",*data);
        //}
        data++;
        readCount++;
        
        while(SERCOM3 -> I2CM.STATUS.bit.RXNACK != 0); // Check ACK for each data transmission
    }
    endCondition(1);

  SERCOM3 -> I2CM.STATUS.bit.BUSSTATE = 0x01; // Transaction performed successfully, set BUS to idle
  return writeCount + readCount; // Return the amount of bytes written and read
}   

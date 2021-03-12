#include <sam.h>
#include "bma250.h"
#include "i2c.h"

/* NOTE
 *  - Accelerometer is BA253
 */

/*
 * Initialize the BMA253 with: 
 * g range setting +2g, this is because no throws or acceleration higher than 1g is performed
 * filtered data collection at 7.81Hz with update time of 64ms to quickly detect a drop 
 */
void bma250_init(){
  uint8_t writeFiltered[2] = {0x10,0b01000};                     // 0x10 indicates PMU_BW bandwith register in BMA250, 0b1011 sets 7.81Hz with 64ms update time
  uint8_t* pointerFiltered = &writeFiltered[0];                 // Create pointer to array
  i2c_transaction(0x19, 0, pointerFiltered, 1);                 // Write the filtered data setting to the accelerometer

  uint8_t writeMeasurementRange[2] = {0x0F,0x03};               // 0x0F indicates PMU_RANGE register in BMA250, 0x03 sets +2g for the accelerometer g range
  uint8_t* pointerMeasurementRange = &writeMeasurementRange[0]; // Create pointer to array
  i2c_transaction(0x19, 0, pointerMeasurementRange, 1);         // Write g range setting to the accelerometer

}

/*
 * Reads in acceleration values from the BMA253 device (NOT THE BMA250!)
 * @param x represents pointer for vertical direction
 * @param y represents pointer for irrelevant horizontal direction (not used in program)
 * @param z represents pointer for irrelevant horizontal direction (not used in program)
 */
void bma250_read_xyz(int16_t* x, int16_t* y, int16_t* z){
   /* writeAddress is an array of registers addresses that will be read from
   * 0x02 : x axis LSB bits, 0x03 : x axis MSB bits 
   * 0x04 : y axis LSB bits, 0x05 : y axis MSB bits
   * 0x06 : z axis LSB bits, 0x07 : z axis MSB bits
   */
  uint8_t xArr[2] = {0x03,0};
  uint8_t *xPoint = &xArr[0];
  uint8_t yArr[2] = {0x05,0};
  uint8_t *yPoint = &yArr[0];
  uint8_t zArr[2] = {0x07,0};
  uint8_t *zPoint = &zArr[0];
  i2c_transaction(0x19, 1, xPoint, 1); // Transactions to get data from BMA253
  i2c_transaction(0x19, 1, yPoint, 1);
  i2c_transaction(0x19, 1, zPoint, 1);

  /* Read only the most significant bits, precision with the least significant bits is not required */
  *x = xArr[1];     // Capture vertical acceleration
  *y = yArr[1];     // Irrelevant capture
  *z = zArr[1];     // Irrelevant capture
  
}

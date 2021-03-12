#ifndef I2C_HEADER
#define I2C_HEADER

void i2c_init();

uint8_t i2c_transaction(uint8_t address, uint8_t dir, uint8_t* data, uint8_t len);


#endif

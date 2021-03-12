#ifndef BMA250_H
#define BMA250_H

void bma250_init();

void bma250_read_xyz(int16_t* x, int16_t* y, int16_t* z); 

#endif

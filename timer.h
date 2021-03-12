#ifndef TIMER_H
#define TIMER_H
void timer5_init();

void timer5_reset();

void timer5_set(uint16_t period_ms);

void TC5_Handler();



#endif

#include <sam.h>
#include <core_cm0plus.h>
#include "timer.h"




volatile int count = 0;


/**
 * Resets the timer using PRESYNC pg 572
 */

/**
 * Reset the timer
 */
void timer3_reset(){
  
  TC3 -> COUNT16.CTRLA.reg = (1<<12);                            // Bit 12 in CTRLA causes a reset for the counter
}

/** 
 *  @Override the Handler function
 */
 
void TC3_Handler(){ 
  timerEnact = true;

  timerFallCheck = true;
  globalCount++;
  timeElapsed++;
  TC3 -> COUNT16.INTFLAG.reg |= 0b00010000;                       // Clear the interrupt flag to rearm for next cycle
}

/**
 * Sets the period that the timer fires for, additionally flooring them so no floating point libraries are added
 *    For example, compare value for 50 ms @ 32.768kHz -> 32,768 / 20 = 1638.4 so it will fire 20 times/sec
 *    Truncates to 1638 which is acceptable in our non-criticle application
 * @param period_ms time in milliseconds that the timer should fire for
 */
 
void timer3_set(uint16_t period_ms){                                                          
  TC3 -> COUNT16.CC[0].reg = (int)(32.768 * period_ms);            // Controlled by CC0 register on Match Frequency Generation
}


/**
 * Initialize the timer with a generic clock, bus clock, and set count, wave, prescaler and interrupts 
 * Correlates to initialization steps provided in the SAMD datasheet
 */
 
void timer3_init(){
    // Step 1: Initialize timer with generic clock 1
    GCLK -> CLKCTRL.reg = GCLK_CLKCTRL_ID_TCC2_TC3_Val;             // select TC3 clock
    GCLK -> CLKCTRL.reg |= GCLK_CLKCTRL_GEN_GCLK1;                  // select clock 1
    GCLK -> CLKCTRL.bit.CLKEN = 1;                                  // enable TC3 clock
    
    // Step 2: Configure synchronous bus clock
    PM -> APBCSEL.bit.APBCDIV = 0;                                  // no prescaler from bus necessary
    PM -> APBCMASK.bit.TC3_ = 1;                                    // enable the TC3 interface
    
    // Step 3: Configure Count Mode (16-bit)
    TC3 -> COUNT16.CTRLA.bit.MODE = 0;                              // select mode 0 to use 16 bit counting register                                
    
    // Step 4: no Prescaler from CTRLA in the clock itself necessary

    
    // Step 5: Configure TC3 Compare Mode for compare channel 0
    TC3 -> COUNT16.CTRLA.bit.WAVEGEN = 1;                           // select match frequency mode, clock resets when hitting compare value
    
    
   
    
    // Step 6: Enable TC3 compare mode interrupt generation
    TC3 -> COUNT16.INTENSET.bit.MC0 = 0x1;                          // Enable match interrupts on compare channel 0 
    
    // Step 7: Enable TC3 itself
    TC3 -> COUNT16.CTRLA.bit.ENABLE = 1;
    while(TC3 -> COUNT16.STATUS.bit.SYNCBUSY == 1);                  // wait until TC3 is enabled
   
   // Step 8: Interupt controller setup, NVIC APIs
   NVIC_SetPriority(TC3_IRQn, 3);
   NVIC_EnableIRQ(TC3_IRQn);

   // Enable the global interrupt as well
   void __enable_irq(void);
      
}

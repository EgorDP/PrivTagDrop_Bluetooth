#include <sam.h>
#include "timer.h"
#include "i2c.h"
#include <SPI.h>
#include "STBLE.h"
#include "ble.h"
#define PIPE_UART_OVER_BTLE_UART_TX_TX 0
/*
 * Manually disconnect the phone
 * 
 */

/* === DO NOT REMOVE: Initialize C library === */
extern "C" void __libc_init_array(void);

/* === Initialize Global Variables === */
volatile int globalCount = 0;         // Timer to get data from accelerometer / then takes role of timer to activate every 10 seconds to send message to BlueTooth device
volatile int timeElapsed = 0;         // Timer elapsed since device was lost in seconds
volatile bool timerEnact = false;     // Boolean to check accelerometer values, uses timer
volatile bool timerFallCheck = false; // Boolean to call detectFall() function, uses timer
bool printValues = false;             // (Feature) Print the values of x,y, and z from accelerometer
volatile bool hasFallen = false;      // Determines if the device has fallen     

/*
 * Function for enabling proper printf functionality to serial monitor
 */
extern "C" int _write(int fd, const void *buf, size_t count) {
    // STDOUT
    if (fd == 1)
      SerialUSB.write((char*)buf, count);
    return 0;
}

/*
 * Enable the BLE
 */
void ble_setup() {
  SerialUSB.begin(115200);
  while (!SerialUSB);               //This line will block until a serial monitor is opened 
  SerialUSB.print("Starting...\n");
  ble_init();
}

bool ble_serial_send_data(bool writeMode, int seconds){
  bool found = false;
  char message;
  if(ble_loop()){ //Process any ACI commands or events from the main BLE handler, must run often. Keep main loop short.
    delay(1000);
    printf("Message was 'found' exiting lost state \n");
    found = true;
  }
 
  if (SerialUSB.available()) {      //Check if serial input is available to send
    delay(10);//should catch input
    uint8_t sendBuffer[21];
    uint8_t sendLength = 0;
    
      while (SerialUSB.available() && sendLength < 19) {
         sendBuffer[sendLength] = SerialUSB.read();  
        sendLength++;
      }
    if (SerialUSB.available()) {
      SerialUSB.print(F("Input truncated, dropped: "));
      if (SerialUSB.available()) {
        SerialUSB.write(SerialUSB.read());
      } 
    }
    
    sendBuffer[sendLength] = '\0'; //Terminate string
    sendLength++;
    if (!lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, (uint8_t*)sendBuffer, sendLength))
    {
      SerialUSB.println(F("TX dropped!"));
    }
  }
  else if(writeMode){             // Check if a write is being performed 
    delay(10);//should catch input
    char sendInit[] = "Privtag lost for\0";
    if (!lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, (uint8_t*)sendInit, 18))
    {
      SerialUSB.println(F("TX dropped!"));
    }
    char sendBuffer[21];
    int digits = seconds; // Temp copy of seconds 
    int count = 0;
    do // Count up amount of digits in seconds for accurate send length
    {
        count++;
        digits /= 10;
    } while(digits != 0);
    
    snprintf( (char*)sendBuffer, sizeof(sendBuffer), " %ds", seconds);
    count++; // Account for blank space before int
    count++; // Account for letter 's'
    if (!lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, (uint8_t*)sendBuffer, count))
    {
      SerialUSB.println(F("TX dropped!"));
    }
    return found;
  
  }
}

/*
 *  METHOD FROM PROJECT 2 
 *  Detects when the device falls
 */
bool detectFall(){
  /* Create registers and respective pointer to x,y,z positions */
  int16_t xAcceleration[1];
  int16_t yAcceleration[1];
  int16_t zAcceleration[1];
  int16_t *x = &xAcceleration[0];    // Represents vertical direction
  int16_t *y = &yAcceleration[0];    // Irrelevant horizontal direction
  int16_t *z = &zAcceleration[0];    // Irrelevant horizontal direction
  
  int timeCount = 0;                  // Counts the time after device has been dropped for the LED activation
  int durationAir = 0;                // Count cycles it is in the air
  int durationLand = 0;               // Count cycles device is stationary after fall
  bool isPlaced = false;              // Determines if the device is placed
  bool stateFloatLater = false;       // Determines if device floates after picking up
  hasFallen = false;
  
  
  /*
   * How the drop detection algorithm works:
   * When stationary on the ground, x reads out values less than 50 from the register measuring vertical velocity. As soon as it moves upwards,
   * the values become over 200 at that point in the register. Then, as the device drops it again has values less than 50. These 3 changes are recorded
   * in the variables isPlaced, stateFloatLater, and stationary. There are also "durationLand" and "durationAir" measurements that make sure
   * the values in each stage are recorded at least twice before confirming the setting of the bool variables. 
   * 
   * Furthermore, a timer makes sure that the data from the accelerometer is read only every 30 milliseconds. This is done by making it switch on a 
   * bool that is turned off when the if statement containing the bma250_read function is turned off. 
   */
  /* Loop tests acceleration and turns on LEDs */
  
  while(!hasFallen){
    if(!hasFallen && timerEnact){            // While the device is at rest and a fall has not been detected
      bma250_read_xyz(x,y,z);                // Read new acceleration data at each point
      if(printValues){                       // Rough visual for values during change (due to required delay for printf functionality)
       delay(100);
        printf("x,y,z is: %d , %d , %d r\n",*x,*y,*z);
      }
        if(*x < 50){
          isPlaced = true;
          durationAir = 0;
        }
        if(isPlaced && *x > 200){             // If it had been placed now it's floating, count up the cycles
          durationAir++;
          durationLand = 0;
          if(durationAir > 1){                // 2 cycles should account for human handling to make sure it is floating
             stateFloatLater = true;
          }
        }
        if( (stateFloatLater && (*x < 50)) ){
          durationLand++;
          if(durationLand > 1){
            printf("Fall initiated");
            hasFallen = true;
          }
        }
      timerEnact = false;  
    }
 }  
 return true;                                   // Return true since fall has been detected
}

/*
 * (Feature) Function - Call the CHIP ID = 250
 * Requires some indication that it is on in the i2c transaction function definition itself
 * such as if(*data == 250) {printf("Chip ID");
 */
void callTest(){
  uint8_t testArr[3] = {0x0,0,0};
  uint8_t *testPointer = &testArr[0];
  i2c_transaction(0x19, 1, testPointer, 1);
}



int main(void) {  
  /* ==== DO NOT REMOVE: USB configuration ==== */
  init();
  __libc_init_array();
  USBDevice.init();
  USBDevice.attach();
  /* =========================================== */

  /* === DO NOT REMOVE: I2C, Timer, BMA253 configuration === */
  i2c_init();
  bma250_init(); 
  timer3_init();
  ble_setup();
  aci_hal_device_standby();     // Put BlueTooth into standby mode, no need to transmit if not lost
  /* ======================================================= */

  bool fallen = false;
  bool found = false;
  timer3_set(20);   // Activate acceleration of device check every 20 milliseconds 
  
  while(true){
    
    if(timerFallCheck){   // Timer activated boolean to check for the fall
      timerFallCheck = false; 
      if(detectFall() ){
        fallen = true;
        timer3_set(1000);   // Initiate time elapsed since drop
        delay(1000);
        printf("\n Device has fallen \n"); // Secondary notification of fall state
        globalCount = 0; // Reset timers for printing out the message to BlueTooth device
        timeElapsed = 0;
      }
    }
  
    while(fallen){ // While in the fallen state...
      found = ble_serial_send_data(false,0);    // Activate BlueTooth (from standby and from no advertisement packets
            
      if(globalCount / 10 >= 1){    // Activate message send every 10s by passing 'true' to ble_serial_send_data
        ble_serial_send_data(true, timeElapsed);
        globalCount = 0;
      }
      
      if(found){    // If it is found, deactivate 'found' mode and stop BlueTooth advertisement packets
        fallen = false;
        aci_gap_set_non_discoverable(); // Disables communication
        timer3_set(20);         // Prep timer for accelerometer checking phase
      }
      
    }
    
 }
    
  return 0;
}

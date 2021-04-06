# PrivTagDrop_Bluetooth
SAMD21 microcontroller software for detecting lost items, small enough to fit within a wallet. 

# Functionality

This product sends a Bluetooth signal to your phone using Bluetooth Low Energy indicating when the device is dropped. A user can acknowledge when they find the device and rearm the product for the next time they lose it by sending the command "found." 

# Software Contributions
Most of the generic Bluetooth software was provided by Tiny Zero. It was modified for the purposes of this project. Out of these only ble.cpp was modified.
Timer, bma250, cse190, and i2c are all original work. 


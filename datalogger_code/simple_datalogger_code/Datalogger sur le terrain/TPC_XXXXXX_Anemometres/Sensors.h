/* *******************************************************************************
code for datalogger with PCB design v2.6 There are two sensors already installed on this PCB, the SHT35 and the BMP388

There are two anemometers: the FS3000 I2C anemometer from sparkfun, which measures winf in only one direction, and the JL-FS2 mechanical anemometer from
DFrobot, which measures wind in all directions, but only if the value is higher than 0.8m/s. It unfortunately only communicates with RS485 so we need a converter from
RS485 to UART

Here are some important remarks when modifying to add or  add or remove a sensor:
    - in Sensors::getFileData () don't forget to add a "%s;" for each new sensor 
    - adjust the struct, getFileHeader (), getFileData (), serialPrint() and mesure() with the sensor used

The conf.txt file must have the following structure:
  300; //deepsleep time in seconds (put at least 120 seconds)
  1; //boolean value to choose if you want to set the RTC clock with time from SD card
  2023/03/31 16:05:00; //time to set the RTC if setRTC = 1

If you have any questions contact me per email at nicolas.schmid.6035@gmail.com
**********************************************************************************
*/ 


#ifndef sensors_h
#define sensors_h
#include "Arduino.h"

size_t readN(uint8_t *buf, size_t len);
float readWindSpeed(uint8_t Address);

class Sensors {
  public: 
    Sensors(); 
    String getFileHeader();
    String getFileData();
    String serialPrint();
    void mesure();
    void tcaselect(uint8_t i);
};

#endif

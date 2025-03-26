/* *******************************************************************************
Base code for datalogger with PCB design v2.6 There are two sensors already installed on this PCB, the SHT35 and the BMP388

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

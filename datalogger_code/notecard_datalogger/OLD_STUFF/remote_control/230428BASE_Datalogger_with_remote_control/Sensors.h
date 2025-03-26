/* *******************************************************************************
Base code for datalogger with Notecard, with only MS8067 (temp, hum, press). As new functionality we can now remotely change some parameters on the conf.txt file remotely.

This code was entirely rewritten by Nicolas Schmid. Here are some important remarks when modifying to add or  add or remove a sensor:
    - adjust the array sensor_names which is in this .ino file
    - in Sensors::getFileData () don't forget to add a "%s;" for each new sensor (if you forget the ";" it will read something completely wrong)
    - adjust the struct, getFileHeader (), getFileData (), serialPrint() and mesure() with the sensor used
    - in Sensors.h don't forget to adjust the number of sensors in the variable num_sensors. This is actually rather a count of the number of sotred measurement
      values.For example if you have an oxygen sensor and you measure the voltage but also directy compute the O2 percentage, this counts as 2 measurements

The conf.txt file must have the following structure:
  300; //deepsleep time in seconds (put at least 240 seconds)
  1; //boolean value to choose if you want to set the RTC clock with GSM time (recommended to put 1)
  12; // number of measurements to be sent at once

If you want to change the deep sleep time or the number of measurements to be sent at once remotely, go to https://notehub.io and under "Fleet" select your device.
Then check the little square to have a blue check in it, then press on the "+Note" icon. A popup will appear with the title "Add note to device". You must select "data.qi" as notefile
and in the JSON note write (with the brackets) {"deep_sleep_time":120} or {"deep_sleep_time":1800} or {"MAX_MEASUREMENTS":10} etc. It will write this value on the sd card of the datalogger
the next time it tries to send data over gsm, or also when you reboot it.

If you have any questions contact me per email at nicolas.schmid.6035@gmail.com
**********************************************************************************
*/ 


#ifndef sensors_h
#define sensors_h
#include "Arduino.h"
#define num_sensors 3 ///change here is you add sensors !!!!!!!! 


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

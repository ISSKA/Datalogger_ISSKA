# TinyPico BASE CODE FROM MARCH 2023
## Explications 
This is the base code for simple datalogger (without Notecard). For a new datalogger, you only have to copy the 3 files (.ino, sensors.cpp, sensors.h) and adjust the sensor.cpp file with the sensors you want to connect.

## How to modify the code for new sensors
Here are the things you should modify in the sensors.cpp file when to adding or removing a sensor:
1. import the libraries needed for your sensors e.g. `#include <SparkFun_PHT_MS8607_Arduino_Library.h>`
2. get an instance of the class needed for your sensor e.g. `MS8607 barometricSensor;`
3. in the `Mesures` struct, add an entry for each value to be measured. e.g.:
               `typedef struct {
                 float meas1;
                 float meas2;
                 ...
               } Mesures;`
4. in `Sensors::getFileHeader ()` adjust the header with the values which will be measured e.g.: `return (String)"meas1;meas2;...";`
5. in `Sensors::getFileData ()` don't forget to add a "%s;" for each new sensor and `String(mesures.meas1).c_str(), ...` for each sensor.
6. in `Sensors::serialPrint()` add the following line for each value measured: `sensor_values_str = sensor_values_str + "hum: " + String(mesures.meas1) + "\n";`
7. in `Sensors::mesure()` select the correct entry of the multiplexer according to your wiring (e.g. `tcaselect(7);`). Initialise your sensor (e.g. `barometricSensor.begin();`) and store the values measured in the struct (e.g. `mesures.temp = barometricSensor.getTemperature();`)

## Values which must be on the SD card
The conf.txt file must have the following structure:

  ```
  300; //deepsleep time in seconds
  1; //boolean value to choose if you want to set the RTC clock with the time on SD card
  2023/03/31 16:05:00; //time to which the RTC would be set if setRTC is 1
  ```
  
  If you choose SetRTC = 1, press the reboot button at the written time to set the RTC at this time. Then check that the logger correctly initialised and then set SetRTC to 0 on the SD card to not set the time every time you push the reboot button.
  
## Price list
| Parts  | Prices |
| ------------- | ------------- |
| TinyPico  | 26.40  |
| OLED 128x64  | 17  |
| RTC DS3231  | 20.15  |
| I2C multiplexer  | 8.20  |
| CR1220 coin cell  | 3  |
| Micro SD module  | 3.5  |
| Micro SD card  | 5  |
| MS8607 PTH Sensor  | 14.90  |
| Mosfet Adafruit  | 4  |
| Printed circuit  | 1.5  |
| **TOTAL**  | **103.65** |


If you have any questions contact me per email at nicolas.schmid.6035@gmail.com

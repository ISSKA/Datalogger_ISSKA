# TinyPico BASE CODE FOR DATALOGGER WITH NOTECARD AND REMOTE CONTROL (MAI 2023)
## Explications 
This is the base code for a datalogger with Notecard. For a new datalogger, you only have to copy the 3 files (.ino, sensors.cpp, sensors.h) and adjust the sensor.cpp file with the sensors you want to connect.

## How to control the datalogger remotely
If you want to change the deep sleep time or the number of measurements to be sent at once remotely, go to https://notehub.io and under `Fleet` select your device.
Then check the little square to have a blue check in it, then press on the `+Note` icon. A popup will appear with the title `Add note to device`. You must select `data.qi` as notefile. There are two different parameters you can change with the following JSON notes commands:
1. `{"deep_sleep_time":300}` This changes the deep sleep time to 300s. Its value must be at least 240s
2. `{"MAX_MEASUREMENTS":10}` This changes the number of measurements sent at once from the datalogger to 10. A small number means you will use more battery power, but know more often what values are read by the datalogger.

These parametersd will be written on the sd card of the datalogger in the conf.txt file the next time it tries to send data over gsm, or also when you reboot it.

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
5. in `Sensors::getFileData ()` don't forget to add a `%s;` for each new sensor and `String(mesures.meas1).c_str(), ...` for each sensor.
6. in `Sensors::serialPrint()` add the following line for each value measured: `sensor_values_str = sensor_values_str + "hum: " + String(mesures.meas1) + "\n";`
7. in `Sensors::mesure()` select the correct entry of the multiplexer according to your wiring (e.g. `tcaselect(7);`). Initialise your sensor (e.g. `barometricSensor.begin();`) and store the values measured in the struct (e.g. `mesures.temp = barometricSensor.getTemperature();`)
8. IMPORTANT, THIS IS DIFFERENT FROM THE SENSOR CODE WITHOUT NOTECARD: in `Sensors.h`, you must specify the number of sensors values to store with the line `#define num_sensors 3`

## Values which must be on the SD card
The conf.txt file must have the following structure:

  ```
  300; //deepsleep time in seconds (put at least 120 seconds)
  1; //boolean value to choose if you want to set the RTC clock with GSM time (recommended to put 1)
  12; // number of measurements to be sent at once
  ```
  
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
| Notecard  | ???  |
| **TOTAL**  | **103.65 + Notecard price** |


If you have any questions contact me per email at nicolas.schmid.6035@gmail.com

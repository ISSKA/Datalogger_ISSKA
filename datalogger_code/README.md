# ISSKA Datalogger codes

This repository contains the different codes used on our dataloggers. There are two Arduino codes for the dataloggers, one for dataloggers with wireless transmission and one for datalogger without wireless transmission. These 2 Arduino codes do not depend on the sensors connected to the datalogger. 

* the simple_datalogger_code is the code with no wireless transmission
* the notecard_datalogger_code is the code with wireless transmission.

There are several different C++ files in the sensors_code folder which are all named Sensors.cpp. For each new sensor configuration, this C++ file must be modified. Refer to the Datalogger Guide pdf for more informations about the Sensors.cpp modifications for a new sensor.

The header file Sensors.h is the same for all codes.

Several different Sensors.cpp code are in the sensors_code folder, for different sensors configurations. All of these codes include the BMP581 and the SHT35 which are already on each PCB. 

## Upload a code on a TinyPico
* select the right Arduino code (wireless transmission or not) and take a Sensors.cpp file in the sensors_code folder for your application or modify one.
* Put the Arduino file (wireless or not), Sensors.cpp file (which you can find in the sensors_code folder) and Sensors.h file in a same folder.
* The folder  must have the name of the arduino file, minus the ".ino"
* Install the needed library on the arduino IDE.
* Connect the TinyPico with your computer and select the right port on Arduino IDE
* Click on the upload button

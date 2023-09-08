# ISSKA Datalogger codes
Code for data-loggers which control different sensors, measure them with a given time step and store their values on an SD card. All the informations you need are in the ISSKA instruction pdf.

* the simple_datalogger_code is the code with no wireless transmission
* the notecard_datalogger_code is the code with wireless transmission.

Only the Sensors.cpp file depends on the sensors added. The Arduino file is exactely the same, regardless of the sensors that you add. 

Several different Sensors.cpp code are in the sensors_code folder, for different sensors configurations. All of these codes include the BMP581 and the SHT35 which are already on each PCB. The header file Sensors.h is the same for all codes.

## To upload these codes
* Put the Arduino file (wireless or not), Sensors.cpp (which you can find in the sensors_code folder) file and Sensors.h file in a same folder.
* The folder  must have the name of the arduino file, minus the ".ino"
* Install the needed library on the arduino IDE.
* Connect the TinyPico with your computer and select the right port on Arduino IDE
* Click on the upload button

For more detailed informations about this code, please check the datalogger instruction PDF which is in this repository.

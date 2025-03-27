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

#include <Wire.h>
#include "Sensors.h"

#include <SparkFun_FS3000_Arduino_Library.h>

#define I2C_MUX_ADDRESS 0x70
// A structure to store all sensors values
typedef struct {
float speedTOP;
float speedBOT;
} Mesures;

Mesures mesures;

//these two sensor measure temperature, humidity and pressur

FS3000 air_velocity_sensor1;
FS3000 air_velocity_sensor2;
Sensors::Sensors() {}
 
String Sensors::getFileHeader () { // Return the file header for the sensors part. This should be something like "<sensor 1 name>;<sensor 2 name>;"
  return (String)"speedTOP;speedBOT;"; //change if you add sensors !!!!!!!!
}

String Sensors::getFileData () { // Return sensors data formated to be write to the CSV file. This should be something like "<sensor 1 value>;<sensor 2 value>;"
  char str[200];
  sprintf(str, "%s;%s;", //"temp", "press", "Hum" , ... put as many %s; as there are sensors!!!!!! don't forget the last ";" !!
     String(mesures.speedTOP).c_str(), String(mesures.speedBOT).c_str());
  return str;
}

String Sensors::serialPrint() { //Display sensor mesures to Serial for debug purposes
  String sensor_values_str = "speedTOP: " + String(mesures.speedTOP) + "\n";
  sensor_values_str = sensor_values_str + "speedBOT: " + String(mesures.speedBOT) + "\n";
  Serial.print(sensor_values_str);
  return sensor_values_str;
  //add a line for each sensor!!!!!!!!!
}

void Sensors::mesure() {
  //connect and start the SHT35 PCB sensor 

  //sparkfun air velocity sensor
  tcaselect(7); //TOP
  delay(50);
  air_velocity_sensor1.begin();
  air_velocity_sensor1.setRange(AIRFLOW_RANGE_7_MPS);
  delay(100);
  mesures.speedTOP=air_velocity_sensor1.readMetersPerSecond();
  delay(200);


  tcaselect(6); //BOTTOM
  delay(50);
  air_velocity_sensor2.begin();
  air_velocity_sensor2.setRange(AIRFLOW_RANGE_7_MPS);
  delay(100);
  mesures.speedBOT=air_velocity_sensor2.readMetersPerSecond();
  delay(100);
  //put here the measurement of other sensors!!!!!!
}

// multiplex bus selection for the first multiplexer 
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}



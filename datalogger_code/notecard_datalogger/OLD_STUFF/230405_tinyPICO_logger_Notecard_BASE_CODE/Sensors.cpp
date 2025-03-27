/* *******************************************************************************
Base code for datalogger with Notecard, with only MS8067 (temp, hum, press)

This code was entirely rewritten by Nicolas Schmid. Here are some important remarks when modifying to add or  add or remove a sensor:
    - adjust the array sensor_names which is in this .ino file
    - in Sensors::getFileData () don't forget to add a "%s;" for each new sensor (if you forget the ";" it will read something completely wrong)
    - adjust the struct, getFileHeader (), getFileData (), serialPrint() and mesure() with the sensor used
    - in Sensors.h don't forget to adjust the number of sensors in the variable num_sensors. This is actually rather a count of the number of sotred measurement
      values.For example if you have an oxygen sensor and you measure the voltage but also directy compute the O2 percentage, this counts as 2 measurements

The conf.txt file must have the following structure:
  300; //deepsleep time in seconds (put at least 120 seconds)
  1; //boolean value to choose if you want to set the RTC clock with GSM time (recommended to put 1)
  12; // number of measurements to be sent at once

If you have any questions contact me per email at nicolas.schmid.6035@gmail.com
**********************************************************************************
*/ 


#include <Wire.h>
#include "Sensors.h"
#include <SparkFun_PHT_MS8607_Arduino_Library.h>

#define I2C_MUX_ADDRESS 0x70

// A structure to store all sensors values
typedef struct {
  float tempEXT;
  float pressEXT;
  float humEXT;
} Mesures;

Mesures mesures;

//these two sensor measure temperature, humidity and pressur
MS8607 barometricSensorEXT; //initialise MS8607 external sensor 

Sensors::Sensors() {}
 
String Sensors::getFileHeader () { // Return the file header for the sensors part. This should be something like "<sensor 1 name>;<sensor 2 name>;"
  return (String)"tempEXT;pressEXT;HumEXT;"; //change if you add sensors !!!!!!!!
}

String Sensors::getFileData () { // Return sensors data formated to be write to the CSV file. This should be something like "<sensor 1 value>;<sensor 2 value>;"
  char str[200];
  sprintf(str, "%s;%s;%s;", //"tempExt", "pressEXT", "HumExt", tempSOIL, pressSOIL HumSOIL , ... put as many %s; as there are sensors!!!!!! don't forget the last ";" !!
     String(mesures.tempEXT).c_str(), String(mesures.pressEXT).c_str(), String(mesures.humEXT).c_str());
  return str;
}

String Sensors::serialPrint() { //Display sensor mesures to Serial for debug purposes
  String sensor_values_str = "temp: " + String(mesures.tempEXT) + "\n";
  sensor_values_str = sensor_values_str + "press: " + String(mesures.pressEXT) + "\n";
  sensor_values_str = sensor_values_str + "hum: " + String(mesures.humEXT) + "\n";
  Serial.print(sensor_values_str);
  return sensor_values_str;
  //add a line for each sensor!!!!!!!!!
}

void Sensors::mesure() {
  //connect and start the MS8067 external sensor 
  tcaselect(7);
  delay(100);
  barometricSensorEXT.begin();
  //read values from the 3 sensors on the MS8067
  mesures.tempEXT = barometricSensorEXT.getTemperature();
  mesures.pressEXT = barometricSensorEXT.getPressure();
  mesures.humEXT = barometricSensorEXT.getHumidity();
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



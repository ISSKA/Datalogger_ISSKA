/* *******************************************************************************
Base code for simple datalogger with only MS8607


Here are some important remarks when modifying to add or  add or remove a sensor:
    - in Sensors::getFileData () don't forget to add a "%s;" for each new sensor 
    - adjust the struct, getFileHeader (), getFileData (), serialPrint() and mesure() with the sensor used

The conf.txt file must have the following structure:
  300; //deepsleep time in seconds (put at least 120 seconds)
  1; //boolean value to choose if you want to set the RTC clock with time from computer (put 1 when sending the code, but put 0 then to avoid resetting the time when pushing the reboot button)
  2023/03/31 16:05:00; //if you choose SetRTC = 1, press the reboot button at the written time to set the RTC at this time. Then check that the logger correctly initialised and then set SetRTC to 0 on the SD card to not set the time every time you push the reboot button

If you have any questions contact me per email at nicolas.schmid.6035@gmail.com
**********************************************************************************
*/ 


#include <Wire.h>
#include "Sensors.h"
#include <SparkFun_PHT_MS8607_Arduino_Library.h>
#include <SparkFun_ADS122C04_ADC_Arduino_Library.h>

#define I2C_MUX_ADDRESS 0x70

// A structure to store all sensors values
typedef struct {
  float tempEXT;
  float pressEXT;
  float humEXT;
  float PT100;
} Mesures;

Mesures mesures;

//these two sensor measure temperature, humidity and pressur
MS8607 barometricSensorEXT; //initialise MS8607 external sensor 
SFE_ADS122C04 PT100_sensor;

Sensors::Sensors() {}
 
String Sensors::getFileHeader () { // Return the file header for the sensors part. This should be something like "<sensor 1 name>;<sensor 2 name>;"
  return (String)"tempEXT;pressEXT;HumEXT;PT100;"; //change if you add sensors !!!!!!!!
}

String Sensors::getFileData () { // Return sensors data formated to be write to the CSV file. This should be something like "<sensor 1 value>;<sensor 2 value>;"
  char str[200];
  sprintf(str, "%s;%s;%s;%s", //"tempExt", "pressEXT", "HumExt" , ... put as many %s; as there are sensors!!!!!! don't forget the last ";" !!
     String(mesures.tempEXT,3).c_str(), String(mesures.pressEXT,3).c_str(), String(mesures.humEXT,3).c_str(),String(mesures.PT100,4).c_str());
  return str;
}

String Sensors::serialPrint() { //Display sensor mesures to Serial for debug purposes
  String sensor_values_str = "temp: " + String(mesures.tempEXT) + "\n";
  sensor_values_str = sensor_values_str + "press: " + String(mesures.pressEXT) + "\n";
  sensor_values_str = sensor_values_str + "hum: " + String(mesures.humEXT) + "\n";
  sensor_values_str = sensor_values_str + "PT100: " + String(mesures.PT100) + "\n";
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

  tcaselect(6);
  PT100_sensor.begin();
  PT100_sensor.configureADCmode(ADS122C04_4WIRE_MODE);
  mesures.PT100 = PT100_sensor.readPT100Centigrade();
  PT100_sensor.powerdown();
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



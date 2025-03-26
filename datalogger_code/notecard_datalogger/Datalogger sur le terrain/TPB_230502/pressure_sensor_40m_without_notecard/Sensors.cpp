/* *******************************************************************************
Code for datalogger with MS8607 and MS5803 connected at 40m (needs slower i2c frequency)


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
#include <SparkFun_MS5803_I2C.h>

#define I2C_MUX_ADDRESS 0x70

// A structure to store all sensors values
typedef struct {
  float temp;
  float press;
  float hum;
  float tempS;
  float pressS;
  float PrEau;
  float HtEau;
} Mesures;

Mesures mesures;

//these two sensor measure temperature, humidity and pressur
MS8607 barometricSensorEXT; //initialise MS8607 external sensor 
MS5803 pressureSensor(ADDRESS_HIGH);

Sensors::Sensors() {}
 
String Sensors::getFileHeader () { // Return the file header for the sensors part. This should be something like "<sensor 1 name>;<sensor 2 name>;"
  return (String)"temp;press;Hum;tempS;pressS;PrEau;HtEau;"; //change if you add sensors !!!!!!!!
}

String Sensors::getFileData () { // Return sensors data formated to be write to the CSV file. This should be something like "<sensor 1 value>;<sensor 2 value>;"
  char str[200];
  sprintf(str, "%s;%s;%s;%s;%s;%s;%s;", //"temp", "press", "Hum" , ... put as many %s; as there are sensors!!!!!! don't forget the last ";" !!
     String(mesures.temp).c_str(), String(mesures.press).c_str(), String(mesures.hum).c_str(), String(mesures.tempS).c_str(), String(mesures.pressS).c_str(), String(mesures.PrEau).c_str(), String(mesures.HtEau).c_str());
  return str;
}

String Sensors::serialPrint() { //Display sensor mesures to Serial for debug purposes
  String sensor_values_str = "temp: " + String(mesures.temp,1) + "\n";
  sensor_values_str = sensor_values_str + "press: " + String(mesures.press,1) + "\n";
  sensor_values_str = sensor_values_str + "hum: " + String(mesures.hum,1) + "\n";
  sensor_values_str = sensor_values_str + "tempS: " + String(mesures.tempS,1) + "\n";
  sensor_values_str = sensor_values_str + "pressS: " + String(mesures.pressS,1) + "\n";
  sensor_values_str = sensor_values_str + "PrEau: " + String(mesures.PrEau,1) + "\n";
  sensor_values_str = sensor_values_str + "HtEau: " + String(mesures.HtEau,1) + "\n";
  Serial.print(sensor_values_str);
  return sensor_values_str;
  //add a line for each sensor!!!!!!!!!
}

void Sensors::mesure() {
  //connect and start the MS8067 external sensor 
  tcaselect(6);
  delay(100);
  barometricSensorEXT.begin();
  //read values from the 3 sensors on the MS8067
  mesures.temp = barometricSensorEXT.getTemperature();
  mesures.press = barometricSensorEXT.getPressure();
  mesures.hum = barometricSensorEXT.getHumidity();
  delay(100);


  tcaselect(5);
  delay(100);

  pressureSensor.reset();
  pressureSensor.begin();
  delay(100);
  mesures.tempS = pressureSensor.getTemperature(CELSIUS, ADC_512);
  mesures.pressS = 2.2+pressureSensor.getPressure(ADC_4096)/10; //2.2  was somehow the shift of pressure between the two sensors without water


  mesures.PrEau = mesures.pressS-mesures.press; //pressure from the water (remove air pressure)
  mesures.HtEau = 10*mesures.PrEau/9.81; //water height in cm
  delay(100);
  tcaselect(0); //do not forgetr to put something else than 5 otherwise rtc doesn't work


  
  //put here the measurement of other sensors!!!!!!
}

// multiplex bus selection for the first multiplexer 
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}




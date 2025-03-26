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
#include <SensirionI2CSfmSf06.h>

#define I2C_MUX_ADDRESS 0x70

// A structure to store all sensors values
typedef struct {
  float temp1;
  float press1;
  float humidityBOX;
  float velSFM;
  float debitSFM;
  float tempSFM;
} Mesures;

Mesures mesures;

//these two sensor measure temperature, humidity and pressur
MS8607 barometricSensorEXT; //initialise MS8607 external sensor 
SensirionI2CSfmSf06 sfmSf06;

Sensors::Sensors() {}
 
String Sensors::getFileHeader () { // Return the file header for the sensors part. This should be something like "<sensor 1 name>;<sensor 2 name>;"
  return (String)"temperature;pressure(mbar);humidityBOX;velocitySFM;debitSFM;tempSFM;"; //change if you add sensors !!!!!!!!
}

String Sensors::getFileData () { // Return sensors data formated to be write to the CSV file. This should be something like "<sensor 1 value>;<sensor 2 value>;"
  char str[200];
  sprintf(str, "%s;%s;%s;%s;%s;%s;", //"tempExt", "pressEXT", "HumExt" , ... put as many %s; as there are sensors!!!!!! don't forget the last ";" !!
     String(mesures.temp1, 3).c_str(), String(mesures.press1, 3).c_str(), String(mesures.humidityBOX, 3).c_str(), String(mesures.velSFM, 7).c_str(),String(mesures.debitSFM, 5).c_str(),String(mesures.tempSFM, 2).c_str());
  return str;
}

String Sensors::serialPrint() { //Display sensor mesures to Serial for debug purposes
  String sensor_values_str = "temp: " + String(mesures.temp1) + "\n";
  sensor_values_str = sensor_values_str + "press: " + String(mesures.press1) + "\n";
  sensor_values_str = sensor_values_str + "hum: " + String(mesures.humidityBOX) + "\n";
  sensor_values_str = sensor_values_str + "VelSFM: " + String(mesures.velSFM,3) + "\n";
  sensor_values_str = sensor_values_str + "Q_SFM: " + String(mesures.debitSFM,3) + "\n";
  sensor_values_str = sensor_values_str + "T_SFM: " + String(mesures.tempSFM) + "\n";
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
  mesures.temp1 = barometricSensorEXT.getTemperature();
  mesures.press1 = barometricSensorEXT.getPressure();
  mesures.humidityBOX = barometricSensorEXT.getHumidity();
  delay(100);
  
      //SFM 3003
  sfmSf06.begin(Wire, ADDR_SFM3003_300_CE);
  delay(100);
//  sfmSf06.stopContinuousMeasurement();
//  delay(100);
  sfmSf06.startO2ContinuousMeasurement();
  delay(500);
  int16_t temperatureRaw = 0;
  uint16_t status = 0;
  float SFMmoyenne = 0;
  int16_t flowRaw=0;
  
  delay(100);
  sfmSf06.readMeasurementDataRaw(flowRaw, temperatureRaw, status);
  delay(1000);
  mesures.debitSFM=(float(flowRaw)+12288)/120;
  //SFMmoyenne +=flowcorr;
  mesures.velSFM=((mesures.debitSFM/1000)/60)/0.00030791;
  mesures.tempSFM = float(temperatureRaw)/200;

}

// multiplex bus selection for the first multiplexer 
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}



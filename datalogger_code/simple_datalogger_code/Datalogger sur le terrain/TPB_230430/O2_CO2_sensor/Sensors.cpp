/* *******************************************************************************
Code for simple datalogger with O2 sensor and MS8607

Don't forget the calibration of the O2 sensor, its output widely depends on the temperature, and a little bit on the pressure


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
#include <SparkFun_TMP117.h> 
#include <Ezo_i2c.h> 
#include "SparkFun_SCD4x_Arduino_Library.h" 

#define I2C_MUX_ADDRESS 0x70
#define I2C_ATLAS_O2_ADRESS 108 

// A structure to store all sensors values
typedef struct {
  float temp;
  float percO2;
  float ppmCO2;
  float tempCO2;
  float humCO2;
} Mesures;

Mesures mesures;


TMP117 temp_sensor; // Initalize sensor
Ezo_board O2sensor = Ezo_board(I2C_ATLAS_O2_ADRESS); //initialise O2 sensor
SCD4x co2_sensor(SCD4x_SENSOR_SCD41); //initialise SCD41 CO2 sensor in soil

Sensors::Sensors() {}
 
String Sensors::getFileHeader () { // Return the file header for the sensors part. This should be something like "<sensor 1 name>;<sensor 2 name>;"
  return (String)"temp;percO2;ppmCO2;tempCO2;humCO2;"; //change if you add sensors !!!!!!!!
}

String Sensors::getFileData () { // Return sensors data formated to be write to the CSV file. This should be something like "<sensor 1 value>;<sensor 2 value>;"
  char str[200];
  sprintf(str, "%s;%s;%s;%s;%s;", //"tempExt", "pressEXT", "HumExt" , ... put as many %s; as there are sensors!!!!!! don't forget the last ";" !!
     String(mesures.temp).c_str(),String(mesures.percO2,3).c_str(),String(mesures.ppmCO2).c_str(), String(mesures.tempCO2).c_str(), String(mesures.humCO2).c_str());
  return str;
}

String Sensors::serialPrint() { //Display sensor mesures to Serial for debug purposes
  String sensor_values_str = "temp: " + String(mesures.temp) + "\n";
  sensor_values_str = sensor_values_str + "O2: " + String(mesures.percO2,3) + "\n";
  sensor_values_str = sensor_values_str + "ppmCO2: " + String(mesures.ppmCO2) + "\n";
  sensor_values_str = sensor_values_str + "tempCO2: " + String(mesures.tempCO2) + "\n";
  sensor_values_str = sensor_values_str + "humCO2: " + String(mesures.humCO2) + "\n";
  Serial.print(sensor_values_str);
  return sensor_values_str;
  //add a line for each sensor!!!!!!!!!
}

void Sensors::mesure() {
  //connect and start the TMP117 sensor 
  tcaselect(6);
  delay(100);
  temp_sensor.begin();
  delay(100);
  mesures.temp=temp_sensor.readTempC();
  delay(100);

  //O2 atlas scientific sensor, this sensor must be calibrated for a given temperature
  //measure a few times without recording to warm it up
  for(int i = 0; i < 5; i ++){
    O2sensor.send_read_with_temp_comp(mesures.temp);
    delay(1000);
    O2sensor.receive_read_cmd();              //get the response data and put it into the [Device].reading variable if successful
    delay(1000);
    Serial.print("O2 warm up: ");
    Serial.println(String(O2sensor.get_last_received_reading(), 3).c_str());
  }

  O2sensor.send_read_with_temp_comp(mesures.temp);
  delay(1000);
  O2sensor.receive_read_cmd();              //get the response data and put it into the [Device].reading variable if successful
  mesures.percO2 = O2sensor.get_last_received_reading(); //print either the reading or an error message
  delay(400);

  //CO2 sensor values
  tcaselect(5);
  delay(100);
  co2_sensor.begin(false, false, false);
  co2_sensor.measureSingleShot();
  delay(10000); //this sensor needs at least 4-10 seconds to measure something good (heating time)
  co2_sensor.readMeasurement();
  mesures.ppmCO2=co2_sensor.getCO2();
  mesures.tempCO2=co2_sensor.getTemperature();
  mesures.humCO2=co2_sensor.getHumidity();
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



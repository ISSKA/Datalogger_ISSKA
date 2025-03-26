#include "HardwareSerial.h"
/* *******************************************************************************
Code for datalogger with TMP117 and MS5803 connected at 10m (needs slower i2c frequency)


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
#include <SparkFun_MS5803_I2C.h>

#define I2C_MUX_ADDRESS 0x70

// A structure to store all sensors values
typedef struct {
  float temp117;
  float pressWat;
  float tempWat;
  float pressBox;
  float tempBox;
  float PrEau;
  float HtEau;
} Mesures;

//create objects needed for measurements
Mesures mesures;
MS5803 pressureWater(ADDRESS_HIGH);
MS5803 pressureBox(ADDRESS_HIGH);
TMP117 temp_sensor;

Sensors::Sensors() {}
 
String Sensors::getFileHeader () { // Return the file header for the sensors part. This should be something like "<sensor 1 name>;<sensor 2 name>;"
  return (String)"temp117;pressWat;tempWat;pressBox;tempBox;PrEau;HtEau;"; //change if you add sensors !!!!!!!!
}

String Sensors::getFileData () { // Return sensors data formated to be write to the CSV file. This should be something like "<sensor 1 value>;<sensor 2 value>;"
  char str[200];
  sprintf(str, "%s;%s;%s;%s;%s;%s;%s;", //"temp", "press", "Hum" , ... put as many %s; as there are sensors!!!!!! don't forget the last ";" !!
     String(mesures.temp117).c_str(), String(mesures.pressWat).c_str(), String(mesures.tempWat).c_str(), String(mesures.pressBox).c_str(), String(mesures.tempBox).c_str(), String(mesures.PrEau).c_str(), String(mesures.HtEau).c_str());
  return str;
}

String Sensors::serialPrint() { //Display sensor mesures to Serial for debug purposes
  String sensor_values_str = "temp117: " + String(mesures.temp117,1) + "\n";
  sensor_values_str = sensor_values_str + "pressWat: " + String(mesures.pressWat,1) + "\n";
  sensor_values_str = sensor_values_str + "tempWat: " + String(mesures.tempWat,1) + "\n";
  sensor_values_str = sensor_values_str + "pressBox: " + String(mesures.pressBox,1) + "\n";
  sensor_values_str = sensor_values_str + "tempBox: " + String(mesures.tempBox,1) + "\n";
  sensor_values_str = sensor_values_str + "PrEau: " + String(mesures.PrEau,1) + "\n";
  sensor_values_str = sensor_values_str + "HtEau: " + String(mesures.HtEau,1) + "\n";
  Serial.print(sensor_values_str);
  return sensor_values_str;
  //add a line for each sensor!!!!!!!!!
}

void Sensors::mesure() {
  //connect and measure the TMP117 sensor 
  tcaselect(4);
  delay(100);
  temp_sensor.begin();
  delay(100);
  mesures.temp117=temp_sensor.readTempC();
  delay(100);

  //connect and measure the MS5803 sensor in the box 
  tcaselect(5);
  delay(100);
  pressureBox.reset();
  pressureBox.begin();
  delay(100);
  mesures.tempBox = pressureBox.getTemperature(CELSIUS, ADC_512);
  mesures.pressBox = pressureBox.getPressure(ADC_4096)/10;
  delay(100);

  //connect and measure the MS5803 sensor in the water 
  tcaselect(6);
  delay(100);
  pressureWater.reset();
  pressureWater.begin();
  delay(100);
  mesures.tempWat = pressureWater.getTemperature(CELSIUS, ADC_512);
  mesures.pressWat = pressureWater.getPressure(ADC_4096)/10 +2.9; //+2.9 probably comes from voltage loss in the loing cable or just some shift of one of the two pressure sensors
  delay(100);

  //compute the diffrence of pressure and water height
  mesures.PrEau = mesures.pressWat-mesures.pressBox; //pressure from the water (remove air pressure)
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






/* *******************************************************************************
Base code for datalogger (isska logger v1.0) with Notecard,and MS5803 connected at 40m  (temp, hum, press). We can now remotely change some parameters on the conf.txt file remotely.


This code was entirely rewritten by Nicolas Schmid. Here are some important remarks when modifying to add or  add or remove a sensor:
    - adjust the array sensor_names which is in this .ino file
    - in Sensors::getFileData () don't forget to add a "%s;" for each new sensor (if you forget the ";" it will read something completely wrong)
    - adjust the struct, getFileHeader (), getFileData (), serialPrint() and mesure() with the sensor used
    - in Sensors.h don't forget to adjust the number of sensors in the variable num_sensors. This is actually rather a count of the number of sotred measurement
      values.For example if you have an oxygen sensor and you measure the voltage but also directy compute the O2 percentage, this counts as 2 measurements

The conf.txt file must have the following structure:
  300; //deepsleep time in seconds (put at least 240 seconds)
  1; //boolean value to choose if you want to set the RTC clock with GSM time (recommended to put 1)
  12; // number of measurements to be sent at once

If you want to change the deep sleep time or the number of measurements to be sent at once remotely, go to https://notehub.io and under "Fleet" select your device.
Then check the little square to have a blue check in it, then press on the "+Note" icon. A popup will appear with the title "Add note to device". You must select "data.qi" as notefile
and in the JSON note write (with the brackets) {"deep_sleep_time":120} or {"deep_sleep_time":1800} or {"MAX_MEASUREMENTS":10} etc. It will write this value on the sd card of the datalogger
the next time it tries to send data over gsm, or also when you reboot it.

If you have any questions contact me per email at nicolas.schmid.6035@gmail.com
**********************************************************************************
*/ 


#include <Wire.h>
#include "Sensors.h"
#include "SHT31.h"
#include "SparkFun_BMP581_Arduino_Library.h"
#include "SFM3000.h"

#define I2C_MUX_ADDRESS 0x73
#define BMP581_sensor_ADRESS 0x46 //values set on the PCB hardware (not possible to change)
#define SHT35_sensor_ADRESS 0x44 //values set on the PCB hardware (not possible to change)
int offset = 32000; // Offset for the sensor
float scale = 140.0; // Scale factor for Air and N2 is 140.0, O2 is 142.8

// A structure to store all sensors values
typedef struct {
  float tempSHT;
  float humSHT;
  float tempBMP;
  float pressBMP;
  float debit;
  double velocity;
} Mesures;

Mesures mesures;

SHT31 sht;
BMP581 bmp;
SFM3000 sfm3000;
//MS5837 sensor_pluvio;


Sensors::Sensors() {}

 
String Sensors::getFileHeader () { // Return the file header for the sensors part. This should be something like "<sensor 1 name>;<sensor 2 name>;"
  return "tempSHT;humSHT;tempBMP;pressBMP;debit;velocity;"; //change if you add sensors !!!!!!!!
}

String Sensors::getFileData () { // Return sensors data formated to be write to the CSV file. This should be something like "<sensor 1 value>;<sensor 2 value>;"
  char str[200];
  sprintf(str, "%s;%s;%s;%s;%s;%s;", //"temp", "press", "Hum" , ... put as many %s; as there are sensors!!!!!! don't forget the last ";" !!
     String(mesures.tempSHT,1).c_str(), String(mesures.humSHT,1).c_str(), String(mesures.tempBMP,3).c_str(),String(mesures.pressBMP,6).c_str(), String(mesures.debit,6).c_str(), String(mesures.velocity,6).c_str());
  return str;
}

String Sensors::serialPrint() { //Display sensor mesures to Serial for debug purposes
  String sensor_values_str = "tempSHT: " + String(mesures.tempSHT,1) + "\n";
  sensor_values_str = sensor_values_str + "H_SHT: " + String(mesures.humSHT,3) + "\n";
  sensor_values_str = sensor_values_str + "T_BMP: " + String(mesures.tempBMP,3) + "\n";
  sensor_values_str = sensor_values_str + "P_BMP: " + String(mesures.pressBMP,3) + "\n";
  sensor_values_str = sensor_values_str + "Flow: " + String(mesures.debit,3) + "\n";
  sensor_values_str = sensor_values_str + "Velo: " + String(mesures.velocity,3) + "\n";
  Serial.print(sensor_values_str);
  return sensor_values_str;
  //add a line for each sensor!!!!!!!!!
}

void Sensors::mesure() {
  //connect and start the MS8067 external sensor 
//connect and start the SHT35 PCB sensor 
  tcaselect(1);
  delay(10);
  sht.begin(SHT35_sensor_ADRESS);    //Sensor I2C Address
  delay(10);
  sht.read();
  mesures.tempSHT=sht.getTemperature();
  mesures.humSHT=sht.getHumidity();
  delay(50);

  //connect and start the BMP581 PCB sensor 
  tcaselect(2);
  delay(10);
  bmp.beginI2C(BMP581_sensor_ADRESS);
  delay(10);
  bmp5_sensor_data data = {0,0};
  int8_t err = bmp.getSensorData(&data);
  mesures.tempBMP=data.temperature;
  mesures.pressBMP=data.pressure/100; //in millibar
  delay(50);
  
  tcaselect(7);
  delay(50);
  sfm3000.begin();
  delay(50);
  float FlowSFM = sfm3000.getMeasurement();
  //Calcul de la vitesse en m/s en divisant par la surface du tube de mesure dont le diamètre est de 19.8 mm
  float FlowSFM_ms=((FlowSFM/1000)/60)/0.00030791;
  
  mesures.debit = FlowSFM;
  mesures.velocity = FlowSFM_ms;

  tcaselect(1);
}

// multiplex bus selection for the first multiplexer 
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}




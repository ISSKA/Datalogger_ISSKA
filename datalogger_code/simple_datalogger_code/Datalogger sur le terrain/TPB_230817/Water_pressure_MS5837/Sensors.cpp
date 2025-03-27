
/* *******************************************************************************
Base code for datalogger with PCB IISKA-LOGGER V1.0 There are two sensors already installed on this PCB, the SHT35 (I2c adress 0x44 and TCA1) and the BMP581 (I2c adress 0x46 and TCA2)

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
#include "SHT31.h"
#include "SparkFun_BMP581_Arduino_Library.h"
#include "MS5837.h"

#define I2C_MUX_ADDRESS 0x73 //this is the adress in the hardware of the PCB V3.2
#define BMP581_sensor_ADRESS 0x46 //values set on the PCB hardware (not possible to change)
#define SHT35_sensor_ADRESS 0x44 //values set on the PCB hardware (not possible to change)

// A structure to store all sensors values
typedef struct {
  float tempSHT;
  float pressBMP;
  float humSHT;
  float tempBMP;
  float pressMS;
  float tempMS;
  float htEau;
} Mesures;

Mesures mesures;

//these two sensor measure temperature, humidity and pressur
SHT31 sht;
BMP581 bmp;
MS5837 ms5;

Sensors::Sensors() {}
 
String Sensors::getFileHeader () { // Return the file header for the sensors part. This should be something like "<sensor 1 name>;<sensor 2 name>;"
  return (String)"tempSHT;pressBMP;humSHT;tempBMP;pressMS;tempMS;htEau;"; //change if you add sensors !!!!!!!!
}

String Sensors::getFileData () { // Return sensors data formated to be write to the CSV file. This should be something like "<sensor 1 value>;<sensor 2 value>;"
  char str[200];
  sprintf(str, "%s;%s;%s;%s;%s;%s;%s;", //"temp", "press", "Hum" , ... put as many %s; as there are sensors!!!!!! don't forget the last ";" !!
     String(mesures.tempSHT).c_str(), String(mesures.pressBMP).c_str(), String(mesures.humSHT).c_str(),String(mesures.tempBMP).c_str(),String(mesures.pressMS).c_str(),String(mesures.tempMS).c_str(),String(mesures.htEau).c_str());
  return str;
}

String Sensors::serialPrint() { //Display sensor mesures to Serial for debug purposes
  String sensor_values_str = "tempSHT: " + String(mesures.tempSHT) + "\n";
  sensor_values_str = sensor_values_str + "pressBMP: " + String(mesures.pressBMP) + "\n";
  sensor_values_str = sensor_values_str + "humSHT: " + String(mesures.humSHT) + "\n";
  sensor_values_str = sensor_values_str + "tempBMP: " + String(mesures.tempBMP) + "\n";
    sensor_values_str = sensor_values_str + "pressMS: " + String(mesures.pressMS) + "\n";
  sensor_values_str = sensor_values_str + "tempMS: " + String(mesures.tempMS) + "\n";
  sensor_values_str = sensor_values_str + "htEau: " + String(mesures.htEau) + "\n";
  Serial.print(sensor_values_str);
  return sensor_values_str;
  //add a line for each sensor!!!!!!!!!
}

void Sensors::mesure() {
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

  //connect and start the MS5837 pressure sensor in water
  Wire.end();
  Wire.begin();
  Wire.setClock(5000);
  tcaselect(7);
  delay(100);
  unsigned long start_time_connection = micros();
  bool timeout = false;
  while (!ms5.init() && !timeout) {
    Serial.println("MS5837 pressure sensor not connected");
    delay(50);
    timeout = (micros()-start_time_connection)>2000000; //timeout after 2 seconds unsuccesful 
  }
  if (timeout){
    Serial.println("Init pressure sensor failed!");
  }
  else{
    Serial.println("Init pressure sensor ok!");
  }
  ms5.setModel(MS5837::MS5837_02BA);
  delay(100);
  ms5.read();
  delay(100);
  mesures.pressMS=ms5.pressure();
  mesures.tempMS =ms5.temperature();
  if(mesures.pressMS>4030){mesures.pressMS=0;}
  if(mesures.tempMS<-50){mesures.tempMS=0;}

  mesures.htEau = (mesures.pressMS - mesures.pressBMP)*10/9.81 + 1.7; //water height in cm

  delay(100);
  tcaselect(0);
  delay(100);
  Wire.end();
  Wire.begin();
  Wire.setClock(50000);
  delay(100);

}

// multiplex bus selection for the first multiplexer 
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}



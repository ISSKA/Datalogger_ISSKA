#include "HardwareSerial.h"
/* *******************************************************************************
Base code for datalogger with PCB design v3.2 There are two sensors already installed on this PCB, the SHT35 and the BMP581

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
#include <SoftwareSerial.h> 

#define I2C_MUX_ADDRESS 0x73 //this is the adress in the hardware of the PCB V3.2
#define BMP581_sensor_ADRESS 0x46 //values set on the PCB hardware (not possible to change)
#define SHT35_sensor_ADRESS 0x44 //values set on the PCB hardware (not possible to change)
#define EZO_PUMP_ADRESS 103              //default I2C ID number for EZO-PMP Embedded Dosing Pump.

#define RX 33
#define TX 32

SoftwareSerial myserial(RX,TX);

// A structure to store all sensors values
typedef struct {
  float tempSHT;
  float press;
  float hum;
  float tempBMP;
} Mesures;

Mesures mesures;

//these two sensor measure temperature, humidity and pressur
SHT31 sht;
BMP581 bmp;

Sensors::Sensors() {}
 
String Sensors::getFileHeader () { // Return the file header for the sensors part. This should be something like "<sensor 1 name>;<sensor 2 name>;"
  return (String)"tempSHT;press;hum;tempBMP;"; //change if you add sensors !!!!!!!!
}

String Sensors::getFileData () { // Return sensors data formated to be write to the CSV file. This should be something like "<sensor 1 value>;<sensor 2 value>;"
  char str[200];
  sprintf(str, "%s;%s;%s;%s;", //"temp", "press", "Hum" , ... put as many %s; as there are sensors!!!!!! don't forget the last ";" !!
     String(mesures.tempSHT).c_str(), String(mesures.press).c_str(), String(mesures.hum).c_str(),String(mesures.tempBMP).c_str());
  return str;
}

String Sensors::serialPrint() { //Display sensor mesures to Serial for debug purposes
  String sensor_values_str = "tempSHT: " + String(mesures.tempSHT) + "\n";
  sensor_values_str = sensor_values_str + "press: " + String(mesures.press) + "\n";
  sensor_values_str = sensor_values_str + "hum: " + String(mesures.hum) + "\n";
  sensor_values_str = sensor_values_str + "tempBMP: " + String(mesures.tempBMP) + "\n";
  Serial.print(sensor_values_str);
  return sensor_values_str;
  //add a line for each sensor!!!!!!!!!
}

void Sensors::mesure() {
  //connect and start the SHT35 PCB sensor 
  sht.begin(SHT35_sensor_ADRESS);    //Sensor I2C Address
  delay(10);
  sht.read();
  mesures.tempSHT=sht.getTemperature();
  mesures.hum=sht.getHumidity();
  delay(10);
  //connect and start the BMP581 PCB sensor 
  bmp.beginI2C(BMP581_sensor_ADRESS);
  delay(10);
  bmp5_sensor_data data = {0,0};
  int8_t err = bmp.getSensorData(&data);
  mesures.tempBMP=data.temperature;
  mesures.press=data.pressure/100; //in millibar
  delay(1000);
  Wire.end();
  delay(1000);
  Wire.begin();
  Wire.setClock(50000);
  delay(1000); 
  char computerdata[6]={'d',',','-','1','0',0}; //d,20 means it will pump 20ml. It needs a 0 at the end (not 0 in ASCII but the char 0)
  Wire.beginTransmission(EZO_PUMP_ADRESS);                          
  Wire.write(computerdata);                                              
  Wire.endTransmission();   

  delay(20000);  //should be long enough for the pumping time (around 105ml per min)        


  
  //put here the measurement of other sensors!!!!!!
}

// multiplex bus selection for the first multiplexer 
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}




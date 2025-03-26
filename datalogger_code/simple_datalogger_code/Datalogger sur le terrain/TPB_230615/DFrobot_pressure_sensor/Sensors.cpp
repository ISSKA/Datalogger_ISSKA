/* *******************************************************************************
Base code for datalogger with PCB design v2.6 There are two sensors already installed on this PCB, the SHT35 and the BMP388

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
#include "Adafruit_BMP3XX.h"

#include <Adafruit_ADS1X15.h>
#include <SparkFun_Qwiic_Relay.h>

#define I2C_MUX_ADDRESS 0x72 //not working on PCB v2.6 (hardware design problem)
#define BMP388_sensor_ADRESS 0x76 //values set on the PCB hardware (not possible to change)
#define SHT35_sensor_ADRESS 0x44 //values set on the PCB hardware (not possible to change)
#define RELAY_ADDR 0x18

//Setting of the water level sensor
#define RANGE 5000 // Depth measuring range 5000mm (for water)
#define CURRENT_INIT 4.00 // Current @ 0mm (uint: mA)
#define DENSITY_WATER 1  // Pure water density normalized to 1


// A structure to store all sensors values
typedef struct {
  float tempSHT;
  float press;
  float hum;
  float tempBMP;
  float voltageDFrobotmV;
  float Hteaucm;
} Mesures;

Mesures mesures;

//these two sensor measure temperature, humidity and pressur
SHT31 sht;
Adafruit_BMP3XX bmp;
Qwiic_Relay relay(RELAY_ADDR);
Adafruit_ADS1115 ads1115;  // Construct an ads1115

Sensors::Sensors() {}
 
String Sensors::getFileHeader () { // Return the file header for the sensors part. This should be something like "<sensor 1 name>;<sensor 2 name>;"
  return (String)"tempSHT;press;hum;tempBMP;voltageDFrobotmV;Hteaucm;"; //change if you add sensors !!!!!!!!
}

String Sensors::getFileData () { // Return sensors data formated to be write to the CSV file. This should be something like "<sensor 1 value>;<sensor 2 value>;"
  char str[200];
  sprintf(str, "%s;%s;%s;%s;%s;%s;", //"temp", "press", "Hum" , ... put as many %s; as there are sensors!!!!!! don't forget the last ";" !!
     String(mesures.tempSHT).c_str(), String(mesures.press).c_str(), String(mesures.hum).c_str(),String(mesures.tempBMP).c_str(),String(mesures.voltageDFrobotmV).c_str(),String(mesures.Hteaucm).c_str());
  return str;
}

String Sensors::serialPrint() { //Display sensor mesures to Serial for debug purposes
  String sensor_values_str = "tempSHT: " + String(mesures.tempSHT) + "\n";
  sensor_values_str = sensor_values_str + "press: " + String(mesures.press) + "\n";
  sensor_values_str = sensor_values_str + "hum: " + String(mesures.hum) + "\n";
  sensor_values_str = sensor_values_str + "tempBMP: " + String(mesures.tempBMP) + "\n";
  sensor_values_str = sensor_values_str + "volt: " + String(mesures.voltageDFrobotmV) + "\n";
  sensor_values_str = sensor_values_str + "Hteau: " + String(mesures.Hteaucm) + "\n";
  Serial.print(sensor_values_str);
  return sensor_values_str;
  //add a line for each sensor!!!!!!!!!
}

void Sensors::mesure() {
  //connect and start the SHT35 PCB sensor 
  sht.begin(SHT35_sensor_ADRESS);    //Sensor I2C Address
  delay(100);
  sht.read();
  mesures.tempSHT=sht.getTemperature();
  mesures.hum=sht.getHumidity();
  delay(300);

  //connect and start the BMP388 PCB sensor 
  bmp.begin_I2C(BMP388_sensor_ADRESS);
  delay(100);
  bmp.performReading();
  delay(100);
  mesures.tempBMP = bmp.temperature;
  mesures.press=bmp.pressure / 100.0; //milibar
  delay(300);

  //get water height
  Wire.end();
  delay(30);
  Wire.begin();
  Wire.setClock(100000);
  delay(50);
  //turn relay on
  if(!relay.begin()){
    Serial.println("I2C communication with Relay failed !");
  }
  delay(100);
  relay.turnRelayOn();
  delay(1500);
  Wire.end();
  delay(30);
  Wire.begin();
  Wire.setClock(100000);
  delay(50);
  //read voltage
  ads1115.begin(0x49);
  ads1115.setGain(GAIN_ONE);
  float ADC_count = ads1115.readADC_SingleEnded(0);//Sensors output is connected to A0 input
  mesures.voltageDFrobotmV = ads1115.computeVolts(ADC_count) * 1000; //*1000 to convert V to mV
  float dataCurrent = mesures.voltageDFrobotmV / 120.0; //Sense Resistor:120ohm (in mA)
  mesures.Hteaucm = (dataCurrent - CURRENT_INIT) * (RANGE / 16.0)*0.1; //Calculate depth from current readings (in cm)
  delay(100);
  relay.turnRelayOff();
}

// multiplex bus selection for the first multiplexer 
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}



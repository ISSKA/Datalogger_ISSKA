/* *******************************************************************************
Code for datalogger TPA_230317 with Notecard, 2 MS8067 (temp, hum, press), 1 SCD41 CO2 sensor and an O2 sensor


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
#include "SparkFun_SCD4x_Arduino_Library.h" 
#include "Adafruit_ADS1X15.h"
#include "SHT31.h"
#include "SparkFun_BMP581_Arduino_Library.h"

#define I2C_MUX_ADDRESS 0x73 //I2C adress of the multiplexer set on the PCB
#define BMP581_sensor_ADRESS 0x46 //I2C adress of the BMP581 set on the PCB
#define SHT35_sensor_ADRESS 0x44 //I2C adress of the SHT35 set on the PCB


//for adc converter for O2
#define ADC_MAX 32768 //ADC 16 bits mais 1 bit de signe -> 2^15 = 32768
#define V_REF 2.048 // 2x gain   +/- 2.048V -> V_REF = 2.048

//for O2 sensor I would use: https://wiki.dfrobot.com/SKU_SEN0465toSEN0476_Gravity_Gas_Sensor_Calibrated_I2C_UART

// A structure to store all sensors values
typedef struct {
  float tempEXT;
  float pressEXT;
  float humEXT;
  float tempSOIL;
  float pressSOIL;
  float humSOIL;
  uint16_t ppmCO2; //change name to co2
  float tempCO2;
  float humCO2;
  float voltageO2;
  float percentageO2;
} Mesures;

Mesures mesures;

//these two sensor measure temperature, humidity and pressur
SHT31 sht;
BMP581 bmp;
MS8607 barometricSensorEXT; //initialise MS8607 external sensor 
MS8607 barometricSensorSOIL; //initialise MS8607 sensor in soil
SCD4x co2_sensor(SCD4x_SENSOR_SCD41); //initialise SCD41 CO2 sensor in soil
Adafruit_ADS1115 ads1115; //initialise the O2 sensor in SOIL

Sensors::Sensors() {}
 
String Sensors::getFileHeader () { // Return the file header for the sensors part. This should be something like "<sensor 1 name>;<sensor 2 name>;"
  return (String)"tempEXT;pressEXT;HumEXT;tempSOIL;pressSOIL;humSOIL;ppmCO2;tempCO2;humCO2;voltageO2;percentageO2;"; //change if you add sensors !!!!!!!!
}

String Sensors::getFileData () { // Return sensors data formated to be write to the CSV file. This should be something like "<sensor 1 value>;<sensor 2 value>;"
  char str[200];
  sprintf(str, "%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;%s;", //"tempExt", "pressEXT", "HumExt", tempSOIL, pressSOIL HumSOIL , ... put as many %s; as there are sensors!!!!!! don't forget the last ";" !!
     String(mesures.tempEXT).c_str(), String(mesures.pressEXT).c_str(), String(mesures.humEXT).c_str(),String(mesures.tempSOIL).c_str(), String(mesures.pressSOIL).c_str(), String(mesures.humSOIL).c_str(),String(mesures.ppmCO2).c_str(), String(mesures.tempCO2).c_str(), String(mesures.humCO2).c_str(), String(mesures.voltageO2,5).c_str(),String(mesures.percentageO2,4).c_str());
  return str;
}

String Sensors::serialPrint() { //Display sensor mesures to Serial for debug purposes
  String sensor_values_str = "temp: " + String(mesures.tempEXT) + "\n";
  sensor_values_str = sensor_values_str + "press: " + String(mesures.pressEXT) + "\n";
  sensor_values_str = sensor_values_str + "hum: " + String(mesures.humEXT) + "\n";
  sensor_values_str = sensor_values_str + "tempS: " + String(mesures.tempSOIL) + "\n";
  sensor_values_str = sensor_values_str + "pressS: " + String(mesures.pressSOIL) + "\n";
  sensor_values_str = sensor_values_str + "humS: " + String(mesures.humSOIL) + "\n";
  sensor_values_str = sensor_values_str + "ppm: " + String(mesures.ppmCO2) + "\n";
  sensor_values_str = sensor_values_str + "tempC: " + String(mesures.tempCO2) + "\n";
  sensor_values_str = sensor_values_str + "humC: " + String(mesures.humCO2) + "\n";
  sensor_values_str = sensor_values_str + "O2V: " + String(mesures.voltageO2,5) + "\n";
  sensor_values_str = sensor_values_str + "O2: " + String(mesures.percentageO2,4) + "\n";
  Serial.print(sensor_values_str);
  return sensor_values_str;
  //add a line for each sensor!!!!!!!!!
}

void Sensors::mesure() {
  //connect and start the MS8067 external sensor 
  //connect and start the BMP581 PCB sensor 
   //connect and start the SHT35 PCB sensor 
  tcaselect(1);
  delay(30); //wait 3ms for the multiplexer to switch
  sht.begin(); 
  sht.read();
  mesures.humEXT =sht.getHumidity(); //humSHT
  tcaselect(2);
  delay(30);
  bmp.beginI2C(BMP581_sensor_ADRESS);
  delay(100);
  bmp5_sensor_data data = {0,0};
  int8_t err = bmp.getSensorData(&data);
  mesures.tempEXT = data.temperature; //tempBMP
  mesures.pressEXT = data.pressure/100; //pressBMP (in millibar)
  

  //connect MS8067 and CO2 sensors in SOIL
  delay(300);
  tcaselect(5);
  delay(300);

  //read MS8067 sensor in Soil
  barometricSensorSOIL.begin();
  //read values from the 3 sensors on the MS8067 in SOIL
  mesures.tempSOIL = barometricSensorSOIL.getTemperature();
  mesures.pressSOIL = barometricSensorSOIL.getPressure();
  mesures.humSOIL = barometricSensorSOIL.getHumidity();
  delay(300); //wait to be done with measurements before going to next sensor

  //measure values from CO2 sensor
  co2_sensor.begin(false, false, false);
  co2_sensor.measureSingleShot();
  delay(10000); //this sensor needs at least 4-10 seconds to measure something good (heating time)
  co2_sensor.readMeasurement();
  mesures.ppmCO2=co2_sensor.getCO2();
  mesures.tempCO2=co2_sensor.getTemperature();
  mesures.humCO2=co2_sensor.getHumidity();
  delay(100);

  //measure the voltage of the O2 sensor (read from the ADC converter)
  ads1115.setGain(GAIN_TWO);// 2x gain   +/- 2.048V -> V_REF = 2.048
  ads1115.begin();
  delay(1000);
  float adcValue=ads1115.readADC_SingleEnded(0);
  delay(1000);
  mesures.voltageO2 = (adcValue/ADC_MAX)*V_REF;

  //compute the percentage of O2 with some dark magic formulas (I hope it is calibrated...)
  float O2_voltage_tcomp=0.002864*mesures.tempSOIL+1.98112; //Voltage à
  //O2 sensor is the SGX-40X
  //0% -> 0V    18.4% -> 1.2V
  mesures.percentageO2 = (mesures.voltageO2*0.2095/O2_voltage_tcomp)*100; //remplacé 0.184 par 0.2095 car on ne fait plus compensation altitude

  //put here the measurement of other sensors!!!!!!
}

// multiplex bus selection for the first multiplexer 
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}



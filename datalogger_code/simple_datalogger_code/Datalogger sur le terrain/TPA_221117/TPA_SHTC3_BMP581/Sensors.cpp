/******************************************************************************
 * Arduino - Low Power Board
 * ISSKA www.isska.ch
 * March 2020 by tothseb
******************************************************************************/

#include <Wire.h>

#include "sensors.h"
#include "SparkFun_BMP581_Arduino_Library.h"
#include "SparkFun_SHTC3.h"

uint8_t i2cAddress = BMP581_I2C_ADDRESS_DEFAULT; // 0x47


// A structure to store all sensors values
typedef struct {

  //PHT from MS8607 (in the box)
  float tempBox;
  float humidityBox;
  float tempBMP;
  float pressBMP;
} Mesures;

  


Mesures mesures;

 // Initialize sensors
SHTC3 mySHTC3; 
BMP581 pressureSensor;




Sensors::Sensors() {}
  
/**
 * Initialize sensors
 */



/**
 * Return the file header for the sensors part
 * This should be something like "<sensor 1 name>;<sensor 2 name>;"
 */
String Sensors::getFileHeader () {
  return "temperatureBox[degC];humidityBox[RH%];Temp_BMP[degC];Pressure_BMP[Pa]";
}

/**
 * Return sensors data formated to be write to the CSV file
 * This should be something like "<sensor 1 value>;<sensor 2 value>;"
 */
String Sensors::getFileData () {
  char str[50];

  sprintf(str, "%s;%s;%s;%s",
     String(mesures.tempBox, 3).c_str(), String(mesures.humidityBox, 3).c_str(), String(mesures.tempBMP, 3).c_str(), String(mesures.pressBMP, 3).c_str());

  return str;
}

/**
 * Return sensor data to print on the OLED display
 */
String Sensors::getDisplayData (uint8_t idx) {
  char str[50];

  switch (idx) {
    // Sensor 1 data

    case 0:
      sprintf(str, "TempBox: %s", String(mesures.tempBox).c_str());
      break;
    case 1:
      sprintf(str, "HumBox: %s", String(mesures.humidityBox).c_str());
      break;
    case 2:
      sprintf(str, "TempBMP: %s", String(mesures.tempBMP).c_str());
      break;
    case 3:
      sprintf(str, "PressBMP: %s", String(mesures.pressBMP).c_str());
      break;
    default:
      return "";
  }

  return str;
}

/**
 * Display sensor mesures to Serial for debug purposes
 */
void Sensors::serialPrint() {
  // First sensor
  Serial.print(F("TempBox: "));
  Serial.println(mesures.tempBox);
  
  Serial.print(F("HumBox: "));
  Serial.println(mesures.humidityBox);

  Serial.print(F("TempBMP: "));
  Serial.println(mesures.tempBMP);
  
  Serial.print(F("P_BMP: "));
  Serial.println(mesures.pressBMP);

}

void Sensors::mesure()//This function take approximately 122 sec to compute->Adjust wakeUpInterval in deepsleep function 
{
  Wire.begin();
  pressureSensor.beginI2C(i2cAddress);
  delay(100);
  bmp5_sensor_data data = {0,0};
  pressureSensor.getSensorData(&data);
  mesures.tempBMP = data.temperature;
  mesures.pressBMP = data.pressure;

  mySHTC3.begin();
  mySHTC3.update(); 
  delay(190); 
  mesures.humidityBox=mySHTC3.toPercent();
  mesures.tempBox=mySHTC3.toDegC(); 
}


// multiplex bus selection
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;

  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}

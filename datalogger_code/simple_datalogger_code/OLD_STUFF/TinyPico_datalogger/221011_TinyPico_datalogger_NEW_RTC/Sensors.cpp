/******************************************************************************
 * Arduino - Low Power Board
 * ISSKA www.isska.ch
 * March 2020 by tothseb
******************************************************************************/

#include <Wire.h>

#include "sensors.h"

#include <SparkFun_PHT_MS8607_Arduino_Library.h>





// A structure to store all sensors values
typedef struct {

  float temp1;
  float press1;
  float humidity1;
  float compensated_RH;
  float dew_point;

 
} Mesures;

  


Mesures mesures;

MS8607 barometricSensor;


Sensors::Sensors() {}
  
/**
 * Initialize sensors
 */
// void Sensors::setup(){
//  tcaselect(6);//start the serial communication to the computer
//  Seq.reset();


// }

/**
 * Return the file header for the sensors part
 * This should be something like "<sensor 1 name>;<sensor 2 name>;"
 */
String Sensors::getFileHeader () {
  return "temperature;pressure;Box_humidity";
}

/**
 * Return sensors data formated to be write to the CSV file
 * This should be something like "<sensor 1 value>;<sensor 2 value>;"
 */
String Sensors::getFileData () {
  char str[70];

  sprintf(str, "%s;%s;%s",
     String(mesures.temp1,3).c_str(), String(mesures.press1,2).c_str(), String(mesures.humidity1,2).c_str());

  return str;
}

/**
 * Return sensor data to print on the OLED display
 */
String Sensors::getDisplayData (uint8_t idx) {
  char str[60];

  switch (idx) {
    // Sensor 1 data

    case 0:
      sprintf(str, "Patm: %s", String(mesures.press1).c_str());
      break;
    case 1:
      sprintf(str, "Hum: %s", String(mesures.humidity1).c_str());
      break;
    case 2:
      sprintf(str, "Temp: %s", String(mesures.temp1).c_str());
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
  
  Serial.print(F("Temp: "));
  Serial.println(mesures.temp1,3);

  Serial.print(F("Patm: "));
  Serial.println(mesures.press1,2);
  
  Serial.print(F("Hum: "));
  Serial.println(mesures.humidity1,2);

  }

void Sensors::mesure() {
    
  tcaselect(7);
  Wire.begin();
  barometricSensor.begin();
  mesures.temp1 = barometricSensor.getTemperature();
  mesures.press1 = barometricSensor.getPressure();
  mesures.humidity1 = barometricSensor.getHumidity();

    
}


// multiplex bus selection
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;

  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}

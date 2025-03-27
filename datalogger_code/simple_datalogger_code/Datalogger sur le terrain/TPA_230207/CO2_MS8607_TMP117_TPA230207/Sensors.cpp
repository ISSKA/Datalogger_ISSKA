/******************************************************************************
 * Arduino - Low Power Board
 * ISSKA www.isska.ch
 * March 2020 by tothseb
******************************************************************************/

#include <Wire.h>

#include "sensors.h"

#include <SparkFun_PHT_MS8607_Arduino_Library.h>
#include "SparkFun_SCD4x_Arduino_Library.h" 
#include <SparkFun_TMP117.h> 



// A structure to store all sensors values
typedef struct {

  float temp1;
  float press1;
  float humidityBOX;
  uint16_t co2;
  float temp, humidity;
  float tempHP;

} Mesures;

  


Mesures mesures;

 // Initialize sensors
SCD4x sensor1(SCD4x_SENSOR_SCD41);
MS8607 barometricSensor;
TMP117 sensor;


Sensors::Sensors() {}
  
/**
 * Initialize sensors
 */



/**
 * Return the file header for the sensors part
 * This should be something like "<sensor 1 name>;<sensor 2 name>;"
 */
String Sensors::getFileHeader () {
  return "temperature;temp_HP(+-0.1°C);pressure(mbar);humidityBOX;CO2;";
}

/**
 * Return sensors data formated to be write to the CSV file
 * This should be something like "<sensor 1 value>;<sensor 2 value>;"
 */
String Sensors::getFileData () {
  char str[80];

  sprintf(str, "%s;%s;%s;%s;%s",
     String(mesures.temp1, 3).c_str(),String(mesures.tempHP, 5).c_str(), String(mesures.press1, 3).c_str(), String(mesures.humidityBOX, 3).c_str(), String(mesures.co2).c_str());

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
      sprintf(str, "Temp: %s", String(mesures.temp1).c_str(),3);
      break;
    case 1:
      sprintf(str, "T_HP: %s", String(mesures.tempHP).c_str(),3);
      break;
    case 2:
      sprintf(str, "Patm: %s", String(mesures.press1).c_str());
      break;
    case 3:
      sprintf(str, "HumB: %s", String(mesures.humidityBOX).c_str());
      break;
    case 4:
      sprintf(str, "CO2: %s", String(mesures.co2).c_str());
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
  Serial.println(mesures.temp1);

  Serial.print(F("T_HP: "));
  Serial.println(mesures.tempHP);

  Serial.print(F("Patm: "));
  Serial.println(mesures.press1);
  
  Serial.print(F("HumB: "));
  Serial.println(mesures.humidityBOX);
  
  Serial.print(F("CO2: "));
  Serial.println(mesures.co2);
}

void Sensors::mesure() {
// CO2 SCD41 Sensirion
  uint16_t co2;
  float temp, humidity;
    
  uint16_t error;
  char errorMessage[256];
  sensor1.begin(false, false, false);
  sensor1.measureSingleShot();
  delay(10000);

  sensor1.readMeasurement();
  mesures.co2=sensor1.getCO2();

  Wire.begin();
  sensor.begin();
  mesures.tempHP = sensor.readTempC();
  
  Wire.begin();
  barometricSensor.begin();
  delay(100);
  mesures.temp1 = barometricSensor.getTemperature();
  mesures.press1 = barometricSensor.getPressure();
  mesures.humidityBOX = barometricSensor.getHumidity();

}


// multiplex bus selection
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;

  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}

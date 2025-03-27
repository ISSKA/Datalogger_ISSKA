/******************************************************************************
 * Arduino - Low Power Board
 * ISSKA www.isska.ch
 * March 2020 by tothseb
******************************************************************************/

#include <Wire.h>

#include "sensors.h"

#include <SparkFun_PHT_MS8607_Arduino_Library.h>
#include "SparkFun_MS5803_01BA.h"



// A structure to store all sensors values
typedef struct {

  float temp1;
  float press1;
  float humidity1;
  float compensated_RH;
  float dew_point;
  float tempW;
  double pressW;
  double WaterLevel;

 
} Mesures;

  


Mesures mesures;

MS8607 barometricSensor;
MS5803 sensor1(ADDRESS_HIGH);



Sensors::Sensors() {}
  
/**
 * Initialize sensors
 */
 void Sensors::setup(){
  tcaselect(0);
  sensor1.reset();
  sensor1.begin();


 }

/**
 * Return the file header for the sensors part
 * This should be something like "<sensor 1 name>;<sensor 2 name>;"
 */
String Sensors::getFileHeader () {
  return "temperature;pressure;Humidity;TempW;PressW;WaterLevel_cm";
}

/**
 * Return sensors data formated to be write to the CSV file
 * This should be something like "<sensor 1 value>;<sensor 2 value>;"
 */
String Sensors::getFileData () {
  char str[50];

  sprintf(str, "%s;%s;%s;%s;%s;%s",
     String(mesures.temp1).c_str(), String(mesures.press1).c_str(), String(mesures.humidity1).c_str(),String(mesures.tempW, 3).c_str(), String(mesures.pressW, 3).c_str(), String(mesures.WaterLevel, 3).c_str());

  return str;
}

/**
 * Return sensor data to print on the OLED display
 */
String Sensors::getDisplayData (uint8_t idx) {
  char str[40];

  switch (idx) {
    // Sensor 1 data
    case 0:
      sprintf(str, "PressW: %s", String(mesures.pressW,2).c_str());
      break;
    case 1:
      sprintf(str, "TempW: %s", String(mesures.tempW,2).c_str());
      break;
    case 2:
      sprintf(str, "Patm: %s", String(mesures.press1).c_str());
      break;
    case 3:
      sprintf(str, "TempA: %s", String(mesures.temp1).c_str());
      break;
    case 4:
      sprintf(str, "HumiA: %s", String(mesures.humidity1).c_str());
      break;
    case 5:
      sprintf(str, "WatLev: %s", String(mesures.WaterLevel,2).c_str());
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
  Serial.print(F("PressW: "));
  Serial.println(mesures.pressW,2);

  Serial.print(F("TempW: "));
  Serial.println(mesures.tempW,2);
  
  Serial.print(F("Temp: "));
  Serial.println(mesures.temp1);

  Serial.print(F("Patm: "));
  Serial.println(mesures.press1);
  
  Serial.print(F("Humi: "));
  Serial.println(mesures.humidity1);

  Serial.print(F("WatLev: "));
  Serial.println(mesures.WaterLevel,2);

  }

void Sensors::mesure() {

  tcaselect(7);
  delay(50);
  float p2=0;
  if(barometricSensor.begin())
  {
    delay(100);
    //barometricSensor.setResolution(ms5637_resolution_osr_8192);
    delay(100);
    mesures.temp1 = barometricSensor.getTemperature();
    mesures.press1 = barometricSensor.getPressure();
    mesures.humidity1 = barometricSensor.getHumidity();
    p2=mesures.press1;
  }
  else
  {
    Serial.println("I2C of MS8607: Acknowledgment failed !!");
  }
  tcaselect(5);
  delay(100);
  sensor1.reset();
  sensor1.begin();
  mesures.tempW = sensor1.getTemperature(CELSIUS, ADC_256);
  delay(100);
  // Read pressure from the sensor in mbar.
  mesures.pressW = sensor1.getPressure(ADC_512);
  double p1=sensor1.getPressure(ADC_512);
  delay(100);
  mesures.WaterLevel=(p1-p2)*1.01972;
}


// multiplex bus selection
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;

  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}

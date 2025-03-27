/******************************************************************************
 * Arduino - Low Power Board
 * ISSKA www.isska.ch
 * March 2020 by tothseb
******************************************************************************/

#include <Wire.h>

#include "sensors.h"

#include <SparkFun_PHT_MS8607_Arduino_Library.h>
#include <SensirionI2CSfmSf06.h>




// A structure to store all sensors values
typedef struct {

  float temp1;
  float press1;
  float humidity1;
  float velSFM;
  float debitSFM;
  float tempSFM;

 
} Mesures;

  


Mesures mesures;

MS8607 barometricSensor;
SensirionI2CSfmSf06 sfmSf06;


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
  return "temperature;pressure;Box_humidity;velocitySFM;debitSFM;tempSFM";
}

/**
 * Return sensors data formated to be write to the CSV file
 * This should be something like "<sensor 1 value>;<sensor 2 value>;"
 */
String Sensors::getFileData () {
  char str[70];

  sprintf(str, "%s;%s;%s;%s;%s;%s;",
     String(mesures.temp1,3).c_str(), String(mesures.press1,2).c_str(), String(mesures.humidity1,2).c_str(), String(mesures.velSFM, 7).c_str(),String(mesures.debitSFM, 5).c_str(),String(mesures.tempSFM, 2).c_str());

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
      sprintf(str, "V_SFM: %s", String(mesures.debitSFM,4).c_str());
      break;
    case 1:
      sprintf(str, "D_SFM: %s", String(mesures.velSFM,4).c_str());
      break;
    case 2:
      sprintf(str, "Patm: %s", String(mesures.press1).c_str());
      break;
    case 3:
      sprintf(str, "Hum: %s", String(mesures.humidity1).c_str());
      break;
    case 4:
      sprintf(str, "Temp: %s", String(mesures.temp1).c_str());
      break;
    case 5:
      sprintf(str, "TempSFM: %s", String(mesures.tempSFM).c_str());
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

  Serial.print(F("DebSFM: "));
  Serial.println(mesures.debitSFM,4);
  
  Serial.print(F("VelSFM: "));
  Serial.println(mesures.velSFM,4);

  Serial.print(F("TempSFM: "));
  Serial.println(mesures.tempSFM,2);
  
  Serial.print(F("Temp: "));
  Serial.println(mesures.temp1,3);

  Serial.print(F("Patm: "));
  Serial.println(mesures.press1,2);
  
  Serial.print(F("Hum: "));
  Serial.println(mesures.humidity1,2);

  }

void Sensors::mesure() {
  Serial.println("Entering mesure function.");
  //MS8607 PHT  
  tcaselect(7);
  barometricSensor.begin();
  delay(100);
  mesures.temp1 = barometricSensor.getTemperature();
  mesures.press1 = barometricSensor.getPressure();
  mesures.humidity1 = barometricSensor.getHumidity();

  //SFM 3003
  sfmSf06.begin(Wire, ADDR_SFM3003_300_CE);
  delay(100);
//  sfmSf06.stopContinuousMeasurement();
//  delay(100);
  sfmSf06.startO2ContinuousMeasurement();
  delay(500);
  int16_t temperatureRaw = 0;
  uint16_t status = 0;
  float SFMmoyenne = 0;
  int16_t flowRaw=0;
  
    delay(100);
    sfmSf06.readMeasurementDataRaw(flowRaw, temperatureRaw, status);
    delay(1000);
    mesures.debitSFM=(float(flowRaw)+12288)/120;
    //SFMmoyenne +=flowcorr;
	  mesures.velSFM=((mesures.debitSFM/1000)/60)/0.00030791;
   mesures.tempSFM = float(temperatureRaw)/200;
}


// multiplex bus selection
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;

  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}

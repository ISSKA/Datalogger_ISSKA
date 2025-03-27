/******************************************************************************
 * Arduino - Low Power Board
 * ISSKA www.isska.ch
 * March 2020 by tothseb
******************************************************************************/

#include <Wire.h>

#include "sensors.h"

//#include "SDP3x.h"
#include <SparkFun_PHT_MS8607_Arduino_Library.h>
#include "SparkFun_SCD4x_Arduino_Library.h" 
#include <SparkFun_ADS122C04_ADC_Arduino_Library.h>



// A structure to store all sensors values
typedef struct {

  float temp1;
  float press1;
  uint16_t co2;
  float temp, humidity;
  float tempPT100;
} Mesures;

  


Mesures mesures;

 // Initialize sensors
SCD4x sensor1(SCD4x_SENSOR_SCD41);
MS8607 barometricSensor;
SFE_ADS122C04 mySensor;



Sensors::Sensors() {}
  
/**
 * Initialize sensors
 */



/**
 * Return the file header for the sensors part
 * This should be something like "<sensor 1 name>;<sensor 2 name>;"
 */
String Sensors::getFileHeader () {
  return "temperature;pressure;tempCO2;humidity;CO2;T_PT100;";
}

/**
 * Return sensors data formated to be write to the CSV file
 * This should be something like "<sensor 1 value>;<sensor 2 value>;"
 */
String Sensors::getFileData () {
  char str[50];

  sprintf(str, "%s;%s;%s;%s;%s;%s",
     String(mesures.temp1, 3).c_str(), String(mesures.press1, 3).c_str(), String(mesures.temp, 3).c_str(), String(mesures.humidity, 3).c_str(), String(mesures.co2).c_str(),
     String(mesures.tempPT100, 4).c_str());

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
      sprintf(str, "Temp: %s", String(mesures.temp1).c_str());
      break;
    case 1:
      sprintf(str, "Patm: %s", String(mesures.press1).c_str());
      break;
    case 2:
      sprintf(str, "Temp: %s", String(mesures.temp).c_str());
      break;
    case 3:
      sprintf(str, "Humi: %s", String(mesures.humidity).c_str());
      break;
    case 4:
      sprintf(str, "CO2: %s", String(mesures.co2).c_str());
      break;
    case 5:
      sprintf(str, "PT100: %s", String(mesures.tempPT100,3).c_str());
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

  Serial.print(F("Patm: "));
  Serial.println(mesures.press1);
  
  Serial.print(F("Temp: "));
  Serial.println(mesures.temp);

  Serial.print(F("Humidity: "));
  Serial.println(mesures.humidity);
  
  Serial.print(F("CO2: "));
  Serial.println(mesures.co2);

  Serial.print(F("PT100: "));
  Serial.println(mesures.tempPT100,4);
}

void Sensors::mesure() {
// CO2 SCD41 Sensirion
  uint16_t co2;
  float temp, humidity;
  tcaselect(7);
    
    uint16_t error;
    char errorMessage[256];
    sensor1.begin(false, false, false);
   sensor1.measureSingleShot();
   delay(10000);

    sensor1.readMeasurement();

 mesures.co2=sensor1.getCO2();
 mesures.temp=sensor1.getTemperature();
  //scd4x.powerDown();

  Serial.println();

  Serial.print(F("CO2(ppm):"));
  Serial.print(sensor1.getCO2());

  Serial.print(F("\tTemperature(C):"));
  Serial.print(sensor1.getTemperature(), 1);

  Serial.print(F("\tHumidity(%RH):"));
  Serial.print(sensor1.getHumidity(), 1);

  Serial.println();

  
  tcaselect(0);
  Wire.begin();
  barometricSensor.begin();
  delay(100);
  mesures.temp1 = barometricSensor.getTemperature();
  mesures.press1 = barometricSensor.getPressure();
  mesures.humidity = barometricSensor.getHumidity();

  tcaselect(5);
  mySensor.begin();
  mySensor.configureADCmode(ADS122C04_4WIRE_MODE);
  mesures.tempPT100 = mySensor.readPT100Centigrade();
  mySensor.powerdown();
}


// multiplex bus selection
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;

  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}

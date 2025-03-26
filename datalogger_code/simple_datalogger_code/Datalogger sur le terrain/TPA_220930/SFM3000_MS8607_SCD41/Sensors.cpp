/******************************************************************************
 * Arduino - Low Power Board
 * ISSKA www.isska.ch
 * March 2020 by tothseb
******************************************************************************/

#include <Wire.h>

#include "sensors.h"
#include "SFM3000.h"

//#include "SDP3x.h"
#include <SparkFun_PHT_MS8607_Arduino_Library.h>




// A structure to store all sensors values
typedef struct {

  float temp1;
  float press1;
  float humidityBOX;
  uint16_t co2;
  float temp, humidity;
  float debit;
  double velocity;
} Mesures;

  


Mesures mesures;
int offset = 32000; // Offset for the sensor
float scale = 140.0; // Scale factor for Air and N2 is 140.0, O2 is 142.8

 // Initialize sensors
MS8607 barometricSensor;
SFM3000 sfm3000;



Sensors::Sensors() {}
  
/**
 * Initialize sensors
 */
void Sensors::setup() {
  // SFM3000
    sfm3000.begin();
}


/**
 * Return the file header for the sensors part
 * This should be something like "<sensor 1 name>;<sensor 2 name>;"
 */
String Sensors::getFileHeader () {
  return "temperature;pressure;humidityBOX;debit;velocity;";
}

/**
 * Return sensors data formated to be write to the CSV file
 * This should be something like "<sensor 1 value>;<sensor 2 value>;"
 */
String Sensors::getFileData () {
  char str[70];

  sprintf(str, "%s;%s;%s;%s;%s",
     String(mesures.temp1, 3).c_str(), String(mesures.press1, 3).c_str(), String(mesures.humidityBOX, 3).c_str(),String(mesures.debit, 8).c_str(), String(mesures.velocity, 8).c_str());

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
      sprintf(str, "HumB: %s", String(mesures.humidityBOX).c_str());
      break;
	  case 3:
      sprintf(str, "Deb: %s", String(mesures.debit, 4).c_str());
      break;
    case 4:
      sprintf(str, "Vel: %s", String(mesures.velocity, 4).c_str());
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
  
  Serial.print(F("HumB: "));
  Serial.println(mesures.humidityBOX);
  
  Serial.print(F("Debit: "));
  Serial.println(mesures.debit, 8);

  Serial.print(F("Velocity: "));
  Serial.println(mesures.velocity, 8);
}

void Sensors::mesure() {



  
  tcaselect(7);
  Wire.begin();
  barometricSensor.begin();
  delay(100);
  mesures.temp1 = barometricSensor.getTemperature();
  mesures.press1 = barometricSensor.getPressure();
  mesures.humidityBOX = barometricSensor.getHumidity();
  
    // SFM3000
  
//  float SFMmoyenne = 0;
  //tcaselect(0);
//  delay(100);
//  measflow.init();
//  delay(1500);
//  for (int i = 0; i < NOMBRE_ECHANTILLONS; i++) {
//    delay(200);
//    unsigned int result = measflow.getvalue();
//     delay(500);
//    SFMmoyenne += (float)result - offset;
  //}
tcaselect(3);
  float FlowSFM = sfm3000.getMeasurement();
  //Calcul de la vitesse en m/s en divisant par la surface du tube de mesure dont le diamètre est de 19.8 mm
  float FlowSFM_ms=((FlowSFM/1000)/60)/0.00030791;
  
  mesures.debit = FlowSFM;
  mesures.velocity = FlowSFM_ms;


}


// multiplex bus selection
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;

  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}

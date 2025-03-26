/******************************************************************************
 * Arduino - Low Power Board
 * ISSKA www.isska.ch
 * March 2020 by tothseb
******************************************************************************/

#include <Wire.h>

#include "sensors.h"
#include "SFM3000.h"
#include <SparkFun_PHT_MS8607_Arduino_Library.h>




// A structure to store all sensors values
typedef struct {

  float temp1;
  float press1;
  float humidity1;
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
  return "temperature;pressure;Humidity;Debit;Velocity";
}

/**
 * Return sensors data formated to be write to the CSV file
 * This should be something like "<sensor 1 value>;<sensor 2 value>;"
 */
String Sensors::getFileData () {
  char str[50];

  sprintf(str, "%s;%s;%s;%s;%s",
     String(mesures.temp1).c_str(), String(mesures.press1).c_str(), String(mesures.humidity1).c_str(),String(mesures.debit, 8).c_str(), String(mesures.velocity, 8).c_str());

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
      sprintf(str, "Temp: %s", String(mesures.temp1,2).c_str());
      break;
    case 1:
      sprintf(str, "Hum: %s", String(mesures.humidity1,2).c_str());
      break;
    case 2:
      sprintf(str, "Patm: %s", String(mesures.press1).c_str());
      break;
    case 3:
      sprintf(str, "Deb.: %s", String(mesures.debit,4).c_str());
      break;
    case 4:
      sprintf(str, "Vel.: %s", String(mesures.velocity,4).c_str());
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
  Serial.print(F("Press: "));
  Serial.println(mesures.press1,2);

  Serial.print(F("Temp: "));
  Serial.println(mesures.temp1,2);
  
  Serial.print(F("Hum: "));
  Serial.println(mesures.humidity1);

  Serial.print(F("Deb.: "));
  Serial.println(mesures.debit,6);
  
  Serial.print(F("Vel.: "));
  Serial.println(mesures.velocity,6);

  }

void Sensors::mesure() {
 
 
  tcaselect(7);
  delay(50);

  barometricSensor.begin();
  
    //barometricSensor.setResolution(ms5637_resolution_osr_8192);
    delay(100);
    mesures.temp1 = barometricSensor.getTemperature();
    mesures.press1 = barometricSensor.getPressure();
    mesures.humidity1 = barometricSensor.getHumidity();
  delay(200);
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

//This function returns all the measured values by the sensors as float numbers
void Sensors::getMeasuredValues(float arr[][Nb_values_sensors+2], int RowSelected )
{
  arr[RowSelected][2] = mesures.temp1;
  arr[RowSelected][3] = mesures.press1;
  arr[RowSelected][4] = mesures.humidity1;
  arr[RowSelected][5] = mesures.debit;
  arr[RowSelected][6] = mesures.velocity;
}

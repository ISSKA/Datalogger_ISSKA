/******************************************************************************
   Arduino - Low Power Board
   ISSKA www.isska.ch
   March 2020 by tothseb
******************************************************************************/

#include <Wire.h>

#include "sensors.h"

//#include "SDP3x.h"
//#include <SparkFun_PHT_MS8607_Arduino_Library.h>

const uint8_t bufferlen = 32;                         //total buffer size for the response_data array
char response_data[bufferlen];                        //character array to hold the response data from modules
String inputstring = "";

// A structure to store all sensors values
typedef struct {

  float temp1;
  float press1;
  float humidityBOX;
  uint16_t co2;
  float temp, humidity;
  float tempHP;
  float O2;
} Mesures;




Mesures mesures;

// Initialize sensors
//SensirionI2CScd4x scd4x;
//MS8607 barometricSensor;
//TMP117 sensor;
//Ezo_uart Module1(Serial2, "O");


Sensors::Sensors() {}

/**
   Initialize sensors
*/



/**
   Return the file header for the sensors part
   This should be something like "<sensor 1 name>;<sensor 2 name>;"
*/
String Sensors::getFileHeader () {
  return "temperature;temp_HP;pressure;humidityBOX;humidity;CO2;O2;";
}

/**
   Return sensors data formated to be write to the CSV file
   This should be something like "<sensor 1 value>;<sensor 2 value>;"
*/
String Sensors::getFileData () {
  char str[50];

  sprintf(str, "%s;%s;%s;%s;%s;%s;%s",
          String(mesures.temp1, 3).c_str(), String(mesures.tempHP, 5).c_str(), String(mesures.press1, 3).c_str(), String(mesures.humidityBOX, 3).c_str(), String(mesures.humidity, 3).c_str(), String(mesures.co2).c_str(), String(mesures.O2).c_str());

  return str;
}

/**
   Return sensor data to print on the OLED display
*/
String Sensors::getDisplayData (uint8_t idx) {
  char str[50];

  switch (idx) {
    // Sensor 1 data

    case 0:
      sprintf(str, "Temp: %s", String(mesures.temp1).c_str(), 3);
      break;
    case 1:
      sprintf(str, "T_HP: %s", String(mesures.tempHP).c_str(), 3);
      break;
    case 2:
      sprintf(str, "Patm: %s", String(mesures.press1).c_str());
      break;
    case 3:
      sprintf(str, "HumB: %s", String(mesures.humidityBOX).c_str());
      break;
    case 4:
      sprintf(str, "Humi: %s", String(mesures.humidity).c_str());
      break;
    case 5:
      sprintf(str, "CO2: %s", String(mesures.co2).c_str());
      break;
    case 6:
      sprintf(str, "O2: %s", String(mesures.O2).c_str());
      break;
    default:
      return "";
  }

  return str;
}

/**
   Display sensor mesures to Serial for debug purposes
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

  Serial.print(F("Humidity: "));
  Serial.println(mesures.humidity);

  Serial.print(F("CO2: "));
  Serial.println(mesures.co2);

  Serial.print(F("O2: "));
  Serial.println(mesures.O2);
}

void Sensors::mesure() {
  // CO2 SCD41 Sensirion
  //  uint16_t co2;
  //  float temp, humidity;
  //  tcaselect(0);
  //
  //    uint16_t error;
  //    char errorMessage[256];
  //    scd4x.begin(Wire);
  //    scd4x.wakeUp();
  //
  //    // stop potentially previously started measurement
  //    scd4x.stopPeriodicMeasurement();
  //    scd4x.measureSingleShot();
  //
  //    delay(5500);
  //      // Read Measurement
  //    error = scd4x.readMeasurement(co2, temp, humidity);
  //    mesures.humidity=humidity;
  //    mesures.co2=co2;
  //
  //  scd4x.powerDown();
  //
  //  tcaselect(7);
  //  Wire.begin();
  //  sensor.begin();
  //  mesures.tempHP = sensor.readTempC();
  //
  //  tcaselect(5);
  //  Wire.begin();
  //  barometricSensor.begin();
  //  delay(100);
  //  mesures.temp1 = barometricSensor.getTemperature();
  //  mesures.press1 = barometricSensor.getPressure();
  //  mesures.humidityBOX = barometricSensor.getHumidity();
  ///**********
  //O2 mesure
  //**********/
  //  delay(100);
  //  Serial2.begin(9600, SERIAL_8N1, TX, RX);
  //  Module1.send_read();
  //  delay(100);
  //  mesures.O2=Module1.get_reading();
  mesures.temp1=1;
  mesures.press1=2;
  mesures.humidityBOX=3;
  mesures.co2=4;
  mesures.temp=5;
  mesures.humidity=6;
  mesures.tempHP=7;
  mesures.O2=8;

}


// multiplex bus selection
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;

  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}

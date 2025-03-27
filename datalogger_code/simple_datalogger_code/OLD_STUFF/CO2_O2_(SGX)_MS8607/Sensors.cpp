/******************************************************************************
   Arduino - Low Power Board
   ISSKA www.isska.ch
   March 2020 by tothseb
******************************************************************************/

#include <Wire.h>

#include "sensors.h"

//#include "SDP3x.h"
#include <SparkFun_PHT_MS8607_Arduino_Library.h>
#include <SensirionI2CScd4x.h>
#include <Adafruit_ADS1X15.h>
#define ADC_MAX 32768 //ADC 16 bits mais 1 bit de signe -> 2^15 = 32768
#define V_REF 2.048 // 2x gain   +/- 2.048V -> V_REF = 2.048





// A structure to store all sensors values
typedef struct {

  float tempBox;
  float pressBox;
  float humidityBox;
  uint16_t co2;
  float temperatureCO2;
  float humidityCO2;
  float o2;
} Mesures;




Mesures mesures;

// Initialize sensors
SensirionI2CScd4x scd4x;
MS8607 barometricSensor;
Adafruit_ADS1115 ads1115;  // Construct an ads1115 






Sensors::Sensors() {}

/**
   Initialize sensors
*/



/**
   Return the file header for the sensors part
   This should be something like "<sensor 1 name>;<sensor 2 name>;"
*/
String Sensors::getFileHeader () {
  return "temperature;pressure;humidityBOX;CO2;O2;tempCO2;humiCO2";
}

/**
   Return sensors data formated to be write to the CSV file
   This should be something like "<sensor 1 value>;<sensor 2 value>;"
*/
String Sensors::getFileData () {
  char str[50];

  sprintf(str, "%s;%s;%s;%s;%s;%s;%s",
          String(mesures.tempBox, 3).c_str(), String(mesures.pressBox, 3).c_str(), String(mesures.humidityBox, 3).c_str(), String(mesures.co2).c_str(), String(mesures.o2, 2).c_str(), String(mesures.temperatureCO2, 2).c_str(), String(mesures.humidityCO2, 2).c_str());

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
      sprintf(str, "Temp: %s", String(mesures.tempBox).c_str());
      break;
    case 1:
      sprintf(str, "Patm: %s", String(mesures.pressBox).c_str());
      break;
    case 2:
      sprintf(str, "HumB: %s", String(mesures.humidityBox).c_str());
      break;
    case 3:
      sprintf(str, "CO2: %s", String(mesures.co2).c_str());
      break;
    case 4:
      sprintf(str, "O2: %s", String(mesures.o2).c_str());
      break;
    case 5:
      sprintf(str, "TempCO2: %s", String(mesures.temperatureCO2).c_str());
      break;
    case 6:
      sprintf(str, "HumCO2: %s", String(mesures.humidityCO2).c_str());
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
  Serial.println(mesures.tempBox);

  Serial.print(F("Patm: "));
  Serial.println(mesures.pressBox);

  Serial.print(F("HumB: "));
  Serial.println(mesures.humidityBox);

  Serial.print(F("O2: "));
  Serial.println(mesures.o2);

  Serial.print(F("CO2: "));
  Serial.println(mesures.co2);
  
  Serial.print(F("TempCO2: "));
  Serial.println(mesures.temperatureCO2);

  Serial.print(F("HumCO2: "));
  Serial.println(mesures.humidityCO2);
}

void Sensors::mesure() {
  // CO2 SCD41 Sensirion
  uint16_t co2;
  float temp, humidity;

  uint16_t error;
  char errorMessage[256];
  scd4x.begin(Wire);
  scd4x.wakeUp();

  // stop potentially previously started measurement
  scd4x.stopPeriodicMeasurement();
  scd4x.measureSingleShot();

  delay(5500);
  // Read Measurement
  error = scd4x.readMeasurement(co2, temp, humidity);
  mesures.humidityCO2 = humidity;
  mesures.co2 = co2;
  mesures.temperatureCO2 = temp;

  scd4x.powerDown();


  Wire.begin();
  barometricSensor.begin();
  delay(100);
  mesures.tempBox = barometricSensor.getTemperature();
  mesures.pressBox = barometricSensor.getPressure();
  mesures.humidityBox = barometricSensor.getHumidity();


  //O2 measurement
  //SGX-40X sensor: https://www.sgxsensortech.com/content/uploads/2022/04/DS-0143-SGX-4OX-datasheet-v5.pdf
  delay(10000);
  Serial.println("Start of O2 measurement");
  float adcValue, voltage;
  ads1115.setGain(GAIN_TWO);// 2x gain   +/- 2.048V -> V_REF = 2.048
  ads1115.begin();
  delay(100);
  adcValue=ads1115.readADC_SingleEnded(0);
  Serial.print("ADC count value: ");Serial.println(adcValue);
  voltage = adcValue/ADC_MAX*V_REF;
  float O2_voltage_tcomp=0.0020703953*mesures.tempBox+1.08898;
  Serial.print("Voltage: ");Serial.println(voltage);
  //O2 sensor is the SGX-40X
  //0% -> 0V    18.4% -> 1.2V
  //Mettre un meilleur calcul ici + compensation de la température
  mesures.o2 = 18.4/1.2*O2_voltage_tcomp;
}


// multiplex bus selection
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;

  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}

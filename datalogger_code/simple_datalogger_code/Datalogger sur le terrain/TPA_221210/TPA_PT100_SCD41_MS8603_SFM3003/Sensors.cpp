/******************************************************************************
 * Arduino - Low Power Board
 * ISSKA www.isska.ch
 * March 2020 by tothseb
******************************************************************************/

#include <Wire.h>

#include "sensors.h"
#include <SparkFun_PHT_MS8607_Arduino_Library.h>
#include "SparkFun_SCD4x_Arduino_Library.h" 
#include <SparkFun_ADS122C04_ADC_Arduino_Library.h>




// A structure to store all sensors values
typedef struct {

  //PHT from MS8607 (in the box)
  float tempBox;
  float humidityBox;
  float pressBox;
  uint16_t co2;
  float tempPT100;

  
} Mesures;

  


Mesures mesures;

 // Initialize sensors
MS8607 barometricSensor;
SFE_ADS122C04 mySensor;
SCD4x sensor1(SCD4x_SENSOR_SCD41);





Sensors::Sensors() {}
  
/**
 * Initialize sensors
 */



/**
 * Return the file header for the sensors part
 * This should be something like "<sensor 1 name>;<sensor 2 name>;"
 */
String Sensors::getFileHeader () {
  return "temperatureBox[degC];humidityBox[RH%];Pression_Box;T_PT100;CO2;";
}

/**
 * Return sensors data formated to be write to the CSV file
 * This should be something like "<sensor 1 value>;<sensor 2 value>;"
 */
String Sensors::getFileData () {
  char str[80];

  sprintf(str, "%s;%s;%s;%s;%s;",
     String(mesures.tempBox, 3).c_str(), String(mesures.humidityBox, 3).c_str(), String(mesures.pressBox, 3).c_str(),String(mesures.tempPT100, 4).c_str(),String(mesures.co2).c_str());

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
      sprintf(str, "T_Box: %s", String(mesures.tempBox).c_str());
      break;
    case 1:
      sprintf(str, "H_Box: %s", String(mesures.humidityBox).c_str());
      break;
    case 2:
      sprintf(str, "P_Box: %s", String(mesures.pressBox).c_str());
      break;
    case 3:
      sprintf(str, "PT100: %s", String(mesures.tempPT100,3).c_str());
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
  Serial.print(F("TempBox: "));
  Serial.println(mesures.tempBox);
  
  Serial.print(F("HumBox: "));
  Serial.println(mesures.humidityBox);
  
  Serial.print(F("P_Box: "));
  Serial.println(mesures.pressBox);
  
  Serial.print(F("CO2: "));
  Serial.println(mesures.co2);

  Serial.print(F("PT100: "));
  Serial.println(mesures.tempPT100,4);

}

void Sensors::mesure()//This function take approximately 122 sec to compute->Adjust wakeUpInterval in deepsleep function 
{

    uint16_t co2;
  float temp, humidity;
  tcaselect(5);
    
    uint16_t error;
    char errorMessage[256];
    sensor1.begin(false, false, false);
   sensor1.measureSingleShot();
   delay(10000);

    sensor1.readMeasurement();

 mesures.co2=sensor1.getCO2();
 //mesures.temp=sensor1.getTemperature();

 
  tcaselect(3);
  Wire.begin();
  barometricSensor.begin();
  delay(300);
  mesures.tempBox = barometricSensor.getTemperature();
  mesures.pressBox = barometricSensor.getPressure();
  mesures.humidityBox = barometricSensor.getHumidity();

  tcaselect(7);
  mySensor.begin();
  mySensor.configureADCmode(ADS122C04_4WIRE_MODE);
  mesures.tempPT100 = mySensor.readPT100Centigrade();
  mySensor.powerdown();
  tcaselect(0);

    //SFM 3003

  delay(100);

  delay(500);
  int16_t temperatureRaw = 0;
  uint16_t status = 0;
  int16_t flowRaw=0;
  
  delay(100);


  
}


// multiplex bus selection
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;

  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}

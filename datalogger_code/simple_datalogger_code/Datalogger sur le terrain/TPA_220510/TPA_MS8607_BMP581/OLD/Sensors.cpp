/******************************************************************************
 * Arduino - Low Power Board
 * ISSKA www.isska.ch
 * March 2020 by tothseb
******************************************************************************/

#include <Wire.h>

#include "sensors.h"

//PHT MS8607 sensor library
#include <SparkFun_PHT_MS8607_Arduino_Library.h>
//SVM41 VOC & NOx sensor library
//#include <SensirionI2CSvm41.h>




// A structure to store all sensors values
typedef struct {

  //PHT from MS8607 (in the box)
  float tempBox;
  float pressBox;
  float humidityBox;
  //Temp, humidity, nox, voc from SVM41 (out of the box)
  float humidityAir;
  float tempAir;
  float vocIndex;
  float noxIndex;
} Mesures;

  


Mesures mesures;

 // Initialize sensors
//SensirionI2CSvm41 svm41;
MS8607 barometricSensor;




Sensors::Sensors() {}
  
/**
 * Initialize sensors
 */



/**
 * Return the file header for the sensors part
 * This should be something like "<sensor 1 name>;<sensor 2 name>;"
 */
String Sensors::getFileHeader () {
  return "temperatureBox[degC];pressureBox[mbar];humidityBox[RH%];humidityAir[RH%];tempAir[degC];vocIndex;noxIndex;";
}

/**
 * Return sensors data formated to be write to the CSV file
 * This should be something like "<sensor 1 value>;<sensor 2 value>;"
 */
String Sensors::getFileData () {
  char str[50];

  sprintf(str, "%s;%s;%s;%s;%s;%s;%s",
     String(mesures.tempBox, 3).c_str(), String(mesures.pressBox, 3).c_str(), String(mesures.humidityBox, 3).c_str(), String(mesures.humidityAir, 3).c_str(), String(mesures.tempAir).c_str(),String(mesures.vocIndex).c_str(),String(mesures.noxIndex).c_str());

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
      sprintf(str, "TempBox: %s", String(mesures.tempBox).c_str());
      break;
    case 1:
      sprintf(str, "PressureBox: %s", String(mesures.pressBox).c_str());
      break;
    case 2:
      sprintf(str, "HumBox: %s", String(mesures.humidityBox).c_str());
      break;
    case 3:
      sprintf(str, "HumAir: %s", String(mesures.humidityAir).c_str());
      break;
    case 4:
      sprintf(str, "TempAir: %s", String(mesures.tempAir).c_str());
      break;
    case 5:
      sprintf(str, "VOCIndex: %s", String(mesures.vocIndex).c_str());
      break;
    case 6:
      sprintf(str, "NOxIndex: %s", String(mesures.noxIndex).c_str());
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

  Serial.print(F("PressureBox: "));
  Serial.println(mesures.pressBox);
  
  Serial.print(F("HumBox: "));
  Serial.println(mesures.humidityBox);

  Serial.print(F("HumidityAir: "));
  Serial.println(mesures.humidityAir);
  
  Serial.print(F("TempAir: "));
  Serial.println(mesures.tempAir);
  
  Serial.print(F("VOCIndex: "));
  Serial.println(mesures.vocIndex);

  Serial.print(F("NOxIndex: "));
  Serial.println(mesures.noxIndex);
}

void Sensors::mesure()//This function take approximately 122 sec to compute->Adjust wakeUpInterval in deepsleep function 
{
  //Mesure SVM41
//  uint16_t error;
//  char errorMessage[256];
//  
//  tcaselect(0);
//  Wire.begin();
//  svm41.begin(Wire);
//  error = svm41.deviceReset();
//    if (error) {
//        Serial.print("Error trying to execute deviceReset(): ");
//        errorToString(error, errorMessage, 256);
//        Serial.println(errorMessage);
//    }
//  // Delay to let the serial monitor catch up
//    delay(2000);
//
//    uint8_t serialNumber[32];
//    uint8_t serialNumberSize = 32;
//    error = svm41.getSerialNumber(serialNumber, serialNumberSize);
//    if (error) {
//        Serial.print("Error trying to execute getSerialNumber(): ");
//        errorToString(error, errorMessage, 256);
//        Serial.println(errorMessage);
//    } else {
//        Serial.print("SerialNumber:");
//        Serial.println((char*)serialNumber);
//    }
//    // Start Measurement
//    error = svm41.startMeasurement();
//    if (error) {
//        Serial.print("Error trying to execute startMeasurement(): ");
//        errorToString(error, errorMessage, 256);
//        Serial.println(errorMessage);
//    }
//    //Wait at least 45 sec to have VOC and NOx values (based on the measurements done during these 45sec)
//    Serial.println("Wait 2 minutes until VOC and NOx values are available ...");
//    delay(120000);
    //Read the measurement
    //error = svm41.readMeasuredValues(mesures.humidityAir, mesures.tempAir, mesures.vocIndex, mesures.noxIndex);
//    if (error)
//    {
//        Serial.print("Error trying to execute readMeasuredValues(): ");
//        errorToString(error, errorMessage, 256);
//        Serial.println(errorMessage);
//    }
//  //Stop the measurement
//  svm41.stopMeasurement();

  //Value of the PHT
  mesures.humidityAir=1;
  mesures.tempAir=2;
  mesures.vocIndex=3;
  mesures.noxIndex=4;
// tcaselect(7);
//  Wire.begin();
//  barometricSensor.begin();
//  delay(100);
//  mesures.tempBox = barometricSensor.getTemperature();
//  mesures.pressBox = barometricSensor.getPressure();
//  mesures.humidityBox = barometricSensor.getHumidity();
 mesures.tempBox = 5;
  mesures.pressBox = 6;
  mesures.humidityBox = 7;

}


// multiplex bus selection
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;

  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}

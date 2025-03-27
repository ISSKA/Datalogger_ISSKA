/******************************************************************************
 * Arduino - Low Power Board
 * ISSKA www.isska.ch
 * March 2020 by tothseb
******************************************************************************/

#include <Wire.h>

#include "sensors.h"

#include <SensirionI2CSdp.h>
#include <SparkFun_PHT_MS8607_Arduino_Library.h>
#include <SensirionI2CSfmSf06.h>




// A structure to store all sensors values
typedef struct {

  float temp1;
  float press1;
  float humidity1;
  float compensated_RH;
  float dew_point;
  float P_SDP;
  float Q_SDP;
  float velSFM;
  float debitSFM;

 
} Mesures;

  


Mesures mesures;

MS8607 barometricSensor;
SensirionI2CSfmSf06 sfmSf06;
SensirionI2CSdp sdp;


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
  return "temperature;pressure;Box_humidity;P_SDP;debitSDP;velocitySFM;debitSFM";
}

/**
 * Return sensors data formated to be write to the CSV file
 * This should be something like "<sensor 1 value>;<sensor 2 value>;"
 */
String Sensors::getFileData () {
  char str[70];

  sprintf(str, "%s;%s;%s;%s;%s;%s;%s",
     String(mesures.temp1,3).c_str(), String(mesures.press1,2).c_str(), String(mesures.humidity1,2).c_str(),String(mesures.P_SDP, 7).c_str(),String(mesures.Q_SDP, 7).c_str(), String(mesures.velSFM, 7).c_str(),String(mesures.debitSFM, 5).c_str());

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
      sprintf(str, "P_SDP: %s", String(mesures.P_SDP,4).c_str());
      break;
    case 1:
      sprintf(str, "Q_SDP: %s", String(mesures.Q_SDP,4).c_str());
      break;
    case 2:
      sprintf(str, "V_SFM: %s", String(mesures.debitSFM,4).c_str());
      break;
    case 3:
      sprintf(str, "D_SFM: %s", String(mesures.velSFM,4).c_str());
      break;
    case 4:
      sprintf(str, "Patm: %s", String(mesures.press1).c_str());
      break;
    case 5:
      sprintf(str, "Hum: %s", String(mesures.humidity1).c_str());
      break;
    case 6:
      sprintf(str, "Temp: %s", String(mesures.temp1).c_str());
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
  Serial.print(F("P_SDP: "));
  Serial.println(mesures.P_SDP,4);

   Serial.print(F("DebSDP: "));
  Serial.println(mesures.Q_SDP,4); 

  Serial.print(F("DebSFM: "));
  Serial.println(mesures.debitSFM,4);
  
    Serial.print(F("VelSFM: "));
  Serial.println(mesures.velSFM,4);
  
  Serial.print(F("Temp: "));
  Serial.println(mesures.temp1,3);

  Serial.print(F("Patm: "));
  Serial.println(mesures.press1,2);
  
  Serial.print(F("Hum: "));
  Serial.println(mesures.humidity1,2);

  }

void Sensors::mesure() {
    
  tcaselect(7);
  Wire.begin();
  barometricSensor.begin();
  delay(100);
  mesures.temp1 = barometricSensor.getTemperature();
  mesures.press1 = barometricSensor.getPressure();
  mesures.humidity1 = barometricSensor.getHumidity();

  sdp.begin(Wire, SDP3X_I2C_ADDRESS_0);
  sdp.stopContinuousMeasurement();
  delay(200);
  sdp.startContinuousMeasurementWithDiffPressureTCompAndAveraging();
  float differentialPressure;
  float temperatureSDP;
  sdp.readMeasurement(differentialPressure, temperatureSDP);

  mesures.P_SDP =differentialPressure;
  mesures.Q_SDP=((-0.0000000003 * pow(mesures.P_SDP, 6)) + (0.0000001 * pow(mesures.P_SDP, 5)) - (0.00003 * pow(mesures.P_SDP, 4)) + (0.0024 * pow(mesures.P_SDP,3))-(0.1099 * pow(mesures.P_SDP,2))+(3.127*mesures.P_SDP));
  //float FlowSDPspeed = sqrt(2 * 1.225 * abs(FlowSDP));
    //if (FlowSDP < 0) {
    // Si la valeur d'origine était négative, applique le négatif au résultat qui sort de la racine
    //FlowSDPspeed *= -1;
  //}
  //mesures.velocitySDP = FlowSDPspeed;
  delay(100);

  //tcaselect(0);
  sfmSf06.begin(Wire, ADDR_SFM3003_300_CE);
  delay(100);
//  sfmSf06.stopContinuousMeasurement();
//  delay(100);
  sfmSf06.startO2ContinuousMeasurement();
  delay(500);
  float temperature = 0;
  uint16_t status = 0;
  float SFMmoyenne = 0;
  float flow=0;
  
    delay(100);
    sfmSf06.readMeasurementData(flow, temperature, status);
    delay(1000);
    mesures.debitSFM=(flow+12288)/120;
    //SFMmoyenne +=flowcorr;
	  mesures.velSFM=((mesures.debitSFM/1000)/60)/0.00030791;
    

    
}


// multiplex bus selection
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;

  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}

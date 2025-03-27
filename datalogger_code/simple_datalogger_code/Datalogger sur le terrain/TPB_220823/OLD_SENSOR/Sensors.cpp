

/******************************************************************************
   Arduino - Low Power Board
   ISSKA www.isska.ch
   August 2022 - MS8607 & water level (https://wiki.dfrobot.com/Throw-in_Type_Liquid_Level_Transmitter_SKU_KIT0139) sensors
   
   ⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠
   The frequency of the I2C need to be at least 100'000 Hz otherwise the relay doesn't respond
   #define I2C_FREQUENCY 100000
   Wire.setClock(I2C_FREQUENCY); and u8x8.setBusClock(I2C_FREQUENCY);
   ⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠⚠
******************************************************************************/

#include <Wire.h>

#include "sensors.h"

//Library for the MS8607 PHT sensor
#include <SparkFun_PHT_MS8607_Arduino_Library.h>
//library for the 16 bits ADC (ADS1115)
#include <Adafruit_ADS1X15.h>
//Library for the relay
#include <SparkFun_Qwiic_Relay.h>
#define RELAY_ADDR 0x18

//Setting of the water level sensor
#define RANGE 5000 // Depth measuring range 5000mm (for water)
#define CURRENT_INIT 4.00 // Current @ 0mm (uint: mA)
#define DENSITY_WATER 1  // Pure water density normalized to 1

// A structure to store all sensors values
typedef struct {

  float temp1;
  float press1;
  float humidity1;
  float waterDepth;
} Mesures;




Mesures mesures;

// Initialize sensors
MS8607 barometricSensor;
Adafruit_ADS1115 ads1115;  // Construct an ads1115
Qwiic_Relay relay(RELAY_ADDR);




Sensors::Sensors() {}

/**
   Initialize sensors
*/



/**
   Return the file header for the sensors part
   This should be something like "<sensor 1 name>;<sensor 2 name>;"
*/
String Sensors::getFileHeader () {
  return "temperature [degC];pressure [mbar];humidity [%];waterDepth [mm];";
}

/**
   Return sensors data formated to be write to the CSV file
   This should be something like "<sensor 1 value>;<sensor 2 value>;"
*/
String Sensors::getFileData () {
  char str[50];

  sprintf(str, "%s;%s;%s;%s",
          String(mesures.temp1, 3).c_str(), String(mesures.press1, 3).c_str(), String(mesures.humidity1, 3).c_str(), String(mesures.waterDepth, 3).c_str());

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
      sprintf(str, "Temp: %s", String(mesures.temp1).c_str());
      break;
    case 1:
      sprintf(str, "Patm: %s", String(mesures.press1).c_str());
      break;
    case 2:
      sprintf(str, "HumB: %s", String(mesures.humidity1).c_str());
      break;
    case 3:
      sprintf(str, "Wdepth: %s", String(mesures.waterDepth).c_str());
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

  Serial.print(F("Patm: "));
  Serial.println(mesures.press1);

  Serial.print(F("Humi: "));
  Serial.println(mesures.humidity1);

  Serial.print(F("WaterDepth: "));
  Serial.println(mesures.waterDepth);
}

void Sensors::mesure() {
  Serial.println("Entering mesure function !");
  
  //Water depth measurement
  if(!relay.begin())
  {
    Serial.println("I2C communication with Relay failed !");
  }
  if(!ads1115.begin(0x48))
  {
    Serial.println("I2C communication with ADC failed !");
  }
    // 1x gain   +/- 4.096V  1 bit = 0.125mV
  ads1115.setGain(GAIN_ONE);
  Serial.print("ADS1115 gain: ");
  Serial.println(ads1115.getGain());
  //Turn on the relay
  //uint8_t isRelayOn = 0;
//  while(isRelayOn ==0)
//  {
//    relay.turnRelayOn();
//    isRelayOn = relay.getState();
//    Serial.print("Relay is: ");
//    Serial.println(isRelayOn);
//    delay(50);
//  }
  relay.turnRelayOn();
  
  Serial.println("relay turned on !");
  delay(1500);
  
  int16_t ADC_count = ads1115.readADC_SingleEnded(3);//Sensors output is connected to A3 input
  Serial.print("Analog Value (count):"); Serial.println(ADC_count);
  float dataVoltage = ads1115.computeVolts(ADC_count) * 1000; //*1000 to convert V to mV
  Serial.print("dataVoltage (mV)"); Serial.println(dataVoltage);
  float dataCurrent = dataVoltage / 120.0; //Sense Resistor:120ohm
  Serial.print("dataCurrent (mA):"); Serial.println(dataCurrent);
  mesures.waterDepth = (dataCurrent - CURRENT_INIT) * (RANGE / DENSITY_WATER / 16.0); //Calculate depth from current readings

  if (mesures.waterDepth < 0)
  {
    mesures.waterDepth = 0.0;
  }

  //Serial print results
  Serial.print("depth:");
  Serial.print(mesures.waterDepth);
  Serial.println("mm");
  Serial.println("--------------------------------");
  relay.turnRelayOff();
  delay(100);

  
  //MS8607 measurement (PHT)
  Serial.println("tcaselect made!");
  if(!barometricSensor.begin())
  {
    Serial.println("I2C communication with MS8607 failed !");
  }
  delay(100);
  mesures.temp1 = barometricSensor.getTemperature();
  mesures.press1 = barometricSensor.getPressure();
  mesures.humidity1 = barometricSensor.getHumidity();
  Serial.println("PHT measurement made !");
  Serial.println("mesure function finished !");

}


// multiplex bus selection
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;

  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}

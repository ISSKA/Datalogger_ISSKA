/* *******************************************************************************
Code for datalogger with MS8607 and MS5803 connected at 40m (needs slower i2c frequency)


Here are some important remarks when modifying to add or  add or remove a sensor:
    - in Sensors::getFileData () don't forget to add a "%s;" for each new sensor 
    - adjust the struct, getFileHeader (), getFileData (), serialPrint() and mesure() with the sensor used

The conf.txt file must have the following structure:
  300; //deepsleep time in seconds (put at least 120 seconds)
  1; //boolean value to choose if you want to set the RTC clock with time from computer (put 1 when sending the code, but put 0 then to avoid resetting the time when pushing the reboot button)
  2023/03/31 16:05:00; //if you choose SetRTC = 1, press the reboot button at the written time to set the RTC at this time. Then check that the logger correctly initialised and then set SetRTC to 0 on the SD card to not set the time every time you push the reboot button

If you have any questions contact me per email at nicolas.schmid.6035@gmail.com
**********************************************************************************
*/ 


#include <Wire.h>
#include "Sensors.h"
#include <SparkFun_PHT_MS8607_Arduino_Library.h>
#include <SensorModbusMaster.h>
#include "KellerModbus.h"

#define I2C_MUX_ADDRESS 0x70

#define RX_485 32
#define TX_485 33
kellerModel model = Acculevel_kellerModel;
byte modbusAddress = 0x01;

const int PwrPin = 22;  // The pin sending power to the sensor *AND* RS485 adapter
const int DEREPin = -1;

// A structure to store all sensors values
typedef struct {
  float tempAIR;
  float pressAIR;
  float hum;
  float T_eau;
  float P_eau;
  float HtEau;
} Mesures;

Mesures mesures;

//these two sensor measure temperature, humidity and pressur
MS8607 barometricSensorEXT; //initialise MS8607 external sensor 
keller sensor1;

Sensors::Sensors() {}
 
String Sensors::getFileHeader () { // Return the file header for the sensors part. This should be something like "<sensor 1 name>;<sensor 2 name>;"
  return (String)"tempAir;pressAir;hum;T_eau;P_eau;HtEau;"; //change if you add sensors !!!!!!!!
}

String Sensors::getFileData () { // Return sensors data formated to be write to the CSV file. This should be something like "<sensor 1 value>;<sensor 2 value>;"
  char str[200];
  sprintf(str, "%s;%s;%s;%s;%s;%s;", //"temp", "press", "Hum" , ... put as many %s; as there are sensors!!!!!! don't forget the last ";" !!
     String(mesures.tempAIR).c_str(), String(mesures.pressAIR).c_str(), String(mesures.hum).c_str(), String(mesures.T_eau).c_str(), String(mesures.P_eau,3).c_str(), String(mesures.HtEau,3).c_str());
  return str;
}

String Sensors::serialPrint() { //Display sensor mesures to Serial for debug purposes
  String sensor_values_str = "temp: " + String(mesures.tempAIR,1) + "\n";
  sensor_values_str = sensor_values_str + "p_air: " + String(mesures.pressAIR,2) + "\n";
  sensor_values_str = sensor_values_str + "hum: " + String(mesures.hum,2) + "\n";
  sensor_values_str = sensor_values_str + "t_eau: " + String(mesures.T_eau,2) + "\n";
  sensor_values_str = sensor_values_str + "p_eau: " + String(mesures.P_eau,3) + "\n";
  sensor_values_str = sensor_values_str + "HtEau: " + String(mesures.HtEau,3) + "\n";
  Serial.print(sensor_values_str);
  return sensor_values_str;
  //add a line for each sensor!!!!!!!!!
}

void Sensors::mesure() {
  //connect and start the MS8067 external sensor 
  //Mesure Keller 26x 60m
  pinMode(PwrPin, OUTPUT);
  digitalWrite(PwrPin, HIGH);
      if (DEREPin > 0) pinMode(DEREPin, OUTPUT);
  Serial1.begin(9600, SERIAL_8N1, RX_485, TX_485);  // The modbus serial stream - Baud rate MUST be 9600.
    
  float waterPressureBar;
  float waterTempertureC;
  float waterDepthM;
    // Start up the modbus sensor
  sensor1.begin(model, modbusAddress, &Serial1, DEREPin);
  delay (1000);
  sensor1.getValues(waterPressureBar, waterTempertureC);
  waterDepthM = sensor1.calcWaterDepthM(waterPressureBar, waterTempertureC);
  mesures.P_eau =waterPressureBar*1000;  //value in mbar
  mesures.HtEau =waterDepthM*100;  //value in cm 
  mesures.T_eau=waterTempertureC;
  delay(100);
  //Serial1.end();
  delay(100);
  tcaselect(2);
  delay(500);
  Wire.begin();
//mesure sonde barometric MS8607
  // mesures.P_eau =1;  //value in mbar
  // mesures.HtEau =2;  //value in cm 
  // mesures.T_eau=3;
  tcaselect(6);
  delay(100);
  barometricSensorEXT.begin();
  delay(100);
  //read values from the 3 sensors on the MS8067
  mesures.tempAIR = barometricSensorEXT.getTemperature();
  delay(50);
  mesures.pressAIR = barometricSensorEXT.getPressure();
  delay(50);
  mesures.hum = barometricSensorEXT.getHumidity();
  delay(50);


  tcaselect(0); //do not forget to put something else than 5 otherwise rtc doesn't work  
  delay(50);
  //put here the measurement of other sensors!!!!!!
}

// multiplex bus selection for the first multiplexer 
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}




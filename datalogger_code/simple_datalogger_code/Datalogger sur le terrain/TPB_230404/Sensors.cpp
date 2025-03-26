
/* *******************************************************************************
Code for TPB_230404 datalogger with MS8607, TMP117, AO121AU sonar

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
#include <SparkFun_TMP117.h> 

#define I2C_MUX_ADDRESS 0x70
#define TX_sonar 33
#define RX_sonar 32

// A structure to store all sensors values
typedef struct {
  float tempM;
  float press;
  float hum;
  float temp117;
  float distmm;
} Mesures;

Mesures mesures;

//these two sensor measure temperature, humidity and pressur
MS8607 barometricSensor; //initialise MS8607 external sensor 
TMP117 temp_sensor; // Initalize sensor

Sensors::Sensors() {}
 
String Sensors::getFileHeader () { // Return the file header for the sensors part. This should be something like "<sensor 1 name>;<sensor 2 name>;"
  return (String)"tempM;press;Hum;temp117;distmm;"; //change if you add sensors !!!!!!!!
}

String Sensors::getFileData () { // Return sensors data formated to be write to the CSV file. This should be something like "<sensor 1 value>;<sensor 2 value>;"
  char str[200];
  sprintf(str, "%s;%s;%s;%s;%s;", //"tempExt", "pressEXT", "HumExt" , ... put as many %s; as there are sensors!!!!!! don't forget the last ";" !!
     String(mesures.tempM).c_str(), String(mesures.press).c_str(), String(mesures.hum).c_str(),String(mesures.temp117).c_str(),String(mesures.distmm).c_str());
  return str;
}

String Sensors::serialPrint() { //Display sensor mesures to Serial for debug purposes
  String sensor_values_str = "tempM: " + String(mesures.tempM) + "\n";
  sensor_values_str = sensor_values_str + "press: " + String(mesures.press) + "\n";
  sensor_values_str = sensor_values_str + "hum: " + String(mesures.hum) + "\n";
  sensor_values_str = sensor_values_str + "temp117: " + String(mesures.temp117) + "\n";
  sensor_values_str = sensor_values_str + "distmm: " + String(mesures.distmm) + "\n";
  Serial.print(sensor_values_str);
  return sensor_values_str;
  //add a line for each sensor!!!!!!!!!
}

void Sensors::mesure() {

  //read values from the 3 sensors on the MS8067
  tcaselect(7);
  delay(100);
  barometricSensor.begin();
  mesures.tempM = barometricSensor.getTemperature();
  mesures.press = barometricSensor.getPressure();
  mesures.hum = barometricSensor.getHumidity();
  delay(1000);


    //measure distance sonnar (UART)
  int16_t distance_mm = -1;  // The last measured distance
  bool newData = false; // Whether new data is available from the sensor
  uint8_t buffer[4];  // our buffer for storing data
  uint8_t idx = 0;  // our idx into the storage buffer
  Serial1.begin(9600, SERIAL_8N1, TX_sonar, RX_sonar);
  while (!newData){
    if (Serial1.available()){
      uint8_t c = Serial1.read();
      // See if this is a header byte
      if (idx == 0 && c == 0xFF) buffer[idx++] = c;
      // Two middle bytes can be anything
      else if ((idx == 1) || (idx == 2)) buffer[idx++] = c;
      else if (idx == 3) {
        uint8_t sum = 0;
        sum = buffer[0] + buffer[1] + buffer[2];
        if (sum == c) {
          distance_mm = ((uint16_t)buffer[1] << 8) | buffer[2];
          newData = true;
        }
        idx = 0;
      }
    }
  }
  mesures.distmm = distance_mm;
  delay(1000);

  //read values from the sensor TMP117 (more accurate for temperature)    
  tcaselect(6);
  delay(100);
  temp_sensor.begin();
  mesures.temp117 = temp_sensor.readTempC();
  delay(100);


  //put here the measurement of other sensors!!!!!!

}


// multiplex bus selection for the first multiplexer 
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}



/* *******************************************************************************
Base code for simple datalogger with only MS8607


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
#include <SparkFun_ADS122C04_ADC_Arduino_Library.h>
#include <Ezo_i2c.h>
#include <Ezo_i2c_util.h>

#define I2C_MUX_ADDRESS 0x70
#define I2C_ATLAS_HUMIDITY_ADRESS  111              //default I2C ID number for EZO Humidity sensor.

char Humidity_data[32];          //we make a 32-byte character array to hold incoming data from the Humidity sensor.
char *HUMID;                     //char pointer used in string parsing.
char *TMP;                       //char pointer used in string parsing.
char *NUL;                       //char pointer used in string parsing (the sensor outputs some text that we don't need).
char *DEW;                       //char pointer used in string parsing.

float HUMID_float;               //float var used to hold the float value of the humidity.
float TMP_float;                 //float var used to hold the float value of the temperature.
float DEW_float;                 //float var used to hold the float value of the dew point.

// A structure to store all sensors values
typedef struct {
  float tempEXT;
  float pressEXT;
  float humEXT;
  float PT100;
  float HUM;
  float TMP;
  float DEW;

} Mesures;

Mesures mesures;
//these two sensor measure temperature, humidity and pressur
MS8607 barometricSensorEXT; //initialise MS8607 external sensor 
SFE_ADS122C04 PT100_sensor;
Ezo_board HUM = Ezo_board(111, "HUM");

Sensors::Sensors() {}
 
String Sensors::getFileHeader () { // Return the file header for the sensors part. This should be something like "<sensor 1 name>;<sensor 2 name>;"
  return (String)"tempEXT;pressEXT;HumEXT;PT100;HUM;TMP;DEW;"; //change if you add sensors !!!!!!!!
}

String Sensors::getFileData () { // Return sensors data formated to be write to the CSV file. This should be something like "<sensor 1 value>;<sensor 2 value>;"
  char str[200];
  sprintf(str, "%s;%s;%s;%s;%s;%s;%s;", //"tempExt", "pressEXT", "HumExt" , ... put as many %s; as there are sensors!!!!!! don't forget the last ";" !!
     String(mesures.tempEXT,3).c_str(), String(mesures.pressEXT,3).c_str(), String(mesures.humEXT,3).c_str(),String(mesures.PT100,4).c_str(),String(mesures.HUM,2).c_str(), String(mesures.TMP,2).c_str(),String(mesures.DEW,2).c_str());
  return str;
}

String Sensors::serialPrint() { //Display sensor mesures to Serial for debug purposes
  String sensor_values_str = "temp: " + String(mesures.tempEXT) + "\n";
  sensor_values_str = sensor_values_str + "press: " + String(mesures.pressEXT) + "\n";
  sensor_values_str = sensor_values_str + "hum: " + String(mesures.humEXT) + "\n";
  sensor_values_str = sensor_values_str + "PT100: " + String(mesures.PT100) + "\n";
  sensor_values_str = sensor_values_str + "HUM: " + String(mesures.HUM) + "\n";
  sensor_values_str = sensor_values_str + "TMP: " + String(mesures.TMP) + "\n";
  sensor_values_str = sensor_values_str + "DEW: " + String(mesures.DEW) + "\n";
  Serial.print(sensor_values_str);
  return sensor_values_str;
  //add a line for each sensor!!!!!!!!!
}

void Sensors::mesure() {
  //connect and start the MS8067 external sensor 
  tcaselect(6);
  delay(100);
  barometricSensorEXT.begin();
  //read values from the 3 sensors on the MS8067
  mesures.tempEXT = barometricSensorEXT.getTemperature();
  mesures.pressEXT = barometricSensorEXT.getPressure();
  mesures.humEXT = barometricSensorEXT.getHumidity();
  delay(100);

  tcaselect(5); //PT100
  PT100_sensor.begin();
  PT100_sensor.configureADCmode(ADS122C04_4WIRE_MODE);
  mesures.PT100 = PT100_sensor.readPT100Centigrade();
  PT100_sensor.powerdown();
  delay(100);


  tcaselect(7); //atlas scientific humidity
  delay(1400); //delays are here somehow important otherwise it reads 0
delay(1000);
  HUM.send_cmd("o,t,1");        //send command to enable temperature output
  delay(1000);
  HUM.send_cmd("o,dew,1");      //send command to enable dew point output
  delay(1500);
  HUM.send_cmd("r");
  delay(1500);
  HUM.receive_cmd(Humidity_data, 32);
  delay(300);       //put the response into the buffer

  HUMID = strtok(Humidity_data, ",");       //let's pars the string at each comma.
  TMP = strtok(NULL, ",");                  //let's pars the string at each comma.
  NUL = strtok(NULL, ",");                  //let's pars the string at each comma (the sensor outputs the word "DEW" in the string, we dont need it.
  DEW = strtok(NULL, ","); 
  HUMID_float=atof(HUMID);
  TMP_float=atof(TMP);
  DEW_float=atof(DEW);                 //let's pars the string at each comma.
  mesures.HUM=HUMID_float;
  mesures.TMP=TMP_float;
  mesures.DEW=DEW_float;
  delay(100);
  tcaselect(2); //atlas scientific humidity
}

// multiplex bus selection for the first multiplexer 
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}
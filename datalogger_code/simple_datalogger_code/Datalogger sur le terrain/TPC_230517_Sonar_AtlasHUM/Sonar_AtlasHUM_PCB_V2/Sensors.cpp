/* *******************************************************************************
Base code for datalogger with PCB design v2.6 There are two sensors already installed on this PCB, the SHT35 and the BMP388

Here are some important remarks when modifying to add or  add or remove a sensor:
    - in Sensors::getFileData () don't forget to add a "%s;" for each new sensor 
    - adjust the struct, getFileHeader (), getFileData (), serialPrint() and mesure() with the sensor used

The conf.txt file must have the following structure:
  300; //deepsleep time in seconds (put at least 120 seconds)
  1; //boolean value to choose if you want to set the RTC clock with time from SD card
  2023/03/31 16:05:00; //time to set the RTC if setRTC = 1

If you have any questions contact me per email at nicolas.schmid.6035@gmail.com
**********************************************************************************
*/  

#include <Wire.h>
#include "Sensors.h"
#include "SHT31.h"
#include <Adafruit_Sensor.h>
#include "Adafruit_BMP3XX.h"
#include <Ezo_i2c.h>
#include <Ezo_i2c_util.h>

#define I2C_MUX_ADDRESS 0x72 //not working on PCB v2.6 (hardware design problem)
#define BMP388_sensor_ADRESS 0x76 //values set on the PCB hardware (not possible to change)
#define SHT35_sensor_ADRESS 0x44 //values set on the PCB hardware (not possible to change)

#define TX_sonar 33
#define RX_sonar 32

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
  float tempSHT;
  float press;
  float hum;
  float tempBMP;
  float distmm;
  float HUM;
  float TMP;
  float DEW;
} Mesures;

Mesures mesures;

//these two sensor measure temperature, humidity and pressur
SHT31 sht;
Adafruit_BMP3XX bmp;
Ezo_board HUM = Ezo_board(111, "HUM");

Sensors::Sensors() {}
 
String Sensors::getFileHeader () { // Return the file header for the sensors part. This should be something like "<sensor 1 name>;<sensor 2 name>;"
  return (String)"tempSHT;pressBMP;humSHT;tempBMP;distmm;HUM;TMP;DEW"; //change if you add sensors !!!!!!!!
}

String Sensors::getFileData () { // Return sensors data formated to be write to the CSV file. This should be something like "<sensor 1 value>;<sensor 2 value>;"
  char str[200];
  sprintf(str, "%s;%s;%s;%s;%s;%s;%s;%s;", //"temp", "press", "Hum" , ... put as many %s; as there are sensors!!!!!! don't forget the last ";" !!
     String(mesures.tempSHT).c_str(), String(mesures.press).c_str(), String(mesures.hum).c_str(),String(mesures.tempBMP).c_str(),String(mesures.distmm).c_str(),String(mesures.HUM,3).c_str(),String(mesures.TMP,3).c_str(),String(mesures.DEW).c_str());
  return str;
}

String Sensors::serialPrint() { //Display sensor mesures to Serial for debug purposes
  String sensor_values_str = "tempSHT: " + String(mesures.tempSHT) + "\n";
  sensor_values_str = sensor_values_str + "press: " + String(mesures.press) + "\n";
  sensor_values_str = sensor_values_str + "hum: " + String(mesures.hum) + "\n";
  sensor_values_str = sensor_values_str + "tempBMP: " + String(mesures.tempBMP) + "\n";
  sensor_values_str = sensor_values_str + "distmm: " + String(mesures.distmm) + "\n";
  sensor_values_str = sensor_values_str + "HUM: " + String(mesures.HUM,1) + "\n";
  sensor_values_str = sensor_values_str + "TMP: " + String(mesures.TMP,1) + "\n";
  sensor_values_str = sensor_values_str + "DEW: " + String(mesures.DEW,1) + "\n";
  Serial.print(sensor_values_str);
  return sensor_values_str;
  //add a line for each sensor!!!!!!!!!
}

void Sensors::mesure() {
  //connect and start the SHT35 PCB sensor 
  //connect and start the BMP388 PCB sensor 
  delay(500);
  bmp.begin_I2C(BMP388_sensor_ADRESS);
  delay(500);
  bmp.setTemperatureOversampling(BMP3_OVERSAMPLING_8X);
  bmp.setPressureOversampling(BMP3_OVERSAMPLING_4X);
  bmp.setIIRFilterCoeff(BMP3_IIR_FILTER_COEFF_3);
  bmp.setOutputDataRate(BMP3_ODR_50_HZ);
  delay(300);
  bmp.performReading();
  delay(300);
  mesures.tempBMP = bmp.temperature;
  mesures.press=bmp.pressure / 100.0; //milibar
  delay(2000);


  sht.begin();    //Sensor I2C Address
  delay(100);
  sht.read();
  mesures.tempSHT=sht.getTemperature();
  mesures.hum=sht.getHumidity();
  delay(300);


  //put here the measurement of other sensors!!!!!!

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

  //put here the measurement of other sensors!!!!!!
}

// multiplex bus selection for the first multiplexer 
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}



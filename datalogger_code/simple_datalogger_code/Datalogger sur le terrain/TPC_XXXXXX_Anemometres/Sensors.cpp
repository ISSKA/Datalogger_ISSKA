/* *******************************************************************************
code for datalogger with PCB design v2.6 There are two sensors already installed on this PCB, the SHT35 and the BMP388

There are two anemometers: the FS3000 I2C anemometer from sparkfun, which measures winf in only one direction, and the JL-FS2 mechanical anemometer from
DFrobot, which measures wind in all directions, but only if the value is higher than 0.8m/s. It unfortunately only communicates with RS485 so we need a converter from
RS485 to UART

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
#include "Adafruit_BMP3XX.h"

#include <SparkFun_FS3000_Arduino_Library.h>
#include <SoftwareSerial.h>


SoftwareSerial mySerial(33, 32); //Define RX and TX ports

#define I2C_MUX_ADDRESS 0x72 //not working on PCB v2.6 (hardware design problem)
#define BMP388_sensor_ADRESS 0x76 //values set on the PCB hardware (not possible to change)
#define SHT35_sensor_ADRESS 0x44 //values set on the PCB hardware (not possible to change)

// A structure to store all sensors values
typedef struct {
  float tempSHT;
  float press;
  float hum;
  float tempBMP;
  float speed;
  float DFspeed;
} Mesures;

Mesures mesures;

//these two sensor measure temperature, humidity and pressur
SHT31 sht;
Adafruit_BMP3XX bmp;

FS3000 air_velocity_sensor;

Sensors::Sensors() {}
 
String Sensors::getFileHeader () { // Return the file header for the sensors part. This should be something like "<sensor 1 name>;<sensor 2 name>;"
  return (String)"tempSHT;press;hum;tempBMP;speed;DFspeed;"; //change if you add sensors !!!!!!!!
}

String Sensors::getFileData () { // Return sensors data formated to be write to the CSV file. This should be something like "<sensor 1 value>;<sensor 2 value>;"
  char str[200];
  sprintf(str, "%s;%s;%s;%s;%s;%s;", //"temp", "press", "Hum" , ... put as many %s; as there are sensors!!!!!! don't forget the last ";" !!
     String(mesures.tempSHT).c_str(), String(mesures.press).c_str(), String(mesures.hum).c_str(),String(mesures.tempBMP).c_str(),String(mesures.speed).c_str(),String(mesures.DFspeed).c_str());
  return str;
}

String Sensors::serialPrint() { //Display sensor mesures to Serial for debug purposes
  String sensor_values_str = "tempSHT: " + String(mesures.tempSHT) + "\n";
  sensor_values_str = sensor_values_str + "press: " + String(mesures.press) + "\n";
  sensor_values_str = sensor_values_str + "hum: " + String(mesures.hum) + "\n";
  sensor_values_str = sensor_values_str + "tempBMP: " + String(mesures.tempBMP) + "\n";
  sensor_values_str = sensor_values_str + "speed: " + String(mesures.speed) + "\n";
  sensor_values_str = sensor_values_str + "DFspeed: " + String(mesures.DFspeed) + "\n";
  Serial.print(sensor_values_str);
  return sensor_values_str;
  //add a line for each sensor!!!!!!!!!
}

void Sensors::mesure() {
  //connect and start the SHT35 PCB sensor 
  sht.begin(SHT35_sensor_ADRESS);    //Sensor I2C Address
  delay(100);
  sht.read();
  mesures.tempSHT=sht.getTemperature();
  mesures.hum=sht.getHumidity();
  delay(300);

  //connect and start the BMP388 PCB sensor 
  bmp.begin_I2C(BMP388_sensor_ADRESS);
  delay(100);
  bmp.performReading();
  delay(100);
  mesures.tempBMP = bmp.temperature;
  mesures.press=bmp.pressure / 100.0; //milibar
  delay(300);

  //sparkfun air velocity sensor
  air_velocity_sensor.begin();
  air_velocity_sensor.setRange(AIRFLOW_RANGE_7_MPS);
  delay(100);
  mesures.speed=air_velocity_sensor.readMetersPerSecond();
  delay(100);

  //DF robot anemometer
  mySerial.begin(9600);
  delay(500);
  mesures.DFspeed= readWindSpeed(0x10);


  //put here the measurement of other sensors!!!!!!
}


size_t readN(uint8_t *buf, size_t len)
{
  size_t offset = 0, left = len;
  int16_t Tineout = 1500;
  uint8_t  *buffer = buf;
  long curr = millis();
  while (left) {
    if (mySerial.available()) {
      buffer[offset] = mySerial.read();
      offset++;
      left--;
    }
    if (millis() - curr > Tineout) {
      break;
    }
  }
  return offset;
}

float readWindSpeed(uint8_t Address)
{
  uint8_t Data[7] = {0};   //Store raw data packets returned by sensors
  uint8_t COM[8] = {0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00};   //command to read wind speed
  boolean ret = false;   //Wind speed acquisition success flag
  float WindSpeed = 0;
  long curr = millis();
  long curr1 = curr;
  uint8_t ch = 0;
  COM[0] = Address;    //Refer to the communication protocol to complete the instruction package.

  uint16_t crc = 0xFFFF;
  for (int pos = 0; pos < 6; pos++)
  {
    crc ^= (uint16_t)COM[pos];
    for (int i = 8; i != 0; i--)
    {
      if ((crc & 0x0001) != 0)
      {
        crc >>= 1;
        crc ^= 0xA001;
      }
      else
      {
        crc >>= 1;
      }
    }
  }
  COM[6] = crc % 0x100;
  COM[7] = crc / 0x100;
  mySerial.write(COM, 8);  //Send command to read wind speed

  while (!ret) {
    if (millis() - curr > 1000) {
      WindSpeed = -1;    //If the wind speed has not been read for more than 1000 milliseconds, it will be considered a timeout and return -1.
      break;
    }

    if (millis() - curr1 > 100) {
      mySerial.write(COM, 8);  //If the last command to read the wind speed is issued for more than 100 milliseconds and the return command has not been received, the command to read the wind speed will be sent again
      curr1 = millis();
    }
    if (readN(&ch, 1) == 1) {
      if (ch == Address) {          //read and judge the HEAD
        Data[0] = ch;
        if (readN(&ch, 1) == 1) {
          if (ch == 0x03) {         //read and judge the HEAD
            Data[1] = ch;
            if (readN(&ch, 1) == 1) {
              if (ch == 0x02) {       //read and judge the HEAD
                Data[2] = ch;      
                if (readN(&Data[3], 4) == 4) {
                  uint16_t crc = 0xFFFF;
                  for (int pos = 0; pos < 5; pos++)
                  {
                    crc ^= (uint16_t)Data[pos];
                    for (int i = 8; i != 0; i--)
                    {
                      if ((crc & 0x0001) != 0)
                      {
                        crc >>= 1;
                        crc ^= 0xA001;
                      }
                      else
                      {
                        crc >>= 1;
                      }
                    }
                  }
                  crc = ((crc & 0x00ff) << 8) | ((crc & 0xff00) >> 8);
                  if (crc == (Data[5] * 256 + Data[6])) {  //CRC
                    ret = true;
                    WindSpeed = (Data[3] * 256 + Data[4]) / 10.00;  //calculate the windspeed
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return WindSpeed;
}

// multiplex bus selection for the first multiplexer 
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}



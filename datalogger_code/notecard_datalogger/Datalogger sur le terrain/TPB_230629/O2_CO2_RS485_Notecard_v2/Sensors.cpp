#include "WString.h"
//#include "HardwareSerial.h"
#include "pins_arduino.h"
#include "esp32-hal-gpio.h"
#include <Ezo_uart.h>
/* *******************************************************************************
Code for isska-logger v1.0 with Notecard, with two external O2 and CO2 sensors. We can now remotely change some parameters on the conf.txt file remotely.

This code was entirely rewritten by Nicolas Schmid. Here are some important remarks when modifying to add or  add or remove a sensor:
    - adjust the array sensor_names which is in this .ino file
    - in Sensors::getFileData () don't forget to add a "%s;" for each new sensor (if you forget the ";" it will read something completely wrong)
    - adjust the struct, getFileHeader (), getFileData (), serialPrint() and mesure() with the sensor used
    - in Sensors.h don't forget to adjust the number of sensors in the variable num_sensors. This is actually rather a count of the number of sotred measurement
      values.For example if you have an oxygen sensor and you measure the voltage but also directy compute the O2 percentage, this counts as 2 measurements

The conf.txt file must have the following structure:
  300; //deepsleep time in seconds (put at least 240 seconds)
  1; //boolean value to choose if you want to set the RTC clock with GSM time (recommended to put 1)
  12; // number of measurements to be sent at once

If you want to change the deep sleep time or the number of measurements to be sent at once remotely, go to https://notehub.io and under "Fleet" select your device.
Then check the little square to have a blue check in it, then press on the "+Note" icon. A popup will appear with the title "Add note to device". You must select "data.qi" as notefile
and in the JSON note write (with the brackets) {"deep_sleep_time":120} or {"deep_sleep_time":1800} or {"MAX_MEASUREMENTS":10} etc. It will write this value on the sd card of the datalogger
the next time it tries to send data over gsm, or also when you reboot it.

If you have any questions contact me per email at nicolas.schmid.6035@gmail.com
**********************************************************************************
*/   

#include <Wire.h>
#include "Sensors.h"
#include "SHT31.h"
#include "SparkFun_BMP581_Arduino_Library.h"

#define UART_O2 Serial1
#define UART_CO2 Serial2

Ezo_uart O2sensor(UART_O2, "O");
Ezo_uart CO2sensor(UART_CO2, "CO2");

#define RX_485 32
#define TX_485 33
#define SERIAL_PORT_EXPANDER_SELECTOR 27
#define MOSFET_SENSORS_PIN 26

#define I2C_MUX_ADDRESS 0x73 //this is the adress in the hardware of the PCB V3.2
#define BMP581_sensor_ADRESS 0x46 //values set on the PCB hardware (not possible to change)
#define SHT35_sensor_ADRESS 0x44 //values set on the PCB hardware (not possible to change)
//#define EXT_SENSORS_MOSFET 26 //allow current to flow in external sensor's box
//#define SELECT_RXTX 27 //low selects P1, high selects P2

//#define RX 32
//#define TX 33

// A structure to store all sensors values
typedef struct {
  float tempSHT;
  float pressBMP;
  float humSHT;
  float tempBMP;
  float ppmCO2;
  float percO2;
  float SensorVbat;
} Mesures;

Mesures mesures;

//these two sensor measure temperature, humidity and pressur
SHT31 sht;
BMP581 bmp;


//SoftwareSerial myserial(RX,TX);
//SoftwareSerial myserial2(RX,TX);

Sensors::Sensors() {}
 
String Sensors::getFileHeader () { // Return the file header for the sensors part. This should be something like "<sensor 1 name>;<sensor 2 name>;"
  return (String)"tempSHT;pressBMP;humSHT;tempBMP;ppmCO2;percO2;"; //change if you add sensors !!!!!!!!
}

String Sensors::getFileData () { // Return sensors data formated to be write to the CSV file. This should be something like "<sensor 1 value>;<sensor 2 value>;"
  char str[200];
  sprintf(str, "%s;%s;%s;%s;%s;%s;", //"temp", "press", "Hum" , ... put as many %s; as there are sensors!!!!!! don't forget the last ";" !!
     String(mesures.tempSHT).c_str(), String(mesures.pressBMP).c_str(), String(mesures.humSHT).c_str(),String(mesures.tempBMP).c_str(),String(mesures.ppmCO2).c_str(),String(mesures.percO2).c_str());
  return str;
}

String Sensors::serialPrint() { //Display sensor mesures to Serial for debug purposes
  String sensor_values_str = "tempSHT: " + String(mesures.tempSHT) + "\n";
  sensor_values_str = sensor_values_str + "pressBMP: " + String(mesures.pressBMP) + "\n";
  sensor_values_str = sensor_values_str + "humSHT: " + String(mesures.humSHT) + "\n";
  sensor_values_str = sensor_values_str + "tempBMP: " + String(mesures.tempBMP) + "\n";
  sensor_values_str = sensor_values_str + "ppmCO2: " + String(mesures.ppmCO2) + "\n";
  sensor_values_str = sensor_values_str + "percO2: " + String(mesures.percO2) + "\n";
  Serial.print(sensor_values_str);
  return sensor_values_str;
  //add a line for each sensor!!!!!!!!!
}

void Sensors::mesure() {
  //connect and start the SHT35 PCB sensor 
  tcaselect(1);
  delay(10);
  sht.begin();    //Sensor I2C Address
  delay(10);
  sht.read();
  mesures.tempSHT=sht.getTemperature();
  mesures.humSHT=sht.getHumidity();
  delay(50);

  //connect and start the BMP581 PCB sensor 
  tcaselect(2);
  delay(10);
  bmp.beginI2C(BMP581_sensor_ADRESS);
  delay(10);
  bmp5_sensor_data data = {0,0};
  int8_t err = bmp.getSensorData(&data);
  mesures.tempBMP=data.temperature;
  mesures.pressBMP=data.pressure/100; //in millibar
  delay(50);
  const uint8_t bufferlen = 32;                         //total buffer size for the response_data array
  char response_data[bufferlen]; 
  int timeOutCounter=0;

  pinMode(MOSFET_SENSORS_PIN,OUTPUT);
  digitalWrite(MOSFET_SENSORS_PIN, HIGH);
  delay(1000);
  // Select channel 1 of serial port expander
  pinMode(SERIAL_PORT_EXPANDER_SELECTOR, OUTPUT);
  digitalWrite(SERIAL_PORT_EXPANDER_SELECTOR, LOW);
  delay(100);
  //O2 measurement
  //O2 sensor is set at baudrate 2400!!
  UART_O2.begin(2400, SERIAL_8N1, RX_485, TX_485);
  delay(200);
  O2sensor.flush_rx_buffer();
  delay(200);
  while(mesures.percO2==0 && timeOutCounter<20)
  {
    O2sensor.send_read_with_temp_comp(10.3); //remplacé la lecture de la sonde T par une T fixe, car capteur T est dehors
    //O2sensor.send_read();
    delay(100);
    mesures.percO2 = O2sensor.get_reading();
    delay(200);
    O2sensor.send_cmd("Status", response_data, bufferlen);
    Serial.println(response_data);
    timeOutCounter++;
  }
  
  //flush the serial port to erase all incoming data
  //O2sensor.flush_rx_buffer();
  UART_O2.end();
  delay(200);

  //Select channel 2 of serial port expander
  digitalWrite(SERIAL_PORT_EXPANDER_SELECTOR, HIGH);
  //UART_CO2.begin(9600,SERIAL_8N1, RX_485, TX_485);
  //CO2 sensor is set at baudrate 2400!!
  UART_CO2.begin(2400, SERIAL_8N1, RX_485, TX_485);
                       //character array to hold the response data from modules
  delay(11000);
  Serial.println("Measure of CO2");
  CO2sensor.flush_rx_buffer();
  delay(200);
  timeOutCounter=0;
  while (mesures.ppmCO2 == 0 && timeOutCounter<20)
  {
    delay(300);
    CO2sensor.send_read();
    delay(100);
    //Lecture des valeurs de CO2 avec un shift de -550 ppm déduit 
    //après comparaison avec autres capteurs et CO2 atmos.
    mesures.ppmCO2 = CO2sensor.get_reading()-855; // -855 de correction après comparaison (06.02.24 Eric)
    O2sensor.send_cmd("Status", response_data, bufferlen);
    Serial.println(response_data);
    timeOutCounter++;
  }
  timeOutCounter=0;
  while ((mesures.SensorVbat == 0 || mesures.SensorVbat > 10)&&timeOutCounter<20)
  {
    CO2sensor.send_cmd("Status", response_data, bufferlen);
    //find the last comma in the array
    char * pch;
    pch = strchr(response_data, ',');
    int index_coma = 0;
    while (pch != NULL)
    {
      Serial.print("found at %d\n");
      Serial.println(pch - response_data + 1);
      index_coma = pch - response_data + 1;
      pch = strchr(pch + 1, ',');
    }
    String tempString = response_data;
    String sensorVoltage = tempString.substring(index_coma);
    mesures.SensorVbat = sensorVoltage.toFloat();
    //CO2sensor.flush_rx_buffer();
    delay(300);
    timeOutCounter++;
  }

  delay(200);
  UART_CO2.end();
  digitalWrite(MOSFET_SENSORS_PIN, LOW);
}

// multiplex bus selection for the first multiplexer 
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}



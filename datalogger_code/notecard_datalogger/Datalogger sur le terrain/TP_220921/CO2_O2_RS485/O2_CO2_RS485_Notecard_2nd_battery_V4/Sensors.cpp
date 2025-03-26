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
#include "SHT31.h"
#include "SparkFun_BMP581_Arduino_Library.h"
#include <Ezo_uart.h>

#define I2C_MUX_ADDRESS 0x73 //not working on PCB v2.6 (hardware design problem)
#define BMP581_sensor_ADRESS 0x46 //values set on the PCB hardware (not possible to change)
#define SHT35_sensor_ADRESS 0x44 //values set on the PCB hardware (not possible to change)
uint8_t i2cAddressBMP = BMP581_I2C_ADDRESS_SECONDARY; // 0x46

#define UART_O2 Serial1
#define UART_CO2 Serial2

#define RX_485 32
#define TX_485 33

#define SERIAL_PORT_EXPANDER_SELECTOR 26
#define MOSFET_SENSORS_PIN 27

Ezo_uart O2sensor(UART_O2, "O");
Ezo_uart CO2sensor(UART_CO2, "CO2");

// A structure to store all sensors values
typedef struct {
  float humSHT;
  float tempSHT;
  float pressBMP;
  float tempBMP;
  float O2;
  float CO2;
  float SensorVbat;
} Mesures;

Mesures mesures;

//these two sensor measure temperature, humidity and pressur
SHT31 sht;
BMP581 bmp;

Sensors::Sensors() {}
 
String Sensors::getFileHeader () { // Return the file header for the sensors part. This should be something like "<sensor 1 name>;<sensor 2 name>;"
  return (String)"tempSHT;pressBMP;humSHT;tempBMP;O2;CO2;SensorVbat;"; //change if you add sensors !!!!!!!!
}

String Sensors::getFileData () { // Return sensors data formated to be write to the CSV file. This should be something like "<sensor 1 value>;<sensor 2 value>;"
  char str[200];
  sprintf(str, "%s;%s;%s;%s;%s;%s;%s;", //"temp", "press", "Hum" , ... put as many %s; as there are sensors!!!!!! don't forget the last ";" !!
     String(mesures.tempSHT).c_str(), String(mesures.pressBMP).c_str(), String(mesures.humSHT).c_str(), String(mesures.tempBMP).c_str(), String(mesures.O2, 3).c_str(), String(mesures.CO2, 3).c_str(), String(mesures.SensorVbat, 3).c_str());
  return str;
}

String Sensors::serialPrint() { //Display sensor mesures to Serial for debug purposes
  String sensor_values_str = "t_SHT: " + String(mesures.tempSHT,1) + "\n";
  sensor_values_str = sensor_values_str + "press: " + String(mesures.pressBMP,1) + "\n";
  sensor_values_str = sensor_values_str + "hum: " + String(mesures.humSHT,1) + "\n";
  sensor_values_str = sensor_values_str + "t_BMP: " + String(mesures.tempBMP,1) + "\n";
  sensor_values_str = sensor_values_str + "O2: " + String(mesures.O2,1) + "\n";
  sensor_values_str = sensor_values_str + "CO2: " + String(mesures.CO2,1) + "\n";

  Serial.print(sensor_values_str);
  return sensor_values_str;
  //add a line for each sensor!!!!!!!!!
}

void Sensors::mesure() {
  //connect and start the MS8067 external sensor 
tcaselect(1);
  sht.begin(SHT35_sensor_ADRESS);    //Sensor I2C Address
  delay(100);
  sht.read();
  mesures.tempSHT=sht.getTemperature();
  mesures.humSHT=sht.getHumidity();
  delay(300);

  tcaselect(2);
  //connect and start the BMP388 PCB sensor 
      while(bmp.beginI2C(i2cAddressBMP) != BMP5_OK)
    {
        // Not connected, inform user
        Serial.println("Error: BMP581 not connected, check wiring and I2C address!");

        // Wait a bit to see if connection is established
        delay(1000);
    }

    Serial.println("BMP581 connected!");
  delay(100);
  //data = {0,0};
  bmp5_sensor_data data = {0,0};
  int8_t err = bmp.getSensorData(&data);
  delay(100);
  mesures.tempBMP = data.temperature;
  mesures.pressBMP = data.pressure/100;
  delay(300);

  const uint8_t bufferlen = 32;                         //total buffer size for the response_data array
  char response_data[bufferlen]; 
  int timeOutCounter=0;
  //Power on the sensors side of the datalogger
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
  while(mesures.O2==0 && timeOutCounter<20)
  {
    O2sensor.send_read_with_temp_comp(10.3); //remplacé la lecture de la sonde T par une T fixe, car capteur T est dehors
    //O2sensor.send_read();
    delay(100);
    mesures.O2 = O2sensor.get_reading();
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
  while (mesures.CO2 == 0 && timeOutCounter<20)
  {
    delay(300);
    CO2sensor.send_read();
    delay(100);
    //Lecture des valeurs de CO2 avec un shift de -550 ppm déduit 
    //après comparaison avec autres capteurs et CO2 atmos.
    mesures.CO2 = CO2sensor.get_reading(); // le -550 a été supprimé, car valeurs pas cohérentes. 
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

  tcaselect(0); //do not forget to put something else than 5 otherwise rtc doesn't work  
  //put here the measurement of other sensors!!!!!!
}

// multiplex bus selection for the first multiplexer 
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}




/* *******************************************************************************
This sensor code is for the two sensors that are installed by default on most PCB.
We have the SHT35 (I2c adress 0x44 and TCA1) and the BMP581 (I2c adress 0x46 and TCA2)

To add new sensors here are the things you must modify in this file:
  - Update the String names[] array with all the measurements names that you need
  - Import the library of your sensor
  - create an instance of the sensors'library class
  - In the measure() method, measure your sensor's value and store the value in the float values[] array.
    You must give the same index to a value than to its name in the names[] arrays.
    E.g. if names={"var0","var1","var2"}, then to store the value of var2 you must write: values[2] = mysensor.getvalue2() 

If you have any questions contact me per email at nicolas.schmid.6035@gmail.com
**********************************************************************************
*/  


//include libraries for the sensors
#include <Wire.h>
#include "Sensors.h"
#include "SHT31.h"
#include "SparkFun_BMP581_Arduino_Library.h"
#include <TinyPICO.h>
#include "Amsler.h"
#include "SparkFun_MS5803_01BA.h"


//parameters which depend on the PCB version
#define I2C_MUX_ADDRESS 0x73 //I2C adress of the multiplexer set on the PCB
#define BMP581_sensor_ADRESS 0x46 //I2C adress of the BMP581 set on the PCB
#define SHT35_sensor_ADRESS 0x44 //I2C adress of the SHT35 set on the PCB

//create an instance of the sensors' classes
TinyPICO tiny = TinyPICO();
SHT31 sht;
BMP581 bmp;
Amsler ams;
MS5803 sensor1(ADDRESS_HIGH);

//declare arrays where the names and values of the sensors are stored
String names[]={"Vbatt","tempSHT","humSHT","tempBMP","pressBMP","velAmsler","tempMS5803","pressMS5803","waterlevel"}; //update this list if you add sensors!!!
const int nb_values = sizeof(names)/sizeof(names[0]);
float values[nb_values];

//return the names array pointer
String* Sensors::get_names(){ 
  return names;
}

//return the number of values
int Sensors::get_nb_values(){ 
  return nb_values;
}

// Return the file header string in the format "<sensor 1 name>;<sensor 2 name>;"
String Sensors::getFileHeader () { 
  String header_string = "";
  for(int i = 0; i< nb_values; i++){
    header_string = header_string + names[i] + ";";
  }
  return header_string; 
}

// Return sensors data string formated to be write to the CSV file. The format is the following: "<sensor 1 value>;<sensor 2 value>;"
String Sensors::getFileData () { 
  String datastring = "";
  for(int i = 0; i< nb_values; i++){
    datastring = datastring + String(values[i],2) + ";";
  }
  return datastring;
}

// Return a string with the name of the sensors and their value to be shown on the display. Also prints the string into the serial"
String Sensors::serialPrint() { //Display sensor mesures to Serial for debug purposes
  String sensor_display_str = "";
  for(int i = 0; i< nb_values; i++){
    sensor_display_str = sensor_display_str + names[i]+": "+String(values[i],1) + "\n";
  }
  Serial.print(sensor_display_str);
  return sensor_display_str;
}

//measure all sensors'values and store them in the values arrays
void Sensors::measure() {
  delay(5); //delay of 5 ms to ensure that sensors are properly powered

  //measure the battery volatge of the battery which powers the datalogger
  values[0]=tiny.GetBatteryVoltage(); //Vbatt

  //connect and start the SHT35 PCB sensor 
  tcaselect(1);
  delay(3); //wait 3ms for the multiplexer to switch
  sht.begin(); 
  sht.read();
  values[1]=sht.getTemperature(); //tempSHT
  values[2]=sht.getHumidity(); //humSHT

  //connect and start the BMP581 PCB sensor 
  tcaselect(2);
  delay(3);
  bmp.beginI2C(BMP581_sensor_ADRESS);
  delay(5);
  bmp5_sensor_data data = {0,0};
  int8_t err = bmp.getSensorData(&data);
  values[3]=data.temperature; //tempBMP
  values[4]=data.pressure/100; //pressBMP (in millibar)

  //Current velocity sensor
  values[5] = ams.measure_vel();
  
  //pressure sensor
  tcaselect(7);
  delay(300);
  sensor1.reset();
  sensor1.begin();
  delay(300);
  values[6] = sensor1.getTemperature(CELSIUS, ADC_256);
  values[7] = sensor1.getPressure(ADC_256);

  float p1 = values[4];
  float p2 = values[7];
  values[8] = (p2-p1)*1.01972;
  delay(300);
  tcaselect(1);

}

// multiplex bus selection for the first multiplexer 
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}








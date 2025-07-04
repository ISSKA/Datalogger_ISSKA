/* *******************************************************************************
Sensor code with one PT100 temperature sensor on the channel 7 of the multiplexer
**********************************************************************************

This sensor code also includes the two sensors that are installed by default on most PCB.
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
#include <SensirionI2cSfmSf06.h>
#include "SparkFun_SCD4x_Arduino_Library.h" 
//parameters which depend on the PCB version
#define I2C_MUX_ADDRESS 0x73 //I2C adress of the multiplexer set on the PCB
#define BMP581_sensor_ADRESS 0x46 //I2C adress of the BMP581 set on the PCB
#define SHT35_sensor_ADRESS 0x44 //I2C adress of the SHT35 set on the PCB

//create an instance of the sensors' classes
TinyPICO tiny = TinyPICO();
SHT31 sht;
BMP581 bmp;
SensirionI2cSfmSf06 sfmSf06;
SCD4x co2_sensor(SCD4x_SENSOR_SCD41); //initialise SCD41 CO2 sensor
SCD4x co2_2_sensor(SCD4x_SENSOR_SCD41); //initialise SCD41 CO2 sensor

// Déclarer les tableaux pour les noms des capteurs, leurs valeurs et leurs décimales
String names[] = {"Vbatt", "tempSHT", "humSHT", "tempBMP", "pressBMP", "CO2_1", "CO2_2","debitSFM","velSFM","tempSFM"}; // Mise à jour si vous ajoutez des capteurs !
const int nb_values = sizeof(names) / sizeof(names[0]);
float values[nb_values];
int decimals[] = {2, 3, 1, 2, 2, 2, 2, 6, 6, 3}; // Nombre de décimales pour chaque capteur

// Retourne le pointeur du tableau des noms
String* Sensors::get_names() { 
  return names;
}

// Retourne le nombre de valeurs
int Sensors::get_nb_values() { 
  return nb_values;
}

// Retourne l'en-tête du fichier au format "<nom capteur 1>;<nom capteur 2>;"
String Sensors::getFileHeader () { 
  String header_string = "";
  for (int i = 0; i < nb_values; i++) {
    header_string = header_string + names[i] + ";";
  }
  return header_string; 
}

// Retourne les données des capteurs formatées pour être écrites dans un fichier CSV
// Format : "<valeur capteur 1>;<valeur capteur 2>;"
String Sensors::getFileData () { 
  String datastring = "";
  for (int i = 0; i < nb_values; i++) {
    datastring = datastring + String(values[i], decimals[i]) + ";";
  }
  return datastring;
}

// Retourne une chaîne contenant les noms des capteurs et leurs valeurs pour l'affichage
// Affiche également la chaîne dans le Serial
String Sensors::serialPrint() { // Affiche les mesures des capteurs pour le débogage
  String sensor_display_str = "";
  for (int i = 0; i < nb_values; i++) {
    sensor_display_str = sensor_display_str + names[i] + ": " + String(values[i],2) + "\n";
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
  delay(100);
  //connect and read CO2 sensor 1
  tcaselect(7);
  delay(5);
  co2_sensor.begin();
  for(int i = 0; i<4;i++){
    co2_sensor.measureSingleShot();
    delay(1500); //delay needed specified in the datasheet for single shot measurements
    co2_sensor.readMeasurement();
    values[5]=co2_sensor.getCO2();
    Serial.print("CO2:");
    Serial.println(values[5]); //only last value printed is stored
  }
  delay(100);
    //connect and read CO2 sensor 2
  tcaselect(6);
  delay(5);
  co2_2_sensor.begin();
  for(int i = 0; i<4;i++){
    co2_2_sensor.measureSingleShot();
    delay(1500); //delay needed specified in the datasheet for single shot measurements
    co2_2_sensor.readMeasurement();
    values[6]=co2_2_sensor.getCO2();
    Serial.print("CO2_2:");
    Serial.println(values[6]); //only last value printed is stored
  }
  delay(100);
  //connect and read SFM3003
  //SFM 3003
  tcaselect(5);
  delay(100);
  sfmSf06.begin(Wire, SFM3003_I2C_ADDR_2D);
  delay(100);
  //sfmSf06.stopContinuousMeasurement();
  //delay(100);
  sfmSf06.startO2ContinuousMeasurement();
  delay(200);
  int16_t temperatureRaw = 0;
  uint16_t status = 0;
  float SFMmoyenne = 0;
  int16_t flowRaw=0;
  
  delay(100);
  sfmSf06.readMeasurementDataRaw(flowRaw, temperatureRaw, status);
  delay(200);
  values[7]=(float(flowRaw)+12288)/120;
  //SFMmoyenne +=flowcorr;
  if (values[7]>=0){
    values[8]=(0.0034*pow(values[7],3)-0.0559*pow(values[7],2)+0.4035*values[7]);
  }
  else{
    values[8]=-1*(0.0034*pow(abs(values[7]),3)-0.0559*pow(values[7],2)+0.4035*abs(values[7]));
  }
  values[9] = float(temperatureRaw)/200;
  
  delay(50);
  tcaselect(2);
  delay(50);
  //put here the measurement of other sensors!!!
}

// multiplex bus selection for the first multiplexer 
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}








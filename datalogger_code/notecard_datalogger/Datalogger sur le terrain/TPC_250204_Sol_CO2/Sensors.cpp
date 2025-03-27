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
#include "Adafruit_ADS1X15.h"
#include "SparkFun_SCD4x_Arduino_Library.h" 
#include <SparkFun_PHT_MS8607_Arduino_Library.h>


//parameters which depend on the PCB version
#define I2C_MUX_ADDRESS 0x73 //I2C adress of the multiplexer set on the PCB
#define BMP581_sensor_ADRESS 0x46 //I2C adress of the BMP581 set on the PCB
#define SHT35_sensor_ADRESS 0x44 //I2C adress of the SHT35 set on the PCB

//for adc converter for O2
#define ADC_MAX 32768 //ADC 16 bits mais 1 bit de signe -> 2^15 = 32768
#define V_REF 2.048 // 2x gain   +/- 2.048V -> V_REF = 2.048


//create an instance of the sensors' classes
TinyPICO tiny = TinyPICO();
SHT31 sht;
BMP581 bmp;
SCD4x co2_sensor(SCD4x_SENSOR_SCD41); //initialise SCD41 CO2 sensor in soil
Adafruit_ADS1115 ads1115; //initialise the O2 sensor in SOIL
MS8607 barometricSensorSOIL; //initialise MS8607 sensor in soil


// Déclarer les tableaux pour les noms des capteurs, leurs valeurs et leurs décimales
String names[] = {"Vbatt", "tempSHT", "humSHT", "tempBMP", "pressBMP", "T_soil", "P_soil", "H_soil", "CO2", "T_CO2", "H_CO2", "Volt_O2", "O2"}; // Mise à jour si vous ajoutez des capteurs !
const int nb_values = sizeof(names) / sizeof(names[0]);
float values[nb_values];
int decimals[] = {2, 3, 1, 2, 1, 3, 2,2,2,2,2,4,3}; // Nombre de décimales pour chaque capteur

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
  delay(30); //wait 3ms for the multiplexer to switch
  sht.begin(); 
  sht.read();
  values[1]=sht.getTemperature(); //tempSHT
  values[2]=sht.getHumidity(); //humSHT

  //connect and start the BMP581 PCB sensor 
  tcaselect(2);
  delay(30);
  bmp.beginI2C(BMP581_sensor_ADRESS);
  delay(100);
  bmp5_sensor_data data = {0,0};
  int8_t err = bmp.getSensorData(&data);
  values[3]=data.temperature; //tempBMP
  values[4]=data.pressure/100; //pressBMP (in millibar)

    //connect MS8067 and CO2 sensors in SOIL
  delay(300);
  tcaselect(5);
  delay(300);

  //read MS8067 sensor in Soil
  barometricSensorSOIL.begin();
  //read values from the 3 sensors on the MS8067 in SOIL
  //values[5] = 5;
  //values[6] = 6;
  //values[7] = 7;
  values[5] = barometricSensorSOIL.getTemperature();
  values[6] = barometricSensorSOIL.getPressure();
  values[7] = barometricSensorSOIL.getHumidity();
  delay(300); //wait to be done with measurements before going to next sensor
    //measure values from CO2 sensor
  co2_sensor.begin(false, false, false);
  co2_sensor.measureSingleShot();
  delay(10000); //this sensor needs at least 4-10 seconds to measure something good (heating time
  //values[8]=8;
  //values[9]=9;
 // values[10]=10;
  co2_sensor.readMeasurement();
  values[8]=co2_sensor.getCO2();
  values[9]=co2_sensor.getTemperature();
  values[10]=co2_sensor.getHumidity();
  delay(100);

  //measure the voltage of the O2 sensor (read from the ADC converter)
  ads1115.setGain(GAIN_TWO);// 2x gain   +/- 2.048V -> V_REF = 2.048
  ads1115.begin();
  delay(1000);
  float adcValue=ads1115.readADC_SingleEnded(0);
  delay(1000);
  values[11] = (adcValue/ADC_MAX)*V_REF;
  //values[11] = 11;

  //compute the percentage of O2 with some dark magic formulas (I hope it is calibrated...)
  float O2_voltage_tcomp=0.002864*values[5]+1.98112; //Voltage à
  //O2 sensor is the SGX-40X
  //0% -> 0V    18.4% -> 1.2V
  values[12] = (values[11]*0.2095/O2_voltage_tcomp)*100; //remplacé 0.184 par 0.2095 car on ne fait plus compensation altitude
  //values[12] =12;

  delay(300);
  tcaselect(2);
  delay(300);
  //put here the measurement of other sensors!!!
}

// multiplex bus selection for the first multiplexer 
void Sensors::tcaselect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(I2C_MUX_ADDRESS);
  Wire.write(1 << i);
  Wire.endTransmission();
}







